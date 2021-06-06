#ifndef FBX_LOADER_HPP__
#define FBX_LOADER_HPP__

#include "core/reference.h"
#include "core/ustring.h"
#include "loader.hpp"
#include "scene/3d/mesh_instance.h"
#include "scene/3d/skeleton.h"
#include <fbxsdk.h>
#include <fbxsdk/core/fbxmanager.h>
#include <fbxsdk/fileio/fbximporter.h>
#include <fbxsdk/fileio/fbxiosettings.h>
#include <fbxsdk/scene/fbxscene.h>
// #include "fbxsdk.h"

#include <map>

/**
  @file fbx_loader.hpp
  @brief Holds data structure and utilities to store and parse en FBX file

  This module use the FBX SDK to parse and store meshes/skeleton/skin/scenes.
  The SDK requires a global initialization at the programm startup. This
  is done with Loader::init_fbx_sdk().
  Releasing memory of the SDK is done with Loader::clean_fbx_sdk().

  A specialization of Loader::Base_loader is provided to import/export FBX files
  into an intermediate data representation.
*/

// Forward def to avoid the depencie of FBX SDK in this header

// =============================================================================
namespace Loader {
// =============================================================================

/// InitiLoaderfbx SDK memory manager. This is mandatory to use the SDK
/// and must be done once at the programm startup
/// @see clean()
void init_fbx_sdk();

/// This erase the fbx SDK memory manager and must be called once when the
/// application is closed
void clean_fbx_sdk();

/**
    @class Fbx_file
    @brief main interface for an alias fbx file.
*/
class Fbx_file : public Base_loader {
public:
	//friend class Fbx_anim_eval;
	struct FbxNodeInfo {
		FbxNodeInfo() {
			SkelectID = -1;
			NodeType = FbxNodeAttribute::eUnknown;
		}
		String ObjectName;
		uint64_t UniqueId;
		FbxAMatrix transform;
		FbxNode *SourceNode;
		Transform godot_trans;
		FbxVector4 RotationPivot;
		FbxVector4 ScalePivot;

		String AttributeName;
		uint64_t AttributeUniqueId;
		String AttributeType;
		FbxNodeAttribute::EType NodeType;

		String ParentName;
		uint64_t ParentUniqueId;
		uint64_t SkelectID;
	};
	Vector<FbxNodeInfo> HierarchyInfo;
	Skeleton *FindSkelect(FbxNode *bone_node) {
		int skelectID = -999;
		for (int i = 0; i < HierarchyInfo.size(); ++i) {
			if (HierarchyInfo[i].SourceNode == bone_node) {
				skelectID = HierarchyInfo[i].SkelectID;
			}
		}
		FbxNode *node = nullptr;
		for (int i = 0; i < HierarchyInfo.size(); ++i) {
			if (HierarchyInfo[i].NodeType == FbxNodeAttribute::eSkeleton) {
				node = HierarchyInfo[i].SourceNode;
			}
		}
		Map<FbxNode *, Skeleton *>::Element *ret = AllSkin.find(node);
		if (ret) {
			return ret->value();
		}
		return nullptr;
	}
	Fbx_file(const String &file_name) :
			Base_loader(file_name),
			_fbx_scene(nullptr),
			RetFBXScene(nullptr) {
		import_file(file_name);
	}

	~Fbx_file() { free_mem(); }

	bool import_file(const String &file_path);
	bool export_file(const String &file_path) {
		Base_loader::update_paths(file_path);
		// TODO
		return false;
	}

	/// @return parsed animations or NULL.
	void get_anims(std::vector<Base_anim_eval *> &anims) const {
		get_animations(anims);
	}

	/// Transform internal FBX representation into our skeleton representation
	void get_skeleton(Abs_skeleton &skel) const;

	/// Get fbx animation evaluator. Animation are concatenated in 'anims'
	void get_animations(std::vector<Base_anim_eval *> &anims) const;

	/// transform internal fbx representation into generic representation
	void get_mesh(Abs_mesh &mesh);

	/// transform generic representation into internal fbx representation
	void set_mesh(const Abs_mesh &mesh);

	void free_mem();
	void LoadScene(FbxScene *fbx_scene, Spatial *root_scene);
	void ProcessNode(FbxNode *fbx_node, FbxNode *parent_node, Spatial *root);
	void LoadSkelect(Skeleton *skin, FbxNode *node, FbxNode *root_node);

private:
	/// compute attributes '_offset_verts' and '_size_mesh'
	void compute_size_mesh();

	/// The FBX file data once parsed with load_file()
	FbxScene *_fbx_scene;

	Spatial *RetFBXScene;
	Map<FbxNode *, MeshInstance *> AllMesh;
	Map<FbxNode *, Skeleton *> AllSkin;

	/// Stores for each FBX mesh node the offset introduced in vertices index
	/// because we concatenate meshes
	std::map<const void *, int> _offset_verts;

	/// Nb vertices of all the FBX meshes concatenated
	int _size_mesh;
};

} // namespace Loader

#endif // FBX_LOADER_HPP__
