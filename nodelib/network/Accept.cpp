/*
 * Acceptor.cpp
 *
 *  Created on: Dec 16,2015
 *      Author: zhangyalei
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "Base_Function.h"
#include "Endpoint.h"
#include "Accept.h"

Accept::Accept(void):
	endpoint_(0),
	port_(0),
  listenfd_(0)
{ }

Accept::~Accept(void) { }

int Accept::init(Endpoint *endpoint, int port) {
	endpoint_ = endpoint;
	port_ = port;
	return 0;
}

int Accept::fini(void) {
	LOG_INFO("close listenfd = %d", listenfd_);
	::close(listenfd_);
	return 0;
}

void Accept::server_listen(void) {
	struct sockaddr_in serveraddr;

	if ((listenfd_ = ::socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		LOG_FATAL("server_listen, socket fatal, listenfd = %d", listenfd_);
	}

	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(port_);

	int flag = 1;
	if (::setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1) {
		LOG_FATAL("server_listen, setsockopt SO_REUSEADDR fatal, listenfd = %d", listenfd_);
	}

	int peer_buf_len = 2 * 1024 * 1024;
	if (::setsockopt(listenfd_, SOL_SOCKET, SO_SNDBUF, &peer_buf_len, sizeof(peer_buf_len)) == -1) {
		LOG_FATAL("server_listen, setsockopt SO_SNDBUF fatal, listenfd = %d", listenfd_);
	}
	if (::setsockopt(listenfd_, SOL_SOCKET, SO_RCVBUF, &peer_buf_len, sizeof(peer_buf_len)) == -1) {
		LOG_FATAL("server_listen, setsockopt SO_RCVBUF fatal, listenfd = %d", listenfd_);
	}

	struct timeval timeout = {1, 0};//0.1s
	if (::setsockopt(listenfd_, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == -1) {
		LOG_FATAL("server_listen, setsockopt SO_SNDTIMEO fatal, listenfd = %d", listenfd_);
	}

	if (::bind(listenfd_, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) == -1) {
		LOG_FATAL("server_listen, bind fatal, listenfd = %d", listenfd_);
	}

	if (::listen(listenfd_, 1024) == -1) {
		LOG_FATAL("server_listen, listen fatal, listenfd = %d", listenfd_);
	}
}

void Accept::server_accept(void) {
	int connfd = 0;
	struct sockaddr_in client_addr;
	socklen_t addr_len = 0;
	char client_ip[128];

	while (1) {
		memset(&client_addr, 0, sizeof(client_addr));
		addr_len = sizeof(client_addr);
		if ((connfd = ::accept4(listenfd_, (struct sockaddr *) &client_addr, &addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC)) == -1) {
			if(errno == EMFILE || errno == ENFILE) {
				//如果文件描述符超过最大限制，将线程短暂休眠，防止出现死循环
				Time_Value::sleep(Time_Value(0, 100));
			}
			continue;
		}
		struct linger so_linger;
		so_linger.l_onoff = 1;
		so_linger.l_linger = 0;
		if (::setsockopt(connfd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger)) < 0) {
			LOG_ERROR("server_accept setsockopt SO_LINGER fail, connfd = %d", connfd);
			continue;
		}

/** turn on/off TCP Nagel algorithm
		int nodelay_on=1;
		if (::setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY, &nodelay_on, sizeof(nodelay_on)) < 0) {
			LOG_ERROR("setsockopt nodelay");
		}
*/
		memset(client_ip, 0, sizeof(client_ip));
		if (::inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip)) == NULL) {
			LOG_ERROR("server_accept inet_ntop fail, client_ip:%s", client_ip);
			continue;
		}

		accept_svc(connfd);
	}
}

int Accept::accept_svc(int connfd) {
	LOG_TRACE("SHOULD NOT HERE");
	return 0;
}

void Accept::run_handler(void) {
	server_listen();
	server_accept();
}

void Accept::exit_handler(void) {
	fini();
}
