/*
 *		Daemon_Manager.h
 *
 *  Created on: Sep 19, 2016
 *      Author: zhangyalei
 */

#ifndef DAEMON_MANAGER_H_
#define DAEMON_MANAGER_H_

#include <getopt.h>
#include <string>
#include "boost/unordered_map.hpp"
#include "Node_Define.h"

class Epoll_Watcher;
class Daemon_Manager {
	typedef std::vector<Node_Info> Node_List;
	//pid--Node_Info
	typedef boost::unordered_map<int, Node_Info> Int_Node_Map;
	//node_type--core_num
	typedef boost::unordered_map<int, int> Int_Core_Map;
public:
	static Daemon_Manager *instance(void);

	int init(int argc, char *argv[]);
	int start(int argc, char *argv[]);

	int parse_cmd_arguments(int argc, char *argv[]);
	int fork_process(int node_type, int node_id, int endpoint_gid, std::string &node_name);
	void run_daemon_server(void);

	static void sigcld_handle(int signo);
	void restart_process(int pid);

private:
	Daemon_Manager(void);
	~Daemon_Manager(void);
	Daemon_Manager(const Daemon_Manager &);
	const Daemon_Manager &operator=(const Daemon_Manager &);

private:
	static Daemon_Manager *instance_;
	static struct option long_options[];
	Epoll_Watcher *wait_watcher_;

	std::string exec_name_;
	std::string server_name_;

	Node_List node_list_;
	Int_Node_Map node_map_;
	Int_Core_Map core_map_;
};

#define DAEMON_MANAGER Daemon_Manager::instance()

#endif /* DAEMON_MANAGER_H_ */
