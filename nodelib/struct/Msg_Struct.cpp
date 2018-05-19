/*
 * Msg_Struct.cpp
 *
 *  Created on: Aug 2, 2016
 *      Author: zhangyalei
 */

#include "V8_Base.h"
#include "Struct_Manager.h"
#include "Msg_Struct.h"

Msg_Struct::Msg_Struct() : 
	Base_Struct(),
	msg_id_(0),
	msg_name_() {}

Msg_Struct::~Msg_Struct() {
	msg_id_ = 0;
	msg_name_.clear();
}

int Msg_Struct::init(Xml &xml, TiXmlNode *node) {
	if(node) {
		msg_id_ = xml.get_attr_int(node, "msg_id");
		msg_name_ = xml.get_attr_str(node, "msg_name");
	}

	//Base_Struct初始化放在最后面，因为Base_Struct初始化内部会遍历node节点
	return Base_Struct::init(xml, node);
}

v8::Local<v8::Object> Msg_Struct::build_json_msg_object(Isolate* isolate, const Msg_Head &msg_head, const Json::Value &value) {
	EscapableHandleScope handle_scope(isolate);

	Local<Context> context(isolate->GetCurrentContext());
	v8::Local<v8::Object> object = build_value_object(isolate, field_vec(), value);
	if(object.IsEmpty()) {
		return handle_scope.Escape(v8::Local<v8::Object>());
	}

	object->Set(context, String::NewFromUtf8(isolate, "cid", NewStringType::kNormal).ToLocalChecked(),
			Int32::New(isolate, msg_head.cid)).FromJust();
	object->Set(context, String::NewFromUtf8(isolate, "msg_id", NewStringType::kNormal).ToLocalChecked(),
			Int32::New(isolate, msg_head.msg_id)).FromJust();
	object->Set(context, String::NewFromUtf8(isolate, "msg_type", NewStringType::kNormal).ToLocalChecked(),
			Int32::New(isolate, msg_head.msg_type)).FromJust();
	return handle_scope.Escape(object);
}

v8::Local<v8::Object> Msg_Struct::build_buffer_msg_object(Isolate* isolate, const Msg_Head &msg_head, Bit_Buffer &buffer) {
	EscapableHandleScope handle_scope(isolate);

	Local<Context> context(isolate->GetCurrentContext());
	v8::Local<v8::Object> object = Object::New(isolate);
	build_bit_object(isolate, field_vec(), buffer, object);
	if(object.IsEmpty()) {
		return handle_scope.Escape(v8::Local<v8::Object>());
	}

	object->Set(context, String::NewFromUtf8(isolate, "cid", NewStringType::kNormal).ToLocalChecked(),
			Int32::New(isolate, msg_head.cid)).FromJust();
	object->Set(context, String::NewFromUtf8(isolate, "msg_id", NewStringType::kNormal).ToLocalChecked(),
			Int32::New(isolate, msg_head.msg_id)).FromJust();
	object->Set(context, String::NewFromUtf8(isolate, "msg_type", NewStringType::kNormal).ToLocalChecked(),
			Int32::New(isolate, msg_head.msg_type)).FromJust();
	object->Set(context, String::NewFromUtf8(isolate, "sid", NewStringType::kNormal).ToLocalChecked(),
			Uint32::New(isolate, msg_head.sid)).FromJust();
	return handle_scope.Escape(object);
}
