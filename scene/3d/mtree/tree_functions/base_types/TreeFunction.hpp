#pragma once
#include "core/object/ref_counted.h"
#include "scene/3d/mtree/tree/Node.hpp"
#include "scene/3d/mtree/utilities/RandomGenerator.hpp"
#include <vector>

class Tree3DFunction : RefCounted {
	GDCLASS(Tree3DFunction, RefCounted)
protected:
	static void _bind_methods();

	RandomGenerator rand_gen;
	std::vector<std::shared_ptr<Tree3DFunction>> children;
	void execute_children(std::vector<Tree3DStem> &stems, int id);

public:
	int seed = 42;

	virtual void execute(std::vector<Tree3DStem> &stems, int id = 0, int parent_id = 0) = 0;
	void add_child(std::shared_ptr<Tree3DFunction> child);
};