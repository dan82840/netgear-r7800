#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern char *config_get(char *name);
extern void config_set(char *name, char *value);
extern char *config_commit(void);

#define DELETE_LAST_CHAR(str) str[strlen(str) - 1] = 0
#define POLL_FLAG	"_"
#define MAX_TYPE 51
#define MIN_TYPE 1

struct device
{
	char mac[18];
	int type;
	struct device *next;
};

int priority_table[] = {
	20, /* AMAZON_KINDLE */
	20, /* ANDROID_DEVICE */
	30, /* ANDROID_PHONE */
	20, /* ANDROID_TABLET */
	30, /* APPLE_AIRPORT_EXPRESS */
	10, /* BLU_RAY_PLAYER */
	30, /* BRIDGE */
	10, /* CABLE_STB */
	40, /* CAMERA */
	30, /* ROUTER */
	10, /* DVR */
	10, /* GAMING_CONSOLE */
	20, /* IMAC */
	20, /* IPAD */
	20, /* IPAD_MINI */
	30, /* IPONE_5_5S_5C */
	30, /* IPHONE */
	30, /* IPOD_TOUCH */
	20, /* LINUX_PC */
	20, /* MAC_MINI */
	20, /* MAC_PRO */
	20, /* MAC_BOOK */
	10, /* MEDIA_DEVICE */
	30, /* NETWORK_DEVICE */
	10, /* OTHER_STB */
	30, /* POWERLINE */
	40, /* PRINTER */
	30, /* REPEATER */
	10, /* SATELLITE_STB */
	40, /* SCANNER */
	10, /* SLING_BOX */
	30, /* SMART_PHONE */
	40, /* STORAGE_NAS */
	30, /* SWITCH */
	10, /* TV */
	20, /* TABLET */
	20, /* UNIX_PC */
	20, /* WINDOWS_PC */
	20, /* SURFACE */
	30, /* WIFI_EXTERNDER */
	10, /* APPLE_TV */
	10, /* AV_RECEIVER */
	10, /* CHROMECAST */
	30, /* GOOGLE_NEXUS_5 */
	30, /* GOOGLE_NEXUS_7 */
	30, /* GOOGLE_NEXUS_10 */
	30, /* OTHERS */
	30, /* WN1000RP */
	30,  /* WN2500RP */
	10,  /*VoIP*/
	30   /*iPhone 6*/

};

struct device *init_user_list()
{
	char device_index[32], device_value[128]; 
	char *mac, *priority, *type;
	struct device *list, *cur, *p;
	int i;

	list = (struct device *)malloc(sizeof(struct device));
	if(list == NULL)
		return NULL;
	list->next = NULL;
	cur = list;

	for(i = 0; ; i++){
		sprintf(device_index, "device_list%d", i);
		strcpy(device_value, config_get(device_index));
		if(*device_value != '\0'){
			mac = strtok(device_value, " ");
			priority = strtok(NULL, " ");
			if(strcmp(priority, POLL_FLAG))
				continue;
			type = strtok(NULL, " ");

			p = (struct device *)malloc(sizeof(struct device));
			if(p == NULL)
				return NULL;
			strcpy(p->mac, mac);
			p->type = atoi(type);
			cur->next = p;
			cur = p;
			cur->next = NULL;
		}
		else{
			break;
		}
	}
	return list;
}

struct sb2netgear_priority_map_table{
	int  sb_priority;
	char *netgear_priority;
};

struct sb2netgear_priority_map_table priority_map_table[]={
	{10, "HIGHEST"},
	{20, "HIGH"   },
	{30, "MEDIUM" },
	{40, "LOW"    },
	{0 , NULL     }
};

int sb_priority_2_netgear_prioriry(int sb_priority, char *netgear_priority)
{
	int i;

	for(i = 0; priority_map_table[i].netgear_priority != NULL; i++){
		if(sb_priority == priority_map_table[i].sb_priority){
			strcpy(netgear_priority, priority_map_table[i].netgear_priority);
			return 0;
		}
	}
	return -1;
}

