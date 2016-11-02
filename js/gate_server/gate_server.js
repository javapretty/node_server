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

//gate node_info
var gate_node_info = null;
//game_node_id--game_cid
var game_nid_cid_map = new Map();
//client_cid--session
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

	var msg = new node_2();
	msg.node_info = node_info;
	send_msg(Endpoint.GATE_CENTER_CONNECTOR, 0, Msg.SYNC_NODE_INFO, Msg_Type.NODE_MSG, 0, msg);
}

function on_drop(cid) {
	var session = cid_session_map.get(cid);
	if (session) {
		var msg = new node_6();
		send_msg(session.game_eid, session.game_cid, Msg.SYNC_GAME_GATE_LOGOUT, Msg_Type.NODE_MSG, session.sid, msg);
		
		on_remove_session(session);
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

function on_add_session(session) {
	cid_session_map.set(session.client_cid, session);
	sid_session_map.set(session.sid, session);
	account_session_map.set(session.account, session);
	//将session添加到C++层
	add_session(session);

	//通知game
	var msg_3 = new node_4();
	msg_3.gate_nid = gate_node_info.node_id;
	send_msg(session.game_eid, session.game_cid, Msg.SYNC_GATE_GAME_ADD_SESSION, Msg_Type.NODE_MSG, session.sid, msg_3);
	
	//通知client
	var msg_res = new s2c_3();
	msg_res.account = session.account;
	send_msg(session.client_eid, session.client_cid, Msg.RES_CONNECT_GATE, Msg_Type.S2C, 0, msg_res);
}

function on_remove_session(session) {
	//通知center
	var msg = new node_7();
	send_msg(Endpoint.GATE_CENTER_CONNECTOR, 0, Msg.SYNC_GATE_CENTER_REMOVE_SESSION, Msg_Type.NODE_MSG, session.sid, msg);	
	
	cid_session_map.delete(session.client_cid);
	sid_session_map.delete(session.sid);
	account_session_map.delete(session.account);
}

function on_close_session(cid, error_code) {
	var msg = new s2c_5();
	msg.error_code = error_code;
	send_msg(Endpoint.GATE_CLIENT_SERVER, cid, Msg.RES_ERROR_CODE, Msg_Type.S2C, 0, msg);
	//关闭网络层链接
	close_session(Endpoint.GATE_CLIENT_SERVER, cid);	
}

function process_gate_client_msg(msg) {
	switch(msg.msg_id) {
	case Msg.REQ_HEARTBEAT: {
		var session = cid_session_map.get(msg.cid);
		if (session) {
			session.on_heartbeat(msg);
		} else {
			on_close_session(msg.cid, Error_Code.DISCONNECT_NOLOGIN);
		}
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
	case Msg.SYNC_NODE_INFO:
		set_node_info(msg);
		break;
	case Msg.SYNC_GATE_CENTER_VERIFY_TOKEN:
		verify_token(msg);
		break;
	case Msg.SYNC_GAME_GATE_LOGOUT:
		game_logout(msg);
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
		return on_close_session(msg.cid, Error_Code.DISCONNECT_RELOGIN);	
	}
	
	var msg_res = new node_3();
	msg_res.account = msg.account;
	msg_res.token = msg.token;
	msg_res.client_cid = msg.cid;
	send_msg(Endpoint.GATE_CENTER_CONNECTOR, 0, Msg.SYNC_GATE_CENTER_VERIFY_TOKEN, Msg_Type.NODE_MSG, 0, msg_res);
}

function set_node_info(msg) {
	game_nid_cid_map.set(msg.node_info.node_id, msg.cid);
}

function verify_token(msg) {
	var session = new Session();
	session.client_eid = Endpoint.GATE_CLIENT_SERVER;
	session.client_cid = msg.client_cid;
	session.game_eid = Endpoint.GATE_NODE_SERVER;
	session.game_cid = game_nid_cid_map.get(msg.game_nid);
	session.sid = msg.sid;
	session.account = msg.account;
	on_add_session(session);
}

function game_logout(msg) {
	var session = sid_session_map.get(msg.sid);
	if (session) {
		on_close_session(session.client_cid, msg.error_code);
		on_remove_session(session);
	}
}
