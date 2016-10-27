/*
 * DB_Manager.h
 *
 *  Created on: Dec 26, 2016
 *      Author: lijunliang
 */

#ifndef DB_MANAGER_H_
#define DB_MANAGER_H_

#include "Block_Buffer.h"
#include "boost/unordered_map.hpp"

class DB_Manager {
public:
	typedef boost::unordered_map<int64_t, Block_Buffer *> TABLE_BUFFER_MAP;
	typedef boost::unordered_map<const char *, TABLE_BUFFER_MAP *> DB_BUFFER_MAP;
	
	static DB_Manager *instance(void);

	void save_data(const char *table, int64_t index, Block_Buffer *buffer);
	Block_Buffer *load_data(const char *table, int64_t index);

private:
	DB_Manager(void);
	virtual ~DB_Manager(void);
	DB_Manager(const DB_Manager &);
	const DB_Manager &operator=(const DB_Manager &);

private:
	static DB_Manager *instance_;
	DB_BUFFER_MAP db_buffer_map_;
};

#define DB_MANAGER DB_Manager::instance()

#endif /* DB_MANAGER_H_ */

