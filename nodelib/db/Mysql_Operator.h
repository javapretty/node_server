/*
 * Mysql_Operator.h
 *
 *  Created on: Jul 20, 2016
 *      Author: zhangyalei
 */

#ifndef MYSQL_OPERATOR_H_
#define MYSQL_OPERATOR_H_

#include <unordered_map>
#include "Mysql_Conn.h"
#include "DB_Operator.h"

class Mysql_Operator : public DB_Operator {
public:
	typedef std::unordered_map<int, Mysql_Conn *> Connection_Map;
	//typedef std::unordered_map<std::string, Connection_Map *> DB_Connection_Map;

	Mysql_Operator(void);
	virtual ~Mysql_Operator(void);

	virtual bool connect_to_db(int db_id, std::string &ip, int port, std::string &user, std::string &password, std::string &pool_name);

	//根据条件查询db数据
	virtual int select_db_data(int db_id, DB_Struct *db_struct, std::string &condition_name, std::string &condition_value, std::string &query_name, std::string &query_type, Bit_Buffer &buffer);
	//数据库加载数据到内存buffer,保存数据通过内存buffer操作
	virtual int load_data(int db_id, DB_Struct *db_struct, int64_t key_index, std::vector<Bit_Buffer *> &buffer_vec);
	virtual int save_data(int db_id, DB_Struct *db_struct, Bit_Buffer &buffer);
	virtual int delete_data(int db_id, DB_Struct *db_struct, int64_t key_index);

private:
	Mysql_Conn *get_connection(int db_id);

	int load_data_single(DB_Struct *db_struct, sql::ResultSet *result, Bit_Buffer &buffer);
	int load_data_arg(DB_Struct *db_struct, const Field_Info &field_info, sql::ResultSet *result, Bit_Buffer &buffer);
	int load_data_vector(DB_Struct *db_struct, const Field_Info &field_info, sql::ResultSet *result, Bit_Buffer &buffer);
	int load_data_map(DB_Struct *db_struct, const Field_Info &field_info, sql::ResultSet *result, Bit_Buffer &buffer);
	int load_data_struct(DB_Struct *db_struct, const Field_Info &field_info, sql::ResultSet *result, Bit_Buffer &buffer);

	//根据byte_buffer生成bit_buffer
	int build_bit_buffer_arg(DB_Struct *db_struct, const Field_Info &field_info, Bit_Buffer &bit_buffer, Byte_Buffer &byte_buffer);
	int build_bit_buffer_vector(DB_Struct *db_struct, const Field_Info &field_info, Bit_Buffer &bit_buffer, Byte_Buffer &byte_buffer);
	int build_bit_buffer_map(DB_Struct *db_struct, const Field_Info &field_info, Bit_Buffer &bit_buffer, Byte_Buffer &byte_buffer);
	int build_bit_buffer_struct(DB_Struct *db_struct, const Field_Info &field_info, Bit_Buffer &bit_buffer, Byte_Buffer &byte_buffer);

	//根据bit_buffer生成byte_buffer
	int build_byte_buffer_arg(DB_Struct *db_struct, const Field_Info &field_info, Bit_Buffer &bit_buffer, Byte_Buffer &byte_buffer);
	int build_byte_buffer_vector(DB_Struct *db_struct, const Field_Info &field_info, Bit_Buffer &bit_buffer, Byte_Buffer &byte_buffer);
	int build_byte_buffer_map(DB_Struct *db_struct, const Field_Info &field_info, Bit_Buffer &bit_buffer, Byte_Buffer &byte_buffer);
	int build_byte_buffer_struct(DB_Struct *db_struct, const Field_Info &field_info, Bit_Buffer &bit_buffer, Byte_Buffer &byte_buffer);

private:
	Connection_Map connection_map_;
};

#endif /* MYSQL_OPERATOR_H_ */
