/*
 * Bit_Buffer.h
 *
 *  Created on: Oct 31,2016
 *      Author: zhangyalei
 */

#ifndef BIT_BUFFER_H_
#define BIT_BUFFER_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>

union Int64 {
	int64_t value;
	struct {
		uint low;
		uint high;
	}UInt32;
};

union UInt64 {
	uint64_t value;
	struct {
		uint low;
		uint high;
	}UInt32;
};

class Bit_Buffer {
public:
	Bit_Buffer(char* ary = NULL, int len = 0) :
		byte_ary(ary), byte_ary_capacity(len), write_byte_pos(len),
		w_bit_pos(0), read_byte_pos(0), r_bit_pos(0) {
	}

	~Bit_Buffer() {
		clear();
	}

	void set_ary(char* ary, int len) {
		if(byte_ary) {
			free(byte_ary);
			byte_ary = NULL;
		}
		byte_ary = (char*)malloc(len);
		memcpy(byte_ary, ary, len);
		//去掉数据指针赋值机制，防止指针在类外面被free，在Bit_Buffer调用析构free指针时候报错
		//byte_ary = ary;
		byte_ary_capacity = len;
		write_byte_pos = len;
		w_bit_pos = 0;
		read_byte_pos = 0;
		r_bit_pos = 0;
	}

	unsigned int get_byte_size() {
		if(w_bit_pos > 0) {
			return write_byte_pos + 1;
		}
		else {
			return write_byte_pos;
		}
	}

	unsigned int get_byte_capacity() { return byte_ary_capacity; }
	int set_capability(unsigned int cap) {
		char* old_data = byte_ary;
		byte_ary = (char*)malloc(cap);

		if(old_data) {
			int data_size = cap > write_byte_pos+1 ? write_byte_pos+1:cap;
			memcpy(byte_ary, old_data, data_size);
			free(old_data);
		}

		byte_ary_capacity = cap;
		if(write_byte_pos > cap) {
			write_byte_pos = cap;
		}
		if(w_bit_pos > write_byte_pos * 8) {
			w_bit_pos = write_byte_pos * 8;
		}

		return write_byte_pos;
	}

	char* data() { return byte_ary; }

	void clear() {
		if(byte_ary) {
			free(byte_ary);
			byte_ary = NULL;
		}
	}

	void reset() {
		clear();
		byte_ary_capacity = 0;
		write_byte_pos = 0;
		w_bit_pos = 0;
		read_byte_pos = 0;
		r_bit_pos = 0;
	}

	void enlarge(int add_size) {
		if((write_byte_pos + add_size) >= byte_ary_capacity) {
			set_capability(((write_byte_pos + add_size) / 128 + 1 )* 128);
		}
	}

	void write_bool(bool b) {
		enlarge(1);
		_write_flag(b);
	}

	void write_int(int i, unsigned int bits) {
		if(bits == 0) {
			return;
		}

		enlarge(4);
		if(i < 0) {
			//将负整数转为二进制，第一位为标志位1
			_write_flag(true);
			_write_u32((unsigned int)-i, bits-1);
		}
		else {
			_write_flag(false);
			_write_u32((unsigned int)i, bits-1);
		}
	}

	void write_uint(unsigned int i, unsigned int bits) {
		enlarge(4);
		_write_u32(i ,bits);
	}

	void write_int64(int64_t i) {
		enlarge(8);

		Int64 i64 = {0};
		i64.value = i;
		_write_u32(i64.UInt32.high, 32);
		_write_u32(i64.UInt32.low, 32);
	}

	void write_uint64(uint64_t i) {
		enlarge(8);

		UInt64 u64 = {0};
		u64.value = i;
		_write_u32(u64.UInt32.high, 32);
		_write_u32(u64.UInt32.low, 32);
	}

	void write_float(float f) {
		unsigned int f_mem = 0;
		memcpy(&f_mem, &f, sizeof(f));
		write_uint(f_mem, 32);
	}

	//TODO:encode str with huffman tree
	void write_str(const char* str) {
		//参数为空时候，写入空字符串结尾符
		if (str == NULL) {
			str = "";	 // write /0
		}

		int len = strlen(str) + 1;
		if(len <= 0) {
			str = "";
			len = 1; // write /0
		}

		if(w_bit_pos > 0) {
			++write_byte_pos;
			w_bit_pos = 0; // give up rest bits in current byte
		}

		enlarge(len);
		memcpy(byte_ary + write_byte_pos, str, len);
		write_byte_pos += len;
	}

