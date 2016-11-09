/*
 * DB_Wrap.h
 *
 *  Created on: Nov 1, 2016
 *      Author: zhangyalei
 */

#ifndef DB_WRAP_H_
#define DB_WRAP_H_

#include "include/v8.h"
using namespace v8;

//函数说明：初始化db		参数：无 返回值：无
void init_db(const FunctionCallbackInfo<Value>& args);

#endif /* DB_WRAP_H_ */
