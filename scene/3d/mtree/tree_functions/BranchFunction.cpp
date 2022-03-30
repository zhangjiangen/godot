#include <iostream>
#include <queue>

#include "BranchFunction.hpp"
#include "scene/3d/mtree/utilities/GeometryUtilities.hpp"
#include "scene/3d/mtree/utilities/NodeUtilities.hpp"

namespace {
void update_positions_rec(Tree3DNode &node, const Vector3 &position) {
	auto &info = static_cast<Tree3DBranchFunction::BranchGrowthInfo &>(*node.growthInfo);
	info.position = position;

	for (auto &child : node.children) {
		Vector3 child_position = position + node.direction * node.length * child->position_in_parent;
		update_positions_rec(child->node, child_position);
	}
}

bool avoid_floor(const Vector3 &node_position, Vector3 &node_direction, float parent_length) // return true if branch should be terminated
{
	if (node_direction.z < 0) {
		node_direction[2] -= node_direction[2] * 2 / (2 + node_position.z);
	}
	return (node_position + node_direction).z * parent_length * 4 < 0; // is node heading to floor too fast
}

Vector3 get_main_child_direction(Tree3DNode &parent, const Vector3 &parent_position, const float up_attraction, const float flatness, const float randomness, const float resolution, bool &should_terminate) {
	Vector3 random_dir = Tree3DGeometry::random_vec(flatness).normalized() + Vector3{ 0, 0, 1 } * up_attraction;
	Vector3 child_direction = parent.direction + random_dir * randomness / resolution;
	should_terminate = avoid_floor(parent_position, child_direction, parent.length);
	child_direction.normalize();
	return child_direction;
}

Vector3 get_split_direction(const Tree3DNode &parent, const Vector3 &parent_position, const float up_attraction, const float flatness, const float resolution, const float angle) {
	Vector3 child_direction = Tree3DGeometry::random_vec();
	child_direction = child_direction.cross(parent.direction) + Vector3{ 0, 0, 1 } * up_attraction * flatness;
	Vector3 flat_normal = Vector3{ 0, 0, 1 }.cross(parent.direction).cross(parent.direction).normalized();
	child_direction -= child_direction.dot(flat_normal) * flatness * flat_normal;
	avoid_floor(parent_position, child_direction, parent.length);
	child_direction = Tree3DGeometry::lerp(parent.direction, child_direction, angle / 90); // TODO use slerp for correct angle
	child_direction.normalize();
	return child_direction;
}

void mark_inactive(Tree3DNode &node) {
	auto &info = static_cast<Tree3DBranchFunction::BranchGrowthInfo &>(*node.growthInfo);
	info.inactive = true;
}

bool propagate_inactive_rec(Tree3DNode &node) {
	auto *info = dynamic_cast<Tree3DBranchFunction::BranchGrowthInfo *>(node.growthInfo.get());

	if (node.children.size() == 0 || info->inactive)
		return info->inactive;

	bool inactive = false;
	for (size_t i = 0; i < node.children.size(); i++) {
		if (propagate_inactive_rec(node.children[i]->node))
			inactive = true;
	}
	info->inactive = inactive;
	return inactive;
}
} //namespace

void Tree3DBranchFunction::apply_gravity_to_branch(Tree3DNode &branch_origin) {
	propagate_inactive_rec(branch_origin);
	update_weight_rec(branch_origin);
	apply_gravity_rec(branch_origin, Quaternion());
	BranchGrowthInfo &info = static_cast<BranchGrowthInfo &>(*branch_origin.growthInfo);
	update_positions_rec(branch_origin, info.position);
}

void Tree3DBranchFunction::apply_gravity_rec(Tree3DNode &node, Quaternion curent_rotation) {
	BranchGrowthInfo &info = static_cast<BranchGrowthInfo &>(*node.growthInfo);
	if (!info.inactive || true) {
		float horizontality = 1 - abs(node.direction.z);
		info.age += 1 / resolution;
		float displacement = horizontality * std::pow(info.cumulated_weight, .5f) * gravity_strength / resolution / resolution / 1000 / (1 + info.age);
		displacement *= std::exp(-std::abs(info.deviation_from_rest_pose / resolution * stiffness));
		info.deviation_from_rest_pose += displacement;

		Vector3 tangent = node.direction.cross(Vector3{ 0, 0, -1 }).normalized();
		Quaternion rot = Quaternion(tangent, displacement);
		curent_rotation = rot * curent_rotation;

		node.direction = curent_rotation.xform(node.direction);
	}

	for (auto &child : node.children) {
		apply_gravity_rec(child->node, curent_rotation);
	}
}

