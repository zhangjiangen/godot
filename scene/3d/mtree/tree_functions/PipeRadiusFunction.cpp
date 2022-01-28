#include "PipeRadiusFunction.hpp"
#include "scene/3d/mtree/utilities/GeometryUtilities.hpp"
void Tree3DPipeRadiusFunction::update_radius_rec(Tree3DNode &node) {
	if (node.children.size() == 0) {
		node.radius = end_radius;
		return;
	}

	float total_children_radius = 0;
	for (auto &child : node.children) {
		update_radius_rec(child->node);
		total_children_radius += pow(child->node.radius, power);
	}
	node.radius = pow(total_children_radius, 1 / power) + constant_growth * node.length / 100;
}
void Tree3DPipeRadiusFunction::execute(std::vector<Tree3DStem> &stems, int id, int parent_id) {
	rand_gen.set_seed(seed);

	for (auto &stem : stems) {
		update_radius_rec(stem.node);
	}
	execute_children(stems, id);
}
