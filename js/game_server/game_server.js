/*
*	描述：game_server脚本
*	作者：张亚磊
*	时间：2016/02/24
*/

require('enum.js');
require('message.js');
require('struct.js');
require('config.js');
require('util.js');
require('timer.js');
require('game_server/game_player.js');
require('game_server/bag.js');
require('game_server/mail.js');

//game进程节点信息
var game_node_info = null;
//sid--gate_endpoint_id
var sid_gate_eid_map = new Map();
//sid--logout_time
var logout_map = new Map();

//sid----game_player
var sid_game_player_map = new Map();
//role_id---game_player
var role_id_game_player_map = new Map();
//role_name---game_player
var role_name_game_player_map = new Map();

//配置管理器
var config = new Config();
//定时器
var timer = new Timer();

function init(node_info) {
	log_info('game_server init, node_type:',node_info.node_type,' node_id:',node_info.node_id,' node_name:',node_info.node_name);
	config.init();
	timer.init(Node_Type.GAME_SERVER);
	game_node_info = node_info;

	var msg = new node_2();
	msg.node_info = node_info;
	send_msg(Endpoint.GAME_MASTER_CONNECTOR, 0, Msg.SYNC_NODE_INFO, Msg_Type.NODE_MSG, 0, msg);
	send_msg(Endpoint.GAME_GATE1_CONNECTOR, 0, Msg.SYNC_NODE_INFO, Msg_Type.NODE_MSG, 0, msg);
	send_msg(Endpoint.GAME_GATE2_CONNECTOR, 0, Msg.SYNC_NODE_INFO, Msg_Type.NODE_MSG, 0, msg);	
}

function on_drop(cid) { }

function on_msg(msg) {
	log_debug('game_server on_msg, cid:',msg.cid,' msg_type:',msg.msg_type,' msg_id:',msg.msg_id,' sid:', msg.sid);
	
	if (msg.msg_type == Msg_Type.NODE_C2S) {
		process_game_client_msg(msg);
	} 
	else if (msg.msg_type == Msg_Type.NODE_S2C) {
		var game_player = sid_game_player_map.get(msg.sid);
		if (game_player) {
			send_msg(game_player.gate_eid, 0 , msg.msg_id, msg.msg_type, msg.sid, msg);
		}
	} 
	else if (msg.msg_type == Msg_Type.NODE_MSG) {
		process_game_node_msg(msg);
	}
}

function on_tick(timer_id) {
	var timer_handler = timer.get_timer_handler(timer_id);
	if (timer_handler != null) {
		timer_handler();
	}
}

function send_msg_to_db(msg_id, sid, msg) {
	send_msg(Endpoint.GAME_DATA_CONNECTOR, 0, msg_id, Msg_Type.NODE_MSG, sid, msg);
}

function send_msg_to_log(msg_id, sid, msg) {
	send_msg(Endpoint.GAME_LOG_CONNECTOR, 0, msg_id, Msg_Type.NODE_MSG, sid, msg);
}

function send_msg_to_public(msg_id, sid, msg) {
	send_msg(Endpoint.GAME_PUBLIC_CONNECTOR, 0, msg_id, Msg_Type.NODE_MSG, sid, msg);
}

function on_add_session(sid, gate_nid) {
	var gate_eid = gate_nid % 10000 + 4;
	sid_gate_eid_map.set(sid, gate_eid);
}

function on_remove_session(sid, error_code) {
	var gate_eid = 0;
	var game_player = sid_game_player_map.get(sid);
	if (game_player) {
		gate_eid = game_player.gate_eid;
		game_player.logout();
	} else {
		gate_eid = sid_gate_eid_map.get(sid);
	}
	//session移除时候通知gate
	var msg = new node_6();
	msg.error_code = error_code;
	send_msg(gate_eid, 0, Msg.SYNC_GAME_GATE_LOGOUT, Msg_Type.NODE_MSG, sid, msg);
	sid_gate_eid_map.delete(sid);
}

