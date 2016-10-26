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
	print('gate_server init, node_type:',node_info.node_type,' node_id:',node_info.node_id,' node_name:',node_info.node_name);
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
	send_msg_to_center(Msg.NODE_CENTER_NODE_INFO, 0, msg);	
}

function on_drop(cid) {
	var session = cid_session_map.get(cid);
	if (session) {
		var msg = new node_6();
		send_msg_to_game(session.game_endpoint, Msg.NODE_GATE_PUBLIC_LOGIN_GAME_LOGOUT, session.sid, msg);
		
		sid_set.delete(session.sid);
		cid_session_map.delete(session.cid);
		sid_session_map.delete(session.sid);
		account_session_map.delete(session.account);
	}
}

function on_msg(msg) {
	print('gate_server on_msg, cid:',msg.cid,' msg_type:',msg.msg_type,' msg_id:',msg.msg_id,' sid:', msg.sid);
	
	if (msg.msg_type == Msg_Type.C2S) {
		process_gate_client_msg(msg);
	} else if (msg.msg_type == Msg_Type.NODE_S2C) {
		process_gate_s2c_msg(msg);
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
		print('get_sid error, cal count:', count, ' cid:', cid); 
		remove_session(cid, Error_Code.DISCONNECT_SELF);
		return -1;
	}
	return sid_idx;
}

function send_msg_to_client(cid, msg_id, msg) {
	send_msg(Endpoint.GATE_CLIENT_SERVER, cid, msg_id, Msg_Type.S2C, 0, msg);
}

function send_msg_to_game(endpoint_id, msg_id, sid, msg) {
	send_msg(endpoint_id, 0, msg_id, Msg_Type.NODE_MSG, sid, msg);
}

function send_msg_to_public(msg_id, sid, msg) {
	send_msg(Endpoint.GATE_PUBLIC_CONNECTOR, 0, msg_id, Msg_Type.NODE_MSG, sid, msg);
}

function send_msg_to_center(msg_id, sid, msg) {
	send_msg(Endpoint.GATE_CENTER_CONNECTOR, 0, msg_id, Msg_Type.NODE_MSG, sid, msg);
}

function remove_session(cid, error_code) {
	var msg = new s2c_4();
	msg.error_code = error_code;
	send_msg(Endpoint.GATE_CLIENT_SERVER, cid, Msg.RES_ERROR_CODE, Msg_Type.S2C, 0, msg);
	//关闭网络层链接
	close_session(Endpoint.GATE_CLIENT_SERVER, cid, error_code);	
}

function process_gate_client_msg(msg) {
	switch(msg.msg_id) {
	case Msg.REQ_CONNECT_GATE:
		connect_gate(msg);
		break;
	case Msg.REQ_HEARTBEAT: {
		var session = cid_session_map.get(msg.cid);
		if (!session) {
			remove_session(msg.cid, Error_Code.DISCONNECT_NOLOGIN);
		}
		session.on_heartbeat(msg);
		break;
	}		
	default:
		transmit_msg(msg);
		break;
	}
}

function process_gate_s2c_msg(msg) {
	var session = sid_session_map.get(msg.sid);
	if (!session) {
		return print('session not in gate server,sid:',msg.sid,' msg_id:',msg.msg_id);
	}

	//消息转到client
	send_msg(Endpoint.GATE_CLIENT_SERVER, session.cid, msg.msg_id, Msg_Type.S2C, 0, msg);
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
		print('process_gate_node_msg, msg_id not exist:', msg.msg_id);
		break;
	}
}

function connect_gate(msg) {
	print('connect_gate, account:', msg.account, ' token:', msg.token);
	if (account_session_map.get(msg.account)) {
		print('account in gate server, ', msg.account);
		return remove_session(msg.cid, Error_Code.DISCONNECT_RELOGIN);	
	}
	
	var msg_res = new node_2();
	msg_res.account = msg.account;
	msg_res.token = msg.token;
	send_msg_to_center(Msg.NODE_GATE_CENTER_VERIFY_TOKEN, msg.cid, msg_res);
}

function verify_token(msg) {
	var session = new Session();
	session.game_endpoint = game_node_endpoint_map.get(msg.game_node);
	session.cid = msg.sid;
	session.sid = get_sid(msg.sid);
	session.account = msg.account;
	sid_set.add(session.sid);
	cid_session_map.set(session.cid, session);
	sid_session_map.set(session.sid, session);
	account_session_map.set(session.account, session);

	var msg_res = new s2c_2();
	msg_res.account = msg.account;
	send_msg_to_client(msg.sid, Msg.RES_CONNECT_GATE, msg_res);
}

function game_login_logout(msg) {
	if (msg.login) {
		var msg_res = new node_6();
		send_msg_to_public(Msg.NODE_GATE_PUBLIC_LOGIN_GAME_LOGOUT, msg.sid, msg_res);
	}
	else {
		//玩家下线，清除session信息
		cid_session_map.delete(session.cid);
		sid_session_map.delete(session.sid);
		account_session_map.delete(session.account);
		remove_session(session.cid, Error_Code.PLAYER_KICK_OFF);
	}
}

function transmit_msg(msg) {
	var session = cid_session_map.get(msg.cid);
	if (!session) {
		print('session not in gate server,cid:',msg.cid,' msg_id:',msg.msg_id);
		return remove_session(msg.cid, Error_Code.DISCONNECT_NOLOGIN);
	}
	
	if (msg.msg_id > Msg.REQ_HEARTBEAT && msg.msg_id < Msg.REQ_FETCH_RANK) {
		//客户端消息转到game
		send_msg(session.game_endpoint, 0, msg.msg_id, Msg_Type.NODE_C2S, session.sid, msg);
	} else {
		//客户端消息转到public
		send_msg(Endpoint.GATE_PUBLIC_CONNECTOR, 0, msg.msg_id, Msg_Type.NODE_C2S, session.sid, msg);
	}
}
