#include "nat.h"
#include "ip.h"
#include "icmp.h"
#include "tcp.h"
#include "rtable.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

static struct nat_table nat;

// get the interface from iface name
static iface_info_t *if_name_to_iface(const char *if_name)
{
	iface_info_t *iface = NULL;
	list_for_each_entry(iface, &instance->iface_list, list) {
		if (strcmp(iface->name, if_name) == 0)
			return iface;
	}

	log(ERROR, "Could not find the desired interface according to if_name '%s'", if_name);
	return NULL;
}

// determine the direction of the packet, DIR_IN / DIR_OUT / DIR_INVALID
static int get_packet_direction(char *packet)
{
	fprintf(stdout, "TODO: determine the direction of this packet.\n");

	return DIR_INVALID;
}

// do translation for the packet: replace the ip/port, recalculate ip & tcp
// checksum, update the statistics of the tcp connection
void do_translation(iface_info_t *iface, char *packet, int len, int dir)
{
	fprintf(stdout, "TODO: do translation for this packet.\n");
}

void nat_translate_packet(iface_info_t *iface, char *packet, int len)
{
	int dir = get_packet_direction(packet);
	if (dir == DIR_INVALID) {
		log(ERROR, "invalid packet direction, drop it.");
		icmp_send_packet(packet, len, ICMP_DEST_UNREACH, ICMP_HOST_UNREACH);
		free(packet);
		return ;
	}

	struct iphdr *ip = packet_to_ip_hdr(packet);
	if (ip->protocol != IPPROTO_TCP) {
		log(ERROR, "received non-TCP packet (0x%0hhx), drop it", ip->protocol);
		free(packet);
		return ;
	}

	do_translation(iface, packet, len, dir);
}

// check whether the flow is finished according to FIN bit and sequence number
// XXX: seq_end is calculated by `tcp_seq_end` in tcp.h
static int is_flow_finished(struct nat_connection *conn)
{
    return (conn->internal_fin && conn->external_fin) && \
            (conn->internal_ack >= conn->external_seq_end) && \
            (conn->external_ack >= conn->internal_seq_end);
}

// nat timeout thread: find the finished flows, remove them and free port
// resource
void *nat_timeout()
{
	while (1) {
		fprintf(stdout, "TODO: sweep finished flows periodically.\n");
		sleep(1);
	}

	return NULL;
}

#define MAX_STR_SIZE 100
int get_next_line(FILE *input, char (*strs)[MAX_STR_SIZE], int *num_strings)
{
	const char *delim = " \t\n";
	char buffer[120];
	int ret = 0;
	if (fgets(buffer, sizeof(buffer), input)) {
		char *token = strtok(buffer, delim);
		*num_strings = 0;
		while (token) {
			strcpy(strs[(*num_strings)++], token);
			token = strtok(NULL, delim);
		}

		ret = 1;
	}

	return ret;
}

int read_ip_port(const char *str, u32 *ip, u16 *port)
{
	int i1, i2, i3, i4;
	int ret = sscanf(str, "%d.%d.%d.%d:%hu", &i1, &i2, &i3, &i4, port);
	if (ret != 5) {
		log(ERROR, "parse ip-port string error: %s.", str);
		exit(1);
	}

	*ip = (i1 << 24) | (i2 << 16) | (i3 << 8) | i4;

	return 0;
}

int parse_config(const char *filename)
{
	FILE *input;
	char strings[10][MAX_STR_SIZE];
	int num_strings;

	input = fopen(filename, "r");
	if (input) {
		while (get_next_line(input, strings, &num_strings)) {
			if (num_strings == 0)
				continue;

			if (strcmp(strings[0], "internal-iface:") == 0)
				nat.internal_iface = if_name_to_iface("n1-eth0");
			else if (strcmp(strings[0], "external-iface:") == 0)
				nat.external_iface = if_name_to_iface("n1-eth1");
			else if (strcmp(strings[0], "dnat-rules:") == 0) {
				struct dnat_rule *rule = (struct dnat_rule*)malloc(sizeof(struct dnat_rule));
				read_ip_port(strings[1], &rule->external_ip, &rule->external_port);
				read_ip_port(strings[3], &rule->internal_ip, &rule->internal_port);
				
				list_add_tail(&rule->list, &nat.rules);
			}
			else {
				log(ERROR, "incorrect config file, exit.");
				exit(1);
			}
		}

		fclose(input);
	}
	else {
		log(ERROR, "could not find config file '%s', exit.", filename);
		exit(1);
	}
	
	if (!nat.internal_iface || !nat.external_iface) {
		log(ERROR, "Could not find the desired interfaces for nat.");
		exit(1);
	}

	return 0;
}

// initialize
void nat_init(const char *config_file)
{
	memset(&nat, 0, sizeof(nat));

	for (int i = 0; i < HASH_8BITS; i++)
		init_list_head(&nat.nat_mapping_list[i]);

	init_list_head(&nat.rules);

	// seems unnecessary
	memset(nat.assigned_ports, 0, sizeof(nat.assigned_ports));

	parse_config(config_file);

	pthread_mutex_init(&nat.lock, NULL);

	pthread_create(&nat.thread, NULL, nat_timeout, NULL);
}

void nat_exit()
{
	fprintf(stdout, "TODO: release all resources allocated.\n");
}