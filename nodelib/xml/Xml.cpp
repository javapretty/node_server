/*
 * Xml.cpp
 *
 *  Created on: Jul 15,2016
 *      Author: zhangyalei
 */

#include "Log.h"
#include "Xml.h"

Xml::Xml():
	rootElement_(nullptr),
	doc_(new TiXmlDocument)
{
}

Xml::Xml(const char *path):
	rootElement_(nullptr),
	doc_(new TiXmlDocument)
{
	load_xml(path);
}

Xml::~Xml(){
	if(doc_){
		delete doc_;
		doc_ = nullptr;
		rootElement_ = nullptr;
	}
}

int Xml::load_xml(const char *path){
	bool ret = doc_->LoadFile(path);
	if(!ret) {
		LOG_ERROR("load xml error, path:%s", path);
		return -1;
	}
	rootElement_ = doc_->RootElement();
	return 0;
}

TiXmlNode *Xml::enter_root_node(){
	if(rootElement_ == nullptr) {
		return nullptr;
	}
	else {
		return rootElement_->FirstChild();
	}
}

TiXmlNode *Xml::get_root_node(const char *key) {
	if(rootElement_ == nullptr || strlen(key) <= 0) {
		return nullptr;
	}
	else {
		TiXmlNode *node = rootElement_->FirstChild(key);
		return node;
	}
}

TiXmlNode *Xml::enter_node(TiXmlNode *node, const char *key){
	do {
		if(node->Type() != TiXmlNode::TINYXML_ELEMENT) {
			continue;
		}

		if(get_key(node) == key){
			TiXmlNode* childNode = node->FirstChild();
			do {
				//如果子节点不存在或者子节点存在而且不是注释，就跳出循环
				if (!childNode || childNode->Type() != TiXmlNode::TINYXML_COMMENT) {
					break;
				}
			} while((childNode = childNode->NextSibling()));

			return childNode;
		}
	} while((node = node->NextSibling()));

	return nullptr;
}

std::string Xml::get_key(TiXmlNode *node){
	std::string str(node->Value());
	return str;
}

bool Xml::has_key(TiXmlNode *node, const char *key){
	do{
		if(node->Type() != TiXmlNode::TINYXML_ELEMENT) {
			continue;
		}

		if(get_key(node) == key) {
			return true;
		}
	} while((node = node->NextSibling()));

	return false;
}

std::string Xml::get_val_str(TiXmlNode *node){
	if(node == nullptr) {
		return "";
	}
		
	TiXmlText *ptext = node->ToText();
	std::string str(ptext->Value());
	return str;
}

int Xml::get_val_int(TiXmlNode *node){
	if(node == nullptr) {
		return 0;
	}
		
	TiXmlText *ptext = node->ToText();
	return atoi(ptext->Value());
}

float Xml::get_val_float(TiXmlNode *node){
	if(node == nullptr) {
		return 0.0f;
	}
		
	TiXmlText *ptext = node->ToText();
	return atof(ptext->Value());
}

std::string Xml::get_attr_str(TiXmlNode* node, const char *key){
	if(node == nullptr) {
		return "";
	}
		
	TiXmlElement *element = node->ToElement();
	const char *p= element->Attribute(key);
	if(!p) {
		return "";
	}
	std::string value = p;
	return value;
}

int Xml::get_attr_int(TiXmlNode* node, const char *key){
	if(node == nullptr) {
		return 0;
	}
	
	int value = 0;
	TiXmlElement *element = node->ToElement();
	element->Attribute(key, &value);
	return value;
}

float Xml::get_attr_float(TiXmlNode* node, const char *key){
	if(node == nullptr) {
		return 0.0f;
	}

	double value = 0.0f;
	TiXmlElement *element = node->ToElement();
	element->Attribute(key, &value);
	return value;
}

bool Xml::has_child(TiXmlNode *node) {
	if(node == nullptr || node->Type() != TiXmlNode::TINYXML_ELEMENT) {
		return false;
	}

	TiXmlNode* childNode = node->FirstChild();
	do {
		//如果子节点不存在，就跳出循环
		if(!childNode) {
			break;
		}

		//如果子节点存在而且不是注释,就返回true,否则继续循环
		if(childNode && childNode->Type() != TiXmlNode::TINYXML_COMMENT) {
			return true;
		}
	} while((childNode = childNode->NextSibling()));

	return false;
}

bool Xml::has_attribute(TiXmlNode *node) {
	if(node == nullptr || node->Type() != TiXmlNode::TINYXML_ELEMENT) {
		return false;
	}

	TiXmlElement *element = node->ToElement();
	if(element->FirstAttribute() == nullptr) {
		return false;
	}

	return true;
}

int Xml::count_key(TiXmlNode *node, const char *key) {
	if(node == nullptr || key == nullptr) {
		return -1;
	}
		
	int count = 0;
	do {
		if(get_key(node) == key)
			count++;
	} while((node = node->NextSibling()));

	return count;	
}
