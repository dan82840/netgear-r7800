#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>

#define FIRST_TIME 1
#define SECOND_TIME 2

#define MAX_NODES  255
#define NAME_SIZE 32
#define MAX_FLOWS_NUM 100
#define DELETE_LAST_CHAR(str) str[strlen(str) - 1] = 0
#define GENERAL_TRAFFIC "General Traffic" 
#define PING_DETECT_DEVICE "/tmp/netscan/ping_arp_output"

#define DEBUG		0

#if DEBUG
# define debug_printf(str, args...)  do {printf(str, ## args);} while(0)
#else
# define debug_printf(str, args...) do {;} while(0)
#endif

typedef unsigned long long uint64;


struct sb_flows_info {
	char name[NAME_SIZE+1];
	char map_name[NAME_SIZE+1];
	int uid;
	int time;

	uint64 up1;
	uint64 up2;
	uint64 down1;
	uint64 down2;

	uint64 epoch1;
	uint64 epoch2;

	float uprate;
	float downrate;

	int same_type;
};

struct multi_flows{
	struct sb_flows_info sb_flows[MAX_FLOWS_NUM];
	float node_uprate;
	float node_downrate;
	int flows_number; 
};

int lock_file(int fd);
int unlock_file(int fd);
void call_flows_api(int time, char *mac, struct multi_flows *mfp);
void deal_data_all_device_apps_bandwidth(struct multi_flows *mfp);
void delete_first_time_flows(struct multi_flows *mfp);
void delete_same_type_flows(struct multi_flows *mfp);
int map_friendly_name(struct multi_flows *mfp);
void find_same_type_flows(struct multi_flows *mfp);
void compute_flows_rate(struct multi_flows *mfp);
void find_top5_plus_other_apps(char *mac, struct multi_flows *mfp);
void find_top5_nodes(char nodes_mac[][32], struct multi_flows *mfp);
void get_all_nodes_mac(char nodes_mac[][32]);
void get_single_nodes_apps_bandwidth(int time, char nodes_mac[][32], struct multi_flows *flows_ptr);
void no_interrupt_sleep(int sleep_time);

int nodes_number;

int lock_file(int fd)
{
	struct flock fl;

	fl.l_type   = F_WRLCK;
	fl.l_start  = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len    = 0;

	/*wait until the lock is avaliabe */
	return(fcntl(fd, F_SETLKW, &fl));
}

int unlock_file(int fd)
{
	struct flock fl;

	fl.l_type   = F_UNLCK;
	fl.l_start  = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len    = 0;

	return(fcntl(fd, F_SETLK, &fl));
}