void update_device_list(struct device *p)
{
	char device_index[32], device_value[128]; 
	char *mac, *priority, *type, *name;
	int i;

	for(i = 0; ; i++){
		sprintf(device_index, "device_list%d", i);
		strcpy(device_value, config_get(device_index));
		if(*device_value != '\0'){
			mac = strtok(device_value, " ");
			if(strcmp(mac, p->mac))
				continue;
			priority = strtok(NULL, " ");
			type = strtok(NULL, " ");
			/* 
			 *  name can be NULL(means clear the custom name and use the default name)
			 *  or includes one or more SPACE characters
			 */
			name = type + strlen(type) + 1;
			char new_value[128], netgear_priority[8];
			if(sb_priority_2_netgear_prioriry(priority_table[p->type-1],
				netgear_priority) == -1)
					return;
			snprintf(new_value, sizeof(new_value), "%s %s %s %s", mac,
				   	netgear_priority, type, name);
			config_set(device_index, new_value);
			config_commit();
			break;
		}
		else{
			break;
		}
	}
}


struct device *init_sb_list()
{
	struct device *list;

	list = (struct device *)malloc(sizeof(struct device));
	if(list == NULL)
		return NULL;
	list->next = NULL;
	return list;
}

void remove_old_list(struct device *list)
{
	struct device *cur, *p;

	cur = list->next;
	while(cur){
		p = cur->next;
		free(cur);
		cur = p;
	}
}

void create_new_list(struct device *list)
{
	struct device *cur, *p;
	FILE *fp;
	char mac[19];

	setenv("REQUEST_METHOD", "GET", 1);
	setenv("REQUEST_URI", "/cgi-bin/ozker/api/nodes", 1);
	fp = popen("/www/cgi-bin/ozker | sed \'1,2d\' | jq \'.nodes[] | .Pipeline.mac_addr\'", "r");
	if(fp){
		cur = list;
		while(1){
				if(fgets(mac, sizeof(mac), fp) == NULL)
						break;
				DELETE_LAST_CHAR(mac);
				p = (struct device *)malloc(sizeof(struct device));
				if(p == NULL)
					return;
				strcpy(p->mac, mac);
				cur->next = p;
				cur = p;
				cur->next = NULL;
		}
		pclose(fp);
	}
}

void retrieve_sb_list(struct device *list)
{
	remove_old_list(list);
	create_new_list(list);
}

int exist_in_sb_list(struct device *user_device, struct device *sb_list)
{
	struct device *p;

	for(p = sb_list->next; p != NULL; p = p->next){
		if(strcasecmp(user_device->mac, p->mac) == 0)
			return 1;
	}
	return 0;
}


void set_priority(struct device *user_device)
{
	if(user_device->type > MAX_TYPE || user_device->type < MIN_TYPE)
		return;

	char cmd[128];
	snprintf(cmd, sizeof(cmd), "sb_set_priority %s %d", user_device->mac, 
			priority_table[user_device->type-1]);
	system(cmd);
}

int main(int argc, char **argv)
{
	struct device *user_list, *sb_list, *p, *q;

	user_list = init_user_list();
	if(user_list == NULL)
		exit(-1);
	else if(user_list->next == NULL)
		exit(0);

	sb_list = init_sb_list();
	if(sb_list == NULL)
		exit(-1);

	daemon(1, 1);
	while(1){
		retrieve_sb_list(sb_list);
		for(q = user_list, p = user_list->next; p != NULL; ){
			if(exist_in_sb_list(p, sb_list)){
				set_priority(p);
				update_device_list(p);
				q->next = p->next;
				free(p);
				p = q->next;
				if(user_list->next == NULL)
					exit(0);
				continue;
			}
			q = q->next;
		   	p = p->next;
		}
		sleep(5);
	}
	return 0;
}
