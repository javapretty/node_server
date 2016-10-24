/*
 * Node_Config.h
 *
 *  Created on: Sep 20, 2016
 *      Author: zhangyalei
 */

#ifndef NODE_CONFIG_H_
#define NODE_CONFIG_H_

#include "Config.h"

class Node_Config : public Config {
public:
	struct Node_Config_Entry {
		Config_Entry node_conf;
		Config_Entry node_misc;

		Config_Lock lock;
	};

	static Node_Config *instance();

	void load_node_config(void);
	const Json::Value &node_conf(void);
	const Json::Value &node_misc(void);

private:
	Node_Config(void);
	virtual ~Node_Config(void);
	Node_Config(const Node_Config &);
	const Node_Config &operator=(const Node_Config &);

private:
	static Node_Config *instance_;
	Node_Config_Entry node_config_;
};

#define NODE_CONFIG Node_Config::instance()

#endif /* NODE_CONFIG_H_ */
