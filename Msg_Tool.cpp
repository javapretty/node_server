#include "Msg_Tool.h"
#include "Node_Config.h"

Msg_Tool::Msg_Tool()
{
	NODE_CONFIG->load_node_config();
	const Json::Value &node_misc = NODE_CONFIG->node_misc();
	for (uint i = 0; i < node_misc["msg_struct_path"].size(); ++i) {
		STRUCT_MANAGER->init_struct(node_misc["msg_struct_path"][i].asCString(), MSG_STRUCT);
	}
}

Msg_Tool::~Msg_Tool() {

}

int Msg_Tool::write_struct() {
	char temp[64] = {};
	sprintf(temp, "js/struct.js");
	FILE *fp = fopen(temp, "w");
	if(fp == NULL){
		LOG_ERROR("open js/struct.js error!");
		return -1;
	}
	write_to_struct(fp);
	fclose(fp);

	memset(temp, 0, 64);
	sprintf(temp, "js/message.js");
	fp = fopen(temp, "w");
	if(fp == NULL){
		LOG_ERROR("open js/message.js error!");
		return -1;
	}
	write_to_message(fp);
	fclose(fp);

	return 0;
}

int Msg_Tool::write_to_struct(FILE *fp) {
	Struct_Manager::Struct_Name_Map msg_struct_map = STRUCT_MANAGER->msg_struct_name_map();
	char temp[256] = {};
	for(Struct_Manager::Struct_Name_Map::iterator iter = msg_struct_map.begin();
			iter != msg_struct_map.end(); iter++){
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
			else if(info.field_label == "map" ||
					info.field_label == "unordered_map") {
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
	return 0;
}

int Msg_Tool::write_to_message(FILE *fp) {
	Struct_Manager::Struct_Name_Map msg_struct_map = STRUCT_MANAGER->msg_struct_name_map();
	char temp[256] = {};
	memset(temp, 0, 256);
	sprintf(temp, BEGIN_MESSAGE);
	fputs(temp, fp);
	for(Struct_Manager::Struct_Name_Map::iterator iter = msg_struct_map.begin();
		iter != msg_struct_map.end(); iter++){
		Base_Struct *base_struct = iter->second;
		if(base_struct->msg_id() > 0) {
			LOG_DEBUG("proc %s, msg_id is %d", base_struct->struct_name().c_str(), base_struct->msg_id());
			memset(temp, 0, 256);
			sprintf(temp, MESSAGE_BODY, base_struct->msg_name().c_str(), base_struct->msg_id());
			fputs(temp, fp);
		}
	}
	memset(temp, 0, 256);
	sprintf(temp, END_MESSAGE);
	fputs(temp, fp);
	return 0;
}

