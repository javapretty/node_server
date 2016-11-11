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
			case SYNC_GAME_DB_LOAD_PLAYER:
				load_player(msg_head.cid, msg_head.sid, bit_buffer);
				break;
			case SYNC_GAME_DB_SAVE_PLAYER:
				save_player(msg_head.cid, msg_head.sid, bit_buffer);
				break;
			case SYNC_PUBLIC_DB_LOAD_DATA:
				break;
			case SYNC_PUBLIC_DB_SAVE_DATA:
				break;
			case SYNC_PUBLIC_DB_DELETE_DATA:
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

void DB_Manager::load_player(int cid, int sid, Bit_Buffer &buffer) {
	std::string account = "";
	std::string index_name = "role_id";
	std::string table_name = "game.role";
	std::string query_name = "account";
	buffer.read_str(account);
	DB_Struct *db_struct = STRUCT_MANAGER->get_table_struct(table_name);
	if(db_struct == nullptr) {
		LOG_ERROR("db_struct %s is null", table_name.c_str());
		return;
	}
	int64_t role_id = DATA_MANAGER->select_table_index(DB_GAME, db_struct, query_name, account);
	Msg_Head msg_head;
	msg_head.eid = 1;
	msg_head.cid = cid;
	msg_head.msg_type = NODE_MSG;
	msg_head.sid = sid;
	if (role_id > 0) {
		msg_head.msg_id = SYNC_DB_GAME_PLAYER_INFO;
		Bit_Buffer bit_buffer;
		DATA_MANAGER->load_db_data(DB_GAME, "Player_Data", role_id, bit_buffer);
		NODE_MANAGER->send_msg(msg_head, bit_buffer.data(), bit_buffer.get_byte_size());
	} else {
		msg_head.msg_id = SYNC_ERROR_CODE;
		Bit_Buffer bit_buffer;
		bit_buffer.write_uint(1, 16); //Error_Code.NEED_CREATE_ROLE = 1
		NODE_MANAGER->send_msg(msg_head, bit_buffer.data(), bit_buffer.get_byte_size());
	}
}

void DB_Manager::save_player(int cid, int sid, Bit_Buffer &buffer) {
	bool logout = buffer.read_bool();
	DATA_MANAGER->save_db_data(SAVE_DB_CLEAR_CACHE, DB_GAME, "Player_Data", buffer);
	if (logout) {
		Msg_Head msg_head;
		msg_head.eid = 1;
		msg_head.cid = cid;
		msg_head.msg_id = SYNC_ERROR_CODE;
		msg_head.msg_type = NODE_MSG;
		msg_head.sid = sid;

		Bit_Buffer bit_buffer;
		bit_buffer.write_uint(11, 16); //Error_Code.PLAYER_SAVE_COMPLETE = 11
		NODE_MANAGER->send_msg(msg_head, bit_buffer.data(), bit_buffer.get_byte_size());
	}
}

void load_public_data(int cid, int sid, Bit_Buffer &buffer) {
	Msg_Head msg_head;
	msg_head.eid = 1;
	msg_head.cid = cid;
	msg_head.msg_id = SYNC_PUBLIC_DB_SAVE_DATA;
	msg_head.msg_type = NODE_MSG;
	msg_head.sid = sid;
	uint data_type = buffer.read_uint(4);
	Bit_Buffer bit_buffer;
	bit_buffer.write_uint(data_type, 4);
	switch(data_type) {
	case 1:
		DATA_MANAGER->load_db_data(DB_GAME, "game.guild", 0, bit_buffer);
		break;
	case 2:
		DATA_MANAGER->load_db_data(DB_GAME, "game.rank", 0, bit_buffer);
		break;
	}
	NODE_MANAGER->send_msg(msg_head, bit_buffer.data(), bit_buffer.get_byte_size());
}

void save_public_data(int cid, int sid, Bit_Buffer &buffer) {
	uint data_type = buffer.read_uint(4);
	switch(data_type) {
	case 1:
		DATA_MANAGER->save_db_data(SAVE_CACHE_DB, DB_GAME, "game.guild", buffer);
		break;
	case 2:
		DATA_MANAGER->save_db_data(SAVE_CACHE_DB, DB_GAME, "game.rank", buffer);
		break;
	}
}

void delete_public_data(int cid, int sid, Bit_Buffer &buffer) {
	uint data_type = buffer.read_uint(4);
	switch(data_type) {
	case 1:
		DATA_MANAGER->delete_db_data(DB_GAME, "game.guild", buffer);
		break;
	case 2:
		DATA_MANAGER->delete_db_data(DB_GAME, "game.rank", buffer);
		break;
	}
}
