/*
 * Accept.h
 *
 *  Created on: Dec 16,2015
 *      Author: zhangyalei
 */

#ifndef ACCEPT_H_
#define ACCEPT_H_

#include "Thread.h"

class Endpoint;
class Accept: public Thread {
public:
	Accept(void);
	virtual ~Accept(void);

	int init(Endpoint *endpoint, int port);
	int fini(void);

	void server_listen(void);
	void server_accept(void);
	virtual int accept_svc(int connfd);

	virtual void run_handler(void);
	virtual void exit_handler(void);

protected:
	Endpoint *endpoint_;

private:
	int port_;
	int listenfd_;
};

#endif /* ACCEPT_H_ */
