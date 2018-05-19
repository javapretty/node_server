/*
 * V8_Manager.cpp
 *
 *  Created on: Jan 22, 2016
 *      Author: zhangyalei
 */

#include "Base_Function.h"
#include "Struct_Manager.h"
#include "V8_Base.h"
#include "V8_Wrap.h"
#include "Node_Manager.h"
#include "V8_Manager.h"

V8_Manager::V8_Manager(void) :
	platform_(nullptr),
	isolate_(nullptr),
	drop_info_tick_(Time_Value::gettimeofday()),
	gc_tick_(Time_Value::gettimeofday()),
	file_path_(""),
	drop_eid_(0),
	drop_cid_(0),
	timer_id_(0),
	push_buffer_func(nullptr),
	push_tick_func(nullptr),
	plugin_handle_map_(get_hash_table_size(64))
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

int V8_Manager::init(const Node_Info &node_info) {
	node_info_ = node_info;
	for(uint i = 0; i < node_info_.plugin_list.size(); i++) {
		const char* path = node_info_.plugin_list[i].c_str();
		void *handle = dlopen(path, RTLD_NOW);
		if(handle == nullptr) {
			LOG_FATAL("can't open the plugin %s, dlerror:%s", path, dlerror());
		}
		plugin_handle_map_[path] = handle;
		push_buffer_func = (void (*)(Byte_Buffer *))dlsym(handle, "push_buffer");
		push_tick_func = (void (*)(int))dlsym(handle, "push_tick");

		void (*create_thread_func)(const Node_Info &);
		create_thread_func = (void (*)(const Node_Info &))dlsym(handle, "create_thread");
		if(create_thread_func != nullptr) {
			create_thread_func(node_info);
		}
	}
	return 0;
}

int V8_Manager::fini(void) {
	//调用gc函数，释放v8资源
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
	//运行全局数据脚本和主脚本
	run_script(isolate_, node_info_.global_script.c_str());
	run_script(isolate_, node_info_.main_script.c_str());
	//脚本初始化
	init_script(context);

	//热更新，掉线cid，消息，定时器循环
	Byte_Buffer *buffer = nullptr;
	while(true) {
		notify_lock_.lock();

		//put wait in while cause there can be spurious wake up (due to signal/ENITR)
		while(hotupdate_file_list_.empty() && drop_eid_list_.empty() && drop_cid_list_.empty() 
			&& buffer_list_.empty() && timer_list_.empty() && tick_list_.empty()) {
			notify_lock_.wait();
		}

		//热更新文件处理
		if(!hotupdate_file_list_.empty()) {
			file_path_ = hotupdate_file_list_.pop_front();
			process_hotupdate(context);
		}

		//node eid掉线处理
		if(!drop_eid_list_.empty()) {
			drop_eid_ = drop_eid_list_.front();
			process_drop_eid(context);
		}

		//client cid掉线处理
		if(!drop_cid_list_.empty()) {
			drop_cid_ = drop_cid_list_.pop_front();
			process_drop_cid(context);
		}

		//消息buffer处理
		buffer = buffer_list_.pop_front();
		if(buffer != nullptr) {
			int ret = process_buffer(context, buffer);
			if(ret <= 0) {
				NODE_MANAGER->push_buffer(msg_head_.eid, msg_head_.cid, buffer);
			}
			NODE_MANAGER->add_msg_count(msg_head_.msg_type * 1000 + msg_head_.msg_id);
		}
		
		//js定时器处理
		if(!timer_list_.empty()) {
			timer_id_ = timer_list_.pop_front();
			process_timer(context);
		}

		//C++定时器处理
		if(!tick_list_.empty()) {
			Time_Value now(Time_Value::gettimeofday());
			drop_info_tick(now);
			if(now - gc_tick_ >= Time_Value(60, 0)) {
				gc_tick_ = now;
				//调用gc函数，释放v8资源
				isolate_->LowMemoryNotification();
			}

			int tick_time = tick_list_.pop_front();
			if(push_tick_func != nullptr) {
				push_tick_func(tick_time);
			}
		}

		notify_lock_.unlock();
	}
	
	return 0;
}

int V8_Manager::init_script(Local<Context> context) {
	HandleScope handle_scope(isolate_);

	int ret = 0;
	const int argc = 1;
	Local<Value> argv[argc];
	Base_Struct *base_struct = STRUCT_MANAGER->get_base_struct("Node_Info");
	if (base_struct) {
		Bit_Buffer buffer;
		node_info_.serialize(buffer);
		v8::Local<v8::Object> object = Object::New(isolate_);
		base_struct->build_bit_object(isolate_, base_struct->field_vec(), buffer, object);
		argv[0] = object;
	} else {
		ret = -1;
	}
	ret = call_js_func(context, argc, argv, "init");
	if(ret < 0) {
		fini();
		LOG_FATAL("can't find function 'init'");
	}
	return ret;
}

