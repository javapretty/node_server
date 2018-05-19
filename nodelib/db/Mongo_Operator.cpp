/*
 * Mongo_Operator.cpp
 *
 *  Created on: Dec 29, 2015
 *      Author: zhangyalei
 */

#ifdef MONGO_DB_IMPLEMENT

#include "Base_Function.h"
#include "Data_Manager.h"
#include "Struct_Manager.h"
#include "Mongo_Operator.h"

Mongo_Operator::Mongo_Operator(void) { }

Mongo_Operator::~Mongo_Operator(void) { }

DBClientConnection &Mongo_Operator::get_connection(int db_id) {
	Connection_Map::iterator iter = connection_map_.find(db_id);
	if(iter == connection_map_.end()){
		LOG_FATAL("DataBase %d dosen't connect", db_id);
	}
	return *(iter->second);
}

bool Mongo_Operator::connect_to_db(int db_id, std::string &ip, int port, std::string &user, std::string &password, std::string &pool_name) {
	DBClientConnection *conn = connection_pool_.pop();
	std::string err_msg;
	std::stringstream host_stream;
	host_stream << ip;
	host_stream << ":" << port;
	bool ret = conn->connect(host_stream.str().c_str(), err_msg);
	if(!ret)
		return false;
	connection_map_[db_id] = conn;

	//创建索引
	for(Struct_Manager::Struct_Name_Map::const_iterator iter = STRUCT_MANAGER->db_struct_name_map().begin();
		iter != STRUCT_MANAGER->db_struct_name_map().end(); iter++) {
		DB_Struct *db_struct = dynamic_cast<DB_Struct *>(iter->second);
		if(db_struct && db_struct->table_name() != "") {
			conn->createIndex(db_struct->table_name(), BSON(db_struct->index_name() << 1));
		}
	}
	return true;
}

int Mongo_Operator::select_db_data(int db_id, DB_Struct *db_struct, std::string &condition_name, std::string &condition_value, std::string &query_name, std::string &query_type, Bit_Buffer &buffer) {
	BSONObj bsonobj = get_connection(db_id).findOne(db_struct->table_name(), MONGO_QUERY(condition_name << condition_value));
	if(bsonobj.isEmpty()) {
		return -2;
	}

	if(query_name == "") {
		int ret = load_data_single(db_struct, bsonobj, buffer);
		if(ret < 0) {
			return ret;
		}
	}
	else {
		if(query_type == "int64") {
			int64_t query_value = bsonobj[query_name].numberLong();
			buffer.write_int64(query_value);
		}
		else if(query_type == "string") {
			std::string query_value = bsonobj[query_name].valuestrsafe();
			buffer.write_str(query_value.c_str());
		}
		else {
			LOG_ERROR("select_db_data data not exist, table_name:%s, query_name:%s, query_type:%s", db_struct->table_name().c_str(), query_name.c_str(), query_type.c_str());
			return -1;
		}
	}
	return 0;
}

/////////////////////////////////数据转成buffer///////////////////////////
int Mongo_Operator::load_data(int db_id, DB_Struct *db_struct, int64_t key_index, std::vector<Bit_Buffer *> &buffer_vec) {
	if(key_index == 0) {
		//加载整张表数据
		std::vector<BSONObj> record;
		int len = get_connection(db_id).count(db_struct->table_name());
		if(len > 0) {
			get_connection(db_id).findN(record, db_struct->table_name(), Query(), len);
		}
		for (int i = 0; i < len; ++i) {
			Bit_Buffer *buffer = DATA_MANAGER->pop_buffer();
			int ret = load_data_single(db_struct, record[i], *buffer);
			if(ret < 0) {
				DATA_MANAGER->push_buffer(buffer);
				return ret;
			}
			buffer_vec.push_back(buffer);
		}
	} else {
		//加载单条数据
		BSONObj bsonobj = get_connection(db_id).findOne(db_struct->table_name(), MONGO_QUERY(db_struct->index_name() << (long long int)key_index));
		if(bsonobj.isEmpty()) {
			return -2;
		}

		Bit_Buffer *buffer = DATA_MANAGER->pop_buffer();
		int ret = load_data_single(db_struct, bsonobj, *buffer);
		if(ret < 0) {
			DATA_MANAGER->push_buffer(buffer);
			return ret;
		}
		buffer_vec.push_back(buffer);
	}
	return 0;
}

