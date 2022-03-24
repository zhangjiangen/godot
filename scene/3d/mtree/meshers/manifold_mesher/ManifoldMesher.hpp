#pragma once
#include "../base_types/TreeMesher.hpp"
#include <tuple>

class ManifoldMesher : public TreeMesher {
public:
	struct AttributeNames {
		inline static std::string smooth_amount = "smooth_amount";
		inline static std::string radius = "radius";
		inline static std::string direction = "direction";
	};
	virtual ~ManifoldMesher() {}
	int radial_resolution = 8;
	int smooth_iterations = 4;
	Tree3DMesh mesh_tree(Tree3D &tree) override;
};
