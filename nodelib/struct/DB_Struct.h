/*
 * DB_Struct.h
 *
 *  Created on: Oct 21, 2016
 *      Author: zhangyalei
 */

#ifndef DB_STRUCT_H_
#define DB_STRUCT_H_

#include "Base_Struct.h"

class DB_Struct: public Base_Struct {
public:
	DB_Struct();
	virtual ~DB_Struct();

	virtual int init(Xml &xml, TiXmlNode *node);

	inline const std::string &table_name() { return table_name_; }
	inline const std::string &db_name() { return db_name_; }
	inline const std::string &index_name() { return index_name_; }
	inline const std::string &index_type() { return index_type_; }

private:
	std::string db_name_;
	std::string table_name_;
	std::string index_name_;
	std::string index_type_;
};

#endif /* DB_STRUCT_H_ */
