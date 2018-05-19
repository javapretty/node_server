/*
 * Node_Manager.h
  *
 *  Created on: Sep 20, 2016
 *      Author: zhangyalei
 */

#ifndef NODE_MANAGER_H_
#define NODE_MANAGER_H_

#include "Xml.h"
#include "Node_Endpoint.h"

class Node_Manager {
public:
	typedef Object_Pool<Server, Mutex_Lock> Server_Pool;
	typedef Object_Pool<Connector, Mutex_Lock> Connector_Pool;
	typedef List<int, Mutex_Lock> Int_List;
	typedef std::unordered_map<int, Endpoint *> Endpoint_Map;	//endpoint_id--endpoint
	typedef std::unordered_map<int, int> Msg_Count_Map;			//msg_id--msg_count
public:
	static Node_Manager *instance(void);

	//初始化
	int init(int node_type, int node_id, int endpoint_gid, const std::string &node_name);
	int init_node_info(int endpoint_gid, Xml &xml, TiXmlNode* node);

	int connect_server(int eid);
	//通知daemon_server创建进程
	int fork_process(int node_type, int node_id, int endpoint_gid, std::string &node_name);
	//获取node堆栈
	int get_node_stack(int node_id, int eid, int cid, int sid);

	inline const Node_Info &node_info(void) { return node_info_; }
	//通知网络层掉线
	inline int push_drop(int eid, int cid);
	//回收消息buffer
	inline int push_buffer(int eid, int cid, Byte_Buffer *buffer);
	//消息过滤器，被过滤的消息，不抛给脚本层，直接由C++处理
	inline bool msg_filter(int msg_type, int msg_id);
	//发送消息
	int send_msg(Msg_Head &msg_head, char const *data, size_t len);

	//释放内存池自由节点
	int free_pool(void);
	//获得消息数量信息
	inline const Msg_Count_Map &msg_count_map(void) { return msg_count_map_; }
	//获得node状态
	const Node_Status &get_node_status(void);
	void reset_node_status(void);
	//字节统计
	inline void add_send_bytes(int send_bytes) {
		node_status_.total_send += send_bytes;
		node_status_.send_per_sec += send_bytes;
	}
	inline void add_recv_bytes(int recv_bytes) {
		node_status_.total_recv += recv_bytes;
		node_status_.recv_per_sec += recv_bytes;
	}
	inline void add_msg_count(int msg_id) {
		msg_count_map_[msg_id]++;
	}

private:
	Node_Manager(void);
	virtual ~Node_Manager(void);
	Node_Manager(const Node_Manager &);
	const Node_Manager &operator=(const Node_Manager &);

private:
	static Node_Manager *instance_;

	Server_Pool server_pool_;
	Connector_Pool connector_pool_;

	int msg_filter_count_;			//消息过滤器数量
	Node_Info node_info_;			//节点信息
	Endpoint_Map endpoint_map_;		//通信端信息

	Msg_Count_Map msg_count_map_;	//消息数量信息
	Time_Value node_status_tick_;	//获取节点状态tick时间
	Node_Status node_status_;		//节点状态
};

#define NODE_MANAGER Node_Manager::instance()

int Node_Manager::push_drop(int eid, int cid) {
	Endpoint_Map::iterator iter = endpoint_map_.find(eid);
	if (iter != endpoint_map_.end()) {
		iter->second->network().push_drop(cid);
	}
	return 0;
}

int Node_Manager::push_buffer(int eid, int cid, Byte_Buffer *buffer) {
	Endpoint_Map::iterator iter = endpoint_map_.find(eid);
	if (iter != endpoint_map_.end()) {
		iter->second->push_buffer(cid, buffer);
	}
	return 0;
}

bool Node_Manager::msg_filter(int msg_type, int msg_id) {
	for (int i = 0; i < msg_filter_count_; ++i) {
		if (msg_type == node_info_.filter_list[i].msg_type && msg_id >= node_info_.filter_list[i].min_msg_id
				&& msg_id <= node_info_.filter_list[i].max_msg_id) {
			return true;
		}
	}
	return false;
}

#endif /* NODE_MANAGER_H_ */