void Tree3DBranchFunction::update_weight_rec(Tree3DNode &node) {
	float node_weight = node.length;
	for (auto &child : node.children) {
		update_weight_rec(child->node);
		BranchGrowthInfo &child_info = dynamic_cast<BranchGrowthInfo &>(*child->node.growthInfo);
		node_weight += child_info.cumulated_weight;
	}

	BranchGrowthInfo *info = dynamic_cast<BranchGrowthInfo *>(node.growthInfo.get());
	info->cumulated_weight = node_weight;
}

// grow extremity by one level (add one or more children)
// 将 extremity 增长一级（添加一个或多个子级）
void Tree3DBranchFunction::grow_node_once(Tree3DNode &node, const int id, std::queue<std::reference_wrapper<Tree3DNode>> &results) {
	bool break_branch = rand_gen.get_0_1() * resolution < break_chance;
	if (break_branch) {
		mark_inactive(node);
		return;
	}

	BranchGrowthInfo &info = static_cast<BranchGrowthInfo &>(*node.growthInfo);
	float factor_in_branch = info.current_length / info.desired_length;

	float child_radius = Tree3DGeometry::lerp(info.origin_radius, info.origin_radius * end_radius, factor_in_branch);
	float child_length = std::min(1 / resolution, info.desired_length - info.current_length);
	bool should_terminate;
	Vector3 child_direction = get_main_child_direction(node, info.position, up_attraction, flatness, randomness.execute(factor_in_branch), resolution, should_terminate);

	if (should_terminate) {
		mark_inactive(node);
		return;
	}

	Tree3DNodeChild child{ Tree3DNode{ child_direction, node.tangent, child_length, child_radius, can_spawn_leafs, id ,nullptr }, 1 };
	node.children.push_back(std::make_shared<Tree3DNodeChild>(std::move(child)));
	auto &child_node = node.children.back()->node;

	float current_length = info.current_length + child_length;
	Vector3 child_position = info.position + child_direction * child_length;
	BranchGrowthInfo child_info{ info.desired_length, info.origin_radius, child_position, current_length };
	child_node.growthInfo = std::make_unique<BranchGrowthInfo>(child_info);
	if (current_length < info.desired_length) {
		results.push(std::ref<Tree3DNode>(child_node));
	}

	bool split = rand_gen.get_0_1() * resolution < split_proba; // should the node split into two children
	if (split) {
		Vector3 split_child_direction = get_split_direction(node, info.position, up_attraction, flatness, resolution, split_angle);
		float split_child_radius = node.radius * split_radius;

		Tree3DNodeChild child{ Tree3DNode{ split_child_direction, node.tangent, child_length, split_child_radius, can_spawn_leafs, id ,nullptr}, rand_gen.get_0_1() };
		node.children.push_back(std::make_shared<Tree3DNodeChild>(std::move(child)));
		auto &cn = node.children.back()->node;

		Vector3 split_child_position = info.position + split_child_direction * child_length;
		BranchGrowthInfo child_info{ info.desired_length, info.origin_radius * split_radius, split_child_position, current_length };
		cn.growthInfo = std::make_unique<BranchGrowthInfo>(child_info);
		if (current_length < info.desired_length) {
			results.push(std::ref<Tree3DNode>(cn));
		}
	}
}

void Tree3DBranchFunction::grow_origins(std::vector<std::reference_wrapper<Tree3DNode>> &origins, const int id) {
	std::queue<std::reference_wrapper<Tree3DNode>> extremities;
	for (auto &node_ref : origins) {
		extremities.push(node_ref);
	}
	int batch_size = extremities.size();
	while (!extremities.empty()) {
		if (batch_size == 0) {
			batch_size = extremities.size();
			for (auto &node_ref : origins) {
				apply_gravity_to_branch(node_ref.get());
			}
		}
		auto &node = extremities.front().get();
		extremities.pop();
		grow_node_once(node, id, extremities);
		batch_size--;
	}
}

