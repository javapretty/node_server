/*
*	描述：public_server脚本
*	作者：张亚磊
*	时间：2016/02/24
*/

require('enum.js');
require('message.js');
require('struct.js');
require('config.js');
require('util.js');
require('timer.js');
require('public_server/public_player.js');
require('public_server/guild.js');
require('public_server/rank.js');

//sid----public_player
var sid_public_player_map = new Map();
//role_id---public_player
var role_id_public_player_map = new Map();

//公会管理器
var guild_manager = new Guild();
//排行榜管理器
var rank_manager = new Rank();
//配置管理器
var config = new Config();
//定时器
var timer = new Timer();

function init(node_info) {
	log_info('public_server init, node_type:',node_info.node_type,' node_id:',node_info.node_id,' node_name:',node_info.node_name);
	config.init();
	timer.init(Node_Type.PUBLIC_SERVER);	
	//加载公共数据
	load_public_data();
	var msg = new node_2();
	msg.node_info = node_info;
	send_msg(Endpoint.PUBLIC_MASTER_CONNECTOR, 0, Msg.SYNC_NODE_INFO, Msg_Type.NODE_MSG, 0, msg);
}

function on_drop(cid) { }

function on_msg(msg) {
	log_debug('public_server on_msg, cid:',msg.cid,' msg_type:',msg.msg_type,' msg_id:',msg.msg_id,' sid:', msg.sid);
	
	if (msg.msg_type == Msg_Type.NODE_C2S) {
		process_public_client_msg(msg);
	} else if (msg.msg_type == Msg_Type.NODE_MSG) {
		process_public_node_msg(msg);
	}
}

function on_tick(timer_id) {
	var timer_handler = timer.get_timer_handler(timer_id);
	if (timer_handler != null) {
		timer_handler();
	}
}

function send_msg_to_db(msg_id, sid, msg) {
	send_msg(Endpoint.PUBLIC_DATA_CONNECTOR, 0, msg_id, Msg_Type.NODE_MSG, sid, msg);
}

function send_public_msg(cid, msg_id, sid, msg) {
	send_msg(Endpoint.PUBLIC_SERVER, cid, msg_id, Msg_Type.NODE_MSG, sid, msg);
}

function load_public_data() {
	var msg = new node_205();
	msg.data_type = Public_Data_Type.ALL_DATA;
	send_msg_to_db(Msg.SYNC_PUBLIC_DB_LOAD_DATA, 0, msg);
}

function process_public_client_msg(msg) {
	var public_player = sid_public_player_map.get(msg.sid);
	if (!public_player) {
		return log_error('process_public_client_msg, public_player not exist, game_cid:', msg.cid, ' sid:', msg.sid, ' msg_id:', msg.msg_id);
	}
	
	switch(msg.msg_id) {
	case Msg.REQ_CREATE_GUILD:
		guild_manager.create_guild(public_player, msg);
		break;
	case Msg.REQ_DISSOVE_GUILD:
		guild_manager.dissove_guild(public_player, msg);
		break;
	default:
		log_error('process_public_client_msg, msg_id not exist:', msg.msg_id);
		break;
	}
}

function process_public_node_msg(msg) {
	switch(msg.msg_id) {
	case Msg.SYNC_ERROR_CODE: {
		if (msg.error_code == Error_Code.GUILD_HAS_EXIST) {
			var player = sid_public_player_map.get(msg.sid);
			if (player) {
				player.send_error_msg(msg.error_code);
			}
		}
		break;
	}
	case Msg.SYNC_DB_PUBLIC_DATA: {
		switch (msg.data_type) {
		case Public_Data_Type.ALL_DATA: {
			guild_manager.load_data(msg);
			rank_manager.load_data(msg);
			break;
		}
		case Public_Data_Type.GUILD_DATA:
			guild_manager.load_data(msg);
			break;
		case Public_Data_Type.CREATE_GUILD_DATA:
			guild_manager.db_create_guild(msg);
			break;
		case Public_Data_Type.RANK_DATA:
			rank_manager.load_data(msg);
			break;
		default :
			break;
		}
		break;
	}
	case Msg.SYNC_GAME_PUBLIC_LOGIN_LOGOUT: {
		//game通知public玩家上线下线
		if (msg.login) {
			var public_player = sid_public_player_map.get(msg.sid);
			if (public_player == null) {
				public_player = new Public_Player();
			}
			public_player.login(msg.cid, msg.sid, msg.player_info);	
		} 
		else {
			var public_player = sid_public_player_map.get(msg.sid);
			if (public_player) {
				public_player.logout();
			}
		}
		break;
	}
	default:
		log_error('proceess_public_node_msg, msg_id not exist:', msg.msg_id);
		break;
	}
}
