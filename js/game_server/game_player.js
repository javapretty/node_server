/*
*	描述：game_player类
*	作者：张亚磊
*	时间：2016/02/24
*/

function Game_Player() {
	this.gate_eid = 0;          //玩家连接的gate端点id
	this.sid = 0;               //玩家sid
	this.data_change = false;   //玩家数据是否改变
	this.save_data_tick = util.now_sec();   //玩家数据保存tick
	this.role_info = null;
	this.mail = new Mail();
	this.bag = new Bag();
}

//玩家上线，加载数据
Game_Player.prototype.login = function(gate_eid, sid, player_data) {
	log_info("game_player login, sid:",sid," role_id:",player_data.role_info.role_id," role_name:",player_data.role_info.role_name," gate_eid:", gate_eid);
	this.gate_eid = gate_eid;
	this.sid = sid;
	this.role_info = player_data.role_info;
	this.role_info.login_time = util.now_sec();
	this.mail.load_data(this, player_data);
	this.bag.load_data(this, player_data);
	
	this.sync_login_to_client();
	this.sync_login_logout_to_public(true);
	global.sid_game_player_map.set(this.sid, this);
	global.role_id_game_player_map.set(this.role_info.role_id, this);
	global.role_name_game_player_map.set(this.role_info.role_name, this);
}

//玩家离线，保存数据
Game_Player.prototype.logout = function () {
    log_info("game_player logout, sid:", this.sid, " role_id:", this.role_info.role_id, " role_name:", this.role_info.role_name," gate_eid:",this.gate_eid);
	this.role_info.logout_time = util.now_sec();
	global.logout_map.set(this.sid, this.role_info.logout_time);
	
	this.sync_player_data_to_db(true);
	this.sync_logout_to_log();
	this.sync_login_logout_to_public(false);
	global.sid_gate_eid_map.delete(this.sid);
	global.sid_game_player_map.delete(this.sid);
	global.role_id_game_player_map.delete(this.role_info.role_id);
	global.role_name_game_player_map.delete(this.role_info.role_name);
}

Game_Player.prototype.tick = function(now) {
	//同步数据到数据库
    if (this.data_change && now - this.save_data_tick >= 30) {
        this.sync_player_data_to_db(false);
        this.save_data_tick = now;
        this.data_change = false;
	}
}

Game_Player.prototype.send_success_msg = function(msg_id, msg) {
	send_msg(this.gate_eid, 0 , msg_id, Msg_Type.NODE_S2C, this.sid, msg);
}

Game_Player.prototype.send_error_msg = function(error_code) {
	var msg = new Object();
    msg.error_code = error_code;
    send_msg(this.gate_eid, 0, Msg.RES_ERROR_CODE, Msg_Type.NODE_S2C, this.sid, msg);
}

Game_Player.prototype.sync_login_to_client = function() {
	var msg = new Object();
	msg.role_info = this.role_info;
	this.send_success_msg(Msg.RES_ROLE_INFO, msg);
}

Game_Player.prototype.sync_login_logout_to_public = function(login) {
	var msg = new Object();
    msg.login = login;
    msg.role_info = this.role_info;
	send_msg_to_public(Msg.SYNC_GAME_PUBLIC_LOGIN_LOGOUT, this.sid, msg);
}

Game_Player.prototype.sync_player_data_to_db = function (logout) {
    log_info("sync_player_data_to_db logout:",logout," sid:", this.sid, " role_id:", this.role_info.role_id, " role_name:", this.role_info.role_name, " gate_eid:", this.gate_eid);
    var msg = new Object();
	if(logout) {
	    msg.save_type = Save_Type.SAVE_DB_CLEAR_CACHE;
	} else {
	    msg.save_type = Save_Type.SAVE_CACHE;
	}
	msg.vector_data = false;
	msg.db_id = DB_Id.GAME;
	msg.struct_name = "Player_Data";
	msg.data_type = DB_Data_Type.PLAYER;
	msg.player_data = new Object();
	msg.player_data.role_info = this.role_info;
	this.mail.save_data(msg.player_data);
	this.bag.save_data(msg.player_data);
	send_msg_to_db(Msg.SYNC_SAVE_DB_DATA, this.sid, msg);
}

Game_Player.prototype.sync_logout_to_log = function() {
	var msg = new Object();
    msg.save_type = Save_Type.SAVE_DB_CLEAR_CACHE;
    msg.vector_data = false;
    msg.db_id = DB_Id.LOG;
    msg.struct_name = "Logout_Info";
    msg.data_type = DB_Data_Type.LOGOUT;
    msg.logout_info = new Object();
    msg.logout_info.role_id = this.role_info.role_id;
    msg.logout_info.role_name = this.role_info.role_name;
    msg.logout_info.account = this.role_info.account;
    msg.logout_info.level = this.role_info.level;
    msg.logout_info.exp = this.role_info.exp;
    msg.logout_info.gender = this.role_info.gender;
    msg.logout_info.career = this.role_info.career;
    msg.logout_info.create_time = this.role_info.create_time;
    msg.logout_info.login_time = this.role_info.login_time;
    msg.logout_info.logout_time = this.role_info.logout_time;
	send_msg_to_log(Msg.SYNC_SAVE_DB_DATA, this.sid, msg);
}

Game_Player.prototype.set_guild_info = function(msg) {
	this.role_info.guild_id = msg.guild_id;
	this.role_info.guild_name = msg.guild_name;
	this.data_change = true;
	log_info('set_guild_info, role_id:', this.role_info.role_id, " role_name:", this.role_info.role_name, 
	" guild_id:", this.role_info.guild_id, " guild_name:", this.role_info.guild_name);
}
