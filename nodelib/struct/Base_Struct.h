/*
 * Base_Struct.h
 *
 *  Created on: May 30, 2016
 *      Author: zhangyalei
 */
 
#ifndef BASE_STRUCT_H_
#define BASE_STRUCT_H_

#include "include/v8.h"
#include "json/json.h"
#include "Byte_Buffer.h"
#include "Bit_Buffer.h"
#include "Xml.h"

using namespace v8;
class Base_Struct {
public:
	Base_Struct();
	virtual ~Base_Struct();

	virtual int init(Xml &xml, TiXmlNode *node);

	inline const std::string &struct_name() { return struct_name_; }
	inline const Field_Vec& field_vec() { return field_vec_; }
	inline bool is_struct(const std::string &type);

	//将Json::Value转成v8::object
	v8::Local<v8::Object> build_value_object(Isolate* isolate, const Field_Vec &field_vec, const Json::Value &value);
	//将v8::object转成Json::Value
	int build_json_value(Isolate* isolate, const Field_Vec &field_vec, v8::Local<v8::Object> object, Json::Value &root);

	//将Bit_Buffer转成v8::object
	int build_bit_object(Isolate* isolate, const Field_Vec &field_vec, Bit_Buffer &buffer, v8::Local<v8::Object> object);
	//将v8::object转换Bit_Buffer
	int build_bit_buffer(Isolate* isolate, const Field_Vec &field_vec, Bit_Buffer &buffer, v8::Local<v8::Object> object);

	//将Bit_Buffer转换Bit_Buffer
	int build_bit_buffer(const Field_Vec &field_vec, Bit_Buffer &src_buffer, Bit_Buffer &dst_buffer);

private:
	//初始化字段信息
	int init_field(Xml &xml, TiXmlNode *node, Field_Info &field_info);

	//将Json::Value转成v8::object
	v8::Local<v8::Value> build_value_object_arg(Isolate* isolate, const Field_Info &field_info, const Json::Value &value);
	v8::Local<v8::Array> build_value_object_vector(Isolate* isolate, const Field_Info &field_info, const Json::Value &value);
	v8::Local<v8::Map> build_value_object_map(Isolate* isolate, const Field_Info &field_info, const Json::Value &value);
	v8::Local<v8::Object> build_value_object_struct(Isolate* isolate, const Field_Info &field_info, const Json::Value &value);

	//将v8::object转成Json::Value
	int build_json_value_arg(Isolate* isolate, const Field_Info &field_info, v8::Local<v8::Value> value, Json::Value &root);
	int build_json_value_vector(Isolate* isolate, const Field_Info &field_info, v8::Local<v8::Value> value, Json::Value &root);
	int build_json_value_map(Isolate* isolate, const Field_Info &field_info, v8::Local<v8::Value> value, Json::Value &root);
	int build_json_value_struct(Isolate* isolate, const Field_Info &field_info, v8::Local<v8::Value> value, Json::Value &root);

	//Bit_Buffer转成v8::object
	v8::Local<v8::Value> build_bit_object_arg(Isolate* isolate, const Field_Info &field_info, Bit_Buffer &buffer);
	v8::Local<v8::Array> build_bit_object_vector(Isolate* isolate, const Field_Info &field_info, Bit_Buffer &buffer);
	v8::Local<v8::Map> build_bit_object_map(Isolate* isolate, const Field_Info &field_info, Bit_Buffer &buffer);
	v8::Local<v8::Object> build_bit_object_struct(Isolate* isolate, const Field_Info &field_info, Bit_Buffer &buffer);

	//将v8::object转换Bit_Buffer
	int build_bit_buffer_arg(Isolate* isolate, const Field_Info &field_info, Bit_Buffer &buffer, v8::Local<v8::Value> value);
	int build_bit_buffer_vector(Isolate* isolate, const Field_Info &field_info, Bit_Buffer &buffer, v8::Local<v8::Value> value);
	int build_bit_buffer_map(Isolate* isolate, const Field_Info &field_info, Bit_Buffer &buffer, v8::Local<v8::Value> value);
	int build_bit_buffer_struct(Isolate* isolate, const Field_Info &field_info, Bit_Buffer &buffer, v8::Local<v8::Value> value);

	//将Bit_Buffer转换Bit_Buffer
	int build_bit_buffer_arg(const Field_Info &field_info, Bit_Buffer &src_buffer, Bit_Buffer &dst_buffer);
	int build_bit_buffer_vector(const Field_Info &field_info, Bit_Buffer &src_buffer, Bit_Buffer &dst_buffer);
	int build_bit_buffer_map(const Field_Info &field_info, Bit_Buffer &src_buffer, Bit_Buffer &dst_buffer);
	int build_bit_buffer_struct(const Field_Info &field_info, Bit_Buffer &src_buffer, Bit_Buffer &dst_buffer);

private:
	std::string struct_name_;
	Field_Vec field_vec_;
};

/////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Base_Struct::is_struct(const std::string &field_type){
	if(field_type == "int" || field_type == "uint" || field_type == "int64" || field_type == "uint64" ||
		 field_type == "float" || field_type == "bool" || field_type == "string") return false;
	return true;
}

#endif /* BASE_STRUCT_H_ */
