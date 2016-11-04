/*
 * Node_Define.cpp
 *
 *  Created on: Sep 20, 2016
 *      Author: zhangyalei
 */

#include "Bit_Buffer.h"
#include "Node_Define.h"

void Msg_Filter::serialize(Bit_Buffer &buffer) {
	buffer.write_int(msg_type, 32);
	buffer.write_int(min_msg_id, 32);
	buffer.write_int(max_msg_id, 32);
}

void Msg_Filter::deserialize(Bit_Buffer &buffer) {
	msg_type = buffer.read_int(32);
	min_msg_id = buffer.read_int(32);
	max_msg_id = buffer.read_int(32);
}

void Msg_Filter::reset(void) {
	msg_type = 0;
	min_msg_id = 0;
	max_msg_id = 0;
}

void Node_Info::serialize(Bit_Buffer &buffer) {
	buffer.write_int(node_type, 32);
	buffer.write_int(node_id, 32);
	buffer.write_str(node_name.c_str());
	buffer.write_str(node_ip.c_str());
	buffer.write_str(script_path.c_str());

	uint plugin_size = plugin_list.size();
	buffer.write_uint(plugin_size, 8);
	for (uint i = 0; i < plugin_size; ++i) {
		buffer.write_str(plugin_list[i].c_str());
	}

	uint filter_size = filter_list.size();
	buffer.write_uint(filter_size, 8);
	for (uint i = 0; i < filter_size; ++i) {
		filter_list[i].serialize(buffer);
	}

	uint endpoint_size = endpoint_list.size();
	buffer.write_uint(endpoint_size, 8);
	for (uint i = 0; i < endpoint_size; ++i) {
		endpoint_list[i].serialize(buffer);
	}
}

void Node_Info::deserialize(Bit_Buffer &buffer) {
	node_type = buffer.read_int(32);
	node_id = buffer.read_int(32);
	buffer.read_str(node_name);
	buffer.read_str(node_ip);
	buffer.read_str(script_path);

	uint plugin_size = buffer.read_uint(8);
	for (uint i = 0; i < plugin_size; ++i) {
		std::string plugin_name = "";
		buffer.read_str(plugin_name);
		plugin_list.push_back(plugin_name);
	}

	uint filter_size = buffer.read_uint(8);
	Msg_Filter msg_filter;
	for (uint i = 0; i < filter_size; ++i) {
		msg_filter.reset();
		msg_filter.deserialize(buffer);
		filter_list.push_back(msg_filter);
	}

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
	script_path = "";
	plugin_list.clear();
	endpoint_list.clear();
}
