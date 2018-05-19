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

	//根据条件查询db数据
	virtual int select_db_data(int db_id, DB_Struct *db_struct, std::string &condition_name, std::string &condition_value, std::string &query_name, std::string &query_type, Bit_Buffer &buffer);
	//数据库加载数据到内存buffer,保存数据通过内存buffer操作
	virtual int load_data(int db_id, DB_Struct *db_struct, int64_t key_index, std::vector<Bit_Buffer *> &buffer_vec);
	virtual int save_data(int db_id, DB_Struct *db_struct, Bit_Buffer &buffer);
	virtual int delete_data(int db_id, DB_Struct *db_struct, int64_t key_index);

private:
	mongo::DBClientConnection &get_connection(int db_id);

	//数据输入：bsonobj  数据输出：buffer
	int load_data_single(DB_Struct *db_struct, BSONObj &bsonobj, Bit_Buffer &buffer);
	int load_data_arg(DB_Struct *db_struct, const Field_Info &field_info, BSONObj &bsonobj, Bit_Buffer &buffer);
	int load_data_vector(DB_Struct *db_struct, const Field_Info &field_info, BSONObj &bsonobj, Bit_Buffer &buffer);
	int load_data_map(DB_Struct *db_struct, const Field_Info &field_info, BSONObj &bsonobj, Bit_Buffer &buffer);
	int load_data_struct(DB_Struct *db_struct, const Field_Info &field_info, BSONObj &bsonobj, Bit_Buffer &buffer);

	//数据输入：buffer  数据输出：builder
	int save_data_arg(DB_Struct *db_struct, const Field_Info &field_info, BSONObjBuilder &builder, Bit_Buffer &buffer);
	int save_data_vector(DB_Struct *db_struct, const Field_Info &field_info, BSONObjBuilder &builder, Bit_Buffer &buffer);
	int save_data_map(DB_Struct *db_struct, const Field_Info &field_info, BSONObjBuilder &builder, Bit_Buffer &buffer);
	int save_data_struct(DB_Struct *db_struct, const Field_Info &field_info, BSONObjBuilder &builder, Bit_Buffer &buffer);

private:
	Connection_Pool connection_pool_;
	Connection_Map connection_map_;
};

#endif

#endif /* MONGO_OPERATOR_H_ */
