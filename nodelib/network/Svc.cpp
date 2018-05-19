/*
 * Svc.cpp
 *
 *  Created on: Dec 16,2015
 *      Author: zhangyalei
 */

#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include "Base_Enum.h"
#include "Endpoint.h"
#include "Svc_Http.h"
#include "Svc_Tcp.h"
#include "Svc_WebSocket.h"

Svc_Handler::Svc_Handler():
	parent_(NULL),
	max_list_size_(SVC_MAX_LIST_SIZE),
	max_pack_size_(SVC_MAX_PACK_SIZE)
{ }

Svc_Handler::~Svc_Handler() { }

void Svc_Handler::reset(void) {
	Data_List::BList blist;
	int cid = parent_->get_cid();

	blist.clear();
	recv_buffer_list_.swap(blist);
	for (Data_List::BList::iterator it = blist.begin(); it != blist.end(); ++it) {
		parent_->push_buffer(cid, *it);
	}

	blist.clear();
	send_buffer_list_.swap(blist);
	for (Data_List::BList::iterator it = blist.begin(); it != blist.end(); ++it) {
		parent_->push_buffer(cid, *it);
	}

	parent_ = 0;
	max_list_size_ = SVC_MAX_LIST_SIZE;
	max_pack_size_ = SVC_MAX_PACK_SIZE;
}

int Svc_Handler::push_recv_buffer(Byte_Buffer *buffer) {
	if (parent_->client() && recv_buffer_list_.size() >= max_list_size_) {
		LOG_ERROR("recv_buffer_list_ full cid = %d, size = %d", parent_->get_cid(), max_list_size_);
		parent_->handle_close();
		return -1;
	}
	recv_buffer_list_.push_back(buffer);
	return 0;
}

int Svc_Handler::push_send_buffer(Byte_Buffer *buffer) {
	if (parent_->client() && send_buffer_list_.size() >= max_list_size_) {
		LOG_ERROR("send_buffer_list_ full cid = %d, size = %d", parent_->get_cid(), max_list_size_);
		parent_->handle_close();
		return -1;
	}
	send_buffer_list_.push_back(buffer);
	parent_->set_send_data(true);
	return 0;
}

Svc::Svc(void):
	endpoint_(0),
	cid_(0),
	closed_(false),
	alive_(false),
	client_(false),
	reg_network_(false),
	send_data_(false),
	peer_port_(0),
	handler_(0)
{ }

Svc::~Svc(void) { }

void Svc::set_endpoint(Endpoint *endpoint) {
	endpoint_ = endpoint;
}

Byte_Buffer *Svc::pop_buffer(int cid) {
	LOG_TRACE("SHOULD NOT HERE");
	return 0;
}

int Svc::push_buffer(int cid, Byte_Buffer *buffer) {
	LOG_TRACE("SHOULD NOT HERE");
	return 0;
}

int Svc::post_buffer(Byte_Buffer* buffer) {
	LOG_TRACE("SHOULD NOT HERE");
	return 0;
}

int Svc::register_network_handler(void) {
	LOG_TRACE("SHOULD NOT HERE");
	return 0;
}

int Svc::unregister_network_handler(void) {
	LOG_TRACE("SHOULD NOT HERE");
	return 0;
}

int Svc::close_handler(int cid) {
	LOG_TRACE("SHOULD NOT HERE");
	return 0;
}

int Svc::create_handler(int protocol_type) {
	switch(protocol_type){
		case TCP:
			handler_ = new Svc_Tcp;
			break;
		case UDP:
			break;
		case HTTP:
			handler_ = new Svc_Http;
			break;
		case WEBSOCKET:
			handler_ = new Svc_Websocket;
			break;
		default:
			break;
	}
	handler_->set_parent(this);
	return 0;
}

int Svc::handle_input(void) {
	if (closed_)
		return 0;

	//svc收到数据包时候，设置为活跃的
	set_alive(true);

	//从对象池新建数据buffer
	Byte_Buffer *buffer = pop_buffer(cid_);
	if (!buffer) {
		LOG_ERROR("pop_buffer fail, cid:%d", cid_);
		return -1;
	}
	buffer->reset();
	buffer->write_int32(endpoint_->endpoint_info().endpoint_id);
	buffer->write_int32(cid_);		//写入客户端连接的cid

	while (1) {
		//每次保证buffer有2000字节的可写空间
		buffer->ensure_writable_bytes(2000);
		int n = 0;
		if ((n = ::read(get_fd(), buffer->get_write_ptr(), buffer->writable_bytes())) < 0) {
			if (errno == EINTR) {
				//被打断,重新继续读
				continue;
			} else if (errno == EWOULDBLOCK){
				//EAGAIN,表示现在没有数据可读,下一次超时再读
				break;
			} else {
				//遇到其他错误，断开连接
				LOG_ERROR("read data return:%d cid:%d peer_ip:%s peer_port:%d endpoint:%s", n, cid_, peer_ip_.c_str(), peer_port_, endpoint_->endpoint_info().endpoint_name.c_str());
				push_buffer(cid_, buffer);
				handle_close();
				return -1;
			}
		} else if (n == 0) { //读数据遇到eof结束符，关闭连接
			LOG_ERROR("read data return:%d cid:%d peer_ip:%s peer_port:%d endpoint:%s", n, cid_, peer_ip_.c_str(), peer_port_, endpoint_->endpoint_info().endpoint_name.c_str());
			push_buffer(cid_, buffer);
			handle_close();
			return -1;
		} else {		//读取数据成功，改写buffer写指针，然后继续读
			buffer->set_write_idx(buffer->get_write_idx() + n);
		}
	}

	if (push_recv_buffer(buffer) == 0) {
		//读取推送数据成功，进行解包处理
		Buffer_Vector buffer_vec;
		handler_->handle_pack(buffer_vec);
		for (Buffer_Vector::iterator iter = buffer_vec.begin(); iter != buffer_vec.end(); ++iter) {
			post_buffer(*iter);
		}
	} else {
		push_buffer(cid_, buffer);
	}
	return 0;
}

int Svc::handle_send(void) {
	return handler_->handle_send();
}

int Svc::handle_close(void) {
	if (!closed_) {
		closed_ = true;
		close_handler(cid_);
	} 
	return 0;
}

int Svc::close_fd(void) {
	if (closed_) {
		LOG_INFO("close fd = %d, cid = %d", this->get_fd(), cid_);
		::close(this->get_fd());
	}
	return 0;
}

int Svc::get_peer_addr(std::string &ip, int &port) {
	struct sockaddr_in sa;
	socklen_t len = sizeof(sa);

	if (::getpeername(this->get_fd(), (struct sockaddr *)&sa, &len) < 0) {
		LOG_ERROR("getpeername wrong, fd:%d", this->get_fd());
		return -1;
	}

	std::stringstream stream;
	stream << inet_ntoa(sa.sin_addr);

	ip = stream.str();
	port = ntohs(sa.sin_port);
	return 0;
}

int Svc::get_local_addr(std::string &ip, int &port) {
	struct sockaddr_in sa;
	socklen_t len = sizeof(sa);

	if (::getsockname(this->get_fd(), (struct sockaddr *)&sa, &len) < 0) {
		LOG_ERROR("getsockname wrong, fd:%d", this->get_fd());
		return -1;
	}

	std::stringstream stream;
	stream << inet_ntoa(sa.sin_addr);

	ip = stream.str();
	port = ntohs(sa.sin_port);
	return 0;
}
