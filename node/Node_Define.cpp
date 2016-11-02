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
	buffer.write_str(node_name.c_str());
	buffer.write_str(node_ip.c_str());
	buffer.write_str(script_path.c_str());

	uint plugin_size = plugin_list.size();
	buffer.write_uint(plugin_size, 8);
	for (uint i = 0; i < plugin_size; ++i) {
		buffer.write_str(plugin_list[i].c_str());
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
	buffer.read_str((char*)node_name.c_str(), 1024);
	buffer.read_str((char*)node_ip.c_str(), 1024);
	buffer.read_str((char*)script_path.c_str(), 1024);

	uint plugin_size = buffer.read_uint(8);
	for (uint i = 0; i < plugin_size; ++i) {
		std::string plugin_name = "";
		buffer.read_str((char*)plugin_name.c_str(), 1024);
		plugin_list.push_back(plugin_name);
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
