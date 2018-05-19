/*
 * Base_Struct.cpp
 *
 *  Created on: May 30, 2016
 *      Author: zhangyalei
 */

#include "V8_Base.h"
#include "Struct_Manager.h"
#include "Base_Struct.h"

Base_Struct::Base_Struct():
	struct_name_()
{
	field_vec_.clear();
}

Base_Struct::~Base_Struct(){
	struct_name_.clear();
	field_vec_.clear();
}

int Base_Struct::init(Xml &xml, TiXmlNode *node) {
	if(node) {
		struct_name_ = xml.get_key(node);

		TiXmlNode* child_node = xml.enter_node(node, struct_name_.c_str());
		if(child_node) {
			XML_LOOP_BEGIN(child_node)
				Field_Info field_info;
				init_field(xml, child_node, field_info);
				field_vec_.push_back(field_info);
			XML_LOOP_END(child_node)
		}
	}
	return 0;
}

int Base_Struct::init_field(Xml &xml, TiXmlNode *node, Field_Info &field_info) {
	if(node) {
		field_info.field_label = xml.get_key(node);
		field_info.field_type = xml.get_attr_str(node, "type");
		field_info.field_bit = xml.get_attr_int(node, "bit");
		field_info.field_name = xml.get_attr_str(node, "name");

		if (field_info.field_label == "vector") {
			field_info.field_vbit = xml.get_attr_int(node, "vbit");
		}
		else if (field_info.field_label == "map") {
			field_info.field_vbit = xml.get_attr_int(node, "vbit");
			field_info.key_type = xml.get_attr_str(node, "key_type");
			field_info.key_bit = xml.get_attr_int(node, "key_bit");
			field_info.key_name = xml.get_attr_str(node, "key_name");
		}
		else if (field_info.field_label == "if" || field_info.field_label == "switch" || field_info.field_label== "case") {
			field_info.case_val = xml.get_attr_int(node, "val");

			TiXmlNode* child_node = xml.enter_node(node, field_info.field_label.c_str());
			if (child_node) {
				XML_LOOP_BEGIN(child_node)
					Field_Info child_field;
					init_field(xml, child_node, child_field);
					field_info.children.push_back(child_field);
				XML_LOOP_END(child_node)
			}
		}
	}
	return 0;
}

///////////////////////////将Json::Value转成v8::object/////////////////////////
v8::Local<v8::Object> Base_Struct::build_value_object(Isolate* isolate, const Field_Vec &field_vec, const Json::Value &value) {
	EscapableHandleScope handle_scope(isolate);
	Local<Context> context(isolate->GetCurrentContext());

	v8::Local<v8::Object> obj = Object::New(isolate);
	Json::Value sub_value;
	Local<String> key;
	for(Field_Vec::const_iterator iter = field_vec.begin(); iter != field_vec.end(); iter++) {
		//根据字段名称获取子json，然后传到不同类型的子函数里面，子函数可以直接使用json的值
		sub_value = value[iter->field_name];
		key = String::NewFromUtf8(isolate, iter->field_name.c_str(), NewStringType::kNormal).ToLocalChecked();
		if(iter->field_label == "arg") {
			v8::Local<v8::Value> v8_value = build_value_object_arg(isolate, *iter, sub_value);
			if (!v8_value.IsEmpty()) {
				obj->Set(context, key, v8_value).FromJust();
			}
			else{
				return handle_scope.Escape(v8::Local<v8::Object>());
			}
		}
		else if(iter->field_label == "vector") {
			v8::Local<v8::Array> array = build_value_object_vector(isolate, *iter, sub_value);
			if (!array.IsEmpty()) {
				obj->Set(context, key, array).FromJust();
			}
			else{
				return handle_scope.Escape(v8::Local<v8::Object>());
			}
		}
		else if(iter->field_label == "map") {
			v8::Local<v8::Map> map = build_value_object_map(isolate, *iter, sub_value);
			if (!map.IsEmpty()) {
				obj->Set(context, key, map).FromJust();
			}
			else{
				return handle_scope.Escape(v8::Local<v8::Object>());
			}
		}
		else if(iter->field_label == "struct") {
			v8::Local<v8::Object> object = build_value_object_struct(isolate, *iter, sub_value);
			if (!object.IsEmpty()) {
				obj->Set(context, key, object).FromJust();
			}
			else{
				return handle_scope.Escape(v8::Local<v8::Object>());
			}
		}
		else if(iter->field_label == "if") {
			if(sub_value != Json::Value::null) {
				v8::Local<v8::Object> object = build_value_object(isolate, iter->children, value);
				if (!object.IsEmpty()) {
					obj->Set(context, key, object).FromJust();
				}
				else{
					return handle_scope.Escape(v8::Local<v8::Object>());
				}
			}
		}
		else if(iter->field_label == "switch") {
			uint case_val = sub_value.asUInt();
			obj->Set(context, key, Uint32::New(isolate, case_val)).FromJust();
			for(uint i = 0; i < iter->children.size(); ++i) {
				if(iter->children[i].field_label == "case" && iter->children[i].case_val == case_val) {
					//找到对应的case标签，对case标签内的child数组进行build
					v8::Local<v8::Object> object = build_value_object(isolate, iter->children, value);
					if (!object.IsEmpty()) {
						obj->Set(context, key, object).FromJust();
					}
					else{
						return handle_scope.Escape(v8::Local<v8::Object>());
					}
					break;
				}
			}
		}
	}
	return handle_scope.Escape(obj);
}

