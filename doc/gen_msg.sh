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
	./struct_tool js $DEFINE_PATH 'base_struct.xml' 'struct'
	./struct_tool js $DEFINE_PATH 'db_struct.xml' 'struct'
	./struct_tool js $DEFINE_PATH 'inner_struct.xml' 'struct'
	./struct_tool js $DEFINE_PATH 'outer_struct.xml' 'struct'
}

function gen_msgid(){
	./struct_tool msg $DEFINE_PATH 'message.define' 'msg'
}

function cp_file(){
	wildcard='.*'
	cp -rf JS/* $JS_TARGET
	cp -rf '../config/struct/db_struct.xml' '../../node_robot/config'
	cp -rf '../config/struct/base_struct.xml' '../../node_robot/config'
	cp -rf '../config/struct/outer_struct.xml' '../../node_robot/config'
}

function rm_file(){
	rm -rf JS
}

gen_msg
