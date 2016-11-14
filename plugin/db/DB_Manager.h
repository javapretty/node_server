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
#include "Node_Define.h"

enum NODE_CODE {
	LOAD_DB_DATA_FAIL				= 1,		//加载db数据失败
	SAVE_DB_DATA_FAIL				= 2,		//保存db数据失败
	DELETE_DB_DATA_FAIL			= 3,		//删除db数据失败
	LOAD_RUNTIME_DATA_FAIL		= 4,		//加载运行时数据失败
	SAVE_RUNTIME_DATA_FAIL		= 5,		//保存运行时数据失败
	DELETE_RUNTIME_DATA_FAIL= 6,		//删除运行时数据失败
	SAVE_DB_DATA_SUCCESS			=	7,		//保存db数据成功
};

enum DB_MESSAGE_CMD {
	SYNC_NODE_CODE = 1,
	SYNC_NODE_INFO = 2,
	SYNC_GET_TABLE_INDEX = 246,
	SYNC_RES_TABLE_INDEX = 247,
	SYNC_GENERATE_ID = 248,
	SYNC_DB_RES_ID = 249,
	SYNC_LOAD_DB_DATA = 250,
	SYNC_SAVE_DB_DATA = 251,
	SYNC_DELETE_DB_DATA = 252,
	SYNC_LOAD_RUNTIME_DATA = 253,
	SYNC_SAVE_RUNTIME_DATA = 254,
	SYNC_DELETE_RUNTIME_DATA = 255,
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

	//key_index操作接口
	void get_table_index(int cid, int sid, Bit_Buffer &buffer);
	void generate_id(int cid, int sid, Bit_Buffer &buffer);

	//db数据操作接口
	void load_db_data(int cid, int sid, Bit_Buffer &buffer);
	void save_db_data(int cid, int sid, Bit_Buffer &buffer);
	void delete_db_data(int cid, int sid, Bit_Buffer &buffer);

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
