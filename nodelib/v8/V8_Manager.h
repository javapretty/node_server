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
#include "Buffer_List.h"
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
	typedef Buffer_List<Mutex_Lock> Data_List;
	typedef List<std::string, Mutex_Lock> String_List;
	typedef List<int, Mutex_Lock> Int_List;
	typedef List<Drop_Info, Mutex_Lock> Drop_List;
	typedef std::unordered_map<const char *, void *> Plugin_Handle_Map;
public:
	static V8_Manager *instance(void);

	int init(const Node_Info &node_info);
	int fini(void);

	virtual void run_handler(void);
	virtual int process_list(void);

	int init_script(Local<Context> context);
	int process_hotupdate(Local<Context> context);
	int process_drop_eid(Local<Context> context);
	int process_drop_cid(Local<Context> context);
	int process_buffer(Local<Context> context, Byte_Buffer *buffer);
	int process_timer(Local<Context> context);
	int call_js_func(Local<Context> context, int argc, Local<Value> argv[], const char* func_name);
	int drop_info_tick(Time_Value &now);
	std::string get_v8_stack(void);

	inline void push_hotupdate_file(const std::string &file_path) {
		notify_lock_.lock();
		hotupdate_file_list_.push_back(file_path);
		notify_lock_.signal();
		notify_lock_.unlock();
	}
	inline void push_drop_eid(int eid) {
		notify_lock_.lock();
		drop_eid_list_.push_back(eid);
		notify_lock_.signal();
		notify_lock_.unlock();
	}
	inline void push_drop_cid(int cid) {
		notify_lock_.lock();
		drop_cid_list_.push_back(cid);
		notify_lock_.signal();
		notify_lock_.unlock();
	}
	inline void push_buffer(Byte_Buffer *buffer) {
		notify_lock_.lock();
		buffer_list_.push_back(buffer);
		notify_lock_.signal();
		notify_lock_.unlock();
	}
	inline void push_timer(int timer_id) {
		notify_lock_.lock();
		timer_list_.push_back(timer_id);
		notify_lock_.signal();
		notify_lock_.unlock();
	}
	inline void push_tick(int tick_time) {
		notify_lock_.lock();
		tick_list_.push_back(tick_time);
		notify_lock_.signal();
		notify_lock_.unlock();
	}
	inline void push_drop(int eid, int cid) { drop_info_list_.push_back(Drop_Info(eid, cid)); }
	inline Data_List &buffer_list(void) { return buffer_list_; }
	inline int drop_cid(void) { return drop_cid_; }
	inline const Msg_Head &msg_head(void) { return msg_head_; }
	inline int timer_id(void) { return timer_id_; }
	inline Plugin_Handle_Map &plugin_handle_map(void) { return plugin_handle_map_; }

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

	Node_Info node_info_;				//节点信息
	String_List hotupdate_file_list_;	//热更新文件列表
	Int_List drop_eid_list_;			//网络层掉线的eid列表
	Int_List drop_cid_list_;			//网络层掉线cid列表
	Data_List buffer_list_;				//消息列表
	Int_List timer_list_;				//定时器编号列表(js定时器)
	Int_List tick_list_;				//定时器tick列表(C++定时器)
	Drop_List drop_info_list_; 			//逻辑层掉线信息列表
	Time_Value drop_info_tick_;			//逻辑层掉线处理tick
	Time_Value gc_tick_;				//v8 gc处理tick

	std::string file_path_;				//热更新文件路径	
	int drop_eid_;						//掉线eid
	int drop_cid_;						//掉线cid
	Msg_Head msg_head_;					//消息头数据
	int timer_id_;						//定时器id

	void (*push_buffer_func)(Byte_Buffer *);//插件push过滤的buffer
	void (*push_tick_func)(int);			//插件push定时器tick
	Plugin_Handle_Map plugin_handle_map_;	//插件列表
};

#define V8_MANAGER V8_Manager::instance()

#endif /* V8_MANAGER_H_ */
