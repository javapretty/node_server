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
require('gate_server/gate_player.js');

//cid--gate_player
var cid_gate_player_map = new Map();
//account--gate_player
var account_gate_player_map = new Map();
//加载配置文件
var config = new Config();
config.init();

function on_msg(msg) {
	print('gate_server on_msg, cid:',msg.cid,' msg_type:',msg.msg_type,' msg_id:',msg.msg_id,' extra:', msg.extra);
	
	if (msg.msg_type == Msg_Type.C2S) {
		process_gate_client_msg(msg);
	} else if (msg.msg_type == Msg_Type.NODE_S2C) {
		process_gate_s2c_msg(msg);
	} else if (msg.msg_type == Msg_Type.NODE_MSG) {
		process_gate_node_msg(msg);
	}
}

function on_tick(tick_time) {
	
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
	//消息转到client
	send_msg(Endpoint.GATE_CLIENT_SERVER, msg.extra, msg.msg_id, Msg_Type.S2C, 0, msg);
	
	//登录game成功，通知public
	if (msg.msg_id == Msg.RES_ROLE_INFO) {
		var msg_res = new node_4();
		msg_res.login = true;
		msg_res.role_id = msg.role_info.role_id;
		send_msg(Endpoint.GATE_PUBLIC_CONNECTOR, 0, Msg.NODE_GATE_PUBLIC_PLAYER_LOGIN_LOGOUT, Msg_Type.NODE_MSG, msg.extra, msg_res);
	}
}

function process_gate_node_msg(msg) {
	switch(msg.msg_id) {
	case Msg.NODE_ERROR_CODE: {
		if (msg.error_code == Error_Code.TOKEN_NOT_EXIST) {
			send_msg(Endpoint.GATE_CLIENT_SERVER, msg.extra, Msg.RES_ERROR_CODE, Msg_Type.S2C, 0, msg);
		}
		break;
	}
	case Msg.NODE_GATE_CENTER_VERIFY_TOKEN:
		verify_token(msg);
		break;
	default:
		print('process_gate_node_msg, msg_id: not exist', msg.msg_id);		
		break;
	}
}

function connect_gate(msg) {
	print('connect_gate, account:', msg.account, ' token:', msg.token);
	if (account_gate_player_map.get(msg.account)) {
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
	var gate_player = new Gate_Player();
	gate_player.game_endpoint = Endpoint.GATE_GAME1_CONNECTOR;
	gate_player.cid = msg.extra;
	gate_player.account = msg.account;
	account_gate_player_map.set(msg.account, gate_player);
	cid_gate_player_map.set(msg.extra, gate_player);

	var msg_res = new s2c_2();
	msg_res.account = msg.account;
	send_msg(Endpoint.GATE_CLIENT_SERVER, msg.extra, Msg.RES_CONNECT_GATE, Msg_Type.S2C, 0, msg_res);
}

function transmit_msg(msg) {
	var gate_player = cid_gate_player_map.get(msg.cid);
	if (!gate_player) {
		print('player not in gate server,cid:',msg.cid,' msg_id:',msg.msg_id);
		return close_session(Endpoint.GATE_CLIENT_SERVER, msg.cid, Error_Code.DISCONNECT_NOLOGIN);
	}
	
	if (msg.msg_id == Msg.REQ_HEARTBEAT) {
		gate_player.on_heartbeat(msg);
	} else if (msg.msg_id > Msg.REQ_HEARTBEAT && msg.msg_id < Msg.REQ_FETCH_RANK) {
		//客户端消息转到game
		send_msg(gate_player.game_endpoint, 0, msg.msg_id, Msg_Type.NODE_C2S, msg.cid, msg);
	} else {
		//客户端消息转到public
		send_msg(Endpoint.GATE_PUBLIC_CONNECTOR, 0, msg.msg_id, Msg_Type.NODE_C2S, msg.cid, msg);
	}
}
