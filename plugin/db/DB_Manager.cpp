/*
 * DB_Manager.cpp
 *
 *  Created on: Nov 9, 2016
 *      Author: zhangyalei
 */

#include "Data_Manager.h"
#include "Struct_Manager.h"
#include "Node_Manager.h"
#include "DB_Manager.h"

DB_Manager::DB_Manager(void) { }

DB_Manager::~DB_Manager(void) { }

DB_Manager *DB_Manager::instance_;

DB_Manager *DB_Manager::instance(void) {
	if (instance_ == 0)
		instance_ = new DB_Manager;
	return instance_;
}

void DB_Manager::run_handler(void) {
	process_list();
}

int DB_Manager::process_list(void) {
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
			Bit_Buffer bit_buffer;
			bit_buffer.set_ary(buffer->get_read_ptr(), buffer->readable_bytes());

			switch(msg_head.msg_id) {
			case SYNC_GET_TABLE_INDEX:
				get_table_index(msg_head.cid, msg_head.sid, bit_buffer);
				break;
			case SYNC_GENERATE_ID:
				generate_id(msg_head.cid, msg_head.sid, bit_buffer);
				break;
			case SYNC_LOAD_DB_DATA:
				load_db_data(msg_head.cid, msg_head.sid, bit_buffer);
				break;
			case SYNC_SAVE_DB_DATA:
				save_db_data(msg_head.cid, msg_head.sid, bit_buffer);
				break;
			case SYNC_DELETE_DB_DATA:
				delete_db_data(msg_head.cid, msg_head.sid, bit_buffer);
				break;
			case SYNC_LOAD_RUNTIME_DATA:
				load_runtime_data(msg_head.cid, msg_head.sid, bit_buffer);
				break;
			case SYNC_SAVE_RUNTIME_DATA:
				save_runtime_data(msg_head.cid, msg_head.sid, bit_buffer);
				break;
			case SYNC_DELETE_RUNTIME_DATA:
				delete_runtime_data(msg_head.cid, msg_head.sid, bit_buffer);
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

void DB_Manager::get_table_index(int cid, int sid, Bit_Buffer &buffer) {
	uint db_id = buffer.read_uint(16);
	std::string table_name = "";
	std::string index_name = "";
	std::string query_name = "";
	std::string query_value = "";
	buffer.read_str(table_name);
	buffer.read_str(index_name);
	buffer.read_str(query_name);
	buffer.read_str(query_value);
	int64_t key_index = DATA_MANAGER->get_table_index(db_id, table_name, index_name, query_name, query_value);

	Msg_Head msg_head;
	msg_head.eid = 1;
	msg_head.cid = cid;
	msg_head.msg_id = SYNC_RES_TABLE_INDEX;
	msg_head.msg_type = NODE_MSG;
	msg_head.sid = sid;
	Bit_Buffer bit_buffer;
	bit_buffer.write_str(table_name.c_str());
	bit_buffer.write_int64(key_index);
	NODE_MANAGER->send_msg(msg_head, bit_buffer.data(), bit_buffer.get_byte_size());
}

void DB_Manager::generate_id(int cid, int sid, Bit_Buffer &buffer) {
	uint db_id = buffer.read_uint(16);
	std::string table_name = "";
	std::string type = "";
	buffer.read_str(table_name);
	buffer.read_str(type);
	int64_t id = DATA_MANAGER->generate_id(db_id, table_name, type);

	Msg_Head msg_head;
	msg_head.eid = 1;
	msg_head.cid = cid;
	msg_head.msg_id = SYNC_DB_RES_ID;
	msg_head.msg_type = NODE_MSG;
	msg_head.sid = sid;
	Bit_Buffer bit_buffer;
	bit_buffer.write_str(type.c_str());
	bit_buffer.write_int64(id);
	NODE_MANAGER->send_msg(msg_head, bit_buffer.data(), bit_buffer.get_byte_size());
}

void DB_Manager::load_db_data(int cid, int sid, Bit_Buffer &buffer) {
	uint db_id = buffer.read_uint(16);
	std::string struct_name = "";
	buffer.read_str(struct_name);
	int64_t key_index = buffer.read_int64();
	uint data_type = buffer.read_uint(8);

	Msg_Head msg_head;
	msg_head.eid = 1;
	msg_head.cid = cid;
	msg_head.msg_id = SYNC_SAVE_DB_DATA;
	msg_head.msg_type = NODE_MSG;
	msg_head.sid = sid;
	Bit_Buffer bit_buffer;
	bit_buffer.write_uint(0, 2);
	bit_buffer.write_bool(false);
	bit_buffer.write_uint(db_id, 16);
	bit_buffer.write_str(struct_name.c_str());
	bit_buffer.write_uint(data_type, 8);
	int ret = DATA_MANAGER->load_db_data(db_id, struct_name, key_index, bit_buffer);
	if(ret < 0) {
		msg_head.msg_id = SYNC_NODE_CODE;
		bit_buffer.reset();
		bit_buffer.write_uint(LOAD_DB_DATA_FAIL, 16);
	}
	NODE_MANAGER->send_msg(msg_head, bit_buffer.data(), bit_buffer.get_byte_size());
}

void DB_Manager::save_db_data(int cid, int sid, Bit_Buffer &buffer) {
	uint save_type = buffer.read_uint(2);
	bool vector_data = buffer.read_bool();
	uint db_id = buffer.read_uint(16);
	std::string struct_name = "";
	buffer.read_str(struct_name);
	/*uint data_type*/buffer.read_uint(8);
	int ret = DATA_MANAGER->save_db_data(save_type, vector_data, db_id, struct_name, buffer);
	Msg_Head msg_head;
	msg_head.eid = 1;
	msg_head.cid = cid;
	msg_head.msg_id = SYNC_NODE_CODE;
	msg_head.msg_type = NODE_MSG;
	msg_head.sid = sid;
	if(ret < 0) {
		Bit_Buffer bit_buffer;
		bit_buffer.write_uint(SAVE_DB_DATA_FAIL, 16);
		NODE_MANAGER->send_msg(msg_head, bit_buffer.data(), bit_buffer.get_byte_size());
	} else {
		if(save_type == SAVE_DB_CLEAR_CACHE) {
			Bit_Buffer bit_buffer;
			bit_buffer.write_uint(SAVE_DB_DATA_SUCCESS, 16);
			NODE_MANAGER->send_msg(msg_head, bit_buffer.data(), bit_buffer.get_byte_size());
		}
	}
}

void DB_Manager::delete_db_data(int cid, int sid, Bit_Buffer &buffer) {
	uint db_id = buffer.read_uint(16);
	std::string struct_name = "";
	buffer.read_str(struct_name);
	int ret = DATA_MANAGER->delete_db_data(db_id, struct_name, buffer);
	if(ret < 0) {
		Msg_Head msg_head;
		msg_head.eid = 1;
		msg_head.cid = cid;
		msg_head.msg_id = SYNC_NODE_CODE;
		msg_head.msg_type = NODE_MSG;
		msg_head.sid = sid;
		Bit_Buffer bit_buffer;
		bit_buffer.write_uint(DELETE_DB_DATA_FAIL, 16);
		NODE_MANAGER->send_msg(msg_head, bit_buffer.data(), bit_buffer.get_byte_size());
	}
}

void DB_Manager::load_runtime_data(int cid, int sid, Bit_Buffer &buffer) {
	std::string struct_name = "";
	buffer.read_str(struct_name);
	int64_t key_index = buffer.read_int64();
	uint data_type = buffer.read_uint(8);

	Msg_Head msg_head;
	msg_head.eid = 1;
	msg_head.cid = cid;
	msg_head.msg_id = SYNC_SAVE_RUNTIME_DATA;
	msg_head.msg_type = NODE_MSG;
	msg_head.sid = sid;
	Bit_Buffer bit_buffer;
	bit_buffer.write_str(struct_name.c_str());
	bit_buffer.write_int64(key_index);
	bit_buffer.write_uint(data_type, 8);
	int ret = DATA_MANAGER->load_runtime_data(struct_name, key_index, bit_buffer);
	if(ret < 0) {
		msg_head.msg_id = SYNC_NODE_CODE;
		bit_buffer.reset();
		bit_buffer.write_uint(LOAD_RUNTIME_DATA_FAIL, 16);
	}
	NODE_MANAGER->send_msg(msg_head, bit_buffer.data(), bit_buffer.get_byte_size());
}

void DB_Manager::save_runtime_data(int cid, int sid, Bit_Buffer &buffer) {
	std::string struct_name = "";
	buffer.read_str(struct_name);
	int64_t key_index = buffer.read_int64();
	/*uint data_type*/buffer.read_uint(8);
	int ret = DATA_MANAGER->save_runtime_data(struct_name, key_index, buffer);
	if(ret < 0) {
		Msg_Head msg_head;
		msg_head.eid = 1;
		msg_head.cid = cid;
		msg_head.msg_id = SYNC_NODE_CODE;
		msg_head.msg_type = NODE_MSG;
		msg_head.sid = sid;
		Bit_Buffer bit_buffer;
		bit_buffer.write_uint(SAVE_RUNTIME_DATA_FAIL, 16);
		NODE_MANAGER->send_msg(msg_head, bit_buffer.data(), bit_buffer.get_byte_size());
	}
}

void DB_Manager::delete_runtime_data(int cid, int sid, Bit_Buffer &buffer) {
	std::string struct_name = "";
	buffer.read_str(struct_name);
	int64_t key_index = buffer.read_int64();
	int ret = DATA_MANAGER->delete_runtime_data(struct_name, key_index);
	if(ret < 0) {
		Msg_Head msg_head;
		msg_head.eid = 1;
		msg_head.cid = cid;
		msg_head.msg_id = SYNC_NODE_CODE;
		msg_head.msg_type = NODE_MSG;
		msg_head.sid = sid;
		Bit_Buffer bit_buffer;
		bit_buffer.write_uint(DELETE_RUNTIME_DATA_FAIL, 16);
		NODE_MANAGER->send_msg(msg_head, bit_buffer.data(), bit_buffer.get_byte_size());
	}
}
