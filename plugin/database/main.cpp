#include "DB_V8_Wrap.h"

using namespace v8;

extern "C" {

void init(Local<ObjectTemplate> &global, Isolate *isolate) {
	global->Set(String::NewFromUtf8(isolate, "init_db_operator", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(isolate, init_db_operator));
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
}

}

