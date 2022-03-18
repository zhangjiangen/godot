#include "register_types.h"
#include "constants/voxel_string_names.h"
#include "edition/voxel_tool.h"
#include "edition/voxel_tool_buffer.h"
#include "edition/voxel_tool_lod_terrain.h"
#include "edition/voxel_tool_terrain.h"
#include "generators/graph/voxel_generator_graph.h"
#include "generators/graph/voxel_graph_node_db.h"
#include "generators/simple/voxel_generator_flat.h"
#include "generators/simple/voxel_generator_heightmap.h"
#include "generators/simple/voxel_generator_image.h"
#include "generators/simple/voxel_generator_noise.h"
#include "generators/simple/voxel_generator_noise_2d.h"
#include "generators/simple/voxel_generator_waves.h"
#include "generators/voxel_generator_script.h"
#include "meshers/blocky/voxel_blocky_library.h"
#include "meshers/blocky/voxel_mesher_blocky.h"
#include "meshers/cubes/voxel_mesher_cubes.h"
#include "meshers/dmc/voxel_mesher_dmc.h"
#include "meshers/transvoxel/voxel_mesher_transvoxel.h"
#include "server/voxel_server_gd.h"
#include "storage/voxel_buffer_gd.h"
#include "storage/voxel_memory_pool.h"
#include "streams/region/voxel_stream_region_files.h"
#include "streams/sqlite/voxel_stream_sqlite.h"
#include "streams/vox_loader.h"
#include "streams/voxel_block_serializer_gd.h"
#include "streams/voxel_stream_block_files.h"
#include "streams/voxel_stream_script.h"
#include "terrain/instancing/voxel_instance_component.h"
#include "terrain/instancing/voxel_instance_library_scene_item.h"
#include "terrain/instancing/voxel_instancer.h"
#include "terrain/voxel_box_mover.h"
#include "terrain/voxel_lod_terrain.h"
#include "terrain/voxel_mesh_block.h"
#include "terrain/voxel_terrain.h"
#include "terrain/voxel_viewer.h"
#include "util/macros.h"
#include "util/noise/fast_noise_lite.h"
#include "util/noise/fast_noise_lite_gradient.h"

#ifdef VOXEL_ENABLE_FAST_NOISE_2
#include "util/noise/fast_noise_2.h"
#endif

#include <core/config/engine.h>

#ifdef TOOLS_ENABLED
#include "editor/editor_plugin.h"
#include "editor/fast_noise_lite/fast_noise_lite_editor_plugin.h"
#include "editor/graph/voxel_graph_editor_plugin.h"
#include "editor/instance_library/voxel_instance_library_editor_plugin.h"
#include "editor/instancer/voxel_instancer_editor_plugin.h"
#include "editor/terrain/voxel_terrain_editor_plugin.h"
#include "editor/vox/vox_editor_plugin.h"
#include "editor/voxel_debug.h"
#ifdef VOXEL_ENABLE_FAST_NOISE_2
#include "editor/fast_noise_2/fast_noise_2_editor_plugin.h"
#endif
#endif // TOOLS_ENABLED

#ifdef VOXEL_RUN_TESTS
#include "tests/tests.h"
#endif

