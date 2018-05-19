/*
 * Data_Manager.cpp
 *
 *  Created on: 2016年10月27日
 *      Author: zhangyalei
 */

#include "Base_Enum.h"
#include "Mysql_Operator.h"
#ifdef MONGO_DB_IMPLEMENT
#include "Mongo_Operator.h"
#endif
#include "Struct_Manager.h"
#include "Data_Manager.h"

Data_Manager::Data_Manager(void) :
	db_operator_map_(get_hash_table_size(512)),
	db_data_map_(get_hash_table_size(512)),
	runtime_data_map_(get_hash_table_size(4096))
{ }

Data_Manager::~Data_Manager(void) { 
	for(DB_Operator_Map::iterator iter = db_operator_map_.begin(); iter != db_operator_map_.end(); ++iter) {
		if(iter->second) {
			delete iter->second;
		}
	}

	for(DB_Data_Map::iterator iter = db_data_map_.begin(); iter != db_data_map_.end(); ++iter) {
		for(Table_Buffer_Map::iterator iter_table = iter->second->begin(); iter_table != iter->second->end(); ++iter_table) {
			for(Index_Buffer_Map::iterator iter_record = iter_table->second->begin(); iter_record != iter_table->second->end(); ++iter_record) {
				push_buffer(iter_record->second);
			}
			iter_table->second->clear();
			delete iter_table->second;
		}
		iter->second->clear();
		delete iter->second;
	}

	for(Runtime_Data_Map::iterator iter = runtime_data_map_.begin(); iter != runtime_data_map_.end(); ++iter) {
		for(Index_Buffer_Map::iterator iter_record = iter->second->begin(); iter_record != iter->second->end(); ++iter_record) {
			push_buffer(iter_record->second);
		}
		iter->second->clear();
		delete iter->second;
	}
}

Data_Manager *Data_Manager::instance_;

Data_Manager *Data_Manager::instance(void) {
	if (instance_ == 0)
		instance_ = new Data_Manager;
	return instance_;
}

int Data_Manager::init_db_operator() {
#ifdef MONGO_DB_IMPLEMENT
	Mongo_Operator *mongo = new Mongo_Operator();
	db_operator_map_[MONGO] = mongo;
#endif
	Mysql_Operator *mysql = new Mysql_Operator();
	db_operator_map_[MYSQL] = mysql;
	return 0;
}

bool Data_Manager::connect_to_db(int db_id, std::string &ip, int port, std::string &user, std::string &password, std::string &pool_name) {
	DB_Operator *db_operator = get_db_operator(db_id);
	if(db_operator == nullptr){
		return false;
	}
	return db_operator->connect_to_db(db_id, ip, port, user, password, pool_name);
}

int Data_Manager::select_db_data(int db_id, std::string &struct_name, std::string &condition_name, std::string &condition_value, std::string &query_name, std::string &query_type, Bit_Buffer &buffer) {
	DB_Struct *db_struct = STRUCT_MANAGER->get_db_struct(struct_name);
	if(db_struct == nullptr) {
		return -1;
	}

	DB_Operator *db_operator = get_db_operator(db_id);
	if(db_operator == nullptr){
		return -1;
	}

	return db_operator->select_db_data(db_id, db_struct, condition_name, condition_value, query_name, query_type, buffer);
}

