/*
 * DB_Manager.h
 *
 *  Created on: Nov 9, 2016
 *      Author: zhangyalei
 */

#ifndef DB_MANAGER_H_
#define DB_MANAGER_H_

#include <unordered_map>
#include "Bit_Buffer.h"
#include "Buffer_List.h"
#include "Object_Pool.h"
#include "Thread.h"
#include "DB_Struct.h"
#include "Node_Define.h"

enum DB_ID {
	DB_GAME = 1001,
	DB_LOG = 1002,
};

enum DB_MESSAGE_CMD {
	SYNC_ERROR_CODE = 1,
	SYNC_NODE_INFO = 2,
	SYNC_GAME_DB_LOAD_PLAYER = 201,
	SYNC_GAME_DB_SAVE_PLAYER = 202,
	SYNC_DB_GAME_PLAYER_INFO = 203,
	SYNC_PUBLIC_DB_LOAD_DATA = 205,
	SYNC_PUBLIC_DB_SAVE_DATA = 206,
	SYNC_PUBLIC_DB_DELETE_DATA = 207,
};

class DB_Manager: public Thread {
	typedef Buffer_List<Mutex_Lock> Data_List;
	typedef std::vector<Node_Info> Node_List;
public:
	static DB_Manager *instance(void);

	void init(const Node_Info &node_info) { node_info_ = node_info; }

	virtual void run_handler(void);
	virtual int process_list(void);

	inline void push_buffer(Byte_Buffer *buffer) {
		notify_lock_.lock();
		buffer_list_.push_back(buffer);
		notify_lock_.signal();
		notify_lock_.unlock();
	}

	void load_player(int cid, int sid, Bit_Buffer &buffer);
	void save_player(int cid, int sid, Bit_Buffer &buffer);

	void load_db_data(int db_id, std::string table_name, int64_t key_index, Byte_Buffer &buffer);
	void load_single_data(int db_id, DB_Struct *db_struct, int64_t key_index, Byte_Buffer &buffer);
	void save_db_data(int save_flag, int db_id, std::string table_name, Bit_Buffer &buffer);

private:
	DB_Manager(void);
	virtual ~DB_Manager(void);
	DB_Manager(const DB_Manager &);
	const DB_Manager &operator=(const DB_Manager &);

private:
	static DB_Manager *instance_;

	Node_Info node_info_;					//节点信息
	Data_List buffer_list_;				//消息列表
	Node_List log_list_;						//log链接器列表
};

#define DB_MANAGER DB_Manager::instance()

#endif /* DB_MANAGER_H_ */
