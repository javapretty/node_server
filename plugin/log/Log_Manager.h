/*
 * Log_Manager.h
 *
 *  Created on: Nov 9, 2016
 *      Author: zhangyalei
 */

#ifndef LOG_MANAGER_H_
#define LOG_MANAGER_H_

#include <unordered_map>
#include "Bit_Buffer.h"
#include "Buffer_List.h"
#include "Object_Pool.h"
#include "Thread.h"
#include "Node_Define.h"

class Log_Manager: public Thread {
	typedef Buffer_List<Mutex_Lock> Data_List;
	typedef std::vector<Node_Info> Node_List;
public:
	static Log_Manager *instance(void);

	void init(const Node_Info &node_info) { node_info_ = node_info; }

	virtual void run_handler(void);
	virtual int process_list(void);

	inline void push_buffer(Byte_Buffer *buffer) {
		notify_lock_.lock();
		buffer_list_.push_back(buffer);
		notify_lock_.signal();
		notify_lock_.unlock();
	}

private:
	Log_Manager(void);
	virtual ~Log_Manager(void);
	Log_Manager(const Log_Manager &);
	const Log_Manager &operator=(const Log_Manager &);

private:
	static Log_Manager *instance_;

	Node_Info node_info_;					//节点信息
	Data_List buffer_list_;				//消息列表
	Node_List log_list_;						//log链接器列表
};

#define LOG_MANAGER Log_Manager::instance()

#endif /* LOG_MANAGER_H_ */
