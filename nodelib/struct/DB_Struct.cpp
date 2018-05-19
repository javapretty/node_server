/*
 * DB_Struct.cpp
 *
 *  Created on: Oct 21, 2016
 *      Author: zhangyalei
 */

#include "DB_Struct.h"

DB_Struct::DB_Struct() : 
	Base_Struct(), 	
	db_name_(),
	table_name_(),
	index_name_(),
	index_type_() { }

DB_Struct::~DB_Struct() {
	db_name_.clear();
	table_name_.clear();
	index_name_.clear();
	index_type_.clear();
}

int DB_Struct::init(Xml &xml, TiXmlNode *node) {
	if(node) {
		table_name_ = xml.get_attr_str(node, "table_name");
		db_name_ = table_name_.substr(0, table_name_.find("."));
		index_name_ = xml.get_attr_str(node, "index_name");
		index_type_ = xml.get_attr_str(node, "index_type");
	}

	//Base_Struct初始化放在最后面，因为Base_Struct初始化内部会遍历node节点
	return Base_Struct::init(xml, node);
}