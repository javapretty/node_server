/*
*	描述：排行榜实现类
*	作者：李俊良
*	时间：2016/05/25
*/

function Rank() {
	this.rank_map = new Map();
}

Rank.prototype.load_data = function(obj) {
	log_info('load rank data, size:', obj.rank_list.length);	
	for(var i = 0; i < obj.rank_list.length; i++) {
		var rank_info = obj.rank_list[i];
		log_info('rank type:', rank_info.rank_type, ' min_role_id:', rank_info.min_role_id);	
		this.rank_map.set(rank_info.rank_type, rank_info);
	}
}

Rank.prototype.save_data = function() {	
	var msg = new node_251();
	msg.save_type = Save_Type.SAVE_CACHE_DB;
	msg.vector_data = true;
	msg.db_id = DB_Id.GAME;
	msg.struct_name = "Rank_Info";
	msg.data_type = DB_Data_Type.RANK;
	for (var value of this.rank_map.values()) {
		msg.rank_list.push(value);
	}
	send_msg_to_db(Msg.SYNC_SAVE_DB_DATA, 0, msg);
}

Rank.prototype.get_rank_value = function(type, player) {
	//按照类型获取player对应数值
	var rank_value = 0;
	switch(type) {
	case Rank_Type.LEVEL_RANK:
		rank_value = player.role_info.level;
		break;
	case Rank_Type.COMBAT_RANK:
		rank_value = 1;
		break;
	default:
		rank_value = -1;
		break;
	}

	return rank_value;
}

Rank.prototype.update_rank = function(type, player) {
	var rank_info = this.rank_map.get(type);
	if(rank_info == null) {
		rank_info = new Rank_Info();
		rank_info.rank_type = type;
		rank_info.min_value = 0x7fffffff;
		this.rank_map.set(type, rank_info);
	}
	
	var rank_value = this.get_rank_value(type, player);
	var rank_member = rank_info.member_map.get(player.role_info.role_id);
	if(rank_member == null) {
		rank_member = new Rank_Member_Detail();
		rank_member.role_id = player.role_info.role_id;
		rank_member.role_name = player.role_info.role_name;
		rank_member.value = rank_value;
		if(rank_info.member_map.size < 100) {
			rank_info.member_map.set(rank_member.role_id, rank_member);
			if(rank_member.value < rank_info.min_value){
				rank_info.min_value = rank_member.value;
				rank_info.min_role_id = rank_member.role_id;
			}
		}
		else {
			if(rank_member.value > rank_info.min_value){
				rank_info.member_map.delete(rank_info.min_role_id);
				rank_info.member_map.set(rank_member.role_id, rank_member);
				this.refresh_min_value(rank_info);
			}
		}
	}
	else {
		rank_member.value = rank_value;
		if(rank_member.value < rank_info.min_value){
			rank_info.min_value = rank_member.value;
			rank_info.min_role_id = rank_member.role_id;
		}
		else {
			this.refresh_min_rank_value(rank_info);
		}
	}
}

Rank.prototype.refresh_min_rank_value = function(rank_info){
	rank_info.min_value = 0x7fffffff;
	for (var value of rank_info.member_map.values()) {
  		if(value.value < rank_info.min_value){
			rank_info.min_value = value.value;
			rank_info.min_role_id = value.role_id;
		}
	}
}
