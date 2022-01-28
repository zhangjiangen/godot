#pragma once
#include "GrowthInfo.hpp"
#include <memory>
#include <vector>

struct Tree3DNodeChild;

class Tree3DNode {
public:
	std::vector<std::shared_ptr<Tree3DNodeChild>> children;
	Vector3 direction;
	Vector3 tangent;
	float length;
	float radius;
	bool can_spawn_leaf = true;
	int creator_id = 0;
	std::unique_ptr<GrowthInfo> growthInfo = nullptr;

	bool is_leaf() const;

	Tree3DNode(Vector3 direction, Vector3 parent_tangent, float length, float radius, bool is_spawn_leaf, int creator_id);
};

struct Tree3DNodeChild {
	Tree3DNode node;
	float position_in_parent;
};

struct Tree3DStem {
	Tree3DNode node;
	Vector3 position;
};
