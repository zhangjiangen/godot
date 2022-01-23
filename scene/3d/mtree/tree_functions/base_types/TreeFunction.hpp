#pragma once
#include "scene/3d/mtree/tree/Node.hpp"
#include "scene/3d/mtree/utilities/RandomGenerator.hpp"
#include <vector>

namespace Mtree {
class TreeFunction {
protected:
	RandomGenerator rand_gen;
	std::vector<std::shared_ptr<TreeFunction>> children;
	void execute_children(std::vector<Stem> &stems, int id);

public:
	int seed = 42;

	virtual void execute(std::vector<Stem> &stems, int id = 0, int parent_id = 0) = 0;
	void add_child(std::shared_ptr<TreeFunction> child);
};
} //namespace Mtree