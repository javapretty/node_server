/*
 * DB_Manager.cpp
 *
 *  Created on: Dec 26, 2016
 *      Author: lijunliang
 */

#include "DB_Manager.h"

DB_Manager::DB_Manager(void):
	db_buffer_map_()
{

}

DB_Manager::~DB_Manager(void) { }

DB_Manager *DB_Manager::instance_;

DB_Manager *DB_Manager::instance(void) {
	if (instance_ == 0)
		instance_ = new DB_Manager;
	return instance_;
}

void DB_Manager::save_data(const char *table, int64_t index, Block_Buffer *buffer) {
//	if()
}

Block_Buffer *DB_Manager::load_data(const char *table, int64_t index) {
	return NULL;
}

