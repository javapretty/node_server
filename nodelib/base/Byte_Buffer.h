/*
 * Byte_Buffer.h
 *
 *  Created on: Dec 16,2015
 *      Author: zhangyalei
 */

/*
 +------------+----------------+-----------------+
 | head_bytes | readable bytes | writeable bytes |
 |            |   (CONTENT)    |                 |
 +------------+----------------+-----------------+
  |                        |                 		  |
 0  read_index(init_offset)  write_index     vector::size()
 */

#ifndef BYTE_BUFFER_H_
#define BYTE_BUFFER_H_

#include <stdint.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <endian.h>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <algorithm>
#include "Base_Enum.h"
#include "Base_Define.h"
#include "Log.h"

#define BLOCK_LITTLE_ENDIAN

class Byte_Buffer {
public:
	Byte_Buffer()
	: max_use_times_(1),
	  use_times_(0),
	  init_size_(2048),
	  init_offset_(16),
	  read_index_(16),
	  write_index_(16),
	  buffer_(2064)
	{ }

	inline void reset(void);
	inline void swap(Byte_Buffer &buffer);

	/// 当前缓冲内可读字节数
	inline size_t readable_bytes(void) const;
	/// 当前缓冲内可写字节数
	inline size_t writable_bytes(void) const;

	inline char * get_read_ptr(void);
	inline char * get_write_ptr(void);
	inline size_t get_buffer_size(void);

	inline int get_read_idx(void);
	inline void set_read_idx(int ridx);
	inline int get_write_idx(void);
	inline void set_write_idx(int widx);

	inline void ensure_writable_bytes(size_t len);

	inline void copy(Byte_Buffer *buffer);
	inline void copy(std::string const &str);
	inline void copy(void const *data, size_t len);
	inline void copy(char const *data, size_t len);
	inline void copy_out(char *data, size_t len);

	inline void debug(void);

	//从buffer里面读取数据，不改变读指针
	inline int peek_int8(int8_t &v);
	inline int peek_int16(int16_t &v);
	inline int peek_int32(int32_t &v);
	inline int peek_int64(int64_t &v);
	inline int peek_uint8(uint8_t &v);
	inline int peek_uint16(uint16_t &v);
	inline int peek_uint32(uint32_t &v);
	inline int peek_uint64(uint64_t &v);
	inline int peek_double(double &v);
	inline int peek_bool(bool &v);
	inline int peek_string(std::string &str);

	//从buffer里面读取数据，改变读指针
	inline int read_int8(int8_t &v);
	inline int read_int16(int16_t &v);
	inline int read_int32(int32_t &v);
	inline int read_int64(int64_t &v);
	inline int read_uint8(uint8_t &v);
	inline int read_uint16(uint16_t &v);
	inline int read_uint32(uint32_t &v);
	inline int read_uint64(uint64_t &v);
	inline int read_double(double &v);
	inline int read_bool(bool &v);
	inline int read_string(std::string &str);

	//往buffer里面写入数据，改变写指针
	inline int write_int8(int8_t v);
	inline int write_int16(int16_t v);
	inline int write_int32(int32_t v);
	inline int write_int64(int64_t v);
	inline int write_uint8(uint8_t v);
	inline int write_uint16(uint16_t v);
	inline int write_uint32(uint32_t v);
	inline int write_uint64(uint64_t);
	inline int write_double(double v);
	inline int write_bool(bool v);
	inline int write_string(const std::string &str);

	//从buffer读取数据，改变读指针
	inline Byte_Buffer &operator>>(int8_t &v);
	inline Byte_Buffer &operator>>(int16_t &v);
	inline Byte_Buffer &operator>>(int32_t &v);
	inline Byte_Buffer &operator>>(int64_t &v);
	inline Byte_Buffer &operator>>(uint8_t &v);
	inline Byte_Buffer &operator>>(uint16_t &v);
	inline Byte_Buffer &operator>>(uint32_t &v);
	inline Byte_Buffer &operator>>(uint64_t &v);
	inline Byte_Buffer &operator>>(double &v);
	inline Byte_Buffer &operator>>(bool &v);
	inline Byte_Buffer &operator>>(std::string &str);