int V8_Manager::process_hotupdate(Local<Context> context) {
	HandleScope handle_scope(isolate_);

	if(file_path_ != node_info_.global_script) {
		//全局变量脚本不能热更新
		std::string pattern = "/";
		std::vector<std::string> folder_vec = split(file_path_, pattern);
		std::string folder_name = "";
		for(uint i = 0; i < folder_vec.size() -1 ; ++i) {
			folder_name += folder_vec[i];
			folder_name += "/";
		}
		//判断被更新的文件是否在可更新文件列表内
		for(uint i = 0; i < node_info_.hotupdate_list.size(); ++i) {
			if(folder_name == node_info_.hotupdate_list[i]) {
				LOG_WARN("hot_update file_path_:%s, node_type:%d, node_id:%d, node_name:%s",  file_path_.c_str(), node_info_.node_type, node_info_.node_id, node_info_.node_name.c_str());
				if(strstr(file_path_.c_str(), ".js")) {
					run_script(isolate_, file_path_.c_str());
				}
				else if(strstr(file_path_.c_str(), ".xml")) {
					const int argc = 1;
					Local<Value> argv[argc] = {String::NewFromUtf8(isolate_, file_path_.c_str(), NewStringType::kNormal).ToLocalChecked()};
					call_js_func(context, argc, argv, "on_hotupdate");
				}
				break;
			}
		}
	}
	return 0;
}

int V8_Manager::process_drop_eid(Local<Context> context) {
	HandleScope handle_scope(isolate_);

	int ret = NODE_MANAGER->connect_server(drop_eid_);
	if (ret > 0) {
		//连接服务器成功，从掉线eid列表里面删除eid
		drop_eid_list_.pop_front();

		//通知js层处理
		const int argc = 1;
		Local<Value> argv[argc] = {Int32::New(isolate_, drop_eid_)};
		call_js_func(context, argc, argv, "on_drop_eid");
	}
	return 0;
}

int V8_Manager::process_drop_cid(Local<Context> context) {
	HandleScope handle_scope(isolate_);

	const int argc = 1;
	Local<Value> argv[argc] = {Int32::New(isolate_, drop_cid_)};
	call_js_func(context, argc, argv, "on_drop_cid");
	return 0;
}

