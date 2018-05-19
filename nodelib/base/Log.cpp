/*
 * Log.cpp
 *
 *  Created on: Dec 16,2015
 *      Author: zhangyalei
 */

#include <stdarg.h>
#include <execinfo.h>
#include <sys/stat.h>
#include <errno.h>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include "Base_Function.h"
#include "Log.h"

Log::Log(void) : log_file_(false), log_level_(0), log_dir_("./log"), folder_name_("") { }

Log::~Log(void) { }

int Log::msg_buf_size = 4096;
int Log::backtrace_size = 512;

std::string Log::msg_head[] = {
		"[LOG_DEBUG] ",
		"[LOG_INFO] ",
		"[LOG_WARN] ",
		"[LOG_ERROR] ",
		"[LOG_TRACE] ",
		"[LOG_FATAL] "
};

Log *Log::instance_ = 0;
Log *Log::instance(void) {
	if (!instance_)
		instance_ = new Log;
	return instance_;
}

void Log::destroy(void) {
	if (instance_) {
		delete instance_;
		instance_ = 0;
	}
}

void Log::run_handler(void) {
	process_list();
}

int Log::process_list(void) {
	std::string log_str = "";
	while (true) {
		//此处加锁是为了防止本线程其他地方同时等待条件变量成立
		notify_lock_.lock();

		//put wait in while cause there can be spurious wake up (due to signal/ENITR)
		while (log_list_.empty()) {
			notify_lock_.wait();
		}

		log_str = log_list_.pop_front();
		log_file(log_str);

		//操作完成解锁条件变量
		notify_lock_.unlock();
	}
	return 0;
}

void Log::log_debug(const char *fmt, ...) {
	va_list	ap;
	va_start(ap, fmt);
	assembly_msg(LOG_DEBUG, fmt, ap);
	va_end(ap);
}

void Log::log_info(const char *fmt, ...) {
	va_list	ap;
	va_start(ap, fmt);
	assembly_msg(LOG_INFO, fmt, ap);
	va_end(ap);
}

void Log::log_warn(const char *fmt, ...) {
	va_list	ap;
	va_start(ap, fmt);
	assembly_msg(LOG_WARN, fmt, ap);
	va_end(ap);
}

void Log::log_error(const char *fmt, ...) {
	va_list	ap;
	va_start(ap, fmt);
	assembly_msg(LOG_ERROR, fmt, ap);
	va_end(ap);
}

void Log::log_trace(const char *fmt, ...) {
	va_list	ap;
	va_start(ap, fmt);
	assembly_msg(LOG_TRACE, fmt, ap);
	va_end(ap);
}

void Log::log_fatal(const char *fmt, ...) {
	va_list	ap;
	va_start(ap, fmt);
	assembly_msg(LOG_FATAL, fmt, ap);
	va_end(ap);
}

void Log::assembly_msg(int log_flag, const char *fmt, va_list ap) {
	if (log_flag < log_level_) {
		return;
	}

	std::ostringstream log_stream;
	struct tm tm_v;
	time_t time_v = time(NULL);
	localtime_r(&time_v, &tm_v);
	log_stream << "<pid=" << (int)getpid() << "|tid=" << pthread_self()
			<< ">(" << (tm_v.tm_hour) << ":" << (tm_v.tm_min) << ":" << (tm_v.tm_sec) << ")";

	log_stream << msg_head[log_flag - 1];

	char line_buf[msg_buf_size];
	memset(line_buf, 0, sizeof(line_buf));
	vsnprintf(line_buf, sizeof(line_buf), fmt, ap);
	log_stream << line_buf;

	switch (log_flag) {
	case LOG_DEBUG: {
		set_color(STDERR_FILENO, WHITE);
		log_stream << std::endl;
		break;
	}
	case LOG_INFO: {
		set_color(STDERR_FILENO, LGREEN);
		log_stream << std::endl;
		break;
	}
	case LOG_WARN: {
		set_color(STDERR_FILENO, YELLOW);
		log_stream << std::endl;
		break;
	}
	case LOG_ERROR: {
		set_color(STDERR_FILENO, LRED);
		log_stream << ", errno = " << errno;
		memset(line_buf, 0, sizeof(line_buf));
		strerror_r(errno, line_buf, sizeof(line_buf));
		log_stream << ", errstr=[" << line_buf << "]" << std::endl;
		break;
	}
	case LOG_TRACE: {
		set_color(STDERR_FILENO, MAGENTA);
		int nptrs;
		void *buffer[backtrace_size];
		char **strings;

		nptrs = backtrace(buffer, backtrace_size);
		strings = backtrace_symbols(buffer, nptrs);
		if (strings == NULL)
			return ;

		log_stream << std::endl;
		for (int i = 0; i < nptrs; ++i) {
			log_stream << (strings[i]) << std::endl;
		}
		free(strings);
		break;
	}
	case LOG_FATAL: {
		set_color(STDERR_FILENO, LBLUE);
		log_stream << "errno = " << errno;
		memset(line_buf, 0, sizeof(line_buf));
		strerror_r(errno, line_buf, sizeof(line_buf));
		log_stream << ", errstr=[" << line_buf << "]" << std::endl;

		if(!log_file_) {
			std::cerr << log_stream.str();
			reset_color(STDERR_FILENO);
		}
		abort();
		break;
	}
	default: {
		break;
	}
	}

	if (log_file_) {
		push_log(log_stream.str());
	}
	else {
		std::cerr << log_stream.str();
		reset_color(STDERR_FILENO);
	}
	return ;
}

void Log::make_log_dir(void) {
	int ret = mkdir(log_dir_.c_str(), 0775);
	if (ret == -1 && errno != EEXIST) {
		perror("mkdir error");
		return;
	}

	if (folder_name_ != "") {
		std::stringstream stream;
		stream 	<< log_dir_ << "/" << folder_name_;
		log_dir_ = stream.str();
		mkdir(log_dir_.c_str(), 0775);
		if (ret == -1 && errno != EEXIST) {
			perror("mkdir error");
		}
	}
}

void Log::make_log_filepath(std::string &path) {
	time_t time_v = time(NULL);
	struct tm tm_v;
	localtime_r(&time_v, &tm_v);

	std::stringstream stream;
	stream 	<< log_dir_ << "/" << (tm_v.tm_year + 1900) << "-" << (tm_v.tm_mon + 1) << "-" << (tm_v.tm_mday)
			<< "-" << (tm_v.tm_hour) << ".log";

	path = stream.str().c_str();
}

void Log::log_file(std::string &log_str) {
	GUARD(Log_File_Lock, mon, file_lock_);

	if (! file_info_.fp) {
		make_log_dir();
		make_log_filepath(file_info_.filepath);

		if ((file_info_.fp = fopen(file_info_.filepath.c_str(), "a")) == NULL) {
			printf("filepath=[%s]\n", file_info_.filepath.c_str());
			perror("fopen error");
			return;
		}
		file_info_.tv = Time_Value::gettimeofday();
	} else {
		if (! is_same_hour(file_info_.tv, Time_Value::gettimeofday())) {
			fclose(file_info_.fp);
			file_info_.tv = Time_Value::gettimeofday();
			//make_log_dir();
			make_log_filepath(file_info_.filepath);

			if ((file_info_.fp = fopen(file_info_.filepath.c_str(), "a")) == NULL) {
				printf("filepath=[%s]", file_info_.filepath.c_str());
				perror("fopen error");
				return;
			}
		}
	}

	fputs(log_str.c_str(), file_info_.fp);
	fflush(file_info_.fp);
}
