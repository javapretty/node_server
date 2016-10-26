/*
*	描述：center_server脚本
*	作者：张亚磊
*	时间：2016/09/22
*/

require('enum.js');
require('message.js');
require('struct.js');
require('config.js');
require('util.js');
require('timer.js');

//account--Token_Info
var account_token_map = new Map();
//game_node--sid_set
var game_sid_map = new Map();
//gate列表
var gate_list = new Array();
//game列表
var game_list = new Array();
//配置管理器
var config = new Config();
//定时器
var timer = new Timer();

function init(node_info) {
	log_info('center_server init, node_type:',node_info.node_type,' node_id:',node_info.node_id,' node_name:',node_info.node_name);
	config.init();
	timer.init(Node_Type.CENTER_SERVER);
}

function on_drop(cid) { }

function on_msg(msg) {
	log_debug('center_server on_msg, cid:',msg.cid,' msg_type:',msg.msg_type,' msg_id:',msg.msg_id,' sid:', msg.sid);
	
	switch(msg.msg_id) {
	case Msg.REQ_SELECT_GATE:
		select_gate(msg);
		break;	
	case Msg.NODE_CENTER_NODE_INFO:
		set_node_info(msg);
		break;		
	case Msg.NODE_GATE_CENTER_VERIFY_TOKEN:
		verify_token(msg);
		break;
	case Msg.NODE_GAME_CENTER_LOGIN_LOGOUT:
		game_login_logout(msg);
		break;
	default:
		log_error('center_server on_msg, msg_id not exist:', msg.msg_id);
		break;
	}
}

function on_tick(timer_id) {
	var timer_handler = timer.get_timer_handler(timer_id);
	if (timer_handler != null) {
		timer_handler();
	}
}

function remove_session(account, cid, error_code) {
	account_token_map.delete(account);
	var msg = new s2c_4();
	msg.error_code = error_code;
	send_msg(Endpoint.CENTER_CLIENT_SERVER, cid, Msg.RES_ERROR_CODE, Msg_Type.S2C, 0, msg);
	//关闭网络层链接
	close_session(Endpoint.GATE_CLIENT_SERVER, cid);	
}

function select_gate(msg) {
	log_info('select gate, account:', msg.account);
	if (account_token_map.get(msg.account)) {
		log_error('account in center_server:', msg.account);
		return remove_session(msg.account, msg.cid, Error_Code.DISCONNECT_RELOGIN);
	}

	var index = hash(msg.account) % (gate_list.length);
	var gate_info = gate_list[index];
	var token_info = new Token_Info();
	token_info.cid = msg.cid;
	token_info.token = generate_token(msg.account);
	token_info.token_time = util.now_sec;
	account_token_map.set(msg.account, token_info);
	
	var msg_res = new s2c_1();
	msg_res.gate_ip = gate_info.server_list[0].server_ip;
	msg_res.gate_port = gate_info.server_list[0].server_port;
	msg_res.token = token_info.token;
	send_msg(Endpoint.CENTER_CLIENT_SERVER, msg.cid, Msg.RES_SELECT_GATE, Msg_Type.S2C, 0, msg_res);
}

function set_node_info(msg) {
	if (msg.node_info.node_type == Node_Type.GATE_SERVER) {
		gate_list.push(msg.node_info);
	}
	else if (msg.node_info.node_type == Node_Type.GAME_SERVER) {
		game_list.push(msg.node_info);
	}
}

function verify_token(msg) {
	var token_info = account_token_map.get(msg.account);
	if (!token_info || token_info.token != msg.token) {		
		log_error('verify_token, token error, account:', msg.account, ' token:', msg.token);
		var msg_res = new s2c_4();
		msg_res.error_code = Error_Code.TOKEN_NOT_EXIST;
		return send_msg(Endpoint.CENTER_SERVER, msg.cid, Msg.RES_ERROR_CODE, Msg_Type.NODE_S2C, msg.sid, msg_res);
	}

	var index = hash(msg.account) % (game_list.length);
	var game_info = game_list[index];
	var msg_res = new node_2();
	msg_res.account = msg.account;
	msg_res.game_node = game_info.node_id;
	send_msg(Endpoint.CENTER_SERVER, msg.cid, Msg.NODE_GATE_CENTER_VERIFY_TOKEN, Msg_Type.NODE_MSG, msg.sid, msg_res);
}

function game_login_logout(msg) {
	var sid_set = game_sid_map.get(msg.game_node);
	if (sid_set) {
		if (msg.login) {
			sid_set.add(msg.sid);
		} else {
			sid_set.delete(msg.sid);
		}		
	} 
	else {
		if (msg.login) {
			sid_set = new Set();
			sid_set.add(msg.sid);
			game_sid_map.set(msg.game_node, sid_set);
		}
	}
}
