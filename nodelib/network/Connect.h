/*
 * Connect.h
 *
 *  Created on: Dec 16,2015
 *      Author: zhangyalei
 */

#ifndef CONNECT_H_
#define CONNECT_H_

class Endpoint;
class Connect {
public:
	Connect(void);
	virtual ~Connect(void);

	int init(Endpoint *endpoint);
	int connect(const char *ip, int port);
	virtual int connect_svc(int connfd);

protected:
	Endpoint *endpoint_;
};

#endif /* CONNECT_H_ */
