/*
*	描述：db_server脚本
*	作者：张亚磊
*	时间：2016/09/22
*/

require('enum.js');
require('message.js');
require('struct.js');
require('config.js');
require('util.js');
require('timer.js');

//配置管理器
var config = new Config();
//定时器
var timer = new Timer();

function init(node_info) {
	print('db_server init, node_type:',node_info.node_type,' node_id:',node_info.node_id,' node_name:',node_info.node_name);
	config.init();
	timer.init(Node_Type.DB_SERVER);	
	//连接game数据库
	init_db_connect();
}

function on_msg(msg) {
	print('db_server on_msg, cid:',msg.cid,' msg_type:',msg.msg_type,' msg_id:',msg.msg_id,' extra:', msg.extra);
	
	switch(msg.msg_id) {
	case Msg.NODE_GAME_DB_CREATE_PLAYER:
		create_player(msg);
		break;
	case Msg.NODE_GAME_DB_LOAD_PLAYER:
		load_player(msg);
		break;
	case Msg.NODE_GAME_DB_SAVE_PLAYER:
		save_player(msg);
		break;
	case Msg.NODE_PUBLIC_DB_CREATE_GUILD:
		create_guild(msg);
		break;
	case Msg.NODE_PUBLIC_DB_LOAD_DATA:
		load_public_data(msg);
		break;
	case Msg.NODE_PUBLIC_DB_SAVE_DATA:
		save_public_data(msg);
		break;
	case Msg.NODE_PUBLIC_DB_DELETE_DATA:
		delete_public_data(msg);
		break;
	default:
		print('process_msg, msg_id: not exist', msg.msg_id);
		break;
	}
}

function init_db_connect() {
	for(var i = 0; i < config.node_json['node'].length; i++){
		if(config.node_json['node'][i]['node_type'] == Node_Type.DB_SERVER){
			var mysql_conf = config.node_json['node'][i]['mysql_db'];
			if(mysql_conf != null){
				for(var j = 0; j < mysql_conf.length; j++){
					var ret = connect_to_mysql(mysql_conf[j]['db_id'], mysql_conf[j]['ip'], mysql_conf[j]['port'], mysql_conf[j]['user'],
						mysql_conf[j]['password'], mysql_conf[j]['pool_name']);
					if(!ret) {
						print("connect to db ", mysql_conf['db_id'], ' error');
						return;
					}	
				}
			}
			
			var mongo_conf = config.node_json['node'][i]['mongo_db'];
			if(mongo_conf != null){
				for(var j = 0; j < mongo_conf.length; j++){
					var ret = connect_to_mongo(mongo_conf[j]['db_id'], mongo_conf[j]['ip'], mongo_conf[j]['port']);
					if(!ret) {
						print("connect to db ", mongo_conf['db_id'], ' error');
						return;
					}	
				}
			}
		}
	}
}

function send_error_code(cid, extra, error_code) {
	var msg_res = new node_1();
	msg_res.error_code = error_code;
	send_msg(Endpoint.DB_SERVER, cid, Msg.NODE_ERROR_CODE, Msg_Type.NODE_MSG, extra, msg_res);
}

function create_player(msg) {
	var msg_code = Error_Code.RET_OK;
	var role_id = select_table_index(DB_Id.GAME, "game.role", "account", msg.account);
	if (role_id > 0) {
		print('create player, has exist, account:',msg.account);
		send_error_code(msg.cid, msg.extra, Error_Code.ROLE_HAS_EXIST);
	} else {
		role_id = generate_id(DB_Id.GAME, "game.serial", "role_id");
		var msg_res = new node_203();
		msg_res.player_data.player_info.role_id = role_id;
		msg_res.player_data.player_info.account = msg.account;
		msg_res.player_data.player_info.role_name = msg.role_name;
		msg_res.player_data.player_info.level = 1;
		msg_res.player_data.player_info.gender = msg.gender;
		msg_res.player_data.player_info.career = msg.career;
		msg_res.player_data.player_info.create_time = util.now_sec();
		msg_res.player_data.bag_info.role_id = role_id;
		msg_res.player_data.mail_info.role_id = role_id;
		//将玩家数据写到数据库
		save_db_data(DB_Id.GAME, "Player_Data", msg_res.player_data);
		send_msg(Endpoint.DB_SERVER, msg.cid, Msg.NODE_DB_GAME_PLAYER_INFO, Msg_Type.NODE_MSG, msg.extra, msg_res);
	}
}

