/*
 * Node_Manager.cpp
 *
 *  Created on: Sep 20, 2016
 *      Author: zhangyalei
 */

#include "Base_Function.h"
#include "Data_Manager.h"
#include "Struct_Manager.h"
#include "Hot_Update.h"
#include "Node_Timer.h"
#include "Node_Manager.h"

Node_Manager::Node_Manager(void):
	msg_filter_count_(0),
	endpoint_map_(get_hash_table_size(256)),
	msg_count_map_(get_hash_table_size(1024)),
	node_status_tick_(Time_Value::gettimeofday()) { }

Node_Manager::~Node_Manager(void) { }

Node_Manager *Node_Manager::instance_;

Node_Manager *Node_Manager::instance(void) {
	if (instance_ == 0)
		instance_ = new Node_Manager;
	return instance_;
}

int Node_Manager::init(int node_type, int node_id, int endpoint_gid, const std::string &node_name) {
	Xml xml;
	bool ret = xml.load_xml("./config/node/node_conf.xml");
	if(ret < 0) {
		LOG_FATAL("load config:node_conf.xml abort");
		return -1;
	}

	//初始化node_info
	bool node_exist = false;
	TiXmlNode* node = xml.get_root_node("node_list");
	if(node) {
		TiXmlNode* child_node = xml.enter_node(node, "node_list");
		if(child_node) {
			XML_LOOP_BEGIN(child_node)
				if(xml.get_attr_int(child_node, "type") == node_type) {
					node_exist = true;
					node_info_.node_type = node_type;
					node_info_.node_id = node_id;
					node_info_.endpoint_gid = endpoint_gid;
					node_info_.node_name = node_name;
					node_info_.node_ip = "127.0.0.1";	//获得本机ip
					init_node_info(node_info_.endpoint_gid, xml, child_node);
					break;
				}
			XML_LOOP_END(child_node)
		}
	}
	if(!node_exist) {
		LOG_ERROR("init node error, node_type:%d, node_id:%d, endpoint_gid:%d, node_name:%s", node_type, node_id, endpoint_gid, node_name.c_str());
		return -1;
	}
	
	//初始化消息过滤器数量
	msg_filter_count_ = node_info_.filter_list.size();

	//设置服务器启动时间
	node_status_.start_time = Time_Value::gettimeofday().sec();

	//设置进程资源限制
	set_rlimit();

	//初始化log
	TiXmlNode* log_node = xml.get_root_node("log");
	if(log_node) {
		LOG_INSTACNE->set_log_file(xml.get_attr_int(log_node, "file"));
		LOG_INSTACNE->set_log_level(xml.get_attr_int(log_node, "level"));
		LOG_INSTACNE->set_folder_name(node_info_.node_name);
	}
	LOG_INSTACNE->thr_create();

	//初始化struct
	TiXmlNode* struct_node = xml.get_root_node("struct");
	if(struct_node) {
		STRUCT_MANAGER->set_log_trace(xml.get_attr_int(struct_node, "log_trace"));
		STRUCT_MANAGER->set_agent_num(xml.get_attr_int(struct_node, "agent_num"));
		STRUCT_MANAGER->set_server_num(xml.get_attr_int(struct_node, "server_num"));
		TiXmlNode* child_node = xml.enter_node(struct_node, "struct");
		if(child_node) {
			XML_LOOP_BEGIN(child_node)
				std::string key = xml.get_key(child_node);
				if(key == "base_struct") {
					STRUCT_MANAGER->init_struct(xml.get_attr_str(child_node, "path").c_str(), BASE_STRUCT);
				}
				else if(key == "msg_struct") {
					STRUCT_MANAGER->init_struct(xml.get_attr_str(child_node, "path").c_str(), MSG_STRUCT);
				}
				else if(key == "db_struct") {
					STRUCT_MANAGER->init_struct(xml.get_attr_str(child_node, "path").c_str(), DB_STRUCT);
				}
			XML_LOOP_END(child_node)
		}
	}

	//启动server线程
	for (Endpoint_List::iterator iter = node_info_.endpoint_list.begin(); iter != node_info_.endpoint_list.end(); ++iter) {
		if (iter->endpoint_type == CLIENT_SERVER || iter->endpoint_type == SERVER) {
			Server *server = server_pool_.pop();
			server->init(*iter);
			server->start();
			endpoint_map_.insert(std::make_pair(iter->endpoint_id, server));
			LOG_INFO("%s listen ip:%s, port:%d, network_protocol:%d", iter->endpoint_name.c_str(), iter->server_ip.c_str(), iter->server_port, iter->protocol_type);
		}
	}

	//启动connector线程
	for (Endpoint_List::iterator iter = node_info_.endpoint_list.begin(); iter != node_info_.endpoint_list.end(); ++iter) {
		if (iter->endpoint_type == CONNECTOR) {
			Connector *connector = connector_pool_.pop();
			connector->init(*iter);
			connector->start();
			endpoint_map_.insert(std::make_pair(iter->endpoint_id, connector));
			int ret = connect_server(iter->endpoint_id);
			if(ret < 0) {
				connector_pool_.push(connector);
				LOG_FATAL("%s can't connect server, server_ip:%s, port:%d", iter->endpoint_name.c_str(), iter->server_ip.c_str(), iter->server_port);
			}
		}
	}

	//启动V8线程，需要在网络线程启动后
	V8_MANAGER->init(node_info_);
	V8_MANAGER->thr_create();

	//启动定时器线程,需在V8线程启动后
	NODE_TIMER->thr_create();

	//启动热更新线程
	bool auto_hotupdate = false;
	std::vector<std::string> hotupdate_folder_list;
	TiXmlNode* hotupdate_node = xml.get_root_node("hotupdate");
	if(hotupdate_node) {
		auto_hotupdate = xml.get_attr_int(hotupdate_node, "auto");
		TiXmlNode* child_node = xml.enter_node(hotupdate_node, "hotupdate");
		if(child_node) {
			XML_LOOP_BEGIN(child_node)
				std::string key = xml.get_key(child_node);
				if(key == "folder") {
					hotupdate_folder_list.push_back(xml.get_attr_str(child_node, "path"));
				}
			XML_LOOP_END(child_node)
		}
	}
	//开启线程自动检测文件热更新
	if(auto_hotupdate) {
		HOT_UPDATE->init(hotupdate_folder_list);
		HOT_UPDATE->thr_create();
	}

	return 0;
}