function process_game_client_msg(msg) {
	if (msg.msg_id == Msg.REQ_FETCH_ROLE) {
		return fetch_role(msg);
	} else if (msg.msg_id == Msg.REQ_CREATE_ROLE) { 
		return create_role(msg);
	}

	var game_player = sid_game_player_map.get(msg.sid);
	if (!game_player) {
		return log_error('process_game_client_msg, game_player not exist, gate_cid:', msg.cid, ' sid:', msg.sid, ' msg_id:', msg.msg_id);
	}
	
	switch(msg.msg_id) {
	case Msg.REQ_FETCH_MAIL:
		game_player.mail.fetch_mail();
		break;
	case Msg.REQ_PICKUP_MAIL:
		game_player.mail.pickup_mail(msg);
		break;
	case Msg.REQ_DEL_MAIL:
		game_player.mail.delete_mail(msg);
		break;
	case Msg.REQ_FETCH_BAG:
		game_player.bag.fetch_bag();
		break;
	default:
		send_msg(Endpoint.GAME_PUBLIC_CONNECTOR, 0, msg.msg_id, msg.msg_type, msg.sid, msg);
		break;
	}
}

function process_game_node_msg(msg) {
	switch(msg.msg_id) {
	case Msg.SYNC_ERROR_CODE:
		process_error_code(msg);
		break;
	case Msg.SYNC_GATE_GAME_ADD_SESSION:
		on_add_session(msg.sid, msg.gate_nid);
		break;
	case Msg.SYNC_GAME_GATE_LOGOUT: {
		var game_player = sid_game_player_map.get(msg.sid);
		if (game_player) {
			//gate通知玩家下线时候
			game_player.logout();
		}
		break;	
	}
	case Msg.SYNC_DB_GAME_PLAYER_INFO:
		load_player_data(msg);
		break;
	case Msg.SYNC_PUBLIC_GAME_GUILD_INFO: {
		var game_player = role_id_game_player_map.get(msg.role_id);
		if (game_player) {
			game_player.set_guild_info(msg);
		}
		break;
	}
	default:
		log_error('process_game_node_msg, msg_id not exist:', msg.msg_id);
		break;
	}
}

function process_error_code(msg) {
	switch (msg.error_code) {
	case Error_Code.NEED_CREATE_ROLE:
	case Error_Code.ROLE_HAS_EXIST: {
		var gate_eid = sid_gate_eid_map.get(msg.sid);
		var msg_res = new s2c_5();
		msg_res.error_code = msg.error_code;
		send_msg(gate_eid, 0, Msg.RES_ERROR_CODE, Msg_Type.NODE_S2C, msg.sid, msg_res);
		break;
	}
	case Error_Code.PLAYER_SAVE_COMPLETE:{
		logout_map.delete(msg.sid);
		break;
	}
	}
}

function fetch_role(msg) {
	var game_player = role_id_game_player_map.get(msg.role_id);
	if (game_player || logout_map.get(msg.sid)) {
		log_error('relogin account:', msg.account);
		return on_remove_session(msg.sid, Error_Code.DISCONNECT_RELOGIN);
	}

	//登录从数据库加载玩家信息
	log_info('load player info from db, account:', msg.account, ' gate_cid:', msg.cid, ' sid:', msg.sid);
	var msg_res = new node_201();
	msg_res.account = msg.account;
	send_msg_to_db(Msg.SYNC_GAME_DB_LOAD_PLAYER, msg.sid, msg_res);
}

function create_role(msg) {
	if (msg.account.length <= 0 || msg.role_name.length <= 0 || logout_map.get(msg.sid)) {
		log_error('create_role parameter invalid, account:', msg.account, ' role_name:', msg.role_name);
		return on_remove_session(msg.sid, Error_Code.CLIENT_PARAM_ERROR);
	}
	
	var msg_res = new node_200();
	msg_res.account = msg.account;
	msg_res.role_name = msg.role_name;
	msg_res.gender = msg.gender;
	msg_res.career = msg.career;
	send_msg_to_db(Msg.SYNC_GAME_DB_CREATE_PLAYER, msg.sid, msg_res);
}

function load_player_data(msg) {
	var game_player = new Game_Player();
	game_player.login(sid_gate_eid_map.get(msg.sid), msg.sid, msg);
}
