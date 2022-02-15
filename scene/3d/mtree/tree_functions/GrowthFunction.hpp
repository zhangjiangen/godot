#pragma once
#include "scene/3d/mtree/tree_functions/base_types/TreeFunction.hpp"
#include <vector>

class Tree3DGrowthFunction : public Tree3DFunction {
private:
	float update_vigor_ratio_rec(Tree3DNode &node);
	void update_vigor_rec(Tree3DNode &node, float vigor);
	void simulate_growth_rec(Tree3DNode &node, int id);
	void get_weight_rec(Tree3DNode &node);
	void apply_gravity_rec(Tree3DNode &node, Basis curent_rotation);
	void update_absolute_position_rec(Tree3DNode &node, const Vector3 &node_position);

public:
	int iterations = 5;
	float apical_dominance = .7f;
	float grow_threshold = .5f;
	float split_angle = 60;
	float branch_length = 1;
	float gravitropism = .1f;
	float randomness = .1f;
	float cut_threshold = .2f;
	float split_threshold = .7f;
	float gravity_strength = 1;

	float apical_control = .7f;
	float codominant_proba = .1f;
	int codominant_count = 2;
	float branch_angle = 60;
	float philotaxis_angle = 2.399f;
	float flower_threshold = .5f;

	float growth_delta = .1f;
	float flowering_delta = .1f;

	float root_flux = 5;

	void execute(std::vector<Tree3DStem> &stems, int id, int parent_id) override;
};

class BioNodeInfo : public GrowthInfo {
public:
	enum class NodeType { Meristem,
		Branch,
		Cut,
		Ignored } type;
	float branch_weight = 0;
	Vector3 center_of_mass;
	Vector3 absolute_position;
	float vigor_ratio = 1;
	float vigor = 0;
	int age = 0;
	float philotaxis_angle = 0;

	BioNodeInfo(NodeType type, int age = 0, float philotaxis_angle = 0) {
		this->type = type;
		this->age = age;
		this->philotaxis_angle = philotaxis_angle;
	};
};
