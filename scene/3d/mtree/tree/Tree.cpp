#include <vector>
#include <iostream>
#include "Tree.hpp"
#include "Node.hpp"

namespace Mtree
{
	Tree::Tree(std::shared_ptr<TreeFunction> trunkFunction)
	{
		firstFunction = trunkFunction;
	}
	void Tree::set_first_function(std::shared_ptr<TreeFunction> function)
	{
		firstFunction = function;
	}
	void Tree::execute_functions()
	{
		firstFunction->execute(stems);
	}

	void Tree::print_tree()
	{
		std::cout << "tree " << "stems:" << stems.size() << std::endl;
		int count = 0;
		Node* current_node = &stems[0].node;
		while (true)
		{
			count++;
			if (current_node->children.size() == 0)
				break;
			current_node = &current_node->children[0]->node;
		}
	}
	TreeFunction& Tree::get_first_function()
	{
		return *firstFunction;
	}
	
	std::vector<Stem>& Tree::get_stems()
	{
		return stems;
	}
}
