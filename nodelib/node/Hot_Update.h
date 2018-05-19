/*
 * Hot_Update.h
 *
 *  Created on: Nov 21,2016
 *      Author: zhangyalei
 */

#ifndef HOT_UPDATE_H_
#define HOT_UPDATE_H_

#include <vector>
#include <unordered_map>
#include "Thread.h"

class Hot_Update: public Thread {
	typedef std::unordered_map<std::string, std::string> Md5_Str_Map;	//file_path--md5_str
public:
	Hot_Update();
	virtual ~Hot_Update();

	static Hot_Update *instance(void);
	virtual void run_handler(void);
	int init(const std::vector<std::string> &folder_list);

private:
	int init_file_md5(const std::string folder_path);
	std::string calc_file_md5(const std::string &file_path);

private:
	static Hot_Update *instance_;
	Md5_Str_Map md5_str_map_;
};

#define HOT_UPDATE Hot_Update::instance()

#endif /* HOTUPDATE_H_ */
