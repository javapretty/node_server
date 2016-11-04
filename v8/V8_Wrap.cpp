/*
 * V8_Wrap.cpp
 *
 *  Created on: Feb 1, 2016
 *      Author: zhangyalei
 */

#include <sstream>
#include "Base_V8.h"
#include "Msg_Struct.h"
#include "Struct_Manager.h"
#include "Node_Timer.h"
#include "Node_Manager.h"
#include "V8_Manager.h"
#include "V8_Wrap.h"

std::string get_struct_name(int msg_type, int msg_id) {
	std::ostringstream stream;
	switch(msg_type) {
	case C2S:
	case NODE_C2S:
		stream << "c2s_" << msg_id;
		break;
	case S2C:
	case NODE_S2C:
		stream << "s2c_" << msg_id;
		break;
	case NODE_MSG:
		stream << "node_" << msg_id;
		break;
	case HTTP_MSG:
		stream << "http_" << msg_id;
		break;
	default: {
		break;
	}
	}
	return stream.str();
}

Local<Context> create_context(Isolate* isolate) {
	Local<ObjectTemplate> global = ObjectTemplate::New(isolate);
	global->Set(String::NewFromUtf8(isolate, "fork_process", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, fork_process));
	global->Set(String::NewFromUtf8(isolate, "require", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, require));
	global->Set(String::NewFromUtf8(isolate, "read_json", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, read_json));
	global->Set(String::NewFromUtf8(isolate, "read_xml", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, read_xml));
	global->Set(String::NewFromUtf8(isolate, "hash", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, hash));
	global->Set(String::NewFromUtf8(isolate, "generate_token", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, generate_token));
	global->Set(String::NewFromUtf8(isolate, "log_trace", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, log_trace));
	global->Set(String::NewFromUtf8(isolate, "log_debug", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, log_debug));
	global->Set(String::NewFromUtf8(isolate, "log_info", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, log_info));
	global->Set(String::NewFromUtf8(isolate, "log_warn", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, log_warn));
	global->Set(String::NewFromUtf8(isolate, "log_error", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, log_error));

	global->Set(String::NewFromUtf8(isolate, "register_timer", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, register_timer));
	global->Set(String::NewFromUtf8(isolate, "send_msg", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, send_msg));
	global->Set(String::NewFromUtf8(isolate, "add_session", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, add_session));
	global->Set(String::NewFromUtf8(isolate, "close_session", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, close_session));

	for(V8_Manager::Plugin_Handle_Map::iterator iter = V8_MANAGER->plugin_handle_map().begin();
			iter != V8_MANAGER->plugin_handle_map().end(); iter++){
		void *handle = iter->second;
		void (*init_func)(Local<ObjectTemplate>&, Isolate*);
		init_func = (void (*)(Local<ObjectTemplate>&, Isolate*))dlsym(handle, "init");
		if(init_func == nullptr) {
			LOG_FATAL("plugin %s can't find init function, dlerror:%s", iter->first, dlerror());
		}
		init_func(global, isolate);
	}

	return Context::New(isolate, NULL, global);
}

void fork_process(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 4) {
		LOG_ERROR("fork_process args error, length: %d\n", args.Length());
		return;
	}

	Local<Context> context(args.GetIsolate()->GetCurrentContext());
	int node_type = args[0]->Int32Value(context).FromMaybe(0);
	int node_id = args[1]->Int32Value(context).FromMaybe(0);
	int endpoint_gid = args[2]->Int32Value(context).FromMaybe(0);
	String::Utf8Value name_str(args[3]->ToString(context).ToLocalChecked());
	std::string node_name = to_string(name_str);
	//fork process via daemon_server
	NODE_MANAGER->fork_process(node_type, node_id, endpoint_gid, node_name);
}

void register_timer(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 3) {
		LOG_ERROR("register timer args error, length: %d\n", args.Length());
		return;
	}

	Local<Context> context(args.GetIsolate()->GetCurrentContext());
	int timer_id = args[0]->Int32Value(context).FromMaybe(0);
	int interval = args[1]->Int32Value(context).FromMaybe(0);
	int first_tick = args[2]->Int32Value(context).FromMaybe(0);
	NODE_TIMER->register_handler(timer_id, interval, first_tick);
	LOG_INFO("register_timer, timer_id:%d, interval:%d", timer_id, interval);
}

void send_msg(const FunctionCallbackInfo<Value>& args) {
	Local<Context> context(args.GetIsolate()->GetCurrentContext());

	int eid = args[0]->Int32Value(context).FromMaybe(0);
	int cid = args[1]->Int32Value(context).FromMaybe(0);
	int msg_id = args[2]->Int32Value(context).FromMaybe(0);
	int msg_type = args[3]->Int32Value(context).FromMaybe(0);
	uint32_t sid = args[4]->Uint32Value(context).FromMaybe(0);

	Byte_Buffer buffer;
	std::string struct_name = get_struct_name(msg_type, msg_id);
	Msg_Struct *msg_struct = STRUCT_MANAGER->get_msg_struct(struct_name);
	if (msg_struct) {
		Bit_Buffer bit_buffer;
		msg_struct->build_bit_buffer(args.GetIsolate(), args[5]->ToObject(context).ToLocalChecked(), msg_struct->field_vec(), bit_buffer);
		buffer.copy(bit_buffer.data(), bit_buffer.get_byte_size());
	}
	NODE_MANAGER->send_msg(eid, cid, msg_id, msg_type, sid, &buffer);
}

void add_session(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 1 || !args[0]->IsObject()) {
		LOG_ERROR("add_session args error, length: %d\n", args.Length());
		return;
	}

	Session *session = NODE_MANAGER->pop_session();
	if (!session) {
		LOG_ERROR("pop session error");
		return;
	}

	Local<Context> context(args.GetIsolate()->GetCurrentContext());
	Local<Object> object = args[0]->ToObject(context).ToLocalChecked();
	session->client_eid = (object->Get(context,
			String::NewFromUtf8(args.GetIsolate(), "client_eid", NewStringType::kNormal).
			ToLocalChecked()).ToLocalChecked())->Int32Value(context).FromJust();
	session->client_cid = (object->Get(context,
			String::NewFromUtf8(args.GetIsolate(), "client_cid", NewStringType::kNormal).
			ToLocalChecked()).ToLocalChecked())->Int32Value(context).FromJust();
	session->game_eid = (object->Get(context,
		String::NewFromUtf8(args.GetIsolate(), "game_eid", NewStringType::kNormal).
		ToLocalChecked()).ToLocalChecked())->Int32Value(context).FromJust();
	session->game_cid = (object->Get(context,
			String::NewFromUtf8(args.GetIsolate(), "game_cid", NewStringType::kNormal).
			ToLocalChecked()).ToLocalChecked())->Int32Value(context).FromJust();
	session->sid = (object->Get(context,
			String::NewFromUtf8(args.GetIsolate(), "sid", NewStringType::kNormal).
			ToLocalChecked()).ToLocalChecked())->Int32Value(context).FromJust();

	NODE_MANAGER->add_session(session);
}

void close_session(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 2) {
		LOG_ERROR("close_session args error, length: %d\n", args.Length());
		return;
	}

	Local<Context> context(args.GetIsolate()->GetCurrentContext());
	Drop_Info drop_info;
	drop_info.eid = args[0]->Int32Value(context).FromMaybe(0);
	drop_info.cid = args[1]->Int32Value(context).FromMaybe(0);
	NODE_MANAGER->push_drop(drop_info);
}