	//notice:some bits may not use in last byte
	int read_bits_available() {
		return (write_byte_pos - read_byte_pos) * 8 + w_bit_pos - r_bit_pos;
	}

	//peek接口：读取数据，不改变读指针
	bool peek_bool() {
		bool value = read_bool();
		r_bit_pos = 0;
		read_byte_pos = 0;
		return value;
	}

	int peek_int(unsigned int bits) {
		int value = read_int(bits);
		r_bit_pos = 0;
		read_byte_pos = 0;
		return value;
	}

	unsigned int peek_uint(unsigned int bits) {
		unsigned int value = read_uint(bits);
		r_bit_pos = 0;
		read_byte_pos = 0;
		return value;
	}

 	int64_t peek_int64(void) {
		int64_t value = read_int64();
		r_bit_pos = 0;
		read_byte_pos = 0;
		return value;
	}

	uint64_t peek_uint64(void) {
		uint64_t value = read_uint64();
		r_bit_pos = 0;
		read_byte_pos = 0;
		return value;
	}

	float peek_float() {
		float value = read_float();
		r_bit_pos = 0;
		read_byte_pos = 0;
		return value;
	}

	int peek_str(char* str, int len) {
		int value = read_str(str, len);
		r_bit_pos = 0;
		read_byte_pos = 0;
		return value;
	}

	int peek_str(std::string &str) {
		int value = read_str(str);
		r_bit_pos = 0;
		read_byte_pos = 0;
		return value;
	}

	//read接口：读取数据，改变读指针
	bool read_bool() {
		return _read_flag();
	}

	int read_int(unsigned int bits) {
		if(bits == 0) {
			return 0;
		}

		if(_read_flag()) {
			return - (int)_read_u32(bits-1);
		}
		else {
			return (int)_read_u32(bits-1);
		}
	}

	unsigned int read_uint(unsigned int bits) {
		return _read_u32(bits);
	}

	int64_t read_int64(void) {
		uint high = _read_u32(32);
		uint low = _read_u32(32);
		return ((int64_t)high << 32) | (low);
	}

	uint64_t read_uint64(void) {
		uint high = _read_u32(32);
		uint low = _read_u32(32);
		return ((uint64_t)high << 32) | (low);
	}

	float read_float() {
		unsigned int f_mem = _read_u32(32);
		float f = 0.0f;
		memcpy(&f, &f_mem, sizeof(f));
		return f;
	}

	int read_str(char* str, int len) {
		if(len<=0) {
			return 0;
		}

		if(r_bit_pos > 0) {
			++read_byte_pos;
			r_bit_pos = 0; // give up rest bits in current byte
		}

		int readed_len = 0;
		char* src = byte_ary+read_byte_pos;
		for(; read_byte_pos<write_byte_pos && *src && readed_len < len; ++readed_len,++src,++read_byte_pos,++str) {
			*str = *src;
		}

		//如果读到的长度等于需要读的长度，说明src还没读完，就将读到的str减一，最后一个字符设为/0
		if(readed_len == len) {
			--str;
		}
		//设置字符串结尾/0字符
		*str = 0;

		//如果src没有读完，继续循环，直到读到/0,循环增加read_byte_pos
		for(; read_byte_pos<write_byte_pos && *src; ++src, ++read_byte_pos);
		//读指针加上字符串结尾空字符占用的字节
		if(read_byte_pos < write_byte_pos) {
			++read_byte_pos;
		}
		return readed_len;
	}

	int read_str(std::string &str) {
		if(r_bit_pos > 0) {
			++read_byte_pos;
			r_bit_pos = 0; // give up rest bits in current byte
		}

		int read_byte_pos_org = read_byte_pos;
		//计算字符串长度
		int len = 0;
		char* src = byte_ary+read_byte_pos;
		for(; read_byte_pos<write_byte_pos && *src; ++src, ++read_byte_pos) {
			len++;
		}
		//设置字符串str的长度
		str.resize(len);
		memcpy((char*)str.c_str(), byte_ary+read_byte_pos_org, len);
		//读指针加上字符串结尾空字符占用的字节
		if(read_byte_pos < write_byte_pos) {
			++read_byte_pos;
		}
		return len;
	}

private:
	bool _write_flag(bool b) {
		if(b) {
			byte_ary[write_byte_pos] |= (0x1<<w_bit_pos);
		}
		else {
			byte_ary[write_byte_pos] &= ~(0x1<<w_bit_pos);
		}

		++w_bit_pos;
		if(w_bit_pos >= 8) {
			++write_byte_pos;
		}
		w_bit_pos &= 0x7;
		return b;
	}

