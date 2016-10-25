/*
 * Node_Manager.h
  *
 *  Created on: Sep 20, 2016
 *      Author: zhangyalei
 */

#ifndef NODE_MANAGER_H_
#define NODE_MANAGER_H_

#include "Block_List.h"
#include "Log.h"
#include "Endpoint.h"
#include "Node_Define.h"

class Node_Manager: public Thread {
public:
	typedef Object_Pool<Block_Buffer, Thread_Mutex> Block_Pool;
	typedef Object_Pool<Server, Thread_Mutex> Server_Pool;
	typedef Object_Pool<Connector, Thread_Mutex> Connector_Pool;

	typedef List<Drop_Info, Thread_Mutex> Drop_List;
	typedef List<int, Thread_Mutex> Int_List;
	typedef boost::unordered_map<int, Endpoint *> Endpoint_Map;
	typedef boost::unordered_map<int, int> Msg_Count_Map;
public:
	static Node_Manager *instance(void);

	//初始化
	int init(const Node_Info &node);
	//主动关闭
	int self_close(void);

	virtual void run_handler(void);
	virtual int process_list(void);

	inline void push_drop(const Drop_Info &drop_info) { drop_list_.push_back(drop_info); }
	inline void push_tick(int tick);
	inline int pop_tick(void);
	inline int get_drop_list();

	//从endpoint中取消息buffer
	Block_Buffer *pop_buffer(void);
	//回收消息buffer
	int push_buffer(int endpoint_id, int cid, Block_Buffer *buffer);
	//发送消息buffer
	int send_buffer(int endpoint_id, int cid, Block_Buffer &buffer);
	//释放进程pool缓存，后台调用
	int free_pool(void);

	//定时器处理
	int tick(void);
	inline const Time_Value &tick_time(void) { return tick_time_; }
	int drop_list_tick(Time_Value &now);			//关闭连接定时器
	int node_info_tick(Time_Value &now);			//收集服务器信息定时器

	//服务器信息收集
	virtual void get_node_info(void);
	virtual void print_node_info(void);

	//消息统计
	void print_msg_info(void);
	inline void add_msg_count(int msg_id) {
		if (msg_count_) {
			++(msg_count_map_[msg_id]);
		}
	}
	inline void add_recv_bytes(int bytes) {
		if (msg_count_) {
			total_recv_bytes += bytes;
		}
	}
	inline void add_send_bytes(int bytes) {
		if (msg_count_) {
			total_send_bytes += bytes;
		}
	}

private:
	Node_Manager(void);
	virtual ~Node_Manager(void);
	Node_Manager(const Node_Manager &);
	const Node_Manager &operator=(const Node_Manager &);

private:
	static Node_Manager *instance_;

	Block_Pool block_pool_;
	Server_Pool server_pool_;
	Connector_Pool connector_pool_;

	Drop_List drop_list_; 				//逻辑层发起的掉线cid列表
	Int_List tick_list_;					//定时器列表
	Int_List js_tick_list_;				//js定时器列表

	Node_Info node_info_;				//节点信息
	Endpoint_Map endpoint_map_;			//通信端信息

	Time_Value tick_time_;				//节点tick时间
	Time_Value node_info_tick_;			//节点信息tick

	bool msg_count_;
	Msg_Count_Map msg_count_map_;
	int total_recv_bytes;				//总共接收的字节
	int total_send_bytes;				//总共发送的字节
};

#define NODE_MANAGER Node_Manager::instance()

/////////////////////////////////////////////////////////////////
inline void Node_Manager::push_tick(int tick) {
	tick_list_.push_back(tick);
	js_tick_list_.push_back(tick);
}

inline int Node_Manager::pop_tick() {
	if (js_tick_list_.empty()) {
		return 0;
	} else {
		return js_tick_list_.pop_front();
	}
}

int Node_Manager::get_drop_list() {
	int drop_cid = -1;
	for(Endpoint_Map::iterator iter = endpoint_map_.begin();
			iter != endpoint_map_.end(); iter++){
		if(iter->second->endpoint_info().endpoint_type == CLIENT_SERVER) {
			if(!iter->second->drop_list().empty()) {
				drop_cid = iter->second->drop_list().front();
				iter->second->drop_list().pop_front();
			}
		}
	}
	return drop_cid;
}

#endif /* NODE_MANAGER_H_ */
