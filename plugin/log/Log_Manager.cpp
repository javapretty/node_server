/*
 * Log_Manager.cpp
 *
 *  Created on: Nov 9, 2016
 *      Author: zhangyalei
 */

#include "Data_Manager.h"
#include "Node_Manager.h"
#include "Log_Manager.h"

Log_Manager::Log_Manager(void) { }

Log_Manager::~Log_Manager(void) { }

Log_Manager *Log_Manager::instance_;

Log_Manager *Log_Manager::instance(void) {
	if (instance_ == 0)
		instance_ = new Log_Manager;
	return instance_;
}

void Log_Manager::run_handler(void) {
	process_list();
}

int Log_Manager::process_list(void) {
	Msg_Head msg_head;
	Bit_Buffer bit_buffer;
	Byte_Buffer *buffer = nullptr;
	while (true) {
		//此处加锁是为了防止本线程其他地方同时等待条件变量成立
		notify_lock_.lock();

		//put wait in while cause there can be spurious wake up (due to signal/ENITR)
		while (buffer_list_.empty()) {
			notify_lock_.wait();
		}

		buffer = buffer_list_.pop_front();
		if(buffer != nullptr) {
			msg_head.reset();
			buffer->read_head(msg_head);
			bit_buffer.reset();
			bit_buffer.set_ary(buffer->get_read_ptr(), buffer->readable_bytes());

			switch(msg_head.msg_id) {
			case SYNC_SAVE_DB_DATA:
				save_db_data(bit_buffer);
				break;
			default:
				break;
			}

			//回收buffer
			NODE_MANAGER->push_buffer(msg_head.eid, msg_head.cid, buffer);
		}

		//操作完成解锁条件变量
		notify_lock_.unlock();
	}
	return 0;
}

void Log_Manager::save_db_data(Bit_Buffer &buffer) {
	uint save_type = buffer.read_uint(2);
	bool vector_data = buffer.read_bool();
	uint db_id = buffer.read_uint(16);
	std::string struct_name = "";
	buffer.read_str(struct_name);
	/*uint data_type*/buffer.read_uint(8);
	DATA_MANAGER->save_db_data(save_type, vector_data, db_id, struct_name, buffer);
}