function load_player(msg) {
	var msg_code = Error_Code.RET_OK;
	var role_id = select_table_index(DB_Id.GAME, "game.role", "account", msg.account);
	if (role_id > 0) {
		var msg_res = new node_203();
		msg_res.player_data = load_db_data(DB_Id.GAME, "Player_Data", role_id);
		send_msg(Endpoint.DB_SERVER, msg.cid, Msg.NODE_DB_GAME_PLAYER_INFO, Msg_Type.NODE_MSG, msg.extra, msg_res);
	} else {
		send_error_code(msg.cid, msg.extra, Error_Code.NEED_CREATE_ROLE);
	}
}

function save_player(msg) {
	save_db_data(DB_Id.GAME, "Player_Data", msg.player_data);
	if (msg.logout) {
		send_error_code(msg.cid, msg.extra, Error_Code.SAVE_PLAYER_COMPLETE);
	}
}

function create_guild(msg) {
	var msg_code = Error_Code.RET_OK;
	var guild_id = select_table_index(DB_Id.GAME, "game.guild", "guild_name", msg.guild_name);
	if (guild_id > 0) {
		print('create guild, has exist, guild_name:',msg.guild_name);
		send_error_code(msg.cid, msg.extra, Error_Code.GUILD_HAS_EXIST);
	} else {
		guild_id = generate_id(DB_Id.GAME, "game.serial", "guild_id");
		var guild_info = new Guild_Info();
		guild_info.guild_id = guild_id;
		guild_info.guild_name = msg.guild_name;
		guild_info.chief_id = msg.chief_id;
		guild_info.create_time = util.now_sec();
		//将公会数据写到数据库
		save_db_data(DB_Id.GAME, "game.guild", guild_info);
		var msg_res = new node_208();
		msg_res.data_type = Public_Data_Type.CREATE_GUILD_DATA;
		msg_res.guild_list.push(guild_info);
		send_msg(Endpoint.DB_SERVER, msg.cid, Msg.NODE_DB_PUBLIC_DATA, Msg_Type.NODE_MSG, msg.extra, msg_res);
	}
}

function load_public_data(msg) {
	var msg_res = new node_208();
	msg_res.data_type = msg.data_type;
	switch (msg.data_type) {
	case Public_Data_Type.ALL_DATA:{
		msg_res.guild_list = load_db_data(DB_Id.GAME, "game.guild", 0);
		msg_res.rank_list = load_db_data(DB_Id.GAME, "game.rank", 0);
		break;
	}
	case Public_Data_Type.GUILD_DATA:
		msg_res.guild_list = load_db_data(DB_Id.GAME, "game.guild", 0);
		break;
	case Public_Data_Type.RANK_DATA:
		msg_res.rank_list = load_db_data(DB_Id.GAME, "game.rank", 0);
		break;
	default:
		print('load_public_data, data_type: not exist', msg.data_type);
		break;
	}
	send_msg(Endpoint.DB_SERVER, msg.cid, Msg.NODE_DB_PUBLIC_DATA, Msg_Type.NODE_MSG, msg.extra, msg_res);
}

function save_public_data(msg) {
	switch (msg.data_type) {
	case Public_Data_Type.GUILD_DATA:
		save_db_data(DB_Id.GAME, "game.guild", msg.guild_list);
		break;
	case Public_Data_Type.RANK_DATA:
		save_db_data(DB_Id.GAME, "game.rank", msg.rank_list);
		break;
	default:
		print('save_public_data, data_type: not exist', msg.data_type);
		break;
	}
}

function delete_public_data(msg) {
	switch (msg.data_type) {
	case Public_Data_Type.GUILD_DATA:
		delete_db_data(DB_Id.GAME, "game.guild", msg.index_list);
		break;
	case Public_Data_Type.RANK_DATA:
		delete_db_data(DB_Id.GAME, "game.rank", msg.index_list);
		break;
	default:
		print('delete_public_data, data_type: not exist', msg.data_type);
		break;
	}
}