///////////////////////////将v8::object转成Json::Value/////////////////////////
int Base_Struct::build_json_value(Isolate* isolate, const Field_Vec &field_vec, v8::Local<v8::Object> object, Json::Value &root) {
	HandleScope handle_scope(isolate);

	int ret = 0;
	v8::Local<v8::Value> value;
	for(Field_Vec::const_iterator iter = field_vec.begin(); iter != field_vec.end(); iter++) {
		value = object->Get(isolate->GetCurrentContext(), String::NewFromUtf8(isolate, iter->field_name.c_str(), NewStringType::kNormal).ToLocalChecked()).ToLocalChecked();
		if(iter->field_label == "arg") {
			ret = build_json_value_arg(isolate, *iter, value, root);
		}
		else if(iter->field_label == "vector") {
			ret = build_json_value_vector(isolate, *iter, value, root);
		}
		else if(iter->field_label == "map") {
			ret = build_json_value_map(isolate, *iter, value, root);
		}
		else if(iter->field_label == "struct") {
			ret = build_json_value_struct(isolate, *iter, value, root);
		}
		else if(iter->field_label == "if") {
			if(!value->IsUndefined()) {
				ret = build_json_value(isolate, iter->children, object, root);
			}
		}
		else if(iter->field_label == "switch") {
			uint case_val = 0;
			if(value->IsUint32()) {
				case_val = value->Uint32Value(isolate->GetCurrentContext()).FromJust();
			}

			for(uint i = 0; i < iter->children.size(); ++i) {
				if(iter->children[i].field_label == "case" && iter->children[i].case_val == case_val) {
					//找到对应的case标签，对case标签内的child数组进行build
					ret = build_json_value(isolate, iter->children[i].children, object, root);
					break;
				}
			}
		}

		if(ret < 0) {
			return ret;
		}
	}

	return 0;
}

/////////////////////////////将Bit_Buffer转成v8::object///////////////////////////
int Base_Struct::build_bit_object(Isolate* isolate, const Field_Vec &field_vec, Bit_Buffer &buffer, v8::Local<v8::Object> object) {
	HandleScope handle_scope(isolate);
	Local<Context> context(isolate->GetCurrentContext());

	Local<String> key;
	for(Field_Vec::const_iterator iter = field_vec.begin(); iter != field_vec.end(); iter++) {
		key = String::NewFromUtf8(isolate, iter->field_name.c_str(), NewStringType::kNormal).ToLocalChecked();
		if(iter->field_label == "arg") {
			v8::Local<v8::Value> value = build_bit_object_arg(isolate, *iter, buffer);
			if (!value.IsEmpty()) {
				object->Set(context, key, value).FromJust();
			}
			else {
				return -1;
			}
		}
		else if(iter->field_label == "vector") {
			v8::Local<v8::Array> array = build_bit_object_vector(isolate, *iter, buffer);
			if(!array.IsEmpty()) {
				object->Set(context, key, array).FromJust();
			}
			else {
				return -1;
			}
		}
		else if(iter->field_label == "map") {
			v8::Local<v8::Map> map = build_bit_object_map(isolate, *iter, buffer);
			if(!map.IsEmpty()) {
				object->Set(context, key, map).FromJust();
			}
			else {
				return -1;
			}
		}
		else if(iter->field_label == "struct") {
			v8::Local<v8::Object> sub_object = build_bit_object_struct(isolate, *iter, buffer);
			if(!sub_object.IsEmpty()) {
				object->Set(context, key, sub_object).FromJust();
			}
			else {
				return -1;
			}
		}
		else if(iter->field_label == "if") {
			if(buffer.read_bits_available() >= 1) {
				bool field_exist = buffer.read_bool();
				if(field_exist) {
					int ret = build_bit_object(isolate, iter->children, buffer, object);
					if(ret < 0) {
						return ret;
					}
				}
		    }
			else {
				LOG_ERROR("bit not enough,field_type:%s,available_bit:%d,field_bit:%d,field_name:%s,struct_name:%s",
					iter->field_type.c_str(), buffer.read_bits_available(), 1, iter->field_name.c_str(), struct_name().c_str());
				return -1;
			}
		}
		else if(iter->field_label == "switch") {
			if(buffer.read_bits_available() >= iter->field_bit) {
				uint case_val = buffer.read_uint(iter->field_bit);
				v8::Local<v8::Value> value = Uint32::New(isolate, case_val);
				object->Set(context, key, value).FromJust();
				for(uint i = 0; i < iter->children.size(); ++i) {
					if(iter->children[i].field_label == "case" && iter->children[i].case_val == case_val) {
						//找到对应的case标签，对case标签内的child数组进行build
						int ret = build_bit_object(isolate, iter->children[i].children, buffer, object);
						if(ret < 0) {
							return ret;
						}
						break;
					}
				}
			}
			else {
				LOG_ERROR("bit not enough,field_type:%s,available_bit:%d,field_bit:%d,field_name:%s,struct_name:%s",
					iter->field_type.c_str(), buffer.read_bits_available(), iter->field_bit, iter->field_name.c_str(), struct_name().c_str());
				return -1;
			}
		}
	}
	return 0;
}

