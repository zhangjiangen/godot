#pragma once
#include "../tree/Node.hpp"


struct NodeSelectionElement {
	Tree3DNode *node;
	Vector3 node_position;
	NodeSelectionElement(Tree3DNode &node, const Vector3 &position) :
			node(&node), node_position(position){};
};

using NodeSelection = std::vector<NodeSelectionElement>;
using BranchSelection = std::vector<NodeSelection>;

float get_branch_length(Tree3DNode &branch_origin);
BranchSelection select_from_tree(std::vector<Tree3DStem> &stems, int id);
Vector3 get_position_in_node(const Vector3 &node_position, const Tree3DNode &node, const float factor);

