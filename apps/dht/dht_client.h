#ifndef DHT_CLIENT_H
#define DHT_CLIENT_H

int dht_find_node(char *topic_name, char result_node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH], int *result_node_leader);
int dht_create_topic(char *topic_name, unsigned long element_size, unsigned long history_size);
int dht_register_topic(char *topic_name);
int dht_subscribe(char *topic_name, char *handler);
unsigned long dht_publish(char *topic_name, void *element);

#endif