////////////////////////////////将v8::object转换Bit_Buffer///////////////////////////
int Base_Struct::build_bit_buffer(Isolate* isolate, const Field_Vec &field_vec, Bit_Buffer &buffer, v8::Local<v8::Object> object) {
	HandleScope handle_scope(isolate);

	int ret = 0;
	v8::Local<v8::Value> value;
	for(Field_Vec::const_iterator iter = field_vec.begin(); iter != field_vec.end(); iter++) {
		value = object->Get(isolate->GetCurrentContext(), String::NewFromUtf8(isolate, iter->field_name.c_str(), NewStringType::kNormal).ToLocalChecked()).ToLocalChecked();
		if(iter->field_label == "arg") {
			ret = build_bit_buffer_arg(isolate, *iter, buffer, value);
		}
		else if(iter->field_label == "vector") {
			ret = build_bit_buffer_vector(isolate, *iter, buffer, value);
		}
		else if(iter->field_label == "map") {
			ret = build_bit_buffer_map(isolate, *iter, buffer, value);
		}
		else if(iter->field_label == "struct") {
			ret = build_bit_buffer_struct(isolate, *iter, buffer, value);
		}
		else if(iter->field_label == "if") {
			if(value->IsUndefined()) {
				buffer.write_bool(false);
			}
			else {
				buffer.write_bool(true);
				ret = build_bit_buffer(isolate, iter->children, buffer, object);
			}
		}
		else if(iter->field_label == "switch") {
			uint case_val = 0;
			if(value->IsUint32()) {
				case_val = value->Uint32Value(isolate->GetCurrentContext()).FromJust();
			}
			buffer.write_uint(case_val, iter->field_bit);

			for(uint i = 0; i < iter->children.size(); ++i) {
				if(iter->children[i].field_label == "case" && iter->children[i].case_val == case_val) {
					//找到对应的case标签，对case标签内的child数组进行build
					ret = build_bit_buffer(isolate, iter->children[i].children, buffer, object);
					break;
				}
			}
		}

		if(ret < 0) {
			return ret;
		}
	}
	return 0;
}

////////////////////////////////将Bit_Buffer转换Bit_Buffer///////////////////////////
int Base_Struct::build_bit_buffer(const Field_Vec &field_vec, Bit_Buffer &src_buffer, Bit_Buffer &dst_buffer) {
	for(Field_Vec::const_iterator iter = field_vec.begin(); iter != field_vec.end(); iter++) {
		int ret = 0;
		if(iter->field_label == "arg") {
			ret = build_bit_buffer_arg(*iter, src_buffer, dst_buffer);
		}
		else if(iter->field_label == "vector") {
			ret = build_bit_buffer_vector(*iter, src_buffer, dst_buffer);
		}
		else if(iter->field_label == "map") {
			ret = build_bit_buffer_map(*iter, src_buffer, dst_buffer);
		}
		else if(iter->field_label == "struct") {
			ret = build_bit_buffer_struct(*iter, src_buffer, dst_buffer);
		}
		else if(iter->field_label == "if") {
			if(src_buffer.read_bits_available() >= 1) {
				bool field_exist = src_buffer.read_bool();
				dst_buffer.write_bool(field_exist);
				if(field_exist) {
					ret = build_bit_buffer(iter->children, src_buffer, dst_buffer);
				}
		    }
			else {
				LOG_ERROR("bit not enough,field_type:%s,available_bit:%d,field_bit:%d,field_name:%s,struct_name:%s",
					iter->field_type.c_str(), src_buffer.read_bits_available(), 1, iter->field_name.c_str(), struct_name().c_str());
				ret = -1;
			}
		}
		else if(iter->field_label == "switch") {
			if(src_buffer.read_bits_available() >= iter->field_bit) {
				uint case_val = src_buffer.read_uint(iter->field_bit);
				dst_buffer.write_uint(case_val, iter->field_bit);

				for(uint i = 0; i < iter->children.size(); ++i) {
					if(iter->children[i].field_label == "case" && iter->children[i].case_val == case_val) {
						//找到对应的case标签，对case标签内的child数组进行build
						ret = build_bit_buffer(iter->children[i].children, src_buffer, dst_buffer);
						break;
					}
				}
			}
			else {
				LOG_ERROR("bit not enough,field_type:%s,available_bit:%d,field_bit:%d,field_name:%s,struct_name:%s",
					iter->field_type.c_str(), src_buffer.read_bits_available(), iter->field_bit, iter->field_name.c_str(), struct_name().c_str());
				ret = -1;
			}
		}

		if(ret < 0) {
			return ret;
		}
	}
	return 0;
}

