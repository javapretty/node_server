/*
 * V8_Base.cpp
 *
 *  Created on: Oct 21, 2016
 *      Author: zhangyalei
 */

#include <unordered_map>
#include <sstream>
#include "Base_Function.h"
#include "Proc_Info.h"
#include "Log.h"
#include "V8_Base.h"

std::string get_struct_name(int msg_type, int msg_id) {
	std::ostringstream stream;
	switch(msg_type) {
	case TCP_C2S:
	case NODE_C2S:
		stream << "c2s_" << msg_id;
		break;
	case TCP_S2C:
	case NODE_S2C:
		stream << "s2c_" << msg_id;
		break;
	case NODE_MSG:
	case DATA_MSG:
		stream << "node_" << msg_id;
		break;
	case HTTP_MSG:
		stream << "http_" << msg_id;
		break;
	case WS_C2S:
		stream << "ws_c2s_" << msg_id;
		break;
	case WS_S2C:
		stream << "ws_s2c_" << msg_id;
		break;
	default: {
		LOG_ERROR("get_struct_name error, msg_type:%d, msg_id:%d", msg_type, msg_id);
		break;
	}
	}
	return stream.str();
}

std::string to_string(const v8::String::Utf8Value& value) {
	if (*value == NULL) {
		return "";
	} else {
		std::stringstream stream;
		stream << *value;
		return stream.str();
	}
}

int run_script(Isolate* isolate, const char* file_path) {
	HandleScope handle_scope(isolate);

	Local<String> source;
	if (!read_file(isolate, file_path).ToLocal(&source)) {
		LOG_ERROR("read script:%s error\n", file_path);
		return -1;
	}

	Local<Context> context(isolate->GetCurrentContext());
	TryCatch try_catch(isolate);

	// Compile the script and check for errors.
	Local<Script> compiled_script;
	if (!Script::Compile(context, source).ToLocal(&compiled_script)) {
		report_exception(isolate, &try_catch, file_path);
		return -1;
	 }

	// Run the script!
	Local<Value> result;
	if (!compiled_script->Run(context).ToLocal(&result)) {
		report_exception(isolate, &try_catch, file_path);
		return -1;
	  }
  	return 0;
}

MaybeLocal<String> read_file(Isolate* isolate, const char* file_path) {
	FILE* file = fopen(file_path, "rb");
	if (file == NULL) 
		return MaybeLocal<String>();

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	rewind(file);

	char* chars = new char[size + 1];
	chars[size] = '\0';
	for (size_t i = 0; i < size;) {
		i += fread(&chars[i], 1, size - i, file);
		if (ferror(file)) {
			fclose(file);
			delete[] chars;
			return MaybeLocal<String>();
		}
	}
	fclose(file);
	MaybeLocal<String> result = String::NewFromUtf8(
		isolate, chars, NewStringType::kNormal, static_cast<int>(size));
	delete[] chars;
	return result;
}

void report_exception(Isolate* isolate, TryCatch* try_catch,  const char* file_path) {
	HandleScope handle_scope(isolate);

	String::Utf8Value exception(try_catch->Exception());
	std::string exception_string = to_string(exception);
	Local<Message> message = try_catch->Message();
	if (message.IsEmpty()) {
		LOG_ERROR("%s\n", exception_string.c_str());
	} 
	else {
		Local<Context> context(isolate->GetCurrentContext());
		//打印文件名
		//if (file_path == nullptr) {
		//	Local<Value> script_name = message->GetScriptOrigin().ResourceName()();
		//	String::Utf8Value filename(script_name->ToString(context).ToLocalChecked());
		//	std::string filename_string = to_string(filename);
		//	LOG_ERROR("%s: %d: %s", filename_string.c_str(), linenum, exception_string.c_str());
		//} else {
		//	LOG_ERROR("%s: %d: %s", file_path, linenum, exception_string.c_str());
	  	//}

		//打印源代码
		int linenum = message->GetLineNumber(context).FromJust();
		String::Utf8Value sourceline(message->GetSourceLine(context).ToLocalChecked());
		std::string sourceline_string = to_string(sourceline);
		LOG_ERROR("linenum:%d, %s", linenum, sourceline_string.c_str());
			//打印堆栈
		Local<Value> stacktrace_value;
		if (try_catch->StackTrace(context).ToLocal(&stacktrace_value) &&
    			stacktrace_value->IsString() && Local<String>::Cast(stacktrace_value)->Length() > 0) {
			String::Utf8Value stacktrace(stacktrace_value);
			std::string stacktrace_string = to_string(stacktrace);
			LOG_ERROR("%s", stacktrace_string.c_str());
		}
	}
}

