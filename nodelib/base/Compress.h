/*
 *	Compress.h
 *
 *  Created on: Dec 28, 2016
 *      Author: zhangyalei
 */

#ifndef COMPRESS_H_
#define COMPRESS_H_

#include <zlib.h>

//获取压缩后buf长度，在压缩前调用
uLong comp_bound(uLong src_len);
//zlib压缩算法
bool comp(char* dest, uLongf* dest_len, const char* src, uLong src_len);
bool decomp(char* dest, uLongf* dest_len, const char* src, uLong src_len);

//文件压缩解压
int file_comp(const char* file_path);
int file_decomp(const char* file_path);

//文件夹压缩解压
int folder_comp(const char* folder_path);
int folder_decomp(const char* folder_path);

#endif // COMPRESS_H_

