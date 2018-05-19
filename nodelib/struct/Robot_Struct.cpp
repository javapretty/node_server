/*
 * Robot_Struct.cpp
 *
 *  Created on: Sep 27, 2016
 *      Author: zhangyalei
 */

#include "Struct_Manager.h"
#include "Robot_Struct.h"

Robot_Struct::Robot_Struct() : 
	Base_Struct(),
	msg_id_(0),
	msg_name_() {}

Robot_Struct::~Robot_Struct() {
	msg_id_ = 0;
	msg_name_.clear();
}

int Robot_Struct::init(Xml &xml, TiXmlNode *node) {
	if(node) {
		msg_id_ = xml.get_attr_int(node, "msg_id");
		msg_name_ = xml.get_attr_str(node, "msg_name");
	}

	//Base_Struct初始化放在最后面，因为Base_Struct初始化内部会遍历node节点
	return Base_Struct::init(xml, node);
}

void Robot_Struct::write_bit_buffer(const Field_Vec &field_vec, Bit_Buffer &buffer) {
	for(Field_Vec::const_iterator iter = field_vec.begin(); iter != field_vec.end(); iter++) {
		if((*iter).field_label == "arg") {
			write_bit_buffer_arg((*iter), buffer);
		}
		else if((*iter).field_label == "vector") {
			write_bit_buffer_vector((*iter), buffer);
		}
		else if((*iter).field_label == "map") {
			write_bit_buffer_map((*iter), buffer);
		}
		else if((*iter).field_label == "struct") {
			write_bit_buffer_struct((*iter), buffer);
		}
		else if((*iter).field_label == "if") {
			buffer.write_bool(true);
			write_bit_buffer(iter->children, buffer);
		}
		else if((*iter).field_label == "switch") {
			uint case_val = 1;
			buffer.write_uint(case_val, iter->field_bit);

			for(uint i = 0; i < iter->children.size(); ++i) {
				if(iter->children[i].field_label == "case" && iter->children[i].case_val == case_val) {
					//找到对应的case标签，对case标签内的child数组进行build
					write_bit_buffer(iter->children[i].children, buffer);
					break;
				}
			}
		}
	}
}

void Robot_Struct::read_bit_buffer(const Field_Vec &field_vec, Bit_Buffer &buffer) {
	for(Field_Vec::const_iterator iter = field_vec.begin(); iter != field_vec.end(); iter++) {
		if((*iter).field_label == "arg") {
			read_bit_buffer_arg((*iter), buffer);
		}
		else if((*iter).field_label == "vector") {
			read_bit_buffer_vector((*iter), buffer);
		}
		else if((*iter).field_label == "map") {
			read_bit_buffer_map((*iter), buffer);
		}
		else if((*iter).field_label == "struct") {
			read_bit_buffer_struct((*iter), buffer);
		}
		else if((*iter).field_label == "if") {
			if(buffer.read_bits_available() >= 1 && buffer.read_bool()) {
				read_bit_buffer(iter->children, buffer);
		    }
		}
		else if((*iter).field_label == "switch") {
			if (buffer.read_bits_available() >= iter->field_bit) {
				uint case_val = buffer.read_uint(iter->field_bit);
				for(uint i = 0; i < iter->children.size(); ++i) {
					if(iter->children[i].field_label == "case" && iter->children[i].case_val == case_val) {
						//找到对应的case标签，对case标签内的child数组进行build
						read_bit_buffer(iter->children[i].children, buffer);
						break;
					}
				}
			}
		}
	}
}

//////////////////////////////////读写bit_buffer///////////////////////////
void Robot_Struct::write_bit_buffer_arg(const Field_Info &field_info, Bit_Buffer &buffer) {
	if(field_info.field_type == "int") {
		buffer.write_int(0, field_info.field_bit);
	}
	else if(field_info.field_type == "uint") {
		buffer.write_uint(0, field_info.field_bit);
	}
	else if(field_info.field_type == "int64") {
		buffer.write_int64(0);
	}
	else if(field_info.field_type == "uint64") {
		buffer.write_uint64(0);
	}
	else if(field_info.field_type == "float") {
		buffer.write_float(0.0f);
	}
	else if(field_info.field_type == "bool") {
		buffer.write_bool(false);
	}
	else if(field_info.field_type == "string") {
		std::stringstream stream;
		int rand_num = random() % 1000000;
		stream << "name_" << rand_num;
		buffer.write_str(stream.str().c_str());
	}
	else {
		LOG_ERROR("Can not find the field_type:%s, field_name:%s, struct_name:%s",
				field_info.field_type.c_str(), field_info.field_name.c_str(), struct_name().c_str());
	}
}

void Robot_Struct::write_bit_buffer_vector(const Field_Info &field_info, Bit_Buffer &buffer) {
	buffer.write_uint(0, field_info.field_vbit);
}

void Robot_Struct::write_bit_buffer_map(const Field_Info &field_info, Bit_Buffer &buffer) {
	buffer.write_uint(0, field_info.field_vbit);
}

void Robot_Struct::write_bit_buffer_struct(const Field_Info &field_info, Bit_Buffer &buffer) {
	Robot_Struct *robot_struct = STRUCT_MANAGER->get_robot_struct(field_info.field_type);
	if (robot_struct == NULL) {
		LOG_ERROR("get_robot_struct, struct_name %s not found!", field_info.field_type.c_str());
		return;
	}

	write_bit_buffer(robot_struct->field_vec(), buffer);
}

