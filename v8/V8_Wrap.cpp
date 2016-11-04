/*
 * V8_Wrap.cpp
 *
 *  Created on: Feb 1, 2016
 *      Author: zhangyalei
 */

#include <sstream>
#include "Base_V8.h"
#include "Msg_Struct.h"
#include "Data_Manager.h"
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

	global->Set(String::NewFromUtf8(isolate, "connect_mysql", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, connect_mysql));
	global->Set(String::NewFromUtf8(isolate, "connect_mongo", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, connect_mongo));
	global->Set(String::NewFromUtf8(isolate, "generate_table_index", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, generate_table_index));
	global->Set(String::NewFromUtf8(isolate, "select_table_index", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, select_table_index));
	global->Set(String::NewFromUtf8(isolate, "load_db_data", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, load_db_data));
	global->Set(String::NewFromUtf8(isolate, "save_db_data", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, save_db_data));
	global->Set(String::NewFromUtf8(isolate, "delete_db_data", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, delete_db_data));
	global->Set(String::NewFromUtf8(isolate, "set_runtime_data", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, set_runtime_data));
	global->Set(String::NewFromUtf8(isolate, "get_runtime_data", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, get_runtime_data));
	global->Set(String::NewFromUtf8(isolate, "delete_runtime_data", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, delete_runtime_data));

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
		msg_struct->build_bit_buffer(args.GetIsolate(), args[5]->ToObject(context).ToLocalChecked(), bit_buffer);
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

void connect_mysql(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 6) {
		LOG_ERROR("connect_mysql args error, length: %d\n", args.Length());
		return ;
	}

	Local<Context> context(args.GetIsolate()->GetCurrentContext());
	int db_id = args[0]->Int32Value(context).FromMaybe(0);
	String::Utf8Value ip_str(args[1]->ToString(context).ToLocalChecked());
	std::string ip = to_string(ip_str);
	int port = args[2]->Int32Value(context).FromMaybe(0);
	String::Utf8Value user_str(args[3]->ToString(context).ToLocalChecked());
	std::string user = to_string(user_str);
	String::Utf8Value passwd_str(args[4]->ToString(context).ToLocalChecked());
	std::string password = to_string(passwd_str);
	String::Utf8Value poolname_str(args[5]->ToString(context).ToLocalChecked());
	std::string pool_name = to_string(poolname_str);

	DB_Operator *db_operator = DB_OPERATOR(db_id);
	bool ret = false;
	if(db_operator != NULL) {
		ret = db_operator->connect_to_db(db_id, ip, port, user, password, pool_name);;
	}
	args.GetReturnValue().Set(ret);
}

void connect_mongo(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 3) {
		LOG_ERROR("connect_mongo args error, length: %d\n", args.Length());
		return ;
	}

	Local<Context> context(args.GetIsolate()->GetCurrentContext());
	int db_id = args[0]->Int32Value(context).FromMaybe(0);
	String::Utf8Value ip_str(args[1]->ToString(context).ToLocalChecked());
	std::string ip = to_string(ip_str);
	int port = args[2]->Int32Value(context).FromMaybe(0);
	std::string user = "";
	std::string password = "";
	std::string pool_name = "";

	DB_Operator *db_operator = DB_OPERATOR(db_id);
	bool ret = false;
	if(db_operator != nullptr) {
		ret = db_operator->connect_to_db(db_id, ip, port, user, password, pool_name);
	}
	args.GetReturnValue().Set(ret);
}

void generate_table_index(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 3) {
		LOG_ERROR("generate_table_index args error, length: %d\n", args.Length());
		return ;
	}

	Local<Context> context(args.GetIsolate()->GetCurrentContext());
	int db_id = args[0]->Int32Value(context).FromMaybe(0);
	String::Utf8Value table_str(args[1]);
	std::string table_name = to_string(table_str);
	String::Utf8Value type_str(args[2]);
	std::string type = to_string(type_str);

	double id = 0.0f;
	DB_Operator *db_operator = DB_OPERATOR(db_id);
	if(db_operator != nullptr) {
		DB_Struct *db_struct = STRUCT_MANAGER->get_table_struct(table_name);
		if(db_struct == nullptr) {
			LOG_ERROR("db_struct %s is NULL", table_name.c_str());
			args.GetReturnValue().Set(id);
			return;
		}
		id = db_operator->generate_table_index(db_id, db_struct, type);
	}
	args.GetReturnValue().Set(id);
}

