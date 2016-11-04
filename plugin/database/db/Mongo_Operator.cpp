/*
 * Mongo_Operator.cpp
 *
 *  Created on: Dec 29, 2015
 *      Author: zhangyalei
 */

#ifdef MONGO_DB_IMPLEMENT

#include "Base_Function.h"
#include "Base_V8.h"
#include "Data_Manager.h"
#include "Struct_Manager.h"
#include "Mongo_Operator.h"
#include "Base_Struct.h"

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
	return true;
}

int Mongo_Operator::init_db(int db_id, DB_Struct *db_struct) {
	//创建索引
	get_connection(db_id).createIndex(db_struct->table_name(), BSON(db_struct->index_name() << 1));
	db_struct->set_db_init();
	return 0;
}

int64_t Mongo_Operator::generate_table_index(int db_id, DB_Struct *db_struct, std::string &type) {
	if(!db_struct->db_init()) {
		init_db(db_id, db_struct);
	}

	int idx = 1;
	BSONObj result;
	char str_json[128] = {0};
	sprintf(str_json, "{findandmodify:'%s', query:{type:'%s'}, update:{$inc:{value:1}}, upsert:true}",
			db_struct->table_name().c_str(), type.c_str());
	if (get_connection(db_id).runCommand(db_struct->db_name(), fromjson(str_json), result) == false) {
		LOG_ERROR("findandmodify type='%s' failed", type.c_str());
		return -1;
	}

	idx = result.getFieldDotted("value.value").numberLong() + 1;
	int64_t id = make_id(STRUCT_MANAGER->agent_num(), STRUCT_MANAGER->server_num(), idx);
	return id;
}

int64_t Mongo_Operator::select_table_index(int db_id, DB_Struct *db_struct,  std::string &query_name, std::string &query_value) {
	if(!db_struct->db_init()) {
		init_db(db_id, db_struct);
	}
	std::string table = db_struct->table_name();
	BSONObj result = get_connection(db_id).findOne(table, MONGO_QUERY(query_name << query_value));
	if (!result.isEmpty()) {
		return result[db_struct->index_name()].numberLong();
	} else {
		return -1;
	}
}

/////////////////////////////////数据转成buffer///////////////////////////
int Mongo_Operator::load_data(int db_id, DB_Struct *db_struct, int64_t key_index, std::vector<Bit_Buffer *> &buffer_vec) {
	if(!db_struct->db_init()) {
		init_db(db_id, db_struct);
	}
	int len = 0;
	if(key_index == 0) {
		//加载整张表数据
		std::vector<BSONObj> record;
		len = get_connection(db_id).count(db_struct->table_name());
		if(len > 0) {
			get_connection(db_id).findN(record, db_struct->table_name(), Query(), len);
		}
		for (int i = 0; i < len; ++i) {
			Bit_Buffer *buffer = DATA_MANAGER->pop_buffer();
			load_data_single(db_struct, record[i], *buffer);
			buffer_vec.push_back(buffer);
		}
	} else {
		//加载单条数据
		BSONObj bsonobj = get_connection(db_id).findOne(db_struct->table_name(), MONGO_QUERY(db_struct->index_name() << (long long int)key_index));
		Bit_Buffer *buffer = DATA_MANAGER->pop_buffer();
		load_data_single(db_struct, bsonobj, *buffer);
		buffer_vec.push_back(buffer);
		len = 1;
	}
	return len;
}

void Mongo_Operator::save_data(int db_id, DB_Struct *db_struct, Bit_Buffer *buffer) {
	if(!db_struct->db_init()) {
		init_db(db_id, db_struct);
	}
	BSONObjBuilder set_builder;
	int64_t key_index = buffer->peek_int64();
	LOG_INFO("table %s save key_index:%ld", db_struct->table_name().c_str(), key_index);
	if (key_index <= 0) {
		return;
	}

	for(Field_Vec::const_iterator iter = db_struct->field_vec().begin();
			iter != db_struct->field_vec().end(); iter++) {
		if(iter->field_label == "arg") {
			save_data_arg(db_struct, *iter, set_builder, *buffer);
		}
		else if(iter->field_label == "vector" || iter->field_label == "map") {
			save_data_vector(db_struct, *iter, set_builder, *buffer);
		}
		else if(iter->field_label == "struct") {
			save_data_struct(db_struct, *iter, set_builder, *buffer);
		}
	}
	get_connection(db_id).update(db_struct->table_name(), MONGO_QUERY(db_struct->index_name() << (long long int)key_index),
			BSON("$set" << set_builder.obj() ), true);
}

