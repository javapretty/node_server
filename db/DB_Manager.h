/*
 * DB_Manager.h
 *
 *  Created on: Oct 27, 2016
 *      Author: zhangyalei
 */

#ifndef DB_MANAGER_H_
#define DB_MANAGER_H_

#include "boost/unordered_map.hpp"
#include "DB_Operator.h"

class DB_Manager {
public:
	typedef boost::unordered_map<int, DB_Operator *> DB_Operator_Map;
public:
	static DB_Manager *instance(void);

	int init_db_operator();
	DB_Operator *db_operator(int type);

private:
	DB_Manager(void);
	virtual ~DB_Manager(void);
	DB_Manager(const DB_Manager &);
	const DB_Manager &operator=(const DB_Manager &);

private:
	static DB_Manager *instance_;
	DB_Operator_Map db_operator_map_;
};

#define DB_MANAGER 	DB_Manager::instance()
#define DB_OPERATOR(db_id) DB_MANAGER->db_operator((db_id))

#endif /* DB_MANAGER_H_ */
