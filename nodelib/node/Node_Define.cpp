/*
 * Node_Define.cpp
 *
 *  Created on: Sep 20, 2016
 *      Author: zhangyalei
 */

#include "Bit_Buffer.h"
#include "Node_Define.h"

void Node_Info::serialize(Bit_Buffer &buffer) {
	buffer.write_int(node_type, 32);
	buffer.write_int(node_id, 32);
	buffer.write_int(endpoint_gid, 32);
	buffer.write_int(max_session_count, 32);
	buffer.write_str(node_name.c_str());
	buffer.write_str(node_ip.c_str());

	uint endpoint_size = endpoint_list.size();
	buffer.write_uint(endpoint_size, 8);
	for (uint i = 0; i < endpoint_size; ++i) {
		endpoint_list[i].serialize(buffer);
	}
}

void Node_Info::deserialize(Bit_Buffer &buffer) {
	node_type = buffer.read_int(32);
	node_id = buffer.read_int(32);
	endpoint_gid = buffer.read_int(32);
	max_session_count = buffer.read_int(32);
	buffer.read_str(node_name);
	buffer.read_str(node_ip);

	uint endpoint_size = buffer.read_uint(8);
	Endpoint_Info endpoint_info;
	for (uint i = 0; i < endpoint_size; ++i) {
		endpoint_info.reset();
		endpoint_info.deserialize(buffer);
		endpoint_list.push_back(endpoint_info);
	}
}

void Node_Info::reset(void) {
	node_type = 0;
	node_id = 0;
	node_name = "";
	node_ip = "";
	global_script = "";
	main_script = "";
	hotupdate_list.clear();
	plugin_list.clear();
	filter_list.clear();
	endpoint_list.clear();
}
