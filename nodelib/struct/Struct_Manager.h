/*
 * Struct_Manager.h
 *
 *  Created on: Aug 4, 2016
 *      Author: zhangyalei
 */

#ifndef STRUCT_MANAGER_H_
#define STRUCT_MANAGER_H_

#include <unordered_map>
#include "DB_Struct.h"
#include "Msg_Struct.h"
#include "Robot_Struct.h"

class Struct_Manager {
public:
	typedef std::unordered_map<std::string, Base_Struct *> Struct_Name_Map;
public:
	static Struct_Manager *instance(void);

	int init_struct(const char *file_path, int struct_type);

	Base_Struct *get_base_struct(const std::string &struct_name);
	Msg_Struct *get_msg_struct(const std::string &struct_name);
	Robot_Struct *get_robot_struct(const std::string &struct_name);
	DB_Struct *get_db_struct(const std::string &struct_name);

	inline const Struct_Name_Map &base_struct_name_map(void) { return base_struct_name_map_; }
	inline const Struct_Name_Map &msg_struct_name_map(void) { return msg_struct_name_map_; }
	inline const Struct_Name_Map &robot_struct_name_map(void) { return robot_struct_name_map_; }
	inline const Struct_Name_Map &db_struct_name_map(void) { return db_struct_name_map_; }

	inline void set_log_trace(bool log_trace) { log_trace_ = log_trace; }
	inline bool log_trace(void) { return log_trace_; }
	inline void set_agent_num(int agent_num) { agent_num_ = agent_num; }
	inline int agent_num() { return agent_num_; }
	inline void set_server_num(int server_num) { server_num_ = server_num; }
	inline int server_num() { return server_num_; }

private:
	int load_struct(const char *file_path, int struct_type, Struct_Name_Map &struct_name_map);

private:
	Struct_Manager(void);
	virtual ~Struct_Manager(void);
	Struct_Manager(const Struct_Manager &);
	const Struct_Manager &operator=(const Struct_Manager &);

private:
	static Struct_Manager *instance_;

	bool log_trace_;	//是否打印日志跟踪struct
	int agent_num_;		//代理编号
	int server_num_;	//服务器编号

	Struct_Name_Map base_struct_name_map_;
	Struct_Name_Map msg_struct_name_map_;
	Struct_Name_Map robot_struct_name_map_;
	Struct_Name_Map db_struct_name_map_;
};

#define STRUCT_MANAGER 	Struct_Manager::instance()

#endif /* STRUCT_MANAGER_H_ */
