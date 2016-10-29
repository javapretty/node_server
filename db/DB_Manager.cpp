/*
 * DB_Manager.cpp
 *
 *  Created on: 2016年10月27日
 *      Author: zhangyalei
 */

#include "Base_Enum.h"
#include "Base_Function.h"
#include "Mongo_Operator.h"
#include "Mysql_Operator.h"
#include "DB_Manager.h"

DB_Manager::DB_Manager(void) :
	db_operator_map_(get_hash_table_size(512)),
	db_buffer_map_(get_hash_table_size(512)),
	temp_data_map_(get_hash_table_size(2048))
{ }

DB_Manager::~DB_Manager(void) { }

DB_Manager *DB_Manager::instance_;

DB_Manager *DB_Manager::instance(void) {
	if (instance_ == 0)
		instance_ = new DB_Manager;
	return instance_;
}

int DB_Manager::init_db_operator() {
	Mongo_Operator *mongo = new Mongo_Operator();
	Mysql_Operator *mysql = new Mysql_Operator();
	db_operator_map_[MYSQL] = mysql;
	db_operator_map_[MONGO] = mongo;
	return 0;
}

DB_Operator *DB_Manager::db_operator(int type) {
	DB_Operator_Map::iterator iter = db_operator_map_.find(type / 1000);
	if(iter != db_operator_map_.end())
		return iter->second;
	LOG_ERROR("DB_TYPE %d not found!", type);
	return NULL;
}

void DB_Manager::save_data(int db_id, DB_Struct *db_struct, Block_Buffer *buffer, int flag) {
	int64_t index = 0;
	buffer->peek_int64(index);
	RECORD_BUFFER_MAP *record_buffer_map = get_record_map(db_id, db_struct->table_name());

	RECORD_BUFFER_MAP::iterator it = record_buffer_map->find(index);
	if(it == record_buffer_map->end()){
		switch(flag) {
		case 0:
			(*record_buffer_map)[index] = buffer;
			break;
		case 1:
			(*record_buffer_map)[index] = buffer;
			DB_OPERATOR(db_id)->save_data(db_id, db_struct, buffer);
			break;
		case 2:
			DB_OPERATOR(db_id)->save_data(db_id, db_struct, buffer);
			break;
		}
	}
	else {
		switch(flag) {
		case 0:
			push_buffer(it->second);
			it->second = buffer;
			break;
		case 1:
			push_buffer(it->second);
			it->second = buffer;
			DB_OPERATOR(db_id)->save_data(db_id, db_struct, buffer);
			break;
		case 2:
			push_buffer(it->second);
			record_buffer_map->erase(it);
			DB_OPERATOR(db_id)->save_data(db_id, db_struct, buffer);
			break;
		}
	}
}

int DB_Manager::load_data(int db_id, DB_Struct *db_struct, int64_t index, std::vector<Block_Buffer *> &buffer_vec) {
	RECORD_BUFFER_MAP *record_buffer_map = get_record_map(db_id, db_struct->table_name());
	int len = 0;
	if(index == 0) {
		if(record_buffer_map->empty()) {
			DB_OPERATOR(db_id)->load_data(db_id, db_struct, index, buffer_vec);
			for(std::vector<Block_Buffer *>::iterator iter = buffer_vec.begin();
					iter != buffer_vec.end(); iter++){
				int64_t ind = 0;
				(*iter)->peek_int64(ind);
				(*	record_buffer_map)[ind] = (*iter);
			}
		}
		else {
			for(RECORD_BUFFER_MAP::iterator iter = record_buffer_map->begin();
					iter != record_buffer_map->end(); iter++){
				buffer_vec.push_back(iter->second);
			}
		}
	}
	else {
		RECORD_BUFFER_MAP::iterator iter;
		if((iter = record_buffer_map->find(index)) == record_buffer_map->end()){
			DB_OPERATOR(db_id)->load_data(db_id, db_struct, index, buffer_vec);
			for(std::vector<Block_Buffer *>::iterator iter = buffer_vec.begin();
					iter != buffer_vec.end(); iter++){
				int64_t ind = 0;
				(*iter)->peek_int64(ind);
				(*	record_buffer_map)[ind] = (*iter);
			}
		}
		else {
			buffer_vec.push_back(iter->second);
		}
	}
	len = buffer_vec.size();
	return len;
}

int DB_Manager::delete_data(int db_id, DB_Struct *db_struct, Block_Buffer *buffer) {
	RECORD_BUFFER_MAP *record_buffer_map = get_record_map(db_id, db_struct->table_name());
	int rdx = buffer->get_read_idx();
	uint16_t len = 0;
	buffer->read_uint16(len);
	for(uint i = 0; i < len; i++){
		int64_t key = 0;
		buffer->read_int64(key);
		record_buffer_map->erase(key);
	}
	buffer->set_read_idx(rdx);
	DB_OPERATOR(db_id)->delete_data(db_id, db_struct, buffer);
	return 0;
}

void DB_Manager::set_data(int64_t index, DB_Struct *db_struct, Block_Buffer *buffer) {
	TEMP_DATA_MAP::iterator iter = temp_data_map_.find(index);
	if(iter == temp_data_map_.end()) {
		temp_data_map_[index] = buffer;
	}
	else {
		push_buffer(iter->second);
		iter->second = buffer;
	}
}

Block_Buffer *DB_Manager::get_data(int64_t index, DB_Struct *db_struct) {
	TEMP_DATA_MAP::iterator iter = temp_data_map_.find(index);
	if(iter != temp_data_map_.end()) {
		return iter->second;
	}
	return NULL;
}

void DB_Manager::clear_data(int64_t index) {
	TEMP_DATA_MAP::iterator iter = temp_data_map_.find(index);
	if(iter != temp_data_map_.end()) {
		push_buffer(iter->second);
		temp_data_map_.erase(iter);
	}
}

DB_Manager::RECORD_BUFFER_MAP *DB_Manager::get_record_map(int db_id, std::string table_name) {
	TABLE_BUFFER_MAP *table_buffer_map = NULL;
	DB_BUFFER_MAP::iterator iter = db_buffer_map_.find(db_id);
	if(iter == db_buffer_map_.end()){
		table_buffer_map = new TABLE_BUFFER_MAP(get_hash_table_size(512));
		db_buffer_map_[db_id] = table_buffer_map;
	}
	else {
		table_buffer_map = iter->second;
	}

	RECORD_BUFFER_MAP *record_buffer_map = NULL;
	TABLE_BUFFER_MAP::iterator it = table_buffer_map->find(table_name);
	if(it == table_buffer_map->end()){
		record_buffer_map = new RECORD_BUFFER_MAP(get_hash_table_size(MAX_RECORD_NUM));
		(*table_buffer_map)[table_name] = record_buffer_map;
	}
	else {
		record_buffer_map = it->second;
	}
	return record_buffer_map;
}
