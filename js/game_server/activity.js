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
		log_warn("item_conf null, day_index:", msg.day_index);
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
	var date = new Date();
	var month = date.getMonth();	//(0~11)
	var day = date.getDate();		//(1~31)

	if(this.activity_info.sign_in_award_status[day-1]) {
		return this.game_player.send_error_msg(Error_Code.AWARD_GET_ALREADY);
	}

	var month_conf = global.config.general.sign_in_award.month[month];
	if(month_conf == null) {
		log_warn("month_conf null, month:", month);
		return this.game_player.send_error_msg(Error_Code.CONFIG_ERROR);	
	}

	var item_conf = month_conf.item[day-1];
	if(item_conf == null) {
		log_warn("item_conf null, day:", day);
		return this.game_player.send_error_msg(Error_Code.CONFIG_ERROR);	
	}

	var item_info = new Item_Info();
	item_info.item_id = item_conf.id;
	item_info.amount = item_conf.amount;
	var result = this.game_player.bag.insert_item(item_info);
	if (result != 0) {
		return this.game_player.send_error_msg(result);
	}

	this.activity_info.sign_in_award_status[day-1] = true;
	this.game_player.data_change = true;

    this.game_player.send_success_msg(Msg.RES_SIGN_IN, new Object());
}

Activity.prototype.resign_in = function(msg) {
	var date = new Date();
	var month = date.getMonth();	//(0~11)
	
	if(this.activity_info.sign_in_award_status[msg.day_index]) {
		return this.game_player.send_error_msg(Error_Code.AWARD_GET_ALREADY);
	}

	var month_conf = global.config.general.sign_in_award.month[month];
	if(month_conf == null) {
		log_warn("month_conf null, month:", month);
		return this.game_player.send_error_msg(Error_Code.CONFIG_ERROR);	
	}

	var item_conf = month_conf.item[msg.day_index];
	if(item_conf == null) {
		log_warn("item_conf null, day_index:", msg.day_index);
		return this.game_player.send_error_msg(Error_Code.CONFIG_ERROR);	
	}

	//此处直接判断钻石是否足够，防止插入背包失败导致扣除不该扣的钻石
	if(this.game_player.role_info.diamond < item_conf.resign) {
		return this.game_player.send_error_msg(Error_Code.DIAMOND_NOT_ENOUGH);
	}

	var item_info = new Item_Info();
	item_info.item_id = item_conf.id;
	item_info.amount = item_conf.amount;
	var result = this.game_player.bag.insert_item(item_info);
	if (result != 0) {
		return this.game_player.send_error_msg(result);
	}

	this.game_player.sub_money(0, item_conf.resign);
	this.activity_info.sign_in_award_status[msg.day_index] = true;
	this.game_player.data_change = true;

	var msg_res = new Object();
	msg_res.day_index = msg.day_index;
    this.game_player.send_success_msg(Msg.RES_RESIGN_IN, msg_res);
}