void select_table_index(const FunctionCallbackInfo<Value>& args) {
	Local<Context> context(args.GetIsolate()->GetCurrentContext());

	int db_id = args[0]->Int32Value(context).FromMaybe(0);
	String::Utf8Value index_str(args[1]->ToString(context).ToLocalChecked());
	std::string table_name = to_string(index_str);
	String::Utf8Value query_name_str(args[2]->ToString(context).ToLocalChecked());
	std::string query_name = to_string(query_name_str);
	String::Utf8Value query_value_str(args[3]->ToString(context).ToLocalChecked());
	std::string query_value = to_string(query_value_str);

	double key_index = 0.0f;
	DB_Operator *db_operator = DB_OPERATOR(db_id);
	if(db_operator != nullptr) {
		DB_Struct *db_struct = STRUCT_MANAGER->get_table_struct(table_name);
		if(db_struct == nullptr) {
			LOG_ERROR("db_struct %s is NULL", table_name.c_str());
			args.GetReturnValue().Set(key_index);
			return;
		}
		key_index = db_operator->select_table_index(db_id, db_struct, query_name, query_value);
	}
	args.GetReturnValue().Set(key_index);
}

void load_db_data(const FunctionCallbackInfo<Value>& args) {
	Local<Context> context(args.GetIsolate()->GetCurrentContext());

	int db_id = args[0]->Int32Value(context).FromMaybe(0);
	String::Utf8Value table_str(args[1]->ToString(context).ToLocalChecked());
	std::string table_name = to_string(table_str);
	int64_t key_index = args[2]->NumberValue(context).FromMaybe(0);

	DB_Struct *db_struct = STRUCT_MANAGER->get_table_struct(table_name);
	if (db_struct == nullptr) {
		//数据库表名为空，表示该结构体为多张数据库表的集合，对每个字段分别处理
		db_struct = STRUCT_MANAGER->get_db_struct(table_name);
		if(db_struct == nullptr) {
			LOG_ERROR("db_struct %s is NULL", table_name.c_str());
			args.GetReturnValue().SetNull();
			return;
		}
		HandleScope handle_scope(args.GetIsolate());
		Local<ObjectTemplate> localTemplate = ObjectTemplate::New(args.GetIsolate());
		v8::Local<v8::Object> object = localTemplate->NewInstance(context).ToLocalChecked();

		for(std::vector<Field_Info>::const_iterator iter = db_struct->field_vec().begin();
				iter != db_struct->field_vec().end(); ++iter) {
			DB_Struct *sub_struct = STRUCT_MANAGER->get_db_struct(iter->field_type);
			if(db_struct == nullptr) {
				LOG_ERROR("db_struct %s is NULL", iter->field_type.c_str());
				args.GetReturnValue().SetNull();
				return;
			}
			v8::Local<v8::Object> sub_object = load_single_data(args.GetIsolate(), db_id, sub_struct, key_index);
			object->Set(context,
					String::NewFromUtf8(args.GetIsolate(), iter->field_name.c_str(), NewStringType::kNormal).ToLocalChecked(),
					sub_object).FromJust();
			}
			args.GetReturnValue().Set(object);
	}
	else {
		v8::Local<v8::Object> object = load_single_data(args.GetIsolate(), db_id, db_struct, key_index);
		args.GetReturnValue().Set(object);
	}
}

void save_db_data(const FunctionCallbackInfo<Value>& args) {
	Local<Context> context(args.GetIsolate()->GetCurrentContext());

	int save_flag = args[0]->Int32Value(context).FromMaybe(0);
	int db_id = args[1]->Int32Value(context).FromMaybe(0);
	String::Utf8Value table_str(args[2]->ToString(context).ToLocalChecked());
	std::string table_name = to_string(table_str);
	Local<Object> object = args[3]->ToObject(context).ToLocalChecked();

	if (object->IsArray()) {
		Local<Array> array = Local<Array>::Cast(object);
		int len = array->Length();
		for (int i = 0; i < len; ++i) {
			Local<Value> value = array->Get(context, i).ToLocalChecked();
			Local<Object> sub_object = value->ToObject(context).ToLocalChecked();
			save_single_data(args.GetIsolate(), db_id, table_name, sub_object, save_flag);
		}
	} else {
		save_single_data(args.GetIsolate(), db_id, table_name, object, save_flag);
	}
}

void delete_db_data(const FunctionCallbackInfo<Value>& args) {
	Local<Context> context(args.GetIsolate()->GetCurrentContext());

	int db_id = args[0]->Int32Value(context).FromMaybe(0);
	String::Utf8Value table_str(args[1]->ToString(context).ToLocalChecked());
	std::string table_name = to_string(table_str);

	DB_Struct *db_struct = STRUCT_MANAGER->get_table_struct(table_name);
	if(db_struct == nullptr) {
		LOG_ERROR("db_struct %s is NULL!", table_name.c_str());
		return;
	}
	DB_Operator *db_operator = DB_OPERATOR(db_id);
	if(db_operator != nullptr) {
		db_operator->delete_data(db_id, db_struct, args.GetIsolate(), args[2]->ToObject(context).ToLocalChecked());
	}
}

