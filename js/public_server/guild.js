/*
*	描述：公会系统
*	作者：张亚磊
*	时间：2016/09/22
*/

function Guild() {
	this.guild_map = new Map();
	this.drop_list = new Array();
	this.is_change = false;
}

Guild.prototype.load_data = function(msg) {
	log_info('load guild data, size:', msg.guild_list.length);	
	for(var i = 0; i < msg.guild_list.length; i++) {
		var guild_info = msg.guild_list[i];
		log_info('guild_id:', guild_info.guild_id, ' guild_name:', guild_info.guild_name);
		this.guild_map.set(guild_info.guild_id, guild_info);
	}
}

Guild.prototype.save_data = function(){
	var msg = new node_206();
	msg.data_type = Public_Data_Type.GUILD_DATA;
	for (var value of this.guild_map.values()) {
		msg.guild_list.push(value);
	}
	send_msg_to_db(Msg.NODE_PUBLIC_DB_SAVE_DATA, 0, msg);
	this.is_change = false;
}

Guild.prototype.drop_guild = function(){
	if (this.drop_list.length <= 0) return;
		
	var msg = new node_207();
	msg.data_type = Public_Data_Type.GUILD_DATA;
	msg.index_list = this.drop_list;
	send_msg_to_db(Msg.NODE_PUBLIC_DB_DELETE_DATA, 0, msg);
	this.drop_list = [];
}

Guild.prototype.save_data_handler = function() {
	if (!this.is_change) return;
	this.save_data();
	this.drop_guild();
	this.is_change = false;
}

Guild.prototype.sync_guild_info_to_game = function(player, guild_id, guild_name){
	var msg = new node_209();
	msg.role_id = player.player_info.role_id;
	msg.guild_id = guild_id;
	msg.guild_name = guild_name;
	send_public_msg(player.game_cid, Msg.NODE_PUBLIC_GAME_GUILD_INFO, player.sid, msg);
}

Guild.prototype.member_join_guild = function(player, guild_info) {
	var member_detail = new Guild_Member_Detail();
	member_detail.role_id = player.player_info.role_id;
	member_detail.role_name = player.player_info.role_name;
	member_detail.level = player.player_info.level;
	guild_info.member_list.push(member_detail);
	player.player_info.guild_id = guild_info.guild_id;
	player.player_info.guild_name = guild_info.guild_name;
	this.is_change = true;
}

Guild.prototype.create_guild = function(player, msg) {
	log_debug('create_guild, guild_name:', msg.guild_name, ' chief_id:', player.player_info.role_id);
	var msg_res = new node_204();
	msg_res.guild_name = msg.guild_name;
	msg_res.chief_id = player.player_info.role_id;
	send_msg_to_db(Msg.NODE_PUBLIC_DB_CREATE_GUILD, msg.sid, msg_res);
}

Guild.prototype.db_create_guild = function(msg) {
	var player = sid_public_player_map.get(msg.sid);
	if (!player || msg.guild_list.length != 1) {
		log_error('db_create_guild param error');
		return;
	}

	var msg_res = new s2c_201();
	msg_res.guild_info = msg.guild_list[0];
	log_debug('db_create_guild, guild_id:', msg_res.guild_info.guild_id, ' guild_name:', msg_res.guild_info.guild_name, ' chief_id:', msg_res.guild_info.chief_id);
	this.member_join_guild(player, msg_res.guild_info);
	this.guild_map.set(msg_res.guild_info.guild_id, msg_res.guild_info);
	this.sync_guild_info_to_game(player, msg_res.guild_info.guild_id, msg_res.guild_info.guild_name);
	player.send_success_msg(Msg.RES_CREATE_GUILD, msg_res);
}

Guild.prototype.dissove_guild = function(player, msg) {
	log_debug('dissove_guild, role_id:', player.player_info.role_id, ' role_name:', player.player_info.role_name);
	var guild_info = this.guild_map.get(player.player_info.guild_id);
	if(guild_info == null){
		return player.send_error_msg(Error_Code.GUILD_NOT_EXIST);
	}
	for(var i = 0; i < guild_info.member_list.length; i++){
		var mem_player = role_id_public_player_map.get(guild_info.member_list[i].role_id);
		if(mem_player == null){
			//离线数据，保存到离线数据列表
			offline_manager.set_offline_detail(guild_info.member_list[i].role_id, guild_info.guild_id, guild_info.guild_name);
		} else {
			this.sync_guild_info_to_game(mem_player, 0, "");
		}
	}
	this.guild_map.delete(guild_info.guild_id);
	this.drop_list.push(guild_info.guild_id);
	
	var msg_res = new s2c_202();
	msg_res.guild_id = msg.guild_id;
	player.send_success_msg(Msg.RES_DISSOVE_GUILD, msg_res);
	this.is_change = true;
}
