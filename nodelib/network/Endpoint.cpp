/*
 * Endpoint.cpp
 *
 *  Created on: Sep 22, 2016
 *      Author: zhangyalei
 */

#include "Endpoint.h"

int Endpoint_Accept::accept_svc(int connfd) {
	Svc *svc = endpoint_->pop_svc(connfd);
	if (!svc) {
		LOG_ERROR("pop svc return null");
		return -1;
	}

	LOG_INFO("endpoint_name:%s eid:%d server_ip:%s port:%d peer_ip:%s port:%d cid:%d",
			endpoint_->endpoint_info().endpoint_name.c_str(), endpoint_->endpoint_info().endpoint_id,
			endpoint_->endpoint_info().server_ip.c_str(), endpoint_->endpoint_info().server_port, 
			svc->get_peer_ip().c_str(), svc->get_peer_port(), svc->get_cid());
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
int Endpoint_Connect::connect_svc(int connfd) {
	Svc *svc = endpoint_->pop_svc(connfd);
	if (!svc) {
		LOG_ERROR("pop svc return null");
		return -1;
	}
	return svc->get_cid();
}

////////////////////////////////////////////////////////////////////////////////
int Endpoint_Network::drop_handler(int cid) {
	LOG_INFO("endpoint_name:%s eid:%d, server_ip:%s server_port:%d cid:%d",
			endpoint_->endpoint_info().endpoint_name.c_str(), endpoint_->endpoint_info().endpoint_id,
			endpoint_->endpoint_info().server_ip.c_str(), endpoint_->endpoint_info().server_port, cid);

	endpoint_->post_drop_cid(cid);
	return endpoint_->push_svc(cid);
}

Svc *Endpoint_Network::find_svc(int cid) {
	return endpoint_->find_svc(cid);
}

////////////////////////////////////////////////////////////////////////////////
Byte_Buffer *Endpoint_Svc::pop_buffer(int cid) {
	return endpoint_->pop_buffer(cid);
}

int Endpoint_Svc::push_buffer(int cid, Byte_Buffer *buffer) {
	return endpoint_->push_buffer(cid, buffer);
}

int Endpoint_Svc::post_buffer(Byte_Buffer* buffer) {
	endpoint_->post_buffer(buffer);
	return 0;
}

int Endpoint_Svc::register_network_handler(void) {
	if (! reg_network()) {
		endpoint_->network().register_svc(this);
		set_reg_network(true);
	}
	return 0;
}

int Endpoint_Svc::unregister_network_handler(void) {
	if (reg_network()) {
		endpoint_->network().unregister_svc(this);
		set_reg_network(false);
	}
	return 0;
}

int Endpoint_Svc::close_handler(int cid) {
	endpoint_->network().push_drop(cid);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
Endpoint::Endpoint(void) { }

Endpoint::~Endpoint(void) { }

int Endpoint::send_buffer(int cid, Byte_Buffer &buffer) {
	Svc *svc = find_svc(cid);
	if (!svc) {
		LOG_ERROR("find svc error, endpoint_name:%s eid:%d, server_ip:%s server_port:%d cid:%d",
				endpoint_info_.endpoint_name.c_str(), endpoint_info_.endpoint_id, endpoint_info_.server_ip.c_str(), endpoint_info_.server_port, cid);
		return -1;
	}

	Byte_Buffer *buf = pop_buffer(cid);
	if (!buf) {
		LOG_ERROR("pop buffer error, endpoint_name:%s eid:%d, cid:%d", endpoint_info_.endpoint_name.c_str(), endpoint_info_.endpoint_id, cid);
		return -1;
	}

	buf->reset();
	buf->copy(&buffer);
	if ((svc->push_send_buffer(buf)) != 0) {
		push_buffer(cid, buf);
	}

	return 0;
}

Svc *Endpoint::pop_svc(int connfd) {
	Endpoint_Svc *svc = svc_pool_.pop();
	if (!svc) {
		return nullptr;
	}

	int cid = svc_list_.record_svc(svc);
	if (cid < 0) {
		svc_pool_.push(svc);
		return nullptr;
	}

	svc->reset();
	svc->create_handler(endpoint_info_.protocol_type);
	svc->set_endpoint(this);
	svc->set_fd(connfd);
	svc->set_cid(cid);
	svc->set_alive(true);	//新建立的svc设置为活的，防止被网络心跳T掉
	svc->set_client(endpoint_info_.endpoint_type == CLIENT_SERVER);
	svc->set_peer_addr();
	svc->register_network_handler();
	return svc;
}

int Endpoint::push_svc(int cid) {
	Endpoint_Svc *svc = 0;
	svc_list_.get_used_svc(cid, svc);
	if (svc) {
		svc->close_fd();
		svc->reset();
		svc_list_.erase_svc(cid);
		svc_pool_.push(svc);
	}
	return 0;
}

Svc *Endpoint::find_svc(int cid) {
	Endpoint_Svc *svc = nullptr;
	svc_list_.get_used_svc(cid, svc);
	return svc;
}

int Endpoint::get_svc_info(Svc_Info &svc_info) {
	svc_info.endpoint_gid = endpoint_info_.endpoint_gid;
	svc_info.endpoint_id = endpoint_info_.endpoint_id;
	svc_info.svc_pool_free_size = svc_pool_.free_obj_list_size();
	svc_info.svc_pool_used_size = svc_pool_.used_obj_list_size();
	svc_info.svc_list_size = svc_list_.size();
	buffer_pool_group_.get_buffer_group(svc_info.buffer_group_list);
	return 0;
}

void Endpoint::free_pool(void) {
	buffer_pool_group_.shrink_all();
	svc_pool_.shrink_all();
}