void Mongo_Operator::delete_data(int db_id, DB_Struct *db_struct, Isolate* isolate, v8::Local<v8::Object> object) {
	if (!object->IsArray()) {
		LOG_ERROR("delete_data, object is not array, struct_name:%s", db_struct->struct_name().c_str());
		return;
	}

	Local<Context> context(isolate->GetCurrentContext());
	v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(object);
	uint16_t len = array->Length();
	for (uint i = 0; i < len; ++i) {
		v8::Local<v8::Value> value = array->Get(context, i).ToLocalChecked();
		int64_t key_index = value->NumberValue(context).FromJust();
		get_connection(db_id).remove(db_struct->table_name(), MONGO_QUERY(db_struct->index_name() << (long long int)(key_index)));
		DATA_MANAGER->delete_db_data(db_id, db_struct, key_index);
	}
}

void Mongo_Operator::load_data_single(DB_Struct *db_struct, BSONObj &bsonobj, Bit_Buffer &buffer) {
	for(Field_Vec::const_iterator iter = db_struct->field_vec().begin();
			iter != db_struct->field_vec().end(); ++iter) {
		if(iter->field_label == "arg") {
			load_data_arg(db_struct, *iter, bsonobj, buffer);
		}
		else if(iter->field_label == "vector" || iter->field_label == "map") {
			load_data_vector(db_struct, *iter, bsonobj, buffer);
		}
		else if(iter->field_label == "struct") {
			load_data_struct(db_struct, *iter, bsonobj, buffer);
		}
	}
}

void Mongo_Operator::load_data_arg(DB_Struct *db_struct, const Field_Info &field_info, BSONObj &bsonobj, Bit_Buffer &buffer) {
	if(field_info.field_type == "int") {
		int8_t val = bsonobj[field_info.field_name].numberInt();
		buffer.write_int(val, field_info.field_bit);
	}
	else if(field_info.field_type == "uint") {
		uint8_t val = bsonobj[field_info.field_name].numberInt();
		buffer.write_uint(val, field_info.field_bit);
	}
	else if(field_info.field_type == "int64") {
		int64_t val = bsonobj[field_info.field_name].numberLong();
		buffer.write_int64(val);
	}
	else if(field_info.field_type == "uint64") {
		uint64_t val = bsonobj[field_info.field_name].numberLong();
		buffer.write_uint64(val);
	}
	else if(field_info.field_type == "decimal") {
		double val = bsonobj[field_info.field_name].numberDouble();
		buffer.write_decimal(val, field_info.field_bit);
	}
	else if(field_info.field_type == "udecimal") {
		double val = bsonobj[field_info.field_name].numberDouble();
		buffer.write_udecimal(val, field_info.field_bit);
	}
	else if(field_info.field_type == "bool") {
		bool val = bsonobj[field_info.field_name].booleanSafe();
		buffer.write_bool(val);
	}
	else if(field_info.field_type == "string") {
		std::string val = bsonobj[field_info.field_name].valuestrsafe();
		buffer.write_str(val.c_str());
	}
	else {
		LOG_ERROR("Can not find the field_type:%s, field_name:%s, struct_name:%s",
				field_info.field_type.c_str(), field_info.field_name.c_str(), db_struct->struct_name().c_str());
	}
}

void Mongo_Operator::load_data_vector(DB_Struct *db_struct, const Field_Info &field_info, BSONObj &bsonobj, Bit_Buffer &buffer) {
	BSONObj fieldobj = bsonobj.getObjectField(field_info.field_name);
	uint16_t len = fieldobj.nFields();
	BSONObjIterator field_iter(fieldobj);
	BSONObj obj;

	buffer.write_uint(len, field_info.field_vbit);
	if(db_struct->is_struct(field_info.field_type)) {
		for(int i = 0; i < len; ++i) {
			obj = field_iter.next().embeddedObject();
			load_data_struct(db_struct, field_info, obj, buffer);
		}
	}
	else {
		for(int i = 0; i < len; ++i) {
			obj = field_iter.next().embeddedObject();
			load_data_arg(db_struct, field_info, obj, buffer);
		}
	}
}

