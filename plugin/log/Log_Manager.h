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

enum DB_MESSAGE_CMD {
	SYNC_NODE_INFO = 2,
	SYNC_SAVE_DB_DATA = 251,
};

class Log_Manager: public Thread {
	typedef Buffer_List<Mutex_Lock> Data_List;
public:
	static Log_Manager *instance(void);

	void init(const Node_Info &node_info) {
		node_info_ = node_info;
		cur_log_id = node_info_.node_id;
	}

	virtual void run_handler(void);
	virtual int process_list(void);

	inline void push_buffer(Byte_Buffer *buffer) {
		notify_lock_.lock();
		buffer_list_.push_back(buffer);
		notify_lock_.signal();
		notify_lock_.unlock();
	}

	void add_log_connector(int cid) { log_list_.push_back(cid); }
	//传递消息，用于路由节点
	int transmit_msg(Msg_Head &msg_head, Byte_Buffer *buffer);
	int save_db_data(Bit_Buffer &buffer);

private:
	Log_Manager(void);
	virtual ~Log_Manager(void);
	Log_Manager(const Log_Manager &);
	const Log_Manager &operator=(const Log_Manager &);

private:
	static Log_Manager *instance_;

	Data_List buffer_list_;			//消息列表
	Node_Info node_info_;				//节点信息
	int cur_log_id;
	std::vector<int> log_list_;	//log链接器cid列表
};

#define LOG_MANAGER Log_Manager::instance()

#endif /* LOG_MANAGER_H_ */
