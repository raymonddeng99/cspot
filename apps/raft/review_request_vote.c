#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "raft.h"

char function_tag[] = "review_request_vote";

int review_request_vote(WOOF *wf, unsigned long seq_no, void *ptr)
{
	RAFT_FUNCTION_LOOP *function_loop = (RAFT_FUNCTION_LOOP *)ptr;
	RAFT_REVIEW_REQUEST_VOTE *reviewing = &(function_loop->review_request_vote);

	log_set_level(LOG_INFO);
	log_set_level(LOG_DEBUG);
	log_set_output(stdout);

	// get the server's current term
	unsigned long last_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
	if (WooFInvalid(last_server_state)) {
		sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_SERVER_STATE_WOOF);
		log_error(function_tag, log_msg);
		exit(1);
	}
	RAFT_SERVER_STATE server_state;
	int err = WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, last_server_state);
	if (err < 0) {
		log_error(function_tag, "couldn't get the server state");
		exit(1);
	}

	unsigned long latest_vote_request = WooFGetLatestSeqno(RAFT_REQUEST_VOTE_ARG_WOOF);
	if (WooFInvalid(latest_vote_request)) {
		sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_REQUEST_VOTE_ARG_WOOF);
		log_error(function_tag, log_msg);
		exit(1);
	}
	
	unsigned long i;
	RAFT_REQUEST_VOTE_ARG request;
	RAFT_REQUEST_VOTE_RESULT result;
	for (i = reviewing->last_request_seqno + 1; i <= latest_vote_request; ++i) {
		// read the request
		err = WooFGet(RAFT_REQUEST_VOTE_ARG_WOOF, &request, i);
		if (err < 0) {
			sprintf(log_msg, "couldn't get the request at %lu", i);
			log_error(function_tag, log_msg);
			exit(1);
		}

		result.candidate_vote_pool_seqno = request.candidate_vote_pool_seqno;
		if (request.term < server_state.current_term) { // current term is higher than the request
			result.term = server_state.current_term; // server term will always be greater than reviewing term
			result.granted = false;
			sprintf(log_msg, "rejected vote from lower term %lu at term %lu", request.term, server_state.current_term);
			log_debug(function_tag, log_msg);
		} else {
			if (request.term > server_state.current_term) {
				// fallback to follower
				RAFT_TERM_ENTRY new_term;
				new_term.term = request.term;
				new_term.role = RAFT_FOLLOWER;
				unsigned long seq = WooFPut(RAFT_TERM_ENTRIES_WOOF, NULL, &new_term);
				if (WooFInvalid(seq)) {
					log_error(function_tag, "couldn't queue the new term request to chair");
					exit(1);
				}
				sprintf(log_msg, "request term %lu is higher than the current term %lu, fall back to follower", request.term, server_state.current_term);
				log_debug(function_tag, log_msg);

				reviewing->last_request_seqno = i - 1;
				sprintf(function_loop->next_invoking, "term_chair");
				seq = WooFPut(RAFT_FUNCTION_LOOP_WOOF, "term_chair", function_loop);
				if (WooFInvalid(seq)) {
					log_error(function_tag, "couldn't queue the next function_loop: term_chair");
					exit(1);
				}
				return 1;
			}
			
			result.term = request.term;
			if (reviewing->voted_for[0] == 0 || strcmp(reviewing->voted_for, request.candidate_woof) == 0) {
				// check if the log is up-to-date ($5.4)
				// we don't need to worry about append_entries running in parallel
				// because if we're receiveing append_entries request, it means the majority has voted and this vote doesn't matter
				unsigned long latest_log_entry = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);	
				if (WooFInvalid(latest_log_entry)) {
					sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_LOG_ENTRIES_WOOF);
					log_error(function_tag, log_msg);
					exit(1);
				}
				RAFT_LOG_ENTRY last_log_entry;
				if (latest_log_entry > 0) {
					int err = WooFGet(RAFT_LOG_ENTRIES_WOOF, &last_log_entry, latest_log_entry);
					if (err < 0) {
						sprintf(log_msg, "couldn't get the latest log entry %lu from %s", latest_log_entry, RAFT_LOG_ENTRIES_WOOF);
						log_error(function_tag, log_msg);
						exit(1);
					}
				}
				if (latest_log_entry > 0 && last_log_entry.term > request.last_log_term) {
					// the server has more up-to-dated entries than the candidate
					result.granted = false;
					sprintf(log_msg, "rejected vote from server with outdated log (last entry at term %lu)", request.last_log_term);
					log_debug(function_tag, log_msg);
				} else if (latest_log_entry > 0 && last_log_entry.term == request.last_log_term && latest_log_entry > request.last_log_index) {
					// both have same term but the server has more entries
					result.granted = false;
					sprintf(log_msg, "rejected vote from server with outdated log (last entry at index %lu)", request.last_log_index);
					log_debug(function_tag, log_msg);
				} else {
					// the candidate has more up-to-dated log entries
					memcpy(reviewing->voted_for, request.candidate_woof, RAFT_WOOF_NAME_LENGTH);
					result.granted = true;
					sprintf(log_msg, "granted vote at term %lu", server_state.current_term);
					log_debug(function_tag, log_msg);
				}
			} else {
				result.granted = false;
			}
		}
		// return the request
		char candidate_result_woof[RAFT_WOOF_NAME_LENGTH];
		sprintf(candidate_result_woof, "%s/%s", request.candidate_woof, RAFT_REQUEST_VOTE_RESULT_WOOF);
		if (result.granted == true) {
			unsigned long seq = WooFPut(candidate_result_woof, "count_vote", &result);
			if (WooFInvalid(seq)) {
				sprintf(log_msg, "couldn't return the vote result to %s", candidate_result_woof);
				log_error(function_tag, log_msg);
				exit(1);
			}
		} else {
			unsigned long seq = WooFPut(candidate_result_woof, NULL, &result);
			if (WooFInvalid(seq)) {
				sprintf(log_msg, "couldn't return the vote result to %s", candidate_result_woof);
				log_error(function_tag, log_msg);
				exit(1);
			}
		}
		reviewing->last_request_seqno = i;
	}
	// queue the next review function
	usleep(RAFT_LOOP_RATE * 1000);
	sprintf(function_loop->next_invoking, "term_chair");
	unsigned long seq = WooFPut(RAFT_FUNCTION_LOOP_WOOF, "term_chair", function_loop);
	if (WooFInvalid(seq)) {
		log_error(function_tag, "couldn't queue the next function_loop: term_chair");
		exit(1);
	}
	return 1;

}
