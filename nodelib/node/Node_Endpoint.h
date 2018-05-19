/*
 * Node_Endpoint.h
 *
 *  Created on: Nov 7, 2016
 *      Author: zhangyalei
 */

#ifndef NODE_ENDPOINT_H_
#define NODE_ENDPOINT_H_

#include "Endpoint.h"
#include "V8_Manager.h"

class Server: public Endpoint {
public:
	Server(void);
	virtual ~Server(void);

	virtual int init(Endpoint_Info &endpoint_info);
	virtual int start(void);

	virtual void post_buffer(Byte_Buffer* buffer) { V8_MANAGER->push_buffer(buffer); }
	virtual void post_drop_cid(int cid) {
		if(endpoint_info().endpoint_type == CLIENT_SERVER) {
			V8_MANAGER->push_drop_cid(cid);
		}
	}
};

class Connector: public Endpoint {
public:
	Connector(void);
	virtual ~Connector(void);

	virtual int init(Endpoint_Info &endpoint_info);
	virtual int start(void);
	int connect_server(std::string ip = "", int port = 0);
	int get_cid(void) { return cid_; }

	virtual void post_buffer(Byte_Buffer* buffer) { V8_MANAGER->push_buffer(buffer); }
	virtual void post_drop_cid(int cid) {
		V8_MANAGER->push_drop_eid(endpoint_info().endpoint_id);
	}

private:
	int cid_;
};

#endif /* NODE_ENDPOINT_H_ */
