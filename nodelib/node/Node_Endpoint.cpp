/*
 * Node_Endpoint.cpp
 *
 *  Created on: Nov 7, 2016
 *      Author: zhangyalei
 */

#include "Node_Endpoint.h"

Server::Server(void) { }

Server::~Server(void) { }

int Server::init(Endpoint_Info &endpoint_info) {
	Endpoint::init(endpoint_info);

	accept().init(this, endpoint_info.server_port);
	network().init(this, endpoint_info.heartbeat_timeout);
	return 0;
}

int Server::start(void) {
	accept().thr_create();
	network().thr_create();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
Connector::Connector(void) : cid_(-1) { }

Connector::~Connector(void) { }

int Connector::init(Endpoint_Info &endpoint_info) {
	Endpoint::init(endpoint_info);

	connect().init(this);
	network().init(this, endpoint_info.heartbeat_timeout);
	return 0;
}

int Connector::start(void) {
	network().thr_create();
	return 0;
}

int Connector::connect_server(std::string ip, int port) {
	if (ip == "" && port == 0) {
		cid_ = connect().connect(endpoint_info().server_ip.c_str(), endpoint_info().server_port);
	} else {
		cid_ = connect().connect(ip.c_str(), port);
	}
	return cid_;
}