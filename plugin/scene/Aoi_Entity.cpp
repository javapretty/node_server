/*
 *  Created on: Aug 11, 2016
 *      Author: lijunliang
 */

#include "Aoi_Entity.h"
#include "Aoi_Manager.h"

Aoi_Entity::Aoi_Entity():
	sid_(0),
	eid_(0),
	aoi_manager_(nullptr),
	pos_(0,0,0),
	opos_(0,0,0),
	radius_(500),
	x_pos_(),
	y_pos_(),
	enter_map_(),
	leave_map_(),
	aoi_map_()
{

}

Aoi_Entity::~Aoi_Entity() {

}

Aoi_Entity_Pool Aoi_Entity::aoi_entity_pool_;
	
Aoi_Entity *Aoi_Entity::create_aoi_entity(int sid, int eid) {
	Aoi_Entity *aoi_entity = aoi_entity_pool_.pop();
	aoi_entity->sid(sid);
	aoi_entity->eid(eid);
	return aoi_entity;
}

void Aoi_Entity::reclaim_aoi_entity(Aoi_Entity *entity) {
	entity->reset();
	aoi_entity_pool_.push(entity);
}

void Aoi_Entity::add_aoi_entity(Aoi_Entity *entity) {
	if(aoi_map_.find(entity->sid()) != aoi_map_.end())
		return;
	aoi_map_[entity->sid()] = entity;
}

void Aoi_Entity::del_aoi_entity(Aoi_Entity *entity) {
	if(aoi_map_.find(entity->sid()) == aoi_map_.end())
		return;
	aoi_map_.erase(entity->sid());
}

void Aoi_Entity::update_aoi_map(AOI_MAP &new_map) {
	enter_map_.clear();
	leave_map_.clear();
	for(AOI_MAP::iterator iter = aoi_map_.begin();
			iter != aoi_map_.end(); iter++){
		if(new_map.find(iter->second->sid()) == new_map.end()){
			del_aoi_entity(iter->second);
			iter->second->del_aoi_entity(this);
			leave_map_[iter->second->sid()] = iter->second;
		}
	}
	for(AOI_MAP::iterator iter = new_map.begin();
			iter != new_map.end(); iter++){
		if(aoi_map_.find(iter->second->sid()) == aoi_map_.end()){
			add_aoi_entity(iter->second);
			iter->second->add_aoi_entity(this);
			enter_map_[iter->second->sid()] = iter->second;
		}
	}
//	for(AOI_MAP::iterator iter = aoi_map_.begin();
//			iter != aoi_map_.end(); iter++){
//		LOG_ERROR("%d's aoi_entity is %d", entity_id(), iter->second->entity_id());
//	}
}

void Aoi_Entity::clear_aoi_map() {
	for(AOI_MAP::iterator iter = aoi_map_.begin(); iter != aoi_map_.end(); iter++){
		iter->second->del_aoi_entity(this);
	}
	enter_map_.clear();
	leave_map_.clear();
	aoi_map_.clear();
}

void Aoi_Entity::reset() {
	sid_ = 0;
	eid_ = 0;
	aoi_manager_ = nullptr;
	pos_ = Position(0,0,0);
	opos_ = Position(0,0,0);
	radius_ = 0;
	enter_map_.clear();
	leave_map_.clear();
	aoi_map_.clear();
}
