/*
 * Mysql_Operator.cpp
 *
 *  Created on: Jul 20, 2016
 *      Author: zhangyalei
 */

#include "Base_Function.h"
#include "Base_V8.h"
#include "Data_Manager.h"
#include "Struct_Manager.h"
#include "Mysql_Operator.h"

Mysql_Operator::Mysql_Operator(void) { }

Mysql_Operator::~Mysql_Operator(void) { }

Mysql_Conn *Mysql_Operator::get_connection(int db_id) {
	Connection_Map::iterator iter = connection_map_.find(db_id);
	if(iter == connection_map_.end()){
		LOG_FATAL("DataBase %d dosen't connect", db_id);
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
	std::string dbpoolname_ = dbpool_stream.str();
	int ret = MYSQL_MANAGER->init(ip, port, user, password, dbpoolname_, 16);
	if(ret != 0)
		return false;
	conn = MYSQL_MANAGER->get_mysql_conn(dbpoolname_);
	connection_map_[db_id] = conn;
	return true;
}

int Mysql_Operator::init_db(int db_id, DB_Struct *db_struct) {
	return 0;
}

int64_t Mysql_Operator::generate_table_index(int db_id, DB_Struct *db_struct, std::string &type) {
	Mysql_Conn* conn = get_connection(db_id);
	int idx = 1;
	char str_sql[256] = {0};
	sprintf(str_sql, "select * from %s where type='%s'",db_struct->table_name().c_str(), type.c_str());
	sql::ResultSet *result = conn->execute_query(str_sql);
	if (result && result->next()) {
		idx = result->getInt("value") + 1;
	}

	sprintf(str_sql, "insert into %s (type,value) values ('%s',%d) ON DUPLICATE KEY UPDATE type='%s',value=%d", db_struct->table_name().c_str(), type.c_str(), 1, type.c_str(), idx);
	conn->execute_update(str_sql);
	int64_t id = make_id(STRUCT_MANAGER->agent_num(), STRUCT_MANAGER->server_num(), idx);
	return id;
}

int64_t Mysql_Operator::select_table_index(int db_id, DB_Struct *db_struct, std::string &query_name, std::string &query_value) {
	int64_t key_index = -1;
	char str_sql[256] = {0};
	sprintf(str_sql, "select %s from %s where %s='%s'", db_struct->index_name().c_str(), db_struct->table_name().c_str(), query_name.c_str(), query_value.c_str());
	sql::ResultSet *result = get_connection(db_id)->execute_query(str_sql);
	if (result && result->next()) {
		key_index = result->getInt64(db_struct->index_name());
	}
	return key_index;
}

/////////////////////////////////数据转成buffer///////////////////////////
int Mysql_Operator::load_data(int db_id, DB_Struct *db_struct, int64_t key_index, std::vector<Bit_Buffer*> &buffer_vec) {
	Mysql_Conn* conn = get_connection(db_id);
	char str_sql[128] = {0};
	int len = 0;
	if(key_index == 0) {
		//加载整张表数据
		sprintf(str_sql, "select * from %s", db_struct->table_name().c_str());
		sql::ResultSet *result = conn->execute_query(str_sql);
		if (result) {
			len = result->rowsCount();
		}
		while(result->next()) {
			Bit_Buffer *buffer = DATA_MANAGER->pop_buffer();
			load_data_single(db_struct, result, *buffer);
			buffer_vec.push_back(buffer);
		}
	} else {
		//加载单条数据
		sprintf(str_sql, "select * from %s where %s=%ld", db_struct->table_name().c_str(), db_struct->index_name().c_str(), key_index);
		sql::ResultSet *result = conn->execute_query(str_sql);
		if (result && result->next()) {
			Bit_Buffer *buffer = DATA_MANAGER->pop_buffer();
			load_data_single(db_struct, result, *buffer);
			buffer_vec.push_back(buffer);
			len = 1;
		}
	}
	return len;
}

void Mysql_Operator::save_data(int db_id, DB_Struct *db_struct, Bit_Buffer *buffer) {
	int64_t key_index = buffer->peek_int64();
	LOG_INFO("table %s save key_index:%ld", db_struct->table_name().c_str(), key_index);
	if (key_index <= 0) {
		return;
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
	sql::PreparedStatement* pstmt = get_connection(db_id)->create_pstmt(save_sql);

	int param_index = 0;
	int param_len = db_struct->field_vec().size();
	for(Field_Vec::const_iterator iter = db_struct->field_vec().begin();
			iter != db_struct->field_vec().end(); iter++) {
		//参数索引从1开始，所以先将参数索引++
		param_index++;
		if(iter->field_label == "arg") {
			if(iter->field_type == "int") {
				int val = buffer->read_int(iter->field_bit);
				pstmt->setInt(param_index, val);
				pstmt->setInt(param_index + param_len, val);
			}
			else if(iter->field_type == "uint") {
				uint val = buffer->read_uint(iter->field_bit);
				pstmt->setUInt(param_index, val);
				pstmt->setUInt(param_index + param_len, val);
			}
			else if(iter->field_type == "int64") {
				int64_t val = buffer->read_int64();
				pstmt->setInt64(param_index, val);
				pstmt->setInt64(param_index + param_len, val);
			}
			else if(iter->field_type == "uint64") {
				uint64_t val = buffer->read_uint64();
				pstmt->setUInt64(param_index, val);
				pstmt->setUInt64(param_index + param_len, val);
			}
			else if(iter->field_type == "decimal") {
				double val = buffer->read_decimal(iter->field_bit);
				pstmt->setDouble(param_index, val);
				pstmt->setDouble(param_index + param_len, val);
			}
			else if(iter->field_type == "udecimal") {
				double val = buffer->read_udecimal(iter->field_bit);
				pstmt->setDouble(param_index, val);
				pstmt->setDouble(param_index + param_len, val);
			}
			else if(iter->field_type == "bool") {
				bool val = buffer->read_bool();
				pstmt->setBoolean(param_index, val);
				pstmt->setBoolean(param_index + param_len, val);
			}
			else if(iter->field_type == "string") {
				std::string val = "";
				buffer->read_str(val);
				pstmt->setString(param_index, val);
				pstmt->setString(param_index + param_len, val);
			}
			else {
				LOG_ERROR("Can not find the field_type:%s, field_name:%s, struct_name:%s",
						iter->field_type.c_str(), iter->field_name.c_str(), db_struct->struct_name().c_str());
			}
		}
		else if(iter->field_label == "vector" || iter->field_label == "map") {
			Byte_Buffer byte_buffer;
			int field_len = build_byte_buffer_vector(db_struct, *iter, *buffer, byte_buffer);
			std::string blob_data = base64_encode((unsigned char *)byte_buffer.get_read_ptr(), field_len);
			pstmt->setString(param_index, blob_data);
			pstmt->setString(param_index + param_len, blob_data);
		}
		else if(iter->field_label == "struct") {
			Byte_Buffer byte_buffer;
			int field_len = build_byte_buffer_struct(db_struct, *iter, *buffer, byte_buffer);
			std::string blob_data = base64_encode((unsigned char *)byte_buffer.get_read_ptr(), field_len);
			pstmt->setString(param_index, blob_data);
			pstmt->setString(param_index + param_len, blob_data);
		}
	}
	pstmt->execute();
	delete pstmt;
}

void Mysql_Operator::delete_data(int db_id, DB_Struct *db_struct, Isolate* isolate, v8::Local<v8::Object> object) {
	if (!object->IsArray()) {
		LOG_ERROR("delete_data, object is not array");
		return;
	}

	Mysql_Conn* conn = get_connection(db_id);
	Local<Context> context(isolate->GetCurrentContext());
	v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(object);
	int16_t len = array->Length();
	for (int i = 0; i < len; ++i) {
		v8::Local<v8::Value> value = array->Get(context, i).ToLocalChecked();
		int64_t key_index = value->NumberValue(context).FromJust();
		char sql_str[128] = {0};
		sprintf(sql_str, "delete from %s where %s=%ld", db_struct->table_name().c_str(), db_struct->index_name().c_str(), key_index);
		conn->execute(sql_str);
		DATA_MANAGER->delete_db_data(db_id, db_struct, key_index);
	}
}

void Mysql_Operator::load_data_single(DB_Struct *db_struct, sql::ResultSet *result, Bit_Buffer &buffer) {
	for(Field_Vec::const_iterator iter = db_struct->field_vec().begin();
			iter != db_struct->field_vec().end(); ++iter) {
		if(iter->field_label == "arg") {
			load_data_arg(db_struct, *iter, result, buffer);
		}
		else if(iter->field_label == "vector" || iter->field_label == "map") {
			load_data_vector(db_struct, *iter, result, buffer);
		}
		else if(iter->field_label == "struct") {
			load_data_struct(db_struct, *iter, result, buffer);
		}
	}
}

void Mysql_Operator::load_data_arg(DB_Struct *db_struct, const Field_Info &field_info, sql::ResultSet *result, Bit_Buffer &buffer) {
	if(field_info.field_type == "int") {
		int32_t val = result->getInt(field_info.field_name);
		buffer.write_int(val, field_info.field_bit);
	}
	else if(field_info.field_type == "uint") {
		uint32_t val = result->getUInt(field_info.field_name);
		buffer.write_uint(val, field_info.field_bit);
	}
	else if(field_info.field_type == "int64") {
		int64_t val = result->getInt64(field_info.field_name);
		buffer.write_int64(val);
	}
	else if(field_info.field_type == "uint64") {
		uint64_t val = result->getUInt64(field_info.field_name);
		buffer.write_uint64(val);
	}
	else if(field_info.field_type == "decimal") {
		double val = result->getDouble(field_info.field_name);
		buffer.write_decimal(val, field_info.field_bit);
	}
	else if(field_info.field_type == "udecimal") {
		double val = result->getDouble(field_info.field_name);
		buffer.write_udecimal(val, field_info.field_bit);
	}
	else if(field_info.field_type == "bool") {
		bool val = result->getBoolean(field_info.field_name);
		buffer.write_bool(val);
	}
	else if(field_info.field_type == "string") {
		std::string val = result->getString(field_info.field_name);
		buffer.write_str(val.c_str());
	}
	else {
		LOG_ERROR("Can not find the field_type:%s, field_name:%s, struct_name:%s",
				field_info.field_type.c_str(), field_info.field_name.c_str(), db_struct->struct_name().c_str());
	}
}

void Mysql_Operator::load_data_vector(DB_Struct *db_struct, const Field_Info &field_info, sql::ResultSet *result, Bit_Buffer &buffer) {
	std::string blob_str = result->getString(field_info.field_name);
	std::string decode = base64_decode(blob_str);
	Byte_Buffer byte_buffer;
	byte_buffer.copy(decode);
	build_bit_buffer_vector(db_struct, field_info, buffer, byte_buffer);
}

void Mysql_Operator::load_data_struct(DB_Struct *db_struct, const Field_Info &field_info, sql::ResultSet *result, Bit_Buffer &buffer) {
	std::string blob_str = result->getString(field_info.field_name);
	std::string decode = base64_decode(blob_str);
	Byte_Buffer byte_buffer;
	byte_buffer.copy(decode);
	build_bit_buffer_struct(db_struct, field_info, buffer, byte_buffer);
}

/////////////////////////////根据byte_buffer生成bit_buffer//////////////////////
int Mysql_Operator::build_bit_buffer_arg(DB_Struct *db_struct, const Field_Info &field_info, Bit_Buffer &bit_buffer, Byte_Buffer &byte_buffer) {
	if(field_info.field_type == "int") {
		int32_t val = 0;
		byte_buffer.read_int32(val);
		bit_buffer.write_int(val, field_info.field_bit);
	}
	else if(field_info.field_type == "uint") {
		uint32_t val = 0;
		byte_buffer.read_uint32(val);
		bit_buffer.write_uint(val, field_info.field_bit);
	}
	else if(field_info.field_type == "int64") {
		int64_t val = 0;
		byte_buffer.read_int64(val);
		bit_buffer.write_int64(val);
	}
	else if(field_info.field_type == "uint64") {
		uint64_t val = 0;
		byte_buffer.read_uint64(val);
		bit_buffer.write_uint64(val);
	}
	else if(field_info.field_type == "decimal") {
		double val = 0;
		byte_buffer.read_double(val);
		bit_buffer.write_decimal(val, field_info.field_bit);
	}
	else if(field_info.field_type == "udecimal") {
		double val = 0;
		byte_buffer.read_double(val);
		bit_buffer.write_udecimal(val, field_info.field_bit);
	}
	else if(field_info.field_type == "bool") {
		bool val = 0;
		byte_buffer.read_bool(val);
		bit_buffer.write_bool(val);
	}
	else if(field_info.field_type == "string") {
		std::string val = "";
		byte_buffer.read_string(val);
		bit_buffer.write_str(val.c_str());
	}
	else {
		LOG_ERROR("Can not find the field_type:%s, field_name:%s, struct_name:%s",
				field_info.field_type.c_str(), field_info.field_name.c_str(), db_struct->struct_name().c_str());
	}
	return 0;
}

int Mysql_Operator::build_bit_buffer_vector(DB_Struct *db_struct, const Field_Info &field_info, Bit_Buffer &bit_buffer, Byte_Buffer &byte_buffer) {
	uint16_t length = 0;
	byte_buffer.read_uint16(length);
	bit_buffer.write_uint(length, field_info.field_vbit);
	if(db_struct->is_struct(field_info.field_type)) {
		for(uint i = 0; i < length; ++i) {
			build_bit_buffer_struct(db_struct, field_info, bit_buffer, byte_buffer);
		}
	}
	else{
		for(uint i = 0; i < length; ++i) {
			build_bit_buffer_arg(db_struct, field_info, bit_buffer, byte_buffer);
		}
	}
	return 0;
}

int Mysql_Operator::build_bit_buffer_struct(DB_Struct *db_struct, const Field_Info &field_info, Bit_Buffer &bit_buffer, Byte_Buffer &byte_buffer) {
	DB_Struct *sub_struct = STRUCT_MANAGER->get_db_struct(field_info.field_type);
	if(sub_struct == nullptr) {
		return 0;
	}

	Field_Vec field_vec = sub_struct->field_vec();
	for(Field_Vec::const_iterator iter = field_vec.begin();
			iter != field_vec.end(); iter++) {
		if(iter->field_label == "arg") {
			build_bit_buffer_arg(db_struct, *iter, bit_buffer, byte_buffer);
		}
		else if(iter->field_label == "vector" || iter->field_label == "map") {
			build_bit_buffer_vector(db_struct, *iter, bit_buffer, byte_buffer);
		}
		else if(iter->field_label == "struct") {
			build_bit_buffer_struct(db_struct, *iter, bit_buffer, byte_buffer);
		}
	}
	return 0;
}

////////////////////////////////根据bit_buffer生成byte_buffer//////////////////////
int Mysql_Operator::build_byte_buffer_arg(DB_Struct *db_struct, const Field_Info &field_info, Bit_Buffer &bit_buffer, Byte_Buffer &byte_buffer) {
	int field_len = 0;
	if(field_info.field_type == "int") {
		field_len = sizeof(int32_t);
		int val = bit_buffer.read_int(field_info.field_bit);
		byte_buffer.write_int32(val);
	}
	else if(field_info.field_type == "uint") {
		field_len = sizeof(uint32_t);
		uint val = bit_buffer.read_uint(field_info.field_bit);
		byte_buffer.write_uint32(val);
	}
	else if(field_info.field_type == "int64") {
		field_len = sizeof(int64_t);
		int64_t val = bit_buffer.read_int64();
		byte_buffer.write_int64(val);
	}
	else if(field_info.field_type == "uint64") {
		field_len = sizeof(uint64_t);
		uint64_t val = bit_buffer.read_uint64();
		byte_buffer.write_uint64(val);
	}
	else if(field_info.field_type == "decimal") {
		field_len = sizeof(double);
		double val = bit_buffer.read_decimal(field_info.field_bit);
		byte_buffer.write_double(val);
	}
	else if(field_info.field_type == "udecimal") {
		field_len = sizeof(double);
		double val = bit_buffer.read_udecimal(field_info.field_bit);
		byte_buffer.write_double(val);
	}
	else if(field_info.field_type == "bool") {
		field_len = sizeof(bool);
		bool val = bit_buffer.read_bool();
		byte_buffer.write_bool(val);
	}
	else if(field_info.field_type == "string") {
		std::string val = "";
		bit_buffer.read_str(val);
		byte_buffer.write_string(val);
		field_len = sizeof(uint16_t) + val.length();
	}
	else {
		LOG_ERROR("Can not find the field_type:%s, field_name:%s, struct_name:%s",
				field_info.field_type.c_str(), field_info.field_name.c_str(), db_struct->struct_name().c_str());
	}
	return field_len;
}

int Mysql_Operator::build_byte_buffer_vector(DB_Struct *db_struct, const Field_Info &field_info, Bit_Buffer &bit_buffer, Byte_Buffer &byte_buffer) {
	int field_len = sizeof(uint16_t);
	uint length = bit_buffer.read_uint(field_info.field_vbit);
	byte_buffer.write_uint16(length);
	if(db_struct->is_struct(field_info.field_type)) {
		for(uint i = 0; i < length; ++i) {
			field_len += build_byte_buffer_struct(db_struct, field_info, bit_buffer, byte_buffer);
		}
	}
	else{
		for(uint i = 0; i < length; ++i) {
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
		else if(iter->field_label == "vector" || iter->field_label == "map") {
			field_len += build_byte_buffer_vector(db_struct, *iter, bit_buffer, byte_buffer);
		}
		else if(iter->field_label == "struct") {
			field_len += build_byte_buffer_struct(db_struct, *iter, bit_buffer, byte_buffer);
		}
	}
	return field_len;
}