int Data_Manager::load_db_data(int db_id, const std::string &struct_name, int64_t key_index, Bit_Buffer &buffer) {
	DB_Struct *db_struct = STRUCT_MANAGER->get_db_struct(struct_name);
	if(db_struct == nullptr) {
		return -1;
	}

	if(db_struct->table_name() == "" && key_index != 0) {
		//数据库表名为空，表示该结构体为多张数据库表的集合，对每张表单独处理，此时长度只能为1，写在最前面
		for(Field_Vec::const_iterator iter = db_struct->field_vec().begin();
				iter != db_struct->field_vec().end(); ++iter) {
			DB_Struct *sub_struct = STRUCT_MANAGER->get_db_struct(iter->field_type);
			if(sub_struct == nullptr) {
				return -1;
			}

			std::vector<Bit_Buffer *> buffer_vec;
			int ret = load_db_data(db_id, sub_struct, key_index, buffer_vec);
			if(ret < 0) {
				return ret;
			}
			if(buffer_vec.size() <= 0) {
				return -2;
			}

			for(uint i = 0; i < buffer_vec.size(); i++) {
				int ret = sub_struct->build_bit_buffer(sub_struct->field_vec(), *buffer_vec[i], buffer);
				if(ret < 0) {
					return ret;
				}
			}
		}
	}
	else {
		//单张表数据处理，可能加载整张表数据，key_index可能为0
		std::vector<Bit_Buffer *> buffer_vec;
		int ret = load_db_data(db_id, db_struct, key_index, buffer_vec);
		if (ret < 0) {
			return ret;
		}
		if(buffer_vec.size() <= 0) {
			return -2;
		}

		uint length = buffer_vec.size();
		if(key_index == 0) {
			buffer.write_uint(length, 16);
		}
		for(uint i = 0; i < length; i++) {
			int ret = db_struct->build_bit_buffer(db_struct->field_vec(), *buffer_vec[i], buffer);
			if(ret < 0) {
				return ret;
			}
		}
	}
	return 0;
}

int Data_Manager::save_db_data(int save_type, bool vector_data, int db_id, const std::string &struct_name, Bit_Buffer &buffer) {
	DB_Struct *db_struct = STRUCT_MANAGER->get_db_struct(struct_name);
	if(db_struct == nullptr) {
		return -1;
	}

	uint length = 1;
	if(vector_data) {
		length = buffer.read_uint(16);
	}
	for(uint i = 0; i < length; ++i) {
		if(db_struct->table_name() == "") {
			//数据库表名为空，表示该结构体为多张数据库表的集合，对每张表单独处理，此时长度只能为1，写在最前面
			for(Field_Vec::const_iterator iter = db_struct->field_vec().begin();
					iter != db_struct->field_vec().end(); ++iter) {
				DB_Struct *sub_struct = STRUCT_MANAGER->get_db_struct(iter->field_type);
				if(sub_struct == nullptr) {
					LOG_ERROR("db_struct %s is null", iter->field_type.c_str());
					return -1;
				}

				Bit_Buffer dst_buffer;
				int ret = sub_struct->build_bit_buffer(sub_struct->field_vec(), buffer, dst_buffer);
				if(ret < 0) {
					return ret;
				}
				ret = save_db_data(save_type, db_id, sub_struct, dst_buffer);
				if(ret < 0) {
					return ret;
				}
			}
		}
		else {
			//单张表数据处理
			Bit_Buffer dst_buffer;
			int ret = db_struct->build_bit_buffer(db_struct->field_vec(), buffer, dst_buffer);
			if(ret < 0) {
				return ret;
			}
			ret = save_db_data(save_type, db_id, db_struct, dst_buffer);
			if(ret < 0) {
				return ret;
			}
		}
	}
	return 0;
}

int Data_Manager::delete_db_data(int db_id, const std::string &struct_name, Bit_Buffer &buffer) {
	DB_Struct *db_struct = STRUCT_MANAGER->get_db_struct(struct_name);
	if(db_struct == nullptr) {
		return -1;
	}

	return delete_db_data(db_id, db_struct, buffer);
}

int Data_Manager::load_runtime_data(const std::string &struct_name, int64_t key_index, Bit_Buffer &buffer) {
	DB_Struct *db_struct = STRUCT_MANAGER->get_db_struct(struct_name);
	if(db_struct == nullptr) {
		return -1;
	}

	Index_Buffer_Map *runtime_buffer_map = get_runtime_buffer_map(struct_name);
	Index_Buffer_Map::iterator iter = runtime_buffer_map->find(key_index);
	if(iter != runtime_buffer_map->end()) {
		int ret = db_struct->build_bit_buffer(db_struct->field_vec(), *(iter->second), buffer);
		if(ret < 0) {
			return ret;
		}
	}
	else {
		LOG_ERROR("can't find runtime data, struct_name:%s, key_index:%ld", struct_name.c_str(), key_index);
		return -1;
	}
	return 0;
}

