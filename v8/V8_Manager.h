/*
 * V8_Manager.h
 *
 *  Created on: Jan 22, 2016
 *      Author: zhangyalei
 */

#ifndef V8_MANAGER_H_
#define V8_MANAGER_H_

#include <string.h>
#include "include/v8.h"
#include "include/libplatform/libplatform.h"
#include "Thread.h"

using namespace v8;

class Block_Buffer;
class ArrayBufferAllocator : public ArrayBuffer::Allocator {
 public:
  virtual void* Allocate(size_t length) {
    void* data = AllocateUninitialized(length);
    return data == nullptr ? data : memset(data, 0, length);
  }
  virtual void* AllocateUninitialized(size_t length) { return malloc(length); }
  virtual void Free(void* data, size_t) { free(data); }
};

class V8_Manager: public Thread {
public:
	static V8_Manager *instance(void);
	virtual void run_handler(void);
	virtual int process_list(void);

	int init(const char *script_path);
	int fini(void);

private:
	V8_Manager(void);
	virtual ~V8_Manager(void);
	V8_Manager(const V8_Manager &);
	const V8_Manager &operator=(const V8_Manager &);

private:
	static V8_Manager *instance_;
	Platform* platform_;
	Isolate* isolate_;
	Global<Context> context_;
	std::string script_path_;
};

#define V8_MANAGER V8_Manager::instance()

#endif /* V8_MANAGER_H_ */
