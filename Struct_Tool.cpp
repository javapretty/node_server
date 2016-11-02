/*
 * Struct_Tool.cpp
 *  Created on: Oct 29, 2016
 *      Author: zhangyalei
 */

#include "Struct_Manager.h"
#include "Log.h"
#include "Node_Config.h"
#include "Struct_Tool.h"

Struct_Tool::Struct_Tool()
{
	NODE_CONFIG->load_node_config();
	const Json::Value &node_misc = NODE_CONFIG->node_misc();
	for (uint i = 0; i < node_misc["base_struct_path"].size(); ++i) {
		STRUCT_MANAGER->init_struct(node_misc["base_struct_path"][i].asCString(), BASE_STRUCT);
	}
}

Struct_Tool::~Struct_Tool() {

}

int Struct_Tool::write_struct() {
	write_to_struct();
	write_to_message();
	write_to_sql();

	LOG_INFO("write to file success!");
	return 0;
}

int Struct_Tool::write_to_struct() {
	char temp[256] = {};
	sprintf(temp, "js/struct.js");
	FILE *fp = fopen(temp, "w");
	if(fp == NULL){
		LOG_ERROR("open js/struct.js error!");
		return -1;
	}
	for(Struct_Manager::Struct_Name_Map::const_iterator iter = STRUCT_MANAGER->base_struct_name_map().begin();
			iter != STRUCT_MANAGER->base_struct_name_map().end(); iter++){
		Base_Struct *base_struct = iter->second;
		memset(temp, 0, 256);
		sprintf(temp, BEGIN_IMPLEMENT, base_struct->struct_name().c_str());
		fputs(temp, fp);

		for(std::vector<Field_Info>::const_iterator it = base_struct->field_vec().begin();
				it != base_struct->field_vec().end(); it++) {
			Field_Info info = *it;
			memset(temp, 0, 256);
			if(info.field_label == "arg") {
				if(info.field_type == "string") {
					sprintf(temp, RESET_STRING, info.field_name.c_str());
				}
				else {
					sprintf(temp, RESET_ZERO, info.field_name.c_str());
				}
			}
			else if(info.field_label == "vector") {
				sprintf(temp, RESET_VECTOR, info.field_name.c_str());
			}
			else if(info.field_label == "map") {
				sprintf(temp, RESET_MAP, info.field_name.c_str());
			}
			else if(info.field_label == "struct") {
				sprintf(temp, RESET_STRUCT, info.field_name.c_str(), info.field_type.c_str());
			}
			fputs(temp, fp);
		}

		memset(temp, 0, 256);
		sprintf(temp, END_IMPLEMENT);
		fputs(temp, fp);
	}
	fclose(fp);
	return 0;
}

int Struct_Tool::write_to_message() {
	char temp[256] = {};
	sprintf(temp, "js/message.js");
	FILE *fp = fopen(temp, "w");
	if(fp == NULL){
		LOG_ERROR("open js/message.js error!");
		return -1;
	}
	memset(temp, 0, 256);
	sprintf(temp, BEGIN_MESSAGE);
	fputs(temp, fp);
	for(Struct_Manager::Struct_Name_Map::const_iterator iter = STRUCT_MANAGER->base_struct_name_map().begin();
		iter != STRUCT_MANAGER->base_struct_name_map().end(); iter++){
		Base_Struct *base_struct = iter->second;
		if(base_struct->msg_id() > 0) {
			memset(temp, 0, 256);
			sprintf(temp, MESSAGE_BODY, base_struct->msg_name().c_str(), base_struct->msg_id());
			fputs(temp, fp);
		}
	}
	memset(temp, 0, 256);
	sprintf(temp, END_MESSAGE);
	fputs(temp, fp);
	fclose(fp);
	return 0;
}

int Struct_Tool::write_to_sql() {
	system("rm -f ./config/sql/*");
	char temp[256] = {};
	for(Struct_Manager::Struct_Name_Map::const_iterator iter = STRUCT_MANAGER->base_struct_name_map().begin();
		iter != STRUCT_MANAGER->base_struct_name_map().end(); iter++){
		Base_Struct *base_struct = iter->second;
		std::string db_table = base_struct->table_name();
		int npos = db_table.find('.');
		std::string db_name = db_table.substr(0, npos);
		std::string table_name = db_table.substr(npos + 1, db_table.size());
		if(base_struct->table_name() != "") {
			memset(temp, 0, 256);
			sprintf(temp, "config/sql/%s.sql", db_name.c_str());
			FILE *fp = fopen(temp, "rb+");
			if(fp == NULL) {
				fp = fopen(temp, "w");
				memset(temp, 0, 256);
				sprintf(temp, SQL_HEAD, db_name.c_str(), db_name.c_str());
				fputs(temp, fp);
			}
			else {
				fseek(fp, 0, SEEK_END);
			}
			memset(temp, 0, 256);
			sprintf(temp, TABLE_HEAD, table_name.c_str(), table_name.c_str());
			fputs(temp, fp);
			for(std::vector<Field_Info>::const_iterator it = base_struct->field_vec().begin();
					it != base_struct->field_vec().end(); it++) {
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
			memset(temp, 0, 256);
			sprintf(temp, PRIMARY_KEY, base_struct->index_name().c_str());
			fputs(temp, fp);
			memset(temp, 0, 256);
			sprintf(temp, TABLE_END);
			fputs(temp, fp);
			fclose(fp);
		}
	}
	return 0;
}

