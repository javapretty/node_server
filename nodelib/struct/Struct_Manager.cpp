/*
 * Struct_Manager.cpp
 *
 *  Created on: Aug 4, 2016
 *      Author: zhangyalei
 */

#include "Base_Enum.h"
#include "Base_Function.h"
#include "Msg_Struct.h"
#include "Robot_Struct.h"
#include "Struct_Manager.h"

Struct_Manager::Struct_Manager(void) :
	agent_num_(0),
	server_num_(0),
	base_struct_name_map_(get_hash_table_size(8192)),
	msg_struct_name_map_(get_hash_table_size(8192)),
	robot_struct_name_map_(get_hash_table_size(8192)),
	db_struct_name_map_(get_hash_table_size(8192))
{ }

Struct_Manager::~Struct_Manager(void) { 
	for(Struct_Name_Map::iterator iter = base_struct_name_map_.begin(); iter != base_struct_name_map_.end(); ++iter) {
		if(iter->second) {
			delete iter->second;
		}
	}
	base_struct_name_map_.clear();

	for(Struct_Name_Map::iterator iter = msg_struct_name_map_.begin(); iter != msg_struct_name_map_.end(); ++iter) {
		if(iter->second) {
			delete iter->second;
		}
	}
	msg_struct_name_map_.clear();

	for(Struct_Name_Map::iterator iter = robot_struct_name_map_.begin(); iter != robot_struct_name_map_.end(); ++iter) {
		if(iter->second) {
			delete iter->second;
		}
	}
	robot_struct_name_map_.clear();

	for(Struct_Name_Map::iterator iter = db_struct_name_map_.begin(); iter != db_struct_name_map_.end(); ++iter) {
		if(iter->second) {
			delete iter->second;
		}
	}
	db_struct_name_map_.clear();
}

Struct_Manager *Struct_Manager::instance_;

Struct_Manager *Struct_Manager::instance(void) {
	if (instance_ == 0)
		instance_ = new Struct_Manager;
	return instance_;
}

int Struct_Manager::init_struct(const char *file_path, int struct_type) {
	switch(struct_type) {
	case BASE_STRUCT:
		load_struct(file_path, struct_type, base_struct_name_map_);
		break;
	case MSG_STRUCT:
		load_struct(file_path, struct_type, msg_struct_name_map_);
		break;
	case ROBOT_STRUCT:
		load_struct(file_path, struct_type, robot_struct_name_map_);
		break;
	case DB_STRUCT:
		load_struct(file_path, struct_type, db_struct_name_map_);
		break;
	default:
		LOG_FATAL("init_struct struct_type = %d abort", struct_type);
	}
	return 0;
}

Base_Struct *Struct_Manager::get_base_struct(const std::string &struct_name) {
	Struct_Name_Map::iterator iter = base_struct_name_map_.find(struct_name);
	if(iter != base_struct_name_map_.end()) {
		return iter->second;
	}
	LOG_ERROR("get_base_struct, struct_name:%s not found!", struct_name.c_str());
	return nullptr;
}

Msg_Struct *Struct_Manager::get_msg_struct(const std::string &struct_name) {
	Struct_Name_Map::iterator iter = msg_struct_name_map_.find(struct_name);
	if(iter != msg_struct_name_map_.end()) {
		return dynamic_cast<Msg_Struct*>(iter->second);
	}
	return nullptr;
}

Robot_Struct *Struct_Manager::get_robot_struct(const std::string &struct_name) {
	Struct_Name_Map::iterator iter = robot_struct_name_map_.find(struct_name);
	if(iter != robot_struct_name_map_.end()) {
		return dynamic_cast<Robot_Struct*>(iter->second);
	}
	return nullptr;
}

DB_Struct *Struct_Manager::get_db_struct(const std::string &struct_name) {
	Struct_Name_Map::iterator iter = db_struct_name_map_.find(struct_name);
	if(iter != db_struct_name_map_.end()) {
		return dynamic_cast<DB_Struct*>(iter->second);
	}
	LOG_ERROR("get_db_struct, struct_name:%s not found!", struct_name.c_str());
	return nullptr;
}

int Struct_Manager::load_struct(const char *file_path, int struct_type, Struct_Name_Map &struct_name_map){
	Xml xml;
	xml.load_xml(file_path);
	TiXmlNode *node = xml.enter_root_node();
	if(node == nullptr) {
		LOG_FATAL("load_struct abort, struct_type:%d, file_path:%s", struct_type, file_path);
		return -1;
	}
	XML_LOOP_BEGIN(node)
		Base_Struct *base_struct = nullptr;
		if (struct_type == BASE_STRUCT) {
			base_struct = new Base_Struct();
		}
		else if (struct_type == MSG_STRUCT) {
			base_struct = new Msg_Struct();
		}
		else if (struct_type == ROBOT_STRUCT) {
			base_struct = new Robot_Struct();
		}
		else if (struct_type == DB_STRUCT)	{
			base_struct = new DB_Struct();
		}

		base_struct->init(xml, node);
		struct_name_map.insert(std::pair<std::string, Base_Struct*>(base_struct->struct_name(), base_struct));
	XML_LOOP_END(node)
	return 0;
}
