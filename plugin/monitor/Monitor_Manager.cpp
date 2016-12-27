/*
 * Monitor_Manager.cpp
 *
 *  Created on: Dec 26, 2016
 *      Author: zhangyalei
 */

#include "Base_Function.h"
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
	std::ostringstream fifo_stream;
	fifo_stream << NODE_FIFO << "_" << node_info_.node_id;

	while(true) {
		//循环从fifo读数据，读到数据就创建新进程
		Byte_Buffer buffer;
		buffer.ensure_writable_bytes(1024);
		int real_read = read_fifo(fifo_stream.str().c_str(), buffer.get_write_ptr(), buffer.writable_bytes());
		if (real_read > 0) {
			//读取数据完毕，设置写指针，否则从buf里面读取数据将会报错
			buffer.set_write_idx(buffer.get_write_idx() + real_read);
			int eid = 0;
			int cid = 0;
			int sid = 0;
			buffer.read_int32(eid);
			buffer.read_int32(cid);
			buffer.read_int32(sid);
			LOG_INFO("read from fifo:%s,real_read:%d,eid:%d,cid:%d,sid:%d", fifo_stream.str().c_str(), real_read, eid, cid, sid);
			sync_node_stack_info(eid, cid, sid);
		}

		//短暂睡眠，开启下次循环
		Time_Value::sleep(Time_Value(0, 500*1000));
	}
	return 0;
}

int Monitor_Manager::sync_node_stack_info(int eid, int cid, int sid) {
	int drop_cid = V8_MANAGER->drop_cid();
	int timer_id = V8_MANAGER->timer_id();
	Msg_Head stack_msg_head = V8_MANAGER->msg_head();

	Bit_Buffer bit_buffer;
	bit_buffer.write_int(drop_cid, 32);
	bit_buffer.write_int(timer_id, 32);
	bit_buffer.write_int(stack_msg_head.msg_id, 32);
	bit_buffer.write_int(stack_msg_head.msg_type, 32);
	bit_buffer.write_int(stack_msg_head.sid, 32);
	bit_buffer.write_str("");
	Msg_Head msg_head;
	msg_head.eid = eid;
	msg_head.cid = cid;
	msg_head.protocol = TCP;
	msg_head.msg_id = SYNC_NODE_STACK_INFO;
	msg_head.msg_type = NODE_MSG;
	msg_head.sid = sid;
	NODE_MANAGER->send_msg(msg_head, bit_buffer.data(), bit_buffer.get_byte_size());
	return 0;
}