/*
*	描述：活动系统
*	作者：张亚磊
*	时间：2017/01/10
*/

function Activity() {
	this.game_player = null;
	this.activity_info = null;
}
	
Activity.prototype.load_data = function(game_player, player_data) {
	this.game_player = game_player;
	this.activity_info = player_data.activity_info;	
}

Activity.prototype.save_data = function(player_data) {
	player_data.activity_info = this.activity_info;
}

Activity.prototype.seven_day_award = function(msg) {
	if(msg.day_index < 0 || msg.day_index >= 7) {
		return this.game_player.send_error_msg(Error_Code.CLIENT_PARAM_ERROR);
	}

	if(this.activity_info.seven_day_award_status[msg.day_index]) {
		return this.game_player.send_error_msg(Error_Code.AWARD_GET_ALREADY);
	}

	var item_conf = global.config.general.seven_day_award.item[msg.day_index];
	if(item_conf == null) {
		log_warn("item null, day_index:", msg.day_index);
		return this.game_player.send_error_msg(Error_Code.CONFIG_ERROR);	
	}
	var item_info = new Item_Info();
	item_info.item_id = item_conf.id;
	item_info.amount = item_conf.amount;
	var result = this.game_player.bag.insert_item(item_info);
	if (result != 0) {
		return this.game_player.send_error_msg(result);
	}

	this.activity_info.seven_day_award_status[msg.day_index] = true;
	this.game_player.data_change = true;

	var msg_res = new Object();
	msg_res.day_index = msg.day_index;
    this.game_player.send_success_msg(Msg.RES_SEVEN_DAY_AWARD, msg_res);
}

Activity.prototype.sign_in = function(msg) {
	
}

Activity.prototype.resign_in = function(msg) {
	
}