std::string get_stack_trace(Isolate* isolate) {
	HandleScope handle_scope(isolate);

	std::ostringstream stream;
	stream << "v8 stacktrace:\n";
	Local<v8::StackTrace> stack = StackTrace::CurrentStackTrace(isolate, 10, StackTrace::kDetailed);
	for (int i = 0; i < stack->GetFrameCount() - 1; i++) {
		Local<StackFrame> stack_frame = stack->GetFrame(i);
		String::Utf8Value function_name(stack_frame->GetFunctionName());
		String::Utf8Value script_name(stack_frame->GetScriptNameOrSourceURL());
		const int line_number = stack_frame->GetLineNumber();
		const int column = stack_frame->GetColumn();

		char stack_str[512] = {0};
		//js脚本是否通过eval函数调用
		if (stack_frame->IsEval()) {
			if (stack_frame->GetScriptId() == Message::kNoScriptIdInfo) {
				sprintf(stack_str, "	[eval] :%d:%d\n", line_number, column);
			} else {
				sprintf(stack_str, "	[eval] (%s:%d:%d)\n", *script_name, line_number, column);
			}
			stream << stack_str;
			break;
		}

		//函数名称是否为空
		if (function_name.length() == 0) {
			sprintf(stack_str, "	%s:%d:%d\n", *script_name, line_number, column);
		} else {
			sprintf(stack_str, "	%s(%s:%d:%d)\n", *function_name, *script_name, line_number, column);
		}
		stream << stack_str;
	}

	return stream.str();
}

void read_json(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 1) {
		LOG_FATAL("read_json fatal, args length: %d\n", args.Length());
		args.GetReturnValue().SetNull();
		return;
	}
	String::Utf8Value file(args[0]);
	if (*file == nullptr) {
		LOG_FATAL("read_json fatal, file is null\n");
		args.GetReturnValue().SetNull();
		return;
	}

	Local<String> source;
	if (!read_file(args.GetIsolate(), *file).ToLocal(&source)) {
		LOG_ERROR("read_file:%s fail\n", *file);
		args.GetReturnValue().SetNull();
		return;
	}
	args.GetReturnValue().Set(source);
}

void read_xml(const FunctionCallbackInfo<Value>& args) {
	Local<Context> context(args.GetIsolate()->GetCurrentContext());
	String::Utf8Value path_str(args[0]->ToString(context).ToLocalChecked());
	std::string path = to_string(path_str);

	Xml xml(path.c_str());
	TiXmlNode *node = xml.enter_root_node();
	if(node == nullptr) {
		args.GetReturnValue().SetNull();
		return;
	}
	Local<Object> object = Object::New(args.GetIsolate());
	read_xml_data(args.GetIsolate(), xml, node, object);
	args.GetReturnValue().Set(object);
}

void read_xml_data(Isolate *isolate, Xml &xml, TiXmlNode *node, Local<Object> &object) {
	Local<Context> context(isolate->GetCurrentContext());
	std::unordered_map<std::string, uint> count_map;
	XML_LOOP_BEGIN(node)
		std::string node_key = xml.get_key(node);
		Local<Object> obj = Object::New(isolate);
		//获取node节点属性值
		if(xml.has_attribute(node)) {
			XML_ATTR_LOOP_BEGIN(node, attr)
				Local<String> key = String::NewFromUtf8(isolate, attr->Name(), NewStringType::kNormal).ToLocalChecked();
				Local<Value> value;
				int string_type = get_string_type(attr->Value());
				switch(string_type){
				case 0:
					value = String::NewFromUtf8(isolate, attr->Value(), NewStringType::kNormal).ToLocalChecked();
					break;
				case 1:
					value = Int32::New(isolate, attr->IntValue());
					break;
				case 2:
					value = Number::New(isolate, attr->DoubleValue());
					break;
				default:
					LOG_ERROR("get string type error, type:%d", string_type);
					break;
				}
				obj->Set(context, key, value);
			XML_ATTR_LOOP_END(attr)
		}
		//如果node有子节点，递归遍历
		if(xml.has_child(node)) {
			read_xml_data(isolate, xml, xml.enter_node(node, node_key.c_str()), obj);
		}

		//判断相同名字节点数量，如果数量大于1，放到数组里面
		Local<String> node_key_string = String::NewFromUtf8(isolate, node_key.c_str(), NewStringType::kNormal).ToLocalChecked();
		std::unordered_map<std::string, uint>::iterator iter = count_map.find(node_key);
		if(iter != count_map.end()){
			Local<Array> array = Local<Array>::Cast(object->Get(node_key_string));
			array->Set(context, iter->second, obj);
			iter->second++;
		}
		else {
			int count = xml.count_key(node, node_key.c_str());
			if(count == 1) {
				object->Set(context, node_key_string, obj);
			}
			else if(count > 1) {
				Local<Array> array = Array::New(isolate, count);
				array->Set(context, 0, obj);
				object->Set(context, node_key_string, array);
				count_map[node_key] = 1;
			}
		}
	XML_LOOP_END(node)
}

