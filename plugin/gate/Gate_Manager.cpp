/*
 * Gate_Manager.cpp
 *
 *  Created on: Nov 8, 2016
 *      Author: zhangyalei
 */

#include "Base_Function.h"
#include "Node_Manager.h"
#include "Gate_Manager.h"

Gate_Manager::Gate_Manager(void):
	cid_session_map_(10000),
	sid_session_map_(10000) { }

Gate_Manager::~Gate_Manager(void) { }

Gate_Manager *Gate_Manager::instance_;

Gate_Manager *Gate_Manager::instance(void) {
	if (instance_ == 0)
		instance_ = new Gate_Manager;
	return instance_;
}

void Gate_Manager::run_handler(void) {
	process_list();
}

int Gate_Manager::process_list(void) {
	Byte_Buffer *buffer = nullptr;
	while (true) {
		//此处加锁是为了防止本线程其他地方同时等待条件变量成立
		notify_lock_.lock();

		//put wait in while cause there can be spurious wake up (due to signal/ENITR)
		while (buffer_list_.empty()) {
			notify_lock_.wait();
		}

		buffer = buffer_list_.pop_front();
		if(buffer != nullptr) {
			int32_t eid = 0;
			int32_t cid = 0;
			uint8_t protocol = 0;
			uint8_t client_msg = 0;
			uint8_t msg_id = 0;
			uint8_t msg_type = 0;
			uint32_t sid = 0;
			buffer->read_int32(eid);
			buffer->read_int32(cid);
			buffer->read_uint8(protocol);
			buffer->read_uint8(client_msg);
			buffer->read_uint8(msg_id);
			if (client_msg) {
				msg_type = C2S;
			} else {
				buffer->read_uint8(msg_type);
				buffer->read_uint32(sid);
			}

			transmit_msg(eid, cid, msg_id, msg_type, sid, buffer);
		}

		//操作完成解锁条件变量
		notify_lock_.unlock();
	}
	return 0;
}

int Gate_Manager::transmit_msg(int eid, int cid, int msg_id, int msg_type, uint32_t sid, Byte_Buffer *buffer) {
	int buf_eid = 0;
	int buf_cid = 0;
	int buf_msg_id = msg_id;
	int buf_msg_type = 0;
	int buf_sid = 0;

	if (msg_type == C2S) {
		Session *session = find_session_by_cid(cid);
		if (!session) {
			LOG_ERROR("find_session_by_cid error, eid:%d, cid:%d, msg_type:%d, msg_id:%d, sid:%d", eid, cid, msg_type, msg_id, sid);
			return -1;
		}

		buf_eid = session->game_eid;
		buf_cid = session->game_cid;
		buf_msg_type = NODE_C2S;
		buf_sid = session->sid;
	} else if (msg_type == NODE_S2C) {
		Session *session = find_session_by_sid(sid);
		if (!session) {
			LOG_ERROR("find_session_by_sid error, eid:%d, cid:%d, msg_type:%d, msg_id:%d, sid:%d", eid, cid, msg_type, msg_id, sid);
			return -1;
		}

		buf_eid = session->client_eid;
		buf_cid = session->client_cid;
		buf_msg_type = S2C;
		buf_sid = session->sid;
	}
	NODE_MANAGER->send_msg(buf_eid, buf_cid, buf_msg_id, buf_msg_type, buf_sid, buffer);
	NODE_MANAGER->push_buffer(eid, cid, buffer);
	return 0;
}

int Gate_Manager::add_session(Session *session) {
	if (!session) {
		LOG_ERROR("node_id:%d, node_name:%s add session error", NODE_MANAGER->node_info().node_id, NODE_MANAGER->node_info().node_name.c_str());
		return -1;
	}

	cid_session_map_.insert(std::make_pair(session->client_cid, session));
	sid_session_map_.insert(std::make_pair(session->sid, session));
	return 0;
}

int Gate_Manager::remove_session(int cid) {
	Session_Map::iterator iter = cid_session_map_.find(cid);
	if (iter != sid_session_map_.end()) {
		Session *session = iter->second;
		cid_session_map_.erase(iter);
		sid_session_map_.erase(session->sid);
		session_pool_.push(session);
	}
	return 0;
}

Session *Gate_Manager::find_session_by_cid(int cid) {
	Session_Map::iterator iter = cid_session_map_.find(cid);
	if (iter != cid_session_map_.end()) {
		return iter->second;
	}
	return nullptr;
}

Session *Gate_Manager::find_session_by_sid(int sid) {
	Session_Map::iterator iter = sid_session_map_.find(sid);
	if (iter != sid_session_map_.end()) {
		return iter->second;
	}
	return nullptr;
}
