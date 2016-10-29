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
	
	if (msg.msg_type == Msg_Type.NODE_MSG) {
		process_master_node_msg(msg);
	} else if (msg.msg_type == Msg_Type.HTTP_MSG) {
		process_master_http_msg(msg);
	}

}

function on_tick(timer_id) {}

function process_master_node_msg(msg) {
	switch(msg.msg_id) {
	case Msg.SYNC_NODE_INFO:
		send_msg(Endpoint.MASTER_CENTER_CONNECTOR, 0, msg.msg_id, msg.msg_type, msg.sid, msg);
		break;		
	default:
		log_error('process_master_node_msg, msg_id not exist:', msg.msg_id);
		break;
	}
}

//可以使用curl命令，向服务器发送post消息，格式如下
//curl -d "{\"msg_id\":1,\"node_info\":{\"node_type\":6,\"node_id\":60003,\"node_name\":\"game_server3\",\"node_ip\":\"127.0.0.1\",\"script_path\":\"js/game_server/game_server.js\",\"plugin_list\":[],\"endpoint_list\":[{\"endpoint_type\":2,\"endpoint_id\":1,\"endpoint_name\":\"game3_db_connector\",\"server_ip\":\"127.0.0.1\",\"server_port\":8200,\"protocol_type\":0,\"receive_timeout\":0}]}}" "http://127.0.0.1:8080" 
function process_master_http_msg(msg) {
	switch(msg.msg_id) {
	case Msg.HTTP_CREATE_NODE_PROCESS:
		log_info('create node process, node_type:',msg.node_info.node_type,' node_id:',msg.node_info.node_id,' node_name:',msg.node_info.node_name);
		fork_process(msg.node_info);
		break;		
	default:
		log_error('process_master_http_msg, msg_id not exist:', msg.msg_id);
		break;
	}
}