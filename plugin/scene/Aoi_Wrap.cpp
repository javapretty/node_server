/*
 * Aoi_Wrap.cpp
 *
 *  Created on: Sep 3, 2016
 *      Author: lijunliang
 */

#include "Aoi_Wrap.h"
#include "Aoi_Manager.h"
#include "Struct_Manager.h"
#include "Node_Manager.h"
#include "V8_Base.h"

extern Global<ObjectTemplate> _g_aoi_entity_template;

Local<Object> wrap_aoi_entity(Isolate* isolate, Aoi_Entity *entity) {
	EscapableHandleScope handle_scope(isolate);

	Local<ObjectTemplate> localTemplate = Local<ObjectTemplate>::New(isolate, _g_aoi_entity_template);
	localTemplate->SetInternalFieldCount(1);
	Local<External> entity_ptr = External::New(isolate, entity);
	//将指针存在V8对象内部
	Local<Object> entity_obj = localTemplate->NewInstance(isolate->GetCurrentContext()).ToLocalChecked();
	entity_obj->SetInternalField(0, entity_ptr);

	return handle_scope.Escape(entity_obj);
}

Aoi_Entity *unwrap_aoi_entity(Local<Object> obj) {
	Local<External> field = Local<External>::Cast(obj->GetInternalField(0));
	void* ptr = field->Value();
	return static_cast<Aoi_Entity*>(ptr);
}

void create_aoi_manager(const FunctionCallbackInfo<Value>& args) {
	int mgr_id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
	bool ret = Aoi_Manager::create_aoi_manager(mgr_id);
	if(!ret)
		LOG_ERROR("create_scene %d failre", mgr_id);
	args.GetReturnValue().Set(ret);
}

void create_aoi_entity(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 3) {
		LOG_ERROR("create_scene args error, length: %d, needs 3\n", args.Length());
		return;
	}
	int sid = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
	int eid = args[1]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
	int radius = args[2]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);

	Aoi_Entity *entity = Aoi_Entity::create_aoi_entity(sid, eid, radius);
	args.GetReturnValue().Set(wrap_aoi_entity(args.GetIsolate(), entity));
}

void enter_aoi(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 3) {
		LOG_ERROR("enter_scene args error, length: %d\n", args.Length());
		return;
	}
	Isolate *isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	Aoi_Entity *entity = unwrap_aoi_entity(args.Holder());
	if(!entity) {
		return;
	}
	int mgr_id = args[0]->Int32Value(context).FromMaybe(0);
	int x = args[1]->Int32Value(context).FromMaybe(0);
	int y = args[2]->Int32Value(context).FromMaybe(0);
	entity->pos().x = x;
	entity->pos().y = y;
	Aoi_Manager *manager = Aoi_Manager::get_aoi_manager(mgr_id);
	if(manager == nullptr) {
		LOG_ERROR("aoi manager %d not exist!", mgr_id);
		args.GetReturnValue().SetNull();
		return;
	}
	manager->on_enter_aoi(entity);
	Aoi_Manager::add_entity(entity);

	AOI_MAP aoi_map = entity->aoi_map();	
	EscapableHandleScope handle_scope(isolate);
	Local<Array> aoi_list_obj = Array::New(isolate, aoi_map.size());
	Local<Value> value;
	int index = 0;
	for(AOI_MAP::iterator iter = aoi_map.begin(); iter != aoi_map.end(); iter++) {
		if(iter->second->sid() == 0)
			continue;
		value = Number::New(isolate, iter->second->sid());
		aoi_list_obj->Set(context, index, value).FromJust();
		index++;
	}

	args.GetReturnValue().Set(handle_scope.Escape(aoi_list_obj));
}

void leave_aoi(const FunctionCallbackInfo<Value>& args) {
	Aoi_Entity *entity = unwrap_aoi_entity(args.Holder());
	if(!entity) {
		return;
	}
	Aoi_Manager *manager = entity->aoi_manager();
	if(manager != nullptr){
		manager->on_leave_aoi(entity);
		Aoi_Manager::rmv_entity(entity);
	}
}

