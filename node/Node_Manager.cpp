/*
 * Node_Manager.cpp
 *
 *  Created on: Sep 20, 2016
 *      Author: zhangyalei
 */

#include "Node_Timer.h"
#include "Node_Config.h"
#include "V8_Manager.h"
#include "Struct_Manager.h"
#include "Node_Manager.h"

Node_Manager::Node_Manager(void):
	tick_time_(Time_Value::zero),
	node_info_tick_(Time_Value::zero),
  msg_count_(false),
  total_recv_bytes(0),
  total_send_bytes(0)
{
	sem_init(&tick_sem_, 0, 0);
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

int Node_Manager::init(const Node_Info &node_info) {
	node_info_ = node_info;
	const Json::Value &node_misc = NODE_CONFIG->node_misc();
	Log::instance()->set_folder_name(node_info_.node_name);
	Log::instance()->set_log_switcher(node_misc["log_switcher"].asInt());
	msg_count_ = node_misc["msg_count"].asInt();
	STRUCT_MANAGER->set_agent_num(node_misc["agent_num"].asInt());
	STRUCT_MANAGER->set_server_num(node_misc["server_num"].asInt());

	//初始化结构体
	for (uint i = 0; i < node_misc["msg_struct_path"].size(); ++i) {
		STRUCT_MANAGER->init_struct(node_misc["msg_struct_path"][i].asCString(), MSG_STRUCT);
	}
	for (uint i = 0; i < node_misc["db_struct_path"].size(); ++i) {
		STRUCT_MANAGER->init_struct(node_misc["db_struct_path"][i].asCString(), DB_STRUCT);
	}
	STRUCT_MANAGER->init_db_operator();
	//启动定时器线程
	NODE_TIMER->thr_create();
	//启动server线程
	for (Endpoint_List::iterator iter = node_info_.server_list.begin(); iter != node_info_.server_list.end(); ++iter) {
		Server *server = server_pool_.pop();
		server->init(*iter);
		server->start();
		endpoint_map_.insert(std::make_pair(iter->endpoint_id, server));
		LOG_INFO("%s listen ip:%s, port:%d, network_protocol:%d", iter->endpoint_name.c_str(), iter->server_ip.c_str(), iter->server_port, iter->protocol_type);
	}

	//短暂延迟让服务器启动
	Time_Value::sleep(Time_Value(0, 500 * 1000));

	//启动connector线程
	for (Endpoint_List::iterator iter = node_info_.connector_list.begin(); iter != node_info_.connector_list.end(); ++iter) {
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
	//启动V8线程，需要在网络线程启动后
	V8_MANAGER->init(node_info.script_path.c_str());
	V8_MANAGER->thr_create();

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

	//关闭连接 todo
	int i = 0;
	while (++i < 60) {
		sleep(1);
	}

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

Block_Buffer *Node_Manager::pop_buffer(void) {
	for (Endpoint_Map::iterator iter = endpoint_map_.begin(); iter != endpoint_map_.end(); ++iter) {
		if (!iter->second->block_list().empty()) {
			return iter->second->block_list().pop_front();
		}
	}
	return nullptr;
}

int Node_Manager::push_buffer(int endpoint_id, int cid, Block_Buffer *buffer) {
	Endpoint_Map::iterator iter = endpoint_map_.find(endpoint_id);
	if (iter != endpoint_map_.end()) {
		iter->second->push_block(cid, buffer);
	}
	return 0;
}

int Node_Manager::send_buffer(int endpoint_id, int cid, Block_Buffer &buffer) {
	Endpoint_Map::iterator iter = endpoint_map_.find(endpoint_id);
	if (iter != endpoint_map_.end()) {
		if (iter->second->endpoint_info().endpoint_type == CONNECTOR) {
			iter->second->send_block(iter->second->get_cid(), buffer);
		} else {
			iter->second->send_block(cid, buffer);
		}
	}
	else {
		LOG_ERROR("endpoint_id %d not exists", endpoint_id);
	}
	return 0;
}

int Node_Manager::free_pool(void) {
	for (Endpoint_Map::iterator iter = endpoint_map_.begin();  iter != endpoint_map_.end(); ++iter) {
		iter->second->free_pool();
	}

	server_pool_.shrink_all();
	connector_pool_.shrink_all();
	return 0;
}

int Node_Manager::tick(void) {
	Time_Value now(Time_Value::gettimeofday());
	tick_time_ = now;
	drop_list_tick(now);
	node_info_tick(now);
	return 0;
}

int Node_Manager::drop_list_tick(Time_Value &now) {
	//todo 处理网络层主动掉线cid
	Drop_Info drop_info;
	while (! drop_list_.empty()) {
		drop_info = drop_list_.front();
		if (now - drop_info.drop_time > Time_Value(2, 0)) {
			drop_list_.pop_front();

			//关闭通信层连接
			if (drop_info.drop_code != 0) {
				Endpoint_Map::iterator iter = endpoint_map_.find(drop_info.endpoint_id);
				if (iter != endpoint_map_.end()) {
					iter->second->network().push_drop(drop_info.drop_cid);
				}
			}
		} else {
			break;
		}
	}
	return 0;
}

int Node_Manager::node_info_tick(Time_Value &now) {
	if (now - node_info_tick_ < Time_Value(300, 0))
		return 0;
	node_info_tick_ = now;

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
}

void Node_Manager::print_msg_info(void) {
	std::stringstream stream;
	for (Msg_Count_Map::iterator it = msg_count_map_.begin(); it != msg_count_map_.end(); ++it) {
		stream << (it->first) << "\t" << (it->second) << std::endl;
	}
	LOG_INFO("%s server_id:%d total_recv_bytes:%d total_send_bytes:%d msg_count:%d %s",
			node_info_.node_name.c_str(), node_info_.node_id, total_recv_bytes, total_send_bytes, msg_count_map_.size(), stream.str().c_str());
}
