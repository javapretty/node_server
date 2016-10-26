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

const static int MAX_CORE_NUM = 2000;

enum Msg_Type {
	C2S 		= 1,	//客户端发到服务器的消息
	S2C			= 2,	//服务器发到客户端的消息
	NODE_C2S	= 3,	//客户端经过gate中转发到后端服务器的消息
	NODE_S2C	= 4,	//后端服务器经过gate中转发到gate的消息
	NODE_MSG	= 5,	//服务器进程节点间通信的消息
};

struct Drop_Info {
	int endpoint_id;
	int drop_cid;
	int error_code;
	Time_Value drop_time;

	Drop_Info(void) : endpoint_id(0), drop_cid(-1), error_code(0), drop_time(Time_Value::gettimeofday()) {}
};

typedef std::vector<Endpoint_Info> Endpoint_List;
struct Node_Info {
	int node_type;					//节点类型
	int node_id;					//节点id
	std::string node_name;			//节点名称
	std::string script_path;		//启动的js脚本路径
	Endpoint_List server_list;	//服务器线程列表
	Endpoint_List connector_list;	//链接器线程列表
};

typedef std::vector<Node_Info> Node_List;
struct Node_Conf {
	Node_List node_list;

	void init(void);
};

#endif /* Node_DEFINE_H_ */
