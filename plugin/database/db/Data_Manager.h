/*
 * Data_Manager.h
 *
 *  Created on: Oct 27, 2016
 *      Author: zhangyalei
 */

#ifndef DATA_MANAGER_H_
#define DATA_MANAGER_H_

#include <unordered_map>
#include "Object_Pool.h"
#include "DB_Operator.h"

enum SAVE_FLAG {
	SAVE_BUFFER = 0,
	SAVE_BUFFER_DB = 1,
	SAVE_DB = 2,
};

class Data_Manager {
public:
	typedef std::unordered_map<int, DB_Operator *> DB_Operator_Map;					//db_id--DB_Operator
	typedef std::unordered_map<int64_t, Bit_Buffer *> Record_Buffer_Map; 	//index--buffer
	typedef std::unordered_map<std::string, Record_Buffer_Map *> Table_Buffer_Map;//table_name--record
	typedef std::unordered_map<int, Table_Buffer_Map *> DB_Buffer_Map; 			//db_id--table_buffer
	typedef std::unordered_map<int64_t, Bit_Buffer *> Runtime_Buffer_Map;		//index--buffer
	typedef std::unordered_map<std::string, Runtime_Buffer_Map *> Runtime_Data_Map; //key_name--buffer_map
	typedef Object_Pool<Bit_Buffer, Thread_Mutex> Buffer_Pool;

	typedef std::unordered_map<std::string, int> TABLE_MAP;
public:
	static Data_Manager *instance(void);

	int init_db_operator();
	DB_Operator *db_operator(int type);
	int save_db_data(int db_id, DB_Struct *db_struct, Bit_Buffer *buffer, int flag);
	int load_db_data(int db_id, DB_Struct *db_struct, int64_t index, std::vector<Bit_Buffer *> &buffer_vec);
	int delete_db_data(int db_id, DB_Struct *db_struct, int64_t index);

	void set_runtime_data(int64_t index, DB_Struct *db_struct, Bit_Buffer *buffer);
	Bit_Buffer *get_runtime_data(int64_t index, DB_Struct *db_struct);
	void delete_runtime_data(int64_t index, DB_Struct *db_struct);

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

	Record_Buffer_Map *get_record_map(int db_id, std::string table_name);
	Runtime_Buffer_Map *get_runtime_buffer_map(std::string key_name);
private:
	static Data_Manager *instance_;
	DB_Operator_Map db_operator_map_;
	DB_Buffer_Map db_buffer_map_;
	Runtime_Data_Map runtime_data_map_;
	Buffer_Pool buffer_pool_;
};

#define DATA_MANAGER 	Data_Manager::instance()
#define DB_OPERATOR(db_id) DATA_MANAGER->db_operator((db_id))

#endif /* DATA_MANAGER_H_ */
