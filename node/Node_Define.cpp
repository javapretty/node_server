/*
 * Node_Define.cpp
 *
 *  Created on: Sep 20, 2016
 *      Author: zhangyalei
 */

#include "Block_Buffer.h"
#include "Node_Config.h"
#include "Node_Define.h"

void Node_Info::serialize(Block_Buffer &buffer) {
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

void Node_Info::deserialize(Block_Buffer &buffer) {
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

void Node_Conf::init(void) {
	const Json::Value &node_conf = NODE_CONFIG->node_conf()["node"];
	for(uint i = 0; i < node_conf.size(); ++i) {
		Node_Info node_info;
		node_info.node_type = node_conf[i]["node_type"].asInt();
		node_info.node_id = node_conf[i]["node_id"].asInt();
		node_info.node_name = node_conf[i]["node_name"].asString();
		node_info.node_ip = node_conf[i]["node_ip"].asString();
		node_info.script_path = node_conf[i]["script_path"].asString();

		const Json::Value &plugin_conf = node_conf[i]["plugin"];
		for (uint j = 0; j < plugin_conf.size();++j) {
			std::string plugin_path = plugin_conf[j]["path"].asString();
			node_info.plugin_list.push_back(plugin_path);
		}

		const Json::Value &endpoint_conf = node_conf[i]["endpoint"];
		for (uint j = 0; j < endpoint_conf.size();++j) {
			Endpoint_Info endpoint_info;
			endpoint_info.endpoint_type = endpoint_conf[j]["endpoint_type"].asInt();
			endpoint_info.endpoint_id = endpoint_conf[j]["endpoint_id"].asInt();
			endpoint_info.endpoint_name = endpoint_conf[j]["endpoint_name"].asString();
			endpoint_info.server_ip = endpoint_conf[j]["server_ip"].asString();
			endpoint_info.server_port = endpoint_conf[j]["server_port"].asInt();
			endpoint_info.protocol_type = endpoint_conf[j]["protocol_type"].asInt();
			endpoint_info.receive_timeout = endpoint_conf[j]["receive_timeout"].asInt();
			node_info.endpoint_list.push_back(endpoint_info);
		}

		node_map.insert(std::make_pair(node_info.node_id, node_info));
	}
}
