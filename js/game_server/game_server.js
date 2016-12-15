/*
*	描述：game_server脚本
*	作者：张亚磊
*	时间：2016/02/24
*/

require("js/game_server/game_player.js");
require("js/game_server/bag.js");
require("js/game_server/mail.js");

function init(node_info) {
    log_info('game_server init, node_type:', node_info.node_type, ' node_id:', node_info.node_id, ' node_name:', node_info.node_name);
    global.node_info = node_info;
	global.timer.init();

	var msg = new Object();
	msg.node_info = node_info;
	send_msg(Endpoint.GAME_MASTER_CONNECTOR, 0, Msg.SYNC_NODE_INFO, Msg_Type.NODE_MSG, 0, msg);
	send_msg(Endpoint.GAME_GATE1_CONNECTOR, 0, Msg.SYNC_NODE_INFO, Msg_Type.NODE_MSG, 0, msg);
	send_msg(Endpoint.GAME_GATE2_CONNECTOR, 0, Msg.SYNC_NODE_INFO, Msg_Type.NODE_MSG, 0, msg);	
}

function on_hotupdate(file_path) { }

function on_drop(cid) { }

function on_msg(msg) {
	if (msg.msg_type == Msg_Type.NODE_C2S) {
		process_game_client_msg(msg);
	} 
	else if (msg.msg_type == Msg_Type.NODE_S2C) {
	    var game_player = global.sid_game_player_map.get(msg.sid);
		if (game_player) {
			send_msg(game_player.gate_eid, 0 , msg.msg_id, msg.msg_type, msg.sid, msg);
		}
	} 
	else if (msg.msg_type == Msg_Type.NODE_MSG) {
		process_game_node_msg(msg);
	}
}

function on_tick(timer_id) {
    var timer_handler = global.timer.get_timer_handler(timer_id);
	if (timer_handler != null) {
		timer_handler();
	}
}

function send_msg_to_db(msg_id, sid, msg) {
	send_msg(Endpoint.GAME_DATA_CONNECTOR, 0, msg_id, Msg_Type.NODE_MSG, sid, msg);
}

function send_msg_to_log(msg_id, sid, msg) {
	send_msg(Endpoint.GAME_LOG_CONNECTOR, 0, msg_id, Msg_Type.NODE_MSG, sid, msg);
}

function send_msg_to_public(msg_id, sid, msg) {
	send_msg(Endpoint.GAME_PUBLIC_CONNECTOR, 0, msg_id, Msg_Type.NODE_MSG, sid, msg);
}

function on_add_session(sid, gate_nid) {
    log_info('game add_session, sid:', sid, ' gate_nid:', gate_nid);
	var gate_eid = gate_nid % 10000 + 4;
	global.sid_gate_eid_map.set(sid, gate_eid);
    //增加session时候通知gate，gate可以通知client成功建立session
	send_msg(gate_eid, 0, Msg.SYNC_GATE_GAME_ADD_SESSION, Msg_Type.NODE_MSG, sid, {});
}

function on_remove_session(sid, error_code) {
    log_info('game remove_session, sid:', sid, ' error_code:', error_code);
	var gate_eid = 0;
	var game_player = global.sid_game_player_map.get(sid);
	if (game_player) {
		gate_eid = game_player.gate_eid;
	} else {
	    gate_eid = global.sid_gate_eid_map.get(sid);
	    global.sid_gate_eid_map.delete(sid);
	}
	//移除session时候通知gate清理session
	var msg = new Object();
	msg.error_code = error_code;
	send_msg(gate_eid, 0, Msg.SYNC_GAME_GATE_LOGOUT, Msg_Type.NODE_MSG, sid, msg);
}

function process_game_client_msg(msg) {
	if (msg.msg_id == Msg.REQ_FETCH_ROLE) {
		return fetch_role(msg);
	} else if (msg.msg_id == Msg.REQ_CREATE_ROLE) { 
		return create_role(msg);
	}

	var game_player = global.sid_game_player_map.get(msg.sid);
	if (!game_player) {
	    log_error('process_game_client_msg game_player not exist sid:', msg.sid, ' gate_cid:', msg.cid, ' msg_id:', msg.msg_id);
	    on_remove_session(msg.sid, Error_Code.PLAYER_NOT_EXIST);
	    return;
	}
	
	switch(msg.msg_id) {
	    case Msg.REQ_FETCH_MAIL:
		    game_player.mail.fetch_mail();
		    break;
	    case Msg.REQ_PICKUP_MAIL:
		    game_player.mail.pickup_mail(msg);
		    break;
	    case Msg.REQ_DEL_MAIL:
		    game_player.mail.delete_mail(msg);
		    break;
	    case Msg.REQ_FETCH_BAG:
		    game_player.bag.fetch_bag();
		    break;
	    case Msg.REQ_TEST_ARG:
	        test_arg(msg, game_player);
	        break;
	    case Msg.REQ_TEST_SWITCH:
	        test_switch(msg, game_player);
	        break;
	    default:
		    send_msg(Endpoint.GAME_PUBLIC_CONNECTOR, 0, msg.msg_id, msg.msg_type, msg.sid, msg);
		    break;
	}
}

