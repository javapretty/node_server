/*
 * Node_Define.h
 *
 *  Created on: Sep 20, 2016
 *      Author: zhangyalei
 */

#ifndef Node_DEFINE_H_
#define Node_DEFINE_H_

#include <vector>
#include "Base_Define.h"
#include "Time_Value.h"

struct Drop_Info {
	int eid;
	int cid;
	Time_Value drop_time;

	Drop_Info(void) : eid(0), cid(-1), drop_time(Time_Value::gettimeofday()) {}
	Drop_Info(int eid_, int cid_) : eid(eid_), cid(cid_), drop_time(Time_Value::gettimeofday()) {}
};

//消息过滤器，被过滤的消息，不会抛给脚本层，直接由C++处理
struct Msg_Filter {
	int msg_type;				//消息类型
	int min_msg_id;			//最小消息id
	int max_msg_id;			//最大消息id
};

typedef std::vector<Msg_Filter> Filter_List;
typedef std::vector<Endpoint_Info> Endpoint_List;
struct Node_Info {
	int node_type;				//节点类型
	int node_id;				//节点id
	int endpoint_gid;			//端点组id
	int max_session_count;		//单个进程最多能处理的session上限
	std::string node_name;		//节点名称
	std::string node_ip;		//节点ip
	std::string global_script;	//js全局数据脚本路径
	std::string main_script;	//js主脚本路径
	std::vector<std::string> hotupdate_list;//可以热更新的文件夹列表
	std::vector<std::string> plugin_list;	//插件列表
	Filter_List filter_list;	//消息过滤器列表
	Endpoint_List endpoint_list;//线程列表

	void serialize(Bit_Buffer &buffer);
	void deserialize(Bit_Buffer &buffer);
	void reset(void);
};

struct Node_Status {
	int start_time;				//服务器开启时间
	int64_t total_send;			//总共发送字节数
	int64_t total_recv;			//总共接收字节数
	int send_per_sec;			//每秒发生字节数
	int recv_per_sec;			//每秒接收字节数
	int task_count;				//当前任务数量
};

#endif /* Node_DEFINE_H_ */