void find_top5_nodes(char nodes_mac[][32], struct multi_flows *mfp) 
{
	int i, j, count, index;
	float max;
	struct multi_flows  swap, other; 
	FILE *fp;
	int fd;
	char mac[32];

	/* sorting based on uplink rate  begin */
	for(i = 0, count = 0; nodes_mac[i][0] != '\0' && i < MAX_NODES; i++){
		max = mfp[i].node_uprate;
		count++;
		for( j = i + 1, index = i; nodes_mac[j][0] != '\0' && j < MAX_NODES; j++){
			if(max < mfp[j].node_uprate){
				max = mfp[j].node_uprate; 
				index = j;
			}
		}
		if(index != i){
			swap = mfp[index];
			mfp[index] = mfp[i];
			mfp[i] = swap;
			strncpy(mac, nodes_mac[i], sizeof(mac));
			strncpy(nodes_mac[i], nodes_mac[index], sizeof(nodes_mac[i]));
			strncpy(nodes_mac[index], mac, sizeof(nodes_mac[i]));
		}
	}
	other.node_uprate = 0;
	for(i = count; nodes_mac[i][0] != '\0' && i < MAX_NODES; i++){
		other.node_uprate += mfp[i].node_uprate;
	}
	fp = fopen("/tmp/soap_gcdb_up", "w");
	if(fp){
		fd = fileno(fp);
		lock_file(fd);
		for(i = 0; i < count; i++){
			fprintf(fp, "%s %.2f\n", nodes_mac[i], mfp[i].node_uprate);
			debug_printf("%s %.2f\n", nodes_mac[i], mfp[i].node_uprate);
		}
		if(nodes_mac[count][0] != '\0'){
			fprintf(fp, "Other %.2f\n",  other.node_uprate);
			debug_printf("Other %.2f\n", other.node_uprate);
		}
		unlock_file(fd);
		fclose(fp);
	}
	/* sorting based on uplink rate  end */


	/* sorting based on downlink rate end */
	for(i = 0, count = 0; nodes_mac[i][0] != '\0' && i < MAX_NODES; i++){
		max = mfp[i].node_downrate;
		count++;
		for( j = i + 1, index = i; nodes_mac[j][0] != '\0' && j < MAX_NODES; j++){
			if(max < mfp[j].node_downrate){
				max = mfp[j].node_downrate; 
				index = j;
			}
		}
		if(index != i){
			swap = mfp[index];
			mfp[index] = mfp[i];
			mfp[i] = swap;
			strncpy(mac, nodes_mac[i], sizeof(mac));
			strncpy(nodes_mac[i], nodes_mac[index], sizeof(nodes_mac[i]));
			strncpy(nodes_mac[index], mac, sizeof(nodes_mac[i]));
		}
	}
	other.node_downrate = 0;
	for(i = count; nodes_mac[i][0] != '\0' && i < MAX_NODES; i++){
		other.node_downrate += mfp[i].node_downrate;
	}
	fp = fopen("/tmp/soap_gcdb_down", "w");
	if(fp){
		fd = fileno(fp);
		lock_file(fd);
		for(i = 0; i < count; i++){
			fprintf(fp, "%s %.2f\n", nodes_mac[i], mfp[i].node_downrate);
			debug_printf("%s %.2f\n", nodes_mac[i], mfp[i].node_downrate);
		}
		if(nodes_mac[count][0] != '\0'){
			fprintf(fp, "Other %.2f\n",  other.node_downrate);
			debug_printf("Other %.2f\n",  other.node_downrate);
		}
		unlock_file(fd);
		fclose(fp);
	}
	/* sorting based on downlink rate  end */
}
void compute_nodes_rate(struct multi_flows *mfp)
{
	int i;

	/* A node rate is the sum of flows rate on it */
	mfp->node_uprate   = 0;
	mfp->node_downrate = 0;
	for(i = 0; i < mfp->flows_number; i++){
		mfp->node_downrate += mfp->sb_flows[i].downrate; 
		mfp->node_uprate   += mfp->sb_flows[i].uprate; 
	}
}

void record_nodes_rate(char *mac, struct multi_flows *mfp) 
{
	FILE *fp;
	int fd;
	char file_name[128];

	snprintf(file_name, sizeof(file_name), "/tmp/soap_current_bandwidth_by_mac.%s", mac);
	fp = fopen(file_name, "w");
	if(fp){
		fd = fileno(fp);
		lock_file(fd);
		fprintf(fp, "%s %.2f %.2f", mac, mfp->node_uprate, mfp->node_downrate);
		unlock_file(fd);
		fclose(fp);
	}
}

void get_all_devices_apps_bandwith(int time,  struct multi_flows *mfp)
{
	call_flows_api(time, NULL, mfp);
}

