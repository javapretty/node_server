/*
 *  Created on: Dec 16, 2015
 *      Author: zhangyalei
 */

#include <dirent.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <math.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sstream>
#include <unistd.h>
#include <openssl/md5.h>
#include "Log.h"
#include "Base_Function.h"

void set_nonblock(int fd) {
	int flags = ::fcntl(fd, F_GETFL, 0);
	if (flags == -1) {
		LOG_ERROR("fcntl erro, fd = %d", fd);
		return;
	}
	::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int max_fd(void) {
	struct rlimit rl;
	int ret = 0;

	if ((ret = getrlimit(RLIMIT_NOFILE, &rl)) == -1)
		LOG_FATAL("getrlimit");
	else
		return rl.rlim_cur;

	return sysconf (_SC_OPEN_MAX);
}

void set_rlimit() {
    struct rlimit rlim, rlim_new;

	//设置core文件最大值
    if (0 == getrlimit(RLIMIT_CORE, &rlim)) {
        // Increase RLIMIT_CORE to infinity if possible
        rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
        if (0 != setrlimit(RLIMIT_CORE, &rlim_new)){
            // Set rlimit error
            rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
            (void) setrlimit(RLIMIT_CORE, &rlim_new);
			LOG_ERROR("Set rlimit max error, default max value will be used");
        }
    }
    if (0 != getrlimit(RLIMIT_CORE, &rlim) || rlim.rlim_cur == 0) {
		LOG_FATAL("Failed to ensure core file creation");
    }

    //设置进程最大打开的fd数量
    if (0 != getrlimit(RLIMIT_NOFILE, &rlim)) {
        LOG_FATAL("Failed to getrlimit number of files");
    }
    else
    {
        // Enlarge RLIMIT_NOFILE to allow as many connections as we need
        unsigned int maxfiles = 102400;
        if (rlim.rlim_cur < maxfiles + 3) {
            rlim.rlim_cur = maxfiles + 3;
        }

        if (rlim.rlim_max < rlim.rlim_cur) {
            if (0 == getuid() || 0 == geteuid()) {
                rlim.rlim_max = rlim.rlim_cur;
            }
            else {
                LOG_ERROR("You cannot modify NOFILE hard_limit out of superuser privilege\n");
                rlim.rlim_cur = rlim.rlim_max;
            }
        }

        if (0 != setrlimit(RLIMIT_NOFILE, &rlim)) {
            // Set error
            LOG_ERROR(CLT_ERR, "Failed to set rlimit for open files, try root privilege.\n");
        }
    }
}

int read_folder(const char *folder_path, file_handler handler) {
	struct dirent *ent = NULL;
	struct stat sbuf;
	DIR *pDir = NULL;
	pDir = opendir(folder_path);
	if (pDir == NULL) {
		//被当作目录，但是执行opendir后发现又不是目录，比如软链接就会发生这样的情况。
		LOG_ERROR("open dir error, folder_path:%s", folder_path);
		return -1;
	}

	//遍历目录下面的每个文件
	while (NULL != (ent = readdir(pDir))) {
		if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
			continue;
		}

		std::string file_path(folder_path);
		file_path += ent->d_name;
		//获取文件属性
		if (lstat(file_path.c_str(), &sbuf) < 0) {
			LOG_ERROR("lstat file error, file_path:%s", file_path.c_str());
			continue;
		}

		if (S_ISREG(sbuf.st_mode)) {
			//如果是文件
			if (strstr(ent->d_name, ".js")) {
				handler(file_path.c_str());
			}
		} else if (S_ISDIR(sbuf.st_mode)) {
			//如果是目录，就递归遍历
			file_path += "/";
			read_folder(file_path.c_str(), handler);
		}
	}

	if (pDir) {
		closedir(pDir);
		pDir = NULL;
	}
	return 0;
}

