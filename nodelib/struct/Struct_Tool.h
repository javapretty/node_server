/*
 * Struct_Tool.h
 *  Created on: Oct 29, 2016
 *      Author: zhangyalei
 */

#ifndef STRUCT_TOOL_
#define STRUCT_TOOL_

#include <stdlib.h>
#include <unordered_set>
#include "Base_Define.h"

#define BEGIN_IMPLEMENT "\nfunction %s() {\n"
#define END_IMPLEMENT "}\n"
#define RESET_NUMBER "\tthis.%s = 0;\n"
#define RESET_BOOL "\tthis.%s = false;\n"
#define RESET_STRING "\tthis.%s = \"\";\n"
#define RESET_VECTOR "\tthis.%s = new Array();\n"
#define RESET_MAP "\tthis.%s = new Map();\n"
#define RESET_STRUCT "\tthis.%s = new %s();\n"
#define BEGIN_MESSAGE "if (typeof Msg == \"undefined\") {\n"\
										"\tvar Msg = {};\n"
#define MESSAGE_BODY "\tMsg.%s = %d;\n"
#define END_MESSAGE END_IMPLEMENT

#define SQL_HEAD "CREATE DATABASE IF NOT EXISTS `%s` DEFAULT CHARACTER SET utf8;\n"\
								"USE %s\n"
#define TABLE_HEAD "\nDROP TABLE IF EXISTS `%s`;\n"\
									"CREATE TABLE `%s` (\n"
#define TABLE_END ")ENGINE=InnoDB DEFAULT CHARSET=utf8;\n"
#define SQL_INT_11 "\t%s int(11) NOT NULL default '0',\n"
#define SQL_INT_2 "\t%s int(2) NOT NULL default '0',\n"
#define SQL_BIGINT "\t%s bigint(20) NOT NULL default '0',\n"
#define SQL_VARCHAR "\t%s varchar(120) NOT NULL default '',\n"
#define SQL_TEXT "\t%s text NOT NULL,\n"
#define PRIMARY_KEY "\tPRIMARY KEY (%s)\n"

class Struct_Tool {
	typedef std::unordered_set<std::string> Field_Name_Set;
	typedef std::vector<std::string> Field_Define_Vec;
public:
	Struct_Tool();
	~Struct_Tool();
	int write_struct();

private:
	int write_to_struct();
	int write_field_struct(const Field_Vec &field_vec);
	int write_to_message();
	int write_to_sql();

private:
	//将结构体名字存在set，防止if/switch相同结构体重复生成
	Field_Name_Set field_name_set;
	Field_Define_Vec field_define_vec;
};

#endif
