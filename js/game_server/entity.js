/*
 *   描述：entity类
 *   作者：李俊良
 *   时间：2016/12/8
*/

function Entity() {
	this.centity = null;
	this.player = null;
	this.scene_id = 0;
	this.pos = new Object();
	this.frm_pos = new Object();		//起始位置
	this.to_pos = new Object();			//目标位置
	this.speed = 0;
	this.move_tick = util.now_msec(); 	//移动tick
	this.move_path = new Array(); 		//移动路径
	this.last_mv_tm = 0;				//上次移动时间
}

Entity.prototype.load_data = function(game_player, player_data) {
	this.centity = create_aoi_entity(game_player.sid, game_player.gate_eid, 800);
	this.player = game_player;
	this.speed = player_data.role_info.speed;

	this.enter_scene(player_data.role_info.last_scene, player_data.role_info.last_x, player_data.role_info.last_y);
}

Entity.prototype.save_data = function(player_data, logout) {
	player_data.role_info.last_scene = this.scene_id;
	player_data.role_info.last_x = this.pos.x;
	player_data.role_info.last_y = this.pos.y;

	if(logout) {
		this.leave_scene();
		this.centity.reclaim();
		this.centity = null;
	}
}

Entity.prototype.on_move = function(now) {
	if(this.move_path.length <= 0)
		return;
	if(now - this.move_tick < 300)
		return;
	this.move_tick = now;
	
	this.move_process(now);
}

Entity.prototype.check_path = function(move_path) {
	if(move_path.length <= 0)
		return false;

	return true;
} 

Entity.prototype.move = function(msg) {
	if(!this.check_path(msg.move_path)) {
		log_error("check_path faire");
		return;
	}

	if(this.move_path.length <= 0) {
		for(var i = 0; i < msg.move_path.length; i++) {
			var pos = new Object();
			pos.x = msg.move_path[i].x * 32;
			pos.y = msg.move_path[i].y * 64;
			this.move_path.push(pos);
		}
		this.start_move();
	}
	else {
		//log_error(this.role_info.role_name + "is moving");
		this.stop_move();
		for(var i = 0; i < msg.move_path.length; i++) {
			var pos = new Object();
			pos.x = msg.move_path[i].x * 32;
			pos.y = msg.move_path[i].y * 32;
			this.move_path.push(pos);
		}
		this.start_move();
	}
}

Entity.prototype.start_move = function() {
	this.last_mv_tm = util.now_msec();
	this.frm_pos.x = this.pos.x;
	this.frm_pos.y = this.pos.y;
	this.to_pos.x = this.move_path[0].x;
	this.to_pos.y = this.move_path[0].y;
	var aoi_list = this.centity.get_aoi_list();
	this.broadcast_msg(null, null, aoi_list);
}

Entity.prototype.stop_move = function() {
	var cur_msec = util.now_msec();
	this.move_path.length = 0;
	this.move_process(cur_msec);
}

Entity.prototype.move_process = function(now) {
	var change_target = false;
	var dlt_tm = now - this.last_mv_tm;
	var move_distance = dlt_tm * this.speed / 1000;
	do {
		var dis_x = this.to_pos.x - this.pos.x;
		var dis_y = this.to_pos.y - this.pos.y;
		var distance = util.calculate_distance(this.pos, this.to_pos);
		if(move_distance > distance) {
			this.pos.x = this.to_pos.x;
			this.pos.y = this.to_pos.y;
			move_distance -= distance;
			if(this.move_path.length <= 1) {
				this.to_pos.x = this.pos.x;
				this.to_pos.y = this.pos.y;
			}
			else {
				this.to_pos.x = this.move_path[1].x;
				this.to_pos.y = this.move_path[1].y;
			}
			this.move_path.shift();
			change_target = true;
		}
		else {
			this.pos.x += Math.round((move_distance / distance) * dis_x);
			this.pos.y += Math.round((move_distance / distance) * dis_y);
			break;
		} 
	} while(this.move_path.length > 0);
	var aoi_map = this.centity.update_position(this.pos.x, this.pos.y);
	if(aoi_map != null) {
		if(change_target) { 
			this.frm_pos.x = this.pos.x;
			this.frm_pos.y = this.pos.y;
			this.broadcast_msg(aoi_map.enter_list, aoi_map.leave_list, aoi_map.aoi_list);
		}
		else {
			this.broadcast_msg(aoi_map.enter_list, aoi_map.leave_list, null);
		}
	}
	this.last_mv_tm = now;
}

Entity.prototype.broadcast_msg = function(enter_list, leave_list, aoi_list) {
	if(enter_list && enter_list.length > 0) {
		var msg = new Object();
		msg.sid = this.player.sid;
		msg.role_name = this.player.role_info.role_name;
		msg.level = this.player.role_info.level;
		msg.pos_x = this.pos.x;
		msg.pos_y = this.pos.y;
		this.centity.broadcast_msg_to_all_without_self(enter_list, Msg.RES_ENTER_ZONE, msg);
	
		var msg_list = new Array();
		for (var i = 0; i < enter_list.length; i++) {
			var ply = global.sid_game_player_map.get(enter_list[i]);
			var msg = new Object();
			msg.sid = ply.sid;
			msg.role_name = ply.role_info.role_name;
			msg.level = ply.role_info.level;
			msg.pos_x = ply.entity.pos.x;
			msg.pos_y = ply.entity.pos.y;
			msg_list.push(msg);
		}
		this.centity.send_msg_list(Msg.RES_ENTER_ZONE, msg_list);
		if(this.move_path.length > 0) {
			var msg = new Object();
			msg.sid = this.player.sid;
			msg.frm_x = this.frm_pos.x;
			msg.frm_y = this.frm_pos.y;
			msg.to_x = this.to_pos.x;
			msg.to_y = this.to_pos.y;

			this.centity.broadcast_msg_to_all_without_self(enter_list, Msg.RES_MOVE, msg);
		}
	}
	if(leave_list && leave_list.length > 0) {
		var msg = new Object();
		msg.sid = this.player.sid;
		this.centity.broadcast_msg_to_all_without_self(leave_list, Msg.RES_LEAVE_ZONE, msg);
		
		if(this.scene_id > 0) {
			var msg_list = new Array();
			for (var i = 0; i < leave_list.length; i++) {
				var ply = global.sid_game_player_map.get(leave_list[i]);
				var msg = new Object();
				msg.sid = ply.sid;
				msg_list.push(msg);
			}
			this.centity.send_msg_list(Msg.RES_LEAVE_ZONE, msg_list);
		}
	}
	if(aoi_list && aoi_list.length > 0) {
		var msg = new Object();
		msg.sid = this.player.sid;
		msg.frm_x = this.frm_pos.x;
		msg.frm_y = this.frm_pos.y;
		msg.to_x = this.to_pos.x;
		msg.to_y = this.to_pos.y;

		this.centity.broadcast_msg_to_all_without_self(aoi_list, Msg.RES_MOVE, msg);
	}
}

Entity.prototype.enter_scene = function(scene_id, x, y) {
	this.scene_id = scene_id;
	this.pos.x = x;
	this.pos.y = y;
	var enter_list = this.centity.enter_aoi(scene_id, x, y);
	this.broadcast_msg(enter_list, null, null);
}

Entity.prototype.leave_scene = function() {
	this.scene_id = -1;
	this.pos.x = 0;
	this.pos.y = 0;
	var leave_list = this.centity.get_aoi_list();
	this.broadcast_msg(null, leave_list, null);
	this.centity.leave_aoi();
}

