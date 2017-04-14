#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/ether.h>
#include <ctype.h>

#define SET_PRIORITY_FORMAT "\"{\\\"Pipeline\\\":{\\\"name\\\":\\\"Unknown\\\",\\\"down_limit\\\":0,\\\"down\\\":0,\\\"detection_finished\\\":false,\\\"up_limit\\\":0,\\\"ip_addr\\\":\\\"0.0.0.0\\\",\\\"up\\\":0,\\\"OS_flavor\\\":\\\"Unknown\\\",\\\"future_up\\\":0,\\\"epoch\\\":0,\\\"uid\\\":\\\"%s\\\",\\\"type\\\":\\\"Unknown\\\",\\\"future_down\\\":0,\\\"epoch_last_change\\\":0,\\\"mac_addr\\\":\\\"%s\\\",\\\"HTTP_clients\\\":[{\\\"flavor\\\":\\\"Unknown\\\",\\\"type\\\":\\\"Unknown\\\"}],\\\"OS_type\\\":\\\"Unknown\\\"},\\\"UI\\\":{\\\"priority\\\":%d}}\""
 
int main(int argc, char** argv)
{
	char cmd[2048];
	char *mac;
	int i = 0, priority;

	if (argc != 3) {
		printf("Usage: %s mac_of_dev priority_of_dev\n", argv[0]);
		return 1;
	}

	mac = argv[1];
	if (ether_aton(mac) == NULL) {
		printf("invalid mac address %s\n", mac);
		return 1;
	}

	while(mac[i] != '\0'){
		mac[i] = tolower(mac[i]);
		i++;
	}

	priority = atoi(argv[2]);
	if (priority < 1 || priority > 255 ) {
		printf("invalid priority, it should be a number between 1 ~ 255\n");
		return 1;
	}

	sprintf(cmd, "curl http://127.0.0.1/cgi-bin/ozker/api/nodes 2>/dev/null | grep %s > /dev/null", mac);
	if (system(cmd) != 0) {
		printf("don't find device with mac %s in streamboost nodes\n", mac);
		return 1;
	}

	sprintf(cmd, "curl http://127.0.0.1/cgi-bin/ozker/api/nodes -X PUT -d "SET_PRIORITY_FORMAT" >/dev/null 2>&1", mac, mac, priority);
	system(cmd);
	return 0;
}
