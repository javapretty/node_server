/*
*	描述：game_player类
*	作者：张亚磊
*	时间：2016/02/24
*/

function Game_Player() {
	this.sync_player_data_tick = util.now_sec();
	this.gate_cid = 0;
	this.player_cid = 0;
	this.is_change = false;
	this.player_info = new Game_Player_Info();
	this.mail = new Mail();
	this.bag = new Bag();
}

//玩家上线，加载数据
Game_Player.prototype.load_player_data = function(gate_cid, player_cid, msg) {
	print('********game_player load_data, role_id:', msg.player_data.player_info.role_id, ' role_name:', msg.player_data.player_info.role_name, ' player_cid:', player_cid);
	this.gate_cid = gate_cid;
	this.player_cid = player_cid; 
	this.player_info = msg.player_data.player_info;
	this.mail.load_data(this, msg);
	this.bag.load_data(this, msg);
	
	this.res_role_info();
	this.sync_login_to_public();
	cid_game_player_map.set(util.get_cid(this.gate_cid, this.player_cid), this);
	role_id_game_player_map.set(this.player_info.role_id, this);
	role_name_game_player_map.set(this.player_info.role_name, this);
}

//玩家离线，保存数据
Game_Player.prototype.save_player_data = function() {
	print('********game_player save_data,role_id:', this.player_info.role_id, " role_name:", this.player_info.role_name, ' player_cid:', this.player_cid);
	this.player_info.logout_time = util.now_sec();
	
	this.sync_player_data_to_db(true);
	this.sync_logout_to_log();
	logout_map.set(this.player_info.account, this.player_info.logout_time);
	cid_game_player_map.delete(util.get_cid(this.gate_cid, this.player_cid));
	role_id_game_player_map.delete(this.player_info.role_id);
	role_name_game_player_map.delete(this.player_info.role_name);
}

Game_Player.prototype.tick = function(now) {
	//同步玩家数据到数据库
	if(this.is_change){
		if (now - this.sync_player_data_tick >= 15) {
			this.sync_player_data_to_db(false);
			this.sync_player_data_tick = now;
		}
	}
}

Game_Player.prototype.send_success_msg = function(msg_id, msg) {
	this.is_change = true;
	send_msg(Endpoint.GAME_SERVER, this.gate_cid, msg_id, Msg_Type.NODE_S2C, this.player_cid, msg);
}

Game_Player.prototype.send_error_msg = function(error_code) {
	var msg = new s2c_4();
	msg.error_code = error_code;
	send_msg(Endpoint.GAME_SERVER, this.gate_cid, Msg.RES_ERROR_CODE, Msg_Type.NODE_S2C, this.player_cid, msg);
}

Game_Player.prototype.res_role_info = function() {
	var msg = new s2c_3();
	msg.role_info.role_id = this.player_info.role_id;
	msg.role_info.account = this.player_info.account;
	msg.role_info.role_name = this.player_info.role_name;
	msg.role_info.level = this.player_info.level;
	msg.role_info.exp = this.player_info.exp;
	msg.role_info.career = this.player_info.career;
	msg.role_info.gender = this.player_info.gender;
	this.send_success_msg(Msg.RES_ROLE_INFO, msg);
}

Game_Player.prototype.sync_login_to_public = function() {
	var msg = new node_5();
	msg.player_info.role_id = this.player_info.role_id;
	msg.player_info.account = this.player_info.account;
	msg.player_info.role_name = this.player_info.role_name;
	msg.player_info.level = this.player_info.level;
	msg.player_info.gender = this.player_info.gender;
	msg.player_info.career = this.player_info.career;
	send_msg(Endpoint.GAME_PUBLIC_CONNECTOR, 0, Msg.NODE_GAME_PUBLIC_PLYAER_LOGIN, Msg_Type.NODE_MSG, this.player_cid, msg);
}

Game_Player.prototype.sync_player_data_to_db = function(logout) {
	print('***************sync_player_data_to_db, logout:', logout, ' role_id:', this.player_info.role_id, ' role_name:', this.player_info.role_name);
	var msg = new node_202();
	msg.logout = logout;
	msg.player_data.player_info = this.player_info;
	this.mail.save_data(msg);
	this.bag.save_data(msg);
	send_msg(Endpoint.GAME_DB_CONNECTOR, 0, Msg.NODE_GAME_DB_SAVE_PLAYER, Msg_Type.NODE_MSG, util.get_cid(this.gate_cid, this.player_cid), msg);
	this.is_change = false;
}

Game_Player.prototype.sync_logout_to_log = function() {
	var msg = new node_7();
	msg.logout_info.role_id = this.player_info.role_id;
	msg.logout_info.role_name = this.player_info.role_name;
	msg.logout_info.account = this.player_info.account;
	msg.logout_info.level = this.player_info.level;
	msg.logout_info.client_ip = this.player_info.client_ip;
	msg.logout_info.login_time = this.player_info.login_time;
	msg.logout_info.logout_time = this.player_info.logout_time;
	send_msg(Endpoint.GAME_LOG_CONNECTOR, 0, Msg.NODE_LOG_PLAYER_LOGOUT, Msg_Type.NODE_MSG, 0, msg);
}

Game_Player.prototype.add_exp = function(exp) {
	if (exp <= 0) {
		return this.send_error_msg(Error_Code.CLIENT_PARAM_ERROR);
	}
	
	//经验增加升级
	this.player_info.exp += exp;
	var max_player_level = config.util_json.max_player_level;
	for (var i = this.player_info.level; i < max_player_level; ++i) {
		var level_exp = config.level_json[i].level_exp;
		if (this.player_info.exp < level_exp) 
			break;
		
		this.player_info.level++;
		this.player_info.exp -= level_exp;
	}

	this.sync_login_to_client();
}

Game_Player.prototype.set_guild_info = function(msg) {
	this.player_info.guild_id = msg.guild_id;
	this.player_info.guild_name = msg.guild_name;
	this.is_change = true;
	print('set_guild_info, role_id:', this.player_info.role_id, " role_name:", this.player_info.role_name, 
	" guild_id:", this.player_info.guild_id, " guild_name:", this.player_info.guild_name);
}
