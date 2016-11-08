/*
 * DB_Wrap.h
 *
 *  Created on: Nov 1, 2016
 *      Author: zhangyalei
 */

#ifndef DB_WRAP_H_
#define DB_WRAP_H_

#include "include/v8.h"
#include "DB_Struct.h"
#include "Data_Manager.h"

using namespace v8;

//函数说明：初始化数据操作类		参数：无 返回值：无
void init_db_operator(const FunctionCallbackInfo<Value>& args);
//函数说明：连接mysql数据库数据		参数：1.db_id 2.ip 3.port 4.user 5.password 6.pool_name	返回值：连接成功
void connect_mysql(const FunctionCallbackInfo<Value>& args);
//函数说明：连接mongo数据库数据		参数：1.db_id 2.ip 3.port	 返回值：连接成功
void connect_mongo(const FunctionCallbackInfo<Value>& args);
//函数说明：生成表索引	参数：1.db_id 2.table_name 3. type 返回值：id
void generate_table_index(const FunctionCallbackInfo<Value>& args);
//函数说明：获取表索引	参数：1.db_id 2.table_name 3.query_name 4.query_value 返回值：数据库索引
void select_table_index(const FunctionCallbackInfo<Value>& args);
//函数说明：读取数据库数据		参数：1.db_id, 2.table_name 3.key_index  返回值：数据object
void load_db_data(const FunctionCallbackInfo<Value>& args);
//函数说明：保存数据库数据		参数：1.保存类型(0.只更新缓存1.更新缓存和数据库2.更新数据库清空缓存 默认0) 2.db_id 3.table_name 4.数据object 返回值：无
void save_db_data(const FunctionCallbackInfo<Value>& args);
//函数说明：删除数据库数据		参数：1.db_id 2.table_name 3.数据array		返回值：无
void delete_db_data(const FunctionCallbackInfo<Value>& args);
//函数说明：加载单张表数据		参数：1.db_id 2.table_name 3.数据object	返回值：无
v8::Local<v8::Object> load_single_data(Isolate* isolate, int db_id, DB_Struct *db_struct, int64_t key_index);
//函数说明：保存单张表数据		参数：1.db_id 2.table_name 3.数据object	返回值：无
void save_single_data(Isolate* isolate, int db_id, std::string &table_name, Local<v8::Object> object, int flag);

//函数说明：保存运行时数据		参数：1.key_name, 2.index 3.数据object		返回值：无
void set_runtime_data(const FunctionCallbackInfo<Value>& args);
//函数说明：获取运行时数据		参数：1.key_name, 2.index	返回值：无
void get_runtime_data(const FunctionCallbackInfo<Value>& args);
//函数说明：删除运行时数据		参数：1.key_name, 2.index 返回值：无
void delete_runtime_data(const FunctionCallbackInfo<Value>& args);

#endif /* DB_WRAP_H_ */
