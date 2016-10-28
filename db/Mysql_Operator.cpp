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
	int serial = 1;
	char str_sql[256] = {0};
	sprintf(str_sql, "select * from %s where type='%s'",db_struct->table_name().c_str(), type.c_str());
	sql::ResultSet *result = get_connection(db_id)->execute_query(str_sql);
	if (result && result->next()) {
		serial = result->getInt("value") + 1;
	}

	sprintf(str_sql, "insert into %s (type,value) values ('%s',%d) ON DUPLICATE KEY UPDATE type='%s',value=%d", db_struct->table_name().c_str(), type.c_str(), 1, type.c_str(), serial);
	get_connection(db_id)->execute_update(str_sql);
	int64_t id = make_id(STRUCT_MANAGER->agent_num(), STRUCT_MANAGER->server_num(), serial);
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

v8::Local<v8::Object> Mysql_Operator::load_data(int db_id, DB_Struct *db_struct, Isolate* isolate, int64_t key_index) {
	EscapableHandleScope handle_scope(isolate);
	Local<Context> context(isolate->GetCurrentContext());

	char str_sql[128] = {0};
	if(key_index == 0) {
		//加载整张表数据
		sprintf(str_sql, "select * from %s", db_struct->table_name().c_str());
		sql::ResultSet *result = get_connection(db_id)->execute_query(str_sql);
		int i = 0;
		int len = 0;
		if (result) {
			len = result->rowsCount();
		}
		v8::Local<v8::Array> array = Array::New(isolate, len);
		while(result->next()) {
			v8::Local<v8::Object> data_obj = load_data_single(db_struct, isolate, result);
			if (!data_obj.IsEmpty()) {
				array->Set(context, i++, data_obj).FromJust();
			}
		}
		return handle_scope.Escape(array);
	} else {
		//加载单条数据
		sprintf(str_sql, "select * from %s where %s=%ld", db_struct->table_name().c_str(), db_struct->index_name().c_str(), key_index);
		sql::ResultSet *result = get_connection(db_id)->execute_query(str_sql);
		if (result && result->next()) {
			v8::Local<v8::Object> data_obj = load_data_single(db_struct, isolate, result);
			return handle_scope.Escape(data_obj);
		} else {
			return handle_scope.Escape(v8::Local<v8::Object>());
		}
	}
}