int V8_Manager::process_buffer(Local<Context> context, Byte_Buffer *buffer) {
	HandleScope handle_scope(isolate_);

	NODE_MANAGER->add_recv_bytes(buffer->readable_bytes() - 8);
	//读取消息头
	int read_idx = buffer->get_read_idx();
	msg_head_.reset();
	buffer->read_head(msg_head_);

	//消息被过滤，不经过js层，直接由C++处理
	if (push_buffer_func != nullptr && NODE_MANAGER->msg_filter(msg_head_.msg_type, msg_head_.msg_id)) {
		buffer->set_read_idx(read_idx);
		push_buffer_func(buffer);
		return 1;
	}

	const int argc = 1;
	Local<Value> argv[argc];
	if (msg_head_.protocol == TCP) {
		int ret = 0;
		std::string struct_name = get_struct_name(msg_head_.msg_type, msg_head_.msg_id);
		Msg_Struct *msg_struct = STRUCT_MANAGER->get_msg_struct(struct_name);
		if (msg_struct) {
			Bit_Buffer bit_buffer;
			bit_buffer.set_ary(buffer->get_read_ptr(), buffer->readable_bytes());
			argv[0] = msg_struct->build_buffer_msg_object(isolate_, msg_head_, bit_buffer);
			if(argv[0].IsEmpty()) {
				ret = -1;
			}
		} else {
			ret = -1;
		}

		if(ret < 0) {
			LOG_ERROR("process tcp msg error, struct_name:%s, eid:%d, cid:%d, msg_id:%d, msg_type:%d, sid:%u",
				struct_name.c_str(), msg_head_.eid, msg_head_.cid, msg_head_.msg_id, msg_head_.msg_type, msg_head_.sid);
			return ret;
		}
	} else if (msg_head_.protocol == HTTP) {
		//http消息包
		std::string post_data = "";
		buffer->read_string(post_data);
		Json::Value value;
		Json::Reader reader(Json::Features::strictMode());
		if ( !reader.parse(post_data, value) || !value["msg_id"].isInt()) {
			LOG_ERROR("http post data:%s format error or msg_id not int", post_data.c_str());
			//出错情况下断开连接
			push_drop(msg_head_.eid, msg_head_.cid);
			return -1;
		}

		msg_head_.msg_id = value["msg_id"].asInt();
		msg_head_.msg_type = HTTP_MSG;
		int ret = 0;
		std::string struct_name = get_struct_name(msg_head_.msg_type, msg_head_.msg_id);
		Msg_Struct *msg_struct = STRUCT_MANAGER->get_msg_struct(struct_name);
		if (msg_struct) {
			argv[0] = msg_struct->build_json_msg_object(isolate_, msg_head_, value);
			if(argv[0].IsEmpty()) {
				ret = -1;
			}
		} else {
			ret = -1;
		}

		if(ret < 0) {
			LOG_ERROR("process http msg error, struct_name:%s, eid:%d, cid:%d, msg_id:%d, msg_type:%d, sid:%u",
				struct_name.c_str(), msg_head_.eid, msg_head_.cid, msg_head_.msg_id, msg_head_.msg_type, msg_head_.sid);
			//出错情况下断开连接
			push_drop(msg_head_.eid, msg_head_.cid);
			return ret;
		}
	} else if (msg_head_.protocol == WEBSOCKET) {
		//websocket消息包
		std::string ws_data = "";
		buffer->read_string(ws_data);
		Json::Value value;
		Json::Reader reader(Json::Features::strictMode());
		if ( !reader.parse(ws_data, value) || !value["msg_id"].isInt()) {
			LOG_ERROR("websocket data:%s format error or msg_id not int", ws_data.c_str());
			//出错情况下断开连接
			push_drop(msg_head_.eid, msg_head_.cid);
			return -1;
		}

		msg_head_.msg_id = value["msg_id"].asInt();
		msg_head_.msg_type = WS_C2S;
		int ret = 0;
		std::string struct_name = get_struct_name(msg_head_.msg_type, msg_head_.msg_id);
		Msg_Struct *msg_struct = STRUCT_MANAGER->get_msg_struct(struct_name);
		if (msg_struct) {
			argv[0] = msg_struct->build_json_msg_object(isolate_, msg_head_, value);
			if(argv[0].IsEmpty()) {
				ret = -1;
			}
		} else {
			ret = -1;
		}

		if(ret < 0) {
			LOG_ERROR("process websocket msg error, struct_name:%s, eid:%d, cid:%d, msg_id:%d, msg_type:%d, sid:%u",
				struct_name.c_str(), msg_head_.eid, msg_head_.cid, msg_head_.msg_id, msg_head_.msg_type, msg_head_.sid);
			//出错情况下断开连接
			push_drop(msg_head_.eid, msg_head_.cid);
			return ret;
		}
	}

	return call_js_func(context, argc, argv, "on_msg");
}

int V8_Manager::process_timer(Local<Context> context) {
	HandleScope handle_scope(isolate_);

	const int argc = 1;
	Local<Value> argv[argc] = {Int32::New(isolate_, timer_id_)};
	call_js_func(context, argc, argv, "on_tick");
	return 0;
}

int V8_Manager::call_js_func(Local<Context> context, int argc, Local<Value> argv[], const char* func_name) {
	HandleScope handle_scope(isolate_);
	Local<String> func_string = String::NewFromUtf8(isolate_, func_name, NewStringType::kNormal).ToLocalChecked();
	Local<Value> func_value;
	if (!context->Global()->Get(context, func_string).ToLocal(&func_value) || !func_value->IsFunction()) {
		LOG_ERROR("can't find function:%s", func_name);
		return -1;
	}

	//添加异常处理
	TryCatch try_catch(isolate_);
	Local<Value> result;
	Local<Function> js_func = Local<Function>::Cast(func_value);
	if(!js_func->Call(context, context->Global(), argc, argv).ToLocal(&result)) {
		report_exception(isolate_, &try_catch, nullptr);
		return -1;
	}
	return 0;
}

int V8_Manager::drop_info_tick(Time_Value &now) {
	if(now - drop_info_tick_ < Time_Value(0, 100 * 1000)) {
		return -1;
	}

	drop_info_tick_ = now;
	Drop_Info drop_info;
	while(!drop_info_list_.empty()) {
		drop_info = drop_info_list_.front();
		if (now - drop_info.drop_time >= Time_Value(1, 0)) {
			//删除第一个元素，否则会出现死循环，一直占用cpu
			drop_info_list_.pop_front();
			//关闭通信层连接
			NODE_MANAGER->push_drop(drop_info.eid, drop_info.cid);
		}
		else {
			break;
		}
	}
	return 0;
}

std::string V8_Manager::get_v8_stack(void) {
	return get_stack_trace(isolate_);
}