/*
 * Node_Timer.cpp
 *
 *  Created on: Sep 20, 2016
 *      Author: zhangyalei
 */

#include "V8_Manager.h"
#include "Node_Manager.h"
#include "Node_Timer.h"

Timer_Handler::Timer_Handler(void) { }

Timer_Handler::~Timer_Handler(void) { }

int Timer_Handler::handle_timeout(const Time_Value &tv) {
	NODE_MANAGER->push_tick(tv.sec());
	NODE_TIMER->tick(tv);
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
	Time_Value timeout_tv(0, 100 * 1000);
	watcher_.add(&timer_handler_, EVENT_TIMEOUT, &timeout_tv);
	watcher_.loop();
}

void Node_Timer::register_handler(int timer_id, int internal, int first_tick) {
	V8_Timer *timer = timer_pool_.pop();
	timer->timer_id = timer_id;
	timer->interval = internal;
	timer->next_tick = Time_Value::gettimeofday() + Time_Value(first_tick);
	timer_queue_.push(timer);
}

int Node_Timer::tick(const Time_Value &now){
	while(!timer_queue_.empty() && (now > timer_queue_.top()->next_tick)) {
		V8_Timer *timer = timer_queue_.top();
		timer_queue_.pop();
		V8_MANAGER->push_timer(timer->timer_id);
		timer->next_tick += Time_Value(timer->interval / 1000, timer->interval % 1000 * 1000);
		timer_queue_.push(timer);
	}
	return 0;
}