int Mongo_Operator::save_data(int db_id, DB_Struct *db_struct, Bit_Buffer &buffer) {
	Query query;
	if(db_struct->index_type() == "int64") {
		int64_t key_index = buffer.peek_int64();
		if(key_index < 0) {
			LOG_ERROR("save_data error, table_name:%s, key_index:%ld", db_struct->table_name().c_str(), key_index);
			return -1;
		}
		query = MONGO_QUERY(db_struct->index_name() << (long long int)key_index);
	}
	else if(db_struct->index_type() == "string") {
		std::string key_index = "";
		buffer.peek_str(key_index);
		if(key_index == "") {
			LOG_ERROR("save_data error, table_name:%s, key_index:%s", db_struct->table_name().c_str(), key_index.c_str());
			return -1;
		}
		query = MONGO_QUERY(db_struct->index_name() << key_index);
	}
	else {
		return -1;
	}

	BSONObjBuilder set_builder;
	for(Field_Vec::const_iterator iter = db_struct->field_vec().begin();
			iter != db_struct->field_vec().end(); iter++) {
		int ret = 0;
		if(iter->field_label == "arg") {
			ret = save_data_arg(db_struct, *iter, set_builder, buffer);
		}
		else if(iter->field_label == "vector") {
			ret = save_data_vector(db_struct, *iter, set_builder, buffer);
		}
		else if(iter->field_label == "map") {
			ret = save_data_map(db_struct, *iter, set_builder, buffer);
		}
		else if(iter->field_label == "struct") {
			ret = save_data_struct(db_struct, *iter, set_builder, buffer);
		}

		if(ret < 0) {
			return ret;
		}
	}
	get_connection(db_id).update(db_struct->table_name(), query, BSON("$set" << set_builder.obj() ), true);
	return 0;
}

int Mongo_Operator::delete_data(int db_id, DB_Struct *db_struct, int64_t key_index) {
	get_connection(db_id).remove(db_struct->table_name(), MONGO_QUERY(db_struct->index_name() << (long long int)(key_index)));
	return 0;
}

int Mongo_Operator::load_data_single(DB_Struct *db_struct, BSONObj &bsonobj, Bit_Buffer &buffer) {
	for(Field_Vec::const_iterator iter = db_struct->field_vec().begin();
			iter != db_struct->field_vec().end(); ++iter) {
		int ret = 0;
		if(iter->field_label == "arg") {
			ret = load_data_arg(db_struct, *iter, bsonobj, buffer);
		}
		else if(iter->field_label == "vector") {
			ret = load_data_vector(db_struct, *iter, bsonobj, buffer);
		}
		else if(iter->field_label == "map") {
			ret = load_data_map(db_struct, *iter, bsonobj, buffer);
		}
		else if(iter->field_label == "struct") {
			ret = load_data_struct(db_struct, *iter, bsonobj, buffer);
		}

		if(ret < 0) {
			return ret;
		}
	}
	return 0;
}

int Mongo_Operator::load_data_arg(DB_Struct *db_struct, const Field_Info &field_info, BSONObj &bsonobj, Bit_Buffer &buffer) {
	if(field_info.field_type == "int") {
		int32_t val = bsonobj[field_info.field_name].numberInt();
		buffer.write_int(val, field_info.field_bit);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "uint") {
		uint32_t val = bsonobj[field_info.field_name].numberInt();
		buffer.write_uint(val, field_info.field_bit);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "int64") {
		int64_t val = bsonobj[field_info.field_name].numberLong();
		buffer.write_int64(val);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%ld",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "uint64") {
		uint64_t val = bsonobj[field_info.field_name].numberLong();
		buffer.write_uint64(val);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%ld",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "float") {
		double val = bsonobj[field_info.field_name].numberDouble();
		buffer.write_float(val);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%f",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "bool") {
		bool val = bsonobj[field_info.field_name].booleanSafe();
		buffer.write_bool(val);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "string") {
		std::string val = bsonobj[field_info.field_name].valuestrsafe();
		buffer.write_str(val.c_str());
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%s",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val.c_str());
		}
	}
	else {
		LOG_ERROR("field_type not exist or field_value undefined,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str());
		return -1;
	}
	return 0;
}

