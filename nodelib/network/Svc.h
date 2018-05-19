/*
 * Svc.h
 *
 *  Created on: Dec 16,2015
 *      Author: zhangyalei
 */

#ifndef SVC_H_
#define SVC_H_

#include "Buffer_List.h"
#include "Event_Handler.h"

const static int SVC_MAX_LIST_SIZE = 10240;
const static int SVC_MAX_PACK_SIZE = 60 * 1024;

class Svc;
class Svc_Handler {
public:
	typedef Buffer_List<Mutex_Lock> Data_List;
	typedef std::vector<Byte_Buffer *> Buffer_Vector;
public:
	Svc_Handler(void);
	virtual ~Svc_Handler(void);

	void reset(void);
	void set_parent(Svc *parent) { parent_ = parent; }

	int push_recv_buffer(Byte_Buffer *buffer);
	int push_send_buffer(Byte_Buffer *buffer);

	virtual int handle_send(void) = 0;
	virtual int handle_pack(Buffer_Vector &buffer_vec) = 0;

protected:
	Svc *parent_;
	Data_List recv_buffer_list_;
	Data_List send_buffer_list_;

	size_t max_list_size_;
	size_t max_pack_size_;
};

class Endpoint;
class Svc: public Event_Handler {
public:
	typedef std::vector<Byte_Buffer *> Buffer_Vector;
	
	Svc(void);
	virtual ~Svc(void);

	void reset(void);

	virtual Byte_Buffer *pop_buffer(int cid);
	virtual int push_buffer(int cid, Byte_Buffer *buffer);
	virtual int post_buffer(Byte_Buffer* buffer);

	virtual int register_network_handler(void);
	virtual int unregister_network_handler(void);

	virtual int close_handler(int cid);

	int create_handler(int protocol_type);

	virtual int handle_input(void);
	virtual int handle_send(void);
	virtual int handle_close(void);
	int close_fd(void);

	inline int push_recv_buffer(Byte_Buffer *buffer) {
		if (closed_) {
			return -1;
		} else {
			return handler_->push_recv_buffer(buffer);
		}
	}
	inline int push_send_buffer(Byte_Buffer *buffer) {
		if (closed_) {
			return -1;
		} else {
			return handler_->push_send_buffer(buffer);
		}
	}

	void set_endpoint(Endpoint *endpoint);
	
	inline void set_cid(int cid) { cid_ = cid; }
	inline int get_cid(void) { return cid_; }

	inline void set_closed(bool is_closed) { closed_ = is_closed; }
	inline bool closed(void) { return closed_; }

	inline void set_alive(bool alive) { alive_ = alive; }
	inline bool alive(void) { return alive_; }

	inline void set_client(bool client) { client_ = client; }
	inline bool client(void) { return client_; }

	inline void set_reg_network(bool reg_network) { reg_network_ = reg_network; }
	inline bool reg_network(void) { return reg_network_; }

	inline void set_send_data(bool send_data) { send_data_ = send_data; }
	inline bool send_data(void) { return send_data_; }

	inline void set_peer_addr(void) { get_peer_addr(peer_ip_, peer_port_); }
	int get_peer_addr(std::string &ip, int &port);
	int get_local_addr(std::string &ip, int &port);

	inline std::string &get_peer_ip() { return peer_ip_; }
	inline int get_peer_port() { return peer_port_; }

protected:
	Endpoint *endpoint_;

private:
	int cid_;				//网络连接cid
	bool closed_;			//是否关闭
	bool alive_;			//是否活跃的连接
	bool client_;			//是否为client
	bool reg_network_;		//是否注册网络
	bool send_data_;		//是否发送数据
	std::string peer_ip_;	//client ip
	int peer_port_;			//client port

	Svc_Handler *handler_;	//svc处理句柄
};

inline void Svc::reset(void) {
	cid_ = 0;
	closed_ = false;
	alive_ = false;
	client_ = false;
	reg_network_ = false;
	send_data_ = false;
	peer_ip_.clear();
	peer_port_ = 0;

	if (handler_) {
		handler_->reset();
		delete handler_;
		handler_ = 0;
	}
	endpoint_ = 0;
	Event_Handler::reset();
}

#endif /* SVC_H_ */
