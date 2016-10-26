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
	default: {
		break;
	}
	}
	return stream.str();
}

Local<Object> get_node_object(Isolate* isolate, const Node_Info &node_info) {
	EscapableHandleScope handle_scope(isolate);

	Local<ObjectTemplate> localTemplate = ObjectTemplate::New(isolate);
	Local<Object> node_obj = localTemplate->NewInstance(isolate->GetCurrentContext()).ToLocalChecked();
	node_obj->Set(isolate->GetCurrentContext(),
			String::NewFromUtf8(isolate, "node_type", NewStringType::kNormal).ToLocalChecked(),
			Int32::New(isolate, node_info.node_type)).FromJust();
	node_obj->Set(isolate->GetCurrentContext(),
			String::NewFromUtf8(isolate, "node_id", NewStringType::kNormal).ToLocalChecked(),
			Int32::New(isolate, node_info.node_id)).FromJust();
	node_obj->Set(isolate->GetCurrentContext(),
			String::NewFromUtf8(isolate, "node_name", NewStringType::kNormal).ToLocalChecked(),
			String::NewFromUtf8(isolate, node_info.node_name.c_str(), NewStringType::kNormal).ToLocalChecked()).FromJust();

	int vec_size = node_info.server_list.size();
	Local<Array> server_array = Array::New(isolate, vec_size);
	for(uint16_t i = 0; i < vec_size; ++i) {
		Local<Object> server_obj = localTemplate->NewInstance(isolate->GetCurrentContext()).ToLocalChecked();
		Endpoint_Info server_info = node_info.server_list[i];
		server_obj->Set(isolate->GetCurrentContext(),
			String::NewFromUtf8(isolate, "endpoint_type", NewStringType::kNormal).ToLocalChecked(),
			Int32::New(isolate, server_info.endpoint_type)).FromJust();
		server_obj->Set(isolate->GetCurrentContext(),
			String::NewFromUtf8(isolate, "endpoint_id", NewStringType::kNormal).ToLocalChecked(),
			Int32::New(isolate, server_info.endpoint_id)).FromJust();
		server_obj->Set(isolate->GetCurrentContext(),
			String::NewFromUtf8(isolate, "endpoint_name", NewStringType::kNormal).ToLocalChecked(),
			String::NewFromUtf8(isolate, server_info.endpoint_name.c_str(), NewStringType::kNormal).ToLocalChecked()).FromJust();
		server_obj->Set(isolate->GetCurrentContext(),
			String::NewFromUtf8(isolate, "server_ip", NewStringType::kNormal).ToLocalChecked(),
			String::NewFromUtf8(isolate, server_info.server_ip.c_str(), NewStringType::kNormal).ToLocalChecked()).FromJust();
		server_obj->Set(isolate->GetCurrentContext(),
			String::NewFromUtf8(isolate, "server_port", NewStringType::kNormal).ToLocalChecked(),
			Int32::New(isolate, server_info.server_port)).FromJust();
		server_obj->Set(isolate->GetCurrentContext(),
			String::NewFromUtf8(isolate, "protocol_type", NewStringType::kNormal).ToLocalChecked(),
			Int32::New(isolate, server_info.protocol_type)).FromJust();
		
		server_array->Set(isolate->GetCurrentContext(), i, server_obj).FromJust();
	}
	node_obj->Set(isolate->GetCurrentContext(),
			String::NewFromUtf8(isolate, "server_list", NewStringType::kNormal).ToLocalChecked(),
			server_array).FromJust();

	return handle_scope.Escape(node_obj);
}

Local<Context> create_context(Isolate* isolate) {
	Local<ObjectTemplate> global = ObjectTemplate::New(isolate);
	global->Set(String::NewFromUtf8(isolate, "require", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, require));
	global->Set(String::NewFromUtf8(isolate, "read_json", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, read_json));
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

	return Context::New(isolate, NULL, global);
}

void register_timer(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 3) {
		LOG_ERROR("register timer args error, length: %d\n", args.Length());
		return;
	}

	int timer_id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
	int interval = args[1]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
	int first_tick = args[2]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
	NODE_TIMER->register_handler(timer_id, interval, first_tick);
	LOG_INFO("register_timer, timer_id:%d, interval:%d", timer_id, interval);
}