// get the origins of the branches that will be created.
// origins are created from the nodes made by the parent Tree3DFunction
// 获取将要创建的分支的来源。
// 起源是从父 Tree3DFunction 创建的节点创建的
std::vector<std::reference_wrapper<Tree3DNode>> Tree3DBranchFunction::get_origins(std::vector<Tree3DStem> &stems, const int id, const int parent_id) {
	// get all nodes created by the parent Tree3DFunction, organised by branch
	// 获取父TreeFunction创建的所有节点，按分支组织
	BranchSelection selection = select_from_tree(stems, parent_id);
	std::vector<std::reference_wrapper<Tree3DNode>> origins;

	// 两个连续原点之间的距离
	float origins_dist = 1 / (branches_density + .001); // distance between two consecutive origins

	for (auto &branch : selection) // parent branches
	{
		if (branch.size() == 0) {
			continue;
		}

		float branch_length = get_branch_length(*branch[0].node);
		// 我们可以开始添加新分支起点的长度
		float absolute_start = start * branch_length; // the length at which we can start adding new branch origins
		// 我们停止添加新分支起点的长度
		float absolute_end = end * branch_length; // the length at which we stop adding new branch origins
		float current_length = 0;
		float dist_to_next_origin = absolute_start;
		Vector3 tangent = Tree3DGeometry::get_orthogonal_vector(branch[0].node->direction);

		for (size_t node_index = 0; node_index < branch.size(); node_index++) {
			auto &node = *branch[node_index].node;
			Vector3 node_position = branch[node_index].node_position;
			// 不能添加子节点，因为它会“继续”分支而不是拆分
			if (node.children.size() == 0) // cant add children since it would "continue" the branch and not ad a split
			{
				continue;
			}
			auto rot = Quaternion(node.direction, (phillotaxis + (rand_gen.get_0_1() - .5) * 2) / 180 * M_PI);
			if (dist_to_next_origin > node.length) {
				dist_to_next_origin -= node.length;
				current_length += node.length;
			} else {
				float remaining_node_length = node.length - dist_to_next_origin;
				current_length += dist_to_next_origin;
				int origins_to_create = remaining_node_length / origins_dist + 1; // number of origins to create on the node
				float position_in_parent = dist_to_next_origin / node.length; // position of the first origin within the node
				float position_in_parent_step = origins_dist / node.length; // relative distance between origins within the node

				for (int i = 0; i < origins_to_create; i++) {
					if (current_length > absolute_end) {
						break;
					}
					float factor = (current_length - absolute_start) / std::max(0.001f, absolute_end - absolute_start);
					tangent = rot.xform(tangent);
					Tree3DGeometry::project_on_plane(tangent, node.direction);
					tangent.normalize();
					Vector3 child_direction = Tree3DGeometry::lerp(node.direction, tangent, start_angle.execute(factor) / 90);
					child_direction.normalize();
					float child_radius = node.radius * start_radius.execute(factor);
					float branch_len = length.execute(factor);
					float node_length = std::min(branch_len, 1 / (resolution + 0.001f));
					Tree3DNodeChild child{ Tree3DNode{ child_direction, node.tangent, node_length, child_radius, can_spawn_leafs, id }, position_in_parent };
					node.children.push_back(std::make_shared<Tree3DNodeChild>(std::move(child)));
					auto &child_node = node.children.back()->node;
					Vector3 child_position = node_position + node.direction * node.length * position_in_parent;
					child_node.growthInfo = std::make_unique<BranchGrowthInfo>(branch_len - node_length, child_radius, child_position, child_node.length, 0);

					if (branch_len - node_length > 1e-3)
						origins.push_back(std::ref(child_node));
					position_in_parent += position_in_parent_step;
					if (i > 0) {
						current_length += origins_dist;
					}
				}
				remaining_node_length = (remaining_node_length - (origins_to_create - 1) * origins_dist);
				dist_to_next_origin = origins_dist - remaining_node_length;
			}
		}
	}
	return origins;
}

void Tree3DBranchFunction::execute(std::vector<Tree3DStem> &stems, int id, int parent_id) {
	rand_gen.set_seed(seed);
	auto origins = get_origins(stems, id, parent_id);
	grow_origins(origins, id);
	execute_children(stems, id);
}
