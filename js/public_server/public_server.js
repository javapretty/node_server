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
require('public_server/public_player.js');
require('public_server/guild.js');
require('public_server/rank.js');

//gate_cid----public_player  gate_cid=util.get_cid(gate_cid, player_cid)
var gate_cid_public_player_map = new Map();
//game_cid----public_player  game_cid=util.get_cid(game_cid, player_cid)
var game_cid_public_player_map = new Map();
//role_id---public_player
var role_id_public_player_map = new Map();
//role_name---public_player
var role_name_public_player_map = new Map();

//加载配置文件
var config = new Config();
config.init();
//上次保存数据时间
var last_save_time = util.now_sec();
//公会管理器
var guild_manager = new Guild();
//排行榜管理器
var rank_manager = new Rank();
//加载public服务器公共数据
load_public_data();

function on_msg(msg) {
	print('public_server on_msg, cid:',msg.cid,' msg_type:',msg.msg_type,' msg_id:',msg.msg_id,' extra:', msg.extra);
	
	if (msg.msg_type == Msg_Type.NODE_C2S) {
		process_public_client_msg(msg);
	} else if (msg.msg_type == Msg_Type.NODE_MSG) {
		process_public_node_msg(msg);
	}
}

function on_tick(tick_time) {
	if (tick_time - last_save_time >= 30) {
		last_save_time += 30;
		guild_manager.save_data_handler();
		rank_manager.save_data();
	}
}

function load_public_data() {
	var msg = new node_205();
	msg.data_type = Public_Data_Type.ALL_DATA;
	send_msg(Endpoint.PUBLIC_DB_CONNECTOR, 0, Msg.NODE_PUBLIC_DB_LOAD_DATA, Msg_Type.NODE_MSG, 0, msg);
}

function process_public_client_msg(msg) {	
	var cid = util.get_cid(msg.cid, msg.extra);
	var public_player = gate_cid_public_player_map.get(cid);
	if (!public_player) {
		print('process_public_client_msg public_player not exist, gate_cid:', msg.cid, ' player_cid:', msg.extra, ' msg_id:', msg.msg_id);
		return;
	}
	
	switch(msg.msg_id) {
	case Msg.REQ_FETCH_RANK:
		rank_manager.fetch_rank(public_player, msg);
		break;
	case Msg.REQ_CREATE_GUILD:
		guild_manager.create_guild(public_player, msg);
		break;
	case Msg.REQ_DISSOVE_GUILD:
		guild_manager.dissove_guild(public_player, msg);
		break;
	default:
		print('process_public_client_msg, msg_id not exist:', msg.msg_id);
		break;
	}
}

function process_public_node_msg(msg) {
	switch(msg.msg_id) {
	case Msg.NODE_ERROR_CODE: {
		if (msg.error_code == Error_Code.GUILD_HAS_EXIST) {
			var player = gate_cid_public_player_map.get(msg.extra);
			if (player) {
				player.send_error_msg(msg.error_code);
			}
		}
		break;
	}
	case Msg.NODE_DB_PUBLIC_DATA: {
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
	case Msg.NODE_GATE_PUBLIC_PLAYER_LOGIN_LOGOUT: {
		//gate通知public玩家上线,下线
		if (msg.login) {
			var public_player = role_id_public_player_map.get(msg.role_id);
			if (public_player == null) {
				public_player = new Public_Player();
			}
			public_player.set_gate_cid(msg.cid, msg.extra, msg.role_id);
		} else {
			close_session(Endpoint.PUBLIC_GATE_SERVER, msg.extra, Error_Code.RET_OK);
		}
		break;
	}
	case Msg.NODE_GAME_PUBLIC_PLYAER_LOGIN: {
		//game通知public玩家上线
		var public_player = role_id_public_player_map.get(msg.player_info.role_id);
		if (public_player == null) {
			public_player = new Public_Player();
		}
		public_player.load_player_data(msg.cid, msg.extra, msg.player_info);
		break;
	}
	default:
		print('proceess_public_node_msg, msg_id not exist:', msg.msg_id);
		break;
	}
}
