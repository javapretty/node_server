/*
 * main.cpp
 *
 *  Created on: Nov 1, 2016
 *      Author: zhangyalei
 */

#include "Data_Manager.h"
#include "DB_Manager.h"
#include "include/v8.h"

using namespace v8;

extern "C" {
	void init(Local<ObjectTemplate> &global, Isolate *isolate) {}

	void create_thread(const Node_Info &node_info) {
		DATA_MANAGER->init_db_operator();
		DB_MANAGER->init(node_info);
		DB_MANAGER->thr_create();
	}

	void push_buffer(Byte_Buffer *buffer) {
		DB_MANAGER->push_buffer(buffer);
	}

	void push_tick(int tick_time) {
		DB_MANAGER->push_tick(tick_time);
	}
}