void send_msg(const FunctionCallbackInfo<Value>& args) {
	int endpoint_id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
	int cid = args[1]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
	int msg_id = args[2]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
	int msg_type = args[3]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
	uint32_t sid = args[4]->Uint32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);

	Block_Buffer buffer;
	buffer.write_uint16(0);
	buffer.write_uint8(msg_id);
	if (msg_type != S2C) {
		buffer.write_uint8(msg_type);
		buffer.write_uint32(sid);
	}
	std::string struct_name = get_struct_name(msg_type, msg_id);
	Msg_Struct *msg_struct = STRUCT_MANAGER->get_msg_struct(struct_name);
	if (msg_struct != nullptr) {
		msg_struct->build_msg_buffer(args.GetIsolate(), args[5]->ToObject(args.GetIsolate()->GetCurrentContext()).ToLocalChecked(), buffer);
	}
	buffer.write_len(RPC_PKG);
	NODE_MANAGER->send_buffer(endpoint_id, cid, buffer);
	NODE_MANAGER->add_send_bytes(buffer.readable_bytes());
}

void close_session(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 3) {
		LOG_ERROR("close_client args error, length: %d\n", args.Length());
		args.GetReturnValue().SetNull();
		return;
	}

	Drop_Info drop_info;
	drop_info.endpoint_id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
	drop_info.drop_cid = args[1]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
	drop_info.error_code = args[2]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
	NODE_MANAGER->push_drop(drop_info);
}

void connect_mysql(const FunctionCallbackInfo<Value>& args) {
	int db_id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);

	String::Utf8Value ip_str(args[1]->ToString(args.GetIsolate()->GetCurrentContext()).ToLocalChecked());
	std::string ip = to_string(ip_str);

	int port = args[2]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);

	String::Utf8Value user_str(args[3]->ToString(args.GetIsolate()->GetCurrentContext()).ToLocalChecked());
	std::string user = to_string(user_str);

	String::Utf8Value passwd_str(args[4]->ToString(args.GetIsolate()->GetCurrentContext()).ToLocalChecked());
	std::string password = to_string(passwd_str);

	String::Utf8Value poolname_str(args[5]->ToString(args.GetIsolate()->GetCurrentContext()).ToLocalChecked());
	std::string pool_name = to_string(poolname_str);

	bool ret = DB_OPERATOR(db_id)->connect_to_db(db_id, ip, port, user, password, pool_name);
	args.GetReturnValue().Set(ret);
}

void connect_mongo(const FunctionCallbackInfo<Value>& args) {
	int db_id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);

	String::Utf8Value ip_str(args[1]->ToString(args.GetIsolate()->GetCurrentContext()).ToLocalChecked());
	std::string ip = to_string(ip_str);

	int port = args[2]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);

	std::string user = "";
	std::string password = "";
	std::string pool_name = "";

	bool ret = DB_OPERATOR(db_id)->connect_to_db(db_id, ip, port, user, password, pool_name);
	args.GetReturnValue().Set(ret);
}

void generate_table_index(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 3) {
		LOG_ERROR("generate_table_index args error, length: %d\n", args.Length());
		return ;
	}
	int db_id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);

	String::Utf8Value table_str(args[1]);
	std::string table_name = to_string(table_str);

	String::Utf8Value type_str(args[2]);
	std::string type = to_string(type_str);

	double id = DB_OPERATOR(db_id)->generate_table_index(db_id, STRUCT_MANAGER->get_table_struct(table_name), type);
	args.GetReturnValue().Set(id);
}

void select_table_index(const FunctionCallbackInfo<Value>& args) {
	int db_id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);

	String::Utf8Value index_str(args[1]->ToString(args.GetIsolate()->GetCurrentContext()).ToLocalChecked());
	std::string table_name = to_string(index_str);

	String::Utf8Value query_name_str(args[2]->ToString(args.GetIsolate()->GetCurrentContext()).ToLocalChecked());
	std::string query_name = to_string(query_name_str);

	String::Utf8Value query_value_str(args[3]->ToString(args.GetIsolate()->GetCurrentContext()).ToLocalChecked());
	std::string query_value = to_string(query_value_str);

	double key_index = DB_OPERATOR(db_id)->select_table_index(db_id, STRUCT_MANAGER->get_table_struct(table_name), query_name, query_value);
	args.GetReturnValue().Set(key_index);
}

