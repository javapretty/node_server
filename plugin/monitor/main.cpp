/*
 * main.cpp
 *
 *  Created on: Nov 8, 2016
 *      Author: zhangyalei
 */

#include "include/v8.h"
#include "Node_Define.h"
#include "Monitor_Manager.h"

using namespace v8;

extern "C" {
	void init(Local<ObjectTemplate> &global, Isolate *isolate) {}

	void create_thread(const Node_Info &node_info) {
		MONITOR_MANAGER->init(node_info);
		MONITOR_MANAGER->thr_create();
	}
}
