/*
 * V8_Wrap.cpp
 *
 *  Created on: Feb 1, 2016
 *      Author: zhangyalei
 */

#include <sstream>
#include "V8_Base.h"
#include "Data_Manager.h"
#include "Struct_Manager.h"
#include "DB_V8_Wrap.h"

void init_db_operator(const FunctionCallbackInfo<Value>& args) {
	DATA_MANAGER->init_db_operator();
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

		for(Field_Vec::const_iterator iter = db_struct->field_vec().begin();
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
	Local<Context> context(isolate->GetCurrentContext());
	v8::Local<v8::ObjectTemplate> localTemplate = ObjectTemplate::New(isolate);

	std::vector<Bit_Buffer *> buffer_vec;
	DATA_MANAGER->load_db_data(db_id, db_struct, key_index, buffer_vec);
	//索引为0，表示加载整张表
	if(key_index == 0) {
		uint length = buffer_vec.size();
		Local<Array> array = Array::New(isolate, length);
		for (uint i = 0; i < length; ++i) {
			v8::Local<v8::Object> object = localTemplate->NewInstance(context).ToLocalChecked();
			db_struct->build_bit_object(isolate, db_struct->field_vec(), *buffer_vec[i], object);
			if (!object.IsEmpty()) {
				array->Set(i, object);
			}
		}
		return handle_scope.Escape(array);
	}
	//按照索引加载表里的数据
	else {
		v8::Local<v8::Object> object = localTemplate->NewInstance(context).ToLocalChecked();
		db_struct->build_bit_object(isolate, db_struct->field_vec(), *buffer_vec[0], object);
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
		for(Field_Vec::const_iterator iter = db_struct->field_vec().begin();
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
			sub_struct->build_bit_buffer(isolate, sub_struct->field_vec(), *buffer, sub_object);
			DATA_MANAGER->save_db_data(db_id, sub_struct, buffer, flag);
		}
	} else {
		Bit_Buffer *buffer = DATA_MANAGER->pop_buffer();
		db_struct->build_bit_buffer(isolate, db_struct->field_vec(), *buffer, object);
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
	db_struct->build_bit_buffer(args.GetIsolate(), db_struct->field_vec(), *buffer, args[2]->ToObject(context).ToLocalChecked());
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
		v8::Local<v8::ObjectTemplate> localTemplate = ObjectTemplate::New(args.GetIsolate());
		v8::Local<v8::Object> object = localTemplate->NewInstance(context).ToLocalChecked();
		db_struct->build_bit_object(args.GetIsolate(), db_struct->field_vec(), *buffer, object);
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
