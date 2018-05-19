/*
 * Comm_Func.h
 *
 *  Created on: Dec 16,2015
 *      Author: zhangyalei
 */

#ifndef COMMON_FUNC_H_
#define COMMON_FUNC_H_

#include <string.h>
#include <stdint.h>
#include <vector>
#include "Base_Enum.h"
#include "Date_Time.h"

//设置fd为非阻塞模式
void set_nonblock(int fd);
//获取进程最大fd
int max_fd(void);
//设置进程资源限制
void set_rlimit();

//定义文件处理函数指针，该函数参数是char*,返回值是int
typedef int (*file_handler) (const char *);
//遍历文件夹
int read_folder(const char *folder_path, file_handler handler);

//从fifo里面读数据
int read_fifo(const char *fifo_path, void *buf, int count);
//往fifo里面写数据
int write_fifo(const char *fifo_path, const void *buf, int count);

//获得进程调用堆栈
void backstrace_string(std::string &res);
//字符串分割
std::vector<std::string> split(const std::string &str, const std::string &pattern);
//获取字符串类型  返回值：-1表示字符串为空，0表示字符串，1表示整数，2表示小数
int get_string_type(const char *str);

//设置控制台颜色
void set_color(int fd, Color color);
void reset_color(int fd);

//返回大于1.5 * num的第一个素数
size_t get_hash_table_size(unsigned int num);
//获得elf hash值
int64_t elf_hash(const char *str);
//生成全局唯一id
int64_t make_id(int first_num, int second_num, int idx);
//生成token值
std::string make_token(const char *str);
//校验md5值
int validate_md5(const char *key, const char *account, const char *time, const char *flag);

//设置时间
void set_date_to_day(Date_Time &date_time, int time);
void set_date_time(Date_Time &date_time, int time);
void set_date_to_hour(Date_Time &date_time, int time);
//获取明天0点时间戳
int get_time_zero(void);
//获取指定时间的零点时间戳(不是24点)
int get_zero_time(const int sec);
int get_today_zero(void);
int set_time_to_zero(const Time_Value &time_src, Time_Value &time_des);
//获取星期日零点时间戳
int get_sunday_time_zero(void);
Time_Value get_week_time(int week, int hour = 0, int minute = 0, int second = 0);
// 获取的是相对时间
Time_Value spec_next_day_relative_time(int hour, int minute, int second);
// 获取的是绝对时间
Time_Value spec_today_absolute_time(unsigned int hour, unsigned int minute, unsigned int second);
Time_Value get_day_begin(const Time_Value &now);
void get_next_cycle_time(const Time_Value &begin, const Time_Value &now, const Time_Value &offset, const Time_Value &cycle, Time_Value &next_time);
void get_date_day_gap(const Time_Value &date1, const Time_Value &date2, int &day);
//get_days_delta:获取两个时间之间相差的天数（此天数以日期的天为单位），即1号的任意钟头和2号的任意钟头都是相差一天
int get_days_delta(Time_Value time1, Time_Value time2);

//数字比较函数
inline int max(int first, int second) { return first >= second ? first : second; }
inline int min(int first, int second) { return first <= second ? first : second; }
inline bool is_double_zero(double value) { return value >= -0.0000001 && value <= 0.0000001; }
inline bool is_double_gt_zero(double value) { return value > 0.0000001; }
inline bool is_double_lt_zero(double value) { return value < -0.0000001; }

//base64编码解码
inline bool is_base64(unsigned char c) { return (isalnum(c) || (c == '+') || (c == '/')); }
std::string base64_encode(unsigned char const* , unsigned int len);
std::string base64_decode(std::string const& s);

#endif /* COMMON_FUNC_H_ */
