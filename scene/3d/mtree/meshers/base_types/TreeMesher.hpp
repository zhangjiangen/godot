#pragma once
#include "scene/3d/mtree/mesh/Mesh.hpp"
#include "scene/3d/mtree/tree/Tree.hpp"

class TreeMesher {
public:
	virtual Tree3DMesh mesh_tree(Tree3D &tree) = 0;
	virtual ~TreeMesher(){}
};
