/*
 * Buffer_Pool_Group.h
 *
 *  Created on: Dec 16,2015
 *      Author: zhangyalei
 */

#ifndef BUFFER_POOL_GROUP_H_
#define BUFFER_POOL_GROUP_H_

#include <vector>
#include "Byte_Buffer.h"
#include "Object_Pool.h"

class Buffer_Pool_Group {
public:
	typedef Object_Pool<Byte_Buffer, Mutex_Lock> Buffer_Pool;
	typedef std::vector<Buffer_Pool *> Pool_Group;

	Buffer_Pool_Group(size_t group_size = 1)
	: group_size_(group_size), x_(0) {
		init();
	}

	virtual ~Buffer_Pool_Group(void) {
		fini();
	}

	int init(void) {
		pool_group_.clear();

		Buffer_Pool *pool = 0;
		for (size_t i = 0; i < group_size_; ++i) {
			if ((pool = new Buffer_Pool) == 0) {
				LOG_FATAL("new return 0");
			}
			pool_group_.push_back(pool);
		}
		return 0;
	}

	int fini(void) {
		for (Pool_Group::iterator it = pool_group_.begin(); it != pool_group_.end(); ++it) {
			(*it)->clear();
		}
		pool_group_.clear();
		return 0;
	}

	Byte_Buffer *pop_buffer(int cid) {
		return pool_group_[cid % group_size_]->pop();
	}

	int push_buffer(int cid, Byte_Buffer *buffer) {
		if (!buffer) {
			LOG_TRACE("buffer == 0");
			return -1;
		}
		buffer->recycle_space();
		return pool_group_[cid % group_size_]->push(buffer);
	}

	void shrink_all(void) {
		for (size_t i = 0; i < group_size_; ++i) {
			pool_group_[i]->shrink_all();
		}
	}

	void dump_info(void) {
		for (size_t i = 0; i < group_size_; ++i) {
			LOG_DEBUG("***pool_group_[%u]: ", i);
			pool_group_[i]->debug_info();
		}
	}

	void dump_size(void) {
		size_t sum_size = 0;
		for (Pool_Group::iterator group_it = pool_group_.begin(); group_it != pool_group_.end(); ++group_it) {
			sum_size += (* group_it)->sum_size();
		}
	}

	void get_buffer_group(std::vector<Buffer_Group_Info> &group_list) {
		Buffer_Group_Info group_info;
		for (size_t i = 0; i < group_size_; ++i) {
			group_info.free_size = pool_group_[i]->free_obj_list_size();
			group_info.used_size = pool_group_[i]->used_obj_list_size();
			group_info.sum_bytes = pool_group_[i]->sum_size();
			group_list.push_back(group_info);
		}
	}

private:
	Pool_Group pool_group_;
	size_t group_size_;
	int x_;
};

#endif /* BUFFER_POOL_GROUP_H_ */
