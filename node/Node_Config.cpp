/*
 * Node_Config.cpp
 *
 *  Created on: Sep 20, 2016
 *      Author: zhangyalei
 */

#include "Node_Config.h"

Node_Config::Node_Config(void) { }

Node_Config::~Node_Config(void) { }

Node_Config * Node_Config::instance_;

Node_Config *Node_Config::instance() {
	if (! instance_)
		instance_ = new Node_Config;
	return instance_;
}

void Node_Config::load_node_config(void) {
	GUARD_WRITE(Config_Lock, mon, node_config_.lock);
	{
		std::string path("./config/node/node_conf.json");
		load_json_file(path.c_str(), node_config_.node_conf.get_cur_json());
	}
	{
		std::string path("./config/node/node_misc.json");
		load_json_file(path.c_str(), node_config_.node_misc.get_cur_json());
	}
}

const Json::Value &Node_Config::node_conf(void) {
	return get_json_value_with_rlock(node_config_.lock, node_config_.node_conf);
}

const Json::Value &Node_Config::node_misc(void) {
	return get_json_value_with_rlock(node_config_.lock, node_config_.node_misc);
}
