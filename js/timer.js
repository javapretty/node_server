/*
*	描述：定时器系统
*	作者：张亚磊
*	时间：2016/10/24
*/

function Timer() {
	var timer_map = new Map();
	var timer_id = 1;
	
	this.init = function() {
	    //注册node_server定时器，时间间隔30s
	    this.register_timer(30000, 0, this.node_server_handler);

	    switch(global.node_info.node_type) {
		case Node_Type.CENTER_SERVER: {
			//注册清除token定时器，时间间隔2s
		    this.register_timer(2000, 0, this.center_token_handler);
			break;
		}
		case Node_Type.GAME_SERVER: {
			//注册玩家定时器，时间间隔500ms
		    this.register_timer(500, 0, this.game_player_handler);
			break;
		}
		default: {
		    break;
		}
		}
	}
	
	this.register_timer = function(interval, next_tick, handler) {
		register_timer(timer_id, interval, next_tick);
		timer_map.set(timer_id, handler);
		timer_id++;
	}

	this.get_timer_handler = function(timer_id) {
		return timer_map.get(timer_id);
	}
	
    /////////////////////////////////定时器处理函数/////////////////////////////////
	this.node_server_handler = function() {
	    switch(global.node_info.node_type) {
	        case Node_Type.CENTER_SERVER: {
	            break;
	        }
	        case Node_Type.GATE_SERVER: {
	            util.sync_node_status(Endpoint.GATE_MASTER_CONNECTOR, 0);
	            break;
	        }
	        case Node_Type.DATA_SERVER: {
	            util.sync_node_status(Endpoint.DATA_MASTER_CONNECTOR, 0);
	            break;
	        }
	        case Node_Type.LOG_SERVER: {
	            util.sync_node_status(Endpoint.LOG_MASTER_CONNECTOR, 0);
	            break;
	        }
	        case Node_Type.MASTER_SERVER: {
	            var node_status = util.get_node_status();
	            global.node_status_map.set(node_status.node_id, node_status);
	            break;
	        }
	        case Node_Type.PUBLIC_SERVER: {
	            global.guild_manager.save_data_handler();
	            global.rank_manager.save_data();

	            util.sync_node_status(Endpoint.PUBLIC_MASTER_CONNECTOR, 0);
	            break;
	        }
	        case Node_Type.GAME_SERVER: {
	            util.sync_node_status(Endpoint.GAME_MASTER_CONNECTOR, 0);
	            break;
	        }
	        default: {
	            break;
	        }
	    }
	}

	this.center_token_handler = function() {
		var now = util.now_sec();
		global.account_token_map.forEach(function(value, key, map) {
			if (now - value.token_time >= 2) {
				on_close_session(key, value.cid, Error_Code.TOKEN_TIMEOUT);	
			}
		});
	}
	
	this.game_player_handler = function() {
		var now = util.now_msec();
		for (var value of global.role_id_game_player_map.values()) {
  			value.tick(now);
		}
    }
}
