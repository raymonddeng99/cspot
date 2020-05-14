#ifndef RAFT_H
#define RAFT_H

#include <sys/time.h>
#include <pthread.h>
#include <stdio.h>
#include "woofc.h"

#define RAFT_LEADER 0
#define RAFT_FOLLOWER 1
#define RAFT_CANDIDATE 2
#define RAFT_OBSERVER 3
#define RAFT_SHUTDOWN 4
#define RAFT_CONFIG_STABLE 0
#define RAFT_CONFIG_JOINT 1
#define RAFT_CONFIG_NEW 2
#define RAFT_LOG_ENTRIES_WOOF "log_entries.woof"
#define RAFT_SERVER_STATE_WOOF "server_state.woof"
#define RAFT_HEARTBEAT_WOOF "heartbeat.woof"
#define RAFT_TIMEOUT_CHECKER_WOOF "timeout_checker.woof"
#define RAFT_APPEND_ENTRIES_ARG_WOOF "append_entries_arg.woof"
#define RAFT_APPEND_ENTRIES_RESULT_WOOF "append_entries_result.woof"
#define RAFT_CLIENT_PUT_REQUEST_WOOF "client_put_request.woof"
#define RAFT_CLIENT_PUT_ARG_WOOF "client_put_arg.woof"
#define RAFT_CLIENT_PUT_RESULT_WOOF "client_put_result.woof"
#define RAFT_CONFIG_CHANGE_ARG_WOOF "config_change_arg.woof"
#define RAFT_CONFIG_CHANGE_RESULT_WOOF "config_change_result.woof"
#define RAFT_REPLICATE_ENTRIES_WOOF "replicate_entries.woof"
#define RAFT_REQUEST_VOTE_ARG_WOOF "request_vote_arg.woof"
#define RAFT_REQUEST_VOTE_RESULT_WOOF "request_vote_result.woof"
#define RAFT_MONITOR_NAME "raft"

#define RAFT_WOOF_HISTORY_SIZE 65536
#define RAFT_NAME_LENGTH WOOFNAMESIZE
#define RAFT_MAX_MEMBERS 16
#define RAFT_MAX_OBSERVERS 4
#define RAFT_MAX_ENTRIES_PER_REQUEST 16
#define RAFT_DATA_TYPE_SIZE 1024
#define RAFT_TIMEOUT_MIN 500
#define RAFT_TIMEOUT_MAX 1000
#define RAFT_HEARTBEAT_RATE (RAFT_TIMEOUT_MIN / 5)
#define RAFT_TIMEOUT_CHECKER_DELAY (RAFT_TIMEOUT_MIN / 5)
#define RAFT_REPLICATE_ENTRIES_DELAY 20
#define RAFT_CLIENT_PUT_DELAY 50

#define RAFT_SAMPLING_RATE 10 // number of entries per sample
#define LOG_DEBUG 0
#define LOG_INFO 1
#define LOG_WARN 2
#define LOG_ERROR 3

char RAFT_WOOF_TO_CREATE[][RAFT_NAME_LENGTH] = {RAFT_LOG_ENTRIES_WOOF, RAFT_SERVER_STATE_WOOF, RAFT_HEARTBEAT_WOOF, RAFT_TIMEOUT_CHECKER_WOOF,
	RAFT_APPEND_ENTRIES_ARG_WOOF, RAFT_APPEND_ENTRIES_RESULT_WOOF,
	RAFT_CLIENT_PUT_REQUEST_WOOF, RAFT_CLIENT_PUT_ARG_WOOF, RAFT_CLIENT_PUT_RESULT_WOOF,
	RAFT_CONFIG_CHANGE_ARG_WOOF, RAFT_CONFIG_CHANGE_RESULT_WOOF, RAFT_REPLICATE_ENTRIES_WOOF,
	RAFT_REQUEST_VOTE_ARG_WOOF, RAFT_REQUEST_VOTE_RESULT_WOOF};

unsigned long RAFT_WOOF_ELEMENT_SIZE[] = {sizeof(RAFT_LOG_ENTRY), sizeof(RAFT_SERVER_STATE), sizeof(RAFT_HEARTBEAT), sizeof(RAFT_TIMEOUT_CHECKER_ARG),
	sizeof(RAFT_APPEND_ENTRIES_ARG), sizeof(RAFT_APPEND_ENTRIES_RESULT),
	sizeof(RAFT_CLIENT_PUT_REQUEST), sizeof(RAFT_CLIENT_PUT_ARG), sizeof(RAFT_CLIENT_PUT_RESULT),
	sizeof(RAFT_CONFIG_CHANGE_ARG), sizeof(RAFT_CONFIG_CHANGE_RESULT), sizeof(RAFT_REPLICATE_ENTRIES_ARG),
	sizeof(RAFT_REQUEST_VOTE_ARG), sizeof(RAFT_REQUEST_VOTE_RESULT)};