//////////////////////////////将json转换成object//////////////////////////////
v8::Local<v8::Value> Base_Struct::build_value_object_arg(Isolate* isolate, const Field_Info &field_info, const Json::Value &value) {
	EscapableHandleScope handle_scope(isolate);

	int ret = 0;
	v8::Local<v8::Value> v8_value;
	if(field_info.field_type == "int") {
		if(value.isInt()) {
			int32_t val = value.asInt();
			v8_value = Int32::New(isolate, val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
		}
		else {
			ret = -1;
		}
	}
	else if(field_info.field_type == "uint") {
		if(value.isUInt()) {
			uint32_t val = value.asUInt();
			v8_value = Uint32::New(isolate, val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
		}
		else {
			ret = -1;
		}
	}
	else if(field_info.field_type == "int64" || field_info.field_type == "uint64"
			|| field_info.field_type == "float") {
		if(value.isDouble()) {
			double val = value.asDouble();
			v8_value = Number::New(isolate, val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%f",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
		}
		else {
			ret = -1;
		}
	}
	else if(field_info.field_type == "bool") {
		if(value.isBool()) {
			bool val = value.asBool();
			v8_value = Boolean::New(isolate, val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
		}
		else {
			ret = -1;
		}
	}
	else if(field_info.field_type == "string") {
		if(value.isString()) {
			std::string val = value.asString();
			v8_value = String::NewFromUtf8(isolate, val.c_str(), NewStringType::kNormal).ToLocalChecked();
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%s",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val.c_str());
			}
		}
		else {
			ret = -1;
		}
	}
	else {
		ret = -1;
	}

	if(ret < 0) {
		LOG_ERROR("field_type not exist or field_value undefined,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
			struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str());
		return handle_scope.Escape(v8::Local<v8::Value>());
	}
	else {
		return handle_scope.Escape(v8_value);
	}
}

v8::Local<v8::Array> Base_Struct::build_value_object_vector(Isolate* isolate, const Field_Info &field_info, const Json::Value &value) {
	EscapableHandleScope handle_scope(isolate);
	if (!value.isArray()) {
		LOG_ERROR("value is not array,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
			struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str());
		return handle_scope.Escape(v8::Local<v8::Array>());
	}

	Local<Context> context(isolate->GetCurrentContext());
	uint32_t length = value.size();
	if(STRUCT_MANAGER->log_trace()) {
		LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_length:%d",
			struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),length);
	}
	v8::Local<v8::Array> array = v8::Array::New(isolate, length);
	for(uint32_t i = 0; i < length; ++i) {
		if(is_struct(field_info.field_type)) {
			v8::Local<v8::Object> object = build_value_object_struct(isolate, field_info, value[i]);
			if (!object.IsEmpty()) {
				array->Set(context, i, object).FromJust();
			}
			else {
				return handle_scope.Escape(v8::Local<v8::Array>());
			}
		}
		else {
			v8::Local<v8::Value> v8_value = build_value_object_arg(isolate, field_info, value[i]);
			if (!v8_value.IsEmpty()) {
				array->Set(context, i, v8_value).FromJust();
			}
			else {
				return handle_scope.Escape(v8::Local<v8::Array>());
			}
		}
	}
	return handle_scope.Escape(array);
}

v8::Local<v8::Map> Base_Struct::build_value_object_map(Isolate* isolate, const Field_Info &field_info, const Json::Value &value) {
	EscapableHandleScope handle_scope(isolate);
	if (!value.isArray()) {
		LOG_ERROR("value is not array,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
			struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str());
		return handle_scope.Escape(v8::Local<v8::Map>());
	}

	Field_Info key_info;
	key_info.field_type = field_info.key_type;
	key_info.field_bit = field_info.key_bit;
	key_info.field_name = field_info.key_name;

	Local<Context> context(isolate->GetCurrentContext());
	uint32_t length = value.size();
	if(STRUCT_MANAGER->log_trace()) {
		LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_length:%d",
			struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),length);
	}
	v8::Local<v8::Map> map = v8::Map::New(isolate);
	v8::Local<v8::Value> map_key;
	v8::Local<v8::Value> map_value;
	for(uint32_t i = 0; i < length; ++i) {
		//判断map中key的类型，key可以为基本类型或者结构体类型
		if(is_struct(key_info.field_type)) {
			key_info.field_label = "struct";
			map_key = build_value_object_struct(isolate, key_info, value[i]);
		}
		else {
			key_info.field_label = "arg";
			map_key = build_value_object_arg(isolate, key_info, value[i]);
		}

		//判断map中value的类型，value可以为基本类型或者结构体类型
		if(is_struct(field_info.field_type)) {
			map_value = build_value_object_struct(isolate, field_info, value[i]);
		}
		else {
			map_value = build_value_object_arg(isolate, field_info, value[i]);
		}

		if (!map_key.IsEmpty() && !map_value.IsEmpty()) {
			map->Set(context, map_key, map_value).ToLocalChecked();
		}
		else {
			return handle_scope.Escape(v8::Local<v8::Map>());
		}
	}

	return handle_scope.Escape(map);
}

v8::Local<v8::Object> Base_Struct::build_value_object_struct(Isolate* isolate, const Field_Info &field_info, const Json::Value &value) {
	EscapableHandleScope handle_scope(isolate);
	if (!value.isObject()) {
		LOG_ERROR("value is not object,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
			struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str());
		return handle_scope.Escape(v8::Local<v8::Object>());
	}

	Base_Struct *base_struct = STRUCT_MANAGER->get_base_struct(field_info.field_type);
	if (base_struct == nullptr) {
		return handle_scope.Escape(v8::Local<v8::Object>());
	}

	Local<Context> context(isolate->GetCurrentContext());
	v8::Local<v8::Object> object = build_value_object(isolate, base_struct->field_vec(), value);
	return handle_scope.Escape(object);
}

