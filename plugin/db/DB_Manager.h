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

enum Public_Data_Type {
	CREATE_GUILD_DATA	= 0, 	//创建公会数据
	GUILD_DATA					= 1, 	//公会数据
	RANK_DATA 					= 2,	//排行榜数据
};

enum Runtime_Data_Type {
	RUNTIME_DATA = 1,
};

enum DB_MESSAGE_CMD {
	SYNC_ERROR_CODE = 1,
	SYNC_NODE_INFO = 2,
	SYNC_GAME_DB_LOAD_PLAYER = 202,
	SYNC_GAME_DB_PLAYER_DATA = 203,
	SYNC_PUBLIC_DB_LOAD_DATA = 204,
	SYNC_PUBLIC_DB_DATA = 205,
	SYNC_PUBLIC_DB_DELETE_DATA = 206,
	SYNC_DB_LOAD_RUNTIME_DATA = 207,
	SYNC_DB_RUNTIME_DATA = 208,
	SYNC_DB_DELETE_RUNTIME_DATA = 209,
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

	//玩家数据操作接口
	void load_player(int cid, int sid, Bit_Buffer &buffer);
	void save_player(int cid, int sid, Bit_Buffer &buffer);

	//公共数据操作接口
	void load_public_data(int cid, int sid, Bit_Buffer &buffer);
	void save_public_data(int cid, int sid, Bit_Buffer &buffer);
	void delete_public_data(int cid, int sid, Bit_Buffer &buffer);

	//运行时数据操作接口
	void load_runtime_data(int cid, int sid, Bit_Buffer &buffer);
	void save_runtime_data(int cid, int sid, Bit_Buffer &buffer);
	void delete_runtime_data(int cid, int sid, Bit_Buffer &buffer);

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