typedef struct data_type {
	char val[RAFT_DATA_TYPE_SIZE];
} RAFT_DATA_TYPE;

typedef struct raft_log_entry {
	unsigned long term;
	RAFT_DATA_TYPE data;
	int is_config;
} RAFT_LOG_ENTRY;

typedef struct raft_server_state {
	char woof_name[RAFT_NAME_LENGTH];
	int role;
	unsigned long current_term;
	char voted_for[RAFT_NAME_LENGTH];
	char current_leader[RAFT_NAME_LENGTH];
	
	int members;
	int observers;
	char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH];
	int current_config;
	unsigned long last_config_seqno;

	unsigned long commit_index;
	unsigned long next_index[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS];
	unsigned long match_index[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS];
	unsigned long last_sent_index[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS];
	unsigned long last_sent_timestamp[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS];
} RAFT_SERVER_STATE;

typedef struct raft_heartbeat {
	unsigned long term;
	unsigned long timestamp;
} RAFT_HEARTBEAT;

typedef struct raft_timeout_checker_arg {
	unsigned long timeout_value;
} RAFT_TIMEOUT_CHECKER_ARG;

typedef struct raft_append_entries_arg {
	unsigned long term;
	char leader_woof[RAFT_NAME_LENGTH];
	unsigned long prev_log_index;
	unsigned long prev_log_term;
	RAFT_LOG_ENTRY entries[RAFT_MAX_ENTRIES_PER_REQUEST];
	unsigned long leader_commit;
	unsigned long created_ts;
} RAFT_APPEND_ENTRIES_ARG;

typedef struct raft_append_entries_result {
	char server_woof[RAFT_NAME_LENGTH];
	unsigned long term;
	int success;
	unsigned long last_entry_seq;
	unsigned long seqno;
	unsigned long request_created_ts;
} RAFT_APPEND_ENTRIES_RESULT;

typedef struct raft_client_put_request {
	RAFT_DATA_TYPE data;
	unsigned long created_ts;
} RAFT_CLIENT_PUT_REQUEST;

typedef struct raft_client_put_arg {
	unsigned long last_seqno;
} RAFT_CLIENT_PUT_ARG;

typedef struct raft_client_put_result {
	int redirected;
	unsigned long index;
	unsigned long term;
	char current_leader[RAFT_NAME_LENGTH];
} RAFT_CLIENT_PUT_RESULT;

typedef struct raft_config_change_arg {
	int members;
	char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH];

	int observe;
	char observer_woof[RAFT_NAME_LENGTH];
} RAFT_CONFIG_CHANGE_ARG;

typedef struct raft_config_change_result {
	int redirected;
	int success;
	char current_leader[RAFT_NAME_LENGTH];
} RAFT_CONFIG_CHANGE_RESULT;

typedef struct raft_replicate_entries {
	unsigned long term;
	unsigned long last_seen_result_seqno;
} RAFT_REPLICATE_ENTRIES_ARG;

typedef struct raft_request_vote_arg {
	unsigned long term;
	char candidate_woof[RAFT_NAME_LENGTH];
	unsigned long candidate_vote_pool_seqno;
	unsigned long last_log_index;
	unsigned long last_log_term;
	unsigned long created_ts;
} RAFT_REQUEST_VOTE_ARG;

typedef struct raft_request_vote_result {
	unsigned long term;
	int granted;
	unsigned long candidate_vote_pool_seqno;
	unsigned long request_created_ts;
} RAFT_REQUEST_VOTE_RESULT;

int get_server_state(RAFT_SERVER_STATE *server_state);
int random_timeout(unsigned long seed);
unsigned long get_milliseconds();
int read_config(FILE *fp, int *members, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]);
int node_woof_name(char *node_woof);
int member_id(char *woof_name, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]);
int encode_config(char *dst, int members, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]);
int decode_config(char *src, int *members, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]);
int compute_joint_config(int old_members, char old_member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH],
	int new_members, char new_member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH],
	int *joint_members, char joint_member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]);
int threads_join(int members, pthread_t *pids);
int threads_cancel(int members, pthread_t *pids);

char log_tag[1024];
void log_set_tag(const char *tag);
void log_set_level(int level);
void log_set_output(FILE *file);
void log_debug(const char *message, ...);
void log_info(const char *message, ...);
void log_warn(const char *message, ...);
void log_error(const char *message, ...);
	
int create_woofs();
int start_server(int members, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH], int observer);

#endif

