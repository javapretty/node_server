/*
 * Mysql_Operator.cpp
 *
 *  Created on: Jul 20, 2016
 *      Author: zhangyalei
 */

#include "Base_Function.h"
#include "Data_Manager.h"
#include "Struct_Manager.h"
#include "Mysql_Operator.h"

Mysql_Operator::Mysql_Operator(void) { }

Mysql_Operator::~Mysql_Operator(void) { }

Mysql_Conn *Mysql_Operator::get_connection(int db_id) {
	Connection_Map::iterator iter = connection_map_.find(db_id);
	if(iter == connection_map_.end()){
		return nullptr;
	}
	return iter->second;
}

bool Mysql_Operator::connect_to_db(int db_id, std::string &ip, int port, std::string &user, std::string &password, std::string &pool_name) {
	Mysql_Conn *conn;
	std::stringstream dbpool_stream;
	dbpool_stream.str("");
	dbpool_stream << pool_name;
	dbpool_stream << "_" << pthread_self();
	std::string dbpoolname = dbpool_stream.str();
	int ret = MYSQL_MANAGER->init(ip, port, user, password, dbpoolname, 16);
	if(ret != 0) {
		return false;
	}
	conn = MYSQL_MANAGER->get_mysql_conn(dbpoolname);
	connection_map_[db_id] = conn;
	return true;
}

int Mysql_Operator::select_db_data(int db_id, DB_Struct *db_struct, std::string &condition_name, std::string &condition_value, std::string &query_name, std::string &query_type, Bit_Buffer &buffer) {
	Mysql_Conn* conn = get_connection(db_id);
	if(!conn) {
		LOG_ERROR("db_id:%d dosen't connect", db_id);
		return -1;
	}

	int ret = 0;
	char str_sql[256] = {0};
	sprintf(str_sql, "select * from %s where %s='%s'", db_struct->table_name().c_str(), condition_name.c_str(), condition_value.c_str());
	sql::ResultSet *result = conn->execute_query(str_sql);
	if (result && result->next()) {
		if(query_name == "") {
			ret = load_data_single(db_struct, result, buffer);
		}
		else {
			if(query_type == "int64") {
				int64_t query_value = result->getInt64(query_name);
				buffer.write_int64(query_value);
			}
			else if(query_type == "string") {
				std::string query_value = result->getString(query_name);
				buffer.write_str(query_value.c_str());
			}
			else {
				ret = -1;
			}
		}
	}
	else {
		ret = -2;
	}

	if(ret == -1) {
		LOG_ERROR("select_db_data ret:%d, table_name:%s, query_name:%s, query_type:%s", ret, db_struct->table_name().c_str(), query_name.c_str(), query_type.c_str());
	}

	//不要忘记删除result指针，否则会内存泄漏
	delete result;
	return ret;
}

int Mysql_Operator::load_data(int db_id, DB_Struct *db_struct, int64_t key_index, std::vector<Bit_Buffer*> &buffer_vec) {
	Mysql_Conn* conn = get_connection(db_id);
	if(!conn) {
		LOG_ERROR("db_id:%d dosen't connect", db_id);
		return -1;
	}

	char str_sql[256] = {0};
	if(key_index == 0) {
		//加载整张表数据
		sprintf(str_sql, "select * from %s", db_struct->table_name().c_str());
	} else {
		//加载单条数据
		sprintf(str_sql, "select * from %s where %s=%ld", db_struct->table_name().c_str(), db_struct->index_name().c_str(), key_index);
	}

	sql::ResultSet *result = conn->execute_query(str_sql);
	if (result) {
		while(result->next()) {
			Bit_Buffer *buffer = DATA_MANAGER->pop_buffer();
			int ret = load_data_single(db_struct, result, *buffer);
			if(ret < 0) {
				delete result;
				DATA_MANAGER->push_buffer(buffer);
				return ret;
			}
			buffer_vec.push_back(buffer);
		}
	}
	//不要忘记删除result指针，否则会内存泄漏
	delete result;
	return 0;
}

