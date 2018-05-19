/*
 * Msg_Struct.h
 *
 *  Created on: Aug 2, 2016
 *      Author: zhangyalei
 */

#ifndef MSG_STRUCT_H_
#define MSG_STRUCT_H_

#include "Base_Struct.h"

class Msg_Struct: public Base_Struct {
public:
	Msg_Struct();
	virtual ~Msg_Struct();

	virtual int init(Xml &xml, TiXmlNode *node);

	inline const int msg_id() { return msg_id_; }
	inline const std::string &msg_name() { return msg_name_; }
	
	//将Json::Value转成v8::object
	v8::Local<v8::Object> build_json_msg_object(Isolate* isolate, const Msg_Head &msg_head, const Json::Value &value);
	//Bit_Buffer转成v8::object
	v8::Local<v8::Object> build_buffer_msg_object(Isolate* isolate, const Msg_Head &msg_head, Bit_Buffer &buffer);

private:
	int msg_id_;
	std::string msg_name_;
};

#endif /* MSG_STRUCT_H_ */
