/*
 * Data_Manager.cpp
 *
 *  Created on: 2016年10月27日
 *      Author: zhangyalei
 */

#include "Base_Enum.h"
#include "Base_Function.h"
#include "Mysql_Operator.h"
#include "Data_Manager.h"

#ifdef MONGO_DB_IMPLEMENT
	#include "Mongo_Operator.h"
#endif

Data_Manager::Data_Manager(void) :
	db_operator_map_(get_hash_table_size(512)),
	db_buffer_map_(get_hash_table_size(512)),
	runtime_data_map_(get_hash_table_size(4096))
{ }

Data_Manager::~Data_Manager(void) { }

Data_Manager *Data_Manager::instance_;

Data_Manager *Data_Manager::instance(void) {
	if (instance_ == 0)
		instance_ = new Data_Manager;
	return instance_;
}

int Data_Manager::init_db_operator() {
#ifdef MONGO_DB_IMPLEMENT
	Mongo_Operator *mongo = new Mongo_Operator();
	db_operator_map_[MONGO] = mongo;
#endif
	Mysql_Operator *mysql = new Mysql_Operator();
	db_operator_map_[MYSQL] = mysql;
	return 0;
}

DB_Operator *Data_Manager::db_operator(int type) {
	DB_Operator_Map::iterator iter = db_operator_map_.find(type / 1000);
	if(iter != db_operator_map_.end())
		return iter->second;
	LOG_ERROR("DB_TYPE %d not found!", type);
	return nullptr;
}

int Data_Manager::save_db_data(int db_id, DB_Struct *db_struct, Byte_Buffer *buffer, int flag) {
	int64_t index = 0;
	buffer->peek_int64(index);
	Record_Buffer_Map *record_buffer_map = get_record_map(db_id, db_struct->table_name());

	Record_Buffer_Map::iterator it = record_buffer_map->find(index);
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
	return 0;
}

int Data_Manager::load_db_data(int db_id, DB_Struct *db_struct, int64_t index, std::vector<Byte_Buffer *> &buffer_vec) {
	Record_Buffer_Map *record_buffer_map = get_record_map(db_id, db_struct->table_name());
	int len = 0;
	if(index == 0) {
		if(record_buffer_map->empty()) {
			DB_OPERATOR(db_id)->load_data(db_id, db_struct, index, buffer_vec);
			for(std::vector<Byte_Buffer *>::iterator iter = buffer_vec.begin();
					iter != buffer_vec.end(); iter++){
				int64_t ind = 0;
				(*iter)->peek_int64(ind);
				(*	record_buffer_map)[ind] = (*iter);
			}
		}
		else {
			for(Record_Buffer_Map::iterator iter = record_buffer_map->begin();
					iter != record_buffer_map->end(); iter++){
				buffer_vec.push_back(iter->second);
			}
		}
	}
	else {
		Record_Buffer_Map::iterator iter;
		if((iter = record_buffer_map->find(index)) == record_buffer_map->end()){
			DB_OPERATOR(db_id)->load_data(db_id, db_struct, index, buffer_vec);
			for(std::vector<Byte_Buffer *>::iterator iter = buffer_vec.begin();
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

int Data_Manager::delete_db_data(int db_id, DB_Struct *db_struct, int64_t index) {
	Record_Buffer_Map *record_buffer_map = get_record_map(db_id, db_struct->table_name());
	record_buffer_map->erase(index);
	return 0;
}

void Data_Manager::set_runtime_data(int64_t index, DB_Struct *db_struct, Byte_Buffer *buffer) {
	Runtime_Data_Map::iterator iter = runtime_data_map_.find(index);
	if(iter == runtime_data_map_.end()) {
		runtime_data_map_[index] = buffer;
	}
	else {
		push_buffer(iter->second);
		iter->second = buffer;
	}
}

Byte_Buffer *Data_Manager::get_runtime_data(int64_t index, DB_Struct *db_struct) {
	Runtime_Data_Map::iterator iter = runtime_data_map_.find(index);
	if(iter != runtime_data_map_.end()) {
		return iter->second;
	}
	return nullptr;
}

void Data_Manager::delete_runtime_data(int64_t index) {
	Runtime_Data_Map::iterator iter = runtime_data_map_.find(index);
	if(iter != runtime_data_map_.end()) {
		push_buffer(iter->second);
		runtime_data_map_.erase(iter);
	}
}

Data_Manager::Record_Buffer_Map *Data_Manager::get_record_map(int db_id, std::string table_name) {
	Table_Buffer_Map*table_buffer_map = NULL;
	DB_Buffer_Map::iterator iter = db_buffer_map_.find(db_id);
	if(iter == db_buffer_map_.end()){
		table_buffer_map = new Table_Buffer_Map(get_hash_table_size(512));
		db_buffer_map_[db_id] = table_buffer_map;
	}
	else {
		table_buffer_map = iter->second;
	}

	Record_Buffer_Map *record_buffer_map = NULL;
	Table_Buffer_Map::iterator it = table_buffer_map->find(table_name);
	if(it == table_buffer_map->end()){
		record_buffer_map = new Record_Buffer_Map(get_hash_table_size(10000));
		(*table_buffer_map)[table_name] = record_buffer_map;
	}
	else {
		record_buffer_map = it->second;
	}
	return record_buffer_map;
}
