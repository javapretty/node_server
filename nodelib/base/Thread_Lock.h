/*
 * Thread_Lock.h
 *
 *  Created on: Dec 16,2015
 *      Author: zhangyalei
 *
 */

#ifndef THREAD_LOCK_H_
#define THREAD_LOCK_H_

#include <stdint.h>
#include <pthread.h>

//////////////////////空锁/////////////////////
class Null_Lock {
public:
	Null_Lock(void) { }

	virtual ~Null_Lock(void) { }

	int acquire(void) {
		return 0;
	}

	int release(void) {
		return 0;
	}

	int acquire_read(void) {
		return 0;
	}

	int acquire_write(void) {
		return 0;
	}

private:
	  void operator=(const Null_Lock &);
	  Null_Lock(const Null_Lock &);
};

////////////////////互斥锁///////////////////////
class Mutex_Lock {
public:
	Mutex_Lock(void) {
		::pthread_mutex_init(&lock_, NULL);
	}

	virtual ~Mutex_Lock(void) {
		::pthread_mutex_destroy(&lock_);
	}

	int acquire(void) {
		return ::pthread_mutex_lock(&lock_);
	}

	int release(void) {
		return ::pthread_mutex_unlock(&lock_);
	}

	int acquire_read(void) {
		return this->acquire();
	}

	int acquire_write(void) {
		return this->acquire();
	}

private:
	  void operator=(const Mutex_Lock &);
	  Mutex_Lock(const Mutex_Lock &);

private:
	pthread_mutex_t lock_;
};

////////////////////读写锁///////////////////////
class RW_Lock {
public:
	RW_Lock(void) {
		::pthread_rwlock_init(&lock_, NULL);
	}

	virtual ~RW_Lock(void) {
		::pthread_rwlock_destroy(&lock_);
	}

	int acquire(void) {
		return ::pthread_rwlock_wrlock(&lock_);
	}

	int release(void) {
		return ::pthread_rwlock_unlock(&lock_);
	}

	int acquire_read(void) {
		return ::pthread_rwlock_rdlock(&lock_);
	}

	int acquire_write(void) {
		return ::pthread_rwlock_wrlock(&lock_);
	}

private:
	void operator=(const RW_Lock &);
	RW_Lock(const RW_Lock &);

private:
	pthread_rwlock_t lock_;
};

////////////////////读锁///////////////////////
class RE_Lock {
public:
	RE_Lock(void) {
		::pthread_mutexattr_init(&attr);
		::pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		::pthread_mutex_init(&lock_, &attr);
	}

	virtual ~RE_Lock(void) {
		::pthread_mutex_destroy(&lock_);
		::pthread_mutexattr_destroy(&attr);
	}

	int acquire(void) {
		return ::pthread_mutex_lock(&lock_);
	}

	int release(void) {
		return ::pthread_mutex_unlock(&lock_);
	}

	int acquire_read(void) {
		return this->acquire();
	}

	int acquire_write(void) {
		return this->acquire();
	}

private:
	void operator=(const RE_Lock &);
	RE_Lock(const RE_Lock &);

private:
	pthread_mutexattr_t attr;
	pthread_mutex_t lock_;
};

////////////////////自旋锁///////////////////////
class Spin_Lock {
public:
	Spin_Lock(void) {
		::pthread_spin_init(&lock_, 0);
	}

	virtual ~Spin_Lock(void) {
		::pthread_spin_destroy(&lock_);
	}

	int acquire(void) {
		return ::pthread_spin_lock(&lock_);
	}

	int release(void) {
		return ::pthread_spin_unlock(&lock_);
	}

	int acquire_read(void) {
		return this->acquire();
	}

	int acquire_write(void) {
		return this->acquire();
	}

private:
	void operator=(const RE_Lock &);
	Spin_Lock(const Spin_Lock &);

private:
	pthread_spinlock_t lock_;
};

//////////////////////条件通知锁/////////////////////
class Notify_Lock
{
public:
	Notify_Lock(){
		::pthread_mutexattr_init(&mutexattr_);
		::pthread_mutexattr_settype(&mutexattr_, PTHREAD_MUTEX_RECURSIVE);
		::pthread_mutex_init(&mutex_, &mutexattr_);
		::pthread_cond_init(&cond_, NULL);
	}

	~Notify_Lock(){
		::pthread_mutexattr_destroy(&mutexattr_);
		::pthread_mutex_destroy(&mutex_);
		::pthread_cond_destroy(&cond_);
	}

	void lock() { pthread_mutex_lock(&mutex_); }
	void unlock() { pthread_mutex_unlock(&mutex_); }
	void wait() { pthread_cond_wait(&cond_, &mutex_); }
	void signal() { pthread_cond_signal(&cond_); }

private:
	pthread_mutex_t mutex_;
	pthread_mutexattr_t	mutexattr_;
	pthread_cond_t cond_;
};

//////////////////条件变量////////////////////////
class Condition
{
public:
	Condition() {
		::pthread_mutex_init(&mutex_, NULL);
		::pthread_cond_init(&cond_, NULL);
	}

	~Condition() {
		::pthread_mutex_destroy(&mutex_);
		::pthread_cond_destroy(&cond_);
	}

	void wait() { pthread_cond_wait(&cond_, &mutex_); }
	/*
	 * nWaitTime ms
	 * if recv a signal then return true;
	 * else return false;
	 */
	bool waitTime(uint64_t nWaitTime) {
		uint64_t nTime = nWaitTime * 1000000;
		struct timespec sTime;
		uint64_t nSec = nTime / (1000000000);
		uint64_t nNsec = nTime % (1000000000);
		sTime.tv_sec = time(NULL) + (uint32_t)nSec;
		sTime.tv_nsec = (uint32_t)nNsec;
		if(ETIMEDOUT == pthread_cond_timedwait(&cond_, &mutex_, &sTime)) {
			return false;
		}
		return true;
	}

	void notify() { pthread_cond_signal(&cond_); }
	void notifyAll() { pthread_cond_broadcast(&cond_); }

private:
	pthread_mutex_t mutex_;
	pthread_cond_t cond_;
};

//////////////////////////////////////////
#define NULL_LOCK Null_Lock
#define RW_LOCK RW_Lock
#define RE_LOCK RE_Lock
#define MUTEX_LOCK Mutex_Lock

#endif /* THREAD_LOCK_H_ */
