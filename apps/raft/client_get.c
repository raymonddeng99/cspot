#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "raft.h"
#include "raft_client.h"

#define ARGS "f:i:s:"
char *Usage = "client_get -f config\n\
choose one of the following:\n\
-i index\n\
-s client_put_seqno\n";

int main(int argc, char **argv) {
	char config[256];
	unsigned long index = 0;
	unsigned long client_put_seqno = 0;

	int c;
	while ((c = getopt(argc, argv, ARGS)) != EOF) {
		switch (c) {
			case 'f': {
				strncpy(config, optarg, sizeof(config));
				break;
			}
			case 'i': {
				index = strtoul(optarg, NULL, 0);
				break;
			}
			case 's': {
				client_put_seqno = strtoul(optarg, NULL, 0);
				break;
			}
			default: {
				fprintf(stderr, "Unrecognized command %c\n", (char)c);
				fprintf(stderr, "%s", Usage);
				exit(1);
			}
		}
	}

	if (config[0] == 0 || (index == 0 && client_put_seqno == 0) || (index != 0 && client_put_seqno != 0)) {
		fprintf(stderr, "%s", Usage);
		exit(1);
	}

	FILE *fp = fopen(config, "r");
	if (fp == NULL) {
		fprintf(stderr, "can't read config file\n");
		exit(1);
	}
	int members;
	char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH];
	if (read_config(fp, &members, member_woofs) < 0) {
		fprintf(stderr, "failed to read the config file\n");
		fclose(fp);	
		exit(1);
	}
	if (raft_init_client(members, member_woofs) < 0) {
		fprintf(stderr, "can't init client\n");
		fclose(fp);	
		exit(1);
	}
	fclose(fp);

	RAFT_DATA_TYPE data;
	unsigned long term;
	if (index > 0) {
		term = raft_sync_get(&data, index, 1);
	} else {
		term = raft_async_get(&data, client_put_seqno);
	}
	if (raft_is_error(term)) {
		fprintf(stderr, "failed to get data: %s\n", raft_error_msg);
		exit(1);
	}
	printf("%s\n", data.val);

	return 0;
}

