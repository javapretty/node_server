/*
 * Gate_Wrap.cpp
 *
 *  Created on: Nov 8, 2016
 *      Author: zhangyalei
 */

#include "Gate_Manager.h"
#include "Gate_Wrap.h"

void add_session(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 1 || !args[0]->IsObject()) {
		LOG_ERROR("add_session args error, length: %d\n", args.Length());
		return;
	}

	Session *session = GATE_MANAGER->pop_session();
	if (!session) {
		LOG_ERROR("pop session error");
		return;
	}

	HandleScope handle_scope(args.GetIsolate());
	Local<Context> context(args.GetIsolate()->GetCurrentContext());
	Local<Object> object = args[0]->ToObject(context).ToLocalChecked();
	session->client_eid = (object->Get(context,
			String::NewFromUtf8(args.GetIsolate(), "client_eid", NewStringType::kNormal).
			ToLocalChecked()).ToLocalChecked())->Int32Value(context).FromJust();
	session->client_cid = (object->Get(context,
			String::NewFromUtf8(args.GetIsolate(), "client_cid", NewStringType::kNormal).
			ToLocalChecked()).ToLocalChecked())->Int32Value(context).FromJust();
	session->game_eid = (object->Get(context,
		String::NewFromUtf8(args.GetIsolate(), "game_eid", NewStringType::kNormal).
		ToLocalChecked()).ToLocalChecked())->Int32Value(context).FromJust();
	session->game_cid = (object->Get(context,
			String::NewFromUtf8(args.GetIsolate(), "game_cid", NewStringType::kNormal).
			ToLocalChecked()).ToLocalChecked())->Int32Value(context).FromJust();
	session->sid = (object->Get(context,
			String::NewFromUtf8(args.GetIsolate(), "sid", NewStringType::kNormal).
			ToLocalChecked()).ToLocalChecked())->Uint32Value(context).FromJust();

	GATE_MANAGER->add_session(session);
}

void remove_session(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 1) {
		LOG_ERROR("remove_session args error, length: %d\n", args.Length());
		return;
	}

	HandleScope handle_scope(args.GetIsolate());
	Local<Context> context(args.GetIsolate()->GetCurrentContext());
	int cid = args[0]->Int32Value(context).FromMaybe(0);
	GATE_MANAGER->remove_session(cid);
}
