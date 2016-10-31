/*
 * V8_Manager.cpp
 *
 *  Created on: Jan 22, 2016
 *      Author: zhangyalei
 */

#include "Base_V8.h"
#include "V8_Wrap.h"
#include "Node_Manager.h"
#include "V8_Manager.h"
#include "Struct_Manager.h"

V8_Manager::V8_Manager(void) :
	platform_(nullptr),
	isolate_(nullptr),
	plugin_handle_map_(64)
{ }

V8_Manager::~V8_Manager(void) {
	fini();
}

V8_Manager *V8_Manager::instance_ = nullptr;

V8_Manager *V8_Manager::instance(void) {
	if (! instance_)
		instance_ = new V8_Manager;
	return instance_;
}

void V8_Manager::run_handler(void) {
	process_list();
}

int V8_Manager::process_list(void) {
	//初始化V8
	V8::InitializeICU();
	V8::InitializeExternalStartupData("");
	platform_ = platform::CreateDefaultPlatform();
	V8::InitializePlatform(platform_);
	V8::Initialize();

	//初始化v8虚拟机
	ArrayBufferAllocator allocator;
	Isolate::CreateParams create_params;
	create_params.array_buffer_allocator = &allocator;
	isolate_ = Isolate::New(create_params);
	//进入v8的Isolate内部，才能使用V8引擎
	Isolate::Scope isolate_scope(isolate_);
	//创建V8执行环境
	HandleScope handle_scope(isolate_);
	Local<Context> context = create_context(isolate_);
	context_.Reset(isolate_, context);
	//进入V8执行环境内部
	Context::Scope context_scope(context);
	//运行脚本
	run_script(isolate_, node_info_.script_path.c_str());

	//脚本初始化
	Local<String> func_name = String::NewFromUtf8(isolate_, "init", NewStringType::kNormal).ToLocalChecked();
	Local<Value> func_value;
	if (!context->Global()->Get(context, func_name).ToLocal(&func_value) || !func_value->IsFunction()) {
		fini();
		LOG_FATAL("can't find function 'init'");
		return -1;
	}
	const int argc = 1;
	Local<Value> argv[argc];
	Msg_Struct *msg_struct = STRUCT_MANAGER->get_msg_struct("Node_Info");
	if (msg_struct) {
		Byte_Buffer buffer;
		node_info_.serialize(buffer);
		argv[0] = msg_struct->build_object(isolate_, buffer);
	} else {
		fini();
		return -1;
	}
	Local<Function> js_func = Local<Function>::Cast(func_value);
	js_func->Call(context, context->Global(), argc, argv);
	
	Byte_Buffer *buffer = nullptr;
	while (1) {
		bool all_empty = true;

		//脚本处理cid掉线
		int drop_cid = NODE_MANAGER->pop_drop_cid();
		if(drop_cid != -1) {
			all_empty = false;
			NODE_MANAGER->remove_session(drop_cid);
			Local<String> func_name = String::NewFromUtf8(isolate_, "on_drop", NewStringType::kNormal).ToLocalChecked();
			Local<Value> func_value;
			if (!context->Global()->Get(context, func_name).ToLocal(&func_value) || !func_value->IsFunction()) {
				LOG_ERROR("can't find function 'on_drop'");
				continue;
			}
			const int argc = 1;
			Local<Value> argv[argc] = {Int32::New(isolate_, drop_cid)};
			Local<Function> js_func = Local<Function>::Cast(func_value);
			js_func->Call(context, context->Global(), argc, argv);
		}

		//脚本处理消息buffer
		buffer = NODE_MANAGER->pop_buffer();
		if (buffer) {
			all_empty = false;
			NODE_MANAGER->add_recv_bytes(buffer->readable_bytes() - 8);
			int32_t eid = 0;
			int32_t cid = 0;
			uint8_t protocol = 0;
			uint8_t client_msg = 0;
			uint8_t msg_id = 0;
			uint8_t msg_type = 0;
			uint32_t sid = 0;
			buffer->read_int32(eid);
			buffer->read_int32(cid);
			buffer->read_uint8(protocol);

			const int argc = 1;
			Local<Value> argv[argc];
			if (protocol == TCP) {
				//tcp消息包
				buffer->read_uint8(client_msg);
				buffer->read_uint8(msg_id);
				if (client_msg) {
					msg_type = C2S;
				} else {
					buffer->read_uint8(msg_type);
					buffer->read_uint32(sid);
				}
				NODE_MANAGER->add_msg_count(msg_id);

				//gate消息直接发送，不经过脚本层
				if (NODE_MANAGER->node_info().node_type == GATE_SERVER
						&& (msg_type == C2S || msg_type == NODE_S2C)
						&& msg_id >= 4 && msg_id <= 255) {
					NODE_MANAGER->transmit_msg(eid, cid, msg_id, msg_type, sid, buffer);
					NODE_MANAGER->push_buffer(eid, cid, buffer);
					continue;
				}

				std::string struct_name = get_struct_name(msg_type, msg_id);
				Msg_Struct *msg_struct = STRUCT_MANAGER->get_msg_struct(struct_name);
				if (msg_struct) {
					argv[0] = msg_struct->build_msg_object(isolate_, cid, msg_id, msg_type, sid, *buffer);
				} else {
					continue;
				}
			} else if (protocol == HTTP) {
				//http消息包
				std::string post_data = "";
				buffer->read_string(post_data);
				Json::Value value;
			  Json::Reader reader(Json::Features::strictMode());
				if ( !reader.parse(post_data, value) || !value["msg_id"].isInt()) {
					LOG_ERROR("http post data:%s format error or msg_id not int", post_data.c_str());
					continue;
				}

				msg_id = value["msg_id"].asInt();
				msg_type = HTTP_MSG;
				NODE_MANAGER->add_msg_count(msg_id);
				std::string struct_name = get_struct_name(msg_type, msg_id);
				Msg_Struct *msg_struct = STRUCT_MANAGER->get_msg_struct(struct_name);
				if (msg_struct) {
					argv[0] = msg_struct->build_http_msg_object(isolate_, cid, msg_id, msg_type, value);
				} else {
					continue;
				}
			}

			Local<String> func_name = String::NewFromUtf8(isolate_, "on_msg", NewStringType::kNormal).ToLocalChecked();
			Local<Value> func_value;
			if (!context->Global()->Get(context, func_name).ToLocal(&func_value) || !func_value->IsFunction()) {
				LOG_ERROR("can't find function 'on_msg'");
				continue;
			}
			Local<Function> js_func = Local<Function>::Cast(func_value);
			js_func->Call(context, context->Global(), argc, argv);
			NODE_MANAGER->push_buffer(eid, cid, buffer);
		}
		
		//脚本处理定时器
		if (!timer_list_.empty()) {
			all_empty = false;
			Local<String> func_name = String::NewFromUtf8(isolate_, "on_tick", NewStringType::kNormal).ToLocalChecked();
			Local<Value> func_value;
			if (!context->Global()->Get(context, func_name).ToLocal(&func_value) || !func_value->IsFunction()) {
				LOG_ERROR("can't find function 'on_tick'");
				continue;
			}

			int timer_id = timer_list_.pop_front();
			const int argc = 1;
			Local<Value> argv[argc] = {Int32::New(isolate_, timer_id)};
			Local<Function> js_func = Local<Function>::Cast(func_value);
			js_func->Call(context, context->Global(), argc, argv);
		}

		if (all_empty) {
			Time_Value::sleep(Time_Value(0, 100));
		}
	}
	
	return 0;
}

int V8_Manager::init(const Node_Info &node_info) {
	node_info_ = node_info;
	for(uint i = 0; i < node_info_.plugin_list.size(); i++) {
		const char* path = node_info_.plugin_list[i].c_str();
		void *handle = dlopen(path, RTLD_NOW);
		if(handle == nullptr) {
			LOG_FATAL("can't open the plugin %s, dlerror:%s", path, dlerror());
		}
		plugin_handle_map_[path] = handle;
	}
	return 0;
}

int V8_Manager::fini(void) {
	//释放V8资源
	isolate_->LowMemoryNotification();
	context_.Reset();
	V8::Dispose();
	V8::ShutdownPlatform();
	delete platform_;
	for(Plugin_Handle_Map::iterator iter = plugin_handle_map().begin();
				iter != plugin_handle_map().end(); iter++){
		dlclose(iter->second);
	}
	plugin_handle_map().clear();
	return 0;
}