void update_position(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 2) {
		LOG_ERROR("update_position args error, length: %d\n", args.Length());
		return;
	}

	Isolate *isolate = args.GetIsolate();
	Local<Context> context(isolate->GetCurrentContext());
	Aoi_Entity *entity = unwrap_aoi_entity(args.Holder());
	if(!entity) {
		return;
	}

	int x = args[0]->Int32Value(context).FromMaybe(0);
	int y = args[1]->Int32Value(context).FromMaybe(0);
//	int z = args[2]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
	LOG_INFO("aoi_entity update_position x:%d, y:%d", x, y);

	entity->opos() = entity->pos();
	entity->pos().x = x;
	entity->pos().y = y;

	Aoi_Manager *manager = entity->aoi_manager();
	if(manager != nullptr){
		manager->on_update_aoi(entity);
		AOI_MAP enter_map = entity->enter_map();
		AOI_MAP leave_map = entity->leave_map();
		AOI_MAP aoi_map = entity->aoi_map();

		EscapableHandleScope handle_scope(isolate);
		
		Local<Array> enter_list_obj = Array::New(isolate, enter_map.size());
		Local<Value> value;
		int index = 0;
		for(AOI_MAP::iterator iter = enter_map.begin(); iter != enter_map.end(); iter++) {
			if(iter->second->sid() == 0)
				continue;
			value = Number::New(isolate, iter->second->sid());
			enter_list_obj->Set(context, index, value).FromJust();
			index++;
		}
		
		Local<Array> leave_list_obj = Array::New(isolate, leave_map.size());
		index = 0;
		for(AOI_MAP::iterator iter = leave_map.begin(); iter != leave_map.end(); iter++) {
			if(iter->second->sid() == 0)
				continue;
			value = Number::New(isolate, iter->second->sid());
			leave_list_obj->Set(context, index, value).FromJust();
			index++;
		}
		
		Local<Array> aoi_list_obj = Array::New(isolate, aoi_map.size());
		index = 0;
		for(AOI_MAP::iterator iter = aoi_map.begin(); iter != aoi_map.end(); iter++) {
			if(iter->second->sid() == 0)
				continue;
			value = Number::New(isolate, iter->second->sid());
			aoi_list_obj->Set(context, index, value).FromJust();
			index++;
		}
	
		Local<ObjectTemplate> localTemplate = ObjectTemplate::New(isolate);
		Local<Object> object = localTemplate->NewInstance(context).ToLocalChecked();	
		
		object->Set(context, String::NewFromUtf8(isolate, "enter_list", NewStringType::kNormal).ToLocalChecked(), enter_list_obj).FromJust();
		object->Set(context, String::NewFromUtf8(isolate, "leave_list", NewStringType::kNormal).ToLocalChecked(), leave_list_obj).FromJust();
		object->Set(context, String::NewFromUtf8(isolate, "aoi_list", NewStringType::kNormal).ToLocalChecked(), aoi_list_obj).FromJust();

		args.GetReturnValue().Set(handle_scope.Escape(object));
		return;
	}
	
	args.GetReturnValue().SetNull();
}

void get_aoi_list(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 0) {
		LOG_ERROR("get_aoi_list args error, length: %d\n", args.Length());
		return;
	}
	Aoi_Entity *entity = unwrap_aoi_entity(args.Holder());
	if(!entity) {
		return;
	}
	AOI_MAP aoi_map = entity->aoi_map();
	Isolate *isolate = args.GetIsolate();
	EscapableHandleScope handle_scope(isolate);

	Local<Array> list_obj = Array::New(isolate, aoi_map.size());
	Local<Value> value;
	int index = 0;
	for(AOI_MAP::iterator iter = aoi_map.begin(); iter != aoi_map.end(); iter++) {
		if(iter->second->sid() == 0)
			continue;
		value = Number::New(isolate, iter->second->sid());
		list_obj->Set(isolate->GetCurrentContext(), index, value).FromJust();
		index++;
	}

	args.GetReturnValue().Set(handle_scope.Escape(list_obj));
}

void broadcast_msg_to_all_without_self(const FunctionCallbackInfo<Value>& args) {
	broadcast_msg_sub(args, false);
}

void broadcast_msg_to_all(const FunctionCallbackInfo<Value>& args) {
	broadcast_msg_sub(args, true);
}

