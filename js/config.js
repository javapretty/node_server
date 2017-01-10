/*
*	描述：配置文件
*	作者：张亚磊
*	时间：2016/03/24
*/

function Config() {
	this.general = null;

	this.init = function() {
	    try {
	    	this.general = read_xml("config/xml/general.xml");
		} catch (err) {
			log_error(err.message);
		}
	}
}
