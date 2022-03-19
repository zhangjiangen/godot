#pragma once
#include <array>
#include <iostream>
#include <vector>

struct AbstractAttribute {
	virtual void add_data() = 0;
};

template <typename T>
struct Attribute : AbstractAttribute {
	std::string name;
	std::vector<T> data;

	Attribute(std::string name) :
			name{ name } {};
	virtual ~Attribute() {}

	virtual void add_data() {
		data.emplace_back();
	};
};