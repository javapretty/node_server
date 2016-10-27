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
	db_buffer_map_(get_hash_table_size(512))
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

void DB_Manager::save_data(int db_id, DB_Struct *db_struct, Block_Buffer *buffer) {
	int64_t index = 0;
	buffer->peek_int64(index);
	TABLE_BUFFER_MAP *table_buffer_map = NULL;
	DB_BUFFER_MAP::iterator iter = db_buffer_map_.find(db_id);
	if(iter == db_buffer_map_.end()){
		table_buffer_map = new TABLE_BUFFER_MAP;
		db_buffer_map_[db_id] = table_buffer_map;
	}
	else {
		table_buffer_map = iter->second;
	}

	RECORD_BUFFER_MAP *record_buffer_map = NULL;
	TABLE_BUFFER_MAP::iterator ite = table_buffer_map->find(db_struct->table_name());
	if(ite == table_buffer_map->end()){
		record_buffer_map = new RECORD_BUFFER_MAP;
		table_buffer_map[db_struct->table_name()] = record_buffer_map;
	}
	else {
		record_buffer_map = ite->second;
	}

	RECORD_BUFFER_MAP::iterator it = record_buffer_map->find(index);
	if(it == record_buffer_map->end()){
		record_buffer_map[index] = buffer;
	}
	else {
		Block_Buffer *buf = it->second;
		push_buffer(buf);
		it->second = buffer;
	}
}

Block_Buffer *DB_Manager::load_data(int db_id, DB_Struct *db_struct, int64_t index) {
	TABLE_BUFFER_MAP *table_buffer_map = NULL;
	DB_BUFFER_MAP::iterator iter = db_buffer_map_.find(db_id);
	if(iter == db_buffer_map_.end()){
		table_buffer_map = new TABLE_BUFFER_MAP;
		db_buffer_map_[db_id] = table_buffer_map;
	}
	else {
		table_buffer_map = iter->second;
	}

	RECORD_BUFFER_MAP *record_buffer_map = NULL;
	TABLE_BUFFER_MAP::iterator ite = table_buffer_map->find(db_struct->table_name());
	if(ite == table_buffer_map->end()){
		record_buffer_map = new RECORD_BUFFER_MAP;
		table_buffer_map[db_struct->table_name()] = record_buffer_map;
	}
	else {
		record_buffer_map = ite->second;
	}

	RECORD_BUFFER_MAP::iterator it = record_buffer_map->find(index);
	if(it == table_buffer_map->end()){
		Block_Buffer *buffer = DB_OPERATOR(db_id)->load_data(db_id, db_struct, index);
		table_buffer_map[index] = buffer;
		return buffer;
	}
	else {
		return it->second;
	}
}
