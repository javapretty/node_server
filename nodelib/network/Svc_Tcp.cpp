/*
 * Svc_Tcp.cpp
 *
 *  Created on: Apr 19,2016
 *      Author: zhangyalei
 */

#include <sys/socket.h>
#include <errno.h>
#include "Base_Enum.h"
#include "Svc_Tcp.h"

Svc_Tcp::Svc_Tcp(): Svc_Handler() { }

Svc_Tcp::~Svc_Tcp() { }

int Svc_Tcp::handle_send(void) {
	if (parent_->closed())
		return -1;

	int cid = parent_->get_cid();
	while (!send_buffer_list_.empty()) {
		size_t sum_bytes = 0;
		std::vector<iovec> iov_vec;
		std::vector<Byte_Buffer *> iov_buff;
		iov_vec.clear();
		iov_buff.clear();

		//将所有需要发生的数据构造成iov_vec，进行聚集写
		if (send_buffer_list_.construct_iov(iov_vec, iov_buff, sum_bytes) == 0) {
			LOG_ERROR("construct_iov fail");
			return -1;
		}

		int ret = ::writev(parent_->get_fd(), &*iov_vec.begin(), iov_vec.size());
		if (ret == -1) {
			if (errno == EINTR) { //被打断, 重写
				continue;
			} else if (errno == EWOULDBLOCK) { //EAGAIN,下一次超时再写
				return 1;
			} else { //其他错误，丢掉该客户端全部数据
				LOG_ERROR("writev return -1, cid:%d ip:%s port:%d", cid, parent_->get_peer_ip().c_str(), parent_->get_peer_port());
				parent_->handle_close();
				return ret;
			}
		} else {
			if ((size_t)ret == sum_bytes) { //本次全部写完, 尝试继续写
				for (std::vector<Byte_Buffer *>::iterator it = iov_buff.begin(); it != iov_buff.end(); ++it) {
					parent_->push_buffer(cid, *it);
				}
				send_buffer_list_.pop_front(iov_buff.size());
				continue;
			} else { //本次未写完, 丢掉已发送的数据, 剩余数据下一次超时再写
				size_t writed_bytes = ret, remove_count = 0;
				Byte_Buffer *buf = 0;
				//计算已经发生需要丢弃的数据
				for (std::vector<Byte_Buffer *>::iterator it = iov_buff.begin(); it != iov_buff.end(); ++it) {
					buf = *it;
					if (writed_bytes >= buf->readable_bytes()) {
						++remove_count;
						writed_bytes -= buf->readable_bytes();
						parent_->push_buffer(cid, buf);
					} else {
						break;
					}
				}
				send_buffer_list_.pop_front(remove_count, writed_bytes);
				return ret;
			}
		}
	}
	return 0;
}

