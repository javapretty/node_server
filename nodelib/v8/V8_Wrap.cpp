/*
 * V8_Wrap.cpp
 *
 *  Created on: Feb 1, 2016
 *      Author: zhangyalei
 */

#include <sstream>
#include "Data_Manager.h"
#include "Struct_Manager.h"
#include "Node_Timer.h"
#include "Node_Manager.h"
#include "V8_Manager.h"
#include "V8_Base.h"
#include "V8_Wrap.h"

Local<Context> create_context(Isolate* isolate) {
	Local<ObjectTemplate> global = ObjectTemplate::New(isolate);
	global->Set(String::NewFromUtf8(isolate, "get_proc_info", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, get_proc_info));
	global->Set(String::NewFromUtf8(isolate, "get_heap_info", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, get_heap_info));

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
	
	global->Set(String::NewFromUtf8(isolate, "log_debug", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, log_debug));
	global->Set(String::NewFromUtf8(isolate, "log_info", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, log_info));
	global->Set(String::NewFromUtf8(isolate, "log_warn", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, log_warn));
	global->Set(String::NewFromUtf8(isolate, "log_error", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, log_error));
	global->Set(String::NewFromUtf8(isolate, "log_trace", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, log_trace));

	global->Set(String::NewFromUtf8(isolate, "fork_process", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, fork_process));
	global->Set(String::NewFromUtf8(isolate, "get_node_stack", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, get_node_stack));
	global->Set(String::NewFromUtf8(isolate, "get_node_status", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, get_node_status));
	global->Set(String::NewFromUtf8(isolate, "register_timer", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, register_timer));
	global->Set(String::NewFromUtf8(isolate, "send_msg", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, send_msg));
	global->Set(String::NewFromUtf8(isolate, "close_client", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, close_client));

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

	HandleScope handle_scope(args.GetIsolate());
	Local<Context> context(args.GetIsolate()->GetCurrentContext());
	int node_type = args[0]->Int32Value(context).FromMaybe(0);
	int node_id = args[1]->Int32Value(context).FromMaybe(0);
	int endpoint_gid = args[2]->Int32Value(context).FromMaybe(0);
	String::Utf8Value name_str(args[3]->ToString(context).ToLocalChecked());
	std::string node_name = to_string(name_str);
	NODE_MANAGER->fork_process(node_type, node_id, endpoint_gid, node_name);
}

void get_node_stack(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 4) {
		LOG_ERROR("get_node_stack args error, length: %d\n", args.Length());
		return;
	}

	HandleScope handle_scope(args.GetIsolate());
	Local<Context> context(args.GetIsolate()->GetCurrentContext());
	int node_id = args[0]->Int32Value(context).FromMaybe(0);
	int eid = args[1]->Int32Value(context).FromMaybe(0);
	int cid = args[2]->Int32Value(context).FromMaybe(0);
	int sid = args[3]->Int32Value(context).FromMaybe(0);
	NODE_MANAGER->get_node_stack(node_id, eid, cid, sid);
}

void get_node_status(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context(isolate->GetCurrentContext());

	Node_Info node_info = NODE_MANAGER->node_info();
	Node_Status node_status = NODE_MANAGER->get_node_status();
	int i = 0;
	v8::Local<v8::Array> msg_list = v8::Array::New(isolate, NODE_MANAGER->msg_count_map().size());
	for(Node_Manager::Msg_Count_Map::const_iterator iter = NODE_MANAGER->msg_count_map().begin();
		iter != NODE_MANAGER->msg_count_map().end(); ++iter) {
		v8::Local<v8::Object> msg_object = Object::New(isolate);
		msg_object->Set(context, String::NewFromUtf8(isolate, "msg_id", NewStringType::kNormal).ToLocalChecked(),
			Number::New(isolate, iter->first));
		msg_object->Set(context, String::NewFromUtf8(isolate, "msg_count", NewStringType::kNormal).ToLocalChecked(),
			Number::New(isolate, iter->second));
		msg_list->Set(context, i++, msg_object).FromJust();
	}

	v8::Local<v8::Object> object = Object::New(isolate);
	object->Set(context, String::NewFromUtf8(isolate, "node_type", NewStringType::kNormal).ToLocalChecked(),
		Number::New(isolate, node_info.node_type));
	object->Set(context, String::NewFromUtf8(isolate, "node_id", NewStringType::kNormal).ToLocalChecked(),
		Number::New(isolate, node_info.node_id));
	object->Set(context, String::NewFromUtf8(isolate, "node_name", NewStringType::kNormal).ToLocalChecked(),
		String::NewFromUtf8(isolate, node_info.node_name.c_str(), NewStringType::kNormal).ToLocalChecked());
	object->Set(context, String::NewFromUtf8(isolate, "start_time", NewStringType::kNormal).ToLocalChecked(),
		Number::New(isolate, node_status.start_time));
	object->Set(context, String::NewFromUtf8(isolate, "total_send", NewStringType::kNormal).ToLocalChecked(),
		Number::New(isolate, node_status.total_send));
	object->Set(context, String::NewFromUtf8(isolate, "total_recv", NewStringType::kNormal).ToLocalChecked(),
		Number::New(isolate, node_status.total_recv));
	object->Set(context, String::NewFromUtf8(isolate, "send_per_sec", NewStringType::kNormal).ToLocalChecked(),
		Number::New(isolate, node_status.send_per_sec));
	object->Set(context, String::NewFromUtf8(isolate, "recv_per_sec", NewStringType::kNormal).ToLocalChecked(),
		Number::New(isolate, node_status.recv_per_sec));
	object->Set(context, String::NewFromUtf8(isolate, "task_count", NewStringType::kNormal).ToLocalChecked(),
		Number::New(isolate, node_status.task_count));
	object->Set(context, String::NewFromUtf8(isolate, "msg_list", NewStringType::kNormal).ToLocalChecked(),
		msg_list);
	//重置node状态
	NODE_MANAGER->reset_node_status();

	args.GetReturnValue().Set(object);
}

