#include "primitives/gdproceuler.h"

using namespace godot;

void GDProcEuler::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_x", "p_x"), &GDProcEuler::set_x);
	ClassDB::bind_method(D_METHOD("get_x"), &GDProcEuler::get_x);
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "x", PROPERTY_HINT_NONE, ""), "set_x", "get_x");

	ClassDB::bind_method(D_METHOD("set_y", "p_y"), &GDProcEuler::set_y);
	ClassDB::bind_method(D_METHOD("get_y"), &GDProcEuler::get_y);
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "y", PROPERTY_HINT_NONE, ""), "set_y", "get_y");

	ClassDB::bind_method(D_METHOD("set_z", "p_z"), &GDProcEuler::set_y);
	ClassDB::bind_method(D_METHOD("get_z"), &GDProcEuler::get_y);
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "z", PROPERTY_HINT_NONE, ""), "set_z", "get_z");
}

String GDProcEuler::get_type_name() {
	return "Euler angles";
}

String GDProcEuler::get_description() const {
	return "Helper node to combine a rotation around the x, y and z axis (euler angles) into a quarternion.";
}

void GDProcEuler::_init() {
	// first call super class
	GDProcNode::_init();

	// init our buffer
	value.resize(1);
	value.set(0, Color(0.0, 0.0, 0.0, 1.0));
}

void GDProcEuler::set_x(float x) {
	if (defaults.x != x) {
		defaults.x = x;
		must_update = true;
		emit_signal("changed");
	}
}

float GDProcEuler::get_x() const {
	return defaults.x;
}

void GDProcEuler::set_y(float y) {
	if (defaults.y != y) {
		defaults.y = y;
		must_update = true;
		emit_signal("changed");
	}
}

float GDProcEuler::get_y() const {
	return defaults.y;
}

void GDProcEuler::set_z(float z) {
	if (defaults.z != z) {
		defaults.z = z;
		must_update = true;
		emit_signal("changed");
	}
}

float GDProcEuler::get_z() const {
	return defaults.z;
}

bool GDProcEuler::update(bool p_inputs_updated, const Array &p_inputs) {
	bool updated = must_update || p_inputs_updated;
	must_update = false;

	if (updated) {
		PoolRealArray x;
		PoolRealArray y;
		PoolRealArray z;

		int input_count = p_inputs.size();
		if (input_count > 0) {
			if (p_inputs[0].get_type() == Variant::POOL_REAL_ARRAY) {
				x = p_inputs[0];
			}
		}

		if (input_count > 1) {
			if (p_inputs[1].get_type() == Variant::POOL_REAL_ARRAY) {
				y = p_inputs[1];
			}
		}

		if (input_count > 2) {
			if (p_inputs[2].get_type() == Variant::POOL_REAL_ARRAY) {
				z = p_inputs[2];
			}
		}

		// check our input and add defaults where we need them
		int num_x = x.size();
		if (num_x == 0) {
			x.push_back(defaults.x);
			num_x++;
		}

		int num_y = y.size();
		if (num_y == 0) {
			y.push_back(defaults.y);
			num_y++;
		}

		int num_z = z.size();
		if (num_z == 0) {
			z.push_back(defaults.z);
			num_z++;
		}

		// how many vertices will we output?
		int max = num_x > num_y ? num_x : num_y;
		if (max < num_z) {
			max = num_z;
		}

		// and lets process...
		PoolRealArray::Read rx = x.read();
		PoolRealArray::Read ry = y.read();
		PoolRealArray::Read rz = z.read();

		value.resize(max);
		PoolColorArray::Write vw = value.write();

		for (int i = 0; i < max; i++) {
			float pi_180 = 3.14159265359f / 180.0f;
			Quat q;
			q.set_euler_xyz(Vector3(rx[i % num_x], ry[i % num_y], rz[i % num_z]) * Vector3(pi_180, pi_180, pi_180));

			vw[i] = Color(q.x, q.y, q.z, q.w);
		}
	}

	return updated;
}

int GDProcEuler::get_input_connector_count() const {
	return 3;
}

Variant::Type GDProcEuler::get_input_connector_type(int p_slot) const {
	return Variant::POOL_REAL_ARRAY;
}

String GDProcEuler::get_input_connector_name(int p_slot) const {
	if (p_slot == 0) {
		return "x";
	} else if (p_slot == 1) {
		return "y";
	} else if (p_slot == 2) {
		return "z";
	}

	return "";
}

String GDProcEuler::get_connector_property_name(int p_slot) const {
	if (p_slot == 0) {
		return "x";
	} else if (p_slot == 1) {
		return "y";
	} else if (p_slot == 2) {
		return "z";
	}

	return "";
}

int GDProcEuler::get_output_connector_count() const {
	return 1;
}

Variant::Type GDProcEuler::get_output_connector_type(int p_slot) const {
	return Variant::POOL_COLOR_ARRAY;
}

String GDProcEuler::get_output_connector_name(int p_slot) const {
	return "rotations";
}

Variant GDProcEuler::get_output(int p_slot) const {
	return Variant(value);
}