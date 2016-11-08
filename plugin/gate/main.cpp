/*
 * main.cpp
 *
 *  Created on: Nov 8, 2016
 *      Author: zhangyalei
 */

#include "Gate_Wrap.h"
#include "Gate_Manager.h"

using namespace v8;

extern "C" {
	void init(Local<ObjectTemplate> &global, Isolate *isolate) {
		global->Set(String::NewFromUtf8(isolate, "init_gate", NewStringType::kNormal).ToLocalChecked(),
			FunctionTemplate::New(isolate, init_gate));
		global->Set(String::NewFromUtf8(isolate, "add_session", NewStringType::kNormal).ToLocalChecked(),
			FunctionTemplate::New(isolate, add_session));
		global->Set(String::NewFromUtf8(isolate, "remove_session", NewStringType::kNormal).ToLocalChecked(),
			FunctionTemplate::New(isolate, remove_session));
	}

	void push_buffer(Byte_Buffer *buffer) {
		GATE_MANAGER->push_buffer(buffer);
	}
}