void get_proc_info(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context(isolate->GetCurrentContext());

	uint32_t pid = getpid();
	double cpu_percent = get_cpu_used_percent(pid);
	proc_pid_mem_t pid_used = get_pid_mem_used(pid);

	Local<Object> object = Object::New(isolate);
	object->Set(context, String::NewFromUtf8(isolate, "cpu_percent", NewStringType::kNormal).ToLocalChecked(),
		Number::New(isolate, cpu_percent));
	object->Set(context, String::NewFromUtf8(isolate, "vm_size", NewStringType::kNormal).ToLocalChecked(),
		Number::New(isolate, pid_used.VmSize));
	object->Set(context, String::NewFromUtf8(isolate, "vm_rss", NewStringType::kNormal).ToLocalChecked(),
		Number::New(isolate, pid_used.VmRSS));
	object->Set(context, String::NewFromUtf8(isolate, "vm_data", NewStringType::kNormal).ToLocalChecked(),
		Number::New(isolate, pid_used.VmData));
	args.GetReturnValue().Set(object);
}

void get_heap_info(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context(isolate->GetCurrentContext());

	HeapStatistics v8_heap_stats;
	isolate->GetHeapStatistics(&v8_heap_stats);
	Local<Number> heap_total = Number::New(isolate, v8_heap_stats.total_heap_size());
	Local<Number> heap_used = Number::New(isolate, v8_heap_stats.used_heap_size());
	Local<Number> external_mem = Number::New(isolate, args.GetIsolate()->AdjustAmountOfExternalAllocatedMemory(0));

	Local<Object> object = Object::New(isolate);
	object->Set(context, String::NewFromUtf8(isolate, "heap_total", NewStringType::kNormal).ToLocalChecked(), heap_total);
	object->Set(context, String::NewFromUtf8(isolate, "heap_used", NewStringType::kNormal).ToLocalChecked(), heap_used);
	object->Set(context, String::NewFromUtf8(isolate, "external_mem", NewStringType::kNormal).ToLocalChecked(), external_mem);
	args.GetReturnValue().Set(object);
}

void require(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 1) {
		LOG_ERROR("require args error, length: %d\n", args.Length());
		return ;
	}

	String::Utf8Value str(args[0]);
	if (*str != NULL) {
		run_script(args.GetIsolate(), *str);
	} else {
		LOG_ERROR("require args error");
	}
}

void hash(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 1) {
		LOG_ERROR("hash args error, length: %d\n", args.Length());
		return ;
	}

	String::Utf8Value str(args[0]);
	std::string hash_str = to_string(str);
	if (hash_str == "") {
		args.GetReturnValue().Set(0);
	} else {
		double hash_value = elf_hash(hash_str.c_str());
		args.GetReturnValue().Set(hash_value);
	}
}

void generate_token(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 1) {
		LOG_ERROR("generate_token args error, length: %d\n", args.Length());
		return ;
	}

	String::Utf8Value str(args[0]);
	std::string token_str = to_string(str);
	if (token_str == "") {
		args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), "", NewStringType::kNormal).ToLocalChecked());
	} else {
		args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), make_token(token_str.c_str()).c_str(), NewStringType::kNormal).ToLocalChecked());
	}
}

void log_debug(const FunctionCallbackInfo<Value>& args) {
	HandleScope handle_scope(args.GetIsolate());

	std::ostringstream stream;
	for (int i = 0; i < args.Length(); i++) {
		String::Utf8Value str(args[i]);
		stream << to_string(str);
	}
	LOG_DEBUG("%s", stream.str().c_str());
}

void log_info(const FunctionCallbackInfo<Value>& args) {
	HandleScope handle_scope(args.GetIsolate());

	std::ostringstream stream;
	for (int i = 0; i < args.Length(); i++) {
		String::Utf8Value str(args[i]);
		stream << to_string(str);
	}
	LOG_INFO("%s", stream.str().c_str());
}

void log_warn(const FunctionCallbackInfo<Value>& args) {
	HandleScope handle_scope(args.GetIsolate());

	std::ostringstream stream;
	for (int i = 0; i < args.Length(); i++) {
		String::Utf8Value str(args[i]);
		stream << to_string(str);
	}
	LOG_WARN("%s", stream.str().c_str());
}

void log_error(const FunctionCallbackInfo<Value>& args) {
	HandleScope handle_scope(args.GetIsolate());

	std::ostringstream stream;
	for (int i = 0; i < args.Length(); i++) {
		String::Utf8Value str(args[i]);
		stream << to_string(str);
	}
	LOG_ERROR("%s", stream.str().c_str());
}

void log_trace(const FunctionCallbackInfo<Value>& args) {
	HandleScope handle_scope(args.GetIsolate());

	std::ostringstream stream;
	for (int i = 0; i < args.Length(); i++) {
		String::Utf8Value str(args[i]);
		stream << to_string(str);
	}
	LOG_TRACE("%s", stream.str().c_str());
}
