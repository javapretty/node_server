/*
 * V8_Wrap.h
 *
 *  Created on: Feb 1, 2016
 *      Author: zhangyalei
 */

#ifndef V8_WRAP_H_
#define V8_WRAP_H_

#include "include/v8.h"
#include "DB_Struct.h"
#include "Node_Define.h"

using namespace v8;

//获得消息结构体名称
std::string get_struct_name(int msg_type, int msg_id);

//创建v8运行环境
Local<Context> create_context(Isolate* isolate);
////函数说明：创建进程		参数：1.node_type 2.node_id 3.endpoint_gid 4.node_name 返回值：无
void fork_process(const FunctionCallbackInfo<Value>& args);
//函数说明：注册定时器到c++层		参数：1.定时器id 2.定时器间隔(毫秒单位) 3.从注册定时器到下次定时器到期中间间隔秒数	返回值：无
void register_timer(const FunctionCallbackInfo<Value>& args);
//函数说明：发送消息object	参数：1.eid 2.cid 3.msg_id 4.msg_type 5.sid 6.消息object   返回值：无
void send_msg(const FunctionCallbackInfo<Value>& args);
//函数说明：增加客户端连接  	参数：1.session    返回值：无
void add_session(const FunctionCallbackInfo<Value>& args);
//函数说明：关闭客户端连接  	参数：1.eid 2.drop_cid 3.error_code    返回值：无
void close_session(const FunctionCallbackInfo<Value>& args);

#endif /* V8_WRAP_H_ */
