/*
 *		Daemon_Server.cpp
 *
 *  Created on: Oct 29, 2016
 *      Author: zhangyalei
 */

#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <iterator>
#include <sstream>
#include "Base_Function.h"
#include "Byte_Buffer.h"
#include "Log.h"
#include "Xml.h"
#include "Daemon_Server.h"

Daemon_Server::Daemon_Server(void) : pid_node_map_(get_hash_table_size(512)), node_core_map_(get_hash_table_size(512)) { }

Daemon_Server::~Daemon_Server(void) { }

Daemon_Server *Daemon_Server::instance_;

Daemon_Server *Daemon_Server::instance(void) {
    if (! instance_)
        instance_ = new Daemon_Server();
    return instance_;
}

int Daemon_Server::init(int argc, char *argv[]) {
	Xml xml;
	bool ret = xml.load_xml("./config/node/daemon_conf.xml");
	if(ret < 0) {
		LOG_FATAL("load config:daemon_conf.xml abort");
		return -1;
	}

	TiXmlNode* exec_node = xml.get_root_node("exec");
	if(exec_node) {
		exec_name_ = xml.get_attr_str(exec_node, "name");
		label_ = xml.get_attr_str(exec_node, "label");
	}
	
	TiXmlNode* log_node = xml.get_root_node("log");
	if(log_node) {
		std::string folder_name = "daemon_server";
		LOG_INSTACNE->set_log_file(xml.get_attr_int(log_node, "file"));
		LOG_INSTACNE->set_log_level(xml.get_attr_int(log_node, "level"));
		LOG_INSTACNE->set_folder_name(folder_name);
	}
	LOG_INSTACNE->thr_create();

	TiXmlNode* node = xml.get_root_node("node_list");
	if(node) {
		TiXmlNode* child_node = xml.enter_node(node, "node_list");
		if(child_node) {
			XML_LOOP_BEGIN(child_node)
				Node_Info node_info;
				node_info.node_type = xml.get_attr_int(child_node, "type");
				node_info.node_id = xml.get_attr_int(child_node, "id");
				node_info.endpoint_gid = xml.get_attr_int(child_node, "egid");
				node_info.node_name = xml.get_attr_str(child_node, "name");
				node_list_.push_back(node_info);
			XML_LOOP_END(child_node)
		}
	}

	return 0;
}

int Daemon_Server::start(int argc, char *argv[]) {
	//注册SIGCHLD信号处理函数
	signal(SIGCHLD, sigcld_handle);
	run_daemon_server();

	while(true) {
		//循环从fifo读数据，读到数据就创建新进程
		Byte_Buffer buffer;
		buffer.ensure_writable_bytes(1024);
		int real_read = read_fifo(NODE_FIFO, buffer.get_write_ptr(), buffer.writable_bytes());
		if (real_read > 0) {
			//读取数据完毕，设置写指针，否则从buf里面读取数据将会报错
			buffer.set_write_idx(buffer.get_write_idx() + real_read);
			int node_id = 0;
			int node_type = 0;
			int endpoint_gid = 0;
			std::string node_name = "";
			buffer.read_int32(node_id);
			buffer.read_int32(node_type);
			buffer.read_int32(endpoint_gid);
			buffer.read_string(node_name);
			LOG_INFO("read from fifo:%s,real_read:%d,node_id:%d,node_type:%d,endpoint_gid:%d,node_name:%s", 
				NODE_FIFO, real_read, node_id, node_type, endpoint_gid, node_name.c_str());
			fork_process(node_id, node_type, endpoint_gid, node_name);
		}

		//短暂睡眠，开启下次循环
		Time_Value::sleep(Time_Value(0, 500*1000));
	}
	return 0;
}

void Daemon_Server::run_daemon_server(void) {
	for (Node_List::iterator iter = node_list_.begin(); iter != node_list_.end(); ++iter) {
		fork_process(iter->node_type, iter->node_id, iter->endpoint_gid, iter->node_name);
	}
}

int Daemon_Server::fork_process(int node_type, int node_id, int endpoint_gid, std::string &node_name) {
	std::stringstream execname_stream;
	execname_stream << exec_name_ << " --label=" << label_ << " --node_type=" << node_type << " --node_id=" << node_id << " --endpoint_gid=" << endpoint_gid  << " --node_name=" << node_name;

	std::vector<std::string> exec_str_tok;
	std::istringstream exec_str_stream(execname_stream.str().c_str());
	std::copy(std::istream_iterator<std::string>(exec_str_stream), std::istream_iterator<std::string>(), std::back_inserter(exec_str_tok));
	if (exec_str_tok.empty()) {
		printf("exec_str = %s", execname_stream.str().c_str());
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
		Node_Info node_info;
		node_info.node_type = node_type;
		node_info.node_id = node_id;
		node_info.endpoint_gid = endpoint_gid;
		node_info.node_name = node_name;
		pid_node_map_.insert(std::make_pair(pid, node_info));
		LOG_INFO("fork new process, pid:%d, label:%s, node_type:%d, node_id:%d, endpoint_gid:%d, node_name:%s",
				pid, label_.c_str(), node_type, node_id, endpoint_gid, node_name.c_str());
	}

	return pid;
}

void Daemon_Server::sigcld_handle(int signo) {
	LOG_INFO("daemon_server receive singal:%d", signo);
	//注册SIGCHLD信号处理函数
	signal(SIGCHLD, sigcld_handle);

	int status;
	pid_t pid = wait(&status);
	if (WIFEXITED(status)) {
		//子进程正常终止
		if (WEXITSTATUS(status) != 0) {
			int error_code = WEXITSTATUS(status);
			LOG_WARN("child:%d exit with status code:%d", pid, error_code);
		}
	} 
	else if (WIFSIGNALED(status)) {
		//子进程异常终止
		int signal = WTERMSIG(status);
		LOG_WARN("child:%d killed by singal:%d", pid, signal);
	}

	if (pid < 0) {
		LOG_ERROR("wait error, pid:%d", pid);
		return;
	}
	Time_Value::sleep(Time_Value(0, 500 * 1000));
	DAEMON_SERVER->restart_process(pid);
}

void Daemon_Server::restart_process(int pid) {
	Pid_Node_Map::iterator iter = pid_node_map_.find(pid);
	if (iter == pid_node_map_.end()) {
		LOG_ERROR("cannot find process, pid = %d.", pid);
		return;
	}

	Node_Core_Map::iterator core_iter = node_core_map_.find(iter->second.node_id);
	if (core_iter != node_core_map_.end()) {
		if (core_iter->second++ > 2000) {
			LOG_ERROR("core dump too much, node_type = %d, node_id=%d, core_num=%d", iter->second.node_type, iter->second.node_id, core_iter->second);
			return;
		}
	} else {
		node_core_map_.insert(std::make_pair(iter->second.node_id, 0));
	}

	fork_process(iter->second.node_type, iter->second.node_id, iter->second.endpoint_gid, iter->second.node_name);
	pid_node_map_.erase(pid);
}
