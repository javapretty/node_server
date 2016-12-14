/*
 *   描述：scene_entity类
 *   作者：李俊良
 *   时间：2016/12/8
*/

function Scene_Entity(player) {
	this.centity = create_aoi_entity(player.sid, player.gate_eid);
	this.player = player;
	this.pos = new Position();
	this.speed = 10;
	this.move_tick = util.now_msec(); //移动tick
	this.move_path = new Array(); //移动路径
	this.last_mv_tm = 0;//上次移动时间
}

Scene_Entity.prototype.on_move = function(now) {
	if(now - this.move_tick < 300)
		return;
	this.move_tick = now;
	if(this.move_path.length <= 0)
		return;
	var dlt_tm = now - this.last_mv_tm;
	var move_distance = dlt_tm * this.speed / 1000;
	do {
		var tar_pos = this.move_path[0];
		var dis_x = tar_pos.x * 32 - this.pos.x;
		var dis_y = tar_pos.y * 32 - this.pos.y;
		var distance = Math.sqrt(dis_x * dis_x + dis_y * dis_y);
		this.pos.x = tar_pos.x * 32;
		this.pos.y = tar_pos.y * 32;
		move_distance -= distance;
		this.move_path.shift();
	} while((move_distance > distance) && this.move_path.length > 0);
	if(this.move_path.length > 0) {
		this.pos.x += (move_distance / distance) * dis_x;
		this.pos.y += (move_distance / distance) * dis_y;
	}
	var aoi_map = this.centity.update_position(this.pos.x, this.pos.y);
	if(aoi_map != null)
		this.send_broadcast_msg(aoi_map.enter_list, aoi_map.leave_list, aoi_map.aoi_list);
	
	this.last_mv_tm = now;
}

Scene_Entity.prototype.send_broadcast_msg = function(enter_list, leave_list, aoi_list) {
	if(enter_list && enter_list.length > 0) {
		var msg = new s2c_11();
		msg.sid = this.player.sid;
		msg.role_name = this.player.role_info.role_name;
		msg.level = this.player.role_info.level;
		this.centity.broadcast_msg_to_all_without_self(enter_list, Msg.RES_ENTER_ZONE, msg);
	
		var msg_list = new Array();
		for (var i = 0; i < enter_list.length; i++) {
			var ply = global.sid_game_player_map.get(enter_list[i]);
			var msg = new s2c_11();
			msg.sid = ply.sid;
			msg.role_name = ply.role_info.role_name;
			msg.level = ply.role_info.level;
			msg_list.push(msg);
		}
		this.centity.send_msg_list(Msg.RES_ENTER_ZONE, msg_list);
	}
	if(leave_list && leave_list.length > 0) {
		var msg = new s2c_12();
		msg.sid = this.player.sid;
		this.centity.broadcast_msg_to_all_without_self(leave_list, Msg.RES_LEAVE_ZONE, msg);
	
		var msg_list = new Array();
		for (var i = 0; i < leave_list.length; i++) {
			var ply = global.sid_game_player_map.get(leave_list[i]);
			var msg = new s2c_12();
			msg.sid = ply.sid;
			msg_list.push(msg);
		}
		this.centity.send_msg_list(Msg.RES_LEAVE_ZONE, msg_list);
	}
	if(aoi_list && aoi_list.length > 0) {
		var msg = new s2c_10();
		msg.sid = this.player.sid;
		for(var i = 0; i < this.move_path.length; i++) {
			var pos = this.move_path[i];
			log_info("pos.x:" + pos.x + ", pos.y:" + pos.y);
			msg.move_path.push(pos);
		}
		this.centity.broadcast_msg_to_all_without_self(aoi_list, Msg.RES_MOVE, msg);
	}
}

Scene_Entity.prototype.enter_scene = function(scene_id, x, y) {
	this.pos.x = x;
	this.pos.y = y;
	var enter_list = this.centity.enter_aoi(scene_id, x, y);
	this.send_broadcast_msg(enter_list, null, null);
}

Scene_Entity.prototype.leave_scene = function() {
	this.pos.x = 0;
	this.pos.y = 0;
	var leave_list = this.centity.get_aoi_list();
	this.send_broadcast_msg(null, leave_list, null);
	this.centity.leave_aoi();
}

Scene_Entity.prototype.logout = function() {
	this.leave_scene();
	this.centity.reclaim();
	this.centity = null;
}

