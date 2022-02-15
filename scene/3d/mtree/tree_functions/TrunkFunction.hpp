#pragma once
#include "./base_types/TreeFunction.hpp"
#include <vector>

class Tree3DTrunkFunction : public Tree3DFunction {
public:
	float length = 10;
	float start_radius = .3f;
	float end_radius = .05;
	float shape = .5;
	float resolution = 3.f;
	float randomness = .1f;
	float up_attraction = .6f;

	void execute(std::vector<Tree3DStem> &stems, int id, int parent_id) override;
};