int read_fifo(const char *fifo_path, void *buf, int count) {
	//测试FIFO是否存在，若不存在，mkfifo一个FIFO
	if(access(fifo_path, F_OK) == -1){
		if((mkfifo(fifo_path, 0666) < 0) && (errno != EEXIST)){
			LOG_ERROR("can't create fifo:%s", fifo_path);
			return -1;
		}
	}

	//以只读阻塞方式打开FIFO，返回文件描述符fd
	int fd = 0;
	if((fd = open(fifo_path, O_RDONLY)) == -1){
		LOG_ERROR("open fifo:%s error", fifo_path);
		return -1;
	}

	//调用read将fd指向的FIFO的内容，读到buf中
	int real_read = read(fd, buf, count);
	close(fd);
	return real_read;
}

int write_fifo(const char *fifo_path, const void *buf, int count) {
	//测试FIFO是否存在，若不存在，mkfifo一个FIFO
	if(access(fifo_path, F_OK) == -1) {
		if((mkfifo(fifo_path, 0666) < 0) && (errno != EEXIST)) {
			LOG_ERROR("can't create fifo:%s", fifo_path);
			return -1;
		}
	}

	//以只写阻塞方式打开FIFO，返回文件描述符fd
	int fd = 0;
	if((fd = open(fifo_path, O_WRONLY)) == -1) {
		LOG_ERROR("open fifo:%s error", fifo_path);
		return -1;
	}

	//调用write将buf写到文件描述符fd指向的FIFO中
	int real_write = write(fd, buf, count);
	close(fd);
	return real_write;
}

void backstrace_string(std::string &res) {
	static const int backtrace_size = 512;
	int nptrs = 0;
	void *buffer[backtrace_size] = {0};
	char **strings = 0;

	nptrs = backtrace(buffer, backtrace_size);
	strings = backtrace_symbols(buffer, nptrs);
	if (strings) {
		for (int i = 0; i < nptrs; ++i) {
			res.append(strings[i]);
			res.append("\n");
		}
		free(strings);
	}
}

std::vector<std::string> split(const std::string &str, const std::string &pattern) {
	std::string::size_type pos;
	std::vector<std::string> result;
	int size = str.size();
	for(int i=0; i < size; i++) {
		pos = str.find(pattern, i);
		if(pos != std::string::npos) {
			std::string substr = str.substr(i, pos-i);
			result.push_back(substr);
			i = pos + pattern.size()-1;
		}
		else {
			//查找不到pattern,就把剩余字符放到result里，然后跳出循环
			std::string substr = str.substr(i, size-i);
			result.push_back(substr);
			break;
		}
	}
	return result;
}

int get_string_type(const char *str) {
	if(str == nullptr) {
		return -1;
	}

	bool bdouble = false;
	char c = 0;
	int i = 0;

	while((c = *(str + i))) {
		i++;
		//32代码空格，9代表tab键
		if(c == 32 || c == 9) {
			continue;
		}
		//46代表小数点
		if(c == 46) {
			bdouble = true;
		}
		//48-57代表数字0-9
		else if(c < 48 || c > 57) {
			return 0;
		}
	}

	return bdouble + 1;
}

void set_color(int fd, Color color) {
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "\x1b[%d%sm",
			color >= LRED ? (color - 10) : color,
			color >= LRED ? ";1" : ""
			);
	write(fd, buffer, strlen(buffer));
}

void reset_color(int fd) {
	const char* s = "\x1b[0m";
	write(fd, s, strlen(s));
}

size_t get_hash_table_size(unsigned int num) {
	size_t return_num = 1.5 * num, i, j;
	while (1) {
		/// 素数判断
		j = (size_t)sqrt(return_num++);
		for (i = 2; i < j; ++i) {
			if (return_num % i == 0)
				break;
		}
		if (i == j)
			return return_num;
	}
	return 0;
}

