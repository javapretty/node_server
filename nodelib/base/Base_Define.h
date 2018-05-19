/*
 * Base_Define.h
 *
 *  Created on: Sep 22, 2016
 *      Author: zhangyalei
 */

#ifndef BASE_DEFINE_H_
#define BASE_DEFINE_H_

#include <vector>
#include "Time_Value.h"

struct Data_Info {
	int file_size;
	int data_len;
	char* data;
} ;

struct Msg_Head {
	int32_t eid;
	int32_t cid;
	uint8_t protocol;
	uint8_t pkg_type; 
	uint8_t client_msg;
	uint8_t msg_id;
	uint8_t msg_type;
	uint32_t sid;

	Msg_Head() { reset(); }
	void reset() {
		eid = 0;
		cid = 0;
		protocol = 0;
		pkg_type = 0;
		client_msg = 0;
		msg_id = 0;
		msg_type = 0;
		sid = 0;
	}
};

struct Field_Info {
	std::string field_label;	//字段标签
	std::string field_type;		//字段类型
	int field_bit;				//字段位数
	std::string field_name;		//字段名称
	int field_vbit;				//数组字段长度位数
	uint case_val;				//case标签对应的值
	std::string key_type;		//主键类型
	int key_bit;				//主键位数
	std::string key_name;		//主键名称
	std::vector<Field_Info> children;//if/switch使用存放子字段

	Field_Info() { reset(); }
	void reset() {
		field_label = "";
		field_vbit = 0;
		case_val = 0;
		field_type = "";
		field_bit = 0;
		field_name = "";
		key_type = "";
		key_bit = 0;
		key_name = "";
		children.clear();
	}
};
typedef std::vector<Field_Info> Field_Vec;

class Bit_Buffer;
class Byte_Buffer;
struct Endpoint_Info {
	int endpoint_type;			//端点类型
	int endpoint_gid;			//端点组id
	int endpoint_id;			//端点id
	std::string endpoint_name;	//端点名称
	std::string server_ip;		//服务器ip
	int server_port;			//服务器端口(如果是connect端点，该ip和port表示它要连接的服务器)
	int protocol_type;			//网络协议类型 0:Tcp 1:Udp 2:Websocket 3:Http
	int heartbeat_timeout;		//心跳超时时间 单位:s

	void serialize(Bit_Buffer &buffer);
	void deserialize(Bit_Buffer &buffer);
	void reset(void);
};

struct Buffer_Group_Info {
	int free_size;	//buffer对象池未使用节点数量
	int used_size;	//buffer对象池已使用节点数量
	int sum_bytes;	//对象池总共大小

	void serialize(Bit_Buffer &buffer);
	void deserialize(Bit_Buffer &buffer);
	void reset(void);
};

struct Svc_Info {
	int endpoint_gid;		//端点组id
	int endpoint_id;		//端点id
	int svc_pool_free_size;	//svc对象池未使用节点数量
	int svc_pool_used_size;	//svc对象池已使用节点数量
	int svc_list_size;		//svc列表数量
	std::vector<Buffer_Group_Info> buffer_group_list;

	void serialize(Bit_Buffer &buffer);
	void deserialize(Bit_Buffer &buffer);
	void reset(void);
};

#endif /* BASE_DEFINE_H_ */
