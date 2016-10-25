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
		close_session(Endpoint.GATE_CLIENT_SERVER, cid, Error_Code.DISCONNECT_SELF);
		return -1;
	}
	return sid_idx;
}

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
	send_msg(Endpoint.GATE_CENTER_CONNECTOR, 0, Msg.NODE_CENTER_NODE_INFO, Msg_Type.NODE_MSG, 0, msg);	
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

function on_drop(drop_cid) {
	var session = cid_session_map.get(drop_cid);
	if (session) {
		sid_set.delete(session.sid);
		var msg_3 = new node_3();
		send_msg(session.game_endpoint, 0, Msg.NODE_GATE_GAME_PLAYER_LOGOUT, Msg_Type.NODE_MSG, session.sid, msg_3);
		
		var msg_4 = new node_4();
		msg_4.login = false;
		send_msg(Endpoint.GATE_PUBLIC_CONNECTOR, 0, Msg.NODE_GATE_PUBLIC_PLAYER_LOGIN_LOGOUT, Msg_Type.NODE_MSG, session.sid, msg_4);
	}
	cid_session_map.delete(session.cid);
	sid_session_map.delete(session.sid);
	account_session_map.delete(session.account);
}

function process_gate_client_msg(msg) {
	switch(msg.msg_id) {
	case Msg.REQ_CONNECT_GATE:
		connect_gate(msg);
		break;		
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
	
	//登录game成功，通知public
	if (msg.msg_id == Msg.RES_ROLE_INFO) {
		var msg_res = new node_4();
		msg_res.login = true;
		msg_res.role_id = msg.role_info.role_id;
		send_msg(Endpoint.GATE_PUBLIC_CONNECTOR, 0, Msg.NODE_GATE_PUBLIC_PLAYER_LOGIN_LOGOUT, Msg_Type.NODE_MSG, msg.sid, msg_res);
	}
	
	if (msg.msg_id == Msg.RES_ERROR_CODE && msg.error_code == Error_Code.PLAYER_KICK_OFF) {
		print("kickoff player sid:", msg.sid);
		var session = sid_session_map.get(msg.sid);
		cid_session_map.delete(session.cid);
		sid_session_map.delete(session.sid);
		account_session_map.delete(session.account);
		close_session(Endpoint.GATE_CLIENT_SERVER, session.cid, Error_Code.PLAYER_KICK_OFF);
	}
}

function process_gate_node_msg(msg) {
	switch(msg.msg_id) {
	case Msg.NODE_ERROR_CODE: {
		if (msg.error_code == Error_Code.TOKEN_NOT_EXIST) {
			send_msg(Endpoint.GATE_CLIENT_SERVER, msg.sid, Msg.RES_ERROR_CODE, Msg_Type.S2C, 0, msg);
		}
		else if (msg.error_code == Error_Code.PLAYER_KICK_OFF) {
			print("kickoff player sid:", msg.sid);
			var session = sid_session_map.get(msg.sid);
			cid_session_map.delete(session.cid);
			sid_session_map.delete(session.sid);
			account_session_map.delete(session.account);
			close_session(Endpoint.GATE_CLIENT_SERVER, session.cid, Error_Code.PLAYER_KICK_OFF);
		}
		break;
	}
	case Msg.NODE_GATE_CENTER_VERIFY_TOKEN:
		verify_token(msg);
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
		close_session(Endpoint.GATE_CLIENT_SERVER, msg.cid, Error_Code.DISCONNECT_RELOGIN);
		return;	
	}
	
	var msg_res = new node_2();
	msg_res.account = msg.account;
	msg_res.token = msg.token;
	send_msg(Endpoint.GATE_CENTER_CONNECTOR, 0, Msg.NODE_GATE_CENTER_VERIFY_TOKEN, Msg_Type.NODE_MSG, msg.cid, msg_res);
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
	send_msg(Endpoint.GATE_CLIENT_SERVER, msg.sid, Msg.RES_CONNECT_GATE, Msg_Type.S2C, 0, msg_res);
}

function transmit_msg(msg) {
	var session = cid_session_map.get(msg.cid);
	if (!session) {
		print('session not in gate server,cid:',msg.cid,' msg_id:',msg.msg_id);
		return close_session(Endpoint.GATE_CLIENT_SERVER, msg.cid, Error_Code.DISCONNECT_NOLOGIN);
	}
	
	if (msg.msg_id == Msg.REQ_HEARTBEAT) {
		session.on_heartbeat(msg);
	} else if (msg.msg_id > Msg.REQ_HEARTBEAT && msg.msg_id < Msg.REQ_FETCH_RANK) {
		//客户端消息转到game
		send_msg(session.game_endpoint, 0, msg.msg_id, Msg_Type.NODE_C2S, session.sid, msg);
	} else {
		//客户端消息转到public
		send_msg(Endpoint.GATE_PUBLIC_CONNECTOR, 0, msg.msg_id, Msg_Type.NODE_C2S, session.sid, msg);
	}
}
