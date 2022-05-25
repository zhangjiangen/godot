#include "Tree.hpp"
#include "Node.hpp"
#include <iostream>
#include <vector>

Tree3D::Tree3D(std::shared_ptr<Tree3DFunction> trunkFunction) {
	firstFunction = trunkFunction;
}
void Tree3D::set_first_function(std::shared_ptr<Tree3DFunction> function) {
	firstFunction = function;
}
void Tree3D::execute_functions() {
	firstFunction->execute(stems);
}

void Tree3D::print_tree() {
	std::cout << "tree "
			  << "stems:" << stems.size() << std::endl;

	Tree3DNode *current_node = &stems[0].node;
	while (true) {
		count++;
		if (current_node->children.size() == 0)
			break;
		current_node = &current_node->children[0]->node;
	}
}
Tree3DFunction &Tree3D::get_first_function() {
	return *firstFunction;
}

std::vector<Tree3DStem> &Tree3D::get_stems() {
	return stems;
}
