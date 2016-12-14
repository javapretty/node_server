/*
 * main.cpp
 *
 *  Created on: Dec 7, 2016
 *      Author: lijunliang
 */

#include "Node_Manager.h"
#include "Aoi_Wrap.h"

using namespace v8;

Global<ObjectTemplate> _g_aoi_entity_template;

extern "C" {
	void init(Local<ObjectTemplate> &global, Isolate *isolate) {
		global->Set(String::NewFromUtf8(isolate, "create_aoi_entity", NewStringType::kNormal).ToLocalChecked(),
			FunctionTemplate::New(isolate, create_aoi_entity));
		global->Set(String::NewFromUtf8(isolate, "create_aoi_manager", NewStringType::kNormal).ToLocalChecked(),
			FunctionTemplate::New(isolate, create_aoi_manager));
	
		Local<ObjectTemplate> aoi_entity_template = ObjectTemplate::New(isolate);
		aoi_entity_template->SetInternalFieldCount(1);

		aoi_entity_template->Set(String::NewFromUtf8(isolate, "enter_aoi", NewStringType::kNormal).ToLocalChecked(),
					                FunctionTemplate::New(isolate, enter_aoi));
		aoi_entity_template->Set(String::NewFromUtf8(isolate, "leave_aoi", NewStringType::kNormal).ToLocalChecked(),
						            FunctionTemplate::New(isolate, leave_aoi));
		aoi_entity_template->Set(String::NewFromUtf8(isolate, "update_position", NewStringType::kNormal).ToLocalChecked(),
					                FunctionTemplate::New(isolate, update_position));
		aoi_entity_template->Set(String::NewFromUtf8(isolate, "get_aoi_list", NewStringType::kNormal).ToLocalChecked(),
						            FunctionTemplate::New(isolate, get_aoi_list));
		aoi_entity_template->Set(String::NewFromUtf8(isolate, "send_msg_list", NewStringType::kNormal).ToLocalChecked(),
							        FunctionTemplate::New(isolate, send_msg_list));
		aoi_entity_template->Set(String::NewFromUtf8(isolate, "broadcast_msg_to_all", NewStringType::kNormal).ToLocalChecked(),
							        FunctionTemplate::New(isolate, broadcast_msg_to_all));
		aoi_entity_template->Set(String::NewFromUtf8(isolate, "broadcast_msg_to_all_without_self", NewStringType::kNormal).ToLocalChecked(),
							        FunctionTemplate::New(isolate, broadcast_msg_to_all_without_self));
		aoi_entity_template->Set(String::NewFromUtf8(isolate, "reclaim", NewStringType::kNormal).ToLocalChecked(),
							        FunctionTemplate::New(isolate, reclaim_aoi_entity));
		_g_aoi_entity_template.Reset(isolate, aoi_entity_template);
	}
}
