/*
 * main.cpp
 *
 *  Created on: Nov 9, 2016
 *      Author: zhangyalei
 */

#include "Hot_Update.h"
#include "Node_Manager.h"
#include "include/v8.h"

using namespace v8;

extern "C" {
	void init(Local<ObjectTemplate> &global, Isolate *isolate) {}

	void create_thread(const Node_Info &node_info) {
		HOT_UPDATE->thr_create();
	}
}
