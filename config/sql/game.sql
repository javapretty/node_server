CREATE DATABASE IF NOT EXISTS `game` DEFAULT CHARACTER SET utf8;
USE game

DROP TABLE IF EXISTS `rank`;
CREATE TABLE `rank` (
	rank_type bigint(20) NOT NULL default '0',
	min_role_id bigint(20) NOT NULL default '0',
	min_value int(11) NOT NULL default '0',
	member_map text NOT NULL,
	PRIMARY KEY (rank_type)
)ENGINE=InnoDB DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `mail`;
CREATE TABLE `mail` (
	role_id bigint(20) NOT NULL default '0',
	total_count int(11) NOT NULL default '0',
	mail_map text NOT NULL,
	PRIMARY KEY (role_id)
)ENGINE=InnoDB DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `bag`;
CREATE TABLE `bag` (
	role_id bigint(20) NOT NULL default '0',
	item_map text NOT NULL,
	PRIMARY KEY (role_id)
)ENGINE=InnoDB DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `activity`;
CREATE TABLE `activity` (
	role_id bigint(20) NOT NULL default '0',
	seven_day_start_time int(11) NOT NULL default '0',
	seven_day_award_status text NOT NULL,
	sign_in_award_status text NOT NULL,
	month_card int(11) NOT NULL default '0',
	PRIMARY KEY (role_id)
)ENGINE=InnoDB DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `role`;
CREATE TABLE `role` (
	role_id bigint(20) NOT NULL default '0',
	role_name varchar(120) NOT NULL default '',
	account varchar(120) NOT NULL default '',
	gender int(11) NOT NULL default '0',
	career int(11) NOT NULL default '0',
	level int(11) NOT NULL default '0',
	exp int(11) NOT NULL default '0',
	combat int(11) NOT NULL default '0',
	create_time int(11) NOT NULL default '0',
	login_time int(11) NOT NULL default '0',
	logout_time int(11) NOT NULL default '0',
	gold int(11) NOT NULL default '0',
	diamond int(11) NOT NULL default '0',
	guild_id bigint(20) NOT NULL default '0',
	guild_name varchar(120) NOT NULL default '',
	speed int(11) NOT NULL default '0',
	last_scene int(11) NOT NULL default '0',
	last_x int(11) NOT NULL default '0',
	last_y int(11) NOT NULL default '0',
	PRIMARY KEY (role_id)
)ENGINE=InnoDB DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `account`;
CREATE TABLE `account` (
	account varchar(120) NOT NULL default '',
	role_list text NOT NULL,
	PRIMARY KEY (account)
)ENGINE=InnoDB DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `idx`;
CREATE TABLE `idx` (
	type varchar(120) NOT NULL default '',
	value int(11) NOT NULL default '0',
	PRIMARY KEY (type)
)ENGINE=InnoDB DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `guild`;
CREATE TABLE `guild` (
	guild_id bigint(20) NOT NULL default '0',
	guild_name varchar(120) NOT NULL default '',
	chief_id bigint(20) NOT NULL default '0',
	create_time int(11) NOT NULL default '0',
	member_list text NOT NULL,
	PRIMARY KEY (guild_id)
)ENGINE=InnoDB DEFAULT CHARSET=utf8;