int64_t elf_hash(const char *str) {
	int64_t hash = 0;
	int64_t x = 0;
	int len = strlen(str);
	for(int i = 0; i < len; ++i) {
		//h左移4位，当前字符ASCII存入h的低四位
		hash = (hash << 4) + str[i];
		//如果最高位不为0，则说明字符多余7个，如果不处理，再加第九个字符时，第一个字符会被移出
		if((x = hash & 0xF0000000L) != 0) {
			hash ^= (x >> 24);
		}
		//清空28~31位
		hash &= ~x;
	}
	return hash;
}

int64_t make_id(int first_num, int second_num, int idx) {
	int64_t first = first_num * 10000000000000L;
	int64_t second = second_num * 1000000000L;
	int64_t id = first + second + idx;
	return id;
}

std::string make_token(const char *str){
	long timesamp = Time_Value::gettimeofday().sec() + Time_Value::gettimeofday().usec();
	int64_t hash = elf_hash(str);
	int rand = random() % hash;

	std::stringstream stremsession;
	stremsession << timesamp;
	stremsession << hash;
	stremsession << rand;

	return stremsession.str();
}

int validate_md5(const char *key, const char *account, const char *time, const char *session) {
	char mine_src[256 + 1], mine_md5[256 + 1];
	memset(mine_src, 0x00, 256 + 1);
	memset(mine_md5, 0x00, 256 + 1);

	snprintf(mine_src, sizeof(mine_src), "%s%s%s", account, time, key);
	const unsigned char *tmp_md5 = MD5((const unsigned char *) mine_src, strlen(mine_src), 0);

	for (uint i = 0; i < 16; i++) {
		sprintf(&mine_md5[i * 2], "%.2x", tmp_md5[i]);
    }

	return strncmp(session, mine_md5, strlen(session));
}

void set_date_to_day(Date_Time &date_time, int time) {
	date_time.year(time / 10000);
	time = time % 10000;
	date_time.month(time / 100);
	time = time % 100;
	date_time.day(time);
	date_time.hour(0);
	date_time.minute(0);
	date_time.second(0);
}

void set_date_time(Date_Time &date_time, int time) {
	int int_tmp1, int_tmp2, int_tmp3;
	int_tmp1 = time;
	int_tmp3 = int_tmp1 % 100;
	int_tmp1 = int_tmp1 / 100;
	int_tmp2 = int_tmp1 % 100;
	int_tmp1 = int_tmp1 / 100;
	date_time.hour(int_tmp1);
	date_time.minute(int_tmp2);
	date_time.second(int_tmp3);
}

void set_date_to_hour(Date_Time &date_time, int time) {
	date_time.year(time / 1000000);
	time = time % 1000000;
	date_time.month(time / 10000);
	time = time % 10000;
	date_time.day(time / 100);
	date_time.hour(time % 100);
	date_time.minute(0);
	date_time.second(0);
}

int get_time_zero(void) {
	Date_Time date_time;
	date_time.hour(0);
	date_time.minute(0);
	date_time.second(0);

	return date_time.time_sec() + 86400;
}

int get_time_zero(const int sec) {
	int zeor_time = sec - (sec + 28800) % 86400;
	return zeor_time;
}

int get_today_zero(void) {
	Date_Time date_time;
	date_time.hour(0);
	date_time.minute(0);
	date_time.second(0);

	return date_time.time_sec();
}

int set_time_to_zero(const Time_Value &time_src, Time_Value &time_des) {
	Date_Time date_tmp(time_src);
	date_tmp.hour(0);
	date_tmp.minute(0);
	date_tmp.second(0);
	time_des.sec(date_tmp.time_sec());
	return 0;
}

int get_sunday_time_zero(void) {
	Time_Value time = get_week_time(7, 23, 59, 59);
	return time.sec();
}