v8::Local<v8::Object> load_single_data(Isolate* isolate, int db_id, DB_Struct *db_struct, int64_t key_index) {
	EscapableHandleScope handle_scope(isolate);

	std::vector<Bit_Buffer *> buffer_vec;
	DATA_MANAGER->load_db_data(db_id, db_struct, key_index, buffer_vec);
	//索引为0，表示加载整张表
	if(key_index == 0) {
		uint length = buffer_vec.size();
		Local<Array> array = Array::New(isolate, length);
		for (uint i = 0; i < length; ++i) {
			Local<Object> sub_object = db_struct->build_bit_object(isolate, *buffer_vec[i]);
			if (!sub_object.IsEmpty()) {
				array->Set(i, sub_object);
			}
		}
		return handle_scope.Escape(array);
	}
	//按照索引加载表里的数据
	else {
		Local<Object> object = db_struct->build_bit_object(isolate, *buffer_vec[0]);
		return handle_scope.Escape(object);
	}
}

void save_single_data(Isolate* isolate, int db_id, std::string &table_name, Local<Object> object, int flag) {
	Local<Context> context(isolate->GetCurrentContext());

	DB_Struct *db_struct = STRUCT_MANAGER->get_table_struct(table_name);
	if (db_struct == nullptr) {
		//数据库表名为空，表示该结构体为多张数据库表的集合，对每个字段分别处理
		db_struct = STRUCT_MANAGER->get_db_struct(table_name);
		if(db_struct == nullptr) {
			LOG_ERROR("db_struct %s is NULL", table_name.c_str());
			return;
		}
		for(std::vector<Field_Info>::const_iterator iter = db_struct->field_vec().begin();
				iter != db_struct->field_vec().end(); ++iter) {
			DB_Struct *sub_struct = STRUCT_MANAGER->get_db_struct(iter->field_type);
			if(db_struct == nullptr) {
				LOG_ERROR("db_struct %s is NULL", iter->field_type.c_str());
				return;
			}
			//从object中取出子对象进行保存操作，子对象是单张表单条记录
			Local<Value> value = object->Get(context, String::NewFromUtf8(isolate, iter->field_name.c_str(), NewStringType::kNormal).ToLocalChecked()).ToLocalChecked();
			Local<Object> sub_object = value->ToObject(context).ToLocalChecked();
			Bit_Buffer *buffer = DATA_MANAGER->pop_buffer();
			sub_struct->build_bit_buffer(isolate, sub_object, *buffer);
			DATA_MANAGER->save_db_data(db_id, sub_struct, buffer, flag);
		}
	} else {
		Bit_Buffer *buffer = DATA_MANAGER->pop_buffer();
		db_struct->build_bit_buffer(isolate, object, *buffer);
		DATA_MANAGER->save_db_data(db_id, db_struct, buffer, flag);
	}
}

void set_runtime_data(const FunctionCallbackInfo<Value>& args) {
	Local<Context> context(args.GetIsolate()->GetCurrentContext());

	String::Utf8Value table_str(args[0]->ToString(context).ToLocalChecked());
	std::string key_name = to_string(table_str);

	int64_t index = args[1]->IntegerValue(context).FromMaybe(0);

	DB_Struct *db_struct = STRUCT_MANAGER->get_db_struct(key_name);
	if(db_struct == nullptr) {
		LOG_ERROR("db_struct %s is NULL", key_name.c_str());
		return;
	}
	Bit_Buffer *buffer = DATA_MANAGER->pop_buffer();
	db_struct->build_bit_buffer(args.GetIsolate(), args[2]->ToObject(context).ToLocalChecked(), *buffer);
	DATA_MANAGER->set_runtime_data(index, db_struct, buffer);
}

void get_runtime_data(const FunctionCallbackInfo<Value>& args) {
	Local<Context> context(args.GetIsolate()->GetCurrentContext());

	String::Utf8Value table_str(args[0]->ToString(context).ToLocalChecked());
	std::string key_name = to_string(table_str);

	int64_t index = args[1]->IntegerValue(context).FromMaybe(0);

	DB_Struct *db_struct = STRUCT_MANAGER->get_db_struct(key_name);
	if(db_struct == nullptr) {
		LOG_ERROR("db_struct %s is NULL", key_name.c_str());
		return;
	}
	Bit_Buffer *buffer = DATA_MANAGER->get_runtime_data(index, db_struct);
	if(buffer) {
		Local<Object> object = db_struct->build_bit_object(args.GetIsolate(), *buffer);
		args.GetReturnValue().Set(object);
	}  else {
		args.GetReturnValue().SetNull();
	}
}

void delete_runtime_data(const FunctionCallbackInfo<Value>& args) {
	Local<Context> context(args.GetIsolate()->GetCurrentContext());

	String::Utf8Value table_str(args[0]->ToString(context).ToLocalChecked());
	std::string key_name = to_string(table_str);

	int64_t index = args[1]->IntegerValue(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);

	DB_Struct *db_struct = STRUCT_MANAGER->get_db_struct(key_name);
	if(db_struct == nullptr) {
		LOG_ERROR("db_struct %s is NULL", key_name.c_str());
		return;
	}

	DATA_MANAGER->delete_runtime_data(index, db_struct);
}
