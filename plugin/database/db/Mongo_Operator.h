/*
 * Mongo_Operator.h
 *
 *  Created on: Dec 29, 2015
 *      Author: zhangyalei
 */

#ifndef MOMGO_OPERATOR_H_
#define MONGO_OPERATOR_H_

#ifdef MONGO_DB_IMPLEMENT

#include "mongo/client/dbclient.h"
#include <unordered_map>
#include "Object_Pool.h"
#include "DB_Operator.h"

using namespace mongo;

class Mongo_Operator : public DB_Operator {
public:
	typedef std::unordered_map<int, DBClientConnection *> Connection_Map;
	//typedef std::unordered_map<std::string, Connection_Map *> DB_Connection_Map;
	typedef Object_Pool<DBClientConnection, Mutex_Lock> Connection_Pool;

	Mongo_Operator(void);
	virtual ~Mongo_Operator(void);

	virtual bool connect_to_db(int db_id, std::string &ip, int port, std::string &user, std::string &password, std::string &pool_name);
	virtual int init_db(int db_id, DB_Struct *db_struct);
	virtual int64_t generate_table_index(int db_id, DB_Struct *db_struct, std::string &type);
	virtual int64_t select_table_index(int db_id, DB_Struct *db_struct,  std::string &query_name, std::string &query_value);

	///数据库加载出来的数据转换城buffer，缓存在内存中，保存数据时候通过buffer取数据
	virtual int load_data(int db_id, DB_Struct *db_struct, int64_t key_index, std::vector<Bit_Buffer *> &buffer_vec);
	virtual void save_data(int db_id, DB_Struct *db_struct, Bit_Buffer *buffer);
	//删除数据时候传入索引数组
	virtual void delete_data(int db_id, DB_Struct *db_struct, Isolate* isolate, v8::Local<v8::Object> object);

private:
	mongo::DBClientConnection &get_connection(int db_id);

	//数据输入：bsonobj  数据输出：buffer
	void load_data_single(DB_Struct *db_struct, BSONObj &bsonobj, Bit_Buffer &buffer);
	void load_data_arg(DB_Struct *db_struct, const Field_Info &field_info, BSONObj &bsonobj, Bit_Buffer &buffer);
	void load_data_vector(DB_Struct *db_struct, const Field_Info &field_info, BSONObj &bsonobj, Bit_Buffer &buffer);
	void load_data_struct(DB_Struct *db_struct, const Field_Info &field_info, BSONObj &bsonobj, Bit_Buffer &buffer);

		//数据输入：buffer  数据输出：builder
	void save_data_arg(DB_Struct *db_struct, const Field_Info &field_info, BSONObjBuilder &builder, Bit_Buffer &buffer);
	void save_data_vector(DB_Struct *db_struct, const Field_Info &field_info, BSONObjBuilder &builder, Bit_Buffer &buffer);
	void save_data_struct(DB_Struct *db_struct, const Field_Info &field_info, BSONObjBuilder &builder, Bit_Buffer &buffer);

private:
	Connection_Pool connection_pool_;
	Connection_Map connection_map_;
};

#endif

#endif /* MONGO_OPERATOR_H_ */
