/*
 * Struct_Tool.cpp
 *  Created on: Oct 29, 2016
 *      Author: zhangyalei
 */

#include "Log.h"
#include "Struct_Manager.h"
#include "Struct_Tool.h"

Struct_Tool::Struct_Tool()
{
	Xml xml;
	bool ret = xml.load_xml("./config/node/node_conf.xml");
	if(ret < 0) {
		LOG_FATAL("load config:node_conf.xml abort");
	}
	TiXmlNode* struct_node = xml.get_root_node("struct");
	if(struct_node) {
		TiXmlNode* child_node = xml.enter_node(struct_node, "struct");
		if(child_node) {
			XML_LOOP_BEGIN(child_node)
				std::string key = xml.get_key(child_node);
				if(key == "base_struct") {
					STRUCT_MANAGER->init_struct(xml.get_attr_str(child_node, "path").c_str(), BASE_STRUCT);
				}
				else if(key == "msg_struct") {
					STRUCT_MANAGER->init_struct(xml.get_attr_str(child_node, "path").c_str(), MSG_STRUCT);
				}
				else if(key == "db_struct") {
					STRUCT_MANAGER->init_struct(xml.get_attr_str(child_node, "path").c_str(), DB_STRUCT);
				}
			XML_LOOP_END(child_node)
		}
	}
}

Struct_Tool::~Struct_Tool() {

}

int Struct_Tool::write_struct() {
	write_to_struct();
	write_to_message();
	write_to_sql();
	return 0;
}

int Struct_Tool::write_to_struct() {
	char temp[512] = {};
	sprintf(temp, "js/struct.js");
	FILE *fp = fopen(temp, "w");
	if(fp == NULL){
		LOG_ERROR("open js/struct.js error!");
		return -1;
	}

	for(Struct_Manager::Struct_Name_Map::const_iterator iter = STRUCT_MANAGER->base_struct_name_map().begin();
			iter != STRUCT_MANAGER->base_struct_name_map().end(); iter++) {
		//写结构体头部
		memset(temp, 0, 512);
		sprintf(temp, BEGIN_IMPLEMENT, iter->second->struct_name().c_str());
		fputs(temp, fp);
		//写结构体内容
		field_name_set.clear();
		field_define_vec.clear();
		write_field_struct(iter->second->field_vec());
		for(Field_Define_Vec::iterator it = field_define_vec.begin(); it != field_define_vec.end(); it++) {
			fputs((*it).c_str(), fp);
		}
		//写结构体尾部
		memset(temp, 0, 512);
		sprintf(temp, END_IMPLEMENT);
		fputs(temp, fp);
	}
	fclose(fp);
	return 0;
}

int Struct_Tool::write_field_struct(const Field_Vec &field_vec) {
	char temp[512] = {};
	for(Field_Vec::const_iterator iter = field_vec.begin();
			iter != field_vec.end(); iter++) {
		memset(temp, 0, 512);
		if(iter->field_label == "arg") {
			if(iter->field_type == "string") {
				sprintf(temp, RESET_STRING, iter->field_name.c_str());
			}
			else if(iter->field_type == "bool" ){
				sprintf(temp, RESET_BOOL, iter->field_name.c_str());
			}
			else {
				sprintf(temp, RESET_NUMBER, iter->field_name.c_str());
			}
		}
		else if(iter->field_label == "vector") {
			sprintf(temp, RESET_VECTOR, iter->field_name.c_str());
		}
		else if(iter->field_label == "map") {
			sprintf(temp, RESET_MAP, iter->field_name.c_str());
		}
		else if(iter->field_label == "struct") {
			sprintf(temp, RESET_STRUCT, iter->field_name.c_str(), iter->field_type.c_str());
		}
		else if(iter->field_label == "if") {
			sprintf(temp, RESET_BOOL, iter->field_name.c_str());
			write_field_struct(iter->children);
		}
		else if(iter->field_label == "switch") {
			sprintf(temp, RESET_NUMBER, iter->field_name.c_str());
			write_field_struct(iter->children);
		}
		else if(iter->field_label == "case") {
			write_field_struct(iter->children);
		}

		if (field_name_set.count(iter->field_name) <= 0) {
			field_name_set.insert(iter->field_name);
			field_define_vec.push_back(temp);
		}
	}
	return 0;
}