////////////////////////////将v8::object转成Json::Value//////////////////////
int Base_Struct::build_json_value_arg(Isolate* isolate, const Field_Info &field_info, v8::Local<v8::Value> value, Json::Value &root) {
	HandleScope handle_scope(isolate);
	Local<Context> context(isolate->GetCurrentContext());

	int ret = 0;
	if(field_info.field_type == "int") {
		if(value->IsInt32()) {
			int32_t val = value->Int32Value(context).FromJust();
			root[field_info.field_name] = Json::Value(val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
		}
		else {
			ret = -1;
		}
	}
	else if(field_info.field_type == "uint") {
		if(value->IsUint32()) {
			uint32_t val = value->Uint32Value(context).FromJust();
			root[field_info.field_name] = Json::Value(val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
		}
		else {
			ret = -1;
		}
	}
	else if(field_info.field_type == "int64" || field_info.field_type == "uint64" || field_info.field_type == "float") {
		if(value->IsNumber()) {
			double val = value->NumberValue(context).FromJust();
			root[field_info.field_name] = Json::Value(val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%f",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
		}
		else {
			ret = -1;
		}
	}
	else if(field_info.field_type == "bool") {
		if (value->IsBoolean()) {
			bool val = value->BooleanValue(context).FromJust();
			root[field_info.field_name] = Json::Value(val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
		}
		else {
			ret = -1;
		}
	}
	else if(field_info.field_type == "string") {
		if (value->IsString()) {
			String::Utf8Value str(value->ToString(context).ToLocalChecked());
			std::string val = to_string(str);
			root[field_info.field_name] = Json::Value(val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%s",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val.c_str());
			}
		}
		else {
			ret = -1;
		}
	}
	else {
		ret = -1;
	}

	if(ret < 0) {
		LOG_ERROR("field_type not exist or field_value undefined,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
			struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str());
	}
	return ret;
}

int Base_Struct::build_json_value_vector(Isolate* isolate, const Field_Info &field_info, v8::Local<v8::Value> value, Json::Value &root) {
	HandleScope handle_scope(isolate);
	if (!value->IsArray()) {
		LOG_ERROR("value is not array,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
			struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str());
		return -1;
	}

	Local<Context> context(isolate->GetCurrentContext());
	v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(value);
	uint32_t length = array->Length();
	if(STRUCT_MANAGER->log_trace()) {
		LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_length:%d",
			struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),length);
	}
	for (uint32_t i = 0; i < length; ++i) {
		Json::Value json_value;
		v8::Local<v8::Value> element = array->Get(context, i).ToLocalChecked();
		if(is_struct(field_info.field_type)) {
			int ret = build_json_value_struct(isolate, field_info, element, json_value);
			if(ret < 0) {
				return ret;
			}
		}
		else {
			int ret = build_json_value_arg(isolate, field_info, element, json_value);
			if(ret < 0) {
				return ret;
			}
		}
		//将子节点添加到json数组里
		root[field_info.field_name].append(json_value);
	}
	return 0;
}

int Base_Struct::build_json_value_map(Isolate* isolate, const Field_Info &field_info, v8::Local<v8::Value> value, Json::Value &root) {
	HandleScope handle_scope(isolate);
	if (!value->IsMap()) {
		LOG_ERROR("value is not map,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
			struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str());
		return -1;
	}

	Field_Info key_info;
	key_info.field_type = field_info.key_type;
	key_info.field_bit = field_info.key_bit;
	key_info.field_name = field_info.key_name;

	Local<Context> context(isolate->GetCurrentContext());
	v8::Local<v8::Map> map = v8::Local<v8::Map>::Cast(value);
	uint32_t length = map->Size();
	if(STRUCT_MANAGER->log_trace()) {
		LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_length:%d",
			struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),length);
	}
	v8::Local<v8::Array> array = map->AsArray();
	//index N is the Nth key and index N + 1 is the Nth value
	for (uint32_t i = 0; i < length * 2; i = i + 2) {
		int ret1 = 0;
		int ret2 = 0;
		//判断map中key的类型，key可以为基本类型或者结构体类型
		Json::Value json_key_value;
		v8::Local<v8::Value> map_key = array->Get(context, i).ToLocalChecked();
		if(is_struct(key_info.field_type)) {
			key_info.field_label = "struct";
			ret1 = build_json_value_struct(isolate, key_info, map_key, json_key_value);
		}
		else {
			key_info.field_label = "arg";
			ret1 = build_json_value_arg(isolate, key_info, map_key, json_key_value);
		}

		//判断map中value的类型，value可以为基本类型或者结构体类型
		Json::Value json_map_value;
		v8::Local<v8::Value> map_value = array->Get(context, i + 1).ToLocalChecked();
		if(is_struct(field_info.field_type)) {
			ret2 = build_json_value_struct(isolate, field_info, map_value, json_map_value);
		}
		else {
			ret2 = build_json_value_arg(isolate, field_info, map_value, json_map_value);
		}

		if(ret1 < 0 || ret2 < 0) {
			return -1;
		}

		//将子节点添加到json数组里
		root[field_info.field_name].append(json_key_value);
		root[field_info.field_name].append(json_map_value);
	}
	return 0;
}

int Base_Struct::build_json_value_struct(Isolate* isolate, const Field_Info &field_info, v8::Local<v8::Value> value, Json::Value &root) {
	HandleScope handle_scope(isolate);
	if (!value->IsObject()) {
		LOG_ERROR("value is not object,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
			struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str());
		return -1;
	}

	Base_Struct *base_struct = STRUCT_MANAGER->get_base_struct(field_info.field_type);
	if (base_struct == nullptr) {
		return -1;
	}

	Local<Context> context(isolate->GetCurrentContext());
	Json::Value json_value;
	v8::Local<v8::Object> object = value->ToObject(context).ToLocalChecked();
	int ret = build_json_value(isolate, base_struct->field_vec(), object, json_value);
	if(ret < 0) {
		return ret;
	}
	root[field_info.field_name] = Json::Value(json_value);

	return 0;
}