int Node_Manager::init_node_info(int endpoint_gid, Xml &xml, TiXmlNode* node) {
	node_info_.max_session_count = xml.get_attr_int(node, "max_session_count");
	node_info_.global_script = xml.get_attr_str(node, "global_script");
	node_info_.main_script = xml.get_attr_str(node, "main_script");

	TiXmlNode* child_node = xml.enter_node(node, "node");
	if(child_node) {
		XML_LOOP_BEGIN(child_node)
			std::string key = xml.get_key(child_node);
			if(key == "hotupdate") {
				node_info_.hotupdate_list.push_back(xml.get_attr_str(child_node, "path"));
			}
			else if(key == "plugin") {
				node_info_.plugin_list.push_back(xml.get_attr_str(child_node, "path"));
			}
			else if(key == "msg_filter") {
				Msg_Filter msg_filter;
				msg_filter.msg_type = xml.get_attr_int(child_node, "msg_type");
				msg_filter.min_msg_id = xml.get_attr_int(child_node, "min_msg_id");
				msg_filter.max_msg_id = xml.get_attr_int(child_node, "max_msg_id");
				node_info_.filter_list.push_back(msg_filter);
			}
			else if(key == "endpoint" && xml.get_attr_int(child_node, "gid") == endpoint_gid) {
				Endpoint_Info endpoint_info;
				endpoint_info.endpoint_type = xml.get_attr_int(child_node, "type");
				endpoint_info.endpoint_gid = xml.get_attr_int(child_node, "gid");
				endpoint_info.endpoint_id = xml.get_attr_int(child_node, "id");
				endpoint_info.endpoint_name = xml.get_attr_str(child_node, "name");
				endpoint_info.server_ip = xml.get_attr_str(child_node, "ip");
				endpoint_info.server_port = xml.get_attr_int(child_node, "port");
				endpoint_info.protocol_type = xml.get_attr_int(child_node, "protocol");
				endpoint_info.heartbeat_timeout = xml.get_attr_int(child_node, "heartbeat");
				node_info_.endpoint_list.push_back(endpoint_info);
			}
		XML_LOOP_END(child_node)
	}
	return 0;
}