int Mongo_Operator::load_data_vector(DB_Struct *db_struct, const Field_Info &field_info, BSONObj &bsonobj, Bit_Buffer &buffer) {
	BSONObj fieldobj = bsonobj.getObjectField(field_info.field_name);
	uint32_t length = fieldobj.nFields();
	buffer.write_uint(length, field_info.field_vbit);
	if(STRUCT_MANAGER->log_trace()) {
		LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_length:%d",
			db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),length);
	}
	BSONObjIterator field_iter(fieldobj);
	BSONObj obj;
	for(uint32_t i = 0; i < length; ++i) {
		obj = field_iter.next().embeddedObject();
		int ret = 0;
		if(db_struct->is_struct(field_info.field_type)) {
			ret = load_data_struct(db_struct, field_info, obj, buffer);
		}
		else {
			ret = load_data_arg(db_struct, field_info, obj, buffer);
		}

		if(ret < 0) {
			return ret;
		}
	}
	return 0;
}

int Mongo_Operator::load_data_map(DB_Struct *db_struct, const Field_Info &field_info, BSONObj &bsonobj, Bit_Buffer &buffer) {
	Field_Info key_info;
	key_info.field_type = field_info.key_type;
	key_info.field_bit = field_info.key_bit;
	key_info.field_name = field_info.key_name;

	BSONObj fieldobj = bsonobj.getObjectField(field_info.field_name);
	uint32_t length = fieldobj.nFields();
	buffer.write_uint(length, field_info.field_vbit);
	if(STRUCT_MANAGER->log_trace()) {
		LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_length:%d",
			db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),length);
	}
	BSONObjIterator field_iter(fieldobj);
	BSONObj obj;
	for(uint32_t i = 0; i < length; ++i) {
		obj = field_iter.next().embeddedObject();
		int ret1 = 0;
		int ret2 = 0;
		//判断map中key的类型，key可以为基本类型或者结构体类型
		if(db_struct->is_struct(key_info.field_type)) {
			key_info.field_label = "struct";
			ret1 = load_data_struct(db_struct, key_info, obj, buffer);
		}
		else {
			key_info.field_label = "arg";
			ret1 = load_data_arg(db_struct, key_info, obj, buffer);
		}

		//判断map中value的类型，value可以为基本类型或者结构体类型
		if(db_struct->is_struct(field_info.field_type)) {
			ret2 = load_data_struct(db_struct, field_info, obj, buffer);
		}
		else {
			ret2 = load_data_arg(db_struct, field_info, obj, buffer);
		}

		if(ret1 < 0 || ret2 < 0) {
			return -1;
		}
	}
	return 0;
}

int Mongo_Operator::load_data_struct(DB_Struct *db_struct, const Field_Info &field_info, BSONObj &bsonobj, Bit_Buffer &buffer) {
	DB_Struct *sub_struct = STRUCT_MANAGER->get_db_struct(field_info.field_type);
	if(db_struct == nullptr){
		return -1;
	}

	BSONObj fieldobj = bsonobj.getObjectField(field_info.field_type);
	Field_Vec field_vec = sub_struct->field_vec();
	for(Field_Vec::const_iterator iter = field_vec.begin();
			iter != field_vec.end(); iter++) {
		int ret = 0;
		if(iter->field_label == "arg") {
			ret = load_data_arg(db_struct, *iter, fieldobj, buffer);
		}
		else if(iter->field_label == "vector") {
			ret = load_data_vector(db_struct, *iter, fieldobj, buffer);
		}
		else if(iter->field_label == "map") {
			ret = load_data_map(db_struct, *iter, fieldobj, buffer);
		}
		else if(iter->field_label == "struct") {
			ret = load_data_struct(db_struct, *iter, fieldobj, buffer);
		}

		if(ret < 0) {
			return ret;
		}
	}
	return 0;
}

