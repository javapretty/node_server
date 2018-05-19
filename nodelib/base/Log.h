/*
 * Log.h
 *
 *  Created on: Dec 16,2015
 *      Author: zhangyalei
 *
 */

#ifndef LOG_H_
#define LOG_H_

#include <cstdarg>
#include <string>
#include "List.h"
#include "Thread.h"
#include "Thread_Guard.h"
#include "Time_Value.h"

struct LOG_File_Info {
	LOG_File_Info(void): tv(Time_Value::zero), fp(0)	{ }

	Time_Value tv;
	std::string filepath;
	FILE *fp;
};

class Log: public Thread {
	typedef Mutex_Lock Log_File_Lock;
	typedef List<std::string, Mutex_Lock> String_List;
public:
	static int msg_buf_size;
	static int backtrace_size;
	static std::string msg_head[];

	static Log *instance(void);
	static void destroy(void);

	virtual void run_handler(void);
	virtual int process_list(void);

	void log_debug(const char *fmt, ...);
	void log_info(const char *fmt, ...);
	void log_warn(const char *fmt, ...);
	void log_error(const char *fmt, ...);
	void log_trace(const char *fmt, ...);
	void log_fatal(const char *fmt, ...);

	inline void set_log_file(bool log_file) { log_file_ = log_file; }
	inline void set_log_level(int log_level) { log_level_ = log_level; }
	inline void set_folder_name(std::string &folder_name) { folder_name_ = folder_name; }
	inline void push_log(std::string log_str) {
		notify_lock_.lock();
		log_list_.push_back(log_str);
		notify_lock_.signal();
		notify_lock_.unlock();
	}

private:
	Log(void);
	virtual ~Log(void);

	void assembly_msg(int log_flag, const char *fmt, va_list ap);
	void make_log_dir(void);
	void make_log_filepath(std::string &path);
	void log_file(std::string &log_str);
private:
	static Log *instance_;
	String_List log_list_;	//要写入文件的日志列表

	bool log_file_;			//是否写入文件
	int log_level_;			//日志级别，小于该级别的日志类型不会记录
	std::string log_dir_;
	std::string folder_name_;
	Log_File_Lock file_lock_;
	LOG_File_Info file_info_;
};

#define LOG_INSTACNE Log::instance()

//打印程序调试信息
#define LOG_DEBUG(FMT, ...) do {					\
		LOG_INSTACNE->log_debug("in %s:%d function %s: "#FMT, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
	} while (0)

//打印程序运行信息
#define LOG_INFO(FMT, ...) do {						\
		LOG_INSTACNE->log_info("in %s:%d function %s: "#FMT, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
	} while (0)

//打印程序警告信息
#define LOG_WARN(FMT, ...) do {						\
		LOG_INSTACNE->log_warn("in %s:%d function %s: "#FMT, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
	} while (0)

//打印程序错误信息
#define LOG_ERROR(FMT, ...) do {						\
		LOG_INSTACNE->log_error("in %s:%d function %s: "#FMT, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
	} while (0)

//打印程序运行堆栈
#define LOG_TRACE(FMT, ...) do {						\
		LOG_INSTACNE->log_trace("in %s:%d function %s: "#FMT, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
	} while (0)

//打印程序崩溃信息，将会导致应用程序的退出
#define LOG_FATAL(FMT, ...) do {					\
		LOG_INSTACNE->log_fatal("in %s:%d function %s: "#FMT, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
	} while (0)

#endif /* LOG_H_ */