int Node_Manager::connect_server(int eid) {
	int cid = -1;
	Endpoint_Map::iterator iter = endpoint_map_.find(eid);
	if (iter != endpoint_map_.end() && iter->second->endpoint_info().endpoint_type == CONNECTOR) {
		Connector *connector = dynamic_cast<Connector*>(iter->second);
		Endpoint_Info endpoint_info = connector->endpoint_info();
		int connect_times = 0;
		while(cid <= 0 && connect_times < 1000000) {
			cid = connector->connect_server(endpoint_info.server_ip, endpoint_info.server_port);
			connect_times++;
		}

		if(cid > 0) {
			LOG_INFO("%s connect server ip:%s, port:%d, cid:%d", endpoint_info.endpoint_name.c_str(), endpoint_info.server_ip.c_str(), endpoint_info.server_port, cid);
		}
	}
	return cid;
}

int Node_Manager::fork_process(int node_type, int node_id, int endpoint_gid, std::string &node_name) {
	//往fifo里面写数据
	Byte_Buffer buffer;
	buffer.write_int32(node_type);
	buffer.write_int32(node_id);
	buffer.write_int32(endpoint_gid);
	buffer.write_string(node_name);
	int real_write = write_fifo(NODE_FIFO, buffer.get_read_ptr(), buffer.readable_bytes());
	if(real_write > 0) {
		LOG_INFO("write into fifo:%s, real_write:%d", NODE_FIFO, real_write);
	}
	return 0;
}

int Node_Manager::get_node_stack(int node_id, int eid, int cid, int sid) {
	std::ostringstream fifo_stream;
	fifo_stream << NODE_FIFO << "_" << node_id;

	//往fifo里面写数据
	Byte_Buffer buffer;
	buffer.write_int32(eid);
	buffer.write_int32(cid);
	buffer.write_int32(sid);
	int real_write = write_fifo(fifo_stream.str().c_str(), buffer.get_read_ptr(), buffer.readable_bytes());
	if (real_write > 0) {
		LOG_INFO("write into fifo:%s, real_write:%d\n", fifo_stream.str().c_str(), real_write);
	}
	return 0;
}

int Node_Manager::send_msg(Msg_Head &msg_head, char const *data, size_t len) {
	Endpoint_Map::iterator iter = endpoint_map_.find(msg_head.eid);
	if (iter == endpoint_map_.end()) {
		LOG_ERROR("eid %d not exist, cid:%d, msg_id:%d, msg_type:%d, sid:%d",
				msg_head.eid, msg_head.cid, msg_head.msg_id, msg_head.msg_type, msg_head.sid);
		return -1;
	}

	int send_cid = msg_head.cid;
	if (iter->second->endpoint_info().endpoint_type == CONNECTOR) {
		Connector *connector = dynamic_cast<Connector*>(iter->second);
		send_cid = connector->get_cid();
	}

	//rpc_pkg包长最多是8191个字节，减去的6个字节为包头，msg_id(uint8_t),msg_type(uint8_t),sid(uint32_t)
	if(len < (8192 - 6)) {
		msg_head.pkg_type = RPC_PKG;
	}
	else {
		msg_head.pkg_type = TYPE_PKG;
	}
	Byte_Buffer buf;
	buf.write_head(msg_head);
	buf.copy(data, len);
	buf.write_len(msg_head);
	iter->second->send_buffer(send_cid, buf);
	//统计发送字节数
	add_send_bytes(buf.readable_bytes());
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

const Node_Status &Node_Manager::get_node_status(void) {
	Time_Value now = Time_Value::gettimeofday();
	int interval = now.msec() - node_status_tick_.msec();
	if(interval > 0) {
		node_status_.send_per_sec = node_status_.send_per_sec * 1000.0 / interval;
		node_status_.recv_per_sec = node_status_.recv_per_sec * 1000.0 / interval;
	}
	node_status_tick_ = now;
	node_status_.task_count = V8_MANAGER->buffer_list().size();
	return node_status_;
}

void Node_Manager::reset_node_status(void) {
	node_status_.send_per_sec = 0;
	node_status_.recv_per_sec = 0;
}
