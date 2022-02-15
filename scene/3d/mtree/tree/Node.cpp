#include "Node.hpp"
#include "scene/3d/mtree/utilities/GeometryUtilities.hpp"

bool Tree3DNode::is_leaf() const {
	return children.size() == 0;
}

Tree3DNode::Tree3DNode(Vector3 direction, Vector3 parent_tangent, float length, float radius, bool is_spawn_leaf, int creator_id) {
	this->can_spawn_leaf = is_spawn_leaf;
	this->direction = direction;
	this->tangent = Tree3DGeometry::projected_on_plane(parent_tangent, direction).normalized();
	this->length = length;
	this->radius = radius;
	this->creator_id = creator_id;
}
