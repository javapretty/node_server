#! /bin/bash

DEFINE_PATH='../config/struct/'
JS_TARGET='../js'

function gen_msg(){
	gen_js
	gen_msgid
	cp_file
	rm_file
}

function gen_js(){
	./struct_tool js $DEFINE_PATH 'client_msg.xml' 'struct'
	./struct_tool js $DEFINE_PATH 'node_msg.xml' 'struct'
	./struct_tool js $DEFINE_PATH 'node_struct.xml' 'struct'
	./struct_tool js $DEFINE_PATH 'public_struct.xml' 'struct'
}

function gen_msgid(){
	./struct_tool msg $DEFINE_PATH 'message.define' 'msg'
}

function cp_file(){
	wildcard='.*'
	cp -rf JS/* $JS_TARGET
	cp -rf '../config/struct/client_msg.xml' '../../node_robot/config'
	cp -rf '../config/struct/public_struct.xml' '../../node_robot/config'
}

function rm_file(){
	rm -rf JS
}

gen_msg