	//往buffer里面写入数据，改变写指针
	inline Byte_Buffer &operator<<(int8_t v);
	inline Byte_Buffer &operator<<(int16_t v);
	inline Byte_Buffer &operator<<(int32_t v);
	inline Byte_Buffer &operator<<(int64_t v);
	inline Byte_Buffer &operator<<(uint8_t v);
	inline Byte_Buffer &operator<<(uint16_t v);
	inline Byte_Buffer &operator<<(uint32_t v);
	inline Byte_Buffer &operator<<(uint64_t v);
	inline Byte_Buffer &operator<<(double v);
	inline Byte_Buffer &operator<<(bool v);
	inline Byte_Buffer &operator<<(const std::string &str);

	inline void read_head(Msg_Head &msg_head);
	inline void write_head(const Msg_Head &msg_head);
	inline void write_len(const Msg_Head &msg_head, bool compress = false);
	inline size_t capacity(void);
	inline void recycle_space(void);

	inline bool is_legal(void);
	inline bool verify_read(size_t s);
	inline void log_binary_data(size_t len);

private:
	inline char *begin(void);
	inline const char *begin(void) const;
	inline void make_space(size_t len);

private:
	unsigned short max_use_times_;
	unsigned short use_times_;
	size_t init_size_;
	size_t init_offset_;
	size_t read_index_, write_index_;
	std::vector<char> buffer_;
};

////////////////////////////////////////////////////////////////////////////////
void Byte_Buffer::reset(void) {
	++use_times_;
	recycle_space();

	ensure_writable_bytes(init_offset_);
	read_index_ = write_index_ = init_offset_;
}

void Byte_Buffer::swap(Byte_Buffer &buffer) {
	std::swap(this->max_use_times_, buffer.max_use_times_);
	std::swap(this->use_times_, buffer.use_times_);
	std::swap(this->init_size_, buffer.init_size_);
	std::swap(this->init_offset_, buffer.init_offset_);
	std::swap(this->read_index_, buffer.read_index_);
	std::swap(this->write_index_, buffer.write_index_);
	buffer_.swap(buffer.buffer_);
}

size_t Byte_Buffer::readable_bytes(void) const {
	return write_index_ - read_index_;
}

size_t Byte_Buffer::writable_bytes(void) const {
	return buffer_.size() - write_index_;
}

char *Byte_Buffer::get_read_ptr(void) {
	return begin() + read_index_;
}

char *Byte_Buffer::get_write_ptr(void) {
	return begin() + write_index_;
}

size_t Byte_Buffer::get_buffer_size(void) {
	return buffer_.size();
}

int Byte_Buffer::get_read_idx(void) {
	return read_index_;
}

void Byte_Buffer::set_read_idx(int ridx) {
	if ((size_t)ridx > buffer_.size() || (size_t)ridx > write_index_) {
		LOG_FATAL("set_read_idx error ridx = %d.", ridx);
		debug();
	}

	read_index_ = ridx;
}

int Byte_Buffer::get_write_idx(void) {
	return write_index_;
}

void Byte_Buffer::set_write_idx(int widx) {
	if ((size_t)widx > buffer_.size() || (size_t)widx < read_index_) {
		LOG_FATAL("set_write_idx error widx = %d.", widx);
		debug();
	}

	write_index_ = widx;
}

char *Byte_Buffer::begin(void) {
	return &*buffer_.begin();
}

const char *Byte_Buffer::begin(void) const {
	return &*buffer_.begin();
}

void Byte_Buffer::ensure_writable_bytes(size_t len) {
	if (writable_bytes() < len) {
		make_space(len);
	}
}

void Byte_Buffer::make_space(size_t len) {
	int cond_pos = read_index_ - init_offset_;
	size_t read_begin, head_size;
	if (cond_pos < 0) {
		read_begin = init_offset_ = read_index_;
		head_size = 0;
		LOG_ERROR("read_index_ = %u, init_offset_ = %u", read_index_, init_offset_);
	} else {
		read_begin = init_offset_;
		head_size = cond_pos;
	}

	if (writable_bytes() + head_size < len) {
		buffer_.resize(write_index_ + len);
	} else {
		/// 把数据移到头部，为写腾出空间
		size_t readable = readable_bytes();
		std::copy(begin() + read_index_, begin() + write_index_, begin() + read_begin);
		read_index_ = read_begin;
		write_index_ = read_index_ + readable;
	}
}

