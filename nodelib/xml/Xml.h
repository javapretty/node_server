/*
 * Xml.h
 *
 *  Created on: Jul 15,2016
 *      Author: zhangyalei
 */

#ifndef XML_H_
#define XML_H_

#include <string>
#include "tinyxml.h"

#define XML_LOOP_BEGIN(NODE)\
	do { \
		if(NODE->Type() != TiXmlNode::TINYXML_ELEMENT) \
			continue;

#define XML_LOOP_END(NODE)\
	} while ((NODE = NODE->NextSibling()));

#define XML_ATTR_LOOP_BEGIN(NODE, ATTR)\
	TiXmlElement *NODE##element = node->ToElement();\
	TiXmlAttribute *ATTR = NODE##element->FirstAttribute();\
	do { 

#define XML_ATTR_LOOP_END(ATTR)\
	} while ((ATTR = ATTR->Next()));


class Xml {
public:
	Xml();
	Xml(const char *path);
	~Xml();
	int load_xml(const char *path);

	//获取root下面第一个子节点
	TiXmlNode *enter_root_node();
	//根据key获取root下面子节点
	TiXmlNode *get_root_node(const char *key);
	//获取node下面名字为key节点的第一个子节点
	TiXmlNode *enter_node(TiXmlNode *node, const char *key);

	//获取node节点key名称
	std::string get_key(TiXmlNode *node);
	//判断node节点内是否存在名称为key的节点
	bool has_key(TiXmlNode *node, const char *key);
	//判断node节点是否有字节点
	bool has_child(TiXmlNode *node);
	//判断node节点是否有属性
	bool has_attribute(TiXmlNode *node);
	//统计node节点内名字为key节点数量
	int count_key(TiXmlNode *node, const char *key);

	/** The meaning of 'value' changes for the specific type of TiXmlNode.
		Document:	filename of the xml file
		Element:	name of the element
		Comment:	the comment text
		Unknown:	the tag contents
		Text:		the text string
	*/
	std::string get_val_str(TiXmlNode *node);
	int get_val_int(TiXmlNode *node);
	float get_val_float(TiXmlNode *node);

	//获取属性值
	std::string get_attr_str(TiXmlNode* node, const char *key);
	int get_attr_int(TiXmlNode* node, const char *key);
	float get_attr_float(TiXmlNode* node, const char *key);

private:
	TiXmlElement *rootElement_;
	TiXmlDocument *doc_;
};

#endif