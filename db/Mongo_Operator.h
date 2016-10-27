/*
 * Mongo_Operator.h
 *
 *  Created on: Dec 29, 2015
 *      Author: zhangyalei
 */

#ifndef MOMGO_OPERATOR_H_
#define MONGO_OPERATOR_H_

#include "mongo/client/dbclient.h"
#include "boost/unordered_map.hpp"
#include "Object_Pool.h"
#include "DB_Operator.h"

using namespace mongo;

class Mongo_Operator : public DB_Operator {
public:
	typedef boost::unordered_map<int, DBClientConnection *> Connection_Map;
	//typedef boost::unordered_map<std::string, Connection_Map *> DB_Connection_Map;
	typedef Object_Pool<DBClientConnection, Thread_Mutex> Connection_Pool;

	Mongo_Operator(void);
	virtual ~Mongo_Operator(void);

	virtual bool connect_to_db(int db_id, std::string &ip, int port, std::string &user, std::string &password, std::string &pool_name);

	virtual v8::Local<v8::Object> load_data(int db_id, DB_Struct *db_struct, Isolate* isolate, int64_t key_index);
	virtual void save_data(int db_id, DB_Struct *db_struct, Isolate* isolate, v8::Local<v8::Object> object);
	virtual void delete_data(int db_id, DB_Struct *db_struct, Isolate* isolate, v8::Local<v8::Object> object);

	virtual int init_db(int db_id, DB_Struct *db_struct);
	virtual int64_t generate_table_index(int db_id, DB_Struct *db_struct, std::string &type);
	virtual int64_t select_table_index(int db_id, DB_Struct *db_struct,  std::string &query_name, std::string &query_value);

private:
	mongo::DBClientConnection &get_connection(int db_id);

	//数据输入：bsonobj  数据输出：函数返回值
	v8::Local<v8::Object> load_data_single(DB_Struct *db_struct, Isolate* isolate, BSONObj &bsonobj);
	v8::Local<v8::Value> load_data_arg(DB_Struct *db_struct, Isolate* isolate, const Field_Info &field_info, BSONObj &bsonobj);
	v8::Local<v8::Array> load_data_vector(DB_Struct *db_struct, Isolate* isolate, const Field_Info &field_info, BSONObj &bsonobj);
	v8::Local<v8::Map> load_data_map(DB_Struct *db_struct, Isolate* isolate, const Field_Info &field_info, BSONObj &bsonobj);
	v8::Local<v8::Object> load_data_struct(DB_Struct *db_struct, Isolate* isolate, const Field_Info &field_info, BSONObj &bsonobj);

	//数据输入：value  数据输出：builder
	void save_data_arg(DB_Struct *db_struct, Isolate* isolate, const Field_Info &field_info, BSONObjBuilder &builder, v8::Local<v8::Value> value);
	void save_data_vector(DB_Struct *db_struct, Isolate* isolate, const Field_Info &field_info, BSONObjBuilder &builder, v8::Local<v8::Value> value);
	void save_data_map(DB_Struct *db_struct, Isolate* isolate, const Field_Info &field_info, BSONObjBuilder &builder, v8::Local<v8::Value> value);
	void save_data_struct(DB_Struct *db_struct, Isolate* isolate, const Field_Info &field_info, BSONObjBuilder &builder, v8::Local<v8::Value> value);

private:
	Connection_Pool connection_pool_;
	Connection_Map connection_map_;
};

#endif /* MONGO_OPERATOR_H_ */
