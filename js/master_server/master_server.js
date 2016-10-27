/*
*	描述：master_server脚本
*	作者：张亚磊
*	时间：2016/10/27
*/

require('enum.js');
require('message.js');
require('struct.js');
require('config.js');
require('util.js');
require('timer.js');

//配置管理器
var config = new Config();
//定时器
var timer = new Timer();

function init(node_info) {
	log_info('master_server init, node_type:',node_info.node_type,' node_id:',node_info.node_id,' node_name:',node_info.node_name);
	config.init();
	timer.init(Node_Type.MASTER_SERVER);
}

function on_drop(cid) { }

function on_msg(msg) {
	log_debug('master_server on_msg, cid:',msg.cid,' msg_type:',msg.msg_type,' msg_id:',msg.msg_id,' sid:', msg.sid);
	
	switch(msg.msg_id) {
	case Msg.SYNC_NODE_INFO:
		send_msg(Endpoint.MASTER_CENTER_CONNECTOR, 0, msg.msg_id, msg.msg_type, msg.sid, msg);
		break;		
	default:
		log_error('master_server on_msg, msg_id not exist:', msg.msg_id);
		break;
	}
}

function on_tick(timer_id) {}