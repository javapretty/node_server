/*
*	描述：gate_server脚本
*	作者：张亚磊
*	时间：2016/09/22
*/

require("js/gate_server/session.js");

function init(node_info) {
	log_info('gate_server init, node_type:',node_info.node_type,' node_id:',node_info.node_id,' node_name:',node_info.node_name);
	global.node_info = node_info;
	global.timer.init();

    var msg = new Object();
    msg.node_info = node_info;
    for(var i = 0; i < node_info.endpoint_list.length; ++i) {
    	if(node_info.endpoint_list[i].endpoint_type == Endpoint_Type.CONNECTOR) {
    		send_msg(node_info.endpoint_list[i].endpoint_id, 0, Msg.SYNC_NODE_INFO, Msg_Type.NODE_MSG, 0, msg);		
    	}
    }
}

function on_hotupdate(file_path) { }

function on_drop(cid) {
    var session = global.cid_session_map.get(cid);
	if (session) {		
		on_remove_session(session);
		//删除C++层session
		remove_session(cid);
	}
}

function on_msg(msg) {
    //log_debug("gate on msg, msg_id:", msg.msg_id, " msg_type:", msg.msg_type, " cid:", msg.cid, " sid:", msg.sid);
	if (msg.msg_type == Msg_Type.TCP_C2S) {
		process_gate_client_msg(msg);
	} else if (msg.msg_type == Msg_Type.NODE_MSG) {
		process_gate_node_msg(msg);
	}
}

function on_tick(timer_id) {
    var timer_handler = global.timer.get_timer_handler(timer_id);
    if (timer_handler != null) {
        timer_handler();
    }
}

function on_add_session(session) {
    log_info('gate add_session, sid:', session.sid, ' cid:', session.client_cid, " account:", session.account);
    global.cid_session_map.set(session.client_cid, session);
    global.sid_session_map.set(session.sid, session);
    global.account_session_map.set(session.account, session);
	//将session添加到C++层
	add_session(session);

	//通知game增加session
	var msg = new Object();
	msg.gate_nid = global.node_info.node_id;
	send_msg(session.game_eid, session.game_cid, Msg.SYNC_GATE_GAME_ADD_SESSION, Msg_Type.NODE_MSG, session.sid, msg);
}

function on_remove_session(session) {
    log_info('gate remove_session, sid:', session.sid, ' cid:', session.client_cid, " account:", session.account);
    //通知game移除session
    send_msg(session.game_eid, session.game_cid, Msg.SYNC_GAME_GATE_LOGOUT, Msg_Type.NODE_MSG, session.sid, {});
	//通知center移除session
	send_msg(Endpoint.GATE_CENTER_CONNECTOR, 0, Msg.SYNC_GATE_CENTER_REMOVE_SESSION, Msg_Type.NODE_MSG, session.sid, {});

	global.cid_session_map.delete(session.client_cid);
	global.sid_session_map.delete(session.sid);
	global.account_session_map.delete(session.account);
}

function on_close_session(cid, error_code) {
    log_info('gate close_session, cid:', cid, ' error_code:', error_code);
	var msg = new Object();
	msg.error_code = error_code;
	send_msg(Endpoint.GATE_CLIENT_SERVER, cid, Msg.RES_ERROR_CODE, Msg_Type.TCP_S2C, 0, msg);
	//删除C++层session
	remove_session(cid);
	//关闭客户端网络层链接
	close_client(Endpoint.GATE_CLIENT_SERVER, cid);	
}

function process_gate_client_msg(msg) {
	switch(msg.msg_id) {
	    case Msg.REQ_HEARTBEAT: {
	        var session = global.cid_session_map.get(msg.cid);
		    if (session) {
			    session.on_heartbeat(msg);
		    } else {
			    on_close_session(msg.cid, Error_Code.DISCONNECT_NOLOGIN);
		    }
		    break;
	    }
	    case Msg.REQ_CONNECT_GATE:
		    connect_gate(msg);
		    break;	
	    default:
		    log_error('process_gate_client_msg, msg_id not exist:', msg.msg_id);
		    break;
	}
}

function process_gate_node_msg(msg) {
	switch(msg.msg_id) {
	    case Msg.SYNC_NODE_CODE:
		    process_node_code(msg);
		    break;
	    case Msg.SYNC_NODE_INFO:
		    set_node_info(msg);
		    break;
	    case Msg.SYNC_GATE_CENTER_VERIFY_TOKEN:
		    verify_token(msg);
		    break;
	    case Msg.SYNC_GATE_GAME_ADD_SESSION: {
	        //通知client成功建立session
	        var session = global.sid_session_map.get(msg.sid);
	        send_msg(session.client_eid, session.client_cid, Msg.RES_CONNECT_GATE, Msg_Type.TCP_S2C, 0, {});
	        break;
	    }
	    case Msg.SYNC_GAME_GATE_LOGOUT:
		    game_logout(msg);
		    break;
	    default:
		    log_error('process_gate_node_msg, msg_id not exist:', msg.msg_id);
		    break;
	}
}

function connect_gate(msg) {
	if (global.account_session_map.get(msg.account)) {
		log_error('account in gate server, ', msg.account);
		return on_close_session(msg.cid, Error_Code.DISCONNECT_RELOGIN);	
	}
	
	var msg_res = new Object();
	msg_res.account = msg.account;
	msg_res.token = msg.token;
	msg_res.client_cid = msg.cid;
	send_msg(Endpoint.GATE_CENTER_CONNECTOR, 0, Msg.SYNC_GATE_CENTER_VERIFY_TOKEN, Msg_Type.NODE_MSG, msg.cid, msg_res);
}

function process_node_code(msg) {
    switch (msg.error_code) {
	    case Error_Code.TOKEN_ERROR: {
	        on_close_session(msg.sid, msg.error_code);
		    break;
	    }
	   	default:
	        break;
	}
}

function set_node_info(msg) {
    global.game_nid_cid_map.set(msg.node_info.node_id, msg.cid);
}

function verify_token(msg) {
	var session = new Session();
	session.client_eid = Endpoint.GATE_CLIENT_SERVER;
	session.client_cid = msg.client_cid;
	session.game_eid = Endpoint.GATE_NODE_SERVER;
	session.game_cid = global.game_nid_cid_map.get(msg.game_nid);
	session.sid = msg.sid;
	session.account = msg.account;
	on_add_session(session);
}

function game_logout(msg) {
    var session = global.sid_session_map.get(msg.sid);
	if (session) {
		on_close_session(session.client_cid, msg.error_code);
		on_remove_session(session);
	}
}
