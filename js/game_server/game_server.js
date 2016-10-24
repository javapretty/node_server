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

//cid----game_player cid=util.get_cid(gate_cid,player_cid)
var cid_game_player_map = new Map();
//role_id---game_player
var role_id_game_player_map = new Map();
//role_name---game_player
var role_name_game_player_map = new Map();
//cid---Cid_Info cid=util.get_cid(gate_cid,player_cid)
var login_map = new Map();
//cid---now_sec cid=util.get_cid(gate_cid,player_cid)
var logout_map = new Map();
//配置管理器
var config = new Config();
//定时器
var timer = new Timer();

function init(node_info) {
	print('game_server init, node_type:',node_info.node_type,' node_id:',node_info.node_id,' node_name:',node_info.node_name);
	config.init();
	timer.init(Node_Type.GAME_SERVER);

	var msg = new node_0();
	msg.node_info = node_info;
	send_msg(Endpoint.GAME_CENTER_CONNECTOR, 0, Msg.NODE_CENTER_NODE_INFO, Msg_Type.NODE_MSG, 0, msg);		
}

function on_msg(msg) {
	print('game_server on_msg, cid:',msg.cid,' msg_type:',msg.msg_type,' msg_id:',msg.msg_id,' extra:', msg.extra);
	
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

function process_game_client_msg(msg) {
	if (msg.msg_id == Msg.REQ_FETCH_ROLE) {
		return fetch_role(msg);
	} else if (msg.msg_id == Msg.REQ_CREATE_ROLE) { 
		return create_role(msg);
	}
	
	var cid = util.get_cid(msg.cid, msg.extra);
	var game_player = cid_game_player_map.get(cid);
	if (!game_player) {
		print('process_game_client_msg, game_player not exist, gate_cid:', msg.cid, ' player_cid:', msg.extra, ' msg_id:', msg.msg_id);
		return;
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
		print('process_game_client_msg, msg_id: not exist', msg.msg_id);
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
	case Msg.NODE_GATE_GAME_PLAYER_LOGOUT:
		close_session(Endpoint.GAME_SERVER, msg.extra, Error_Code.RET_OK);
		break;
	case Msg.NODE_PUBLIC_GAME_GUILD_INFO: {
		var game_player = role_id_game_player_map.get(msg.role_id);
		if (game_player) {
			game_player.set_guild_info(msg);
		}
		break;
	}
	default:
		print('process_game_node_msg, msg_id: not exist', msg.msg_id);
		break;
	}
}

function send_client_error_code(cid, extra, error_code) {
	var msg_res = new s2c_4();
	msg_res.error_code = error_code;
	send_msg(Endpoint.GAME_SERVER, cid, Msg.RES_ERROR_CODE, Msg_Type.NODE_S2C, extra, msg_res);
}

function process_error_code(msg) {
	switch (msg.error_code) {
	case Error_Code.NEED_CREATE_ROLE:
	case Error_Code.ROLE_HAS_EXIST: {
		var cid_info = login_map.get(msg.extra);
		if (cid_info) {
			send_client_error_code(cid_info.gate_cid, cid_info.player_cid, msg.error_code);
			login_map.delete(msg.extra);
		}
		break;
	}
	case Error_Code.SAVE_PLAYER_COMPLETE:{
		logout_map.delete(msg.extra);
		break;
	}
	}
}

function fetch_role(msg) {
	var cid = util.get_cid(msg.cid, msg.extra);
	var game_player = role_id_game_player_map.get(msg.role_id);
	if (game_player || login_map.get(cid) || logout_map.get(cid) ) {
		print('relogin account:', msg.account);
		return close_session(Endpoint.GAME_SERVER, msg.extra, Error_Code.DISCONNECT_RELOGIN);
	}

	//登录从数据库加载玩家信息
	print('load player info from db, account:', msg.account, ' gate_cid:', msg.cid, ' player_cid:', msg.extra);
	var cid_info = new Cid_Info();
	cid_info.gate_cid = msg.cid;
	cid_info.player_cid = msg.extra;
	login_map.set(cid, cid_info);

	var msg_res = new node_201();
	msg_res.account = msg.account;
	send_msg(Endpoint.GAME_DB_CONNECTOR, 0, Msg.NODE_GAME_DB_LOAD_PLAYER, Msg_Type.NODE_MSG, cid, msg_res);
}

function create_role(msg) {
	if (msg.account.length <= 0 || msg.role_name.length <= 0) {
		print('create_role parameter invalid, account:', msg.account, ' role_name:', msg.role_name);
		return send_client_error_code(msg.cid, msg.extra, Error_Code.CLIENT_PARAM_ERROR);
	}
	
	var cid = util.get_cid(msg.cid, msg.extra);
	if (login_map.get(cid)) {
		print('account in logining status, account:', msg.account, ' role_name:', msg.role_name);
		return close_session(Endpoint.GAME_SERVER, msg.extra, Error_Code.DISCONNECT_RELOGIN);	
	}

	var cid_info = new Cid_Info();
	cid_info.gate_cid = msg.cid;
	cid_info.player_cid = msg.extra;
	login_map.set(cid, cid_info);
	
	var msg_res = new node_200();
	msg_res.account = msg.account;
	msg_res.role_name = msg.role_name;
	msg_res.gender = msg.gender;
	msg_res.career = msg.career;
	send_msg(Endpoint.GAME_DB_CONNECTOR, 0, Msg.NODE_GAME_DB_CREATE_PLAYER, Msg_Type.NODE_MSG, cid, msg_res);
}

function load_player_data(msg) {
	var cid_info = login_map.get(msg.extra);
	if (cid_info == null) {
		return print('player not in login map, account:', msg.account);
	}
	
	var game_player = new Game_Player();
	game_player.load_player_data(cid_info.gate_cid, cid_info.player_cid, msg);
	login_map.delete(msg.extra);
}