int Mysql_Operator::save_data(int db_id, DB_Struct *db_struct, Bit_Buffer &buffer) {
	Mysql_Conn* conn = get_connection(db_id);
	if(!conn) {
		LOG_ERROR("db_id:%d dosen't connect", db_id);
		return -1;
	}

	char insert_sql[2048] = {};
	std::string insert_name;
	std::string insert_value;
	std::stringstream insert_stream;
	for(Field_Vec::const_iterator iter = db_struct->field_vec().begin();
			iter != db_struct->field_vec().end(); iter++) {
		insert_stream.str("");
		insert_stream << iter->field_name << ",";
		insert_name += insert_stream.str();
		insert_stream.str("");
		insert_stream << "?,";
		insert_value += insert_stream.str();
	}
	insert_name = insert_name.substr(0, insert_name.length()-1);
	insert_value = insert_value.substr(0, insert_value.length()-1);
	sprintf(insert_sql, "INSERT INTO %s (%s) VALUES (%s)",
			db_struct->table_name().c_str(), insert_name.c_str(), insert_value.c_str());
	char update_sql[1024] = {};
	std::string update_value;
	std::stringstream update_stream;
	for(Field_Vec::const_iterator iter = db_struct->field_vec().begin();
			iter != db_struct->field_vec().end(); iter++) {
		update_stream.str("");
		update_stream << iter->field_name << "=?,";
		update_value += update_stream.str();
	}
	update_value = update_value.substr(0, update_value.length()-1);
	sprintf(update_sql, "%s ", update_value.c_str());

	char save_sql[4096] = {};
	sprintf(save_sql, "%s ON DUPLICATE KEY UPDATE %s", insert_sql, update_sql);
	sql::PreparedStatement* pstmt = conn->create_pstmt(save_sql);

	int param_index = 0;
	int param_len = db_struct->field_vec().size();
	for(Field_Vec::const_iterator iter = db_struct->field_vec().begin();
			iter != db_struct->field_vec().end(); iter++) {
		//参数索引从1开始，所以先将参数索引++
		param_index++;
		if(iter->field_label == "arg") {
			if(iter->field_type == "int") {
				int32_t val = buffer.read_int(iter->field_bit);
				pstmt->setInt(param_index, val);
				pstmt->setInt(param_index + param_len, val);
				if(STRUCT_MANAGER->log_trace()) {
					LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
						db_struct->struct_name().c_str(),iter->field_label.c_str(),iter->field_name.c_str(),iter->field_type.c_str(),val);
				}
			}
			else if(iter->field_type == "uint") {
				uint32_t val = buffer.read_uint(iter->field_bit);
				pstmt->setUInt(param_index, val);
				pstmt->setUInt(param_index + param_len, val);
				if(STRUCT_MANAGER->log_trace()) {
					LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
						db_struct->struct_name().c_str(),iter->field_label.c_str(),iter->field_name.c_str(),iter->field_type.c_str(),val);
				}
			}
			else if(iter->field_type == "int64") {
				int64_t val = buffer.read_int64();
				pstmt->setInt64(param_index, val);
				pstmt->setInt64(param_index + param_len, val);
				if(STRUCT_MANAGER->log_trace()) {
					LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%ld",
						db_struct->struct_name().c_str(),iter->field_label.c_str(),iter->field_name.c_str(),iter->field_type.c_str(),val);
				}
			}
			else if(iter->field_type == "uint64") {
				uint64_t val = buffer.read_uint64();
				pstmt->setUInt64(param_index, val);
				pstmt->setUInt64(param_index + param_len, val);
				if(STRUCT_MANAGER->log_trace()) {
					LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%ld",
						db_struct->struct_name().c_str(),iter->field_label.c_str(),iter->field_name.c_str(),iter->field_type.c_str(),val);
				}
			}
			else if(iter->field_type == "float") {
				double val = buffer.read_float();
				pstmt->setDouble(param_index, val);
				pstmt->setDouble(param_index + param_len, val);
				if(STRUCT_MANAGER->log_trace()) {
					LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%f",
						db_struct->struct_name().c_str(),iter->field_label.c_str(),iter->field_name.c_str(),iter->field_type.c_str(),val);
				}
			}
			else if(iter->field_type == "bool") {
				bool val = buffer.read_bool();
				pstmt->setBoolean(param_index, val);
				pstmt->setBoolean(param_index + param_len, val);
				if(STRUCT_MANAGER->log_trace()) {
					LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
						db_struct->struct_name().c_str(),iter->field_label.c_str(),iter->field_name.c_str(),iter->field_type.c_str(),val);
				}
			}
			else if(iter->field_type == "string") {
				std::string val = "";
				buffer.read_str(val);
				pstmt->setString(param_index, val);
				pstmt->setString(param_index + param_len, val);
				if(STRUCT_MANAGER->log_trace()) {
					LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%s",
						db_struct->struct_name().c_str(),iter->field_label.c_str(),iter->field_name.c_str(),iter->field_type.c_str(),val.c_str());
				}
			}
			else {
				LOG_ERROR("field_type error,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
						db_struct->struct_name().c_str(),iter->field_label.c_str(),iter->field_name.c_str(),iter->field_type.c_str());
				return -1;
			}
		}
		else if(iter->field_label == "vector") {
			Byte_Buffer byte_buffer;
			int field_len = build_byte_buffer_vector(db_struct, *iter, buffer, byte_buffer);
			std::string blob_data = base64_encode((unsigned char *)byte_buffer.get_read_ptr(), field_len);
			pstmt->setString(param_index, blob_data);
			pstmt->setString(param_index + param_len, blob_data);
		}
		else if(iter->field_label == "map") {
			Byte_Buffer byte_buffer;
			int field_len = build_byte_buffer_map(db_struct, *iter, buffer, byte_buffer);
			std::string blob_data = base64_encode((unsigned char *)byte_buffer.get_read_ptr(), field_len);
			pstmt->setString(param_index, blob_data);
			pstmt->setString(param_index + param_len, blob_data);
		}
		else if(iter->field_label == "struct") {
			Byte_Buffer byte_buffer;
			int field_len = build_byte_buffer_struct(db_struct, *iter, buffer, byte_buffer);
			std::string blob_data = base64_encode((unsigned char *)byte_buffer.get_read_ptr(), field_len);
			pstmt->setString(param_index, blob_data);
			pstmt->setString(param_index + param_len, blob_data);
		}
	}
	pstmt->execute();
	delete pstmt;
	return 0;
}

