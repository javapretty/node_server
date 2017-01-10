/*
*	描述：背包系统
*	作者：张亚磊
*	时间：2016/02/26
*/

function Bag() {
	this.game_player = null;
	this.bag_info = null;
}
	
Bag.prototype.load_data = function(game_player, player_data) {
	this.game_player = game_player;
	this.bag_info = player_data.bag_info;	
}

Bag.prototype.save_data = function(player_data) {
	player_data.bag_info = this.bag_info;
}

Bag.prototype.insert_item = function(item_info) {
	var item_list = new Array();
	item_list.push(item_info);
	return this.insert_item_list(item_list);
}

Bag.prototype.insert_item_list = function(item_list) {
	if (this.bag_info.item_map.size + item_list.length > 1000) {
		return this.game_player.send_error_msg(Error_Code.BAG_FULL);
	}
	for (var i = 0; i < item_list.length; ++i) {
		var item_info = this.bag_info.item_map.get(item_list[i].item_id);
		if (item_info == null) {
			this.bag_info.item_map.set(item_list[i].item_id, item_list[i]);	
		} else {
			item_info.amount += item_list[i].amount;
		}	
	}
	
	this.active_item_info();
	return 0;
}

Bag.prototype.erase_item = function(item_info) {
	var item_list = new Array();
	item_list.push(item_info);
	return this.erase_item_list(item_list);
}

Bag.prototype.erase_item_list = function(item_list) {
	for (var i = 0; i < item_list.length; ++i) {
		var item_info = this.bag_info.item_map.get(item_list[i].id);
		if (item_info == null) {
			return this.game_player.send_error_msg(Error_Code.ITEM_NOT_EXIST);
		} else {
			if (item_info.amount < item_list[i].amount) {
				return this.game_player.send_error_msg(Error_Code.ITEM_NOT_ENOUGH);
			}
		}
	}
	
	for (var i = 0; i < item_list.length; ++i) {
		var item_info = this.bag_info.item_map.get(item_list[i].id);
		item_info.amount -= item_list[i].amount;
		if (item_info.amount == 0) {
			this.bag_info.item_map.delete(item.id);
		}
	}
	
	this.active_item_info();
	return 0;
}
	
Bag.prototype.active_item_info = function() {
}