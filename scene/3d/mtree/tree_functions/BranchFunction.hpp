#pragma once
#include "./base_types/TreeFunction.hpp"
#include "scene/3d/mtree/tree_functions/base_types/Property.hpp"
#include "scene/3d/mtree/utilities/GeometryUtilities.hpp"
#include "scene/3d/mtree/utilities/NodeUtilities.hpp"
#include <queue>
#include <vector>

namespace Mtree {
class BranchFunction : public TreeFunction {
public:
	float start = .1;
	float end = 1;
	float branches_density = 2; // 0 < x
	PropertyWrapper length{ ConstantProperty(9) }; // x > 0
	PropertyWrapper start_radius{ ConstantProperty(.4) }; // 0 > x > 1
	float end_radius = .05;
	float break_chance = .01; // 0 < x
	float resolution = 3; // 0 < x
	PropertyWrapper randomness{ ConstantProperty(.4) };
	float phillotaxis = 137.5f;
	float gravity_strength = 10;
	float stiffness = .1;
	float up_attraction = .25;
	float flatness = .5; // 0 < x  < 1
	float split_radius = .9f; // 0 < x < 1
	PropertyWrapper start_angle{ ConstantProperty(45) }; // -180 < x < 180
	float split_angle = 45.0f;
	float split_proba = .5f; // 0 < x
	// 是否允许生长树叶
	bool can_spawn_leafs = true;

	void execute(std::vector<Stem> &stems, int id, int parent_id) override;

	class BranchGrowthInfo : public GrowthInfo {
	public:
		float desired_length;
		float current_length;
		float origin_radius;
		float cumulated_weight = 0;
		float deviation_from_rest_pose = 0;
		float age = 0;
		bool inactive = false;
		Vector3 position;
		BranchGrowthInfo(float desired_length, float origin_radius, Vector3 position, float current_length = 0, float deviation = 0) :
				desired_length(desired_length), current_length(current_length), origin_radius(origin_radius), deviation_from_rest_pose(deviation), position(position){};
	};

private:
	std::vector<std::reference_wrapper<Node>> get_origins(std::vector<Stem> &stems, const int id, const int parent_id);

	void grow_origins(std::vector<std::reference_wrapper<Node>> &, const int id);

	void grow_node_once(Node &node, const int id, std::queue<std::reference_wrapper<Node>> &results);

	void apply_gravity_to_branch(Node &node);

	void apply_gravity_rec(Node &node, Quaternion previous_rotations);

	void update_weight_rec(Node &node);
};

} //namespace Mtree
