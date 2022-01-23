#pragma once
#include "Attribute.hpp"
#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include <array>
#include <map>
#include <memory>
#include <vector>

// #include<pybind11/pybind11.h>
// #include<pybind11/numpy.h>

namespace Mtree {

// namespace py = pybind11;

class Mesh {
public:
	std::vector<Vector3> vertices;
	std::vector<Vector3> normals;
	std::vector<Vector2> uvs;
	std::vector<std::array<int, 4>> polygons;
	std::vector<std::array<int, 4>> uv_loops;
	std::map<std::string, std::shared_ptr<AbstractAttribute>> attributes;

	Mesh(){};
	Mesh(std::vector<Vector3> &&vertices) { this->vertices = std::move(vertices); }
	std::vector<std::vector<float>> get_vertices();
	std::vector<std::array<int, 4>> get_polygons() { return this->polygons; };
	int add_vertex(const Vector3 &position);
	int add_polygon();
	template <class T>
	Attribute<T> &add_attribute(std::string name) {
		auto attribute = std::make_shared<Attribute<T>>(name);
		attributes[name] = attribute;
		return *attribute;
	};
};
} //namespace Mtree