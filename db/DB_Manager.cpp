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
	db_operator_map_(get_hash_table_size(512)){ }

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