void load_db_data(const FunctionCallbackInfo<Value>& args) {
	int db_id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);

	String::Utf8Value table_str(args[1]->ToString(args.GetIsolate()->GetCurrentContext()).ToLocalChecked());
	std::string table_name = to_string(table_str);

	int64_t key_index = args[2]->NumberValue(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);

	DB_Struct *db_struct = STRUCT_MANAGER->get_table_struct(table_name);
	if (db_struct == nullptr) {
		//数据库表名为空，表示该结构体为多张数据库表的集合，对每个字段分别处理
		db_struct = STRUCT_MANAGER->get_db_struct(table_name);
		HandleScope handle_scope(args.GetIsolate());
		Local<ObjectTemplate> localTemplate = ObjectTemplate::New(args.GetIsolate());
		v8::Local<v8::Object> object = localTemplate->NewInstance(args.GetIsolate()->GetCurrentContext()).ToLocalChecked();

		for(std::vector<Field_Info>::const_iterator iter = db_struct->field_vec().begin();
				iter != db_struct->field_vec().end(); ++iter) {
			Local<Object> sub_object = Local<Object>();

			DB_Struct *sub_struct = STRUCT_MANAGER->get_db_struct(iter->field_type);
			sub_object = DB_OPERATOR(db_id)->load_data(db_id, sub_struct, args.GetIsolate(), key_index);

			object->Set(args.GetIsolate()->GetCurrentContext(),
					String::NewFromUtf8(args.GetIsolate(), iter->field_name.c_str(), NewStringType::kNormal).ToLocalChecked(),
					sub_object).FromJust();
			args.GetReturnValue().Set(object);
		}
	}
	else {
		Local<v8::Object> obj = DB_OPERATOR(db_id)->load_data(db_id, db_struct, args.GetIsolate(), key_index);
		args.GetReturnValue().Set(obj);
	}
}

void save_db_data(const FunctionCallbackInfo<Value>& args) {
	int db_id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);

	String::Utf8Value table_str(args[1]->ToString(args.GetIsolate()->GetCurrentContext()).ToLocalChecked());
	std::string table_name = to_string(table_str);

	Local<Object> object = args[2]->ToObject(args.GetIsolate()->GetCurrentContext()).ToLocalChecked();
	if (object->IsArray()) {
		Local<Array> array = Local<Array>::Cast(object);
		int len = array->Length();
		for (int i = 0; i < len; ++i) {
			Local<Value> value = array->Get(args.GetIsolate()->GetCurrentContext(), i).ToLocalChecked();
			Local<Object> sub_object = value->ToObject(args.GetIsolate()->GetCurrentContext()).ToLocalChecked();
			save_single_data(args.GetIsolate(), db_id, table_name, sub_object);
		}
	} else {
		save_single_data(args.GetIsolate(), db_id, table_name, object);
	}
}

void save_single_data(Isolate* isolate, int db_id, std::string &table_name, Local<Object> object) {
	DB_Struct *db_struct = STRUCT_MANAGER->get_table_struct(table_name);
	if (db_struct == nullptr) {
		//数据库表名为空，表示该结构体为多张数据库表的集合，对每个字段分别处理
		db_struct = STRUCT_MANAGER->get_db_struct(table_name);
		for(std::vector<Field_Info>::const_iterator iter = db_struct->field_vec().begin();
				iter != db_struct->field_vec().end(); ++iter) {
			DB_Struct *sub_struct = STRUCT_MANAGER->get_db_struct(iter->field_type);
			//从object中取出子对象进行保存操作，子对象是单张表单条记录
			Local<Value> value = object->Get(isolate->GetCurrentContext(), String::NewFromUtf8(isolate, iter->field_name.c_str(), NewStringType::kNormal).ToLocalChecked()).ToLocalChecked();
			Local<Object> sub_object = value->ToObject(isolate->GetCurrentContext()).ToLocalChecked();
			DB_OPERATOR(db_id)->save_data(db_id, sub_struct, isolate, sub_object);
		}
	} else {
		DB_OPERATOR(db_id)->save_data(db_id, db_struct, isolate, object);
	}
}

void delete_db_data(const FunctionCallbackInfo<Value>& args) {
	int db_id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);

	String::Utf8Value table_str(args[1]->ToString(args.GetIsolate()->GetCurrentContext()).ToLocalChecked());
	std::string table_name = to_string(table_str);

	DB_Struct *db_struct = STRUCT_MANAGER->get_table_struct(table_name);
	DB_OPERATOR(db_id)->delete_data(db_id, db_struct, args.GetIsolate(), args[2]->ToObject(args.GetIsolate()->GetCurrentContext()).ToLocalChecked());
}
