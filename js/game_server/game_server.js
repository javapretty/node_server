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
//sid----game_player
var sid_game_player_map = new Map();
//role_id---game_player
var role_id_game_player_map = new Map();
//role_name---game_player
var role_name_game_player_map = new Map();
//sid---gate_cid
var login_map = new Map();
//sid---now_sec
var logout_map = new Map();
//配置管理器
var config = new Config();
//定时器
var timer = new Timer();

function init(node_info) {
	log_info('game_server init, node_type:',node_info.node_type,' node_id:',node_info.node_id,' node_name:',node_info.node_name);
	config.init();
	timer.init(Node_Type.GAME_SERVER);
	game_node_info = node_info;

	var msg = new node_0();
	msg.node_info = node_info;
	send_msg_to_center(Msg.NODE_CENTER_NODE_INFO, 0, msg);		
}

function on_drop(cid) { }

function on_msg(msg) {
	log_debug('game_server on_msg, cid:',msg.cid,' msg_type:',msg.msg_type,' msg_id:',msg.msg_id,' sid:', msg.sid);
	
	if (msg.msg_type == Msg_Type.NODE_C2S) {
		process_game_client_msg(msg);
	} else if (msg.msg_type == Msg_Type.NODE_MSG) {
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
	send_msg(Endpoint.GAME_DB_CONNECTOR, 0, msg_id, Msg_Type.NODE_MSG, sid, msg);
}

function send_msg_to_log(msg_id, sid, msg) {
	send_msg(Endpoint.GAME_LOG_CONNECTOR, 0, msg_id, Msg_Type.NODE_MSG, sid, msg);
}

function send_msg_to_public(msg_id, sid, msg) {
	send_msg(Endpoint.GAME_PUBLIC_CONNECTOR, 0, msg_id, Msg_Type.NODE_MSG, sid, msg);
}

function send_msg_to_center(msg_id, sid, msg) {
	send_msg(Endpoint.GAME_CENTER_CONNECTOR, 0, msg_id, Msg_Type.NODE_MSG, sid, msg);
}

function send_error_msg(cid, sid, error_code) {
	var msg_res = new s2c_4();
	msg_res.error_code = error_code;
	send_msg(Endpoint.GAME_SERVER, cid, Msg.RES_ERROR_CODE, Msg_Type.NODE_S2C, sid, msg_res);
}

function remove_session(sid) {
	var game_player = sid_game_player_map.get(sid);
	if (!game_player) {
		log_error('remove session game_player not exist, sid:', sid);
		return;
	}
	game_player.logout();
	//game_server主动踢玩家下线时候，需要同步消息到gate
	game_player.sync_login_logout_to_gate(false);
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
		log_error('process_game_client_msg, msg_id not exist:', msg.msg_id);
		break;
	}
}

function process_game_node_msg(msg) {
	switch(msg.msg_id) {
	case Msg.NODE_ERROR_CODE:
		process_error_code(msg);
		break;
	case Msg.NODE_DB_GAME_PLAYER_INFO:
		load_player_data(msg);
		break;
	case Msg.NODE_GATE_PUBLIC_LOGIN_GAME_LOGOUT:
		var game_player = sid_game_player_map.get(msg.sid);
		if (game_player) {
			//gate通知玩家下线时候，不需要同步消息到gate
			game_player.logout();
		}
		break;
	case Msg.NODE_PUBLIC_GAME_GUILD_INFO: {
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
		var gate_cid = login_map.get(msg.sid);
		if (gate_cid != 0) {
			send_error_msg(gate_cid, msg.sid, msg.error_code);
			login_map.delete(msg.sid);
		}
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
	if (game_player || login_map.get(msg.sid) || logout_map.get(msg.sid) ) {
		log_error('relogin account:', msg.account);
		return remove_session(msg.sid);
	}

	//登录从数据库加载玩家信息
	log_info('load player info from db, account:', msg.account, ' gate_cid:', msg.cid, ' sid:', msg.sid);
	login_map.set(msg.sid, msg.cid);

	var msg_res = new node_201();
	msg_res.account = msg.account;
	send_msg_to_db(Msg.NODE_GAME_DB_LOAD_PLAYER, msg.sid, msg_res);
}

function create_role(msg) {
	if (msg.account.length <= 0 || msg.role_name.length <= 0) {
		log_error('create_role parameter invalid, account:', msg.account, ' role_name:', msg.role_name);
		return send_error_msg(msg.cid, msg.sid, Error_Code.CLIENT_PARAM_ERROR);
	}

	if (login_map.get(msg.sid)) {
		log_error('account in logining status, account:', msg.account, ' role_name:', msg.role_name);
		return remove_session(msg.sid);
	}

	login_map.set(msg.sid, msg.cid);
	
	var msg_res = new node_200();
	msg_res.account = msg.account;
	msg_res.role_name = msg.role_name;
	msg_res.gender = msg.gender;
	msg_res.career = msg.career;
	send_msg_to_db(Msg.NODE_GAME_DB_CREATE_PLAYER, msg.sid, msg_res);
}

function load_player_data(msg) {
	var game_player = new Game_Player();
	game_player.login(login_map.get(msg.sid), msg.sid, msg);
	login_map.delete(msg.sid);
}