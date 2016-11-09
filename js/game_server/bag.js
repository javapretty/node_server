/*
*	描述：背包系统
*	作者：张亚磊
*	时间：2016/02/26
*/

function Bag() {
	this.game_player = null;
	this.bag_info = new Bag_Info();
}
	
Bag.prototype.load_data = function(game_player, msg) {
	this.game_player = game_player;
	this.bag_info = msg.player_data.bag_info;	
}

Bag.prototype.save_data = function(msg) {
	msg.player_data.bag_info = this.bag_info;
}

Bag.prototype.fetch_bag = function() {
	var msg_res = new s2c_9();
	for (var value of this.bag_info.item_map.values()) {
  		msg_res.item_list.push(value);
	}
  this.game_player.send_success_msg(Msg.RES_FETCH_BAG, msg_res);
}
	
Bag.prototype.bag_add_money = function(copper, gold) {
	if(!util.is_number(copper) || !util.is_number(gold)){
		return Error_Code.CLIENT_PARAM_ERROR;
	}
	this.bag_info.copper += copper;
	this.bag_info.gold += gold;
	this.bag_active_money();
	return 0;
}
	
Bag.prototype.bag_sub_money = function(copper, gold) {
	if(!util.is_number(copper) || !util.is_number(gold)){
		return Error_Code.CLIENT_PARAM_ERROR;
	}
	if (this.bag_info.copper < copper) {
		return Error_Code.COPPER_NOT_ENOUGH;
	}
	if (this.bag_info.gold < gold) {
		return Error_Code.GOLD_NOT_ENOUGH;
	}
	this.bag_info.copper -= copper;
	this.bag_info.gold -= gold;
	this.bag_active_money();
	return 0;
}
	
Bag.prototype.bag_insert_item = function(item_array) {
	if (this.bag_info.item_map.size + item_array.length > 2000) {
		return this.game_player.send_error_msg(Error_Code.BAG_FULL);
	}
	for (var i = 0; i < item_array.length; ++i) {
		var item_info = this.bag_info.item_map.get(item_array[i].item_id);
		if (item_info == null) {
			this.bag_info.item_map.set(item_array[i].item_id, item_array[i]);	
		} else {
			item_info.amount += item_array[i].amount;
		}	
	}
	
	this.bag_active_item();
	return 0;
}
	
Bag.prototype.bag_erase_item = function(item_array) {
	for (var i = 0; i < item_array.length; ++i) {
		var item_info = this.bag_info.item_map.get(item_array[i].id);
		if (item_info == null) {
			return this.game_player.send_error_msg(Error_Code.ITEM_NOT_EXIST);
		} else {
			if (item_info.amount < item_array[i].amount) {
				return this.game_player.send_error_msg(Error_Code.ITEM_NOT_ENOUGH);
			}
		}
	}
	
	for (var i = 0; i < item_array.length; ++i) {
		var item_info = this.bag_info.item_map.get(item_array[i].id);
		item_info.amount -= item_array[i].amount;
		if (item_info.amount == 0) {
			this.bag_info.item_map.delete(item.id);
		}
	}
	
	this.bag_active_item();
	return 0;
}
	
Bag.prototype.bag_active_item = function() {
}

Bag.prototype.bag_active_money = function() {
}