int Mysql_Operator::delete_data(int db_id, DB_Struct *db_struct, int64_t key_index) {
	Mysql_Conn* conn = get_connection(db_id);
	if(!conn) {
		LOG_ERROR("db_id:%d dosen't connect", db_id);
		return -1;
	}

	char sql_str[128] = {0};
	sprintf(sql_str, "delete from %s where %s=%ld", db_struct->table_name().c_str(), db_struct->index_name().c_str(), key_index);
	return conn->execute(sql_str);
}

int Mysql_Operator::load_data_single(DB_Struct *db_struct, sql::ResultSet *result, Bit_Buffer &buffer) {
	for(Field_Vec::const_iterator iter = db_struct->field_vec().begin();
			iter != db_struct->field_vec().end(); ++iter) {
		int ret = 0;
		if(iter->field_label == "arg") {
			ret = load_data_arg(db_struct, *iter, result, buffer);
		}
		else if(iter->field_label == "vector") {
			ret = load_data_vector(db_struct, *iter, result, buffer);
		}
		else if(iter->field_label == "map") {
			ret = load_data_map(db_struct, *iter, result, buffer);
		}
		else if(iter->field_label == "struct") {
			ret = load_data_struct(db_struct, *iter, result, buffer);
		}

		if(ret < 0) {
			return ret;
		}
	}
	return 0;
}

