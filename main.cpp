/*
 * main.cpp
 *  Created on: Sep 19, 2016
 *      Author: zhangyalei
 */

#include <signal.h>
#include <getopt.h>
#include "Time_Value.h"
#include "Node_Manager.h"

static void sighandler(int sig_no) { exit(0); } /// for gprof need normal exit

struct option long_options[] = {
		{"server_name",	required_argument,	0,	's'},
		{"node_type",		required_argument,	0,	't'},
		{"node_id",				required_argument,	0,	'i'},
		{"endpoint_gid",	required_argument,	0,	'e'},
		{"node_name",		required_argument,	0,	'n'},
		{0, 0, 0, 0}
};

int parse_cmd_arguments(int argc, char *argv[]) {
	std::string server_name = "";
	int node_type = 0;
	int node_id = 0;
	int endpoint_gid = 0;
	std::string node_name = "";
	int c = 0;
	while ((c = getopt_long(argc, argv, "vdm:", long_options, NULL)) != -1) {
		switch (c) {
		case 's': { //server_name
			server_name = optarg;
			break;
		}
		case 't': { //node_type
			node_type = atoi(optarg);
			break;
		}
		case 'i': { //node_id
			node_id = atoi(optarg);
			break;
		}
		case 'e': { //endpoint_gid
			endpoint_gid = atoi(optarg);
			break;
		}
		case 'n': { //node_name
			node_name = optarg;
			break;
		}
		default: {
			break;
		}
		}
	}

	LOG_WARN("node init, server_name:%s, node_type:%d, node_id:%d, endpoint_gid:%d, node_name:%s", server_name.c_str(), node_type, node_id, endpoint_gid, node_name.c_str());
	NODE_MANAGER->init(node_type, node_id, endpoint_gid, node_name);
	NODE_MANAGER->thr_create();

	return 0;
}

int main(int argc, char *argv[]) {
	srand(Time_Value::gettimeofday().sec() + Time_Value::gettimeofday().usec());
	srandom(Time_Value::gettimeofday().sec() + Time_Value::gettimeofday().usec());
	signal(SIGPIPE, SIG_IGN);
	signal(SIGUSR1, sighandler);

	//启动node相关线程
	parse_cmd_arguments(argc, argv);

	Epoll_Watcher watcher;
	watcher.loop();
	return 0;
}
