/*
*	描述：gate_server脚本
*	作者：张亚磊
*	时间：2016/09/22
*/

require('enum.js');
require('message.js');
require('struct.js');
require('config.js');
require('util.js');
require('timer.js');
require('gate_server/session.js');

//sid idx
var sid_idx = 0;
var max_idx = 0;
//sid set
var sid_set = new Set();
//game_node--game_endpoint
var game_node_endpoint_map = new Map();
//cid--session
var cid_session_map = new Map();
//sid--session
var sid_session_map = new Map();
//account--session
var account_session_map = new Map();
//配置管理器
var config = new Config();
//定时器
var timer = new Timer();

function init(node_info) {
	log_info('gate_server init, node_type:',node_info.node_type,' node_id:',node_info.node_id,' node_name:',node_info.node_name);
	config.init();
	timer.init(Node_Type.GATE_SERVER);

	gate_node_info = node_info;
	sid_idx = node_info.node_id % 10000 * 1000000;
	max_idx = sid_idx + 1000000;
	//初始化game node_id对应的endpoint map
	game_node_endpoint_map.set(Node_Id.GAME_SERVER1, Endpoint.GATE_GAME1_CONNECTOR);
	game_node_endpoint_map.set(Node_Id.GAME_SERVER2, Endpoint.GATE_GAME2_CONNECTOR);

	var msg = new node_0();
	msg.node_info = node_info;
	send_msg(Endpoint.GATE_CENTER_CONNECTOR, 0, Msg.NODE_CENTER_NODE_INFO, Msg_Type.NODE_MSG, 0, msg);
}

function on_drop(cid) {
	var session = cid_session_map.get(cid);
	if (session) {
		var msg = new node_6();
		send_msg(session.game_endpoint, 0, Msg.NODE_GATE_PUBLIC_LOGIN_GAME_LOGOUT, Msg_Type.NODE_MSG, session.sid, msg);
		
		sid_set.delete(session.sid);
		cid_session_map.delete(session.cid);
		sid_session_map.delete(session.sid);
		account_session_map.delete(session.account);
	}
}

function on_msg(msg) {
	log_debug('gate_server on_msg, cid:',msg.cid,' msg_type:',msg.msg_type,' msg_id:',msg.msg_id,' sid:', msg.sid);
	
	if (msg.msg_type == Msg_Type.C2S) {
		process_gate_client_msg(msg);
	} else if (msg.msg_type == Msg_Type.NODE_MSG) {
		process_gate_node_msg(msg);
	}
}

function on_tick(timer_id) {}

function get_sid(cid) {
	var count = 0;
	do {
		count++;
		sid_idx++;
		if (sid_idx >= max_idx) {
			sid_idx = max_idx - 1000000;
		}
	} while (sid_set.has(sid_idx) && count < 10000)

	if (count >= 10000) {
		log_error('get_sid error, cal count:', count, ' cid:', cid); 
		remove_session(cid, Error_Code.DISCONNECT_SELF);
		return -1;
	}
	return sid_idx;
}

function remove_session(cid, error_code) {
	var msg = new s2c_4();
	msg.error_code = error_code;
	send_msg(Endpoint.GATE_CLIENT_SERVER, cid, Msg.RES_ERROR_CODE, Msg_Type.S2C, 0, msg);
	//关闭网络层链接
	close_session(Endpoint.GATE_CLIENT_SERVER, cid);	
}

function process_gate_client_msg(msg) {
	switch(msg.msg_id) {
	case Msg.REQ_HEARTBEAT: {
		var session = cid_session_map.get(msg.cid);
		if (!session) {
			remove_session(msg.cid, Error_Code.DISCONNECT_NOLOGIN);
		}
		session.on_heartbeat(msg);
		break;
	}
	case Msg.REQ_CONNECT_GATE:
		connect_gate(msg);
		break;	
	default:
		log_error('process_gate_client_msg, msg_id not exist:', msg.msg_id);
		break;
	}
}

function process_gate_node_msg(msg) {
	switch(msg.msg_id) {
	case Msg.NODE_GATE_CENTER_VERIFY_TOKEN:
		verify_token(msg);
		break;
	case Msg.NODE_GAME_GATE_LOGIN_LOGOUT:
		game_login_logout(msg);
		break;
	default:
		log_error('process_gate_node_msg, msg_id not exist:', msg.msg_id);
		break;
	}
}

function connect_gate(msg) {
	log_debug('connect_gate, account:', msg.account, ' token:', msg.token);
	if (account_session_map.get(msg.account)) {
		log_error('account in gate server, ', msg.account);
		return remove_session(msg.cid, Error_Code.DISCONNECT_RELOGIN);	
	}
	
	var msg_res = new node_2();
	msg_res.account = msg.account;
	msg_res.token = msg.token;
	send_msg(Endpoint.GATE_CENTER_CONNECTOR, 0, Msg.NODE_GATE_CENTER_VERIFY_TOKEN, Msg_Type.NODE_MSG, msg.cid, msg_res);
}

function verify_token(msg) {
	var session = new Session();
	session.gate_endpoint = Endpoint.GATE_CLIENT_SERVER;
	session.game_endpoint = game_node_endpoint_map.get(msg.game_node);
	session.public_endpoint = Endpoint.GATE_PUBLIC_CONNECTOR;
	session.cid = msg.sid;
	session.sid = get_sid(msg.sid);
	session.account = msg.account;
	sid_set.add(session.sid);
	cid_session_map.set(session.cid, session);
	sid_session_map.set(session.sid, session);
	account_session_map.set(session.account, session);
	//将session添加到C++层
	add_session(session);

	var msg_res = new s2c_2();
	msg_res.account = msg.account;
	send_msg(Endpoint.GATE_CLIENT_SERVER, msg.sid, Msg.RES_CONNECT_GATE, Msg_Type.S2C, 0, msg_res);
}

function game_login_logout(msg) {
	if (msg.login) {
		var msg_res = new node_6();
		send_msg(Endpoint.GATE_PUBLIC_CONNECTOR, 0, Msg.NODE_GATE_PUBLIC_LOGIN_GAME_LOGOUT, Msg_Type.NODE_MSG, msg.sid, msg_res);
	}
	else {
		//玩家下线，清除session信息
		var session = sid_session_map.get(msg.sid);
		if (session) {
			cid_session_map.delete(session.cid);
			sid_session_map.delete(session.sid);
			account_session_map.delete(session.account);
			remove_session(session.cid, Error_Code.PLAYER_KICK_OFF);	
		}
	}
}