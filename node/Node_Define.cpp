/*
 * Node_Define.cpp
 *
 *  Created on: Sep 20, 2016
 *      Author: zhangyalei
 */

#include "Byte_Buffer.h"
#include "Node_Define.h"

void Node_Info::serialize(Byte_Buffer &buffer) {
	buffer.write_int32(node_type);
	buffer.write_int32(node_id);
	buffer.write_string(node_name);
	buffer.write_string(node_ip);
	buffer.write_string(script_path);

	uint16_t plugin_size = plugin_list.size();
	buffer.write_uint16(plugin_size);
	for (uint16_t i = 0; i < plugin_size; ++i) {
		buffer.write_string(plugin_list[i]);
	}

	uint16_t endpoint_size = endpoint_list.size();
	buffer.write_uint16(endpoint_size);
	for (uint16_t i = 0; i < endpoint_size; ++i) {
		endpoint_list[i].serialize(buffer);
	}
}

void Node_Info::deserialize(Byte_Buffer &buffer) {
	buffer.read_int32(node_type);
	buffer.read_int32(node_id);
	buffer.read_string(node_name);
	buffer.read_string(node_ip);
	buffer.read_string(script_path);

	uint16_t plugin_size = 0;
	buffer.read_uint16(plugin_size);
	for (uint16_t i = 0; i < plugin_size; ++i) {
		std::string plugin_name = "";
		buffer.read_string(plugin_name);
		plugin_list.push_back(plugin_name);
	}

	uint16_t endpoint_size = 0;
	buffer.read_uint16(endpoint_size);
	Endpoint_Info endpoint_info;
	for (uint16_t i = 0; i < endpoint_size; ++i) {
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
