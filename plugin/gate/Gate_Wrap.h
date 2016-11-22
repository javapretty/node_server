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

//函数说明：增加客户端连接  	参数：1.session信息    返回值：无
void add_session(const FunctionCallbackInfo<Value>& args);
//函数说明：移除客户端连接  	参数：1.cid    返回值：无
void remove_session(const FunctionCallbackInfo<Value>& args);


#endif /* GATE_WRAP_H_ */