void call_flows_api(int time, char *mac, struct multi_flows *mfp)
{
	int uid , i;
	FILE *fp;
	char tname[NAME_SIZE+1], api[64];

	setenv("REQUEST_METHOD", "GET", 1);

	if(!mac)
		snprintf(api, sizeof(api), "/cgi-bin/ozker/api/flows");
	else
		snprintf(api, sizeof(api), "/cgi-bin/ozker/api/flows?mac=%s", mac);
	setenv("REQUEST_URI", api, 1);

	fp = popen("/www/cgi-bin/ozker | sed \'1,2d\' | jq \'.flows[] | .uid, .name, .up_bytes, .down_bytes, .epoch\'", "r");
	if(fp){
		if(time == FIRST_TIME)
			mfp->flows_number = 0;
		while(1){
			if(fscanf(fp, "%d\n", &uid) != 1)
				break;
			debug_printf("uid: %d\n", uid);
			if(time == FIRST_TIME){
				mfp->sb_flows[mfp->flows_number].uid = uid;
				if(fgets(mfp->sb_flows[mfp->flows_number].name, NAME_SIZE + 1, fp) == NULL)
					break;

				DELETE_LAST_CHAR(mfp->sb_flows[mfp->flows_number].name);
				if (fscanf(fp, "%llu\n", &mfp->sb_flows[mfp->flows_number].up1) != 1) 
					break;
				if (fscanf(fp, "%llu\n", &mfp->sb_flows[mfp->flows_number].down1) != 1) 
					break;
				if (fscanf(fp, "%llu\n", &mfp->sb_flows[mfp->flows_number].epoch1) != 1) 
					break;

				mfp->sb_flows[mfp->flows_number].time = FIRST_TIME;
				mfp->sb_flows[mfp->flows_number].same_type = 0;
				mfp->flows_number++;
			}
			else{
				for(i = 0; i < mfp->flows_number; i++){
					if(mfp->sb_flows[i].uid == uid){
						if(fgets(tname, NAME_SIZE + 1, fp) == NULL)
							break;
						if (fscanf(fp, "%llu\n", &mfp->sb_flows[i].up2) != 1) 
							break;
						if (fscanf(fp, "%llu\n", &mfp->sb_flows[i].down2) != 1) 
							break;
						if (fscanf(fp, "%llu\n", &mfp->sb_flows[i].epoch2) != 1) 
							break;
						mfp->sb_flows[i].time = SECOND_TIME;	
						break;
					}
				}
			}
		}
		pclose(fp);
	}
}

void deal_data_all_device_apps_bandwidth(struct multi_flows *mfp)
{
	delete_first_time_flows(mfp);
	compute_flows_rate(mfp);
	map_friendly_name(mfp);
	find_same_type_flows(mfp);
	delete_same_type_flows(mfp);
	find_top5_plus_other_apps(NULL, mfp);
}

void delete_first_time_flows(struct multi_flows *mfp)
{
	int i, j;

	for(i = 0; i < mfp->flows_number; ){
		if(mfp->sb_flows[i].time == FIRST_TIME){
			for(j = i + 1; j < mfp->flows_number; j++)
				mfp->sb_flows[j-1] = mfp->sb_flows[j];
			mfp->flows_number--;
			continue;
		}
		i++;
	}
}

void compute_flows_rate(struct multi_flows *mfp)
{
	uint64 interval;
	int i;

	for(i = 0; i < mfp->flows_number; i++){
		interval = mfp->sb_flows[i].epoch2 - mfp->sb_flows[i].epoch1;

		mfp->sb_flows[i].uprate = 1.0 * 8 * (mfp->sb_flows[i].up2 - mfp->sb_flows[i].up1) / 1000000 / interval;
		debug_printf("%s mfp->sb_flows[%d].uprate:%.2f\n",mfp->sb_flows[i].name,  i, mfp->sb_flows[i].uprate);
		mfp->sb_flows[i].downrate = 1.0 * 8 * (mfp->sb_flows[i].down2 - mfp->sb_flows[i].down1) / 1000000 / interval;
		debug_printf("%s mfp->sb_flows[%d].downrate:%.2f\n", mfp->sb_flows[i].name, i, mfp->sb_flows[i].downrate);
	}	
}

