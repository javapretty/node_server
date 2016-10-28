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

	virtual bool connect_to_db(int db_id, std::string &ip, int port, std::string &user, std::string &password, std::string &pool_name) { return false; };

	virtual v8::Local<v8::Object> load_data(int db_id, DB_Struct *db_struct, Isolate* isolate, int64_t key_index) { return v8::Local<v8::Object>(); };
	virtual void save_data(int db_id, DB_Struct *db_struct, Isolate* isolate, v8::Local<v8::Object> object) {};
	virtual void delete_data(int db_id, DB_Struct *db_struct, Isolate* isolate, v8::Local<v8::Object> object) {};
	
	virtual int load_data(int db_id, DB_Struct *db_struct, int64_t key_index, std::vector<Block_Buffer *> &buffer_vec) { return 0;};
	virtual void save_data(int db_id, DB_Struct *db_struct, Block_Buffer *buffer) {};
	virtual void delete_data(int db_id, DB_Struct *db_struct, Block_Buffer *buffer) {};

	virtual int init_db(int db_id, DB_Struct *db_struct) { return 0; };
	virtual int64_t generate_table_index(int db_id, DB_Struct *db_struct, std::string &type) { return 0; }
	virtual int64_t select_table_index(int db_id, DB_Struct *db_struct,  std::string &query_name, std::string &query_value) { return 0; }

protected:
	MUTEX connection_map_lock_;
};

#endif /* DB_OPERATOR_H_ */
