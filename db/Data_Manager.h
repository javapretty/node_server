/*
 * Data_Manager.h
 *
 *  Created on: Oct 27, 2016
 *      Author: zhangyalei
 */

#ifndef DATA_MANAGER_H_
#define DATA_MANAGER_H_

#include "boost/unordered_map.hpp"
#include "DB_Operator.h"
#include "Object_Pool.h"

#define MAX_RECORD_NUM 10000

class Data_Manager {
public:
	typedef boost::unordered_map<int, DB_Operator *> DB_Operator_Map;					//db_id--DB_Operator
	typedef boost::unordered_map<int64_t, Block_Buffer *> Record_Buffer_Map; 	//index--buffer
	typedef boost::unordered_map<std::string, Record_Buffer_Map *> Table_Buffer_Map;//table_name--record
	typedef boost::unordered_map<int, Table_Buffer_Map *> DB_Buffer_Map; 			//db_id--table_buffer
	typedef boost::unordered_map<int64_t, Block_Buffer *> Runtime_Data_Map;		//index--buffer
	typedef Object_Pool<Block_Buffer, Thread_Mutex> Buffer_Pool;
public:
	static Data_Manager *instance(void);

	int init_db_operator();
	DB_Operator *db_operator(int type);
	int save_db_data(int db_id, DB_Struct *db_struct, Block_Buffer *buffer, int flag);
	int load_db_data(int db_id, DB_Struct *db_struct, int64_t index, std::vector<Block_Buffer *> &buffer_vec);
	int delete_db_data(int db_id, DB_Struct *db_struct, Block_Buffer *buffer);

	void set_runtime_data(int64_t index, DB_Struct *db_struct, Block_Buffer *buffer);
	Block_Buffer *get_runtime_data(int64_t index, DB_Struct *db_struct);
	void delete_runtime_data(int64_t index);

	inline Block_Buffer *pop_buffer(){return buffer_pool_.pop();};
	inline void push_buffer(Block_Buffer *buffer){buffer->reset();buffer_pool_.push(buffer);};

private:
	Data_Manager(void);
	virtual ~Data_Manager(void);
	Data_Manager(const Data_Manager &);
	const Data_Manager &operator=(const Data_Manager &);

	Record_Buffer_Map *get_record_map(int db_id, std::string table_name);
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
