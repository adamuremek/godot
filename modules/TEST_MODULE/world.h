#ifndef GODOT_WORLD_H
#define GODOT_WORLD_H

#include "includes.h"

class World : public Object{
	GDCLASS(World, Object);

private:
	rWorld* rWorldInstance;
protected:
	static World* singleton;
	static void _bind_methods();
public:
	World();

	~World();

	static World* get_singleton();

	void start_world(Port port);

	void stop_world();

	void join_world(String world, Port port);
};

#endif //GODOT_WORLD_H
