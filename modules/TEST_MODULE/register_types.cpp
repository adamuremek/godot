#include "register_types.h"
#include "core/config/engine.h"
#include "includes.h"
#include "world.h"

void initialize_TEST_MODULE_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	rDebug::log = [](const char* msg)->void{
		print_line(msg);
	};

	ClassDB::register_class<World>();
	// Create world singleton
	memnew(World);
	Engine::get_singleton()->add_singleton(Engine::Singleton("World", World::get_singleton()));

}

void uninitialize_TEST_MODULE_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	// Delete world singleton
	World* world_singleton = World::get_singleton();
	if(world_singleton){
		memdelete(world_singleton);
	}
}