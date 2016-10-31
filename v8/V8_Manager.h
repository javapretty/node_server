/*
 * V8_Manager.h
 *
 *  Created on: Jan 22, 2016
 *      Author: zhangyalei
 */

#ifndef V8_MANAGER_H_
#define V8_MANAGER_H_

#include <dlfcn.h>
#include <string.h>
#include <unordered_map>
#include "include/v8.h"
#include "include/libplatform/libplatform.h"
#include "Thread.h"
#include "List.h"
#include "Node_Define.h"

using namespace v8;

class Byte_Buffer;
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
	typedef List<int, Thread_Mutex> Int_List;
	typedef std::unordered_map<const char *, void *> Plugin_Handle_Map;
public:
	static V8_Manager *instance(void);
	virtual void run_handler(void);
	virtual int process_list(void);

	int init(const Node_Info &node_info);
	int fini(void);

	inline void push_timer(int timer_id) { timer_list_.push_back(timer_id); }
	inline Plugin_Handle_Map &plugin_handle_map() {return plugin_handle_map_;}

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
	Node_Info node_info_;		//节点信息
	Int_List timer_list_;		//定时器编号
	Plugin_Handle_Map plugin_handle_map_;
};

#define V8_MANAGER V8_Manager::instance()

#endif /* V8_MANAGER_H_ */
