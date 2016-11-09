/*
 * DB_Wrap.cpp
 *
 *  Created on: Nov 1, 2016
 *      Author: zhangyalei
 */

#include <sstream>
#include "Struct_Manager.h"
#include "Data_Manager.h"
#include "DB_Manager.h"
#include "DB_Wrap.h"

void init_db(const FunctionCallbackInfo<Value>& args) {
	if (args.Length() != 1) {
		LOG_ERROR("init_log args error, length: %d\n", args.Length());
		return;
	}

	DATA_MANAGER->init_db_operator();

	Local<Context> context(args.GetIsolate()->GetCurrentContext());
	Base_Struct *base_struct = STRUCT_MANAGER->get_base_struct("Node_Info");
	if (base_struct) {
		Bit_Buffer buffer;
		base_struct->build_bit_buffer(args.GetIsolate(), base_struct->field_vec(), buffer, args[0]->ToObject(context).ToLocalChecked());
		Node_Info node_info;
		node_info.deserialize(buffer);
		DB_MANAGER->init(node_info);
		DB_MANAGER->thr_create();
	}
}
