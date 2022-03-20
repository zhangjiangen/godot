#pragma once
#include "GrowthFunction.hpp"
#include "./base_types/TreeFunction.hpp"
#include "scene/3d/mtree/utilities/GeometryUtilities.hpp"
#include <math.h>
#include <iostream>
#include <vector>

void setup_growth_information_rec(Tree3DNode &node) {
	node.growthInfo = std::make_unique<BioNodeInfo>(node.children.size() == 0 ? BioNodeInfo::NodeType::Meristem : BioNodeInfo::NodeType::Ignored);
	for (auto &child : node.children)
		setup_growth_information_rec(child->node);
}

// get total amount of energy from the node and its descendance, and assign for each node the realtive amount of energy it receive
// 从节点及其后代获取总能量，并为每个节点分配它接收到的实际能量
float Tree3DGrowthFunction::update_vigor_ratio_rec(Tree3DNode &node) {
	BioNodeInfo &info = static_cast<BioNodeInfo &>(*node.growthInfo);
	if (info.type == BioNodeInfo::NodeType::Meristem) {
		return 1;
	} else if (info.type == BioNodeInfo::NodeType::Branch || info.type == BioNodeInfo::NodeType::Ignored) {
		float light_flux = update_vigor_ratio_rec(node.children[0]->node);
		float vigor_ratio = 1;
		for (size_t i = 1; i < node.children.size(); i++) {
			float child_flux = update_vigor_ratio_rec(node.children[i]->node);
			float t = apical_dominance;
			vigor_ratio = (t * light_flux) / (t * light_flux + (1 - t) * child_flux + .001f);
			static_cast<BioNodeInfo *>(node.children[i]->node.growthInfo.get())->vigor_ratio = 1 - vigor_ratio;
			light_flux += child_flux;
		}
		static_cast<BioNodeInfo *>(node.children[0]->node.growthInfo.get())->vigor_ratio = vigor_ratio;
		return light_flux;
	} else {
		info.vigor_ratio = 0;
		return 0;
	}
}

// update the amount of energy available to a node
// 更新节点可用的能量
void Tree3DGrowthFunction::update_vigor_rec(Tree3DNode &node, float vigor) {
	BioNodeInfo &info = static_cast<BioNodeInfo &>(*node.growthInfo);
	info.vigor = vigor;
	for (auto &child : node.children) {
		float child_vigor = static_cast<BioNodeInfo *>(child->node.growthInfo.get())->vigor_ratio * vigor;
		update_vigor_rec(child->node, child_vigor);
	}
}

// apply rules on the node based on the energy available to it
// 根据节点可用的能量在节点上应用规则
void Tree3DGrowthFunction::simulate_growth_rec(Tree3DNode &node, int id) {
	BioNodeInfo &info = static_cast<BioNodeInfo &>(*node.growthInfo);
	bool primary_growth = info.type == BioNodeInfo::NodeType::Meristem && info.vigor > grow_threshold;
	bool secondary_growth = info.vigor > grow_threshold && info.type != BioNodeInfo::NodeType::Ignored; // Todo : should be another parameter
	bool split = info.type == BioNodeInfo::NodeType::Meristem && info.vigor > split_threshold;
	bool cut = info.type == BioNodeInfo::NodeType::Meristem && info.vigor < cut_threshold;
	int child_count = node.children.size();
	if (cut && false) {
		info.type = BioNodeInfo::NodeType::Cut;
		return;
	}
	info.age++;
	if (secondary_growth) {
		node.radius = (1 - std::exp(-info.age * .01f) + .01f) * .5;
	}
	if (primary_growth) {
		Vector3 child_direction = node.direction + Vector3{ 0, 0, 1 } * gravitropism + Tree3DGeometry::random_vec() * randomness;
		child_direction.normalize();
		float child_radius = node.radius;
		float child_length = branch_length * (info.vigor + .1f);
		Tree3DNodeChild child = Tree3DNodeChild{ Tree3DNode{ child_direction, node.tangent, branch_length, child_radius, node.can_spawn_leaf, id }, 1 };
		float child_angle = split ? info.philotaxis_angle + philotaxis_angle : info.philotaxis_angle;
		child.node.growthInfo = std::make_unique<BioNodeInfo>(BioNodeInfo::NodeType::Meristem, 0, child_angle);
		node.children.push_back(std::make_shared<Tree3DNodeChild>(std::move(child)));
		info.type = BioNodeInfo::NodeType::Branch;
	}
	if (split) {
		info.philotaxis_angle += philotaxis_angle;
		Vector3 tangent{ std::cos(info.philotaxis_angle), std::sin(info.philotaxis_angle), 0 };
		tangent = Tree3DGeometry::get_look_at_rot(node.direction).xform(tangent);
		Vector3 child_direction = Tree3DGeometry::lerp(node.direction, tangent, split_angle / 90);
		child_direction.normalize();
		float child_radius = node.radius;
		float child_length = branch_length * (info.vigor + .1f);
		Tree3DNodeChild child = Tree3DNodeChild{ Tree3DNode{ child_direction, node.tangent, branch_length, child_radius, node.can_spawn_leaf, id }, 1 };
		child.node.growthInfo = std::make_unique<BioNodeInfo>(BioNodeInfo::NodeType::Meristem);
		node.children.push_back(std::make_shared<Tree3DNodeChild>(std::move(child)));
		info.type = BioNodeInfo::NodeType::Branch;
	}
	for (int i = 0; i < child_count; i++) {
		simulate_growth_rec(node.children[i]->node, id);
	}
}

