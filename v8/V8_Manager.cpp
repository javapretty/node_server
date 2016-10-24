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

V8_Manager::V8_Manager(void) : platform_(nullptr), isolate_(nullptr) { }

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
	run_script(isolate_, script_path_.c_str());

	Block_Buffer *buffer = nullptr;
	int tick_time = 0;
	while (1) {
		bool all_empty = true;

		buffer = NODE_MANAGER->pop_buffer();
		if (buffer) {
			all_empty = false;

			//获取js函数
			Local<String> func_name = String::NewFromUtf8(isolate_, "on_msg", NewStringType::kNormal).ToLocalChecked();
			Local<Value> func_value;
			if (!context->Global()->Get(context, func_name).ToLocal(&func_value) || !func_value->IsFunction()) {
				LOG_ERROR("can't find function 'on_msg'");
				return -1;
			}
			
			int32_t endpoint_id = 0;
			int32_t cid = 0;
			uint8_t compress = 0;
			uint8_t client_msg = 0;
			uint8_t msg_id = 0;
			uint8_t msg_type = 0;
			int32_t extra = 0;
			buffer->read_int32(endpoint_id);
			buffer->read_int32(cid);
			buffer->read_uint8(compress);
			buffer->read_uint8(client_msg);
			buffer->read_uint8(msg_id);
			if (client_msg) {
				msg_type = C2S;
			} else {
				buffer->read_uint8(msg_type);
				buffer->read_int32(extra);
			}
			NODE_MANAGER->add_msg_count(msg_id);

			const int argc = 1;
			Local<Value> argv[argc];
			std::string struct_name = get_struct_name(msg_type, msg_id);
			Msg_Struct *msg_struct = STRUCT_MANAGER->get_msg_struct(struct_name);
			if (msg_struct != nullptr) {
				argv[0] = msg_struct->build_msg_object(isolate_, cid, msg_type, msg_id, extra, *buffer);
			}

			//转换成js函数对象
			Local<Function> js_func = Local<Function>::Cast(func_value);
			js_func->Call(context, context->Global(), argc, argv);

			NODE_MANAGER->push_buffer(endpoint_id, cid, buffer);
		}
		
		tick_time = NODE_MANAGER->pop_tick();
		if (tick_time > 0) {
			all_empty = false;
			
			//获取js函数
			Local<String> func_name = String::NewFromUtf8(isolate_, "on_tick", NewStringType::kNormal).ToLocalChecked();
			Local<Value> func_value;
			if (!context->Global()->Get(context, func_name).ToLocal(&func_value) || !func_value->IsFunction()) {
				LOG_ERROR("can't find function 'on_msg'");
				return -1;
			}

			const int argc = 1;
			Local<Value> argv[argc] = {Int32::New(isolate_, tick_time)};
			//转换成js函数对象
			Local<Function> js_func = Local<Function>::Cast(func_value);
			js_func->Call(context, context->Global(), argc, argv);
		}

		if (all_empty) {
			Time_Value::sleep(SLEEP_TIME);
		}
	}
	
	return 0;
}

int V8_Manager::init(const char *script_path) {
	script_path_ = script_path;
	return 0;
}

int V8_Manager::fini(void) {
	//释放V8资源
	isolate_->LowMemoryNotification();
	context_.Reset();
	V8::Dispose();
	V8::ShutdownPlatform();
	delete platform_;
	return 0;
}