int Data_Manager::save_runtime_data(const std::string &struct_name, int64_t key_index, Bit_Buffer &buffer) {
	Index_Buffer_Map *runtime_buffer_map = get_runtime_buffer_map(struct_name);
	Index_Buffer_Map::iterator iter = runtime_buffer_map->find(key_index);
	if(iter == runtime_buffer_map->end()) {
		Bit_Buffer *buf = pop_buffer();
		buf->set_ary(buffer.data(), buffer.get_byte_size());
		runtime_buffer_map->insert(std::make_pair(key_index, buf));
	}
	else {
		iter->second->set_ary(buffer.data(), buffer.get_byte_size());
	}
	return 0;
}

int Data_Manager::delete_runtime_data(const std::string &struct_name, int64_t key_index) {
	Index_Buffer_Map *runtime_buffer_map = get_runtime_buffer_map(struct_name);
	Index_Buffer_Map::iterator iter = runtime_buffer_map->find(key_index);
	if(iter != runtime_buffer_map->end()) {
		push_buffer(iter->second);
		runtime_buffer_map->erase(iter);
	} else {
		return -1;
	}

	return 0;
}

int Data_Manager::load_db_data(int db_id, DB_Struct *db_struct, int64_t key_index, std::vector<Bit_Buffer *> &buffer_vec) {
	DB_Operator *db_operator = get_db_operator(db_id);
	if(db_operator == nullptr){
		return -1;
	}

	Index_Buffer_Map *db_buffer_map = get_db_buffer_map(db_id, db_struct->table_name());
	if(key_index == 0) {
		//加载整张表数据
		if(db_buffer_map->empty()) {
			int ret = db_operator->load_data(db_id, db_struct, key_index, buffer_vec);
			if(ret < 0) {
				return ret;
			}

			for(std::vector<Bit_Buffer *>::iterator iter = buffer_vec.begin();
					iter != buffer_vec.end(); iter++){
				int64_t record_index = get_key_index(db_struct, *(*iter));
				if(record_index < 0) {
					continue;
				}
				db_buffer_map->insert(std::make_pair(record_index, *iter));
			}
		}
		else {
			for(Index_Buffer_Map::iterator iter = db_buffer_map->begin();
					iter != db_buffer_map->end(); iter++){
				buffer_vec.push_back(iter->second);
			}
		}
	}
	else {
		//加载单条数据
		Index_Buffer_Map::iterator iter = db_buffer_map->find(key_index);
		if(iter == db_buffer_map->end()) {
			int ret = db_operator->load_data(db_id, db_struct, key_index, buffer_vec);
			if(ret < 0) {
				return ret;
			}

			for(std::vector<Bit_Buffer *>::iterator iter = buffer_vec.begin();
					iter != buffer_vec.end(); iter++){
				int64_t record_index = get_key_index(db_struct, *(*iter));
				if(record_index < 0) {
					continue;
				}
				db_buffer_map->insert(std::make_pair(record_index, *iter));
			}
		}
		else {
			buffer_vec.push_back(iter->second);
		}
	}
	return 0;
}

int Data_Manager::save_db_data(int save_type, int db_id, DB_Struct *db_struct, Bit_Buffer &buffer) {
	DB_Operator *db_operator = get_db_operator(db_id);
	if(db_operator == nullptr){
		return -1;
	}

	int64_t key_index = get_key_index(db_struct, buffer);
	if(key_index < 0) {
		return -1;
	}

	int ret = 0;
	Index_Buffer_Map *db_buffer_map = get_db_buffer_map(db_id, db_struct->table_name());
	Index_Buffer_Map::iterator iter = db_buffer_map->find(key_index);
	if(iter == db_buffer_map->end()) {
		switch(save_type) {
		case SAVE_CACHE: {
			Bit_Buffer *buf = pop_buffer();
			buf->set_ary(buffer.data(), buffer.get_byte_size());
			db_buffer_map->insert(std::make_pair(key_index, buf));
			break;
		}
		case SAVE_DB_AND_CACHE: {
			Bit_Buffer *buf = pop_buffer();
			buf->set_ary(buffer.data(), buffer.get_byte_size());
			db_buffer_map->insert(std::make_pair(key_index, buf));
			ret = db_operator->save_data(db_id, db_struct, buffer);
			break;
		}
		case SAVE_DB_CLEAR_CACHE:
			ret = db_operator->save_data(db_id, db_struct, buffer);
			break;
		}
	}
	else {
		switch(save_type) {
		case SAVE_CACHE:
			iter->second->set_ary(buffer.data(), buffer.get_byte_size());
			break;
		case SAVE_DB_AND_CACHE:
			iter->second->set_ary(buffer.data(), buffer.get_byte_size());
			ret = db_operator->save_data(db_id, db_struct, buffer);
			break;
		case SAVE_DB_CLEAR_CACHE:
			push_buffer(iter->second);
			db_buffer_map->erase(iter);
			ret = db_operator->save_data(db_id, db_struct, buffer);
			break;
		}
	}
	return ret;
}

