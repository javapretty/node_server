/*
*	描述：public_player信息类
*	作者：张亚磊
*	时间：2016/03/26
*/

function Public_Player() {
	this.gate_cid = 0;
	this.game_cid = 0;
	this.player_cid = 0;
	this.player_info = new Public_Player_Info();
}

Public_Player.prototype.set_gate_cid = function(gate_cid, player_cid, role_id) {
	print('********public_player login from gate, gate_cid:', gate_cid, ' player_cid:', player_cid, ' role_id:', role_id);
	
	this.gate_cid = gate_cid;
	this.player_cid = player_cid;
	this.player_info.role_id = role_id;
	gate_cid_public_player_map.set(util.get_cid(gate_cid, player_cid), this);
	role_id_public_player_map.set(this.player_info.role_id, this);
}

//玩家上线，加载数据
Public_Player.prototype.load_player_data = function(game_cid, player_cid, player_info) {
	print('********public_player login from game, game_cid:', game_cid, ' player_cid:', player_cid, ' role_id:', player_info.role_id);

	this.game_cid = game_cid;
	this.player_cid = player_cid;
	this.player_info = player_info;
	game_cid_public_player_map.set(util.get_cid(game_cid, player_cid), this);
	role_id_public_player_map.set(this.player_info.role_id, this);
	role_name_public_player_map.set(this.player_info.role_name, this);
	
	rank_manager.update_rank(Rank_Type.LEVEL_RANK, this);
}
	
//玩家离线，保存数据
Public_Player.prototype.save_player_data = function() {
	print('***************public_player save_data, role_id:', this.player_info.role_id, " role_name:", this.player_info.role_name);
	gate_cid_public_player_map.delete(util.get_cid(this.gate_cid, this.player_cid));
	game_cid_public_player_map.delete(util.get_cid(this.game_cid, this.player_cid));
	role_id_public_player_map.delete(this.player_info.role_id);
	role_name_public_player_map.delete(this.player_info.role_name);
}

Public_Player.prototype.tick = function(now) {
}

Public_Player.prototype.send_success_msg = function(msg_id, msg) {
	send_msg(Endpoint.PUBLIC_GATE_SERVER, this.gate_cid, msg_id, Msg_Type.NODE_S2C, this.player_cid, msg);
}

Public_Player.prototype.send_error_msg = function(error_code) {
	var msg = new s2c_4();
	msg.error_code = error_code;
	send_msg(Endpoint.PUBLIC_GATE_SERVER, this.gate_cid, Msg.RES_ERROR_CODE, Msg_Type.NODE_S2C, this.player_cid, msg);
}