int Mongo_Operator::save_data_arg(DB_Struct *db_struct, const Field_Info &field_info, BSONObjBuilder &builder, Bit_Buffer &buffer) {
	if(field_info.field_type == "int") {
		int32_t val = buffer.read_int(field_info.field_bit);
		builder << field_info.field_name << val;
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "uint") {
		uint32_t val = buffer.read_uint(field_info.field_bit);
		builder << field_info.field_name << val;
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "int64") {
		int64_t val = buffer.read_int64();
		builder << field_info.field_name << (long long int)val;
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%ld",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "uint64") {
		uint64_t val = buffer.read_uint64();
		builder << field_info.field_name << (long long int)val;
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%ld",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "float") {
		double val = buffer.read_float();
		builder << field_info.field_name << val;
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%f",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "bool") {
		bool val = buffer.read_bool();
		builder << field_info.field_name << val;
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "string") {
		std::string val = "";
		buffer.read_str(val);
		builder << field_info.field_name << val;
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%s",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val.c_str());
		}
	}
	else {
		LOG_ERROR("field_type not exist or field_value undefined,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str());
		return -1;
	}
	return 0;
}

int Mongo_Operator::save_data_vector(DB_Struct *db_struct, const Field_Info &field_info, BSONObjBuilder &builder, Bit_Buffer &buffer) {
	std::vector<BSONObj> bson_vec;
	uint32_t len = buffer.read_uint(field_info.field_vbit);
	if(STRUCT_MANAGER->log_trace()) {
		LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_length:%d",
			db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),length);
	}
	for (uint32_t i = 0; i < len; ++i) {
		BSONObjBuilder obj_builder;
		int ret = 0;
		if(db_struct->is_struct(field_info.field_type)) {
			ret = save_data_struct(db_struct, field_info, obj_builder, buffer);
		}
		else {
			ret = save_data_arg(db_struct, field_info, obj_builder, buffer);
		}

		if(ret < 0) {
			return ret;
		}
		bson_vec.push_back(obj_builder.obj());
	}

	builder << field_info.field_name << bson_vec;
	return 0;
}

int Mongo_Operator::save_data_map(DB_Struct *db_struct, const Field_Info &field_info, BSONObjBuilder &builder, Bit_Buffer &buffer) {
	Field_Info key_info;
	key_info.field_type = field_info.key_type;
	key_info.field_bit = field_info.key_bit;
	key_info.field_name = field_info.key_name;

	std::vector<BSONObj> bson_vec;
	uint32_t len = buffer.read_uint(field_info.field_vbit);
	if(STRUCT_MANAGER->log_trace()) {
		LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_length:%d",
			db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),length);
	}
	for (uint32_t i = 0; i < len; ++i) {
		BSONObjBuilder obj_builder;
		int ret1 = 0;
		int ret2 = 0;
		//判断map中key的类型，key可以为基本类型或者结构体类型
		if(db_struct->is_struct(key_info.field_type)) {
			ret1 = save_data_struct(db_struct, key_info, obj_builder, buffer);
		}
		else {
			ret1 = save_data_arg(db_struct, key_info, obj_builder, buffer);
		}

		//判断map中value的类型，value可以为基本类型或者结构体类型
		if(db_struct->is_struct(field_info.field_type)) {
			ret2 = save_data_struct(db_struct, field_info, obj_builder, buffer);
		}
		else {
			ret2 = save_data_arg(db_struct, field_info, obj_builder, buffer);
		}

		if(ret1 < 0 || ret2 < 0) {
			return -1;
		}
		bson_vec.push_back(obj_builder.obj());
	}

	builder << field_info.field_name << bson_vec;
	return 0;
}

int Mongo_Operator::save_data_struct(DB_Struct *db_struct, const Field_Info &field_info, BSONObjBuilder &builder, Bit_Buffer &buffer) {
	DB_Struct *sub_struct = STRUCT_MANAGER->get_db_struct(field_info.field_type);
	if (sub_struct == nullptr) {
		return -1;
	}

	BSONObjBuilder obj_builder;
	Field_Vec field_vec = sub_struct->field_vec();
	for(Field_Vec::const_iterator iter = field_vec.begin();
			iter != field_vec.end(); iter++) {
		int ret = 0;
		if(iter->field_label == "arg") {
			ret = save_data_arg(db_struct, *iter, obj_builder, buffer);
		}
		else if(iter->field_label == "vector") {
			ret = save_data_map(db_struct, *iter, obj_builder, buffer);
		}
		else if(iter->field_label == "map") {
			ret = save_data_map(db_struct, *iter, obj_builder, buffer);
		}
		else if(iter->field_label == "struct") {
			ret = save_data_struct(db_struct, *iter, obj_builder, buffer);
		}

		if(ret < 0) {
			return ret;
		}
	}

	builder << field_info.field_name << obj_builder.obj();
	return 0;
}
#endif