int Mysql_Operator::load_data_arg(DB_Struct *db_struct, const Field_Info &field_info, sql::ResultSet *result, Bit_Buffer &buffer) {
	if(field_info.field_type == "int") {
		int32_t val = result->getInt(field_info.field_name);
		buffer.write_int(val, field_info.field_bit);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "uint") {
		uint32_t val = result->getUInt(field_info.field_name);
		buffer.write_uint(val, field_info.field_bit);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "int64") {
		int64_t val = result->getInt64(field_info.field_name);
		buffer.write_int64(val);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%ld",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "uint64") {
		uint64_t val = result->getUInt64(field_info.field_name);
		buffer.write_uint64(val);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%ld",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "float") {
		double val = result->getDouble(field_info.field_name);
		buffer.write_float(val);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%f",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "bool") {
		bool val = result->getBoolean(field_info.field_name);
		buffer.write_bool(val);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "string") {
		std::string val = result->getString(field_info.field_name);
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

int Mysql_Operator::load_data_vector(DB_Struct *db_struct, const Field_Info &field_info, sql::ResultSet *result, Bit_Buffer &buffer) {
	std::string blob_str = result->getString(field_info.field_name);
	std::string decode = base64_decode(blob_str);
	Byte_Buffer byte_buffer;
	byte_buffer.copy(decode);
	return build_bit_buffer_vector(db_struct, field_info, buffer, byte_buffer);
}

int Mysql_Operator::load_data_map(DB_Struct *db_struct, const Field_Info &field_info, sql::ResultSet *result, Bit_Buffer &buffer) {
	std::string blob_str = result->getString(field_info.field_name);
	std::string decode = base64_decode(blob_str);
	Byte_Buffer byte_buffer;
	byte_buffer.copy(decode);
	return build_bit_buffer_map(db_struct, field_info, buffer, byte_buffer);
}

int Mysql_Operator::load_data_struct(DB_Struct *db_struct, const Field_Info &field_info, sql::ResultSet *result, Bit_Buffer &buffer) {
	std::string blob_str = result->getString(field_info.field_name);
	std::string decode = base64_decode(blob_str);
	Byte_Buffer byte_buffer;
	byte_buffer.copy(decode);
	return build_bit_buffer_struct(db_struct, field_info, buffer, byte_buffer);
}

/////////////////////////////根据byte_buffer生成bit_buffer//////////////////////
int Mysql_Operator::build_bit_buffer_arg(DB_Struct *db_struct, const Field_Info &field_info, Bit_Buffer &bit_buffer, Byte_Buffer &byte_buffer) {
	if(field_info.field_type == "int") {
		int32_t val = 0;
		byte_buffer.read_int32(val);
		bit_buffer.write_int(val, field_info.field_bit);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "uint") {
		uint32_t val = 0;
		byte_buffer.read_uint32(val);
		bit_buffer.write_uint(val, field_info.field_bit);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "int64") {
		int64_t val = 0;
		byte_buffer.read_int64(val);
		bit_buffer.write_int64(val);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%ld",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "uint64") {
		uint64_t val = 0;
		byte_buffer.read_uint64(val);
		bit_buffer.write_uint64(val);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%ld",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "float") {
		double val = 0;
		byte_buffer.read_double(val);
		bit_buffer.write_float(val);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%f",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "bool") {
		bool val = 0;
		byte_buffer.read_bool(val);
		bit_buffer.write_bool(val);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "string") {
		std::string val = "";
		byte_buffer.read_string(val);
		bit_buffer.write_str(val.c_str());
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

int Mysql_Operator::build_bit_buffer_vector(DB_Struct *db_struct, const Field_Info &field_info, Bit_Buffer &bit_buffer, Byte_Buffer &byte_buffer) {
	uint16_t length = 0;
	byte_buffer.read_uint16(length);
	bit_buffer.write_uint(length, field_info.field_vbit);
	if(STRUCT_MANAGER->log_trace()) {
		LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_length:%d",
			db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),length);
	}
	for(uint i = 0; i < length; ++i) {
		int ret = 0;
		if(db_struct->is_struct(field_info.field_type)) {
			ret = build_bit_buffer_struct(db_struct, field_info, bit_buffer, byte_buffer);
		}
		else {
			ret = build_bit_buffer_arg(db_struct, field_info, bit_buffer, byte_buffer);
		}

		if(ret < 0) {
			return ret;
		}
	}
	return 0;
}

int Mysql_Operator::build_bit_buffer_map(DB_Struct *db_struct, const Field_Info &field_info, Bit_Buffer &bit_buffer, Byte_Buffer &byte_buffer) {
	Field_Info key_info;
	key_info.field_type = field_info.key_type;
	key_info.field_bit = field_info.key_bit;
	key_info.field_name = field_info.key_name;

	uint16_t length = 0;
	byte_buffer.read_uint16(length);
	bit_buffer.write_uint(length, field_info.field_vbit);
	if(STRUCT_MANAGER->log_trace()) {
		LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_length:%d",
			db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),length);
	}
	for(uint i = 0; i < length; ++i) {
		int ret1 = 0;
		int ret2 = 0;
		//判断map中key的类型，key可以为基本类型或者结构体类型
		if(db_struct->is_struct(key_info.field_type)) {
			key_info.field_label = "struct";
			ret1 = build_bit_buffer_struct(db_struct, key_info, bit_buffer, byte_buffer);
		}
		else {
			key_info.field_label = "arg";
			ret1 = build_bit_buffer_arg(db_struct, key_info, bit_buffer, byte_buffer);
		}

		//判断map中value的类型，value可以为基本类型或者结构体类型
		if(db_struct->is_struct(field_info.field_type)) {
			ret1 = build_bit_buffer_struct(db_struct, field_info, bit_buffer, byte_buffer);
		}
		else {
			ret1 = build_bit_buffer_arg(db_struct, field_info, bit_buffer, byte_buffer);
		}

		if(ret1 < 0 || ret2 < 0) {
			return -1;
		}
	}
	return 0;
}

