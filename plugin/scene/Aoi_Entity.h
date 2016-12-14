/*
 *  Created on: Aug 11, 2016
 *      Author: lijunliang
 */

#ifndef AOI_ENTITY_H_
#define AOI_ENTITY_H_

#include <string>
#include <stdint.h>
#include <math.h>
#include <list>
#include <unordered_map>
#include "Base_Struct.h"
#include "Object_Pool.h"

class Aoi_Entity;
class Aoi_Manager;

struct Position {
	int x;
	int y;
	int z;
	Position(int x1, int y1, int z1) {
		x = x1;
		y = y1;
		z = z1;
	}
	void operator = (Position pos) {
		x = pos.x;
		y = pos.y;
		z = pos.z;
	}
};

typedef std::list<Aoi_Entity *> AOI_LIST;
typedef std::unordered_map<int, Aoi_Entity *> AOI_MAP;
typedef Object_Pool<Aoi_Entity, Spin_Lock> Aoi_Entity_Pool;

class Aoi_Entity {
public:
	static Aoi_Entity *create_aoi_entity(int sid, int eid);
	static void reclaim_aoi_entity(Aoi_Entity *entity);
public:
	Aoi_Entity();
	~Aoi_Entity();
	void update_aoi_map(AOI_MAP &new_map);
	void add_aoi_entity(Aoi_Entity *entity);
	void del_aoi_entity(Aoi_Entity *entity);
	void clear_aoi_map();
	void reset();

	inline int sid(){return sid_;}
	inline void sid(int sid){sid_ = sid;}
	inline int eid(){return eid_;}
	inline void eid(int eid){eid_ = eid;}
	inline void aoi_manager(Aoi_Manager *manager){aoi_manager_ = manager;}
	inline Aoi_Manager *aoi_manager(){return aoi_manager_;}
	inline Position &pos(){return pos_;}
	inline Position &opos(){return opos_;}
	inline int radius(){return radius_;}

	inline void x_pos(AOI_LIST::iterator iter){x_pos_ = iter;}
	inline void y_pos(AOI_LIST::iterator iter){y_pos_ = iter;}
	inline AOI_LIST::iterator x_pos(){return x_pos_;}
	inline AOI_LIST::iterator y_pos(){return y_pos_;}
	inline AOI_MAP &enter_map(){return enter_map_;}
	inline AOI_MAP &leave_map(){return leave_map_;}
	inline AOI_MAP &aoi_map(){return aoi_map_;}
private:
	static Aoi_Entity_Pool aoi_entity_pool_;
private:
	int sid_;
	int eid_;
	Aoi_Manager *aoi_manager_;
	Position pos_;
	Position opos_;
	int radius_;
	AOI_LIST::iterator x_pos_;
	AOI_LIST::iterator y_pos_;
	AOI_MAP enter_map_;
	AOI_MAP leave_map_;
	AOI_MAP aoi_map_;
};

#endif
