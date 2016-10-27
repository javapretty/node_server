/*
 *		Daemon_Server.cpp
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
#include "Daemon_Server.h"

Daemon_Server::Daemon_Server(void) : wait_watcher_(0), node_map_(512), core_map_(512) { }

Daemon_Server::~Daemon_Server(void) { }

Daemon_Server *Daemon_Server::instance_;

struct option Daemon_Server::long_options[] = {
		{"node_type",			required_argument,	0,	't'},
		{"node_id",			required_argument,	0,	'i'},
		{"server_name",		required_argument,	0,	's'},
		{0, 0, 0, 0}
};

Daemon_Server *Daemon_Server::instance(void) {
    if (! instance_)
        instance_ = new Daemon_Server();
    return instance_;
}

int Daemon_Server::init(int argc, char *argv[]) {
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

int Daemon_Server::start(int argc, char *argv[]) {
	signal(SIGCHLD, sigcld_handle);
	parse_cmd_arguments(argc, argv);
	wait_watcher_->loop();
	return 0;
}

int Daemon_Server::parse_cmd_arguments(int argc, char *argv[]) {
	int node_type = 0;
	int node_id = 0;
	bool daemon_server = false;
	int c = 0;
	while ((c = getopt_long(argc, argv, "vdm:", long_options, NULL)) != -1) {
		switch (c) {
		case 't': { //node_type
			node_type = atoi(optarg);
			break;
		}
		case 'i': { //node_id
			node_id = atoi(optarg);
			break;
		}
		case 's': { //server_name
			server_name_ = optarg;
			break;
		}
		default: {
			daemon_server = true;
			break;
		}
		}
	}

	if (node_type == 0 && node_id == 0) {
		daemon_server = true;
	}

	//进程启动时候，先启动守护进程，守护进程根据node_conf.json配置启动响应的node进程
	if (daemon_server) {
		run_daemon_server();
	} else {
		run_node_server(node_id);
	}

	return 0;
}

int Daemon_Server::fork_process(int node_type, int node_id, const char *node_name) {
	std::stringstream execname_stream;
	execname_stream << exec_name_ << " --node_type=" << node_type << " --node_id=" << node_id << " --server_name=" << server_name_;
	LOG_INFO("exec_str=%s, node_name=%s", execname_stream.str().c_str(), node_name);

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
		Node_Info process_info;
		process_info.node_type = node_type;
		process_info.node_id = node_id;
		process_info.node_name = node_name;
		node_map_.insert(std::make_pair(pid, process_info));
		LOG_INFO("fork new process, pid:%d, node_type:%d, node_id:%d, node_name:%s", pid, node_type, node_id, node_name);
	}

	return pid;
}

void Daemon_Server::run_daemon_server(void) {
	for(Node_List::iterator iter = node_conf_.node_list.begin();
			iter != node_conf_.node_list.end(); iter++) {
		fork_process(iter->node_type, iter->node_id, iter->node_name.c_str());
	}
}

void Daemon_Server::run_node_server(int node_id) {
	for (Node_List::const_iterator iter = node_conf_.node_list.begin();
			iter != node_conf_.node_list.end(); ++iter) {
		if (iter->node_id == node_id) {
			NODE_MANAGER->init(*iter);
			NODE_MANAGER->thr_create();
			break;
		}
	}
}

void Daemon_Server::sigcld_handle(int signo) {
	LOG_ERROR("node receive signo = %d", signo);
	signal(SIGCHLD, sigcld_handle);
	pid_t pid = wait(NULL);
	if (pid < 0) {
		LOG_ERROR("wait error, pid:%d", pid);
	}
	sleep(1);
	DAEMON_SERVER->restart_process(pid);
}

void Daemon_Server::restart_process(int pid) {
	Int_Node_Map::iterator iter = node_map_.find(pid);
	if (iter == node_map_.end()) {
		LOG_ERROR("cannot find process, pid = %d.", pid);
		return;
	}

	Int_Core_Map::iterator core_iter = core_map_.find(iter->second.node_type);
	if (core_iter != core_map_.end()) {
		if (core_iter->second++ > 2000) {
			LOG_ERROR("so many core, node_type=%d, core_num=%d", iter->second.node_type, core_iter->second);
			return;
		}
	} else {
		core_map_.insert(std::make_pair(iter->second.node_type, 0));
	}

	fork_process(iter->second.node_type, iter->second.node_id, iter->second.node_name.c_str());
	node_map_.erase(pid);
}