int Mysql_Operator::build_bit_buffer_struct(DB_Struct *db_struct, const Field_Info &field_info, Bit_Buffer &bit_buffer, Byte_Buffer &byte_buffer) {
	DB_Struct *sub_struct = STRUCT_MANAGER->get_db_struct(field_info.field_type);
	if(sub_struct == nullptr) {
		return -1;
	}

	Field_Vec field_vec = sub_struct->field_vec();
	for(Field_Vec::const_iterator iter = field_vec.begin();
			iter != field_vec.end(); iter++) {
		int ret = 0;
		if(iter->field_label == "arg") {
			ret = build_bit_buffer_arg(db_struct, *iter, bit_buffer, byte_buffer);
		}
		else if(iter->field_label == "vector") {
			ret = build_bit_buffer_vector(db_struct, *iter, bit_buffer, byte_buffer);
		}
		else if(iter->field_label == "map") {
			ret = build_bit_buffer_map(db_struct, *iter, bit_buffer, byte_buffer);
		}
		else if(iter->field_label == "struct") {
			ret = build_bit_buffer_struct(db_struct, *iter, bit_buffer, byte_buffer);
		}

		if(ret < 0) {
			return ret;
		}
	}
	return 0;
}

////////////////////////////////根据bit_buffer生成byte_buffer//////////////////////
int Mysql_Operator::build_byte_buffer_arg(DB_Struct *db_struct, const Field_Info &field_info, Bit_Buffer &bit_buffer, Byte_Buffer &byte_buffer) {
	int field_len = 0;
	if(field_info.field_type == "int") {
		field_len = sizeof(int32_t);
		int32_t val = bit_buffer.read_int(field_info.field_bit);
		byte_buffer.write_int32(val);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "uint") {
		field_len = sizeof(uint32_t);
		uint32_t val = bit_buffer.read_uint(field_info.field_bit);
		byte_buffer.write_uint32(val);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "int64") {
		field_len = sizeof(int64_t);
		int64_t val = bit_buffer.read_int64();
		byte_buffer.write_int64(val);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%ld",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "uint64") {
		field_len = sizeof(uint64_t);
		uint64_t val = bit_buffer.read_uint64();
		byte_buffer.write_uint64(val);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%ld",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "float") {
		field_len = sizeof(double);
		double val = bit_buffer.read_float();
		byte_buffer.write_double(val);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%f",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "bool") {
		field_len = sizeof(bool);
		bool val = bit_buffer.read_bool();
		byte_buffer.write_bool(val);
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
		}
	}
	else if(field_info.field_type == "string") {
		std::string val = "";
		bit_buffer.read_str(val);
		byte_buffer.write_string(val);
		field_len = sizeof(uint16_t) + val.length();
		if(STRUCT_MANAGER->log_trace()) {
			LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%s",
				db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val.c_str());
		}
	}
	else {
		LOG_ERROR("field_type not exist or field_value undefined,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
			db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str());
	}
	return field_len;
}

