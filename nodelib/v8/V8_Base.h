/*
 * V8_Base.h
 *
 *  Created on: Oct 21, 2016
 *      Author: zhangyalei
 */

#ifndef V8_BASE_H_
#define V8_BASE_H_

#include <string>
#include "Xml.h"
#include "include/v8.h"

using namespace v8;

std::string get_struct_name(int msg_type, int msg_id);
std::string to_string(const String::Utf8Value& value);
int run_script(Isolate* isolate, const char* file_path);
MaybeLocal<v8::String> read_file(Isolate* isolate, const char* file_path);
void report_exception(Isolate* isolate, TryCatch* handler, const char* file_path);
std::string get_stack_trace(Isolate* isolate);

//函数说明：获取系统proc信息   参数：无   返回值：proc信息对象
void get_proc_info(const FunctionCallbackInfo<Value>& args);
//函数说明：获取v8堆信息   参数：无   返回值：堆信息对象
void get_heap_info(const FunctionCallbackInfo<Value>& args);

//函数说明：引用js文件   参数：1.文件路径   返回值：无
void require(const FunctionCallbackInfo<Value>& args);
//函数说明：读取json配置文件   参数：1.文件路径   返回值：文件内容字符串对象
void read_json(const FunctionCallbackInfo<Value>& args);
//函数说明：读取xml配置文件   参数：1.文件路径   返回值：文件内容字符串对象
void read_xml(const FunctionCallbackInfo<Value>& args);
void read_xml_data(Isolate *isolate, Xml &xml, TiXmlNode *node, Local<Object> &object);
//函数说明：hash运算   参数：进行hash运算的字符串 返回值：hash值
void hash(const FunctionCallbackInfo<Value>& args);
//函数说明：生成token  参数：1.帐号名    返回值：token
void generate_token(const FunctionCallbackInfo<Value>& args);

//函数说明：打印程序调试信息	参数：可变参数列表   返回值：无
void log_debug(const FunctionCallbackInfo<Value>& args);
//函数说明：打印程序运行信息	参数：可变参数列表   返回值：无
void log_info(const FunctionCallbackInfo<Value>& args);
//函数说明：打印程序警告信息	参数：可变参数列表   返回值：无
void log_warn(const FunctionCallbackInfo<Value>& args);
//函数说明：打印程序错误信息	参数：可变参数列表   返回值：无
void log_error(const FunctionCallbackInfo<Value>& args);
//函数说明：打印程序运行堆栈	参数：可变参数列表   返回值：无
void log_trace(const FunctionCallbackInfo<Value>& args);

#endif /* V8_BASE_H_ */
