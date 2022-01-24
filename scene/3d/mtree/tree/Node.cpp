#include "Node.hpp"
#include "scene/3d/mtree/utilities/GeometryUtilities.hpp"

bool Mtree::Node::is_leaf() const {
	return children.size() == 0;
}

Mtree::Node::Node(Vector3 direction, Vector3 parent_tangent, float length, float radius, bool is_spawn_leaf, int creator_id) {
	this->can_spawn_leaf = is_spawn_leaf;
	this->direction = direction;
	this->tangent = Geometry::projected_on_plane(parent_tangent, direction).normalized();
	this->length = length;
	this->radius = radius;
	this->creator_id = creator_id;
}