void broadcast_msg_sub(const FunctionCallbackInfo<Value>& args, bool with_self) {
	if (args.Length() != 3) {
		LOG_ERROR("broadcast_msg_to_all args error, length: %d\n", args.Length());
		return;
	}
	Local<Object> self = args.Holder();
	Aoi_Entity* entity = unwrap_aoi_entity(self);
	Local<Object> list_obj = args[0]->ToObject(args.GetIsolate()->GetCurrentContext()).ToLocalChecked();
	if (!list_obj->IsArray()) {
		LOG_ERROR("broadcast_msg_to_all arg[0] is not array");
		return;
	}

	Msg_Head msg_head;
	msg_head.msg_id = args[1]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
	msg_head.cid = 0;
	msg_head.protocol = TCP;
	msg_head.msg_type = 4;//NODE_S2C
		
	Bit_Buffer buffer;
	std::string struct_name = get_struct_name(msg_head.msg_type, msg_head.msg_id);
	Msg_Struct *msg_struct = STRUCT_MANAGER->get_msg_struct(struct_name);
	if (msg_struct) {
		msg_struct->build_bit_buffer(args.GetIsolate(), msg_struct->field_vec(), buffer, 
		args[2]->ToObject(args.GetIsolate()->GetCurrentContext()).ToLocalChecked());
	}
	else {
		LOG_ERROR("broadcast to all build bit buffer error");
		return;
	}

	Local<Array> array = Local<Array>::Cast(list_obj);
	int16_t len = array->Length();
	int sid = 0;
	Local<Value> sid_obj;
	for (int i = 0; i < len; ++i) {
		sid_obj = array->Get(args.GetIsolate()->GetCurrentContext(), i).ToLocalChecked();
		sid = sid_obj->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
		if(sid == 0){
			continue;
		}
		Aoi_Entity *sub = Aoi_Manager::find_entity(sid);
		msg_head.sid = sub->sid();
		msg_head.eid = sub->eid();
		NODE_MANAGER->send_msg(msg_head, buffer.data(), buffer.get_byte_size());
	}
	if(with_self) {
		msg_head.sid = entity->sid();
		msg_head.eid = entity->eid();
		NODE_MANAGER->send_msg(msg_head, buffer.data(), buffer.get_byte_size());
	}
}

void send_msg_list(const FunctionCallbackInfo<Value>& args) {
	Isolate *isolate = args.GetIsolate();
	Local<Context> context(isolate->GetCurrentContext());

	Local<Object> self = args.Holder();
	Aoi_Entity* entity = unwrap_aoi_entity(self);
	
	Msg_Head msg_head;
	msg_head.msg_id = args[0]->Int32Value(context).FromMaybe(0);
	msg_head.cid = 0;
	msg_head.sid = entity->sid();
	msg_head.eid = entity->eid();
	msg_head.protocol = TCP;
	msg_head.msg_type = 4;//NODE_S2C

	Local<Object> msg_list = args[1]->ToObject(context).ToLocalChecked();
	if (!msg_list->IsArray()) {
		LOG_ERROR("send_msg_list arg[1] is not array");
		return;
	}
	Local<Array> array = Local<Array>::Cast(msg_list);
	int16_t len = array->Length();
	Local<Value> msg_value;
	Local<Object> msg_obj;
	for (int i = 0; i < len; ++i) {
		msg_value = array->Get(context, i).ToLocalChecked();
		msg_obj = msg_value->ToObject(context).ToLocalChecked();
	
		Bit_Buffer buffer;
		std::string struct_name = get_struct_name(msg_head.msg_type, msg_head.msg_id);
		Msg_Struct *msg_struct = STRUCT_MANAGER->get_msg_struct(struct_name);
		if (msg_struct) {
			msg_struct->build_bit_buffer(args.GetIsolate(), msg_struct->field_vec(), buffer, msg_obj);
		}
		NODE_MANAGER->send_msg(msg_head, buffer.data(), buffer.get_byte_size());
	}
}

void reclaim_aoi_entity(const FunctionCallbackInfo<Value>& args) {
	Local<Object> self = args.Holder();
	Aoi_Entity* entity = unwrap_aoi_entity(self);
	Aoi_Entity::reclaim_aoi_entity(entity);
}