int Svc_Tcp::handle_pack(Buffer_Vector &buffer_vec) {
	Byte_Buffer *front_buf = nullptr;
	Byte_Buffer *second_buf = nullptr;
	while (!recv_buffer_list_.empty()) {
		front_buf = recv_buffer_list_.front();
		if (!front_buf) {
			LOG_ERROR("front_buf is null");
			continue;
		}

		int32_t eid = 0;	//终端id
		int32_t cid = 0;	//链接id
		size_t rd_idx_orig = front_buf->get_read_idx();
		front_buf->read_int32(eid);
		front_buf->read_int32(cid);

		if (front_buf->readable_bytes() <= 0) { //数据块异常, 关闭该连接
			LOG_ERROR("cid:%d fd:%d, buffer read bytes < 0", cid, parent_->get_fd());
			recv_buffer_list_.pop_front();
			front_buf->reset();
			parent_->push_buffer(cid, front_buf);
			parent_->handle_close();
			return -1;
		}

		//LOG_WARN("buffer_len:%d", front_buf->readable_bytes());
		if (front_buf->readable_bytes() < 2) { //2字节的包长度标识都不够
			front_buf->set_read_idx(rd_idx_orig);
			if ((second_buf = recv_buffer_list_.merge_first_second()) == 0) {
				return -1;
			} else {
				//释放第二个包，第一个包继续循环处理
				parent_->push_buffer(cid, second_buf);
				continue;
			}
		}

		//解析包头
		int len = 0;	//包长
		uint8_t pkg_header = 0;
		uint8_t pkg_header_flag = 0;
		uint8_t pkg_type = 0;
		bool compress = false;
		bool client_msg = parent_->client();
		front_buf->peek_uint8(pkg_header);
		pkg_header_flag = pkg_header & 0xc0;
		if(pkg_header_flag == RPC_PKG<<6) {
			pkg_type = RPC_PKG;
			uint16_t read_len = 0;
			front_buf->peek_uint16(read_len);
			compress = IS_PACKAGE_COMPRESSED(read_len);
			len = GET_RPC_PKG_LENGTH(read_len, read_len);
		} else if(pkg_header_flag == TYPE_PKG<<6) {
			pkg_type = TYPE_PKG;
			uint32_t read_len = 0;
			front_buf->peek_uint32(read_len);
			compress = IS_PACKAGE_COMPRESSED(read_len);
			len = GET_TYPE_PKG_LENGTH(read_len, read_len);
		}

		//LOG_WARN("eid:%d, cid:%d, pkg_header:%d, pkg_header_flag:%d, len:%d",  eid, cid, pkg_header, pkg_header_flag, len);
		if (len == 0 || len > (int)max_pack_size_) { //包长度标识为0, 包长度标识超过max_pack_size_, 即视为无效包, 异常, 关闭该连接
			LOG_ERROR("buffer len error, eid:%d, cid:%d, pkg_header:%d, pkg_header_flag:%d, len:%d",  eid, cid, pkg_header, pkg_header_flag, len);
			front_buf->log_binary_data(512);
			recv_buffer_list_.pop_front();
			front_buf->reset();
			parent_->push_buffer(cid, front_buf);
			parent_->handle_close();
			return -1;
		}

		if (compress) {
			//如果包压缩过，进行解压缩
		}

		int len_size = 2;
		if(pkg_type == TYPE_PKG) {
			len_size = 4;
		}
		int data_len = front_buf->readable_bytes() - len_size;
		if (data_len == len) {		//正常的包，推送到逻辑层
			//LOG_WARN("normal package, eid:%d, cid:%d, pkg_header:%d, pkg_header_flag:%d, data_len:%d, len:%d", eid, cid, pkg_header, pkg_header_flag, data_len, len);

			int wr_idx = front_buf->get_write_idx();
			front_buf->set_write_idx(front_buf->get_read_idx());
			front_buf->write_uint8(pkg_type << 4 | TCP);
			front_buf->write_uint8(client_msg);
			front_buf->set_write_idx(wr_idx);
			//将读指针重置到初始位置
			front_buf->set_read_idx(rd_idx_orig);
			recv_buffer_list_.pop_front();
			buffer_vec.push_back(front_buf);
			continue;
		} else {
			if (data_len < len) {	//半包，合并前两个包
				//LOG_WARN("half package, eid:%d, cid:%d, pkg_header:%d, pkg_header_flag:%d, data_len:%d, len:%d", eid, cid, pkg_header, pkg_header_flag, data_len, len);

				front_buf->set_read_idx(rd_idx_orig);
				if ((second_buf = recv_buffer_list_.merge_first_second()) == 0) {
					return -1;
				} else {
					//释放第二个包，第一个包继续循环处理
					parent_->push_buffer(cid, second_buf);
					continue;
				}
			}

			if (data_len > len) { //粘包，需要分包
				//LOG_WARN("splice package, eid:%d, cid:%d, pkg_header:%d, pkg_header_flag:%d, data_len:%d, len:%d", eid, cid, pkg_header, pkg_header_flag, data_len, len);

				size_t wr_idx_orig = front_buf->get_write_idx();
				//将第一个包拷贝到data_buf,丢到逻辑层
				Byte_Buffer *data_buf = parent_->pop_buffer(cid);
				data_buf->reset();
				data_buf->write_int32(eid);
				data_buf->write_int32(cid);
				data_buf->write_uint8(pkg_type << 4 | TCP);
				data_buf->write_uint8(client_msg);
				if(pkg_type == TYPE_PKG) {
					//type_pkg包长占四个字节，第一个字节写入pkg_type << 4 | TCP，第二个字节写入client_msg，后面两个字节无用
					data_buf->write_uint16(0);
				}
				data_buf->copy(front_buf->get_read_ptr() + len_size, len);
				buffer_vec.push_back(data_buf);

				//第二个包的读指针=第一个包读指针+len_size+包长-sizeof(eid)-sizeof(cid)
				size_t second_idx = front_buf->get_read_idx() + len_size + len - sizeof(int32_t) * 2;
				front_buf->set_read_idx(second_idx);
				front_buf->set_write_idx(second_idx);
				front_buf->write_int32(eid);
				front_buf->write_int32(cid);
				front_buf->set_write_idx(wr_idx_orig);
				continue;
			}
		}
	}

	return 0;
}
