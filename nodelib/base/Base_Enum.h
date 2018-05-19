/*
 * Base_Enum.h
 *
 *  Created on: Sep 20, 2016
 *      Author: zhangyalei
 */

#ifndef BASE_ENUM_H_
#define BASE_ENUM_H_

//node命名管道路径
#define NODE_FIFO "/tmp/node_fifo"

//包是否压缩
#define IS_PACKAGE_COMPRESSED(l)	((l)&(0x1<<5))

//rpc package header size: 2bytes, length: 13bits, 0-8191 bytes
#define MAKE_RPC_PKG_HEADER(l,comp)     (RPC_PKG << 6 | ((comp&0x1)<<5) | (((l)&0x1f00)>>8) | (((l)&0x00ff)<<8))
#define GET_RPC_PKG_LENGTH(h,l)         ((((h)&0x1f)<<8) | (((l)&0xff00)>>8))

//type package header size: 4bytes, length: 29bits, 0-536870911 bytes
#define MAKE_TYPE_PKG_HEADER(l,comp)    (TYPE_PKG << 6 | ((comp&0x1)<<5) | (((l)&0x1f000000)>>24) | (((l)&0x00ffffff)<<8))
#define GET_TYPE_PKG_LENGTH(h,l)        ((((h)&0x1f)<<24) | (((l)&0xffffff00)>>8))

enum Protocol_Type {
	TCP 		= 1,
	UDP 		= 2,
	HTTP		= 3,
	WEBSOCKET	= 4,
};

enum Package_Type
{
	RPC_PKG		= 1,	//rpc包，协议定义写到配置文件，包长最多2^13-1个字节
	TYPE_PKG	= 2,	//type包，协议定义写到包里，包长最多2^29-1个字节
};

enum Endpoint_Type {
	CLIENT_SERVER	= 1,	//接受客户端链接的server
	SERVER 			= 2,	//接受内部节点连接的server
	CONNECTOR		= 3,	//内部节点链接器
};

enum Msg_Type {
	TCP_C2S 	= 1,	//客户端发到服务器的消息
	TCP_S2C		= 2,	//服务器发到客户端的消息
	NODE_C2S	= 3,	//客户端经过gate中转发到服务器的消息
	NODE_S2C	= 4,	//服务器经过gate中转发到客户端的消息
	NODE_MSG	= 5,	//服务器进程节点间通信的消息
	DATA_MSG	= 6,	//经过data中转发到data子进程的消息
	HTTP_MSG 	= 7,	//http消息
	WS_C2S 		= 8,	//websocket客户端消息
	WS_S2C 		= 9,	//websocket服务器消息
};

enum Event_Type {
	EVENT_INPUT 		= 0x1,
	EVENT_OUTPUT 		= 0x2,
	EVENT_TIMEOUT 		= 0x4,
	EVENT_ONCE_IO_IN 	= 0x8,	//一次性IO输入事件
	EVENT_ONCE_IO_OUT 	= 0x10,	//一次性IO输出事件
	EVENT_ONCE_TIMER 	= 0x20,	//一次性定时器事件
	WITH_IO_HEARTBEAT 	= 0x40,	//IO附带心跳机制
};

enum Log_Type {
	LOG_DEBUG		= 1,	//细粒度信息事件对调试应用程序是非常有帮助的
	LOG_INFO		= 2,	//消息在粗粒度级别上突出强调应用程序的运行过程
	LOG_WARN		= 3,	//会出现潜在错误的情形
	LOG_ERROR		= 4,	//虽然发生错误事件，但仍然不影响系统的继续运行
	LOG_TRACE		= 5,	//打印程序运行堆栈，跟踪记录数据信息
	LOG_FATAL		= 6,	//严重的错误事件，将会导致应用程序的退出
};

enum Color {
	BLACK = 30,
	RED = 31,
	GREEN = 32,
	BROWN = 33,
	BLUE = 34,
	MAGENTA = 35,
	CYAN = 36,
	GREY = 37,
	LRED = 41,
	LGREEN = 42,
	YELLOW = 43,
	LBLUE = 44,
	LMAGENTA = 45,
	LCYAN = 46,
	WHITE = 47
};

enum Websocket_Frame {
	FRAME_NORMAL = 0x0,
	FRAME_FINAL	 = 0x1,
};

enum Websocket_Opcode {
	OPCODE_CONTINUATION	= 0x0,
	OPCODE_TEXT 	= 0x1,
	OPCODE_BINARY 	= 0x2,
	OPCODE_CLOSE 	= 0x8,
	OPCODE_PING 	= 0x9,
	OPCODE_PONG 	= 0xa,
};

enum Struct_Type {
	BASE_STRUCT 	= 1,	//基本结构体
	MSG_STRUCT 		= 2,	//消息结构体
	ROBOT_STRUCT 	= 3,	//robot结构体
	DB_STRUCT 		= 4,	//数据库结构体
};

enum DB_Type {
	MYSQL = 1,
	MONGO = 2,
};

enum Save_Type {
	SAVE_CACHE 			= 1,	//保存缓存
	SAVE_DB_AND_CACHE 	= 2,	//保存数据库和缓存
	SAVE_DB_CLEAR_CACHE = 3,	//保存数据库清空缓存
};

#endif /* ENUM_H_ */
