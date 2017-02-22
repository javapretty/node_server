/*
*	描述：登录模块
*	作者：张亚磊
*	时间：2017/01/05
*/

function Login() {
	this.gate_eid = 0;          		//玩家连接的gate端点id
	this.sid = 0;               		//玩家sid
	this.account_info = new Object();	//玩家账号信息
	this.create_role_info = new Object();//创建角色信息
}

Login.prototype.fetch_role_list = function(msg) {
	this.account_info.account = msg.account;

	var msg_res = new Object();
	msg_res.db_id = DB_Id.GAME;
	msg_res.struct_name = "Account_Info";
	msg_res.condition_name = "account";
	msg_res.condition_value = msg.account;
	msg_res.query_name = "";
	msg_res.query_type = "";
	msg_res.data_type = Select_Data_Type.ACCOUNT;
	send_msg_to_db(Msg.SYNC_SELECT_DB_DATA, msg.sid, msg_res);
}

Login.prototype.enter_game = function(msg) {
	if (global.sid_game_player_map.get(msg.sid) || global.sid_logout_map.get(msg.sid)) {
		log_error('relogin account:', msg.account);
		return on_remove_session(msg.sid, Error_Code.DISCONNECT_RELOGIN);
	}

    var msg_res = new Object();
    msg_res.db_id = DB_Id.GAME;
    msg_res.key_index = msg.role_id;
    msg_res.struct_name = "Player_Data";
    msg_res.data_type = DB_Data_Type.PLAYER;
    send_msg_to_db(Msg.SYNC_LOAD_DB_DATA, msg.sid, msg_res);
}

Login.prototype.create_role = function(msg) {
    if (msg.role_info.account.length <= 0 || msg.role_info.role_name.length <= 0 || global.sid_logout_map.get(msg.sid)) {
		log_error('create_role parameter invalid, account:', msg.role_info.account, ' role_name:', msg.role_info.role_name);
		return on_remove_session(msg.sid, Error_Code.CLIENT_PARAM_ERROR);
	}
	
    log_info('create_role, account:', msg.role_info.account, ' gate_cid:', msg.cid, ' sid:', msg.sid);
	this.create_role_info = msg.role_info;
	//检查角色名是否存在
	var msg_res = new Object();
	msg_res.db_id = DB_Id.GAME;
	msg_res.struct_name = "Role_Info";
	msg_res.condition_name = "role_name";
	msg_res.condition_value = msg.role_info.role_name;
	msg_res.query_name = "role_id";
	msg_res.query_type = "int64";
	msg_res.data_type = Select_Data_Type.INT64;
	send_msg_to_db(Msg.SYNC_SELECT_DB_DATA, msg.sid, msg_res);
}

Login.prototype.delete_role = function(msg) {

}

Login.prototype.db_create_role = function(msg) {
	//保存account信息
	var account_role_info = new Object();
	account_role_info.role_id = msg.id;
	account_role_info.role_name = this.create_role_info.role_name;
	account_role_info.gender = this.create_role_info.gender;
	account_role_info.career = this.create_role_info.career;
	account_role_info.level = 1;
	account_role_info.combat = 100;
	this.account_info.role_list.push(account_role_info);
	var msg_res = new Object();
	msg_res.save_type = Save_Type.SAVE_DB_AND_CACHE;
	msg_res.vector_data = false;
	msg_res.db_id = DB_Id.GAME;
	msg_res.struct_name = "Account_Info";
	msg_res.data_type = DB_Data_Type.ACCOUNT;
	msg_res.account_info = this.account_info;
	send_msg_to_db(Msg.SYNC_SAVE_DB_DATA, msg.sid, msg_res);

	//保存角色信息
	msg_res.struct_name = "Player_Data";
	msg_res.data_type = DB_Data_Type.PLAYER;
	//初始化玩家数据
	msg_res.player_data = new Player_Data();
	msg_res.player_data.role_info.role_id = msg.id;
	msg_res.player_data.role_info.account = this.create_role_info.account;
	msg_res.player_data.role_info.role_name = this.create_role_info.role_name;
	msg_res.player_data.role_info.gender = this.create_role_info.gender;
	msg_res.player_data.role_info.career = this.create_role_info.career;
	msg_res.player_data.role_info.level = 1;
	msg_res.player_data.role_info.exp = 0;
	msg_res.player_data.role_info.combat = 100;
	msg_res.player_data.role_info.create_time = util.now_sec();
	msg_res.player_data.role_info.login_time = msg_res.player_data.role_info.create_time;
	msg_res.player_data.role_info.speed = 120;
	msg_res.player_data.role_info.last_scene = 1001;
	//初始化背包数据
	msg_res.player_data.activity_info.role_id = msg.id;
	for(var i = 0; i < 7; ++i) {
		msg_res.player_data.activity_info.seven_day_award_status.push(false);
	}
	for(var i = 0; i < 31; ++i) {
		msg_res.player_data.activity_info.sign_in_award_status.push(false);
	}
	msg_res.player_data.bag_info.role_id = msg.id;
	msg_res.player_data.mail_info.role_id = msg.id;
	send_msg_to_db(Msg.SYNC_SAVE_DB_DATA, msg.sid, msg_res);

	//创建角色时候，玩家成功登陆游戏
	var game_player = new Game_Player();
	game_player.login(this.gate_eid, msg.sid, msg_res.player_data);
}