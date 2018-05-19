/*
 * Buffer_List.h
 *
 *  Created on: Dec 16,2015
 *      Author: zhangyalei
 */

#ifndef BUFFER_LIST_H_
#define BUFFER_LIST_H_

#include <sys/uio.h>
#include <limits.h>
#include <list>
#include "Byte_Buffer.h"

template <typename LOCK = NULL_LOCK>
class Buffer_List {
public:
	typedef std::list<Byte_Buffer *> BList;

	Buffer_List(void) : size_(0), max_size_(0) {
		list_.clear();
	}

	virtual ~Buffer_List(void) { }

	int push_back(Byte_Buffer *buffer) {
		int ret = -1;
		if (!buffer) {
			LOG_ERROR("buffer is null");
			ret = -1;
		}

		//Mutex_Guard<LOCK> mon(this->lock_);
		GUARD(LOCK, mon, this->lock_);
		if (max_size_ > 0 &&  size_ > max_size_) {
			ret = -1;
		} else {
			++size_;
			list_.push_back(buffer);
			ret = 0;
		}
		return ret;
	}

	Byte_Buffer *front(void) {
		GUARD(LOCK, mon, this->lock_);
		if (list_.empty())
			return nullptr;
		else
			return list_.front();
	}

	Byte_Buffer *pop_front(void) {
		GUARD(LOCK, mon, this->lock_);
		if (list_.empty()) {
			return nullptr;
		} else {
			Byte_Buffer *buffer = list_.front();
			list_.pop_front();
			--size_;
			return buffer;
		}
	}

	bool empty(void) {
		GUARD(LOCK, mon, this->lock_);
		return list_.empty();
	}

	void clear(void) {
		GUARD(LOCK, mon, this->lock_);
		size_ = 0;
		list_.clear();
	}

	void swap(BList &blist) {
		GUARD(LOCK, mon, this->lock_);
		size_ = blist.size();
		list_.swap(blist);
	}

	size_t size(void) {
		GUARD(LOCK, mon, this->lock_);
		return size_;
	}

	int construct_iov(std::vector<iovec> &iov_vec, std::vector<Byte_Buffer *> &iov_buff, size_t &sum_bytes) {
		GUARD(LOCK, mon, this->lock_);

		int iovcnt = size_ > IOV_MAX ? IOV_MAX : size_; 	/// IOV_MAX为XPG宏, 需要包含limits.h, 一般为1024, C中需加-D_XOPEN_SOURCE参数
		if (iovcnt == 0)
			return 0;

		struct iovec iov;
		int i = 0;
		sum_bytes = 0;
		for (BList::iterator it = list_.begin(); i < iovcnt && it != list_.end(); ++i, ++it) {
			iov.iov_base = (*it)->get_read_ptr();
			iov.iov_len = (*it)->readable_bytes();
			sum_bytes += (*it)->readable_bytes();
			iov_vec.push_back(iov);
			iov_buff.push_back(*it);
		}
		return iovcnt;
	}

	void pop_front(size_t num, size_t offset = 0) {
		GUARD(LOCK, mon, this->lock_);

		if (num > size_) {
			LOG_ERROR("num = %u, size_ = %u", num, size_);
			return;
		}

		size_t i = 0;
		for ( ; i < num; ++i) {
			list_.pop_front();
		}
		size_ -= i;

		if (offset > 0) {
			Byte_Buffer *head = list_.front();
			if ((int)offset > head->get_write_idx()) {
				LOG_ERROR("offset = %ul, read_index = %ul, write_index = %ul.", offset, head->get_read_idx(), head->get_write_idx());
				return;
			}
			head->set_read_idx(head->get_read_idx() + offset);
		}
	}

	/// list_空间小于2, 或异常返回0; 合并成功返回空闲出来的buf指针.
	Byte_Buffer *merge_first_second(void) {
		GUARD(LOCK, mon, this->lock_);

		if (size_ < 2)
			return nullptr;

		Byte_Buffer *front_buf = list_.front();
		BList::iterator second_it = ++(list_.begin());
		Byte_Buffer *second_buf = *second_it;
		list_.erase(second_it);
		--size_;

		//eid和cid各占4个字节
		second_buf->set_read_idx(second_buf->get_read_idx() + sizeof(int32_t) * 2);

		if (second_buf->readable_bytes() <= 0) {
			LOG_ERROR("second_buf->readable_bytes() <= 0");
			second_buf->reset();
		} else {
			front_buf->copy(second_buf);
		}

		return second_buf;
	}

	void set_max_size(size_t max) {
		max_size_ = max;
	}

private:
	BList list_;
	size_t size_;
	size_t max_size_;
	LOCK lock_;
};

#endif /* BUFFER_LIST_H_ */
