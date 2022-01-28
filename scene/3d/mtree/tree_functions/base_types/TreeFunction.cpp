#include "TreeFunction.hpp"

void Tree3DFunction::execute_children(std::vector<Tree3DStem> &stems, int id) {
	int child_id = id;
	for (std::shared_ptr<Tree3DFunction> &child : children) {
		child_id++;
		child->execute(stems, child_id, id);
	}
}
void Tree3DFunction::add_child(std::shared_ptr<Tree3DFunction> child) {
	children.push_back(child);
}
void Tree3DFunction::_bind_methods() {
}
