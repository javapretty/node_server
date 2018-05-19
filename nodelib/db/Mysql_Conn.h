/*
 * Mysql_Conn.h
 *
 *  Created on: Jan 4,2016
 *      Author: zhangyalei
 */

#ifndef MYSQL_CONN_H_
#define MYSQL_CONN_H_

#include <list>
#include <string>
#include <unordered_map>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include "Thread_Lock.h"

class DataBuf : public std::streambuf
{
public:
	DataBuf(char* buf, size_t len) {
		setg(buf, buf, buf+len);
    }
};

class Mysql_Pool;
class Mysql_Conn {
public:
	Mysql_Conn(Mysql_Pool* mysql_pool);
	virtual ~Mysql_Conn();

	int init(void);
	sql::PreparedStatement* create_pstmt(const char* str_sql);
	std::string& get_pool_name();

	sql::ResultSet* execute_query(const char* str_sql);
	int execute_update(const char* str_sql);
	int execute(const char* str_sql);

private:
	Mysql_Pool* mysql_pool_;
	sql::Connection*  conn_;
	sql::Statement* stmt_ ;
};

class Mysql_Pool {
public:
	Mysql_Pool(std::string& pool_name,  std::string& server_ip, uint32_t server_port,
			std::string& username,  std::string& password, int32_t max_conn_cnt);
	virtual ~Mysql_Pool();

	int init(void);
	Mysql_Conn* get_mysql_conn();
	void rel_mysql_conn(Mysql_Conn* conn);

	inline std::string& get_pool_name() { return pool_name_; }
	inline std::string& get_server_ip() { return server_ip_; }
	inline int32_t get_server_port() { return server_port_; }
	inline std::string& get_username() { return username_; }
	inline std::string& get_passwrod() { return password_; }
	inline sql::Driver* get_driver(){return driver_;}

private:
	std::string 		pool_name_;
	std::string 		server_ip_;
	int32_t				server_port_;
	std::string 		username_;
	std::string 		password_;
	int32_t				cur_conn_cnt_;
	int32_t 				max_conn_cnt_;

	sql::Driver*  driver_;
	std::list<Mysql_Conn*>	free_list_;
	Notify_Lock	 notify_lock_;
};

class Mysql_Manager {
public:
	static Mysql_Manager* instance();
	int init(std::string& server_ip, int server_port, std::string& username, std::string& password, std::string& pool_name, int max_conn_cnt);

	Mysql_Conn* get_mysql_conn(std::string& pool_name);
	void rel_mysql_conn(Mysql_Conn* conn);

private:
	Mysql_Manager();
	virtual ~Mysql_Manager();

private:
	static Mysql_Manager*	instance_;
	std::unordered_map<std::string, Mysql_Pool*>	 mysql_pool_map_;
};

#define	MYSQL_MANAGER Mysql_Manager::instance()

#endif /* MYSQL_CONN_H_ */