void find_top5_plus_other_apps(char *mac, struct multi_flows *mfp)
{
	struct sb_flows_info swap, other; 
	int i, j, count, fd, index;
	float max;
	FILE *fp;
	char file_name[64];

	for(i = 0; i < mfp->flows_number; i++){
		if(strcmp(mfp->sb_flows[i].map_name, GENERAL_TRAFFIC) == 0){
			swap = mfp->sb_flows[mfp->flows_number-1];
			mfp->sb_flows[mfp->flows_number-1] = mfp->sb_flows[i];
			mfp->sb_flows[i]= swap;
			break;
		}
	}

	/* sorting based on uplink rate  begin */
	for(i = 0, count = 0; count < 5 && i < mfp->flows_number - 1; i++){
		max = mfp->sb_flows[i].uprate;
		count++;
		for( j = i + 1, index = i; j < mfp->flows_number - 1; j++){
			if(max < mfp->sb_flows[j].uprate){
				max = mfp->sb_flows[j].uprate; 
				index = j;
			}
		}
		if(index != i){
			swap = mfp->sb_flows[index];
			mfp->sb_flows[index] = mfp->sb_flows[i];
			mfp->sb_flows[i] = swap;
		}
	}
	other.uprate = 0;
	for(i = count; i < mfp->flows_number; i++){
		other.uprate += mfp->sb_flows[i].uprate;
	}
	if(!mac)
		fp = fopen("/tmp/all_app_up_bandwidth", "w");
	else{
		snprintf(file_name, sizeof(file_name), "/tmp/single_device_app_up_bandwidth.%s", mac);
		fp = fopen(file_name, "w");
	}
	if(fp){
		fd = fileno(fp);
		lock_file(fd);
		for(i = 0; i < count; i++)
			fprintf(fp, "%s*%.2f\n", mfp->sb_flows[i].map_name, mfp->sb_flows[i].uprate);
		fprintf(fp, "Other*%.2f\n",  other.uprate);
		unlock_file(fd);
		fclose(fp);
	}
	/* sorting based on uplink rate  end */


	/* sorting based on downlink rate  begin */
	for(i = 0, count = 0; count < 5 && i < mfp->flows_number - 1; i++){
		max = mfp->sb_flows[i].downrate;
		count++;
		for( j = i + 1, index = i; j < mfp->flows_number - 1; j++){
			if(max < mfp->sb_flows[j].downrate){
				max = mfp->sb_flows[j].downrate; 
				index = j;
			}
		}
		if(index != i){
			swap = mfp->sb_flows[index];
			mfp->sb_flows[index] = mfp->sb_flows[i];
			mfp->sb_flows[i] = swap;
		}
	}
	other.downrate = 0;
	for(i = count; i < mfp->flows_number; i++){
		other.downrate += mfp->sb_flows[i].downrate;
	}
	if(!mac)
		fp = fopen("/tmp/all_app_down_bandwidth", "w");
	else{
		snprintf(file_name, sizeof(file_name), "/tmp/single_device_app_down_bandwidth.%s", mac);
		fp = fopen(file_name, "w");
	}
	if(fp){
		fd = fileno(fp);
		lock_file(fd);
		for(i = 0; i < count; i++){
			fprintf(fp, "%s*%.2f\n", mfp->sb_flows[i].map_name, mfp->sb_flows[i].downrate);
			debug_printf("%s*%.2f\n", mfp->sb_flows[i].map_name, mfp->sb_flows[i].downrate);
		}
		fprintf(fp, "Other*%.2f\n",  other.downrate);
		unlock_file(fd);
		fclose(fp);
	}
	/* sorting based on downlink rate  end */
}

int map_friendly_name(struct multi_flows *mfp)
{

	    int fd;
		int len;
		int i;
	    void *start;
	    struct stat sb;
		char *p1, *p2 = NULL;

#define FILE_NAME "/www/flows_EN-US.json"

	    fd = open(FILE_NAME, O_RDONLY); 
		if(fd < 0){
			debug_printf("open file:%s", FILE_NAME);
			return -1; 
		}
	    fstat(fd, &sb);
		start = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	    if(start == MAP_FAILED)
		        return -1;
		for(i = 0; i < mfp->flows_number; i++){
			debug_printf("flow name:%s\n", mfp->sb_flows[i].name);
			p1 = strstr(start, mfp->sb_flows[i].name);
			if(p1 != NULL){
				p1 = p1 + strlen(mfp->sb_flows[i].name) + 5;
				for(len = 0, p2 = p1; p2 != '\0'; p2++, len++)
					if(*p2 == '"')
						break;
				strncpy(mfp->sb_flows[i].map_name, p1, len);
				mfp->sb_flows[i].map_name[len] = '\0';
			}
			else{
				mfp->sb_flows[i].name[0] = toupper(mfp->sb_flows[i].name[0]);
				strncpy(mfp->sb_flows[i].map_name, mfp->sb_flows[i].name, NAME_SIZE + 1);
			}
		}
		munmap(start, sb.st_size);
		close(fd);
		return 0;
}

void find_same_type_flows(struct multi_flows *mfp)
{
	int i, j;

	for(i = 0; i < mfp->flows_number; i++){
		if(mfp->sb_flows[i].same_type)
			continue;
		for(j = i + 1; j < mfp->flows_number; j++){
			if(!strncmp(mfp->sb_flows[i].map_name, mfp->sb_flows[j].map_name, NAME_SIZE + 1) && !mfp->sb_flows[j].same_type){
				mfp->sb_flows[i].uprate += mfp->sb_flows[j].uprate;
				mfp->sb_flows[i].downrate += mfp->sb_flows[j].downrate;
				mfp->sb_flows[j].same_type = 1;
			}
		}
	}
}

