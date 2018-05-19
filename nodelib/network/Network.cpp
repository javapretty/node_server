/*
 * Network.cpp
 *
 *  Created on: Oct 20, 2016
 *      Author: zhangyalei
 */

#include "Base_Function.h"
#include "Endpoint.h"
#include "Network.h"

Network::Network(void):
	endpoint_(0),
  reactor_(0),
  svc_map_(max_fd()),
  register_timer_(false),
  heartbeat_tv_(0)
{ }

Network::~Network(void) {
	fini();
}

int Network::init(Endpoint *endpoint, int heartbeat_timeout) {
	endpoint_ = endpoint;
	heartbeat_timeout_ = heartbeat_timeout;
	heartbeat_tv_ = Time_Value::gettimeofday() + Time_Value(heartbeat_timeout_, 0);

	if ((reactor_ = new Epoll_Watcher()) == 0) {
		LOG_FATAL("network new Epoll_Watcher error");
	}

	//注册发送数据定时器
	register_timer();
	return 0;
}

int Network::fini(void) {
	if (reactor_) {
		reactor_->end_loop();
		delete reactor_;
		reactor_ = 0;
	}
	return 0;
}

void Network::run_handler(void) {
	reactor_->loop();
}

int Network::process_drop(void) {
	int cid = 0;
	Svc *svc = nullptr;
	while (!drop_list_.empty()) {
		cid = drop_list_.pop_front();
		if ((svc = find_svc(cid)) != nullptr) {
			svc->set_closed(true);
			svc->unregister_network_handler();
		}
		drop_handler(cid);
	}

	return 0;
}

int Network::register_svc(Svc *svc) {
	reactor_->add(svc, EVENT_INPUT);
	GUARD(Svc_Lock, mon, svc_map_lock_);
	svc_map_.insert(std::make_pair(svc->get_cid(), svc));
	return 0;
}

int Network::unregister_svc(Svc *svc) {
	reactor_->remove(svc);
	GUARD(Svc_Lock, mon, svc_map_lock_);
	svc_map_.erase(svc->get_cid());
	return 0;
}

int Network::drop_handler(int cid) {
	LOG_TRACE("SHOULD NOT HERE");
	return 0;
}

Svc *Network::find_svc(int cid) {
	LOG_TRACE("SHOULD NOT HERE");
	return 0;
}

int Network::register_timer(void) {
	if (!register_timer_) {
		Time_Value send_interval = Time_Value(0, 10 * 1000);
		reactor_->add(this, EVENT_TIMEOUT, &send_interval);
		register_timer_ = true;
	}

	return 0;
}

int Network::handle_timeout(const Time_Value &tv) {
	process_drop();

	bool heartbeat_timeout = false;
	if(heartbeat_timeout_ > 0 && tv >= heartbeat_tv_) {
		heartbeat_tv_ = tv + Time_Value(heartbeat_timeout_, 0);
		heartbeat_timeout = true;
	}
	GUARD(Svc_Lock, mon, svc_map_lock_);
	for (Svc_Map::iterator iter = svc_map_.begin(); iter != svc_map_.end(); ++iter) {
		if(iter->second->closed()) {
			continue;
		}

		//发送svc数据
		if(iter->second->send_data()) {
			int ret = iter->second->handle_send();
			if(ret <= 0) {
				iter->second->set_send_data(false);
			}
		}

		if(heartbeat_timeout) {
			//心跳到时时候，如果svc是活的，就将svc设置为死的，否则关闭svc
			if(iter->second->alive()) {
				iter->second->set_alive(false);
			}
			else {
				LOG_WARN("heartbeat_timeout, close svc, endpoint:%s cid:%d fd:%d", endpoint_->endpoint_info().endpoint_name.c_str(), 
					iter->second->get_cid(), iter->second->get_fd());
				iter->second->handle_close();
			}
		}
	}
	return 0;
}
