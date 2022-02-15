#pragma once
#include "../base_types/TreeMesher.hpp"

class BasicMesher : public TreeMesher {
private:
	struct SplinePoint {
		Vector3 position;
		Vector3 direction;
		float radius;
	};
	std::vector<std::vector<SplinePoint>> get_splines(std::vector<Tree3DStem> &stems);
	void get_splines_rec(std::vector<std::vector<SplinePoint>> &splines, Tree3DNode *current_node, Vector3 current_position);
	void mesh_spline(Tree3DMesh &mesh, std::vector<SplinePoint> &spline);

public:
	int radial_resolution = 8;
	Tree3DMesh mesh_tree(Tree3D &tree) override;
};