function process_game_node_msg(msg) {
	switch(msg.msg_id) {
	    case Msg.SYNC_GATE_GAME_ADD_SESSION:
		    on_add_session(msg.sid, msg.gate_nid);
		    break;
	    case Msg.SYNC_GAME_GATE_LOGOUT: {
	        var game_player = global.sid_game_player_map.get(msg.sid);
		    if (game_player) {
			    game_player.logout();
		    }
		    break;	
	    }
	    case Msg.SYNC_DB_RET_CODE:
	        process_db_ret_code(msg);
	        break;
	    case Msg.SYNC_RES_SELECT_DB_DATA:
	        res_select_db_data(msg);
	        break;
	    case Msg.SYNC_RES_GENERATE_ID:
	        res_generate_id(msg);
	        break;
	    case Msg.SYNC_SAVE_DB_DATA: {
            //加载数据成功，登陆游戏
	        var game_player = new Game_Player();
	        var gate_eid = global.sid_gate_eid_map.get(msg.sid);
	        game_player.login(gate_eid, msg.sid, msg.player_data);
		    break;	
	    }
	    case Msg.SYNC_PUBLIC_GAME_GUILD_INFO: {
	        var game_player = global.role_id_game_player_map.get(msg.role_id);
		    if (game_player) {
			    game_player.set_guild_info(msg);
		    }
		    break;
	    }
	    default:
		    log_error('process_game_node_msg, msg_id not exist:', msg.msg_id);
		    break;
	}
}

function process_db_ret_code(msg) {
    switch (msg.opt_msg_id) {
        case Msg.SYNC_SELECT_DB_DATA: {
            if (msg.ret_code == DB_Ret_Code.DATA_NOT_EXIST && msg.query_name == "role_id") {
                var gate_eid = global.sid_gate_eid_map.get(msg.sid);
                var msg_res = new Object();
                msg_res.error_code = Error_Code.NEED_CREATE_ROLE;
                send_msg(gate_eid, 0, Msg.RES_ERROR_CODE, Msg_Type.NODE_S2C, msg.sid, msg_res);
            }
            else if (msg.ret_code == DB_Ret_Code.OPT_FAIL) {
                on_remove_session(msg.sid, Error_Code.PLAYER_DATA_ERROR);
            }
            break;
        }
        case Msg.SYNC_SAVE_DB_DATA: {
            if (msg.struct_name == "Player_Data") {
                global.logout_map.delete(msg.sid);
            }
            break;
        }
        default:
            break;
	}
}

function fetch_role(msg) {
    if (global.sid_game_player_map.get(msg.sid) || global.logout_map.get(msg.sid)) {
		log_error('relogin account:', msg.account);
		return on_remove_session(msg.sid, Error_Code.DISCONNECT_RELOGIN);
	}

	var msg_res = new Object();
	msg_res.db_id = DB_Id.GAME;
	msg_res.struct_name = "Role_Info";
	msg_res.condition_name = "account";
	msg_res.condition_value = msg.account;
	msg_res.query_name = "role_id";
	msg_res.query_type = "int64";
	msg_res.data_type = Select_Data_Type.INT64;
	send_msg_to_db(Msg.SYNC_SELECT_DB_DATA, msg.sid, msg_res);
}

function res_select_db_data(msg) {
    switch(msg.data_type) {
        case Select_Data_Type.INT64: {
            if (msg.query_name == "role_id" && msg.query_value > 0) {
                log_info('query role_id success sid:', msg.sid, ' role_id:', msg.query_value);
                var msg_res = new Object();
                msg_res.db_id = DB_Id.GAME;
                msg_res.key_index = msg.query_value;
                msg_res.struct_name = "Player_Data";
                msg_res.data_type = DB_Data_Type.PLAYER;
                send_msg_to_db(Msg.SYNC_LOAD_DB_DATA, msg.sid, msg_res);
            }
            break;
        }
        default:
            break;
    }
}

