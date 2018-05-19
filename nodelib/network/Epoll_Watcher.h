/*
 * Epoll_Watcher.h
  *
 *  Created on: Dec 16,2015
 *      Author: zhangyalei
  *
  *  底层使用epoll实现，主要功能为io事件回调、心跳超时机制、毫秒级定时器
  *
*/

#ifndef EPOLL_WATCHER_H_
#define EPOLL_WATCHER_H_

#include <queue>
#include <vector>
#include <unordered_map> 	//使用std::unordered_map, 需添加-std=c++0x参数 -D__GXX_EXPERIMENTAL_CXX0X__
#include "Event_Handler.h"
#include "Object_Pool.h"

class Epoll_Watcher : public Event_Handler {
	//Timer事件
	typedef std::priority_queue<Event_Handler *, std::vector<Event_Handler *>, Event_Handler::greater> Timer_Queue;
	typedef std::unordered_set<Event_Handler *> Event_Timer_Set;
	//IO事件
	typedef std::vector<Event_Handler *> Event_Map;
	//心跳事件
	typedef std::unordered_map<int, Event_Handler *> Heart_Map;
	typedef RE_Lock Mutex;
public:
	/**
	 * type可以指定IO附带心跳机制: WITH_IO_HEARTBEAT
	 * heart_second制定IO心跳超时时间
	 */
	Epoll_Watcher(int type = 0, int heart_second = 30);
	virtual ~Epoll_Watcher(void);

	/**
	 * 添加监听事件
	 *	op可以为EVENT_INPUT, EVENT_OUTPUT, EVENT_TIMEOUT等位集合
	 *	EVENT_INPUT, EVENT_OUTPUT, EVENT_ONCE_IO_IN, EVENT_ONCE_IO_OUT为监听IO事件
	 *	EVENT_TIMEOUT, EVENT_ONCE_TIMER为监听定时器事件
	 *	tv为定时器事件指定的间隔时间
	 */
	virtual int add(Event_Handler *evh, int op, Time_Value *tv = NULL);
	//移除eh的IO/定时器事件监听
	virtual int remove(Event_Handler *eh);

	//开始事件监听
	virtual int loop(void);
	//结束事件监听循环
	virtual int end_loop(void);

	//使阻塞的epoll_wait返回, 执行新的定时器处理函数
	virtual int notify(void);

private:
	//开启epoll
	int open(void);
	//开启io事件
	int io_open(void);
	//开始定时器事件
	int timer_open(void);

	//添加io事件
	int add_io(Event_Handler *evh, int op);
	//添加定时器事件
	int add_timer(Event_Handler *evh, int op, Time_Value *tv);
	//移除io事件
	int remove_io(Event_Handler *evh);
	//移除定时器事件
	int remove_timer(Event_Handler *evh);

	//计算epoll_wait的等待时间
	int calculate_timeout(void);
	//处理已到期定时器事件
	void process_timer_event(void);
	//处理IO事件
	void watcher_loop(void);

	//EPOLLIN事件处理
	int handle_input(void);
	//心跳定时器处理
	int handle_timeout(const Time_Value &tv);
	//下次心跳索引
	inline int next_heart_idx(void) { return (io_heart_idx_ + 1) % 2; }

private:
	int type_;			//表示epoll自身事件类型，是否监听io心跳事件
	bool end_flag_;		//epoll监听结束标记
	int epfd_;			//epoll句柄
	int max_events_;	//最大监听事件数量
	struct epoll_event *events_;

	Event_Map pending_io_map_;			//io事件map
	int io_heart_idx_, heart_second_;	//心跳索引
	Heart_Map io_heart_map_[2];			//心跳io事件
	Mutex io_lock_;

	Timer_Queue tq_; 			//时间优先队列(最小堆)
	Event_Timer_Set timer_set_;	//定时器集合
	Mutex tq_lock_;

	int pipe_fd_[2]; 	//notify管道
};

#endif /* EPOLL_WATCHER_H_ */
