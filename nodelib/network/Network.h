/*
 * NetWork.h
 *
 *  Created on: Oct 20, 2016
 *      Author: zhangyalei
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include "List.h"
#include "Thread.h"
#include "Epoll_Watcher.h"
#include "Svc.h"

class Endpoint;
class Network: public Thread, public Event_Handler {
	typedef Mutex_Lock Svc_Lock;
	typedef std::unordered_map<int, Svc *> Svc_Map;
	typedef List<int, Mutex_Lock> Drop_List;
public:
	Network(void);
	virtual ~Network(void);

	int init(Endpoint *endpoint, int heartbeat_intverval);
	int fini(void);

	virtual void run_handler(void);

	inline int push_drop(int cid) {
		drop_list_.push_back(cid);
		reactor_->notify();
		return 0;
	}
	int process_drop(void);

	int register_svc(Svc *svc);
	int unregister_svc(Svc *svc);

	virtual int drop_handler(int cid);
	virtual Svc *find_svc(int cid);

	int register_timer(void);
	virtual int handle_timeout(const Time_Value &tv);

protected:
	Endpoint *endpoint_;

private:
	Epoll_Watcher *reactor_;
	Drop_List drop_list_;		//掉线cid列表

	Svc_Lock svc_map_lock_;		//svc map锁
	Svc_Map svc_map_;			//svc信息

	bool register_timer_;		//是否注册定时器
	int heartbeat_timeout_;		//心跳超时时间
	Time_Value heartbeat_tv_;	//心跳tick
};

#endif /* NETWORK_H_ */
