/*
 * Gate_Wrap.h
 *
 *  Created on: Nov 8, 2016
 *      Author: zhangyalei
 */

#ifndef GATE_WRAP_H_
#define GATE_WRAP_H_

#include "include/v8.h"

using namespace v8;

//函数说明：初始化gate		参数：无 返回值：无
void init_gate(const FunctionCallbackInfo<Value>& args);
//函数说明：增加客户端连接  	参数：1.session    返回值：无
void add_session(const FunctionCallbackInfo<Value>& args);
//函数说明：移除客户端连接  	参数：1.eid 2.drop_cid 3.error_code    返回值：无
void remove_session(const FunctionCallbackInfo<Value>& args);


#endif /* GATE_WRAP_H_ */
