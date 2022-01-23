#pragma once
#include "scene/3d/mtree/mesh/Mesh.hpp"
#include "scene/3d/mtree/tree/Tree.hpp"

namespace Mtree {
class TreeMesher {
public:
	virtual Mesh mesh_tree(Tree &tree) = 0;
};
} //namespace Mtree
