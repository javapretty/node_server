/*
 * Gate_Manager.h
 *
 *  Created on: Nov 8, 2016
 *      Author: zhangyalei
 */

#ifndef GATE_MANAGER_H_
#define GATE_MANAGER_H_

#include <unordered_map>
#include "Buffer_List.h"
#include "Object_Pool.h"
#include "Thread.h"

struct Session {
	int client_eid;	//gate向client发消息的端点id
	int client_cid;	//client与gate连接的cid
	int game_eid;	//gate向game发消息的端点id
	int game_cid;	//game与gate连接的cid
	uint sid;		//gate生成的全局唯一session_id
};

class Gate_Manager: public Thread {
	typedef Object_Pool<Session, Mutex_Lock> Session_Pool;
	typedef Mutex_Lock Session_Map_Lock;
	typedef std::unordered_map<uint, Session *> Session_Map;
	typedef Buffer_List<Mutex_Lock> Data_List;
public:
	static Gate_Manager *instance(void);

	virtual void run_handler(void);
	virtual int process_list(void);

	inline Session *pop_session(void) { return session_pool_.pop(); }
	int add_session(Session *session);
	int remove_session(int cid);

	inline void push_buffer(Byte_Buffer *buffer) {
		notify_lock_.lock();
		buffer_list_.push_back(buffer);
		notify_lock_.signal();
		notify_lock_.unlock();
	}
	//传递消息
	int transmit_msg(Msg_Head &msg_head, Byte_Buffer *buffer);

private:
	Gate_Manager(void);
	virtual ~Gate_Manager(void);
	Gate_Manager(const Gate_Manager &);
	const Gate_Manager &operator=(const Gate_Manager &);

private:
	static Gate_Manager *instance_;

	Session_Pool session_pool_;
	Session_Map_Lock session_map_lock_;
	Session_Map cid_session_map_;	//cid--session_info
	Session_Map sid_session_map_;	//sid--session_info

	Data_List buffer_list_;			//消息列表
};

#define GATE_MANAGER Gate_Manager::instance()

#endif /* GATE_MANAGER_H_ */