void Byte_Buffer::copy(Byte_Buffer *buffer) {
	copy(buffer->get_read_ptr(), buffer->readable_bytes());
}

void Byte_Buffer::copy(std::string const &str) {
	copy(str.data(), str.length());
}

void Byte_Buffer::copy(void const *data, size_t len) {
	copy(static_cast<const char*> (data), len);
}

void Byte_Buffer::copy(char const *data, size_t len) {
	ensure_writable_bytes(len);
	std::copy(data, data + len, get_write_ptr());
	write_index_ += len;
}

void Byte_Buffer::copy_out(char *data, size_t len) {
	memcpy(data, &(buffer_[read_index_]), len);
	read_index_ += len;
}

void Byte_Buffer::debug(void) {
	LOG_DEBUG("read_index=%d, write_index=%d, buffer_size=%d, buffer_content=%s",
			read_index_, write_index_, buffer_.size(), get_read_ptr());
}

int Byte_Buffer::peek_int8(int8_t &v) {
	if (verify_read(sizeof(v))) {
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
	} else {
		LOG_ERROR("out of range");
		return -1;
	}
	return 0;
}

int Byte_Buffer::peek_int16(int16_t &v) {
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint16_t t, u;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		u = be16toh(t);
		memcpy(&v, &u, sizeof(v));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
#endif
	} else {
		LOG_ERROR("out of range");
		return -1;
	}
	return 0;
}

int Byte_Buffer::peek_int32(int32_t &v) {
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint32_t t, u;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		u = be32toh(t);
		memcpy(&v, &u, sizeof(v));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
#endif
	} else {
		LOG_ERROR("out of range");
		return -1;
	}
	return 0;
}

int Byte_Buffer::peek_int64(int64_t &v) {
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint64_t t, u;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		u = be64toh(t);
		memcpy(&v, &u, sizeof(v));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
#endif
	} else {
		LOG_ERROR("out of range");
		return -1;
	}
	return 0;
}

int Byte_Buffer::peek_uint8(uint8_t &v) {
	if (verify_read(sizeof(v))) {
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
	} else {
		LOG_ERROR("out of range");
		return -1;
	}
	return 0;
}

int Byte_Buffer::peek_uint16(uint16_t &v) {
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint16_t t;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		v = be16toh(t);
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
#endif
	} else {
		LOG_ERROR("out of range");
		return -1;
	}
	return 0;
}

int Byte_Buffer::peek_uint32(uint32_t &v) {
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint32_t t;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		v = be32toh(t);
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
#endif
	} else {
		LOG_ERROR("out of range");
		return -1;
	}
	return 0;
}

int Byte_Buffer::peek_uint64(uint64_t &v) {
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint64_t t, u;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		u = be64toh(t);
		memcpy(&v, &u, sizeof(v));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
#endif
	} else {
		LOG_ERROR("out of range");
		return -1;
	}
	return 0;
}

int Byte_Buffer::peek_double(double &v) {
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint64_t t, u;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		u = be64toh(t);
		memcpy(&v, &u, sizeof(v));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
#endif
	} else {
		LOG_ERROR("out of range");
		return -1;
	}
	return 0;
}

int Byte_Buffer::peek_bool(bool &v) {
	if (verify_read(sizeof(v))) {
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
	} else {
		LOG_ERROR("out of range");
		return -1;
	}
	return 0;
}

int Byte_Buffer::peek_string(std::string &str) {
	uint16_t len = 0;
	peek_uint16(len);
	if (len < 0) return -1;
	str.append(buffer_[read_index_ + sizeof(uint16_t)], len);
	return 0;
}

int Byte_Buffer::read_int8(int8_t &v) {
	if (verify_read(sizeof(v))) {
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
		read_index_ += sizeof(v);
	} else {
		LOG_ERROR("out of range");
		return -1;
	}
	return 0;
}

