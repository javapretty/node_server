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

//sid idx
var sid_idx = 0;
//sid set
var sid_set = new Set();
//node_id--node_info
var node_map = new Map();
//gate列表
var gate_list = new Array();
//game列表
var game_list = new Array();
//配置管理器
var config = new Config();
//定时器
var timer = new Timer();
//cid--account
var cid_account_map = new Map();
//account--Token_Info
var account_token_map = new Map();

function init(node_info) {
	log_info('center_server init, node_type:',node_info.node_type,' node_id:',node_info.node_id,' node_name:',node_info.node_name);
	config.init();
	timer.init(Node_Type.CENTER_SERVER);
}

function on_drop(cid) {
	var account = cid_account_map.get(cid);
	cid_account_map.delete(account);
	account_token_map.delete(account); 
}

function on_msg(msg) {
	log_debug('center_server on_msg, cid:',msg.cid,' msg_type:',msg.msg_type,' msg_id:',msg.msg_id,' sid:', msg.sid);
	
	if (msg.msg_type == Msg_Type.C2S) {
		switch(msg.msg_id) {
		case Msg.REQ_SELECT_GATE:
			select_gate(msg);
			break;	
		default:
			log_error('center_server process client msg, msg_id not exist:', msg.msg_id);
			break;
		}
	} 
	else if (msg.msg_type == Msg_Type.NODE_MSG) {
		switch(msg.msg_id) {
		case Msg.SYNC_NODE_INFO:
			set_node_info(msg);
			break;		
		case Msg.SYNC_GATE_CENTER_VERIFY_TOKEN:
			verify_token(msg);
			break;
		case Msg.SYNC_GATE_CENTER_REMOVE_SESSION:
			sid_set.delete(msg.sid);
			break;
		default:
			log_error('center_server process node msg, msg_id not exist:', msg.msg_id);
			break;
		}	
	}
}

function on_tick(timer_id) {
	var timer_handler = timer.get_timer_handler(timer_id);
	if (timer_handler != null) {
		timer_handler();
	}
}

function on_close_session(account, cid, error_code) {	
	cid_account_map.delete(cid);
	account_token_map.delete(account);
	var msg = new s2c_5();
	msg.error_code = error_code;
	send_msg(Endpoint.CENTER_CLIENT_SERVER, cid, Msg.RES_ERROR_CODE, Msg_Type.S2C, 0, msg);
	//关闭客户端网络层链接
	close_client(Endpoint.CENTER_CLIENT_SERVER, cid);	
}

function select_gate(msg) {
	if (account_token_map.get(msg.account)) {
		log_error('account in center_server:', msg.account);
		return on_close_session(msg.account, msg.cid, Error_Code.DISCONNECT_RELOGIN);
	}

	var index = hash(msg.account) % (gate_list.length);
	var gate_info = gate_list[index];
	var token_info = new Token_Info();
	token_info.cid = msg.cid;
	token_info.token = generate_token(msg.account);
	token_info.token_time = util.now_sec;
	cid_account_map.set(msg.cid, msg.account);
	account_token_map.set(msg.account, token_info);
	
	var msg_res = new s2c_2();
	msg_res.gate_ip = gate_info.endpoint_list[0].server_ip;
	msg_res.gate_port = gate_info.endpoint_list[0].server_port;
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
	node_map.set(msg.node_info.node_id, msg.node_info);
}

function verify_token(msg) {
	var token_info = account_token_map.get(msg.account);
	if (!token_info || token_info.token != msg.token) {		
		log_error('verify_token, token error, account:', msg.account, ' token:', msg.token);
		if (token_info) {
			on_close_session(msg.account, token_info.cid, Error_Code.TOKEN_ERROR);
		}
		var msg_res = new node_1();
		msg_res.node_code = Node_Code.VERIFY_TOKEN_FAIL;
		return send_msg(Endpoint.CENTER_NODE_SERVER, msg.cid, Msg.SYNC_NODE_CODE, Msg_Type.NODE_MSG, msg.sid, msg_res);
	}

	++sid_idx;
	if (sid_idx > 4294967295) {
		sid_idx = 0;
	}
	sid_set.add(sid_idx);
	var index = hash(msg.account) % (game_list.length);
	var game_info = game_list[index];
	var msg_res = new node_3();
	msg_res.account = msg.account;
	msg_res.client_cid = msg.client_cid;
	msg_res.game_nid = game_info.node_id;
	send_msg(Endpoint.CENTER_NODE_SERVER, msg.cid, Msg.SYNC_GATE_CENTER_VERIFY_TOKEN, Msg_Type.NODE_MSG, sid_idx, msg_res);
}
