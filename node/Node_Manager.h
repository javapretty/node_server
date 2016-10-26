/*
 * Node_Manager.h
  *
 *  Created on: Sep 20, 2016
 *      Author: zhangyalei
 */

#ifndef NODE_MANAGER_H_
#define NODE_MANAGER_H_

#include <semaphore.h>
#include "Block_List.h"
#include "Log.h"
#include "Endpoint.h"
#include "Node_Define.h"

class Node_Manager: public Thread {
public:
	typedef Object_Pool<Server, Thread_Mutex> Server_Pool;
	typedef Object_Pool<Connector, Thread_Mutex> Connector_Pool;
	typedef Object_Pool<Session, Thread_Mutex> Session_Pool;

	typedef List<Drop_Info, Thread_Mutex> Drop_List;
	typedef List<int, Thread_Mutex> Int_List;
	typedef boost::unordered_map<int, Endpoint *> Endpoint_Map;
	typedef boost::unordered_map<int, Session *> Session_Map;
	typedef boost::unordered_map<int, int> Msg_Count_Map;
public:
	static Node_Manager *instance(void);

	//初始化
	int init(const Node_Info &node_info);
	//主动关闭
	int self_close(void);

	virtual void run_handler(void);
	virtual int process_list(void);

	inline Node_Info &node_info(void) { return node_info_; }
	inline void push_drop(const Drop_Info &drop_info) { drop_list_.push_back(drop_info); }
	inline int pop_drop_cid();
	inline void push_tick(int tick) {
		//释放信号量，让信号量的值加1，相当于V操作
		sem_post(&tick_sem_);
		tick_list_.push_back(tick);
	}

	//从endpoint中取消息buffer
	inline Block_Buffer *pop_buffer(void);
	//回收消息buffer
	inline int push_buffer(int endpoint_id, int cid, Block_Buffer *buffer);

	//发送消息buffer
	int send_buffer(int endpoint_id, int cid, int msg_id, int msg_type, uint32_t sid, Block_Buffer *buffer);
	//释放进程pool缓存，后台调用
	int free_pool(void);

	inline Session *pop_session(void) { return session_pool_.pop(); }
	int add_session(Session *session);
	int remove_session(int cid);
	Session *find_session_by_cid(int cid);
	Session *find_session_by_sid(int sid);

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

	Drop_List drop_list_; 			//逻辑层发起的掉线cid列表

	sem_t tick_sem_;						//定时器通知信号量
	Int_List tick_list_;				//定时器列表

	Time_Value tick_time_;			//节点tick时间
	Time_Value node_info_tick_;//节点信息tick

	Server_Pool server_pool_;
	Connector_Pool connector_pool_;
	Session_Pool session_pool_;

	Node_Info node_info_;					//节点信息
	Endpoint_Map endpoint_map_;		//通信端信息

	Session_Map cid_session_map_;	//cid--session_info
	Session_Map sid_session_map_;	//sid--session_info

	bool msg_count_;
	Msg_Count_Map msg_count_map_;
	int total_recv_bytes;			//总共接收的字节
	int total_send_bytes;			//总共发送的字节
};

#define NODE_MANAGER Node_Manager::instance()

int Node_Manager::pop_drop_cid() {
	int drop_cid = -1;
	for(Endpoint_Map::iterator iter = endpoint_map_.begin();
			iter != endpoint_map_.end(); iter++){
		int endpoint_type = iter->second->endpoint_info().endpoint_type;
		if(endpoint_type == CLIENT_SERVER || endpoint_type == SERVER) {
			if(!iter->second->drop_list().empty()) {
				drop_cid = iter->second->drop_list().front();
				iter->second->drop_list().pop_front();
			}
		}
	}
	return drop_cid;
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

#endif /* NODE_MANAGER_H_ */

