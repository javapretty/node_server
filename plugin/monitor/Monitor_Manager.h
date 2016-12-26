/*
 * Monitor_Manager.h
 *
 *  Created on: Dec 26, 2016
 *      Author: zhangyalei
 */

#ifndef GATE_MANAGER_H_
#define GATE_MANAGER_H_

#include "Base_Define.h"
#include "Buffer_List.h"
#include "Thread.h"

enum Msg_Id {
	SYNC_NODE_STACK_INFO = 4,
};

class Monitor_Manager: public Thread {
	typedef Buffer_List<Mutex_Lock> Data_List;
public:
	static Monitor_Manager *instance(void);

	virtual void run_handler(void);
	virtual int process_list(void);

	inline void push_buffer(Byte_Buffer *buffer) {
		notify_lock_.lock();
		buffer_list_.push_back(buffer);
		notify_lock_.signal();
		notify_lock_.unlock();
	}

	int sync_node_stack_info(Msg_Head &msg_head);

private:
	Monitor_Manager(void);
	virtual ~Monitor_Manager(void);
	Monitor_Manager(const Monitor_Manager &);
	const Monitor_Manager &operator=(const Monitor_Manager &);

private:
	static Monitor_Manager *instance_;

	Data_List buffer_list_;			//消息列表
};

#define MONITOR_MANAGER Monitor_Manager::instance()

#endif /* GATE_MANAGER_H_ */
