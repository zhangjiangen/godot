#include "Mesh.hpp"

std::vector<std::vector<float>> Tree3DMesh::get_vertices() {
	auto result = std::vector<std::vector<float>>();
	for (Vector3 &vert : this->vertices) {
		result.push_back(std::vector<float>{ (float)vert[0], (float)vert[1], (float)vert[2] });
	}
	return result;
}

int Tree3DMesh::add_vertex(const Vector3 &position) {
	vertices.push_back(position);
	for (auto &attribute : attributes) {
		attribute.second->add_data();
	}
	return (int)vertices.size() - 1;
}
int Tree3DMesh::add_polygon() {
	polygons.emplace_back();
	uv_loops.emplace_back();
	return (int)polygons.size() - 1;
}