void register_timer(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 3) {
		LOG_ERROR("register timer args error, length: %d\n", args.Length());
		return;
	}

	HandleScope handle_scope(args.GetIsolate());
	Local<Context> context(args.GetIsolate()->GetCurrentContext());
	int timer_id = args[0]->Int32Value(context).FromMaybe(0);
	int interval = args[1]->Int32Value(context).FromMaybe(0);
	int first_tick = args[2]->Int32Value(context).FromMaybe(0);
	NODE_TIMER->register_handler(timer_id, interval, first_tick);
}

void send_msg(const FunctionCallbackInfo<Value>& args) {
	HandleScope handle_scope(args.GetIsolate());
	Local<Context> context(args.GetIsolate()->GetCurrentContext());

	Msg_Head msg_head;
	msg_head.eid = args[0]->Int32Value(context).FromMaybe(0);
	msg_head.cid = args[1]->Int32Value(context).FromMaybe(0);
	msg_head.msg_id = args[2]->Int32Value(context).FromMaybe(0);
	msg_head.msg_type = args[3]->Int32Value(context).FromMaybe(0);
	msg_head.sid = args[4]->Uint32Value(context).FromMaybe(0);

	int ret = 0;
	Bit_Buffer buffer;
	std::string struct_name = get_struct_name(msg_head.msg_type, msg_head.msg_id);
	Msg_Struct *msg_struct = STRUCT_MANAGER->get_msg_struct(struct_name);
	if (msg_struct) {
		if(msg_head.msg_type == HTTP_MSG) {
			msg_head.protocol = HTTP;
			Json::Value root;
			root["msg_id"] = Json::Value(msg_head.msg_id);
			msg_struct->build_json_value(args.GetIsolate(), msg_struct->field_vec(), args[5]->ToObject(context).ToLocalChecked(), root);
			Json::FastWriter fw;
			std::string json_str = fw.write(root);
			buffer.write_str(json_str.c_str());
		}
		else if(msg_head.msg_type == WS_S2C) {
			msg_head.protocol = WEBSOCKET;
			Json::Value root;
			root["msg_id"] = Json::Value(msg_head.msg_id);
			msg_struct->build_json_value(args.GetIsolate(), msg_struct->field_vec(), args[5]->ToObject(context).ToLocalChecked(), root);
			Json::FastWriter fw;
			std::string json_str = fw.write(root);
			buffer.write_str(json_str.c_str());
		}
		else {
			msg_head.protocol = TCP;
			ret = msg_struct->build_bit_buffer(args.GetIsolate(), msg_struct->field_vec(), buffer, args[5]->ToObject(context).ToLocalChecked());
		}
	}
	else {
		ret = -1;
	}

	if(ret >= 0) {
		NODE_MANAGER->send_msg(msg_head, buffer.data(), buffer.get_byte_size());
	}
	else {
		LOG_ERROR("send_msg error, struct_name:%s, eid:%d, cid:%d, msg_id:%d, msg_type:%d, sid:%u",
				struct_name.c_str(), msg_head.eid, msg_head.cid, msg_head.msg_id, msg_head.msg_type, msg_head.sid);
	}
}

void close_client(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 2) {
		LOG_ERROR("close_client args error, length: %d\n", args.Length());
		return;
	}

	HandleScope handle_scope(args.GetIsolate());
	Local<Context> context(args.GetIsolate()->GetCurrentContext());
	int eid = args[0]->Int32Value(context).FromMaybe(0);
	int cid = args[1]->Int32Value(context).FromMaybe(0);
	V8_MANAGER->push_drop(eid, cid);
}
