/*
 *		Daemon_Manager.cpp
 *
 *  Created on: Sep 19, 2016
 *      Author: zhangyalei
 */

#include <string.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sstream>
#include <algorithm>
#include "Epoll_Watcher.h"
#include "Node_Manager.h"
#include "Node_Config.h"
#include "Daemon_Manager.h"

Daemon_Manager::Daemon_Manager(void) : wait_watcher_(0), node_map_(512), core_map_(512) { }

Daemon_Manager::~Daemon_Manager(void) { }

Daemon_Manager *Daemon_Manager::instance_;

struct option Daemon_Manager::long_options[] = {
		{"server_name",	required_argument,	0,	's'},
		{"node_type",		required_argument,	0,	't'},
		{"node_id",				required_argument,	0,	'i'},
		{"node_name",		required_argument,	0,	'n'},
		{0, 0, 0, 0}
};

Daemon_Manager *Daemon_Manager::instance(void) {
    if (! instance_)
        instance_ = new Daemon_Manager();
    return instance_;
}

int Daemon_Manager::init(int argc, char *argv[]) {
	if ((wait_watcher_ = new Epoll_Watcher) == 0) {
		LOG_FATAL("new Epoll_Watcher fatal");
	}

	NODE_CONFIG->load_node_config();
	node_conf_.init();
	const Json::Value &node_misc = NODE_CONFIG->node_misc();
	exec_name_ = node_misc["exec_name"].asString();
	server_name_ = node_misc["server_name"].asString();
	return 0;
}

int Daemon_Manager::start(int argc, char *argv[]) {
	signal(SIGCHLD, sigcld_handle);
	parse_cmd_arguments(argc, argv);
	wait_watcher_->loop();
	return 0;
}

int Daemon_Manager::parse_cmd_arguments(int argc, char *argv[]) {
	int node_type = 0;
	int node_id = 0;
	std::string node_name = "";
	bool daemon_server = true;
	int c = 0;
	while ((c = getopt_long(argc, argv, "vdm:", long_options, NULL)) != -1) {
		daemon_server = false;
		switch (c) {
		case 's': { //server_name
			server_name_ = optarg;
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
		case 'n': { //node_name
			node_name = optarg;
			break;
		}
		default: {
			break;
		}
		}
	}

	//先启动守护进程，守护进程启动node进程，无参数时候会启动守护进程
	LOG_WARN("parse_cmd_arguments, daemon_server:%d, node_type:%d, node_id:%d, node_name:%s",
			daemon_server, node_type, node_id, node_name.c_str());

	if (daemon_server) {
		run_daemon_server();
	} else {
		run_node_server(node_id);
	}

	return 0;
}

int Daemon_Manager::fork_process(const Node_Info &node_info) {
	std::stringstream execname_stream;
	execname_stream << exec_name_ << " --server_name=" << server_name_ << " --node_type=" << node_info.node_type << " --node_id=" << node_info.node_id << " --node_name=" << node_info.node_name;

	std::vector<std::string> exec_str_tok;
	std::istringstream exec_str_stream(execname_stream.str().c_str());
	std::copy(std::istream_iterator<std::string>(exec_str_stream), std::istream_iterator<std::string>(), std::back_inserter(exec_str_tok));
	if (exec_str_tok.empty()) {
		LOG_FATAL("exec_str = %s", execname_stream.str().c_str());
	}

	const char *pathname = (*exec_str_tok.begin()).c_str();
	std::vector<const char *> vargv;
	for (std::vector<std::string>::iterator tok_it = exec_str_tok.begin(); tok_it != exec_str_tok.end(); ++tok_it) {
		vargv.push_back((*tok_it).c_str());
	}
	vargv.push_back((const char *)0);

	pid_t pid = fork();
	if (pid < 0) {
		LOG_FATAL("fork fatal, pid:%d", pid);
	} else if (pid == 0) { //child
		if (execvp(pathname, (char* const*)&*vargv.begin()) < 0) {
			LOG_FATAL("execvp %s fatal", pathname);
		}
	} else { //parent
		node_map_.insert(std::make_pair(pid, node_info));
		//动态创建节点进程时候，将节点信息保存到父进程的node_conf，然后子进程创建时候，可以获取节点信息
		Node_Map::iterator iter = node_conf_.node_map.find(node_info.node_id);
		if (iter == node_conf_.node_map.end()) {
			node_conf_.node_map.insert(std::make_pair(node_info.node_id, node_info));
		}
		LOG_INFO("fork new process, pid:%d, server_name:%s, node_type:%d, node_id:%d, node_name:%s",
				pid, server_name_.c_str(), node_info.node_type, node_info.node_id, node_info.node_name.c_str());
	}

	return pid;
}

void Daemon_Manager::run_daemon_server(void) {
	for(Node_Map::iterator iter = node_conf_.node_map.begin(); iter != node_conf_.node_map.end(); iter++) {
		fork_process(iter->second);
	}
}

void Daemon_Manager::run_node_server(int node_id) {
	Node_Map::iterator iter = node_conf_.node_map.find(node_id);
	if (iter != node_conf_.node_map.end()) {
		NODE_MANAGER->init(iter->second);
		NODE_MANAGER->thr_create();
	}
}

void Daemon_Manager::sigcld_handle(int signo) {
	LOG_ERROR("node receive signo = %d", signo);
	signal(SIGCHLD, sigcld_handle);
	pid_t pid = wait(NULL);
	if (pid < 0) {
		LOG_ERROR("wait error, pid:%d", pid);
	}
	sleep(1);
	DAEMON_MANAGER->restart_process(pid);
}

void Daemon_Manager::restart_process(int pid) {
	Int_Node_Map::iterator iter = node_map_.find(pid);
	if (iter == node_map_.end()) {
		LOG_ERROR("cannot find process, pid = %d.", pid);
		return;
	}

	Int_Core_Map::iterator core_iter = core_map_.find(iter->second.node_id);
	if (core_iter != core_map_.end()) {
		if (core_iter->second++ > 2000) {
			LOG_ERROR("core dump too much, node_type = %d, node_id=%d, core_num=%d", iter->second.node_type, iter->second.node_id, core_iter->second);
			return;
		}
	} else {
		core_map_.insert(std::make_pair(iter->second.node_id, 0));
	}

	fork_process(iter->second);
	node_map_.erase(pid);
}
