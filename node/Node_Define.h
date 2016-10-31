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

enum Msg_Type {
	C2S 			= 1,	//客户端发到服务器的消息
	S2C				=	2,	//服务器发到客户端的消息
	NODE_C2S	= 3,	//客户端经过gate中转发到后端服务器的消息
	NODE_S2C	= 4,	//后端服务器经过gate中转发到gate的消息
	NODE_MSG	= 5,	//服务器进程节点间通信的消息
	HTTP_MSG 	= 6,	//http消息
};

enum Node_Type {
	CENTER_SERVER 	= 1,
	GATE_SERVER 		= 2,
	DATA_SERVER 		= 3,
	DB_SERVER				= 4,
	LOG_SERVER 			= 5,
	MASTER_SERVER 	= 6,
	PUBLIC_SERVER 	= 7,
	GAME_SERVER 		= 8,
};

struct Session {
	int client_eid;	//gate向client发消息的端点id
	int client_cid;	//client与gate连接的cid
	int game_eid;		//gate向game发消息的端点id
	int game_cid;		//game与gate连接的cid
	int sid;					//gate生成的全局唯一session_id
};

struct Drop_Info {
	int eid;
	int cid;
	Time_Value drop_time;

	Drop_Info(void) : eid(0), cid(-1), drop_time(Time_Value::gettimeofday()) {}
};

typedef std::vector<Endpoint_Info> Endpoint_List;
struct Node_Info {
	int node_type;									//节点类型
	int node_id;										//节点id
	std::string node_name;					//节点名称
	std::string node_ip;						//节点ip
	std::string script_path;			//启动的js脚本路径
	std::vector<std::string> plugin_list; //插件列表
	Endpoint_List endpoint_list;	//线程列表

	void serialize(Block_Buffer &buffer);
	void deserialize(Block_Buffer &buffer);
	void reset(void);
};

#endif /* Node_DEFINE_H_ */
