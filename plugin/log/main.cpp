/*
 * main.cpp
 *
 *  Created on: Nov 9, 2016
 *      Author: zhangyalei
 */

#include "Log_Wrap.h"
#include "Log_Manager.h"

using namespace v8;

extern "C" {
	void init(Local<ObjectTemplate> &global, Isolate *isolate) {
		global->Set(String::NewFromUtf8(isolate, "init_log", NewStringType::kNormal).ToLocalChecked(),
			FunctionTemplate::New(isolate, init_log));
	}

	void push_buffer(Byte_Buffer *buffer) {
		LOG_MANAGER->push_buffer(buffer);
	}
}
