#pragma once
#include "Node.hpp"
#include "scene/3d/mtree/tree_functions/base_types/TreeFunction.hpp"
#include <vector>

class Tree3D {
private:
	std::vector<Tree3DStem> stems;
	std::shared_ptr<Tree3DFunction> firstFunction;

public:
	Tree3D(std::shared_ptr<Tree3DFunction> trunkFunction);
	Tree3D() { firstFunction = nullptr; };
	void set_first_function(std::shared_ptr<Tree3DFunction> function);
	void execute_functions();
	void print_tree();
	Tree3DFunction &get_first_function();
	std::vector<Tree3DStem> &get_stems();
};