#include "primitives/gdprocrandom.h"
#include <time.h>

using namespace godot;

void GDProcRandom::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_seed", "p_seed"), &GDProcRandom::set_seed);
	ClassDB::bind_method(D_METHOD("get_seed"), &GDProcRandom::get_seed);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "seed", PROPERTY_HINT_NONE, ""), "set_seed", "get_seed");

	ClassDB::bind_method(D_METHOD("set_count", "p_count"), &GDProcRandom::set_count);
	ClassDB::bind_method(D_METHOD("get_count"), &GDProcRandom::get_count);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "count", PROPERTY_HINT_NONE, ""), "set_count", "get_count");

	ClassDB::bind_method(D_METHOD("set_min_value", "p_min_value"), &GDProcRandom::set_min_value);
	ClassDB::bind_method(D_METHOD("get_min_value"), &GDProcRandom::get_min_value);
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "min_value", PROPERTY_HINT_NONE, ""), "set_min_value", "get_min_value");

	ClassDB::bind_method(D_METHOD("set_max_value", "p_max_value"), &GDProcRandom::set_max_value);
	ClassDB::bind_method(D_METHOD("get_max_value"), &GDProcRandom::get_max_value);
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "max_value", PROPERTY_HINT_NONE, ""), "set_max_value", "get_max_value");
}

String GDProcRandom::get_type_name() {
	return "Random";
}

String GDProcRandom::get_description() const {
	return "Generates an array of reals where each real is a number between min_size and max_size.\nCount controls the amount of elements generated.\nSeed controls the seed used to generate the numbers. If seed is zero, a random seed is chosen.";
}

void GDProcRandom::_init() {
	// first call super class
	GDProcNode::_init();

	// init our buffer
	default_seed = 0;
	default_count = 1;
	default_min_value = 0.0;
	default_max_value = 1.0;
}

void GDProcRandom::set_seed(int p_seed) {
	if (default_seed != p_seed) {
		default_seed = p_seed;
		must_update = true;
		emit_signal("changed");
	}
}

int GDProcRandom::get_seed() const {
	return default_seed;
}

void GDProcRandom::set_count(int p_count) {
	if (default_count != p_count) {
		default_count = p_count;
		must_update = true;
		emit_signal("changed");
	}
}

int GDProcRandom::get_count() const {
	return default_count;
}

void GDProcRandom::set_min_value(float p_minvalue) {
	if (default_min_value != p_minvalue) {
		default_min_value = p_minvalue;
		must_update = true;
		emit_signal("changed");
	}
}

float GDProcRandom::get_min_value() const {
	return default_min_value;
}

void GDProcRandom::set_max_value(float p_minvalue) {
	if (default_max_value != p_minvalue) {
		default_max_value = p_minvalue;
		must_update = true;
		emit_signal("changed");
	}
}

float GDProcRandom::get_max_value() const {
	return default_max_value;
}

bool GDProcRandom::update(bool p_inputs_updated, const Array &p_inputs) {
	bool updated = must_update || p_inputs_updated;
	must_update = false;

	if (updated) {
		int seed = default_seed;
		int count = default_count;
		int num_min_values = 0;
		PoolRealArray min_values;
		int num_max_values = 0;
		PoolRealArray max_values;

		int input_count = p_inputs.size();
		if (input_count > 0) {
			if (p_inputs[0].get_type() == Variant::POOL_INT_ARRAY) {
				PoolIntArray input = p_inputs[0];
				if (input.size() > 0) {
					seed = input[0];
				}
			}
		}

		if (input_count > 1) {
			if (p_inputs[1].get_type() == Variant::POOL_INT_ARRAY) {
				PoolIntArray input = p_inputs[1];
				if (input.size() > 0) {
					count = input[0];
				}
			}
		}

		if (input_count > 2) {
			if (p_inputs[2].get_type() == Variant::POOL_REAL_ARRAY) {
				min_values = p_inputs[2];
				num_min_values = min_values.size();
			}
		}

		if (input_count > 3) {
			if (p_inputs[3].get_type() == Variant::POOL_REAL_ARRAY) {
				max_values = p_inputs[3];
				num_max_values = max_values.size();
			}
		}

		// check our input and add defaults where we need them
		if (num_min_values == 0) {
			min_values.push_back(default_min_value);
			num_min_values++;
		}

		if (num_max_values == 0) {
			max_values.push_back(default_max_value);
			num_max_values++;
		}

		// and lets process...
		PoolRealArray::Read min = min_values.read();
		PoolRealArray::Read max = max_values.read();

		values.resize(count);
		if (count > 0) {
			if (seed == 0) {
				srand((int)time(NULL));
			} else {
				srand(seed);
			}

			PoolRealArray::Write w = values.write();

			for (int i = 0; i < count; i++) {
				float a = min[i % num_min_values];
				float b = max[i % num_min_values];
				float r = (float)rand();
				r /= RAND_MAX;
				r *= (b - a);
				w[i] = a + r;
			}
		}
	}

	return updated;
}

int GDProcRandom::get_input_connector_count() const {
	return 4;
}

Variant::Type GDProcRandom::get_input_connector_type(int p_slot) const {
	if (p_slot == 0) {
		return Variant::POOL_INT_ARRAY;
	} else if (p_slot == 1) {
		return Variant::POOL_INT_ARRAY;
	} else if (p_slot == 2) {
		return Variant::POOL_REAL_ARRAY;
	} else if (p_slot == 3) {
		return Variant::POOL_REAL_ARRAY;
	}

	return Variant::NIL;
}

String GDProcRandom::get_input_connector_name(int p_slot) const {
	if (p_slot == 0) {
		return "seed";
	} else if (p_slot == 1) {
		return "count";
	} else if (p_slot == 2) {
		return "min_value";
	} else if (p_slot == 3) {
		return "max_value";
	}

	return "";
}

String GDProcRandom::get_connector_property_name(int p_slot) const {
	if (p_slot == 0) {
		return "seed";
	} else if (p_slot == 1) {
		return "count";
	} else if (p_slot == 2) {
		return "min_value";
	} else if (p_slot == 3) {
		return "max_value";
	}

	return "";
}

int GDProcRandom::get_output_connector_count() const {
	return 1;
}

Variant::Type GDProcRandom::get_output_connector_type(int p_slot) const {
	return Variant::POOL_REAL_ARRAY;
}

String GDProcRandom::get_output_connector_name(int p_slot) const {
	return "values";
}

Variant GDProcRandom::get_output(int p_slot) const {
	return Variant(values);
}