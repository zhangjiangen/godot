#include "LeavesFunction.hpp"
#include "scene/3d/mtree/utilities/GeometryUtilities.hpp"
struct LeavesCandidates {
	Vector3 position;
	Vector3 direction;
	float length;
	float radius;
	bool is_end;
};
// 获取树叶创建的结点信息
void get_leaf_candidates(Vector3 base_pos, Tree3DNode &n, std::vector<LeavesCandidates> &stems, float max_radius) {
	if (n.radius <= max_radius && n.can_spawn_leaf) {
		bool is_end = n.is_leaf();
		Vector3 direction = n.direction;
		float l = 1;
		if (!is_end) {
			auto &child = n.children[0];
			Vector3 d = child->node.direction * child->node.length;
			l = n.length;
			if (l != 0) {
				direction = d.normalized();
			}
		}

		LeavesCandidates can = { base_pos, direction, l, n.radius, is_end };
		if (n.is_leaf()) {
			stems.push_back(can);
		}
	}
	for (int i = 0; i < n.children.size(); ++i) {
		auto &child = n.children[i];
		get_leaf_candidates(base_pos + child->node.direction * child->node.length, child->node, stems, max_radius);
	}
}
void get_leaf_candidates(Tree3DStem &s, std::vector<LeavesCandidates> &stems, float max_radius) {
	get_leaf_candidates(s.position, s.node, stems, max_radius);
}

void Tree3DLeavesFunction::execute(std::vector<Tree3DStem> &stems, int id, int parent_id) {
	rand_gen.set_seed(seed);
}
