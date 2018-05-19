/*
 * Hot_Update.cpp
 *
 *  Created on: Nov 21,2016
 *      Author: zhangyalei
 */

#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <openssl/md5.h>
#include <string.h>
#include "V8_Manager.h"
#include "Hot_Update.h"

Hot_Update::Hot_Update() {}

Hot_Update::~Hot_Update() {}

Hot_Update *Hot_Update::instance_;

Hot_Update *Hot_Update::instance(void) {
	if (! instance_)
		instance_ = new Hot_Update;
	return instance_;
}

void Hot_Update::run_handler(void) {
	while (true) {
		Time_Value::sleep(Time_Value(15));
		//遍历检查每个文件md5,发现不同的就更新该文件
		for (Md5_Str_Map::iterator iter = md5_str_map_.begin(); iter != md5_str_map_.end(); ++iter) {
			std::string md5_str = calc_file_md5(iter->first);
			if(md5_str != iter->second) {
				iter->second = md5_str;
				V8_MANAGER->push_hotupdate_file(iter->first);
			}
		}
	}
}

int Hot_Update::init(const std::vector<std::string> &folder_list) {
	for(std::vector<std::string>::const_iterator iter = folder_list.begin(); iter != folder_list.end(); ++iter) {
		init_file_md5(*iter);
	}
	return 0;
}

int Hot_Update::init_file_md5(const std::string folder_path) {
	struct dirent *ent = NULL;
	struct stat sbuf;
	DIR *pDir = NULL;
	pDir = opendir(folder_path.c_str());
	if (pDir == NULL) {
		//被当作目录，但是执行opendir后发现又不是目录，比如软链接就会发生这样的情况。
		LOG_ERROR("open dir error, folder_path:%s", folder_path.c_str());
		return -1;
	}

	//遍历目录下面的每个文件
	while (NULL != (ent = readdir(pDir))) {
		if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
			continue;
		}

		std::string file_path = folder_path + ent->d_name;
		//获取文件属性
		if (lstat(file_path.c_str(), &sbuf) < 0) {
			LOG_ERROR("lstat file error, file_path:%s", file_path.c_str());
			continue;
		}

		if (S_ISREG(sbuf.st_mode)) {
			//如果是文件，就计算md5,插入map
			if (strstr(ent->d_name, ".js") || strstr(ent->d_name, ".xml")) {
				std::string md5_str = calc_file_md5(file_path);
				if (md5_str != "") {
					md5_str_map_.insert(std::make_pair(file_path, md5_str));
				}
			}
		} else if (S_ISDIR(sbuf.st_mode)) {
			//如果是目录，就递归遍历
			file_path += "/";
			init_file_md5(file_path);
		}
	}

	if (pDir) {
		closedir(pDir);
		pDir = NULL;
	}
	return 0;
}

std::string Hot_Update::calc_file_md5(const std::string &file_path) {
	MD5_CTX md5;
	unsigned char md[16];
	char tmp[33] = {'\0'};
	int length = 0, i = 0;
	char buffer[1024] = {0};
	std::string hash = "";
	MD5_Init(&md5);

	int fd = 0;
	if ((fd = open(file_path.c_str(), O_RDONLY)) < 0) {
		LOG_ERROR("open file error, file_path:%s", file_path.c_str());
		return hash;
	}

	while (true) {
		length = read(fd, buffer, 1024);
		if (length == 0 || ((length == -1) && (errno != EINTR))) {
			break;
		} else if (length > 0) {
			MD5_Update(&md5, buffer, length);
		}
	}

	MD5_Final(md, &md5);
	for(i=0; i < 16; i++) {
		sprintf(tmp, "%02X", md[i]);
		hash += (std::string)tmp;
	}
	close(fd);

	return hash;
}
