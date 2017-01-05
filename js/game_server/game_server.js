/*
*	描述：game_server脚本
*	作者：张亚磊
*	时间：2016/02/24
*/

require("js/game_server/game_player.js");
require("js/game_server/login.js");
require("js/game_server/bag.js");
require("js/game_server/mail.js");
require("js/game_server/entity.js");

function init(node_info) {
    log_info('game_server init, node_type:', node_info.node_type, ' node_id:', node_info.node_id, ' node_name:', node_info.node_name);
    global.node_info = node_info;
	global.timer.init();

    for(var i = 0; i < node_info.endpoint_list.length; ++i) {
        if (node_info.endpoint_list[i].endpoint_type == Endpoint_Type.CONNECTOR
            && node_info.endpoint_list[i].endpoint_name != "game_data_connector"
    		&& node_info.endpoint_list[i].endpoint_name != "game_log_connector") {
    	    util.sync_node_info(node_info.endpoint_list[i].endpoint_id);
    	}
    }

	//create_aoi_manager(1001);
}

function on_hotupdate(file_path) { }

function on_drop_eid(eid) {
    for (var i = 0; i < global.node_info.endpoint_list.length; ++i) {
        if (global.node_info.endpoint_list[i].endpoint_id == eid
    		&& global.node_info.endpoint_list[i].endpoint_name != "game_data_connector"
            && global.node_info.endpoint_list[i].endpoint_name != "game_log_connector") {
            util.sync_node_info(eid);
            break;
        }
    }
}

function on_drop_cid(cid) { }

function on_msg(msg) {
	if (msg.msg_type == Msg_Type.NODE_C2S) {
		if (msg.msg_id >= Msg.REQ_ROLE_LIST && msg.msg_id <= Msg.REQ_DEL_ROLE) {
			process_game_login_msg(msg);
		}
		else {
			process_game_client_msg(msg);	
		}
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
	var login_data = new Login();
	login_data.gate_eid = gate_nid % 10000 + 4;
	global.sid_login_map.set(sid, login_data);
    //增加session时候通知gate，gate可以通知client成功建立session
	send_msg(login_data.gate_eid, 0, Msg.SYNC_GATE_GAME_ADD_SESSION, Msg_Type.NODE_MSG, sid, {});
}

function on_remove_session(sid, error_code) {
    log_info('game remove_session, sid:', sid, ' error_code:', error_code);
	var gate_eid = 0;
	var game_player = global.sid_game_player_map.get(sid);
	if (game_player) {
		gate_eid = game_player.gate_eid;
	} else {
		var login_data = global.sid_login_map.get(sid);
	    gate_eid = login_data.gate_eid;
	    global.sid_login_map.delete(sid);
	}
	//移除session时候通知gate清理session
	var msg = new Object();
	msg.error_code = error_code;
	send_msg(gate_eid, 0, Msg.SYNC_GAME_GATE_LOGOUT, Msg_Type.NODE_MSG, sid, msg);
}

function process_game_login_msg(msg) {
	var login_data = global.sid_login_map.get(msg.sid);
	if (!login_data) {
	    log_error('process_game_login_msg login_data not exist sid:', msg.sid, ' gate_cid:', msg.cid, ' msg_id:', msg.msg_id);
	    on_remove_session(msg.sid, Error_Code.DISCONNECT_NOLOGIN);
	    return;
	}

	switch(msg.msg_id) {
		case Msg.REQ_ROLE_LIST:
			login_data.fetch_role_list(msg);
			break;
	    case Msg.REQ_ENTER_GAME:
		    login_data.enter_game(msg);
		    break;
	    case Msg.REQ_CREATE_ROLE:
		    login_data.create_role(msg);
		    break;
	    case Msg.RES_DEL_ROLE:
		    login_data.delete_role(msg);
		    break;
	    default:
		    break;
	}
}

function process_game_client_msg(msg) {
	var game_player = global.sid_game_player_map.get(msg.sid);
	if (!game_player) {
	    log_error('process_game_client_msg game_player not exist sid:', msg.sid, ' gate_cid:', msg.cid, ' msg_id:', msg.msg_id);
	    on_remove_session(msg.sid, Error_Code.ROLE_NOT_EXIST);
	    return;
	}
	
	switch(msg.msg_id) {
		case Msg.REQ_ROLE_INFO:
			game_player.fetch_role_info(msg);
			break;
	    case Msg.REQ_MAIL_INFO:
		    game_player.mail.fetch_mail_info();
		    break;
	    case Msg.REQ_PICKUP_MAIL:
		    game_player.mail.pickup_mail(msg);
		    break;
	    case Msg.REQ_DEL_MAIL:
		    game_player.mail.delete_mail(msg);
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
	        var login_data = global.sid_login_map.get(msg.sid);
	        game_player.login(login_data.gate_eid, msg.sid, msg.player_data);
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
        	switch(msg.ret_code) {
        		case DB_Ret_Code.OPT_FAIL: {
        			on_remove_session(msg.sid, Error_Code.PLAYER_DATA_ERROR);
        			break;
        		}
        		case DB_Ret_Code.DATA_NOT_EXIST: {
        			if (msg.struct_name == "Account_Info") {
        				var login_data = global.sid_login_map.get(msg.sid);
        				login_data.account_info.role_list = new Array();

        				var msg_res = new Object();
						msg_res.role_list = login_data.account_info.role_list;
						send_msg(login_data.gate_eid, 0 , Msg.RES_ROLE_LIST, Msg_Type.NODE_S2C, msg.sid, msg_res);
        			} 
        			else if (msg.struct_name == "Role_Info") {
        				//角色名称不存在，就创建角色
        				var msg_res = new Object();
						msg_res.type = "role_id";
						send_msg_to_db(Msg.SYNC_GENERATE_ID, msg.sid, msg_res);
        			}
        			break;
        		}
        	}
            break;
        }
        case Msg.SYNC_SAVE_DB_DATA: {
            if (msg.struct_name == "Player_Data") {
                global.sid_logout_map.delete(msg.sid);
            }
            break;
        }
        default:
            break;
	}
}

function res_select_db_data(msg) {
    switch(msg.data_type) {
        case Select_Data_Type.ACCOUNT: {
        	log_warn("res_select_db_data, account:", msg.account_info.account);
        	//返回玩家角色列表
        	var login_data = global.sid_login_map.get(msg.sid);
        	login_data.account_info = msg.account_info;

    	    var msg_res = new Object();
			msg_res.role_list = login_data.account_info.role_list;
			send_msg(login_data.gate_eid, 0 , Msg.RES_ROLE_LIST, Msg_Type.NODE_S2C, msg.sid, msg_res);
        	break;
        }
		case Select_Data_Type.INT64: {
			//创建角色时候，根据role_name查询，返回正常数据，说明role_name已经存在
			if(msg.query_name == "role_id" && msg.query_value > 0) {
				var msg_res = new Object();
				msg_res.error_code = Error_Code.ROLE_HAS_EXIST;
				var login_data = global.sid_login_map.get(msg.sid);
				send_msg(login_data.gate_eid, 0 , Msg.RES_ERROR_CODE, Msg_Type.NODE_S2C, msg.sid, msg_res);
			}
        	break;
        }
        default:
            break;
    }
}

function res_generate_id(msg) {
    if (msg.id <= 0) {
        on_remove_session(msg.sid, Error_Code.GENERATE_ID_ERROR);
    } else {
    	var login_data = global.sid_login_map.get(msg.sid);
    	login_data.db_create_role(msg);
	}
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