////////////////////////////将bit_buffer转换object///////////////////////////
v8::Local<v8::Value> Base_Struct::build_bit_object_arg(Isolate* isolate, const Field_Info &field_info, Bit_Buffer &buffer) {
	EscapableHandleScope handle_scope(isolate);

	int ret = 0;
	v8::Local<v8::Value> value;
	if(field_info.field_type == "int") {
		if(buffer.read_bits_available() < field_info.field_bit) {
			ret = -1;
        }
		else {
			int32_t val = buffer.read_int(field_info.field_bit);
			value = Int32::New(isolate, val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
        }
	}
	else if(field_info.field_type == "uint") {
		if(buffer.read_bits_available() < field_info.field_bit) {
			ret = -1;
        }
		else {
			uint32_t val = buffer.read_uint(field_info.field_bit);
			value = Uint32::New(isolate, val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
        }
	}
	else if(field_info.field_type == "int64") {
		if(buffer.read_bits_available() < 64) {
			ret = -1;
        }
		else {
			int64_t val = buffer.read_int64();
			value = Number::New(isolate, val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%ld",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
        }
	}
	else if(field_info.field_type == "uint64") {
		if(buffer.read_bits_available() < 64) {
			ret = -1;
        }
		else {
			uint64_t val = buffer.read_uint64();
			value = Number::New(isolate, val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%ld",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
        }
	}
	else if(field_info.field_type == "float") {
		if(buffer.read_bits_available() < 32) {
			ret = -1;
        }
		else {
			float val = buffer.read_float();
			value = Number::New(isolate, val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%f",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
        }
	}
	else if(field_info.field_type == "bool") {
		if(buffer.read_bits_available() < 1) {
			ret = -1;
        }
		else {
			bool val = buffer.read_bool();
			value = Boolean::New(isolate, val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
        }
	}
	else if(field_info.field_type == "string") {
		if(buffer.read_bits_available() < 8) {		//at least 1 bytes /0
			ret = -1;
        }
		else {
			std::string val = "";
			buffer.read_str(val);
			value = String::NewFromUtf8(isolate, val.c_str(), NewStringType::kNormal).ToLocalChecked();
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%s",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val.c_str());
			}
        }
	}
	else {
		ret = -1;
	}

	if (ret < 0) {
		LOG_ERROR("bit not enough,available_bit:%d,field_bit:%d,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
			buffer.read_bits_available(),field_info.field_bit,struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str());
		return handle_scope.Escape(v8::Local<v8::Value>());
	} else {
		return handle_scope.Escape(value);
	}
}

v8::Local<v8::Array> Base_Struct::build_bit_object_vector(Isolate* isolate, const Field_Info &field_info, Bit_Buffer &buffer) {
	EscapableHandleScope handle_scope(isolate);
	if(buffer.read_bits_available() < field_info.field_vbit) {
		LOG_ERROR("bit not enough,available_bit:%d,field_bit:%d,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
			buffer.read_bits_available(),field_info.field_vbit,struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str());
		return handle_scope.Escape(v8::Local<v8::Array>());
    }

	uint32_t length = buffer.read_uint(field_info.field_vbit);
	Local<Context> context(isolate->GetCurrentContext());
	v8::Local<v8::Array> array = v8::Array::New(isolate, length);
	for (uint32_t i = 0; i < length; ++i) {
		if(is_struct(field_info.field_type)) {
			v8::Local<v8::Object> object = build_bit_object_struct(isolate, field_info, buffer);
			if (!object.IsEmpty()) {
				array->Set(context, i, object).FromJust();
			}
			else {
				return handle_scope.Escape(v8::Local<v8::Array>());
			}
		}
		else {
			v8::Local<v8::Value> value = build_bit_object_arg(isolate, field_info, buffer);
			if (!value.IsEmpty()) {
				array->Set(context, i, value).FromJust();
			}
			else {
				return handle_scope.Escape(v8::Local<v8::Array>());
			}
		}
	}
	return handle_scope.Escape(array);
}

v8::Local<v8::Map> Base_Struct::build_bit_object_map(Isolate* isolate, const Field_Info &field_info, Bit_Buffer &buffer) {
	EscapableHandleScope handle_scope(isolate);
	if(buffer.read_bits_available() < field_info.field_vbit) {
		LOG_ERROR("bit not enough,available_bit:%d,field_bit:%d,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
			buffer.read_bits_available(),field_info.field_vbit,struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str());
		return handle_scope.Escape(v8::Local<v8::Map>());
    }

	Field_Info key_info;
	key_info.field_type = field_info.key_type;
	key_info.field_bit = field_info.key_bit;
	key_info.field_name = field_info.key_name;

	uint32_t length = buffer.read_uint(field_info.field_vbit);
	Local<Context> context(isolate->GetCurrentContext());
	v8::Local<v8::Map> map = v8::Map::New(isolate);
	v8::Local<v8::Value> map_key;
	v8::Local<v8::Value> map_value;
	for(uint32_t i = 0; i < length; ++i) {
		//判断map中key的类型，key可以为基本类型或者结构体类型
		if(is_struct(key_info.field_type)) {
			key_info.field_label = "struct";
			map_key = build_bit_object_struct(isolate, key_info, buffer);
		}
		else {
			key_info.field_label = "arg";
			map_key = build_bit_object_arg(isolate, key_info, buffer);
		}

		//判断map中value的类型，value可以为基本类型或者结构体类型
		if(is_struct(field_info.field_type)) {
			map_value = build_bit_object_struct(isolate, field_info, buffer);
		}
		else {
			map_value = build_bit_object_arg(isolate, field_info, buffer);
		}

		if (!map_key.IsEmpty() && !map_value.IsEmpty()) {
			map->Set(context, map_key, map_value).ToLocalChecked();
		}
		else {
			return handle_scope.Escape(v8::Local<v8::Map>());
		}
	}
	return handle_scope.Escape(map);
}

v8::Local<v8::Object>  Base_Struct::build_bit_object_struct(Isolate* isolate, const Field_Info &field_info, Bit_Buffer &buffer) {
	EscapableHandleScope handle_scope(isolate);
	Base_Struct *base_struct = STRUCT_MANAGER->get_base_struct(field_info.field_type);
	if (base_struct == nullptr) {
		return handle_scope.Escape(v8::Local<v8::Object>());
	}

	Local<Context> context(isolate->GetCurrentContext());
	v8::Local<v8::ObjectTemplate> localTemplate = ObjectTemplate::New(isolate);
	v8::Local<v8::Object> object = localTemplate->NewInstance(context).ToLocalChecked();
	build_bit_object(isolate, base_struct->field_vec(), buffer, object);
	return handle_scope.Escape(object);
}

////////////////////////////将v8::object转换bit_buffer///////////////////////
int Base_Struct::build_bit_buffer_arg(Isolate* isolate, const Field_Info &field_info, Bit_Buffer &buffer, v8::Local<v8::Value> value) {
	HandleScope handle_scope(isolate);
	Local<Context> context(isolate->GetCurrentContext());

	int ret = 0;
	if(field_info.field_type == "int") {
		if(value->IsInt32()) {
			int32_t val = value->Int32Value(context).FromJust();
			buffer.write_int(val, field_info.field_bit);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
		}
		else {
			ret = -1;
		}
	}
	else if(field_info.field_type == "uint") {
		if(value->IsUint32()) {
			uint32_t val = value->Uint32Value(context).FromJust();
			buffer.write_uint(val, field_info.field_bit);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
		}
		else {
			ret = -1;
		}
	}
	else if(field_info.field_type == "int64") {
		if(value->IsNumber()) {
			int64_t val = value->NumberValue(context).FromJust();
			buffer.write_int64(val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%ld",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
		}
		else {
			ret = -1;
		}
	}
	else if(field_info.field_type == "uint64") {
		if(value->IsNumber()) {
			uint64_t val = value->NumberValue(context).FromJust();
			buffer.write_uint64(val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%ld",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
		}
		else {
			ret = -1;
		}
	}
	else if(field_info.field_type == "float") {
		if (value->IsNumber()) {
			double val = value->NumberValue(context).FromJust();
			buffer.write_float(val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%f",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
		}
		else {
			ret = -1;
		}
	}
	else if(field_info.field_type == "bool") {
		if (value->IsBoolean()) {
			bool val = value->BooleanValue(context).FromJust();
			buffer.write_bool(val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
		}
		else {
			ret = -1;
		}
	}
	else if(field_info.field_type == "string") {
		if (value->IsString()) {
			String::Utf8Value str(value->ToString(context).ToLocalChecked());
			std::string val = to_string(str);
			buffer.write_str(val.c_str());
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%s",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val.c_str());
			}
		}
		else {
			ret = -1;
		}
	}
	else {
		ret = -1;
	}

	if(ret < 0) {
		LOG_ERROR("field_type not exist or field_value undefined,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
			struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str());
	}
	return ret;
}

int Base_Struct::build_bit_buffer_vector(Isolate* isolate, const Field_Info &field_info, Bit_Buffer &buffer, v8::Local<v8::Value> value) {
	HandleScope handle_scope(isolate);

	if (!value->IsArray()) {
		LOG_ERROR("value is not array,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
			struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str());
		return -1;
	}

	Local<Context> context(isolate->GetCurrentContext());
	v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(value);
	uint32_t length = array->Length();
	buffer.write_uint(length, field_info.field_vbit);
	if(STRUCT_MANAGER->log_trace()) {
		LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_length:%d",
			struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),length);
	}
	for (uint32_t i = 0; i < length; ++i) {
		v8::Local<v8::Value> element = array->Get(context, i).ToLocalChecked();
		if(is_struct(field_info.field_type)) {
			int ret = build_bit_buffer_struct(isolate, field_info, buffer, element);
			if(ret < 0) {
				return ret;
			}
		}
		else {
			int ret = build_bit_buffer_arg(isolate, field_info, buffer, element);
			if(ret < 0) {
				return ret;
			}
		}
	}
	return 0;
}

int Base_Struct::build_bit_buffer_map(Isolate* isolate, const Field_Info &field_info, Bit_Buffer &buffer, v8::Local<v8::Value> value) {
	HandleScope handle_scope(isolate);
	if (!value->IsMap()) {
		LOG_ERROR("value is not map,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
			struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str());
		return -1;
	}

	Field_Info key_info;
	key_info.field_type = field_info.key_type;
	key_info.field_bit = field_info.key_bit;
	key_info.field_name = field_info.key_name;

	Local<Context> context(isolate->GetCurrentContext());
	v8::Local<v8::Map> map = v8::Local<v8::Map>::Cast(value);
	uint32_t length = map->Size();
	buffer.write_uint(length, field_info.field_vbit);
	if(STRUCT_MANAGER->log_trace()) {
		LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_length:%d",
			struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),length);
	}
	v8::Local<v8::Array> array = map->AsArray();
	v8::Local<v8::Value> map_key;
	v8::Local<v8::Value> map_value;
	//index N is the Nth key and index N + 1 is the Nth value
	for (uint32_t i = 0; i < length * 2; i = i + 2) {
		int ret1 = 0;
		int ret2 = 0;
		//判断map中key的类型，key可以为基本类型或者结构体类型
		map_key = array->Get(context, i).ToLocalChecked();
		if(is_struct(key_info.field_type)) {
			key_info.field_label = "struct";
			ret1 = build_bit_buffer_struct(isolate, key_info, buffer, map_key);
		}
		else {
			key_info.field_label = "arg";
			ret1 = build_bit_buffer_arg(isolate, key_info, buffer, map_key);
		}

		//判断map中value的类型，value可以为基本类型或者结构体类型
		map_value = array->Get(context, i + 1).ToLocalChecked();
		if(is_struct(field_info.field_type)) {
			ret2 = build_bit_buffer_struct(isolate, field_info, buffer, map_value);
		}
		else {
			ret2 = build_bit_buffer_arg(isolate, field_info, buffer, map_value);
		}

		if(ret1 < 0 || ret2 < 0) {
			return -1;
		}
	}
	return 0;
}

int Base_Struct::build_bit_buffer_struct(Isolate* isolate, const Field_Info &field_info, Bit_Buffer &buffer, v8::Local<v8::Value> value) {
	HandleScope handle_scope(isolate);
	if (!value->IsObject()) {
		LOG_ERROR("value is not object,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
			struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str());
		return -1;
	}

	Base_Struct *base_struct = STRUCT_MANAGER->get_base_struct(field_info.field_type);
	if (base_struct == nullptr) {
		return -1;
	}

	Local<Context> context(isolate->GetCurrentContext());
	v8::Local<v8::Object> object = value->ToObject(context).ToLocalChecked();
	return build_bit_buffer(isolate, base_struct->field_vec(), buffer, object);
}

////////////////////////////将Bit_Buffer转换Bit_Buffer，根据field_vec_结构///////////////////////
int Base_Struct::build_bit_buffer_arg(const Field_Info &field_info, Bit_Buffer &src_buffer, Bit_Buffer &dst_buffer) {
	int ret = 0;
	if(field_info.field_type == "int") {
		if(src_buffer.read_bits_available() < field_info.field_bit) {
			ret = -1;
        }
		else {
			int32_t val = src_buffer.read_int(field_info.field_bit);
			dst_buffer.write_int(val, field_info.field_bit);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
        }
	}
	else if(field_info.field_type == "uint") {
		if(src_buffer.read_bits_available() < field_info.field_bit) {
			ret = -1;
        }
		else {
			uint32_t val = src_buffer.read_uint(field_info.field_bit);
			dst_buffer.write_uint(val, field_info.field_bit);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
        }
	}
	else if(field_info.field_type == "int64") {
		if(src_buffer.read_bits_available() < 64) {
			ret = -1;
        }
		else {
			int64_t val = src_buffer.read_int64();
			dst_buffer.write_int64(val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%ld",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
        }
	}
	else if(field_info.field_type == "uint64") {
		if(src_buffer.read_bits_available() < 64) {
			ret = -1;
        }
		else {
			uint64_t val = src_buffer.read_uint64();
			dst_buffer.write_uint64(val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%ld",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
        }
	}
	else if(field_info.field_type == "float") {
		if(src_buffer.read_bits_available() < 32) {
			ret = -1;
        }
		else {
			float val = src_buffer.read_float();
			dst_buffer.write_float(val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%f",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
        }
	}
	else if(field_info.field_type == "bool") {
		if(src_buffer.read_bits_available() < 1) {
			ret = -1;
        }
		else {
			bool val = src_buffer.read_bool();
			dst_buffer.write_bool(val);
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%d",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val);
			}
        }
	}
	else if(field_info.field_type == "string") {
		if(src_buffer.read_bits_available() < 8) {		//at least 1 bytes /0
			ret = -1;
        }
		else {
			std::string val = "";
			src_buffer.read_str(val);
			dst_buffer.write_str(val.c_str());
			if(STRUCT_MANAGER->log_trace()) {
				LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_value:%s",
					struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),val.c_str());
			}
        }
	}
	else {
		ret = -1;
	}

	if (ret < 0) {
		LOG_ERROR("bit not enough,available_bit:%d,field_bit:%d,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
			src_buffer.read_bits_available(),field_info.field_vbit,struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str());
	}
	return ret;
}

int Base_Struct::build_bit_buffer_vector(const Field_Info &field_info, Bit_Buffer &src_buffer, Bit_Buffer &dst_buffer) {
	if(src_buffer.read_bits_available() < field_info.field_vbit) {
		LOG_ERROR("bit not enough,available_bit:%d,field_bit:%d,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
			src_buffer.read_bits_available(),field_info.field_vbit,struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str());
		return -1;
	}

	uint32_t length = src_buffer.read_uint(field_info.field_vbit);
	dst_buffer.write_uint(length, field_info.field_vbit);
	if(STRUCT_MANAGER->log_trace()) {
		LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_length:%d",
			struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),length);
	}
	for(uint32_t i = 0; i < length; ++i) {
		int ret = 0;
		if(is_struct(field_info.field_type)) {
			ret = build_bit_buffer_struct(field_info, src_buffer, dst_buffer);
		}
		else{
			ret = build_bit_buffer_arg(field_info, src_buffer, dst_buffer);
		}

		if(ret < 0) {
			return ret;
		}
	}
	return 0;
}

int Base_Struct::build_bit_buffer_map(const Field_Info &field_info, Bit_Buffer &src_buffer, Bit_Buffer &dst_buffer) {
	if(src_buffer.read_bits_available() < field_info.field_vbit) {
		LOG_ERROR("bit not enough,available_bit:%d,field_bit:%d,struct_name:%s,field_label:%s,field_name:%s,field_type:%s",
			src_buffer.read_bits_available(),field_info.field_vbit,struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str());
		return -1;
	}

	Field_Info key_info;
	key_info.field_type = field_info.key_type;
	key_info.field_bit = field_info.key_bit;
	key_info.field_name = field_info.key_name;

	uint32_t length = src_buffer.read_uint(field_info.field_vbit);
	dst_buffer.write_uint(length, field_info.field_vbit);
	if(STRUCT_MANAGER->log_trace()) {
		LOG_INFO("struct_name:%s,field_label:%s,field_name:%s,field_type:%s,field_length:%d",
			struct_name_.c_str(),field_info.field_label.c_str(),field_info.field_name.c_str(),field_info.field_type.c_str(),length);
	}
	for(uint32_t i = 0; i < length; ++i) {
		int ret1 = 0;
		int ret2 = 0;
		//判断map中key的类型，key可以为基本类型或者结构体类型
		if(is_struct(key_info.field_type)) {
			key_info.field_label = "struct";
			ret1 = build_bit_buffer_struct(key_info, src_buffer, dst_buffer);
		}
		else {
			key_info.field_label = "arg";
			ret1 = build_bit_buffer_arg(key_info, src_buffer, dst_buffer);
		}

		//判断map中value的类型，value可以为基本类型或者结构体类型
		if(is_struct(field_info.field_type)) {
			ret1 = build_bit_buffer_struct(field_info, src_buffer, dst_buffer);
		}
		else {
			ret1 = build_bit_buffer_arg(field_info, src_buffer, dst_buffer);
		}

		if(ret1 < 0 || ret2 < 0) {
			return -1;
		}
	}
	return 0;
}

int Base_Struct::build_bit_buffer_struct(const Field_Info &field_info, Bit_Buffer &src_buffer, Bit_Buffer &dst_buffer) {
	Base_Struct *base_struct = STRUCT_MANAGER->get_base_struct(field_info.field_type);
	if(base_struct == nullptr) {
		return -1;
	}

	return build_bit_buffer(base_struct->field_vec(), src_buffer, dst_buffer);
}