void Mongo_Operator::load_data_struct(DB_Struct *db_struct, const Field_Info &field_info, BSONObj &bsonobj, Bit_Buffer &buffer) {
	DB_Struct *sub_struct = STRUCT_MANAGER->get_db_struct(field_info.field_type);
	if(db_struct == nullptr){
		return;
	}

	BSONObj fieldobj = bsonobj.getObjectField(field_info.field_type);
	Field_Vec field_vec = sub_struct->field_vec();
	for(Field_Vec::const_iterator iter = field_vec.begin();
			iter != field_vec.end(); iter++) {
		if(iter->field_label == "arg") {
			load_data_arg(db_struct, *iter, fieldobj, buffer);
		}
		else if(iter->field_label == "vector" || iter->field_label == "map") {
			load_data_vector(db_struct, *iter, fieldobj, buffer);
		}
		else if(iter->field_label == "struct") {
			load_data_struct(db_struct, *iter, fieldobj, buffer);
		}
	}
}

void Mongo_Operator::save_data_arg(DB_Struct *db_struct, const Field_Info &field_info, BSONObjBuilder &builder, Bit_Buffer &buffer) {
	if(field_info.field_type == "int") {
		int8_t val = buffer.read_int(field_info.field_bit);
		builder << field_info.field_name << (int)val;
	}
	else if(field_info.field_type == "uint") {
		uint8_t val = buffer.read_uint(field_info.field_bit);
		builder << field_info.field_name << (uint)val;
	}
	else if(field_info.field_type == "int64") {
		int64_t val = buffer.read_int64();
		builder << field_info.field_name << (long long int)val;
	}
	else if(field_info.field_type == "uint64") {
		uint64_t val = buffer.read_uint64();
		builder << field_info.field_name << (long long int)val;
	}
	else if(field_info.field_type == "decimal") {
		double val = buffer.read_decimal(field_info.field_bit);
		builder << field_info.field_name << val;
	}
	else if(field_info.field_type == "udecimal") {
		double val = buffer.read_udecimal(field_info.field_bit);
		builder << field_info.field_name << val;
	}
	else if(field_info.field_type == "bool") {
		bool val = buffer.read_bool();
		builder << field_info.field_name << val;
	}
	else if(field_info.field_type == "string") {
		std::string val = "";
		buffer.read_str(val);
		builder << field_info.field_name << val;
	}
	else {
		LOG_ERROR("Can not find the field_type:%s, field_name:%s, struct_name:%s",
				field_info.field_type.c_str(), field_info.field_name.c_str(), db_struct->struct_name().c_str());
	}
}

void Mongo_Operator::save_data_vector(DB_Struct *db_struct, const Field_Info &field_info, BSONObjBuilder &builder, Bit_Buffer &buffer) {
	std::vector<BSONObj> bson_vec;

	uint16_t len = buffer.read_uint(field_info.field_vbit);
	for (uint i = 0; i < len; ++i) {
		if(db_struct->is_struct(field_info.field_type)) {
			BSONObjBuilder obj_builder;
			save_data_struct(db_struct, field_info, obj_builder, buffer);
			bson_vec.push_back(obj_builder.obj());
		}
		else {
			BSONObjBuilder obj_builder;
			save_data_arg(db_struct, field_info, obj_builder, buffer);
			bson_vec.push_back(obj_builder.obj());
		}
	}

	builder << field_info.field_name << bson_vec;
}

void Mongo_Operator::save_data_struct(DB_Struct *db_struct, const Field_Info &field_info, BSONObjBuilder &builder, Bit_Buffer &buffer) {
	DB_Struct *sub_struct = STRUCT_MANAGER->get_db_struct(field_info.field_type);
	if (sub_struct == nullptr) {
		return;
	}

	BSONObjBuilder obj_builder;
	Field_Vec field_vec = sub_struct->field_vec();
	for(Field_Vec::const_iterator iter = field_vec.begin();
			iter != field_vec.end(); iter++) {
		if(iter->field_label == "arg") {
			save_data_arg(db_struct, *iter, obj_builder, buffer);
		}
		else if(iter->field_label == "vector" || iter->field_label == "map") {
			save_data_vector(db_struct, *iter, obj_builder, buffer);
		}
		else if(iter->field_label == "struct") {
			save_data_struct(db_struct, *iter, obj_builder, buffer);
		}
	}

	builder << field_info.field_name << obj_builder.obj();
}

#endif