int Mysql_Operator::build_byte_buffer_vector(DB_Struct *db_struct, const Field_Info &field_info, Bit_Buffer &bit_buffer, Byte_Buffer &byte_buffer) {
	int field_len = sizeof(uint16_t);
	uint16_t length = bit_buffer.read_uint(field_info.field_vbit);
	byte_buffer.write_uint16(length);
	if(STRUCT_MANAGER->log_trace()) {
		LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_length:%d",
			db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),length);
	}
	for(uint i = 0; i < length; ++i) {
		if(db_struct->is_struct(field_info.field_type)) {
			field_len += build_byte_buffer_struct(db_struct, field_info, bit_buffer, byte_buffer);
		}
		else{
			field_len += build_byte_buffer_arg(db_struct, field_info, bit_buffer, byte_buffer);
		}
	}
	return field_len;
}

int Mysql_Operator::build_byte_buffer_map(DB_Struct *db_struct, const Field_Info &field_info, Bit_Buffer &bit_buffer, Byte_Buffer &byte_buffer) {
	Field_Info key_info;
	key_info.field_type = field_info.key_type;
	key_info.field_bit = field_info.key_bit;
	key_info.field_name = field_info.key_name;

	int field_len = sizeof(uint16_t);
	uint16_t length = bit_buffer.read_uint(field_info.field_vbit);
	byte_buffer.write_uint16(length);
	if(STRUCT_MANAGER->log_trace()) {
		LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_length:%d",
			db_struct->struct_name().c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),length);
	}
	for(uint i = 0; i < length; ++i) {
		//判断map中key的类型，key可以为基本类型或者结构体类型
		if(db_struct->is_struct(key_info.field_type)) {
			key_info.field_label = "struct";
			field_len += build_byte_buffer_struct(db_struct, key_info, bit_buffer, byte_buffer);
		}
		else {
			key_info.field_label = "arg";
			field_len += build_byte_buffer_arg(db_struct, key_info, bit_buffer, byte_buffer);
		}

		//判断map中value的类型，value可以为基本类型或者结构体类型
		if(db_struct->is_struct(field_info.field_type)) {
			field_len += build_byte_buffer_struct(db_struct, field_info, bit_buffer, byte_buffer);
		}
		else {
			field_len += build_byte_buffer_arg(db_struct, field_info, bit_buffer, byte_buffer);
		}
	}
	return field_len;
}

int Mysql_Operator::build_byte_buffer_struct(DB_Struct *db_struct, const Field_Info &field_info, Bit_Buffer &bit_buffer, Byte_Buffer &byte_buffer) {
	int field_len = 0;
	DB_Struct *sub_struct = STRUCT_MANAGER->get_db_struct(field_info.field_type);
	if(sub_struct == nullptr) {
		return field_len;
	}

	Field_Vec field_vec = sub_struct->field_vec();
	for(Field_Vec::const_iterator iter = field_vec.begin();
			iter != field_vec.end(); iter++) {
		if(iter->field_label == "arg") {
			field_len += build_byte_buffer_arg(db_struct, *iter, bit_buffer, byte_buffer);
		}
		else if(iter->field_label == "vector") {
			field_len += build_byte_buffer_vector(db_struct, *iter, bit_buffer, byte_buffer);
		}
		else if(iter->field_label == "map") {
			field_len += build_byte_buffer_map(db_struct, *iter, bit_buffer, byte_buffer);
		}
		else if(iter->field_label == "struct") {
			field_len += build_byte_buffer_struct(db_struct, *iter, bit_buffer, byte_buffer);
		}
	}
	return field_len;
}