int Struct_Tool::write_to_message() {
	char temp[512] = {};
	sprintf(temp, "js/message.js");
	FILE *fp = fopen(temp, "w");
	if(fp == NULL) {
		LOG_ERROR("open js/message.js error!");
		return -1;
	}
	memset(temp, 0, 512);
	sprintf(temp, BEGIN_MESSAGE);
	fputs(temp, fp);

	for(Struct_Manager::Struct_Name_Map::const_iterator iter = STRUCT_MANAGER->msg_struct_name_map().begin();
		iter != STRUCT_MANAGER->msg_struct_name_map().end(); iter++) {
		Msg_Struct *msg_struct = dynamic_cast<Msg_Struct *>(iter->second);
		if(msg_struct && msg_struct->msg_id() > 0) {
			memset(temp, 0, 512);
			sprintf(temp, MESSAGE_BODY, msg_struct->msg_name().c_str(), msg_struct->msg_id());
			fputs(temp, fp);
		}
	}
	memset(temp, 0, 512);
	sprintf(temp, END_MESSAGE);
	fputs(temp, fp);
	fclose(fp);
	return 0;
}

int Struct_Tool::write_to_sql() {
	system("rm -f ./config/sql/*");
	char temp[512] = {};
	for(Struct_Manager::Struct_Name_Map::const_iterator iter = STRUCT_MANAGER->db_struct_name_map().begin();
		iter != STRUCT_MANAGER->db_struct_name_map().end(); iter++){
		DB_Struct *db_struct = dynamic_cast<DB_Struct *>(iter->second);
		if(db_struct && db_struct->table_name() != "") {
			std::string db_table = db_struct->table_name();
			int npos = db_table.find('.');
			std::string db_name = db_table.substr(0, npos);
			std::string table_name = db_table.substr(npos + 1, db_table.size());

			//写入表头
			memset(temp, 0, 512);
			sprintf(temp, "config/sql/%s.sql", db_name.c_str());
			FILE *fp = fopen(temp, "rb+");
			if(fp == NULL) {
				fp = fopen(temp, "w");
				memset(temp, 0, 512);
				sprintf(temp, SQL_HEAD, db_name.c_str(), db_name.c_str());
				fputs(temp, fp);
			}
			else {
				fseek(fp, 0, SEEK_END);
			}
			memset(temp, 0, 512);
			sprintf(temp, TABLE_HEAD, table_name.c_str(), table_name.c_str());
			fputs(temp, fp);
			//写入表字段
			for(Field_Vec::const_iterator it = db_struct->field_vec().begin();
					it != db_struct->field_vec().end(); it++) {
				Field_Info info = *it;
				if(info.field_label == "arg") {
					if(info.field_type == "int64" || info.field_type == "uint64") {
						sprintf(temp, SQL_BIGINT, info.field_name.c_str());
					}
					else if(info.field_type == "bool") {
						sprintf(temp, SQL_INT_2, info.field_name.c_str());
					}
					else if(info.field_type == "string") {
						sprintf(temp, SQL_VARCHAR, info.field_name.c_str());
					}
					else {
						sprintf(temp, SQL_INT_11, info.field_name.c_str());
					}
				}
				else if(info.field_label == "vector" ||
						info.field_label == "map" ||
						info.field_label == "struct") {
					sprintf(temp, SQL_TEXT, info.field_name.c_str());
				}
				fputs(temp, fp);
			}
			memset(temp, 0, 512);
			sprintf(temp, PRIMARY_KEY, db_struct->index_name().c_str());
			fputs(temp, fp);
			memset(temp, 0, 512);
			sprintf(temp, TABLE_END);
			fputs(temp, fp);
			fclose(fp);
		}
	}
	return 0;
}
