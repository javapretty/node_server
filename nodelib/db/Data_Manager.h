/*
 * Data_Manager.h
 *
 *  Created on: Oct 27, 2016
 *      Author: zhangyalei
 */

#ifndef DATA_MANAGER_H_
#define DATA_MANAGER_H_

#include <unordered_map>
#include "Base_Function.h"
#include "Object_Pool.h"
#include "DB_Operator.h"

class Data_Manager {
public:
	typedef std::unordered_map<int, DB_Operator *> DB_Operator_Map;					//db_id--DB_Operator
	typedef std::unordered_map<int64_t, Bit_Buffer *> Index_Buffer_Map; 			//index--buffer
	typedef std::unordered_map<std::string, Index_Buffer_Map *> Table_Buffer_Map;	//table_name--record
	typedef std::unordered_map<int, Table_Buffer_Map *> DB_Data_Map; 				//db_id--table_buffer
	typedef std::unordered_map<std::string, Index_Buffer_Map *> Runtime_Data_Map;	//key_name--buffer_map
	typedef Object_Pool<Bit_Buffer, Mutex_Lock> Buffer_Pool;
public:
	static Data_Manager *instance(void);

	//数据库初始化接口
	int init_db_operator();
	bool connect_to_db(int db_id, std::string &ip, int port, std::string &user, std::string &password, std::string &pool_name);

	//根据条件操作db数据接口
	int select_db_data(int db_id, std::string &struct_name, std::string &condition_name, std::string &condition_value, std::string &query_name, std::string &query_type, Bit_Buffer &buffer);
	//操作db数据接口
	int load_db_data(int db_id, const std::string &struct_name, int64_t key_index, Bit_Buffer &buffer);
	int save_db_data(int save_type, bool vector_data, int db_id, const std::string &struct_name, Bit_Buffer &buffer);
	int delete_db_data(int db_id, const std::string &struct_name, Bit_Buffer &buffer);

	//操作运行时数据接口
	int load_runtime_data(const std::string &struct_name, int64_t key_index, Bit_Buffer &buffer);
	int save_runtime_data(const std::string &struct_name, int64_t key_index, Bit_Buffer &buffer);
	int delete_runtime_data(const std::string &struct_name, int64_t key_index);

	//打印缓存数据
	void print_cache_data(void);

	inline Bit_Buffer *pop_buffer() { return buffer_pool_.pop(); };
	inline void push_buffer(Bit_Buffer *buffer) {
		buffer->reset();
		buffer_pool_.push(buffer);
	};

private:
	Data_Manager(void);
	virtual ~Data_Manager(void);
	Data_Manager(const Data_Manager &);
	const Data_Manager &operator=(const Data_Manager &);

	inline DB_Operator *get_db_operator(int db_id);
	inline int64_t get_key_index(DB_Struct *db_struct, Bit_Buffer &buffer);

	//操作db缓存和db数据库接口
	int load_db_data(int db_id, DB_Struct *db_struct, int64_t key_index, std::vector<Bit_Buffer *> &buffer_vec);
	int save_db_data(int save_type, int db_id, DB_Struct *db_struct, Bit_Buffer &buffer);
	int delete_db_data(int db_id, DB_Struct *db_struct, Bit_Buffer &buffer);

	Index_Buffer_Map *get_db_buffer_map(int db_id, std::string table_name);
	Index_Buffer_Map *get_runtime_buffer_map(std::string key_name);
private:
	static Data_Manager *instance_;
	DB_Operator_Map db_operator_map_;
	DB_Data_Map db_data_map_;
	Runtime_Data_Map runtime_data_map_;
	Buffer_Pool buffer_pool_;
};

#define DATA_MANAGER 	Data_Manager::instance()

DB_Operator *Data_Manager::get_db_operator(int db_id) {
	DB_Operator_Map::iterator iter = db_operator_map_.find(db_id / 1000);
	if(iter != db_operator_map_.end()) {
		return iter->second;
	} else {
		LOG_ERROR("db_id %d error", db_id);
		return nullptr;
	}
}

int64_t Data_Manager::get_key_index(DB_Struct *db_struct, Bit_Buffer &buffer) {
	//计算主键值，如果主键类型是int64,直接取值，如果是string，取出string值，进行elf_hash
	int64_t key_index = 0;
	if(db_struct->index_type() == "int64") {
		key_index = buffer.peek_int64();
	} else if(db_struct->index_type() == "string") {
		std::string hash_str = "";
		buffer.peek_str(hash_str);
		key_index = elf_hash(hash_str.c_str());
	}
	else {
		key_index = -1;
		LOG_ERROR("get key_index fail, key_index:%d, index_type:%s, struct_name:%s", key_index, db_struct->index_type().c_str(), db_struct->struct_name().c_str());
	}

	return key_index;
}

#endif /* DATA_MANAGER_H_ */
