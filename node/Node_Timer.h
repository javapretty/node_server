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

class Timer_Handler: public Event_Handler {
public:
	Timer_Handler(void);
	virtual ~Timer_Handler(void);

	virtual int handle_timeout(const Time_Value &tv);
};

class Node_Timer: public Thread {
public:
	static Node_Timer *instance(void);
	virtual void run_handler(void);

	Epoll_Watcher &watcher(void);
	void register_handler(void);

private:
	Node_Timer(void);
	virtual ~Node_Timer(void);
	Node_Timer(const Node_Timer &);
	const Node_Timer &operator=(const Node_Timer &);

private:
	static Node_Timer *instance_;
	Epoll_Watcher watcher_;
	Timer_Handler timer_handler_;
};

#define NODE_TIMER Node_Timer::instance()

#endif /* NODE_TIMER_H_ */
