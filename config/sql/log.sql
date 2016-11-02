CREATE DATABASE IF NOT EXISTS `log` DEFAULT CHARACTER SET utf8;

USE `log`;

DROP TABLE IF EXISTS `logout`;
CREATE TABLE `logout` (
  role_id bigint(20) NOT NULL default '0',
  role_name varchar(120) NOT NULL default '', 
  account varchar(120) NOT NULL default '',
  level int(11) NOT NULL default '0',
  exp int(11) NOT NULL default '0',
  gender tinyint(3) NOT NULL default '0',
  career tinyint(3) NOT NULL default '0',
  create_time int(11) NOT NULL default '0',
  login_time int(11) NOT NULL default '0',
  logout_time int(11) NOT NULL default '0'
)ENGINE=InnoDB DEFAULT CHARSET=utf8;

