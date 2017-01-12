/*
*	描述：公会系统
*	作者：张亚磊
*	时间：2016/09/22
*/

function Guild() {
    this.guild_info = null;
    this.is_change = false;
}

function Guild_Manager() {
	this.guild_map = new Map();
	this.delete_list = new Array();
}

Guild_Manager.prototype.load_data = function(msg) {
	log_info('load guild data, size:', msg.guild_list.length);	
	for(var i = 0; i < msg.guild_list.length; i++) {
	    var guild = new Guild();
	    guild.guild_info = msg.guild_list[i];
	    this.guild_map.set(guild.guild_info.guild_id, guild);
	}
}

Guild_Manager.prototype.save_data = function(){
	var msg = new Object();
	msg.save_type = Save_Type.SAVE_DB_AND_CACHE;
	msg.vector_data = true;
	msg.db_id = DB_Id.GAME;
	msg.struct_name = "Guild_Info";
	msg.data_type = DB_Data_Type.GUILD;
	msg.guild_list = new Array();
	this.guild_map.forEach(function(value, key, map) {
        if(value.is_change) {
            value.is_change = false;
            msg.guild_list.push(value.guild_info);
	    }
    });
	send_msg_to_db(Msg.SYNC_SAVE_DB_DATA, 0, msg);
}

Guild_Manager.prototype.delete_guild = function(){
    if (this.delete_list.length <= 0) 
        return;
		
	var msg = new Object();
	msg.db_id = DB_Id.GAME;
	msg.struct_name = "Guild_Info";
	msg.index_list = this.delete_list;
	send_msg_to_db(Msg.SYNC_PUBLIC_DB_DELETE_DATA, 0, msg);
	this.delete_list.length = 0;
}

Guild_Manager.prototype.save_data_handler = function() {
	this.save_data();
	this.delete_guild();
}

Guild_Manager.prototype.sync_guild_info_to_game = function(player, guild_id, guild_name){
	var msg = new Object();
	msg.role_id = player.role_info.role_id;
	msg.guild_id = guild_id;
	msg.guild_name = guild_name;
	send_public_msg(player.game_cid, Msg.SYNC_PUBLIC_GAME_GUILD_INFO, player.sid, msg);
}

Guild_Manager.prototype.member_join_guild = function(player, guild) {
    player.role_info.guild_id = guild.guild_info.guild_id;
    player.role_info.guild_name = guild.guild_info.guild_name;

	var member_detail = new Object();
	member_detail.role_id = player.role_info.role_id;
	member_detail.role_name = player.role_info.role_name;
	member_detail.level = player.role_info.level;
	guild.guild_info.member_list = new Array();
	guild.guild_info.member_list.push(member_detail);
	guild.is_change = true;
}

Guild_Manager.prototype.create_guild = function(player, msg) {
	//将公会名字暂时存到帮主player信息里面	
    player.role_info.guild_name = msg.guild_name;

	var msg_res = new Object();
	msg_res.db_id = DB_Id.GAME;
	msg_res.struct_name = "Guild_Info";
	msg_res.condition_name = "guild_name";
	msg_res.condition_value = msg.guild_name;
	msg_res.query_name = "guild_id";
	msg_res.query_type = "int64";
	msg_res.data_type = Select_Data_Type.INT64;
	send_msg_to_db(Msg.SYNC_SELECT_DB_DATA, msg.sid, msg_res);
}

Guild_Manager.prototype.db_create_guild = function(player, guild_info) {
	log_info('db_create_guild, guild_id:', guild_info.guild_id, ' guild_name:', guild_info.guild_name, ' chief_id:', guild_info.chief_id);

	var guild = new Guild();
	guild.guild_info = guild_info;
	this.member_join_guild(player, guild);
	this.guild_map.set(guild_info.guild_id, guild);
	this.sync_guild_info_to_game(player, guild_info.guild_id, guild_info.guild_name);
	
	var msg_res = new Object();
	msg_res.guild_info = guild_info;
	player.send_msg(Msg.RES_CREATE_GUILD, msg_res);
}

Guild_Manager.prototype.dissove_guild = function(player, msg) {
	var guild = this.guild_map.get(player.role_info.guild_id);
	if(guild == null){
		return player.send_error_msg(Error_Code.GUILD_NOT_EXIST);
	}

	log_debug('dissove_guild, guild_id:', player.role_info.guild_id, ' role_id:', player.role_info.role_id);
	for(var i = 0; i < guild.guild_info.member_list.length; i++) {
	    var mem_player = global.role_id_public_player_map.get(guild.guild_info.member_list[i].role_id);
		if(mem_player != null){
		    this.sync_guild_info_to_game(mem_player, 0, "");
		}
	}
	this.guild_map.delete(player.role_info.guild_id);
	this.delete_list.push(player.role_info.guild_id);
	
	var msg_res = new Object();
	msg_res.guild_id = player.role_info.guild_id;
	player.send_msg(Msg.RES_DISSOVE_GUILD, msg_res);
}
