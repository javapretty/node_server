/*
 * Hot_Update.h
 *
 *  Created on: Nov 21,2016
 *      Author: zhangyalei
 */

#ifndef HOT_UPDATE_H_
#define HOT_UPDATE_H_

#include <vector>
#include <set>
#include <unordered_map>
#include "Thread.h"

class Hot_Update: public Thread {
public:
	typedef std::set<std::string> Md5_Str_Set;
	typedef std::unordered_map<std::string, Md5_Str_Set> Md5_Str_Map;

	Hot_Update();
	virtual ~Hot_Update();

	static Hot_Update *instance(void);
	virtual void run_handler(void);

	int notice_update(const std::string module);
	std::string file_md5(std::string file_name);

	void check_config(std::string module);
	void init_all_module(void);
	void get_md5_str(std::string path, Md5_Str_Set &md5_str_set);

private:
	std::vector<std::string> update_module_;
	Time_Value update_time_;
	static Hot_Update *instance_;
	Md5_Str_Map md5_str_map_;
	Time_Value notify_interval_;
};
#define HOT_UPDATE Hot_Update::instance()

#endif /* HOTUPDATE_H_ */