int Data_Manager::delete_db_data(int db_id, DB_Struct *db_struct, Bit_Buffer &buffer) {
	DB_Operator *db_operator = get_db_operator(db_id);
	if(db_operator == nullptr){
		return -1;
	}

	Index_Buffer_Map *db_buffer_map = get_db_buffer_map(db_id, db_struct->table_name());
	uint length = buffer.read_uint(16);
	for(uint i = 0; i < length; ++i) {
		int64_t key_index = buffer.read_int64();
		int ret = db_operator->delete_data(db_id, db_struct, key_index);
		if(ret < 0) {
			return ret;
		}
		db_buffer_map->erase(key_index);
	}
	return 0;
}

Data_Manager::Index_Buffer_Map *Data_Manager::get_db_buffer_map(int db_id, std::string table_name) {
	Table_Buffer_Map*table_buffer_map = nullptr;
	DB_Data_Map::iterator iter = db_data_map_.find(db_id);
	if(iter == db_data_map_.end()){
		table_buffer_map = new Table_Buffer_Map(get_hash_table_size(512));
		db_data_map_.insert(std::make_pair(db_id, table_buffer_map));
	}
	else {
		table_buffer_map = iter->second;
	}

	Index_Buffer_Map *db_buffer_map = nullptr;
	Table_Buffer_Map::iterator it = table_buffer_map->find(table_name);
	if(it == table_buffer_map->end()){
		db_buffer_map = new Index_Buffer_Map(get_hash_table_size(10000));
		table_buffer_map->insert(std::make_pair(table_name, db_buffer_map));
	}
	else {
		db_buffer_map = it->second;
	}

	return db_buffer_map;
}

Data_Manager::Index_Buffer_Map *Data_Manager::get_runtime_buffer_map(std::string key_name) {
	Index_Buffer_Map *runtime_buffer_map = nullptr;
	Runtime_Data_Map::iterator iter = runtime_data_map_.find(key_name);
	if(iter == runtime_data_map_.end()){
		runtime_buffer_map = new Index_Buffer_Map(get_hash_table_size(512));
		runtime_data_map_.insert(std::make_pair(key_name, runtime_buffer_map));
	}
	else {
		runtime_buffer_map = iter->second;
	}

	return runtime_buffer_map;
}

void Data_Manager::print_cache_data(void) {
	//打印数据库数据缓存
	LOG_INFO("-------------print_db_cache------------------");
	for(DB_Data_Map::iterator iter = db_data_map_.begin(); iter != db_data_map_.end(); ++iter) {
		LOG_INFO("db_id:%d, data_size:%d", iter->first, iter->second->size());
		for(Table_Buffer_Map::iterator iter_table = iter->second->begin(); iter_table != iter->second->end(); ++iter_table) {
			LOG_INFO("table_name:%s, data_size:%d", iter_table->first.c_str(), iter_table->second->size());
		}
	}

	//打印运行时数据缓存
	LOG_INFO("-------------print_runtime_cache---------------");
	for(Runtime_Data_Map::iterator iter = runtime_data_map_.begin(); iter != runtime_data_map_.end(); ++iter) {
		LOG_INFO("key_name:%s, data_size:%d", iter->first.c_str(), iter->second->size());
	}
}
