/*
 * Node_Timer.cpp
 *
 *  Created on: Sep 20, 2016
 *      Author: zhangyalei
 */

#include "Node_Manager.h"
#include "Node_Timer.h"

Timer_Handler::Timer_Handler(void) { }

Timer_Handler::~Timer_Handler(void) { }

int Timer_Handler::handle_timeout(const Time_Value &tv) {
	NODE_MANAGER->push_tick(tv.sec());
	return 0;
}

Node_Timer::Node_Timer(void) { }

Node_Timer::~Node_Timer(void) { }

Node_Timer *Node_Timer::instance_;

Node_Timer *Node_Timer::instance(void) {
	if (instance_ == 0)
		instance_ = new Node_Timer;
	return instance_;
}

void Node_Timer::run_handler(void) {
	register_handler();
	watcher_.loop();
}

Epoll_Watcher &Node_Timer::watcher(void) {
	return watcher_;
}

void Node_Timer::register_handler(void) {
	Time_Value timeout_tv(0, 100 * 1000);
	watcher_.add(&timer_handler_, EVENT_TIMEOUT, &timeout_tv);
}
