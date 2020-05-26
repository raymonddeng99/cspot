#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>
#include "woofc.h"
#include "dht.h"
#include "dht_utils.h"

int h_fix_finger_callback(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_FIX_FINGER_CALLBACK_ARG *arg = (DHT_FIX_FINGER_CALLBACK_ARG *)ptr;

	log_set_tag("fix_finger_callback");
	// log_set_level(DHT_LOG_DEBUG);
	log_set_level(DHT_LOG_INFO);
	log_set_output(stdout);

	char woof_name[DHT_NAME_LENGTH];
	if (node_woof_name(woof_name) < 0) {
		log_error("failed to get local node's woof name");
		exit(1);
	}

	DHT_TABLE dht_table = {0};
	if (get_latest_element(DHT_TABLE_WOOF, &dht_table) < 0) {
		log_error("couldn't get latest dht_table: %s", dht_error_msg);
		exit(1);
	}
	log_debug("find_successor leader: %s(%d)", arg->node_replicas[arg->node_leader], arg->node_leader);

	// finger[i] = find_successor(x);
	int i = arg->finger_index;
	// sprintf(msg, "current finger_addr[%d] = %s, %s", i, dht_table.finger_addr[i], result->node_addr);
	// log_info("fix_fingers_callback", msg);
	if (memcmp(dht_table.finger_hash[i], arg->node_hash, SHA_DIGEST_LENGTH) == 0) {
		log_debug("no need to update finger_addr[%d]", i);
		return 1;
	}
	
	if (hmap_set_replicas(arg->node_hash, arg->node_replicas, arg->node_leader) < 0) {
		log_error("failed to put finger's replicas in hashmap: %s", dht_error_msg);
		exit(1);
	}
	memcpy(dht_table.finger_hash[i], arg->node_hash, sizeof(dht_table.finger_hash[i]));
	unsigned long seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_table);
	if (WooFInvalid(seq)) {
		log_error("failed to update finger[%d]", i);
		exit(1);
	}

	char hash_str[2 * SHA_DIGEST_LENGTH + 1];
	print_node_hash(hash_str, dht_table.finger_hash[i]);
	log_debug("updated finger_addr[%d] = %s(%s)", i, arg->node_replicas[arg->node_leader], hash_str);

	return 1;
}