int Byte_Buffer::read_int16(int16_t &v) {
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint16_t t, u;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		u = be16toh(t);
		memcpy(&v, &u, sizeof(v));
		read_index_ += sizeof(v);
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
		read_index_ += sizeof(v);
#endif
	} else {
		LOG_ERROR("out of range");
		return -1;
	}
	return 0;
}

int Byte_Buffer::read_int32(int32_t &v) {
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint32_t t, u;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		u = be32toh(t);
		memcpy(&v, &u, sizeof(v));
		read_index_ += sizeof(v);
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
		read_index_ += sizeof(v);
#endif
	} else {
		LOG_ERROR("out of range");
		return -1;
	}
	return 0;
}

int Byte_Buffer::read_int64(int64_t &v) {
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint64_t t, u;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		u = be64toh(t);
		memcpy(&v, &u, sizeof(v));
		read_index_ += sizeof(v);
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
		read_index_ += sizeof(v);
#endif
	} else {
		LOG_ERROR("out of range");
		return -1;
	}
	return 0;
}

int Byte_Buffer::read_uint8(uint8_t &v) {
	if (verify_read(sizeof(v))) {
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
		read_index_ += sizeof(v);
	} else {
		LOG_ERROR("out of range");
		return -1;
	}
	return 0;
}

int Byte_Buffer::read_uint16(uint16_t &v) {
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint16_t t;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		v = be16toh(t);
		read_index_ += sizeof(v);
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
		read_index_ += sizeof(v);
#endif
	} else {
		LOG_ERROR("out of range");
		return -1;
	}
	return 0;
}

int Byte_Buffer::read_uint32(uint32_t &v) {
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint32_t t;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		v = be32toh(t);
		read_index_ += sizeof(v);
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
		read_index_ += sizeof(v);
#endif
	} else {
		LOG_ERROR("out of range");
		return -1;
	}
	return 0;
}

int Byte_Buffer::read_uint64(uint64_t &v) {
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint64_t t;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		v = be64toh(t);
		read_index_ += sizeof(v);
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
		read_index_ += sizeof(v);
#endif
	} else {
		LOG_ERROR("out of range");
		return -1;
	}
	return 0;
}

int Byte_Buffer::read_double(double &v) {
	if (verify_read(sizeof(v))) {
#ifdef BLOCK_BIG_ENDIAN
		uint64_t t, u;
		memcpy(&t, &(buffer_[read_index_]), sizeof(t));
		u = be64toh(t);
		memcpy(&v, &u, sizeof(v));
		read_index_ += sizeof(t);
#endif
#ifdef BLOCK_LITTLE_ENDIAN
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
		read_index_ += sizeof(v);
#endif
	} else {
		LOG_ERROR("out of range");
		return -1;
	}
	return 0;
}

int Byte_Buffer::read_bool(bool &v) {
	if (verify_read(sizeof(v))) {
		memcpy(&v, &(buffer_[read_index_]), sizeof(v));
		read_index_ += sizeof(v);
	} else {
		LOG_ERROR("out of range");
		return -1;
	}
	return 0;
}

int Byte_Buffer::read_string(std::string &str) {
	uint16_t len = 0;
	if (read_uint16(len) != 0)
		return -1;
	if (!verify_read(len))
		return -1;

	//设置字符串str的长度
	str.resize(len);
	memcpy((char *)str.c_str(), this->get_read_ptr(), len);
	read_index_ += len;
	return 0;
}

int Byte_Buffer::write_int8(int8_t v) {
	copy(&v, sizeof(v));
	return 0;
}

int Byte_Buffer::write_int16(int16_t v) {
#ifdef BLOCK_BIG_ENDIAN
	uint16_t t, u;
	t = *((uint16_t *)&v);
	u = htobe16(t);
	copy(&u, sizeof(u));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
	copy(&v, sizeof(v));
#endif
	return 0;
}

int Byte_Buffer::write_int32(int32_t v) {
#ifdef BLOCK_BIG_ENDIAN
	uint32_t t, u;
	t = *((uint32_t *)&v);
	u = htobe32(t);
	copy(&u, sizeof(u));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
	copy(&v, sizeof(v));
#endif
	return 0;
}

