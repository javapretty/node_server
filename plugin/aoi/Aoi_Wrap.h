/*
 * Aoi_Wrap.h
 *
 *  Created on: Sep 3, 2016
 *      Author: lijunliang
 */

#ifndef AOI_WRAP_H_
#define AOI_WRAP_H_

#include "include/v8.h"

using namespace v8;

void create_aoi_manager(const FunctionCallbackInfo<Value>& args);
void create_aoi_entity(const FunctionCallbackInfo<Value>& args);

void enter_aoi(const FunctionCallbackInfo<Value>& args);
void update_position(const FunctionCallbackInfo<Value>& args);
void leave_aoi(const FunctionCallbackInfo<Value>& args);
void get_aoi_list(const FunctionCallbackInfo<Value>& args);
void send_msg_list(const FunctionCallbackInfo<Value>& args);
void broadcast_msg_to_all(const FunctionCallbackInfo<Value>& args);
void broadcast_msg_to_all_without_self(const FunctionCallbackInfo<Value>& args);
void reclaim_aoi_entity(const FunctionCallbackInfo<Value>& args);
void broadcast_msg_sub(const FunctionCallbackInfo<Value>& args, bool with_self);

#endif /* AOI_WRAP_H_ */
