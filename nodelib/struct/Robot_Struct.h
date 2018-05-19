/*
 * Robot_Struct.h
 *
 *  Created on: Sep 27, 2016
 *      Author: zhangyalei
 */

#ifndef ROBOT_STRUCT_H_
#define ROBOT_STRUCT_H_

#include "Base_Struct.h"

class Robot_Struct: public Base_Struct {
public:
	Robot_Struct();
	virtual ~Robot_Struct();

	virtual int init(Xml &xml, TiXmlNode *node);

	inline const int msg_id() { return msg_id_; }
	inline const std::string &msg_name() { return msg_name_; }
	
	//读写bit_buffer
	void write_bit_buffer(const Field_Vec &field_vec, Bit_Buffer &buffer);
	void read_bit_buffer(const Field_Vec &field_vec, Bit_Buffer &buffer);

private:
	void write_bit_buffer_arg(const Field_Info &field_info, Bit_Buffer &buffer);
	void write_bit_buffer_vector(const Field_Info &field_info, Bit_Buffer &buffer);
	void write_bit_buffer_map(const Field_Info &field_info, Bit_Buffer &buffer);
	void write_bit_buffer_struct(const Field_Info &field_info, Bit_Buffer &buffer);

	void read_bit_buffer_arg(const Field_Info &field_info, Bit_Buffer &buffer);
	void read_bit_buffer_vector(const Field_Info &field_info, Bit_Buffer &buffer);
	void read_bit_buffer_map(const Field_Info &field_info, Bit_Buffer &buffer);
	void read_bit_buffer_struct(const Field_Info &field_info, Bit_Buffer &buffer);

private:
	int msg_id_;
	std::string msg_name_;
};

#endif /* ROBOT_STRUCT_H_ */