void register_voxel_types() {
	using namespace zylann;
	using namespace voxel;

	VoxelMemoryPool::create_singleton();
	VoxelStringNames::create_singleton();
	VoxelGraphNodeDB::create_singleton();
	VoxelServer::create_singleton();
	gd::VoxelServer::create_singleton();

	Engine::get_singleton()->add_singleton(Engine::Singleton("VoxelServer", gd::VoxelServer::get_singleton()));

	// TODO Can I prevent users from instancing it? is "register_virtual_class" correct for a class that's not abstract?
	ClassDB::register_class<gd::VoxelServer>();

	// Misc
	ClassDB::register_class<VoxelBlockyModel>();
	ClassDB::register_class<VoxelBlockyLibrary>();
	ClassDB::register_class<VoxelColorPalette>();
	ClassDB::register_class<VoxelInstanceLibrary>();
	ClassDB::register_abstract_class<VoxelInstanceLibraryItem>();
	ClassDB::register_class<VoxelInstanceLibraryMultiMeshItem>();
	ClassDB::register_class<VoxelInstanceLibrarySceneItem>();
	ClassDB::register_class<VoxelDataBlockEnterInfo>();

	// Storage
	ClassDB::register_class<gd::VoxelBuffer>();

	// Nodes
	ClassDB::register_abstract_class<VoxelNode>();
	ClassDB::register_class<VoxelTerrain>();
	ClassDB::register_class<VoxelLodTerrain>();
	ClassDB::register_class<VoxelViewer>();
	ClassDB::register_class<VoxelInstanceGenerator>();
	ClassDB::register_class<VoxelInstancer>();
	ClassDB::register_class<VoxelInstanceComponent>();

	// Streams
	ClassDB::register_abstract_class<VoxelStream>();
	ClassDB::register_class<VoxelStreamBlockFiles>();
	ClassDB::register_class<VoxelStreamRegionFiles>();
	ClassDB::register_class<VoxelStreamScript>();
	ClassDB::register_class<VoxelStreamSQLite>();

	// Generators
	ClassDB::register_abstract_class<VoxelGenerator>();
	ClassDB::register_class<VoxelGeneratorFlat>();
	ClassDB::register_class<VoxelGeneratorWaves>();
	ClassDB::register_abstract_class<VoxelGeneratorHeightmap>();
	ClassDB::register_class<VoxelGeneratorImage>();
	ClassDB::register_class<VoxelGeneratorNoise2D>();
	ClassDB::register_class<VoxelGeneratorNoise>();
	ClassDB::register_class<VoxelGeneratorGraph>();
	ClassDB::register_class<VoxelGeneratorScript>();

	// Utilities
	ClassDB::register_class<VoxelBoxMover>();
	ClassDB::register_class<VoxelRaycastResult>();
	ClassDB::register_abstract_class<VoxelTool>();
	ClassDB::register_abstract_class<VoxelToolTerrain>();
	ClassDB::register_abstract_class<VoxelToolLodTerrain>();
	// I had to bind this one despite it being useless as-is because otherwise Godot lazily initializes its class.
	// And this can happen in a thread, causing crashes due to the concurrent access
	ClassDB::register_abstract_class<VoxelToolBuffer>();
	ClassDB::register_class<gd::VoxelBlockSerializer>();
	ClassDB::register_class<VoxelVoxLoader>();
	ClassDB::register_class<FastNoiseLite>();
	ClassDB::register_class<FastNoiseLiteGradient>();
	// See SCsub
#ifdef VOXEL_ENABLE_FAST_NOISE_2
	ClassDB::register_class<FastNoise2>();
#endif

	// Meshers
	ClassDB::register_abstract_class<VoxelMesher>();
	ClassDB::register_class<VoxelMesherBlocky>();
	ClassDB::register_class<VoxelMesherTransvoxel>();
	ClassDB::register_class<VoxelMesherDMC>();
	ClassDB::register_class<VoxelMesherCubes>();

	// Reminder: how to create a singleton accessible from scripts:
	// Engine::get_singleton()->add_singleton(Engine::Singleton("SingletonName",singleton_instance));

	// Reminders
	// PRINT_VERBOSE(String("Size of Variant: {0}").format(varray((int)sizeof(Variant))));
	// PRINT_VERBOSE(String("Size of Object: {0}").format(varray((int)sizeof(Object))));
	// PRINT_VERBOSE(String("Size of RefCounted: {0}").format(varray((int)sizeof(RefCounted))));
	// PRINT_VERBOSE(String("Size of Node: {0}").format(varray((int)sizeof(Node))));
	// PRINT_VERBOSE(String("Size of Node3D: {0}").format(varray((int)sizeof(Node3D))));
	// PRINT_VERBOSE(String("Size of gd::VoxelBuffer: {0}").format(varray((int)sizeof(gd::VoxelBuffer))));
	// PRINT_VERBOSE(String("Size of VoxelBufferInternal: {0}").format(varray((int)sizeof(VoxelBufferInternal))));
	// PRINT_VERBOSE(String("Size of VoxelMeshBlock: {0}").format(varray((int)sizeof(VoxelMeshBlock))));
	// PRINT_VERBOSE(String("Size of VoxelTerrain: {0}").format(varray((int)sizeof(VoxelTerrain))));
	// PRINT_VERBOSE(String("Size of VoxelLodTerrain: {0}").format(varray((int)sizeof(VoxelLodTerrain))));
	// PRINT_VERBOSE(String("Size of VoxelInstancer: {0}").format(varray((int)sizeof(VoxelInstancer))));
	// PRINT_VERBOSE(String("Size of VoxelDataMap: {0}").format(varray((int)sizeof(VoxelDataMap))));

#ifdef TOOLS_ENABLED
	EditorPlugins::add_by_type<VoxelGraphEditorPlugin>();
	EditorPlugins::add_by_type<VoxelTerrainEditorPlugin>();
	EditorPlugins::add_by_type<VoxelInstanceLibraryEditorPlugin>();
	EditorPlugins::add_by_type<FastNoiseLiteEditorPlugin>();
	EditorPlugins::add_by_type<magica::VoxEditorPlugin>();
	EditorPlugins::add_by_type<VoxelInstancerEditorPlugin>();
#ifdef VOXEL_ENABLE_FAST_NOISE_2
	EditorPlugins::add_by_type<FastNoise2EditorPlugin>();
#endif
#endif // TOOLS_ENABLED

#ifdef VOXEL_RUN_TESTS
	zylann::voxel::tests::run_voxel_tests();
#endif

	// Compatibility with older version
	ClassDB::add_compatibility_class("VoxelLibrary", "VoxelBlockyLibrary");
	ClassDB::add_compatibility_class("Voxel", "VoxelBlockyModel");
	ClassDB::add_compatibility_class("VoxelInstanceLibraryItem", "VoxelInstanceLibraryMultiMeshItem");
	// Not possible to add a compat class for this one because the new name is indistinguishable from an old one.
	// However this is an abstract class so it should not be found in resources hopefully
	//ClassDB::add_compatibility_class("VoxelInstanceLibraryItemBase", "VoxelInstanceLibraryItem");
}

void unregister_voxel_types() {
	using namespace zylann;
	using namespace voxel;

	// At this point, the GDScript module has nullified GDScriptLanguage::singleton!!
	// That means it's impossible to free scripts still referenced by VoxelServer. And that can happen, because
	// users can write custom generators, which run inside threads, and these threads are hosted in the server...
	// See https://github.com/Zylann/godot_voxel/issues/189

	VoxelStringNames::destroy_singleton();
	VoxelGraphNodeDB::destroy_singleton();
	gd::VoxelServer::destroy_singleton();
	VoxelServer::destroy_singleton();

	// Do this last as VoxelServer might still be holding some refs to voxel blocks
	unsigned int used_blocks = VoxelMemoryPool::get_singleton()->debug_get_used_blocks();
	if (used_blocks > 0) {
		ERR_PRINT(String("VoxelMemoryPool: "
						 "{0} memory blocks are still used when unregistering the module. Recycling leak?")
						  .format(varray(used_blocks)));
	}
	VoxelMemoryPool::destroy_singleton();
	// TODO No remove?

#ifdef TOOLS_ENABLED
	zylann::free_debug_resources();

	// TODO Seriously, no remove?
	//EditorPlugins::remove_by_type<VoxelGraphEditorPlugin>();
#endif
}
