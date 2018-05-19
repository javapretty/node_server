/*
 * Endpoint.h
 *
 *  Created on: Sep 22, 2016
 *      Author: zhangyalei
 */

#ifndef ENDPOINT_H_
#define ENDPOINT_H_

#include "Buffer_Pool_Group.h"
#include "Object_Pool.h"
#include "Accept.h"
#include "Connect.h"
#include "Network.h"
#include "Svc.h"
#include "Svc_Static_List.h"

class Endpoint_Accept: public Accept {
public:
	virtual int accept_svc(int connfd);
};

class Endpoint_Connect: public Connect {
public:
	virtual int connect_svc(int connfd);
};

class Endpoint_Network: public Network {
public:
	virtual int drop_handler(int cid);
	virtual Svc *find_svc(int cid);
};

class Endpoint_Svc: public Svc {
public:
	virtual Byte_Buffer *pop_buffer(int cid);
	virtual int push_buffer(int cid, Byte_Buffer *buffer);
	virtual int post_buffer(Byte_Buffer* buffer);

	virtual int register_network_handler(void);
	virtual int unregister_network_handler(void);

	virtual int close_handler(int cid);
};

class Endpoint {
	typedef Object_Pool<Endpoint_Svc, Spin_Lock> Svc_Pool;
	typedef Svc_Static_List<Endpoint_Svc *, Spin_Lock> Svc_List;
public:
	Endpoint(void);
	virtual ~Endpoint(void);

	virtual int init(Endpoint_Info &endpoint_info) { endpoint_info_ = endpoint_info; return 0; }
	virtual int start(void) { return 0; }

	inline Endpoint_Accept &accept(void) { return accept_; }
	inline Endpoint_Connect &connect(void) { return connect_; }
	inline Endpoint_Network &network(void) { return network_; }
	inline Endpoint_Info &endpoint_info(void) { return endpoint_info_; }

	//创建数据buffer/回收数据buffer
	Byte_Buffer *pop_buffer(int cid)  { return buffer_pool_group_.pop_buffer(cid); }
	int push_buffer(int cid, Byte_Buffer *buffer)  { return buffer_pool_group_.push_buffer(cid, buffer); }

	//发送数据buffer
	int send_buffer(int cid, Byte_Buffer &buffer);

	//投递数据buffer
	virtual void post_buffer(Byte_Buffer* buffer) { }
	//投递掉线cid
	virtual void post_drop_cid(int cid) { }

	//创建svc/回收svc/查找svc
	Svc *pop_svc(int connfd);
	int push_svc(int cid);
	Svc *find_svc(int cid);

	int get_svc_info(Svc_Info &svc_info);
	void free_pool(void);

private:
	Svc_List svc_list_;
	Svc_Pool svc_pool_;
	Buffer_Pool_Group buffer_pool_group_;

	Endpoint_Accept accept_;
	Endpoint_Connect connect_;
	Endpoint_Network network_;
	Endpoint_Info endpoint_info_;
};

#endif /* ENDPOINT_H_ */
