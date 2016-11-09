/*
 * main.cpp
 *
 *  Created on: Nov 1, 2016
 *      Author: zhangyalei
 */

#include "DB_Manager.h"
#include "DB_Wrap.h"

using namespace v8;

extern "C" {
	void init(Local<ObjectTemplate> &global, Isolate *isolate) {
		global->Set(String::NewFromUtf8(isolate, "init_db", NewStringType::kNormal).ToLocalChecked(),
			FunctionTemplate::New(isolate, init_db));
	}

	void push_buffer(Byte_Buffer *buffer) {
		DB_MANAGER->push_buffer(buffer);
	}
}