void delete_same_type_flows(struct multi_flows *mfp)
{
	int i, j;

	for(i = 0; i < mfp->flows_number; ){
		if(mfp->sb_flows[i].same_type){
			for(j = i + 1; j < mfp->flows_number; j++)
				mfp->sb_flows[j-1] = mfp->sb_flows[j];
			mfp->flows_number--;
			continue;
		}
		i++;
	}
}

void get_apps_bandwidth_by_mac(int time, char nodes_mac[][32], struct multi_flows *flows_ptr)
{
	if(time == FIRST_TIME)
		get_all_nodes_mac(nodes_mac);

	get_single_nodes_apps_bandwidth(time, nodes_mac, flows_ptr);
}

void get_all_nodes_mac(char nodes_mac[][32])
{
	int i;
	FILE *fp;

	for(i = 0; i <MAX_NODES; i++)
		memset(nodes_mac[i], 0, sizeof(nodes_mac[i]));

	fp = fopen(PING_DETECT_DEVICE, "r");
	if(fp){
		i = 0;
		while(1){
				if(fgets(nodes_mac[i], sizeof(nodes_mac[i]), fp) == NULL)
						break;
				debug_printf("### mac:%s\n", nodes_mac[i]);
				DELETE_LAST_CHAR(nodes_mac[i]);
				i++;
			}
		fclose(fp);
	}
}

void get_single_nodes_apps_bandwidth(int time, char nodes_mac[][32], struct multi_flows *flows_ptr)
{
	int i;

	for(i = 0; nodes_mac[i][0] != '\0' && i < MAX_NODES; i++){
		call_flows_api(time, nodes_mac[i], &flows_ptr[i]);
	}
}

void deal_data_apps_bandwidth_by_mac(char nodes_mac[][32], struct multi_flows *flows_ptr)
{
	int i;

	for(i = 0; nodes_mac[i][0] != '\0' && i < MAX_NODES; i++){
		delete_first_time_flows(&flows_ptr[i]);
		compute_flows_rate(&flows_ptr[i]);
		map_friendly_name(&flows_ptr[i]);
		find_same_type_flows(&flows_ptr[i]);
		delete_same_type_flows(&flows_ptr[i]);
		compute_nodes_rate(&flows_ptr[i]);
		record_nodes_rate(nodes_mac[i], &flows_ptr[i]);
		find_top5_plus_other_apps(nodes_mac[i], &flows_ptr[i]);
	}
	find_top5_nodes(nodes_mac, flows_ptr);
}

int loop_count = 12; /*12 * 5s = 1min */

void signal_handler(int sig)
{
	if(sig == SIGUSR1)
		loop_count = 12;
}

void no_interrupt_sleep(int sleep_time)
{
	int remain = sleep_time;

	while(remain){
		remain = sleep(remain);
	}
}


int main(int argc ,char **argv)
{
	struct multi_flows all_device_flows; /* for GetCurrentAppBandwidth */
	struct multi_flows single_devcie_flows[MAX_NODES]; /* for GetCurrentAppBandwidthByMAC */
	char nodes_mac[MAX_NODES][32];
	struct sigaction sa;
	int first_run = 1;
	

//	daemon(1,1);
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signal_handler;
	sigaction(SIGUSR1, &sa, NULL);
	system("ping_arp &");
	while(loop_count--){
		get_all_devices_apps_bandwith(FIRST_TIME, &all_device_flows);
		get_apps_bandwidth_by_mac(FIRST_TIME, nodes_mac, single_devcie_flows);
	
		if(first_run){
			first_run = 0;
			loop_count++;
			no_interrupt_sleep(1);
		}
		else
			no_interrupt_sleep(5);

		get_all_devices_apps_bandwith(SECOND_TIME, &all_device_flows);
		get_apps_bandwidth_by_mac(SECOND_TIME, nodes_mac, single_devcie_flows);

		deal_data_all_device_apps_bandwidth(&all_device_flows);
		deal_data_apps_bandwidth_by_mac(nodes_mac, single_devcie_flows);
	}

	return 0;
}
