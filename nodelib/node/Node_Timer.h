/*
 * Node_Timer.h
 *
 *  Created on: Sep 20, 2016
 *      Author: zhangyalei
 */

#ifndef NODE_TIMER_H_
#define NODE_TIMER_H_

#include "Epoll_Watcher.h"
#include "Thread.h"
#include "Priority_Queue.h"

struct V8_Timer {
	int timer_id; 			//js层定时器编号
	int interval; 			//定时时间间隔(毫秒为单位)
	Time_Value next_tick;	//下一次执行时间
};

class V8_Timer_Compare {
public:
	inline bool operator()(V8_Timer *t1, V8_Timer *t2) {
		return t1->next_tick > t2->next_tick;
	}
};

class Timer_Handler: public Event_Handler {
public:
	Timer_Handler(void);
	virtual ~Timer_Handler(void);

	virtual int handle_timeout(const Time_Value &tv);
};

class Node_Timer: public Thread {
	typedef Object_Pool<V8_Timer, Mutex_Lock> Timer_Pool;
	typedef Priority_Queue<V8_Timer*, V8_Timer_Compare, Mutex_Lock> Timer_Queue;
public:
	static Node_Timer *instance(void);
	virtual void run_handler(void);

	void register_handler(int timer_id, int internal, int first_tick); //注册js层定时器
	int tick(const Time_Value &now);

private:
	Node_Timer(void);
	virtual ~Node_Timer(void);
	Node_Timer(const Node_Timer &);
	const Node_Timer &operator=(const Node_Timer &);

private:
	static Node_Timer *instance_;
	Epoll_Watcher watcher_;
	Timer_Handler timer_handler_;
	Timer_Pool timer_pool_;
	Timer_Queue timer_queue_;
};

#define NODE_TIMER Node_Timer::instance()

#endif /* NODE_TIMER_H_ */
