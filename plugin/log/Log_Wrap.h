/*
 * Log_Wrap.h
 *
 *  Created on: Nov 9, 2016
 *      Author: zhangyalei
 */

#ifndef LOG_WRAP_H_
#define LOG_WRAP_H_

#include "include/v8.h"

using namespace v8;

//函数说明：初始化log		参数：无 返回值：无
void init_log(const FunctionCallbackInfo<Value>& args);

#endif /* LOG_WRAP_H_ */
