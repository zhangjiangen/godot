#include "Mesh.hpp"

namespace Mtree {
std::vector<std::vector<float>> Mesh::get_vertices() {
	auto result = std::vector<std::vector<float>>();
	for (Vector3 &vert : this->vertices) {
		result.push_back(std::vector<float>{ vert[0], vert[1], vert[2] });
	}
	return result;
}

int Mesh::add_vertex(const Vector3 &position) {
	vertices.push_back(position);
	for (auto &attribute : attributes) {
		attribute.second->add_data();
	}
	return (int)vertices.size() - 1;
}
int Mesh::add_polygon() {
	polygons.emplace_back();
	uv_loops.emplace_back();
	return (int)polygons.size() - 1;
}
} //namespace Mtree