function create_role(msg) {
    if (msg.role_info.account.length <= 0 || msg.role_info.role_name.length <= 0 || global.logout_map.get(msg.sid)) {
		log_error('create_role parameter invalid, account:', msg.role_info.account, ' role_name:', msg.role_info.role_name);
		return on_remove_session(msg.sid, Error_Code.CLIENT_PARAM_ERROR);
	}
	
    log_info('create_role, account:', msg.role_info.account, ' gate_cid:', msg.cid, ' sid:', msg.sid);
	//将创建角色信息保存起来，等待从db生成role_id后，将角色信息保存到db
	global.sid_create_role_map.set(msg.sid, msg.role_info);
	var msg_res = new Object();
	msg_res.type = "role_id";
	send_msg_to_db(Msg.SYNC_GENERATE_ID, msg.sid, msg_res);
}

function res_generate_id(msg) {
    if (msg.id <= 0) {
        on_remove_session(msg.sid, Error_Code.GENERATE_ID_ERROR);
    } else {
		//创建角色时候，既保存到缓存，又保存到db
		var msg_res = new Object();
		msg_res.save_type = Save_Type.SAVE_DB_AND_CACHE;
		msg_res.vector_data = false;
		msg_res.db_id = DB_Id.GAME;
		msg_res.struct_name = "Player_Data";
		msg_res.data_type = DB_Data_Type.PLAYER;
		//初始化玩家数据
		var role_info = global.sid_create_role_map.get(msg.sid);
		msg_res.player_data = new Object();
		msg_res.player_data.role_info = new Object();
		msg_res.player_data.role_info.role_id = msg.id;
		msg_res.player_data.role_info.account = role_info.account;
		msg_res.player_data.role_info.role_name = role_info.role_name;
		msg_res.player_data.role_info.level = 1;
		msg_res.player_data.role_info.exp = 0;
		msg_res.player_data.role_info.gender = role_info.gender;
		msg_res.player_data.role_info.career = role_info.career;
		msg_res.player_data.role_info.create_time = util.now_sec();
		msg_res.player_data.role_info.login_time = msg_res.player_data.role_info.create_time;
		msg_res.player_data.role_info.logout_time = 0;
		msg_res.player_data.role_info.guild_id = 0;
		msg_res.player_data.role_info.guild_name = "";
		//初始化背包数据
		msg_res.player_data.bag_info = new Object();
		msg_res.player_data.bag_info.role_id = msg.id;
		msg_res.player_data.bag_info.copper = 0;
		msg_res.player_data.bag_info.gold = 0;
		msg_res.player_data.bag_info.item_map = new Map();
		//初始化邮件数据
		msg_res.player_data.mail_info = new Object();
		msg_res.player_data.mail_info.role_id = msg.id;
		msg_res.player_data.mail_info.total_count = 0;
		msg_res.player_data.mail_info.mail_map = new Map();
		send_msg_to_db(Msg.SYNC_SAVE_DB_DATA, this.sid, msg_res);
		
		//创建角色时候，玩家成功登陆游戏
		var game_player = new Game_Player();
		var gate_eid = global.sid_gate_eid_map.get(msg.sid);
		game_player.login(gate_eid, msg.sid, msg_res.player_data);
	}
}

function test_arg(msg, player) {
	var msg_res = new Object();
    msg_res.int4_arg = msg.int4_arg;
    msg_res.uint8_arg = msg.uint8_arg;
    msg_res.uint4_arg = msg.uint4_arg;
    player.send_success_msg(Msg.RES_TEST_ARG, msg_res);
}

function test_switch(msg, player) {
	var msg_res = new Object();
    if (msg.int32_arg != null) {
        msg_res.int32_arg = msg.int32_arg;
    }
    msg_res.type = msg.type;
    switch (msg.type) {
        case 1: {
            msg_res.int64_vec = new Array();
            for (var i = 0; i < 2; ++i) {
                msg_res.int64_vec.push(msg.int64_arg + i);
            }
            break;
        }
        case 2: {
			msg_res.string_vec = new Array();
            for (var i = 0; i < 2; ++i) {
                msg_res.string_vec.push(msg.string_arg + "_" + i);
            }
            break;
        }
    }
    player.send_success_msg(Msg.RES_TEST_SWITCH, msg_res);
}