int Byte_Buffer::write_int64(int64_t v) {
#ifdef BLOCK_BIG_ENDIAN
	uint64_t t, u;
	t = *((uint64_t *)&v);
	u = htobe64(t);
	copy(&u, sizeof(u));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
	copy(&v, sizeof(v));
#endif
	return 0;
}

int Byte_Buffer::write_uint8(uint8_t v) {
	copy(&v, sizeof(v));
	return 0;
}

int Byte_Buffer::write_uint16(uint16_t v) {
#ifdef BLOCK_BIG_ENDIAN
	uint16_t t;
	t = htobe16(v);
	copy(&t, sizeof(t));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
	copy(&v, sizeof(v));
#endif
	return 0;
}

int Byte_Buffer::write_uint32(uint32_t v) {
#ifdef BLOCK_BIG_ENDIAN
	uint32_t t;
	t = htobe32(v);
	copy(&t, sizeof(t));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
	copy(&v, sizeof(v));
#endif
	return 0;
}

int Byte_Buffer::write_uint64(uint64_t v) {
#ifdef BLOCK_BIG_ENDIAN
	uint64_t t;
	t = htobe64(v);
	copy(&t, sizeof(t));
#endif

#ifdef BLOCK_LITTLE_ENDIAN
	copy(&v, sizeof(v));
#endif
	return 0;
}

int Byte_Buffer::write_double(double v) {
#ifdef BLOCK_BIG_ENDIAN
	uint64_t t, u;
	t = *((uint64_t *)&v);
	u = htobe64(t);
	copy(&u, sizeof(u));
#endif
#ifdef BLOCK_LITTLE_ENDIAN
	copy(&v, sizeof(v));
#endif
	return 0;
}

int Byte_Buffer::write_bool(bool v) {
	copy(&v, sizeof(v));
	return 0;
}

int Byte_Buffer::write_string(const std::string &str) {
	uint16_t len = str.length();
	write_uint16(len);
	copy(str.c_str(), str.length());
	return 0;
}

Byte_Buffer &Byte_Buffer::operator>>(int8_t &v) {
	read_int8(v);
	return *this;
}

Byte_Buffer &Byte_Buffer::operator>>(int16_t &v) {
	read_int16(v);
	return *this;
}

Byte_Buffer &Byte_Buffer::operator>>(int32_t &v) {
	read_int32(v);
	return *this;
}

Byte_Buffer &Byte_Buffer::operator>>(int64_t &v) {
	read_int64(v);
	return *this;
}

Byte_Buffer &Byte_Buffer::operator>>(uint8_t &v) {
	read_uint8(v);
	return *this;
}

Byte_Buffer &Byte_Buffer::operator>>(uint16_t &v) {
	read_uint16(v);
	return *this;
}

Byte_Buffer &Byte_Buffer::operator>>(uint32_t &v) {
	read_uint32(v);
	return *this;
}

Byte_Buffer &Byte_Buffer::operator>>(uint64_t &v) {
	read_uint64(v);
	return *this;
}

Byte_Buffer &Byte_Buffer::operator>>(double &v) {
	read_double(v);
	return *this;
}

Byte_Buffer &Byte_Buffer::operator>>(bool &v) {
	read_bool(v);
	return *this;
}

Byte_Buffer &Byte_Buffer::operator>>(std::string &v) {
	read_string(v);
	return *this;
}

Byte_Buffer &Byte_Buffer::operator<<(int8_t v) {
	write_int8(v);
	return *this;
}

Byte_Buffer &Byte_Buffer::operator<<(int16_t v) {
	write_int16(v);
	return *this;
}

Byte_Buffer &Byte_Buffer::operator<<(int32_t v) {
	write_int32(v);
	return *this;
}

Byte_Buffer &Byte_Buffer::operator<<(int64_t v) {
	write_int64(v);
	return *this;
}

Byte_Buffer &Byte_Buffer::operator<<(uint8_t v) {
	write_uint8(v);
	return *this;
}

Byte_Buffer &Byte_Buffer::operator<<(uint16_t v) {
	write_uint16(v);
	return *this;
}

Byte_Buffer &Byte_Buffer::operator<<(uint32_t v) {
	write_uint32(v);
	return *this;
}