Time_Value get_week_time(int week, int hour, int minute, int second) {
	Time_Value time(Time_Value::gettimeofday());
	Date_Time date_now(time);
	int day_gap = 0;
	int weekday = date_now.weekday();
	if (weekday > 0) {
		day_gap = week - weekday;
	}
	int hour_gap = hour - date_now.hour();
	int minute_gap = minute - date_now.minute();
	int second_gap = second - date_now.second();

	int sec = time.sec();
	int rel_sec = 0;
	rel_sec += day_gap*Time_Value::ONE_DAY_IN_SECS;
	rel_sec += hour_gap*Time_Value::ONE_HOUR_IN_SECS;
	rel_sec += minute_gap*Time_Value::ONE_MINUTE_IN_SECS;
	rel_sec += second_gap;
	if (rel_sec <= 0) {
		rel_sec += Time_Value::ONE_DAY_IN_SECS * 7;
	}

	time.set(sec + rel_sec, 0);
	return time;
}

Time_Value spec_next_day_relative_time(int hour, int minute, int second) {
	Time_Value time(Time_Value::gettimeofday());
	Date_Time date_now(time);
	int hour_gap = hour - date_now.hour();
	int minute_gap = minute - date_now.minute();
	int second_gap = second - date_now.second();

	int sec = 0;
	sec += hour_gap * Time_Value::ONE_HOUR_IN_SECS;
	sec += minute_gap * Time_Value::ONE_MINUTE_IN_SECS;
	sec += second_gap;

	if (sec <= 0) {
		sec += Time_Value::ONE_DAY_IN_SECS;
	}

	time.set(sec, 0);
	return time;
}

Time_Value spec_today_absolute_time(unsigned int hour, unsigned int minute, unsigned int second) {
	Time_Value time(Time_Value::gettimeofday());
	Date_Time date_now(time);
	int hour_gap = hour - date_now.hour();
	int minute_gap = minute - date_now.minute();
	int second_gap = second - date_now.second();

	int sec = 0;
	sec += hour_gap * Time_Value::ONE_HOUR_IN_SECS;
	sec += minute_gap * Time_Value::ONE_MINUTE_IN_SECS;
	sec += second_gap;
	sec += time.sec();

	time.set(sec, 0);

	return time;
}

Time_Value get_day_begin(const Time_Value &now) {
	Date_Time date_now(now);
	date_now.hour(0);
	date_now.minute(0);
	date_now.second(0);
	date_now.microsec(0);
	Time_Value day_begin(date_now.time_sec(), 0);
	return day_begin;
}

void get_date_day_gap(const Time_Value &date1, const Time_Value &date2, int &day) {
	Time_Value local_date1 = get_day_begin(date1);
	Time_Value local_date2 = get_day_begin(date2);
	int64_t sec_gap = local_date2.sec() - local_date1.sec();
	day = sec_gap / Time_Value::ONE_DAY_IN_SECS;
}

void get_next_cycle_time(const Time_Value &begin, const Time_Value &now, const Time_Value &offset,
		const Time_Value &cycle, Time_Value &next_time) {
	Time_Value first_begin_time = begin + offset;
	if (now > first_begin_time) {
		if (cycle <= Time_Value::zero) {
			next_time = Time_Value::max;
			return;
		}
		int cycle_sec = cycle.sec();
		int sec_gap = now.sec() - first_begin_time.sec();
		int cycle_nums = sec_gap / cycle_sec;
		++cycle_nums;
		next_time.set(first_begin_time.sec() + cycle.sec() * cycle_nums, 0);
	} else {
		next_time = first_begin_time;
	}
}

int get_days_delta(Time_Value time1, Time_Value time2) {
	Time_Value time_tmp;
	if (time1 < time2) {
		time_tmp = time1;
		time1 = time2;
		time2 = time_tmp;
	}

	Date_Time date1(time1), date2(time2);
	date1.hour(0);
	date1.minute(0);
	date1.second(0);
	date2.hour(0);
	date2.minute(0);
	date2.second(0);

	return (date1.time_sec() - date2.time_sec()) / Time_Value::ONE_DAY_IN_SECS;
}

static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';

  }

  return ret;

}

std::string base64_decode(std::string const& encoded_string) {
  size_t in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::string ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret += char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
  }

  return ret;
}
