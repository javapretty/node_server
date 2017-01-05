/*
 * DB_Manager.h
 *
 *  Created on: Nov 9, 2016
 *      Author: zhangyalei
 */

#ifndef DB_MANAGER_H_
#define DB_MANAGER_H_

#include <unordered_set>
#include <unordered_map>
#include "Bit_Buffer.h"
#include "Buffer_List.h"
#include "Object_Pool.h"
#include "List.h"
#include "Thread.h"
#include "Node_Define.h"

enum Enpoint_Gid {
	GID_DATA_SERVER		= 1,
	GID_DATA_CONNECTOR	= 2,
};

enum Msg_Id {
	SYNC_NODE_INFO = 1,
	SYNC_DB_RET_CODE = 244,
	SYNC_SELECT_DB_DATA = 245,
	SYNC_RES_SELECT_DB_DATA = 246,
	SYNC_GENERATE_ID = 247,
	SYNC_RES_GENERATE_ID = 248,
	SYNC_LOAD_DB_DATA = 249,
	SYNC_SAVE_DB_DATA = 250,
	SYNC_DELETE_DB_DATA = 251,
	SYNC_LOAD_RUNTIME_DATA = 252,
	SYNC_SAVE_RUNTIME_DATA = 253,
	SYNC_DELETE_RUNTIME_DATA = 254,
	SYNC_NODE_CODE = 255,
};

class DB_Manager: public Thread {
	typedef Buffer_List<Mutex_Lock> Data_List;
	typedef List<int, Mutex_Lock> Int_List;
	typedef std::unordered_map<std::string, int> Idx_Value_Map;
	typedef std::unordered_map<uint, int> Session_Map;
	typedef std::unordered_set<uint> UInt_Set;
	typedef std::vector<int> Int_Vec;
public:
	static DB_Manager *instance(void);

	int init(const Node_Info &node_info);
	virtual void run_handler(void);
	virtual int process_list(void);
	int tick(int tick_time);

	inline void push_buffer(Byte_Buffer *buffer) {
		notify_lock_.lock();
		buffer_list_.push_back(buffer);
		notify_lock_.signal();
		notify_lock_.unlock();
	}
	inline void push_tick(int tick_time) {
		notify_lock_.lock();
		tick_list_.push_back(tick_time);
		notify_lock_.signal();
		notify_lock_.unlock();
	}

private:
	DB_Manager(void);
	virtual ~DB_Manager(void);
	DB_Manager(const DB_Manager &);
	const DB_Manager &operator=(const DB_Manager &);

private:
	//根据条件查询db接口
	void select_db_data(Msg_Head &msg_head, Bit_Buffer &buffer);
	void generate_id(Msg_Head &msg_head, Bit_Buffer &buffer);

	//db数据操作接口
	void load_db_data(Msg_Head &msg_head, Bit_Buffer &buffer);
	void save_db_data(Msg_Head &msg_head, Bit_Buffer &buffer);
	void delete_db_data(Msg_Head &msg_head, Bit_Buffer &buffer);

	//运行时数据操作接口
	void load_runtime_data(Msg_Head &msg_head, Bit_Buffer &buffer);
	void save_runtime_data(Msg_Head &msg_head, Bit_Buffer &buffer);
	void delete_runtime_data(Msg_Head &msg_head, Bit_Buffer &buffer);

	//构造返回buffer
	void build_ret_buffer(Bit_Buffer &buffer, uint8_t opt_msg_id, int8_t ret, std::string struct_name, std::string query_name, int64_t key_index) {
		buffer.write_uint(opt_msg_id, 8);
		buffer.write_int(ret, 8);
		buffer.write_str(struct_name.c_str());
		buffer.write_str(query_name.c_str());
		buffer.write_int64(key_index);
	}

private:
	static DB_Manager *instance_;

	Idx_Value_Map idx_value_map_;	//存放idx值信息map
	int save_idx_tick_;				//保存idx表tick时间
	int db_id_;						//数据库id
	std::string struct_name_;		//idx表结构体名称

	Data_List buffer_list_;			//消息列表
	Int_List tick_list_;			//定时器tick列表

	Node_Info node_info_;			//节点信息
	Session_Map session_map_;		//转发到connector进程session信息
	UInt_Set sid_set_;				//本进程sid列表
	int data_node_idx_;				//data链接器id索引
	int data_connector_size_;		//data链接器数量
	Int_Vec data_connector_list_;	//data链接器cid列表
	UInt_Set data_fork_list_;		//data进程启动列表
};

#define DB_MANAGER DB_Manager::instance()

#endif /* DB_MANAGER_H_ */