void Robot_Struct::read_bit_buffer_arg(const Field_Info &field_info, Bit_Buffer &buffer) {
	bool ret = true;
	if(field_info.field_type == "int") {
		if(buffer.read_bits_available() < field_info.field_bit) {
			ret = false;
        }
		else {
			int value = buffer.read_int(field_info.field_bit);
			LOG_INFO("struct_name:%s, field_name:%s, field_value:%d", struct_name().c_str(), field_info.field_name.c_str(), value);
        }
	}
	else if(field_info.field_type == "uint") {
		if(buffer.read_bits_available() < field_info.field_bit) {
			ret = false;
        }
		else {
			uint value = buffer.read_uint(field_info.field_bit);
			LOG_INFO("struct_name:%s, field_name:%s, field_value:%d", struct_name().c_str(), field_info.field_name.c_str(), value);
        }
	}
	else if(field_info.field_type == "int64") {
		if(buffer.read_bits_available() < 64) {
			ret = false;
        }
		else {
			int64_t value = buffer.read_int64();
			LOG_INFO("struct_name:%s, field_name:%s, field_value:%ld", struct_name().c_str(), field_info.field_name.c_str(), value);
        }
	}
	else if(field_info.field_type == "uint64") {
		if(buffer.read_bits_available() < 64) {
			ret = false;
        }
		else {
			uint64_t value = buffer.read_uint64();
			LOG_INFO("struct_name:%s, field_name:%s, field_value:%ld", struct_name().c_str(), field_info.field_name.c_str(), value);
        }
	}
	else if(field_info.field_type == "float") {
		if(buffer.read_bits_available() < 32) {
			ret = false;
        }
		else {
			float value = buffer.read_float();
			LOG_INFO("struct_name:%s, field_name:%s, field_value:%f", struct_name().c_str(), field_info.field_name.c_str(), value);
        }
	}
	else if(field_info.field_type == "bool") {
		if(buffer.read_bits_available() < 1) {
			ret = false;
        }
		else {
			bool value = buffer.read_bool();
			LOG_INFO("struct_name:%s, field_name:%s, field_value:%d", struct_name().c_str(), field_info.field_name.c_str(), value);
        }
	}
	else if(field_info.field_type == "string") {
		if(buffer.read_bits_available() < 8) {		//at least 1 bytes /0
			ret = false;
        }
		else {
			std::string value;
			buffer.read_str(value);
			LOG_INFO("struct_name:%s, field_name:%s, field_value:%s", struct_name().c_str(), field_info.field_name.c_str(), value.c_str());
        }
	}
	else {
		ret = false;
	}

	if (!ret) {
		LOG_ERROR("buffer bit unavailable, field_type:%s, field_name:%s, struct_name:%s",
				field_info.field_type.c_str(), field_info.field_name.c_str(), struct_name().c_str());
	}
}

void Robot_Struct::read_bit_buffer_vector(const Field_Info &field_info, Bit_Buffer &buffer) {
	int length = buffer.read_uint(field_info.field_vbit);
	LOG_INFO("struct_name:%s, field_name:%s, length:%d", struct_name().c_str(), field_info.field_name.c_str(), length);

	for(uint16_t i = 0; i < length; ++i) {
		if(is_struct(field_info.field_type)) {
			read_bit_buffer_struct(field_info, buffer);
		}
		else{
			read_bit_buffer_arg(field_info, buffer);
		}
	}
}

void Robot_Struct::read_bit_buffer_map(const Field_Info &field_info, Bit_Buffer &buffer) {
	int length = buffer.read_uint(field_info.field_vbit);
	LOG_INFO("struct_name:%s, field_name:%s, length:%d", struct_name().c_str(), field_info.field_name.c_str(), length);

	Field_Info key_info;
	key_info.field_type = field_info.key_type;
	key_info.field_bit = field_info.key_bit;
	key_info.field_name = field_info.key_name;
	for(uint16_t i = 0; i < length; ++i) {
		if(is_struct(key_info.field_type)) {
			key_info.field_label = "struct";
			read_bit_buffer_struct(key_info, buffer);
		}
		else {
			key_info.field_label = "arg";
			read_bit_buffer_arg(key_info, buffer);
		}

		if(is_struct(field_info.field_type)) {
			read_bit_buffer_struct(field_info, buffer);
		}
		else{
			read_bit_buffer_arg(field_info, buffer);
		}
	}
}

void Robot_Struct::read_bit_buffer_struct(const Field_Info &field_info, Bit_Buffer &buffer) {
	Robot_Struct *robot_struct = STRUCT_MANAGER->get_robot_struct(field_info.field_type);
	if (robot_struct == NULL) {
		LOG_ERROR("get_robot_struct, struct_name %s not found!", field_info.field_type.c_str());
		return;
	}

	LOG_INFO("struct_name:%s, field_type:%s, field_name:%s", struct_name().c_str(), field_info.field_type.c_str(), field_info.field_name.c_str());
	read_bit_buffer(robot_struct->field_vec(), buffer);
}
