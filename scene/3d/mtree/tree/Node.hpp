#pragma once
#include "GrowthInfo.hpp"
#include <memory>
#include <vector>

namespace Mtree {

struct NodeChild;

class Node {
public:
	std::vector<std::shared_ptr<NodeChild>> children;
	Vector3 direction;
	Vector3 tangent;
	float length;
	float radius;
	bool can_spawn_leaf = true;
	int creator_id = 0;
	std::unique_ptr<GrowthInfo> growthInfo = nullptr;

	bool is_leaf() const;

	Node(Vector3 direction, Vector3 parent_tangent, float length, float radius, bool is_spawn_leaf, int creator_id);
};

struct NodeChild {
	Node node;
	float position_in_parent;
};

struct Stem {
	Node node;
	Vector3 position;
};
} //namespace Mtree