#pragma once
#include "./base_types/TreeFunction.hpp"
#include "core/object/object.h"
#include "scene/3d/mtree/tree_functions/base_types/Property.hpp"
#include <vector>

class Tree3DLeavesFunction : public Tree3DFunction {
public:
	enum LeafType {
		Palmate,
		Serrate,
		Palmatisate,
		Custom,

	};
	float length = 10;
	//float start_radius = .3f;
	PropertyWrapper start_radius{ ConstantProperty(.3) }; // 0 > x > 1
	float end_radius = .05;
	float shape = .5;
	float resolution = 3.f;
	PropertyWrapper randomness{ ConstantProperty(.1) };
	float gravity_strength = 10;
	float flatten = 0.5f;
	float leaf_size = 0.1f;
	LeafType leaf_type = Palmate;

	void execute(std::vector<Tree3DStem> &stems, int id, int parent_id) override;
};
