/*
 * main.cpp
 *
 *  Created on: Nov 9, 2016
 *      Author: zhangyalei
 */

#include "Data_Manager.h"
#include "Log_Manager.h"
#include "include/v8.h"

using namespace v8;

extern "C" {
	void init(Local<ObjectTemplate> &global, Isolate *isolate) {}

	void create_thread(const Node_Info &node_info) {
		DATA_MANAGER->init_db_operator();
		LOG_MANAGER->init(node_info);
		LOG_MANAGER->thr_create();
	}

	void push_buffer(Byte_Buffer *buffer) {
		LOG_MANAGER->push_buffer(buffer);
	}
}
