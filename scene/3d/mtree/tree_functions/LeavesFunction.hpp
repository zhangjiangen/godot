#pragma once
#include "./base_types/TreeFunction.hpp"
#include "core/object/object.h"
#include <vector>

namespace Mtree {
class LeavesFunction : public TreeFunction {
public:
	enum LeafType {
		Palmate,
		Serrate,
		Palmatisate,
		Custom,

	};
	float length = 10;
	float start_radius = .3f;
	float end_radius = .05;
	float shape = .5;
	float resolution = 3.f;
	float randomness = .1f;
	float gravity_strength = 10;
	float flatten = 0.5f;
	float leaf_size = 0.1f;
	LeafType leaf_type = Palmate;

	void execute(std::vector<Stem> &stems, int id, int parent_id) override;
};

} //namespace Mtree