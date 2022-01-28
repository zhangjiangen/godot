#pragma once
#include <vector>
// #include <queue>
#include "./base_types/TreeFunction.hpp"
// #include "scene/3d/mtree/utilities/NodeUtilities.hpp"
// #include "scene/3d/mtree/utilities/GeometryUtilities.hpp"
#include "scene/3d/mtree/tree_functions/base_types/Property.hpp"

class Tree3DPipeRadiusFunction : public Tree3DFunction {
private:
	void update_radius_rec(Tree3DNode &node);

public:
	float power = 2.f;
	float end_radius = .01f;
	float constant_growth = .01f;
	void execute(std::vector<Tree3DStem> &stems, int id, int parent_id) override;
};
