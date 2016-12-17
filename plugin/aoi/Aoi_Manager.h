/*
 *  Created on: Aug 10, 2016
 *      Author: lijunliang
 */

#ifndef AOI_MANAGER_H_
#define AOI_MANAGER_H_

#include "Object_Pool.h"
#include "Aoi_Entity.h"

class Aoi_Manager;
typedef std::unordered_map<int, Aoi_Manager *> AOI_MANAGER_MAP;
typedef std::unordered_map<int, Aoi_Entity *> AOI_ENTITY_MAP;

class Aoi_Manager {
public:
	static bool create_aoi_manager(int id);
	static Aoi_Manager *get_aoi_manager(int id);
	static Aoi_Entity *find_entity(int sid);
	static void add_entity(Aoi_Entity *entity);
	static void rmv_entity(Aoi_Entity *entity);
public:
	Aoi_Manager(int id);
	~Aoi_Manager();
	int on_enter_aoi(Aoi_Entity *entity);
	int on_update_aoi(Aoi_Entity *entity);
	int on_leave_aoi(Aoi_Entity *entity);

private:
	void insert_entity(Aoi_Entity *entity);
	void update_list(Aoi_Entity *entity, bool direct, int xy);
	void update_aoi_map(Aoi_Entity *entity);
	void print_list(AOI_LIST list);
private:
	static AOI_MANAGER_MAP aoi_manager_map_;
	static AOI_ENTITY_MAP aoi_entity_map_;
private:
	int mgr_id_;
	AOI_LIST x_list_;
	AOI_LIST y_list_;
};

#endif
