/*
 * Svc_Http.cpp
 *
 *  Created on: Aug 16,2016
 *      Author: zhangyalei
 */

#include <sys/socket.h>
#include <errno.h>
#include "Bit_Buffer.h"
#include "Http_Parser_Wrap.h"
#include "Svc_Http.h"

Svc_Http::Svc_Http(): Svc_Handler() {}

Svc_Http::~Svc_Http() {}

int Svc_Http::handle_send(void) {
	if (parent_->closed())
		return -1;

	int cid = parent_->get_cid();
	Byte_Buffer *front_buf = 0;
	Bit_Buffer data_buf;
	while (!send_buffer_list_.empty()) {
		front_buf = send_buffer_list_.front();

		//构建http消息头
		data_buf.reset();
		data_buf.set_ary(front_buf->get_read_ptr(), front_buf->readable_bytes());
		std::string str_content = "";
		data_buf.read_str(str_content);
		int content_len = str_content.length();
		int total_len = content_len + strlen(HTTP_RESPONSE_HTML) + 32;
		char *str_http = new char[total_len];
		snprintf(str_http, total_len, HTTP_RESPONSE_HTML, content_len, str_content.c_str());

		//将buffer内容写入到socket
		size_t sum_bytes = strlen(str_http);
		int ret = ::write(parent_->get_fd(), str_http, sum_bytes);
		//不要忘记删除内存
		delete []str_http;
		if (ret == -1) {
			if (errno == EINTR) { //被打断, 重写
				continue;
			}
			else if (errno == EWOULDBLOCK) { //EAGAIN,下一次超时再写
				return 1;
			}
			else { //其他错误，丢掉该客户端全部数据
				LOG_ERROR("writev error, cid:%d ip:%s port:%d", cid, parent_->get_peer_ip().c_str(), parent_->get_peer_port());
				parent_->handle_close();
				return ret;
			}
		} else {
			if ((size_t)ret == sum_bytes) { //本次全部写完, 尝试继续写
				parent_->push_buffer(cid, front_buf);
				send_buffer_list_.pop_front();
				continue;
			}
			else { //未写完, 下一次超时再写
				return ret;
			}
		}
	}
	return 0;
}

int Svc_Http::handle_pack(Buffer_Vector &buffer_vec) {
	int32_t eid = 0;
	int32_t cid = 0;
	Byte_Buffer *front_buf = 0;

	while (! recv_buffer_list_.empty()) {
		front_buf = recv_buffer_list_.front();
		if (! front_buf) {
			LOG_ERROR("front_buf == 0");
			continue;
		}

		front_buf->read_int32(eid);
		front_buf->read_int32(cid);
		if (front_buf->readable_bytes() <= 0) { //数据块异常, 关闭该连接
			LOG_ERROR("cid:%d fd:%d, buffer read bytes<0", cid, parent_->get_fd());
			recv_buffer_list_.pop_front();
			front_buf->reset();
			parent_->push_buffer(parent_->get_cid(), front_buf);
			parent_->handle_close();
			return -1;
		}

		Http_Parser_Wrap http_parser;
		http_parser.parse_http_content(front_buf->get_read_ptr(), front_buf->readable_bytes());

		Byte_Buffer *data_buf = parent_->pop_buffer(cid);
		data_buf->reset();
		data_buf->write_int32(eid);
		data_buf->write_int32(cid);
		data_buf->write_uint8(RPC_PKG << 4 | HTTP);
		data_buf->write_string(http_parser.get_body_content());
		buffer_vec.push_back(data_buf);
		recv_buffer_list_.pop_front();
	}

	return 0;
}
