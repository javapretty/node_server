/*
 * Node_Define.cpp
 *
 *  Created on: Sep 20, 2016
 *      Author: zhangyalei
 */

#include "Node_Config.h"
#include "Node_Define.h"

void Node_Conf::init(void) {
	const Json::Value &node_conf = NODE_CONFIG->node_conf()["node"];
	for(uint i = 0; i < node_conf.size(); ++i) {
		Node_Info node_info;
		node_info.node_type = node_conf[i]["node_type"].asInt();
		node_info.node_id = node_conf[i]["node_id"].asInt();
		node_info.node_name = node_conf[i]["node_name"].asString();
		node_info.script_path = node_conf[i]["script_path"].asString();

		const Json::Value &server_conf = node_conf[i]["server"];
		for (uint j = 0; j < server_conf.size();++j) {
			Endpoint_Info endpoint_info;
			endpoint_info.endpoint_type = server_conf[j]["endpoint_type"].asInt();
			endpoint_info.endpoint_id = server_conf[j]["endpoint_id"].asInt();
			endpoint_info.endpoint_name = server_conf[j]["endpoint_name"].asString();
			endpoint_info.server_ip = server_conf[j]["server_ip"].asString();
			endpoint_info.server_port = server_conf[j]["server_port"].asInt();
			endpoint_info.protocol_type = server_conf[j]["protocol_type"].asInt();
			endpoint_info.receive_timeout = server_conf[j]["receive_timeout"].asInt();
			node_info.server_list.push_back(endpoint_info);
		}

		const Json::Value &connector_conf = node_conf[i]["connector"];
		for (uint j = 0; j < connector_conf.size();++j) {
			Endpoint_Info endpoint_info;
			endpoint_info.endpoint_type = connector_conf[j]["endpoint_type"].asInt();
			endpoint_info.endpoint_id = connector_conf[j]["endpoint_id"].asInt();
			endpoint_info.endpoint_name = connector_conf[j]["endpoint_name"].asString();
			endpoint_info.server_ip = connector_conf[j]["server_ip"].asString();
			endpoint_info.server_port = connector_conf[j]["server_port"].asInt();
			endpoint_info.protocol_type = connector_conf[j]["protocol_type"].asInt();
			endpoint_info.receive_timeout = connector_conf[j]["receive_timeout"].asInt();
			node_info.connector_list.push_back(endpoint_info);
		}

		node_list.push_back(node_info);
	}
}
