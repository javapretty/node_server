/*
 *		Daemon_Server.h
 *
 *  Created on: Oct 29, 2016
 *      Author: zhangyalei
 */

#ifndef DAEMON_H_
#define DAEMON_H_

#include <getopt.h>
#include <string>
#include <unordered_map>
#include <vector>

struct Node_Info {
	int node_type;			//节点类型
	int node_id;			//节点id
	int endpoint_gid;		//端点组id
	std::string node_name;	//节点名称
};

class Daemon_Server {
	typedef std::vector<Node_Info> Node_List;
	//pid--Node_Info
	typedef std::unordered_map<int, Node_Info> Pid_Node_Map;
	//node_id--core_num
	typedef std::unordered_map<int, int> Node_Core_Map;
public:
	static Daemon_Server *instance(void);

	int init(int argc, char *argv[]);
	int start(int argc, char *argv[]);

	void run_daemon_server(void);
	int fork_process(int node_type, int node_id, int endpoint_gid, std::string &node_name);
	static void sigcld_handle(int signo);
	void restart_process(int pid);

private:
	Daemon_Server(void);
	~Daemon_Server(void);
	Daemon_Server(const Daemon_Server &);
	const Daemon_Server &operator=(const Daemon_Server &);

private:
	static Daemon_Server *instance_;

	std::string exec_name_;
	std::string label_;

	Node_List node_list_;
	Pid_Node_Map pid_node_map_;
	Node_Core_Map node_core_map_;
};

#define DAEMON_SERVER Daemon_Server::instance()

#endif /* DAEMON_H_ */