void Tree3DGrowthFunction::get_weight_rec(Tree3DNode &node) {
	BioNodeInfo &info = static_cast<BioNodeInfo &>(*node.growthInfo);
	for (auto &child : node.children) {
		get_weight_rec(child->node);
	}
	float segment_weight = node.length * node.radius * node.radius;
	Vector3 center_of_mass = (info.absolute_position + node.direction * node.length / 2) * segment_weight;
	float total_weight = segment_weight;
	for (auto &child : node.children) {
		BioNodeInfo &child_info = static_cast<BioNodeInfo &>(*child->node.growthInfo);
		center_of_mass += child_info.center_of_mass * child_info.branch_weight;
		total_weight += child_info.branch_weight;
	}
	center_of_mass /= total_weight;
	info.center_of_mass = center_of_mass;
	info.branch_weight = total_weight;
}

void Tree3DGrowthFunction::apply_gravity_rec(Tree3DNode &node, Basis curent_rotation) {
	BioNodeInfo &info = static_cast<BioNodeInfo &>(*node.growthInfo);
	Vector3 offset = (info.center_of_mass - info.absolute_position);
	offset[2] = 0;
	float lever_arm = offset.length();
	float torque = info.branch_weight * lever_arm;
	float bendiness = std::exp(-(info.age / 2 + info.vigor));
	float angle = torque * bendiness * gravity_strength * 50;
	Vector3 tangent = node.direction.cross(Vector3{ 0, 0, -1 });
	Basis rot;
	rot.rotate(tangent, angle);
	//rot = Eigen::AngleAxis<float>(angle, tangent);
	curent_rotation = curent_rotation * rot;
	node.direction = curent_rotation.xform(node.direction);

	for (auto &child : node.children) {
		apply_gravity_rec(child->node, curent_rotation);
	}
}

void Tree3DGrowthFunction::update_absolute_position_rec(Tree3DNode &node, const Vector3 &node_position) {
	static_cast<BioNodeInfo *>(node.growthInfo.get())->absolute_position = node_position;
	for (auto &child : node.children) {
		Vector3 child_position = node_position + node.direction * child->position_in_parent * node.length;
		update_absolute_position_rec(child->node, child_position);
	}
}

void Tree3DGrowthFunction::execute(std::vector<Tree3DStem> &stems, int id, int parent_id) {
	rand_gen.set_seed(seed);

	for (Tree3DStem &stem : stems) {
		setup_growth_information_rec(stem.node);
	}

	// 一次迭代可以看作是成长的一年
	for (int i = 0; i < iterations; i++) // an iteration can be seen as a year of growth
	{
		// 茎之间不共享能量
		for (Tree3DStem &stem : stems) // the energy is not shared between stems
		{
			float target_light_flux = 1 + std::pow((float)i, 1.5);
			float light_flux = update_vigor_ratio_rec(stem.node); // get total available energy

			if (target_light_flux > light_flux) {
				cut_threshold -= .1f;
				//grow_threshold -= .1f
			} else if (target_light_flux < light_flux) {
				cut_threshold += .1f;
			}
			//cut_threshold = (light_flux / target_light_flux) / 2;

			update_vigor_rec(stem.node, target_light_flux); // distribute the energy in each node
			simulate_growth_rec(stem.node, id); // apply rules to the tree
			update_absolute_position_rec(stem.node, stem.position);
			get_weight_rec(stem.node);
			Basis rot;
			apply_gravity_rec(stem.node, rot);
		}
	}
}
