/*
 * Base_Define.cpp
 *
 *  Created on: Sep 22, 2016
 *      Author: zhangyalei
 */

#include "Bit_Buffer.h"
#include "Byte_Buffer.h"
#include "Base_Define.h"

void Endpoint_Info::serialize(Bit_Buffer &buffer) {
	buffer.write_int(endpoint_type, 32);
	buffer.write_int(endpoint_gid, 32);
	buffer.write_int(endpoint_id, 32);
	buffer.write_str(endpoint_name.c_str());
	buffer.write_str(server_ip.c_str());
	buffer.write_int(server_port, 32);
	buffer.write_int(protocol_type, 32);
	buffer.write_int(heartbeat_timeout, 32);
}

void Endpoint_Info::deserialize(Bit_Buffer &buffer) {
	endpoint_type = buffer.read_int(32);
	endpoint_gid = buffer.read_int(32);
	endpoint_id = buffer.read_int(32);
	buffer.read_str(endpoint_name);
	buffer.read_str(server_ip);
	server_port = buffer.read_int(32);
	protocol_type = buffer.read_int(32);
	heartbeat_timeout = buffer.read_int(32);
}

void Endpoint_Info::reset(void) {
	endpoint_type = 0;
	endpoint_gid = 0;
	endpoint_id = 0;
	endpoint_name = "";
	server_ip = "";
	server_port = 0;
	protocol_type = 0;
	heartbeat_timeout = 0;
}

void Buffer_Group_Info::serialize(Bit_Buffer &buffer) {
	buffer.write_int(free_size, 32);
	buffer.write_int(used_size, 32);
	buffer.write_int(sum_bytes, 32);
}

void Buffer_Group_Info::deserialize(Bit_Buffer &buffer) {
	free_size = buffer.read_int(32);
	used_size = buffer.read_int(32);
	sum_bytes = buffer.read_int(32);
}

void Buffer_Group_Info::reset(void) {
	free_size = 0;
	used_size = 0;
	sum_bytes = 0;
}

void Svc_Info::serialize(Bit_Buffer &buffer) {
	buffer.write_int(endpoint_gid, 32);
	buffer.write_int(endpoint_id, 32);
	buffer.write_int(svc_pool_free_size, 32);
	buffer.write_int(svc_pool_used_size, 32);
	buffer.write_int(svc_list_size, 32);

	uint16_t size = buffer_group_list.size();
	buffer.write_uint(size, 16);
	for (uint16_t i = 0; i < size; ++i) {
		buffer_group_list[i].serialize(buffer);
	}
}

void Svc_Info::deserialize(Bit_Buffer &buffer) {
	endpoint_gid = buffer.read_int(32);
	endpoint_id = buffer.read_int(32);
	svc_pool_free_size = buffer.read_int(32);
	svc_pool_used_size = buffer.read_int(32);
	svc_list_size = buffer.read_int(32);

	uint16_t size = buffer.read_uint(16);
	Buffer_Group_Info group_info;
	for (uint16_t i = 0; i < size; ++i) {
		group_info.reset();
		group_info.deserialize(buffer);
		buffer_group_list.push_back(group_info);
	}
}

void Svc_Info::reset(void) {
	endpoint_gid = 0;
	endpoint_id = 0;
	svc_pool_free_size = 0;
	svc_pool_used_size = 0;
	svc_list_size = 0;
	buffer_group_list.clear();
}
