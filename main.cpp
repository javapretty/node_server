/*
 * main.cpp
 *  Created on: Sep 19, 2016
 *      Author: zhangyalei
 */

#include <signal.h>
#include <getopt.h>
#include "Time_Value.h"
#include "Node_Manager.h"
#include "Msg_Tool.h"

static void sighandler(int sig_no) { exit(0); } /// for gprof need normal exit

struct option long_options[] = {
		{"label",			required_argument,	0,	'l'},
		{"node_type",		required_argument,	0,	't'},
		{"node_id",			required_argument,	0,	'i'},
		{"endpoint_gid",	required_argument,	0,	'e'},
		{"node_name",		required_argument,	0,	'n'},
		{"struct_tool",		no_argument,		0,	's'},
		{0, 0, 0, 0}
};

int parse_cmd_arguments(int argc, char *argv[]) {
	std::string label = "";
	int node_type = 0;
	int node_id = 0;
	int endpoint_gid = 0;
	std::string node_name = "";
	bool bspecial = false;
	int c = 0;
	while ((c = getopt_long_only(argc, argv, "vdm:", long_options, NULL)) != -1) {
		switch (c) {
		case 'l': { //server_name
			label = optarg;
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
		case 's': { //struct_tool
			Msg_Tool tool;
			tool.write_struct();
			bspecial = true;
			break;
		}
		default: {
			break;
		}
		}
	}

	if (node_type > 0 && node_id > 0) {
		LOG_WARN("node init, label:%s, node_type:%d, node_id:%d, endpoint_gid:%d, node_name:%s", label.c_str(), node_type, node_id, endpoint_gid, node_name.c_str());
		NODE_MANAGER->init(node_type, node_id, endpoint_gid, node_name);
		NODE_MANAGER->thr_create();
	} else {
		if(bspecial) {
			return 1;
		}
		LOG_FATAL("node init, label:%s, node_type:%d, node_id:%d, endpoint_gid:%d, node_name:%s", label.c_str(), node_type, node_id, endpoint_gid, node_name.c_str());
	}

	return 0;
}

int main(int argc, char *argv[]) {
	srand(Time_Value::gettimeofday().sec() + Time_Value::gettimeofday().usec());
	srandom(Time_Value::gettimeofday().sec() + Time_Value::gettimeofday().usec());
	signal(SIGPIPE, SIG_IGN);
	signal(SIGUSR1, sighandler);

	//启动node相关线程
	if(0 == parse_cmd_arguments(argc, argv)) {
		Epoll_Watcher watcher;
		watcher.loop();
	}
	return 0;
}
