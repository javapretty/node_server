/*
*	描述：public_player信息类
*	作者：张亚磊
*	时间：2016/03/26
*/

function Public_Player() {
	this.game_cid = 0;
	this.sid = 0;
	this.player_info = new Public_Player_Info();
}

//玩家上线，加载数据
Public_Player.prototype.login = function(game_cid, sid, player_info) {
	log_info('********public_player login, game_cid:', game_cid, ' sid:', sid, ' role_id:', player_info.role_id);

	this.game_cid = game_cid;
	this.sid = sid;
	this.player_info = player_info;
	sid_public_player_map.set(this.sid, this);
	role_id_public_player_map.set(this.player_info.role_id, this);
	
	rank_manager.update_rank(Rank_Type.LEVEL_RANK, this);
}

//玩家离线，保存数据
Public_Player.prototype.logout = function() {
	log_info('********public_player logout, role_id:', this.player_info.role_id, ' sid:', this.sid, " role_name:", this.player_info.role_name);
	sid_public_player_map.delete(this.sid);
	role_id_public_player_map.delete(this.player_info.role_id);
}

Public_Player.prototype.tick = function(now) {}

Public_Player.prototype.send_success_msg = function(msg_id, msg) {
	send_msg(Endpoint.PUBLIC_SERVER, this.game_cid, msg_id, Msg_Type.NODE_S2C, this.sid, msg);
}

Public_Player.prototype.send_error_msg = function(error_code) {
	var msg = new s2c_4();
	msg.error_code = error_code;
	send_msg(Endpoint.PUBLIC_SERVER, this.game_cid, Msg.RES_ERROR_CODE, Msg_Type.NODE_S2C, this.sid, msg);
}