	bool _read_flag() {
		bool ret = (bool)(byte_ary[read_byte_pos] & (0x1<<r_bit_pos));

		++r_bit_pos;
		if(r_bit_pos >= 8) {
			++read_byte_pos;
		}
		r_bit_pos &= 0x7;
		return ret;
	}

	void _write_u32(unsigned int i, unsigned int bits) {
		if(bits > 32) {
			bits = 32;
		}

		int rest_bits = bits;
		for(; rest_bits > 0;) {
			int empty_bits = 8 - w_bit_pos;
			int to_fill_bits = empty_bits > rest_bits ? rest_bits : empty_bits;

			byte_ary[write_byte_pos] = (byte_ary[write_byte_pos] & ((0x1<<w_bit_pos) - 1)) | (((i >> (bits-rest_bits)) & ((0x1<<to_fill_bits) - 1))<<w_bit_pos);

			w_bit_pos += to_fill_bits;
			if(w_bit_pos >= 8) {
				++write_byte_pos;
			}
			w_bit_pos &= 0x7;

			rest_bits -= to_fill_bits;
		}
	}

	unsigned int _read_u32(unsigned int bits) {
		if(bits > 32) {
			bits = 32;
		}

		unsigned int ret = 0;
		int rest_bits = bits;
		for(; rest_bits > 0;) {
			int next_bits = 8 - r_bit_pos;
			int to_read_bits = next_bits > rest_bits ? rest_bits : next_bits;

			ret |= ((byte_ary[read_byte_pos] >> r_bit_pos) & ((0x1<<to_read_bits)-1)) << (bits-rest_bits);

			r_bit_pos += to_read_bits;
			if(r_bit_pos >= 8) {
				++read_byte_pos;
				r_bit_pos &= 0x7;
			}

			rest_bits -= to_read_bits;
		}
		return ret;
	}

	//write memory bytes bits to bytes ary
	void _write_bytes(const char* bytes, unsigned int len) {
		if(w_bit_pos > 0) {
			unsigned int i;
			int empty_bits = 8 - w_bit_pos;
			for(i=0; i< len; ++i) {
				byte_ary[write_byte_pos] = (byte_ary[write_byte_pos] & ((0x1<<w_bit_pos) - 1)) | ((bytes[i] & ((0x1<<empty_bits) - 1))<<w_bit_pos);
				++write_byte_pos;
				byte_ary[write_byte_pos] = (byte_ary[write_byte_pos] & 0) | bytes[i] >> empty_bits;
			}
		}
		else {
			memcpy(byte_ary + write_byte_pos, bytes, len);
			write_byte_pos += len;
		}
	}

	//read memory byte bits from bytes ary
	void _read_bytes(char* bytes, unsigned int len) {
		if(r_bit_pos > 0) {
			unsigned int i;
			int next_bits = 8 - r_bit_pos;
			for(i=0; i< len; ++i) {
				bytes[i] = ((byte_ary[read_byte_pos] >> r_bit_pos) & ((0x1<<next_bits)-1)) | (byte_ary[read_byte_pos + 1] & ((0x1<<r_bit_pos) - 1))<<next_bits;
				++read_byte_pos;
			}
		}
		else {
			memcpy(bytes, byte_ary + read_byte_pos, len);
			read_byte_pos += len;
		}
	}

private:
	char*	byte_ary;				//存放字节流的数组
	unsigned int byte_ary_capacity;	//字节数组容量
	unsigned int write_byte_pos;	//写字节位置
	unsigned int w_bit_pos;			//写bit位置
	unsigned int read_byte_pos;		//读字节位置
	unsigned int r_bit_pos;			//读bit位置
};

#endif // #ifndef BIT_BUFFER_H_
