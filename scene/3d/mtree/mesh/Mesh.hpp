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

// namespace py = pybind11;

class Tree3DMesh {
public:
	std::vector<Vector3> vertices;
	std::vector<Vector3> normals;
	std::vector<Vector2> uvs;
	std::vector<std::array<int, 4>> polygons;
	std::vector<int> triangles;
	std::vector<std::array<int, 4>> uv_loops;
	std::map<std::string, std::shared_ptr<AbstractAttribute>> attributes;

	Tree3DMesh(){};
	Tree3DMesh(std::vector<Vector3> &&vertices) { this->vertices = std::move(vertices); }
	std::vector<std::vector<float>> get_vertices();
	std::vector<std::array<int, 4>> get_polygons() { return this->polygons; };
	int add_vertex(const Vector3 &position);
	int add_polygon();
	// 转换成三角形列表
	void build_triangle() {
		triangles.resize(polygons.size() * 4);
		for (int i = 0; i < polygons.size(); ++i) {
			std::array<int, 4> &a = polygons[i];
			int index = i * 6;
			triangles[index] = a[0];
			triangles[index + 1] = a[1];
			triangles[index + 2] = a[2];

			triangles[index + 3] = a[0];
			triangles[index + 4] = a[2];
			triangles[index + 5] = a[3];
		}
	}
	template <class T>
	Attribute<T> &add_attribute(std::string name) {
		auto attribute = std::make_shared<Attribute<T>>(name);
		attributes[name] = attribute;
		return *attribute;
	};
};