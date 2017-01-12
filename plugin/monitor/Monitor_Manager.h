/*
 * Monitor_Manager.h
 *
 *  Created on: Dec 26, 2016
 *      Author: zhangyalei
 */

#ifndef GATE_MANAGER_H_
#define GATE_MANAGER_H_

#include "Buffer_List.h"
#include "Thread.h"
#include "Node_Define.h"

enum Msg_Id {
	SYNC_NODE_STACK_INFO = 3,
};

class Monitor_Manager: public Thread {
public:
	static Monitor_Manager *instance(void);

	void init(const Node_Info &node_info) { node_info_ = node_info; }
	virtual void run_handler(void);
	virtual int process_list(void);
	int sync_node_stack_info(int eid, int cid, int sid);

private:
	Monitor_Manager(void);
	virtual ~Monitor_Manager(void);
	Monitor_Manager(const Monitor_Manager &);
	const Monitor_Manager &operator=(const Monitor_Manager &);

private:
	static Monitor_Manager *instance_;
	Node_Info node_info_;			//节点信息
};

#define MONITOR_MANAGER Monitor_Manager::instance()

#endif /* GATE_MANAGER_H_ */
