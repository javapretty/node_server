/*
 * Mongo_Operator.h
 *
 *  Created on: Dec 18, 2016
 *      Author: lijunliang
 */

#ifndef DB_OPERATOR_H_
#define DB_OPERATOR_H_

#include "DB_Struct.h"

class DB_Operator {
public:
	DB_Operator(void) {};
	virtual ~DB_Operator(void) {};

	virtual bool connect_to_db(int db_id, std::string &ip, int port, std::string &user, std::string &password, std::string &pool_name) { return false; }

	//根据条件查询db数据
	virtual int select_db_data(int db_id, DB_Struct *db_struct, std::string &condition_name, std::string &condition_value, std::string &query_name, std::string &query_type, Bit_Buffer &buffer) { return 0;}
	//数据库加载数据到内存buffer,保存数据通过内存buffer操作
	virtual int load_data(int db_id, DB_Struct *db_struct, int64_t key_index, std::vector<Bit_Buffer *> &buffer_vec) { return 0; }
	virtual int save_data(int db_id, DB_Struct *db_struct, Bit_Buffer &buffer) { return 0; }
	virtual int delete_data(int db_id, DB_Struct *db_struct, int64_t key_index) { return 0; }

protected:
	MUTEX_LOCK connection_map_lock_;
};

#endif /* DB_OPERATOR_H_ */
