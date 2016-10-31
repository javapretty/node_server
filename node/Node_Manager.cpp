/*
 * Node_Manager.cpp
 *
 *  Created on: Sep 20, 2016
 *      Author: zhangyalei
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "Base_Function.h"
#include "Data_Manager.h"
#include "Struct_Manager.h"
#include "Node_Timer.h"
#include "Node_Config.h"
#include "V8_Manager.h"
#include "Node_Manager.h"

Node_Manager::Node_Manager(void):
	tick_time_(Time_Value::zero),
	endpoint_map_(get_hash_table_size(256)),
	cid_session_map_(10000),
	sid_session_map_(10000),
	msg_count_(false),
	msg_count_map_(512),
	total_recv_bytes(0),
	total_send_bytes(0)
{
	sem_init(&tick_sem_, 0, 0);
	node_info_tick_ = Time_Value::gettimeofday() + Time_Value(300, 0);
}

Node_Manager::~Node_Manager(void) {
	sem_destroy(&tick_sem_);
}

Node_Manager *Node_Manager::instance_;

Node_Manager *Node_Manager::instance(void) {
	if (instance_ == 0)
		instance_ = new Node_Manager;
	return instance_;
}

int Node_Manager::init(int node_type, int node_id, int endpoint_gid, const std::string &node_name) {
	//初始化node信息
	NODE_CONFIG->load_node_config();
	init_node_info();
	Node_Map::iterator iter = node_map_.find(node_type);
	if (iter != node_map_.end()) {
		node_info_ = iter->second;
		node_info_.node_id = node_id;
		node_info_.node_name = node_name;
	} else {
		LOG_ERROR("can't init node, node_type:%d error, node_id:%d, node_name:%s", node_type, node_id, node_name.c_str());
		return -1;
	}

	const Json::Value &node_misc = NODE_CONFIG->node_misc();
	Log::instance()->set_folder_name(node_info_.node_name);
	Log::instance()->set_log_switcher(node_misc["log_switcher"].asInt());
	msg_count_ = node_misc["msg_count"].asInt();
	STRUCT_MANAGER->set_agent_num(node_misc["agent_num"].asInt());
	STRUCT_MANAGER->set_server_num(node_misc["server_num"].asInt());

	//初始化结构体
	for (uint i = 0; i < node_misc["base_struct_path"].size(); ++i) {
		STRUCT_MANAGER->init_struct(node_misc["base_struct_path"][i].asCString(), BASE_STRUCT);
	}
	for (uint i = 0; i < node_misc["msg_struct_path"].size(); ++i) {
		STRUCT_MANAGER->init_struct(node_misc["msg_struct_path"][i].asCString(), MSG_STRUCT);
	}
	for (uint i = 0; i < node_misc["db_struct_path"].size(); ++i) {
		STRUCT_MANAGER->init_struct(node_misc["db_struct_path"][i].asCString(), DB_STRUCT);
	}
	DATA_MANAGER->init_db_operator();

	//启动server线程
	for (Endpoint_List::iterator iter = node_info_.endpoint_list.begin(); iter != node_info_.endpoint_list.end(); ++iter) {
		if (iter->endpoint_gid == endpoint_gid && (iter->endpoint_type == CLIENT_SERVER || iter->endpoint_type == SERVER)) {
			Server *server = server_pool_.pop();
			server->init(*iter);
			server->start();
			endpoint_map_.insert(std::make_pair(iter->endpoint_id, server));
			LOG_INFO("%s listen ip:%s, port:%d, network_protocol:%d", iter->endpoint_name.c_str(), iter->server_ip.c_str(), iter->server_port, iter->protocol_type);
		}
	}

	//短暂延迟让服务器启动
	Time_Value::sleep(Time_Value(0, 250 * 1000));

	//启动connector线程
	for (Endpoint_List::iterator iter = node_info_.endpoint_list.begin(); iter != node_info_.endpoint_list.end(); ++iter) {
		if (iter->endpoint_gid == endpoint_gid && iter->endpoint_type == CONNECTOR) {
			Connector *connector = connector_pool_.pop();
			connector->init(*iter);
			connector->start();
			int cid = connector->connect_server(iter->server_ip, iter->server_port);
			if (cid < 2) {
				connector_pool_.push(connector);
				LOG_FATAL("%s fatal port:%d", iter->endpoint_name.c_str(), iter->server_port);
				return -1;
			}
			connector->set_cid(cid);
			endpoint_map_.insert(std::make_pair(iter->endpoint_id, connector));
			LOG_INFO("%s connect server ip:%s, port:%d, cid:%d", iter->endpoint_name.c_str(), iter->server_ip.c_str(), iter->server_port, cid);
		}
	}

	//启动V8线程，需要在网络线程启动后
	V8_MANAGER->init(node_info_);
	V8_MANAGER->thr_create();

	//启动定时器线程,需在V8线程启动后
	NODE_TIMER->thr_create();

	return 0;
}

int Node_Manager::init_node_info(void) {
	const Json::Value &node_conf = NODE_CONFIG->node_conf()["node_info"];
	for(uint i = 0; i < node_conf.size(); ++i) {
		Node_Info node_info;
		node_info.reset();
		node_info.node_type = node_conf[i]["node_type"].asInt();
		node_info.node_ip = "127.0.0.1";		//获得本机ip
		node_info.script_path = node_conf[i]["script_path"].asString();

		const Json::Value &plugin_conf = node_conf[i]["plugin"];
		for (uint j = 0; j < plugin_conf.size();++j) {
			std::string plugin_path = plugin_conf[j]["path"].asString();
			node_info.plugin_list.push_back(plugin_path);
		}

		const Json::Value &endpoint_conf = node_conf[i]["endpoint"];
		for (uint j = 0; j < endpoint_conf.size();++j) {
			Endpoint_Info endpoint_info;
			endpoint_info.endpoint_type = endpoint_conf[j]["endpoint_type"].asInt();
			endpoint_info.endpoint_gid = endpoint_conf[j]["endpoint_gid"].asInt();
			endpoint_info.endpoint_id = endpoint_conf[j]["endpoint_id"].asInt();
			endpoint_info.endpoint_name = endpoint_conf[j]["endpoint_name"].asString();
			endpoint_info.server_ip = endpoint_conf[j]["server_ip"].asString();
			endpoint_info.server_port = endpoint_conf[j]["server_port"].asInt();
			endpoint_info.protocol_type = endpoint_conf[j]["protocol_type"].asInt();
			endpoint_info.receive_timeout = endpoint_conf[j]["receive_timeout"].asInt();
			node_info.endpoint_list.push_back(endpoint_info);
		}

		node_map_.insert(std::make_pair(node_info.node_type, node_info));
	}
	return 0;
}

int Node_Manager::self_close(void) {
	LOG_INFO("%s server_id:%d self close", node_info_.node_name.c_str(), node_info_.node_id);

	//回收server和connector
	for (Endpoint_Map::iterator iter = endpoint_map_.begin();  iter != endpoint_map_.end(); ++iter) {
		int endpoint_type = iter->second->endpoint_info().endpoint_type;
		if (endpoint_type == CLIENT_SERVER || endpoint_type == SERVER) {
			Server *server = dynamic_cast<Server*>(iter->second);
			server_pool_.push(server);
		} else if (endpoint_type == CONNECTOR) {
			Connector *connector = dynamic_cast<Connector*>(iter->second);
			connector_pool_.push(connector);
		}
	}

	int i = 0;
	while (++i < 60) {
		sleep(1);
	}

	return 0;
}

int Node_Manager::fork_process(int node_type, int node_id, int endpoint_gid, std::string &node_name) {
	Block_Buffer buf;
	buf.write_int32(node_type);
	buf.write_int32(node_id);
	buf.write_int32(endpoint_gid);
	buf.write_string(node_name);

	int real_write;
	int fd;
	//测试FIFO是否存在，若不存在，mkfifo一个FIFO
	if(access(NODE_FIFO, F_OK) == -1) {
		if((mkfifo(NODE_FIFO, 0666) < 0) && (errno != EEXIST)) {
			LOG_ERROR("Can not create fifo file,%s\n", NODE_FIFO);
			return -1;
		}
	}

	//调用open以只写方式打开FIFO，返回文件描述符fd
	if((fd = open(NODE_FIFO, O_WRONLY)) == -1) {
		LOG_ERROR("Open fifo error,%s\n", NODE_FIFO);
		return -1;
	}

	//调用write将buff写到文件描述符fd指向的FIFO中
	if ((real_write = write(fd, buf.get_read_ptr(), buf.readable_bytes())) > 0) {
		LOG_INFO("Write into fifo:%s, real_write:%d\n", NODE_FIFO, real_write);
	}
	close(fd);

	return 0;
}

void Node_Manager::run_handler(void) {
	process_list();
}

int Node_Manager::process_list(void) {
	while (1) {
		//等待信号量，如果信号量的值大于0,将信号量的值减1,立即返回;如果信号量的值为0,则线程阻塞。相当于P操作
		sem_wait(&tick_sem_);

		//定时器列表
		if (! tick_list_.empty()) {
			tick_list_.pop_front();
			tick();
		}
	}
	return 0;
}

int Node_Manager::transmit_msg(int eid, int cid, int msg_id, int msg_type, uint32_t sid, Block_Buffer *buffer) {
	int buf_eid = 0;
	int buf_cid = 0;
	int buf_msg_id = msg_id;
	int buf_msg_type = 0;
	int buf_sid = 0;

	if (msg_type == C2S) {
		Session *session = find_session_by_cid(cid);
		if (!session) {
			LOG_ERROR("find_session_by_cid error, eid, cid:%d, msg_type:%d, msg_id:%d, sid:%d", eid, cid, msg_type, msg_id, sid);
			return -1;
		}

		buf_eid = session->game_eid;
		buf_cid = session->game_cid;
		buf_msg_type = NODE_C2S;
		buf_sid = session->sid;
	} else if (msg_type == NODE_S2C) {
		Session *session = NODE_MANAGER->find_session_by_sid(sid);
		if (!session) {
			LOG_ERROR("find_session_by_sid error, eid, cid:%d, msg_type:%d, msg_id:%d, sid:%d", eid, cid, msg_type, msg_id, sid);
			return -1;
		}

		buf_eid = session->client_eid;
		buf_cid = session->client_cid;
		buf_msg_type = S2C;
		buf_sid = session->sid;
	}
	send_msg(buf_eid, buf_cid, buf_msg_id, buf_msg_type, buf_sid, buffer);
	return 0;
}

int Node_Manager::send_msg(int eid, int cid, int msg_id, int msg_type, uint32_t sid, Block_Buffer *buffer) {
	Endpoint_Map::iterator iter = endpoint_map_.find(eid);
	if (iter == endpoint_map_.end()) {
		LOG_ERROR("eid %d not exist, cid:%d, msg_id:%d, msg_type:%d, sid:%d", eid, cid, msg_id, msg_type, sid);
		return -1;
	}

	int send_cid = cid;
	if (iter->second->endpoint_info().endpoint_type == CONNECTOR) {
		send_cid = iter->second->get_cid();
	}

	Block_Buffer buf;
	buf.write_uint16(0);
	buf.write_uint8(msg_id);
	if (msg_type != S2C) {
		buf.write_uint8(msg_type);
		buf.write_uint32(sid);
	}
	buf.copy(buffer);
	buf.write_len(RPC_PKG);
	iter->second->send_block(send_cid, buf);
	add_send_bytes(buf.readable_bytes());
	return 0;
}

int Node_Manager::free_pool(void) {
	for (Endpoint_Map::iterator iter = endpoint_map_.begin();  iter != endpoint_map_.end(); ++iter) {
		iter->second->free_pool();
	}

	server_pool_.shrink_all();
	connector_pool_.shrink_all();
	session_pool_.shrink_all();
	return 0;
}

int Node_Manager::add_session(Session *session) {
	if (!session) {
		LOG_ERROR("node_id:%d, node_name:%s add session error", node_info_.node_id, node_info_.node_name.c_str());
		return -1;
	}
	cid_session_map_.insert(std::make_pair(session->client_cid, session));
	sid_session_map_.insert(std::make_pair(session->sid, session));
	return 0;
}

int Node_Manager::remove_session(int cid) {
	Session_Map::iterator iter = cid_session_map_.find(cid);
	if (iter != sid_session_map_.end()) {
		Session *session = iter->second;
		cid_session_map_.erase(iter);
		sid_session_map_.erase(session->sid);
		session_pool_.push(session);
	}
	return 0;
}

Session *Node_Manager::find_session_by_cid(int cid) {
	Session_Map::iterator iter = cid_session_map_.find(cid);
	if (iter != cid_session_map_.end()) {
		return iter->second;
	}
	return nullptr;
}

Session *Node_Manager::find_session_by_sid(int sid) {
	Session_Map::iterator iter = sid_session_map_.find(sid);
	if (iter != sid_session_map_.end()) {
		return iter->second;
	}
	return nullptr;
}

int Node_Manager::tick(void) {
	Time_Value now(Time_Value::gettimeofday());
	tick_time_ = now;
	drop_list_tick(now);
	node_info_tick(now);
	return 0;
}

int Node_Manager::drop_list_tick(Time_Value &now) {
	Drop_Info drop_info;
	while (! drop_list_.empty()) {
		drop_info = drop_list_.front();
		if (now - drop_info.drop_time >= Time_Value(2, 0)) {
			//关闭通信层连接
			Endpoint_Map::iterator iter = endpoint_map_.find(drop_info.eid);
			if (iter != endpoint_map_.end()) {
				iter->second->network().push_drop(drop_info.cid);
			}

			//删除session
			remove_session(drop_info.cid);
		}
	}
	return 0;
}

int Node_Manager::node_info_tick(Time_Value &now) {
	if (now < node_info_tick_)
		return 0;
	
	node_info_tick_ = now + Time_Value(300, 0);
	get_node_info();
	print_node_info();
	print_msg_info();
	return 0;
}

void Node_Manager::get_node_info(void) { }

void Node_Manager::print_node_info(void) {
	LOG_INFO("%s server_id:%d", node_info_.node_name.c_str(), node_info_.node_id);
	LOG_INFO("server_pool_ free = %d, used = %d", server_pool_.free_obj_list_size(), server_pool_.used_obj_list_size());
	LOG_INFO("connector_pool_ free = %d, used = %d", connector_pool_.free_obj_list_size(), connector_pool_.used_obj_list_size());
	LOG_INFO("session_pool_ free = %d, used = %d", session_pool_.free_obj_list_size(), session_pool_.used_obj_list_size());
}

void Node_Manager::print_msg_info(void) {
	std::stringstream stream;
	for (Msg_Count_Map::iterator it = msg_count_map_.begin(); it != msg_count_map_.end(); ++it) {
		stream << (it->first) << "\t" << (it->second) << std::endl;
	}
	LOG_INFO("%s server_id:%d total_recv_bytes:%d total_send_bytes:%d msg_count:%d %s",
			node_info_.node_name.c_str(), node_info_.node_id, total_recv_bytes, total_send_bytes, msg_count_map_.size(), stream.str().c_str());
}
