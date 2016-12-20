/*
*	描述：center_server脚本
*	作者：张亚磊
*	时间：2016/09/22
*/

function init(node_info) {
    log_info('center_server init, node_type:', node_info.node_type, ' node_id:', node_info.node_id, ' node_name:', node_info.node_name);
    global.node_info = node_info;
    global.timer.init();
}

function on_hotupdate(file_path) { }

function on_drop(cid) { }

function on_msg(msg) {
	if (msg.msg_type == Msg_Type.TCP_C2S) {
		switch(msg.msg_id) {
		    case Msg.REQ_SELECT_GATE:
			    select_gate(msg);
			    break;	
		    default:
			    log_error('center_server process client msg, msg_id not exist:', msg.msg_id);
			    break;
		}
	} 
	else if (msg.msg_type == Msg_Type.NODE_MSG) {
		switch(msg.msg_id) {
		    case Msg.SYNC_NODE_INFO:
			    set_node_info(msg);
			    break;		
		    case Msg.SYNC_GATE_CENTER_VERIFY_TOKEN:
			    verify_token(msg);
			    break;
		    case Msg.SYNC_GATE_CENTER_REMOVE_SESSION:
		        global.sid_set.delete(msg.sid);
			    break;
		    default:
			    log_error('center_server process node msg, msg_id not exist:', msg.msg_id);
			    break;
		}	
	}
	else if (msg.msg_type == Msg_Type.WS_C2S) {
		switch(msg.msg_id) {
			case Msg.REQ_TEST_WEBSOCKET:
				test_websocket(msg);
				break;
			default:
			    log_error('center_server process websocket msg, msg_id not exist:', msg.msg_id);
				break;
		}
	}
}

function on_tick(timer_id) {
    var timer_handler = global.timer.get_timer_handler(timer_id);
	if (timer_handler != null) {
		timer_handler();
	}
}

function on_close_session(account, cid, error_code) {	
    global.account_token_map.delete(account);
    if (error_code != Error_Code.RET_OK) {
        var msg = new Object();
        msg.error_code = error_code;
        send_msg(Endpoint.CENTER_CLIENT_SERVER, cid, Msg.RES_ERROR_CODE, Msg_Type.TCP_S2C, 0, msg);
    }
	//关闭客户端网络层链接
	close_client(Endpoint.CENTER_CLIENT_SERVER, cid);	
}

function select_gate(msg) {
    if (global.account_token_map.get(msg.account)) {
		log_error('account in center_server:', msg.account);
		return on_close_session(msg.account, msg.cid, Error_Code.DISCONNECT_RELOGIN);
	}

	var token_info = new Object();
	token_info.cid = msg.cid;
	token_info.token = generate_token(msg.account);
	token_info.token_time = util.now_sec;
	global.account_token_map.set(msg.account, token_info);
	
    //根据账号hash值选择gate
	var hash_value = hash(msg.account);
	var gate_len = global.gate_list.length;
	var index = hash_value % gate_len;
	var gate_info = global.gate_list[index];
	var msg_res = new Object();
	for (var i = 0; i < gate_info.endpoint_list.length; ++i) {
	    if (gate_info.endpoint_list[i].endpoint_gid == gate_info.endpoint_gid &&
            gate_info.endpoint_list[i].endpoint_id == Endpoint.GATE_CLIENT_SERVER) {
	        msg_res.gate_ip = gate_info.endpoint_list[i].server_ip;
	        msg_res.gate_port = gate_info.endpoint_list[i].server_port;
	        break;
	    }
	}
	msg_res.token = token_info.token;
	send_msg(Endpoint.CENTER_CLIENT_SERVER, msg.cid, Msg.RES_SELECT_GATE, Msg_Type.TCP_S2C, 0, msg_res);
}

function set_node_info(msg) {
    switch(msg.node_info.node_type) {
        case Node_Type.GATE_SERVER:
            global.gate_list.push(msg.node_info);
            break;
        case Node_Type.GAME_SERVER:
            global.game_list.push(msg.node_info);
            break;
        case Node_Type.MASTER_SERVER:
            global.master_list.push(msg.cid);
            break;
        default:
            break;
    }
}

function verify_token(msg) {
    var token_info = global.account_token_map.get(msg.account);
	if (!token_info || token_info.token != msg.token) {		
		log_error('verify_token, token error, account:', msg.account, ' token:', msg.token);
		if (token_info) {
			on_close_session(msg.account, token_info.cid, Error_Code.TOKEN_ERROR);
		}
		var msg_res = new Object();
		msg_res.error_code = Error_Code.TOKEN_ERROR;
		return send_msg(Endpoint.CENTER_NODE_SERVER, msg.cid, Msg.SYNC_NODE_CODE, Msg_Type.NODE_MSG, msg.sid, msg_res);
	}

	++global.sid_idx;
	if (global.sid_idx > 4294967295) {
	    global.sid_idx = 0;
	}
	global.sid_set.add(global.sid_idx);
	var hash_value = hash(msg.account);
	var game_len = global.game_list.length;
	var index = hash_value % game_len;
	var game_info = global.game_list[index];
	var msg_res = new Object();
	msg_res.account = msg.account;
	msg_res.token = msg.token;
	msg_res.client_cid = msg.client_cid;
	msg_res.game_nid = game_info.node_id;
	send_msg(Endpoint.CENTER_NODE_SERVER, msg.cid, Msg.SYNC_GATE_CENTER_VERIFY_TOKEN, Msg_Type.NODE_MSG, global.sid_idx, msg_res);
    //关闭session
	on_close_session(msg.account, token_info.cid, Error_Code.RET_OK);
}

function test_websocket(msg) {
	log_info("test_websocket str:" + msg.str + " int:" + msg.int);
	var res = new Object();
	res.str = "test_websocket res:";
	res.int = msg.int + 10;
    send_msg(Endpoint.CENTER_CLIENT_SERVER, msg.cid, Msg.RES_TEST_WEBSOCKET, Msg_Type.WS_S2C, 0, res);
}