Byte_Buffer &Byte_Buffer::operator<<(uint64_t v) {
	write_uint64(v);
	return *this;
}

Byte_Buffer &Byte_Buffer::operator<<(double v) {
	write_double(v);
	return *this;
}

Byte_Buffer &Byte_Buffer::operator<<(bool v) {
	write_bool(v);
	return *this;
}

Byte_Buffer &Byte_Buffer::operator<<(const std::string &v) {
	write_string(v);
	return *this;
}

void Byte_Buffer::read_head(Msg_Head &msg_head) {
	read_int32(msg_head.eid);
	read_int32(msg_head.cid);
	uint8_t pkg_protocol = 0;
	read_uint8(pkg_protocol);
	msg_head.pkg_type = pkg_protocol >> 4;
	msg_head.protocol = pkg_protocol & 0xf;

	if(msg_head.protocol == HTTP || msg_head.protocol == WEBSOCKET)
		return;

	read_uint8(msg_head.client_msg);
	if(msg_head.pkg_type == TYPE_PKG) {
		//type_pkg包长占四个字节，解包时，第一个字节写入pkg_protocol，第二个字节写入client_msg，后面两个字节无用
		read_index_ += 2;
	}
	read_uint8(msg_head.msg_id);
	if (msg_head.client_msg) {
		msg_head.msg_type = TCP_C2S;
	}
	else {
		read_uint8(msg_head.msg_type);
		read_uint32(msg_head.sid);
	}
}

void Byte_Buffer::write_head(const Msg_Head &msg_head) {
	if(msg_head.protocol == HTTP || msg_head.protocol == WEBSOCKET)
		return;

	if(msg_head.pkg_type == RPC_PKG) {
		write_uint16(0);
	} else {
		write_uint32(0);
	}
	write_uint8(msg_head.msg_id);
	if (msg_head.msg_type != TCP_S2C) {
		write_uint8(msg_head.msg_type);
		write_uint32(msg_head.sid);
	}
}

void Byte_Buffer::write_len(const Msg_Head &msg_head, bool compress) {
	if(msg_head.protocol == HTTP || msg_head.protocol == WEBSOCKET)
		return;

	int wr_idx = get_write_idx();
	if(msg_head.pkg_type == RPC_PKG) {
		int len = readable_bytes() - sizeof(uint16_t);
		len = MAKE_RPC_PKG_HEADER(len, compress);
		set_write_idx(get_read_idx());
		write_uint16(len);
	}
	else {
		int len = readable_bytes() - sizeof(uint32_t);
		len = MAKE_TYPE_PKG_HEADER(len, compress);
		set_write_idx(get_read_idx());
		write_uint32(len);
	}
	set_write_idx(wr_idx);
}

size_t Byte_Buffer::capacity(void) {
	return buffer_.capacity();
}

void Byte_Buffer::recycle_space(void) {
	if (max_use_times_ == 0)
		return;
	if (use_times_ >= max_use_times_) {
			std::vector<char> buffer_free(init_offset_ + init_size_);
 			buffer_.swap(buffer_free);
 			ensure_writable_bytes(init_offset_);
 			read_index_ = write_index_ = init_offset_;
			use_times_ = 0;
	}
}

bool Byte_Buffer::is_legal(void) {
	return read_index_ < write_index_;
}

bool Byte_Buffer::verify_read(size_t s) {
	return (read_index_ + s <= write_index_) && (write_index_ <= buffer_.size());
}

void Byte_Buffer::log_binary_data(size_t len) {
	size_t real_len = (len > readable_bytes()) ? readable_bytes() : len;
	size_t end_index = read_index_ + real_len;
	std::stringstream str_stream;
	char str_buf[32];
	for (size_t i = read_index_; i < end_index; ++i) {
		memset(str_buf, 0, sizeof(str_buf));
		snprintf(str_buf, sizeof(str_buf), "0x%02x", (uint8_t)buffer_[i]);
		str_stream << str_buf << " ";
	}
	LOG_DEBUG("log_binary_data:[%s]", str_stream.str().c_str());
}

#endif /* BYTE_BUFFER_H_ */
