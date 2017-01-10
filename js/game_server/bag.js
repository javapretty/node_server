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
	
Bag.prototype.bag_insert_item = function(item_array) {
	if (this.bag_info.item_map.size + item_array.length > 1000) {
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