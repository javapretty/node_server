/*
 * Monitor_Manager.cpp
 *
 *  Created on: Dec 26, 2016
 *      Author: zhangyalei
 */

#include "Bit_Buffer.h"
#include "Node_Manager.h"
#include "Monitor_Manager.h"

Monitor_Manager::Monitor_Manager(void) { }

Monitor_Manager::~Monitor_Manager(void) { }

Monitor_Manager *Monitor_Manager::instance_;

Monitor_Manager *Monitor_Manager::instance(void) {
	if (instance_ == 0)
		instance_ = new Monitor_Manager;
	return instance_;
}

void Monitor_Manager::run_handler(void) {
	process_list();
}

int Monitor_Manager::process_list(void) {
	Msg_Head msg_head;
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
			switch(msg_head.msg_id) {
			case SYNC_NODE_STACK_INFO:
				sync_node_stack_info(msg_head);
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

int Monitor_Manager::sync_node_stack_info(Msg_Head &msg_head) {
	Bit_Buffer bit_buffer;
	bit_buffer.write_bool(true);
	bit_buffer.write_int(V8_MANAGER->drop_cid(), 32);
	bit_buffer.write_int(V8_MANAGER->timer_id(), 32);
	bit_buffer.write_int(V8_MANAGER->msg_head().msg_id, 32);
	bit_buffer.write_int(V8_MANAGER->msg_head().msg_type, 32);
	bit_buffer.write_int(V8_MANAGER->msg_head().sid, 32);
	bit_buffer.write_str("");
	NODE_MANAGER->send_msg(msg_head, bit_buffer.data(), bit_buffer.get_byte_size());
	return 0;
}