void Mysql_Operator::save_data(int db_id, DB_Struct *db_struct, Isolate* isolate, v8::Local<v8::Object> object) {
	Local<Context> context(isolate->GetCurrentContext());
	v8::Local<v8::Value> key_value = object->Get(context, String::NewFromUtf8(isolate, db_struct->index_name().c_str(), NewStringType::kNormal).ToLocalChecked()).ToLocalChecked();
	int64_t key_index = key_value->NumberValue(context).FromJust();
	LOG_INFO("table %s save key_index:%ld", db_struct->table_name().c_str(), key_index);
	if (key_index <= 0) {
		return;
	}

	char insert_sql[2048] = {};
	std::string insert_name;
	std::string insert_value;
	std::stringstream insert_stream;
	for(std::vector<Field_Info>::const_iterator iter = db_struct->field_vec().begin();
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
	for(std::vector<Field_Info>::const_iterator iter = db_struct->field_vec().begin();
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
	for(std::vector<Field_Info>::const_iterator iter = db_struct->field_vec().begin();
			iter != db_struct->field_vec().end(); iter++) {
		//参数索引从1开始，所以先将参数索引++
		param_index++;
		v8::Local<v8::Value> value = object->Get(context, String::NewFromUtf8(isolate, iter->field_name.c_str(), NewStringType::kNormal).ToLocalChecked()).ToLocalChecked();
		if(iter->field_label == "arg") {
			if(iter->field_type == "int8") {
				int8_t val = 0;
				if (value->IsInt32()) {
					val = value->Int32Value(context).FromJust();
				}
				pstmt->setInt(param_index, val);
				pstmt->setInt(param_index + param_len, val);
			}
			else if(iter->field_type == "int16") {
				int16_t val = 0;
				if (value->IsInt32()) {
					val = value->Int32Value(context).FromJust();
				}
				pstmt->setInt(param_index, val);
				pstmt->setInt(param_index + param_len, val);
			}
			else if(iter->field_type == "int32") {
				int32_t val = 0;
				if (value->IsInt32()) {
					val = value->Int32Value(context).FromJust();
				}
				pstmt->setInt(param_index, val);
				pstmt->setInt(param_index + param_len, val);
			}
			else if(iter->field_type == "int64") {
				int64_t val = 0;
				if (value->IsNumber()) {
					val = value->NumberValue(context).FromJust();
				}
				pstmt->setInt64(param_index, val);
				pstmt->setInt64(param_index + param_len, val);
			}
			else if(iter->field_type == "uint8") {
				uint8_t val = 0;
				if (value->IsUint32()) {
					val = value->Uint32Value(context).FromJust();
				}
				pstmt->setUInt(param_index, val);
				pstmt->setUInt(param_index + param_len, val);
			}
			else if(iter->field_type == "uint16") {
				uint16_t val = 0;
				if (value->IsUint32()) {
					val = value->Uint32Value(context).FromJust();
				}
				pstmt->setUInt(param_index, val);
				pstmt->setUInt(param_index + param_len, val);
			}
			else if(iter->field_type == "uint32") {
				uint32_t val = 0;
				if (value->IsInt32()) {
					val = value->Int32Value(context).FromJust();
				}
				pstmt->setInt(param_index, val);
				pstmt->setInt(param_index + param_len, val);
			}
			else if(iter->field_type == "uint64") {
				uint64_t val = 0;
				if (value->IsNumber()) {
					val = value->NumberValue(context).FromJust();
				}
				pstmt->setUInt64(param_index, val);
				pstmt->setUInt64(param_index + param_len, val);
			}
			else if(iter->field_type == "double") {
				double val = 0;
				if (value->IsNumber()) {
					val = value->NumberValue(context).FromJust();
				}
				pstmt->setDouble(param_index, val);
				pstmt->setDouble(param_index + param_len, val);
			}
			else if(iter->field_type == "bool") {
				bool val = 0;
				if (value->IsBoolean()) {
					val = value->BooleanValue(context).FromJust();
				}
				pstmt->setBoolean(param_index, val);
				pstmt->setBoolean(param_index + param_len, val);
			}
			else if(iter->field_type == "string") {
				std::string val = "";
				if (value->IsString()) {
					String::Utf8Value str(value->ToString(context).ToLocalChecked());
					val = to_string(str);
				}
				pstmt->setString(param_index, val);
				pstmt->setString(param_index + param_len, val);
			}
			else {
				LOG_ERROR("Can not find the field_type:%s, struct_name:%s", iter->field_type.c_str(), db_struct->struct_name().c_str());
			}
		}
		else if(iter->field_label == "vector") {
			Block_Buffer buffer;
			db_struct->build_buffer_vector(isolate, *iter, buffer, value);
			char *ptr = buffer.get_read_ptr();
			int field_len = build_len_vector(db_struct, *iter, buffer);
			std::string blob_data = base64_encode((unsigned char *)ptr, field_len);
			pstmt->setString(param_index, blob_data);
			pstmt->setString(param_index + param_len, blob_data);

			//LOG_INFO("struct_name:%s, fileld_label:%s, field_type:%s, field_name:%s, param_index:%d, field_len:%d, read_idx:%d", struct_name().c_str(),
			//		iter->field_label.c_str(), iter->field_type.c_str(), iter->field_name.c_str(), param_index, field_len, buffer.get_read_idx());
		}
		else if(iter->field_label == "map") {
			Block_Buffer buffer;
			db_struct->build_buffer_map(isolate, *iter, buffer, value);
			char *ptr = buffer.get_read_ptr();
			int field_len = build_len_vector(db_struct, *iter, buffer);
			std::string blob_data = base64_encode((unsigned char *)ptr, field_len);
			pstmt->setString(param_index, blob_data);
			pstmt->setString(param_index + param_len, blob_data);

			//LOG_INFO("struct_name:%s, fileld_label:%s, field_type:%s, field_name:%s, param_index:%d, field_len:%d, read_idx:%d", struct_name().c_str(),
			//		iter->field_label.c_str(), iter->field_type.c_str(), iter->field_name.c_str(), param_index, field_len, buffer.get_read_idx());
		}
		else if(iter->field_label == "struct") {
			Block_Buffer buffer;
			db_struct->build_buffer_struct(isolate, *iter, buffer, value);
			char *ptr = buffer.get_read_ptr();
			int field_len = build_len_struct(db_struct, *iter, buffer);
			std::string blob_data = base64_encode((unsigned char *)ptr, field_len);
			pstmt->setString(param_index, blob_data);
			pstmt->setString(param_index + param_len, blob_data);

			//LOG_INFO("struct_name:%s, fileld_label:%s, field_type:%s, field_name:%s, param_index:%d, field_len:%d, read_idx:%d", struct_name().c_str(),
			//	iter->field_label.c_str(), iter->field_type.c_str(), iter->field_name.c_str(), param_index, field_len, buffer.get_read_idx());
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

	Local<Context> context(isolate->GetCurrentContext());
	v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(object);
	int16_t len = array->Length();
	for (int i = 0; i < len; ++i) {
		v8::Local<v8::Value> value = array->Get(context, i).ToLocalChecked();
		int64_t key_index = value->NumberValue(context).FromJust();
		char sql_str[128] = {0};
		sprintf(sql_str, "delete from %s where %s=%ld", db_struct->table_name().c_str(), db_struct->index_name().c_str(), key_index);
		get_connection(db_id)->execute(sql_str);
	}
}

v8::Local<v8::Object> Mysql_Operator::load_data_single(DB_Struct *db_struct, Isolate* isolate, sql::ResultSet *result) {
	EscapableHandleScope handle_scope(isolate);
	Local<Context> context(isolate->GetCurrentContext());

	Local<ObjectTemplate> localTemplate = ObjectTemplate::New(isolate);
	v8::Local<v8::Object> data_obj = localTemplate->NewInstance(context).ToLocalChecked();
	for(std::vector<Field_Info>::const_iterator iter = db_struct->field_vec().begin();
			iter != db_struct->field_vec().end(); ++iter) {
		v8::Local<v8::String> key = String::NewFromUtf8(isolate, iter->field_name.c_str(), NewStringType::kNormal).ToLocalChecked();
		if(iter->field_label == "arg") {
			v8::Local<v8::Value> value = load_data_arg(db_struct, isolate, *iter, result);
			if (!value.IsEmpty()) {
				data_obj->Set(context, key, value).FromJust();
			}
		}
		else if(iter->field_label == "vector") {
			v8::Local<v8::Array> array = load_data_vector(db_struct, isolate, *iter, result);
			if (!array.IsEmpty()) {
				data_obj->Set(context, key, array).FromJust();
			}
		}
		else if(iter->field_label == "map") {
			v8::Local<v8::Map> map = load_data_map(db_struct, isolate, *iter, result);
			if (!map.IsEmpty()) {
				data_obj->Set(context, key, map).FromJust();
			}
		}
		else if(iter->field_label == "struct") {
			v8::Local<v8::Object> object = load_data_struct(db_struct, isolate, *iter, result);
			if (!object.IsEmpty()) {
				data_obj->Set(context, key, object).FromJust();
			}
		}
	}
	return handle_scope.Escape(data_obj);
}

v8::Local<v8::Value> Mysql_Operator::load_data_arg(DB_Struct *db_struct, Isolate* isolate, const Field_Info &field_info, sql::ResultSet *result) {
	EscapableHandleScope handle_scope(isolate);

	Local<Value> value;
	if(field_info.field_type == "int8") {
		int8_t val = result->getInt(field_info.field_name);
		value = Int32::New(isolate, val);
	}
	else if(field_info.field_type == "int16") {
		int16_t val = result->getInt(field_info.field_name);
		value = Int32::New(isolate, val);
	}
	else if(field_info.field_type == "int32") {
		int32_t val = result->getInt(field_info.field_name);
		value = Int32::New(isolate, val);
	}
	else if(field_info.field_type == "int64") {
		int64_t val = result->getInt64(field_info.field_name);
		value = Number::New(isolate, val);
	}
	else if(field_info.field_type == "uint8") {
		uint8_t val = result->getUInt(field_info.field_name);
		value = Uint32::New(isolate, val);
	}
	else if(field_info.field_type == "uint16") {
		uint16_t val = result->getUInt(field_info.field_name);
		value = Uint32::New(isolate, val);
	}
	else if(field_info.field_type == "uint32") {
		uint32_t val = result->getUInt(field_info.field_name);
		value = Uint32::New(isolate, val);
	}
	else if(field_info.field_type == "uint64") {
		uint64_t val = result->getUInt64(field_info.field_name);
		value = Number::New(isolate, val);
	}
	else if(field_info.field_type == "double") {
		double val = result->getDouble(field_info.field_name);
		value = Number::New(isolate, val);
	}
	else if(field_info.field_type == "bool") {
		bool val = result->getBoolean(field_info.field_name);
		value = Boolean::New(isolate, val);
	}
	else if(field_info.field_type == "string") {
		std::string val = result->getString(field_info.field_name);
		value = String::NewFromUtf8(isolate, val.c_str(), NewStringType::kNormal).ToLocalChecked();
	}
	else {
		LOG_ERROR("Can not find the field_type:%s, struct_name:%s", field_info.field_type.c_str(), db_struct->struct_name().c_str());
		return handle_scope.Escape(Local<Value>());
	}

	return handle_scope.Escape(value);
}

v8::Local<v8::Array> Mysql_Operator::load_data_vector(DB_Struct *db_struct, Isolate* isolate, const Field_Info &field_info, sql::ResultSet *result) {
	EscapableHandleScope handle_scope(isolate);
	Block_Buffer buffer;
	std::string blob_str = result->getString(field_info.field_name);
	std::string decode = base64_decode(blob_str);
	buffer.copy(decode);
	v8::Local<v8::Array> array = db_struct->build_object_vector(isolate, field_info, buffer);

	return handle_scope.Escape(array);
}

v8::Local<v8::Map> Mysql_Operator::load_data_map(DB_Struct *db_struct, Isolate* isolate, const Field_Info &field_info, sql::ResultSet *result) {
	EscapableHandleScope handle_scope(isolate);

	Block_Buffer buffer;
	std::string blob_str = result->getString(field_info.field_name);
	std::string decode = base64_decode(blob_str);
	buffer.copy(decode);
	v8::Local<v8::Map> map = db_struct->build_object_map(isolate, field_info, buffer);

	return handle_scope.Escape(map);
}

v8::Local<v8::Object> Mysql_Operator::load_data_struct(DB_Struct *db_struct, Isolate* isolate, const Field_Info &field_info, sql::ResultSet *result) {
	EscapableHandleScope handle_scope(isolate);

	Block_Buffer buffer;
	std::string blob_str = result->getString(field_info.field_name);
	std::string decode = base64_decode(blob_str);
	buffer.copy(decode);
	v8::Local<v8::Object> object = db_struct->build_object_struct(isolate, field_info, buffer);

	return handle_scope.Escape(object);
}

int Mysql_Operator::build_len_arg(DB_Struct *db_struct, const Field_Info &field_info, Block_Buffer &buffer) {
	int field_len = 0;
	if(field_info.field_type == "int8") {
		field_len = sizeof(int8_t);
	}
	else if(field_info.field_type == "int16") {
		field_len = sizeof(int16_t);
	}
	else if(field_info.field_type == "int32") {
		field_len = sizeof(int32_t);
	}
	else if(field_info.field_type == "int64") {
		field_len = sizeof(int64_t);
	}
	else if(field_info.field_type == "uint8") {
		field_len = sizeof(uint8_t);
	}
	else if(field_info.field_type == "uint16") {
		field_len = sizeof(uint16_t);
	}
	else if(field_info.field_type == "uint32") {
		field_len = sizeof(uint32_t);
	}
	else if(field_info.field_type == "uint64") {
		field_len = sizeof(uint64_t);
	}
	else if(field_info.field_type == "double") {
		field_len = sizeof(double);
	}
	else if(field_info.field_type == "bool") {
		field_len = sizeof(bool);
	}
	else if(field_info.field_type == "string") {
		//std::string value = buffer.read_string();
		//注意：string类型长度要用length计算,还要加上uint16_t类型的长度变量,不用read_string,减少memcpy调用次数
		uint16_t str_len = 0;
		buffer.peek_uint16(str_len);
		field_len = sizeof(uint16_t) + str_len;
	}
	else {
		LOG_ERROR("Can not find the field_type:%s, struct_name:%s", field_info.field_type.c_str(), db_struct->struct_name().c_str());
	}
	//设置buffer读指针，为了下次正确读取数据
	int read_idx = buffer.get_read_idx();
	buffer.set_read_idx(read_idx + field_len);
	return field_len;
}

int Mysql_Operator::build_len_vector(DB_Struct *db_struct, const Field_Info &field_info, Block_Buffer &buffer) {
	int field_len = sizeof(uint16_t);
	uint16_t vec_size = 0;
	buffer.read_uint16(vec_size);
	if(db_struct->is_struct(field_info.field_type)) {
		for(uint16_t i = 0; i < vec_size; ++i) {
			field_len += build_len_struct(db_struct, field_info, buffer);
		}
	}
	else{
		for(uint16_t i = 0; i < vec_size; ++i) {
			field_len += build_len_arg(db_struct, field_info, buffer);
		}
	}
	return field_len;
}

int Mysql_Operator::build_len_struct(DB_Struct *db_struct, const Field_Info &field_info, Block_Buffer &buffer) {
	int field_len = 0;
	DB_Struct *sub_struct = STRUCT_MANAGER->get_db_struct(field_info.field_type);
	if(sub_struct == nullptr) {
		return field_len;
	}

	std::vector<Field_Info> field_vec = sub_struct->field_vec();
	for(std::vector<Field_Info>::const_iterator iter = field_vec.begin();
			iter != field_vec.end(); iter++) {
		if(iter->field_label == "arg") {
			field_len += build_len_arg(db_struct, *iter, buffer);
		}
		else if(iter->field_label == "vector" || iter->field_label == "map") {
			field_len += build_len_vector(db_struct, *iter, buffer);
		}
		else if(iter->field_label == "struct") {
			field_len += build_len_struct(db_struct, *iter, buffer);
		}
	}
	return field_len;
}

int Mysql_Operator::load_data(int db_id, DB_Struct *db_struct, int64_t key_index, std::vector<Block_Buffer*> &buffer_vec) {
	char str_sql[128] = {0};
	int len = 0;
	if(key_index == 0) {
		//加载整张表数据
		sprintf(str_sql, "select * from %s", db_struct->table_name().c_str());
		sql::ResultSet *result = get_connection(db_id)->execute_query(str_sql);
		if (result) {
			len = result->rowsCount();
		}
		while(result->next()) {
			Block_Buffer *buffer = DATA_MANAGER->pop_buffer();
			load_data_single(db_struct, buffer, result);
			buffer_vec.push_back(buffer);
		}
	} else {
		//加载单条数据
		sprintf(str_sql, "select * from %s where %s=%ld", db_struct->table_name().c_str(), db_struct->index_name().c_str(), key_index);
		sql::ResultSet *result = get_connection(db_id)->execute_query(str_sql);
		if (result && result->next()) {
			Block_Buffer *buffer = DATA_MANAGER->pop_buffer();
			load_data_single(db_struct, buffer, result);
			buffer_vec.push_back(buffer);
			len = 1;
		}
	}
	return len;
}

void Mysql_Operator::save_data(int db_id, DB_Struct *db_struct, Block_Buffer *buffer) {
	int64_t key_index = 0;
	buffer->peek_int64(key_index);
	LOG_INFO("table %s save key_index:%ld", db_struct->table_name().c_str(), key_index);
	if (key_index <= 0) {
		return;
	}

	char insert_sql[2048] = {};
	std::string insert_name;
	std::string insert_value;
	std::stringstream insert_stream;
	for(std::vector<Field_Info>::const_iterator iter = db_struct->field_vec().begin();
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
	for(std::vector<Field_Info>::const_iterator iter = db_struct->field_vec().begin();
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
	for(std::vector<Field_Info>::const_iterator iter = db_struct->field_vec().begin();
			iter != db_struct->field_vec().end(); iter++) {
		//参数索引从1开始，所以先将参数索引++
		param_index++;
		if(iter->field_label == "arg") {
			if(iter->field_type == "int8") {
				int8_t val = 0;
				buffer->read_int8(val);
				pstmt->setInt(param_index, val);
				pstmt->setInt(param_index + param_len, val);
			}
			else if(iter->field_type == "int16") {
				int16_t val = 0;
				buffer->read_int16(val);
				pstmt->setInt(param_index, val);
				pstmt->setInt(param_index + param_len, val);
			}
			else if(iter->field_type == "int32") {
				int32_t val = 0;
				buffer->read_int32(val);
				pstmt->setInt(param_index, val);
				pstmt->setInt(param_index + param_len, val);
			}
			else if(iter->field_type == "int64") {
				int64_t val = 0;
				buffer->read_int64(val);
				pstmt->setInt64(param_index, val);
				pstmt->setInt64(param_index + param_len, val);
			}
			else if(iter->field_type == "uint8") {
				uint8_t val = 0;
				buffer->read_uint8(val);
				pstmt->setUInt(param_index, val);
				pstmt->setUInt(param_index + param_len, val);
			}
			else if(iter->field_type == "uint16") {
				uint16_t val = 0;
				buffer->read_uint16(val);
				pstmt->setUInt(param_index, val);
				pstmt->setUInt(param_index + param_len, val);
			}
			else if(iter->field_type == "uint32") {
				uint32_t val = 0;
				buffer->read_uint32(val);
				pstmt->setUInt(param_index, val);
				pstmt->setUInt(param_index + param_len, val);
			}
			else if(iter->field_type == "uint64") {
				uint64_t val = 0;
				buffer->read_uint64(val);
				pstmt->setUInt64(param_index, val);
				pstmt->setUInt64(param_index + param_len, val);
			}
			else if(iter->field_type == "double") {
				double val = 0;
				buffer->read_double(val);
				pstmt->setDouble(param_index, val);
				pstmt->setDouble(param_index + param_len, val);
			}
			else if(iter->field_type == "bool") {
				bool val = 0;
				buffer->read_bool(val);
				pstmt->setBoolean(param_index, val);
				pstmt->setBoolean(param_index + param_len, val);
			}
			else if(iter->field_type == "string") {
				std::string val = "";
				buffer->read_string(val);
				pstmt->setString(param_index, val);
				pstmt->setString(param_index + param_len, val);
			}
			else {
				LOG_ERROR("Can not find the field_type:%s, struct_name:%s", iter->field_type.c_str(), db_struct->struct_name().c_str());
			}
		}
		else if(iter->field_label == "vector" || iter->field_label == "map") {
			char *ptr = buffer->get_read_ptr();
			int field_len = build_len_vector(db_struct, *iter, *buffer);
			std::string blob_data = base64_encode((unsigned char *)ptr, field_len);
			pstmt->setString(param_index, blob_data);
			pstmt->setString(param_index + param_len, blob_data);
		}
		else if(iter->field_label == "struct") {
			char *ptr = buffer->get_read_ptr();
			int field_len = build_len_struct(db_struct, *iter, *buffer);
			std::string blob_data = base64_encode((unsigned char *)ptr, field_len);
			pstmt->setString(param_index, blob_data);
			pstmt->setString(param_index + param_len, blob_data);
		}
	}
	pstmt->execute();
	delete pstmt;
}

void Mysql_Operator::delete_data(int db_id, DB_Struct *db_struct, Block_Buffer *buffer) {
	uint16_t len = 0;
	buffer->read_uint16(len);
	for (uint i = 0; i < len; ++i) {
		int64_t key_index = 0;
		buffer->read_int64(key_index);
		char sql_str[128] = {0};
		sprintf(sql_str, "delete from %s where %s=%ld", db_struct->table_name().c_str(), db_struct->index_name().c_str(), key_index);
		get_connection(db_id)->execute(sql_str);
	}
}

Block_Buffer *Mysql_Operator::load_data_single(DB_Struct *db_struct, Block_Buffer *buffer, sql::ResultSet *result) {
	for(std::vector<Field_Info>::const_iterator iter = db_struct->field_vec().begin();
			iter != db_struct->field_vec().end(); ++iter) {
		if(iter->field_label == "arg") {
			load_data_arg(db_struct, buffer, *iter, result);
		}
		else if(iter->field_label == "vector" || iter->field_label == "map") {
			load_data_vector(db_struct, buffer, *iter, result);
		}
		else if(iter->field_label == "struct") {
			load_data_struct(db_struct, buffer, *iter, result);
		}
	}
	return buffer;
}

Block_Buffer *Mysql_Operator::load_data_arg(DB_Struct *db_struct, Block_Buffer *buffer, const Field_Info &field_info, sql::ResultSet *result) {
	if(field_info.field_type == "int8") {
		int8_t val = result->getInt(field_info.field_name);
		buffer->write_int8(val);
	}
	else if(field_info.field_type == "int16") {
		int16_t val = result->getInt(field_info.field_name);
		buffer->write_int16(val);
	}
	else if(field_info.field_type == "int32") {
		int32_t val = result->getInt(field_info.field_name);
		buffer->write_int32(val);
	}
	else if(field_info.field_type == "int64") {
		int64_t val = result->getInt64(field_info.field_name);
		buffer->write_int64(val);
	}
	else if(field_info.field_type == "uint8") {
		uint8_t val = result->getUInt(field_info.field_name);
		buffer->write_uint8(val);
	}
	else if(field_info.field_type == "uint16") {
		uint16_t val = result->getUInt(field_info.field_name);
		buffer->write_uint16(val);
	}
	else if(field_info.field_type == "uint32") {
		uint32_t val = result->getUInt(field_info.field_name);
		buffer->write_uint32(val);
	}
	else if(field_info.field_type == "uint64") {
		uint64_t val = result->getUInt64(field_info.field_name);
		buffer->write_uint64(val);
	}
	else if(field_info.field_type == "double") {
		double val = result->getDouble(field_info.field_name);
		buffer->write_double(val);
	}
	else if(field_info.field_type == "bool") {
		bool val = result->getBoolean(field_info.field_name);
		buffer->write_bool(val);
	}
	else if(field_info.field_type == "string") {
		std::string val = result->getString(field_info.field_name);
		buffer->write_string(val);
	}
	else {
		LOG_ERROR("Can not find the field_type:%s, struct_name:%s", field_info.field_type.c_str(), db_struct->struct_name().c_str());
		return nullptr;
	}
	return buffer;
}

Block_Buffer *Mysql_Operator::load_data_vector(DB_Struct *db_struct, Block_Buffer *buffer, const Field_Info &field_info, sql::ResultSet *result) {
	std::string blob_str = result->getString(field_info.field_name);
	std::string decode = base64_decode(blob_str);
	buffer->copy(decode);
	return buffer;
}

Block_Buffer *Mysql_Operator::load_data_struct(DB_Struct *db_struct, Block_Buffer *buffer, const Field_Info &field_info, sql::ResultSet *result) {
	std::string blob_str = result->getString(field_info.field_name);
	std::string decode = base64_decode(blob_str);
	buffer->copy(decode);
	return buffer;
}
