/*
 * Log_Manager.cpp
 *
 *  Created on: Nov 9, 2016
 *      Author: zhangyalei
 */

#include "Data_Manager.h"
#include "Node_Manager.h"
#include "Log_Manager.h"

Log_Manager::Log_Manager(void) : cur_log_id(0) { }

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
			int eid = msg_head.eid;
			int cid = msg_head.cid;

			switch(msg_head.msg_id) {
			case SYNC_NODE_INFO:
				add_log_connector(cid);
				break;
			case SYNC_SAVE_DB_DATA: {
				if(node_info_.endpoint_gid == 1) {
					int connector_size = buffer_list_.size() / node_info_.max_session_count;
					if(connector_size > 0) {
						int lack_connector = connector_size - log_list_.size();
						if(lack_connector > 0) {
							for(int i = 0; i < lack_connector; ++i) {
								std::string node_name = "log_connector";
								NODE_MANAGER->fork_process(LOG_SERVER, ++cur_log_id, 2, node_name);
							}
						}
						else {
							transmit_msg(msg_head, buffer);
						}
					}
				} else {
					bit_buffer.reset();
					bit_buffer.set_ary(buffer->get_read_ptr(), buffer->readable_bytes());
					save_db_data(bit_buffer);
				}
				break;
			}
			default:
				break;
			}

			//回收buffer
			NODE_MANAGER->push_buffer(eid, cid, buffer);
		}

		//操作完成解锁条件变量
		notify_lock_.unlock();
	}
	return 0;
}

int Log_Manager::transmit_msg(Msg_Head &msg_head, Byte_Buffer *buffer) {
	int index = random() % (log_list_.size());
	msg_head.cid = log_list_[index];
	NODE_MANAGER->send_msg(msg_head, buffer->get_read_ptr(), buffer->readable_bytes());
	return 0;
}

int Log_Manager::save_db_data(Bit_Buffer &buffer) {
	uint save_type = buffer.read_uint(2);
	bool vector_data = buffer.read_bool();
	uint db_id = buffer.read_uint(16);
	std::string struct_name = "";
	buffer.read_str(struct_name);
	/*uint data_type*/buffer.read_uint(8);
	return DATA_MANAGER->save_db_data(save_type, vector_data, db_id, struct_name, buffer);
}
