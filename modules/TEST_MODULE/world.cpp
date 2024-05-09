#include "world.h"

World* World::singleton = nullptr;

World::World() : rWorldInstance(memnew(rWorld)){
	singleton = this;
}

World::~World(){
	memdelete(rWorldInstance);
	singleton = nullptr;
}

void World::_bind_methods() {
	ClassDB::bind_method(D_METHOD("start_world", "port"), &World::start_world);
	ClassDB::bind_method(D_METHOD("stop_world"), &World::stop_world);
	ClassDB::bind_method(D_METHOD("join_world", "world", "port"), &World::join_world);
}

World *World::get_singleton() { return singleton;}
void World::start_world(Port port) { rWorldInstance->startWorld(port); }
void World::stop_world() { rWorldInstance->stopWorld(); }
void World::join_world(String world, Port port) {
	CharString utf8String = world.utf8();
	rWorldInstance->joinWorld(utf8String.get_data(), port);
}
