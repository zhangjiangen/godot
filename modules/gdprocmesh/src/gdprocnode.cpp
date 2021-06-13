#include "gdprocnode.h"

using namespace godot;

void GDProcNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_type_name"), &GDProcNode::get_type_name);
	ClassDB::bind_method(D_METHOD("get_description"), &GDProcNode::get_description);
	ClassDB::bind_method(D_METHOD("_touch"), &GDProcNode::_touch);

	// position
	ClassDB::bind_method(D_METHOD("get_position"), &GDProcNode::get_position);
	ClassDB::bind_method(D_METHOD("set_position"), &GDProcNode::set_position);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "position", PROPERTY_HINT_NONE, ""), "set_position", "get_position");

	// node name
	ClassDB::bind_method(D_METHOD("get_node_name"), &GDProcNode::get_node_name);
	ClassDB::bind_method(D_METHOD("set_node_name"), &GDProcNode::set_node_name);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "position", PROPERTY_HINT_NONE, ""), "set_node_name", "get_node_name");

	// hidden input
	ClassDB::bind_method(D_METHOD("get_hidden_input"), &GDProcNode::get_hidden_input);
	ClassDB::bind_method(D_METHOD("set_hidden_input"), &GDProcNode::set_hidden_input);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "hide_input", PROPERTY_HINT_NONE, ""), "set_hidden_input", "get_hidden_input");
	ClassDB::bind_method(D_METHOD("get_input_property_type"), &GDProcNode::get_input_property_type);

	// connectors
	ClassDB::bind_method(D_METHOD("get_input_connector_count"), &GDProcNode::get_input_connector_count);
	ClassDB::bind_method(D_METHOD("get_input_connector_type"), &GDProcNode::get_input_connector_type);
	ClassDB::bind_method(D_METHOD("get_input_connector_name"), &GDProcNode::get_input_connector_name);

	ClassDB::bind_method(D_METHOD("get_connector_property_name"), &GDProcNode::get_connector_property_name);

	ClassDB::bind_method(D_METHOD("get_output_connector_count"), &GDProcNode::get_output_connector_count);
	ClassDB::bind_method(D_METHOD("get_output_connector_type"), &GDProcNode::get_output_connector_type);
	ClassDB::bind_method(D_METHOD("get_output_connector_name"), &GDProcNode::get_output_connector_name);

	// signal that our procmesh can subscribe to so we know things need to be regenerated
	// register_signal<GDProcNode>((char *)"changed");

	// and a special signal for node name changes
	{
		ADD_SIGNAL(MethodInfo("timeline_changed", PropertyInfo(Variant::OBJECT, "node"), PropertyInfo(Variant::STRING, "from"), PropertyInfo(Variant::STRING, "to")));
	}
}

GDProcNode::ProcessStatus GDProcNode::get_status() const {
	return status;
}

void GDProcNode::set_status(GDProcNode::ProcessStatus p_status) {
	status = p_status;
}

String GDProcNode::get_type_name() {
	return String("Node");
}

String GDProcNode::get_description() const {
	return "No Description Available";
}

void GDProcNode::set_node_name(const String p_node_name) {
	if (node_name != p_node_name) {
		String was_name = node_name;

		// change the name
		node_name = p_node_name;

		// emit name change signal
		Array arr;
		arr.push_back(this);
		arr.push_back(was_name);
		arr.push_back(node_name);
		emit_signal("node_name_changed", arr);

		// emit changed signal
		emit_signal("changed");
	}
}

String GDProcNode::get_node_name() const {
	return node_name;
}

void GDProcNode::_init() {
	must_update = true;
	position = Vector2(0.0, 0.0);
	node_name = "";
	hidden_input = false;
}

void GDProcNode::_touch() {
	must_update = true;
}

bool GDProcNode::update(bool p_inputs_updated, const Array &p_inputs) {
	bool updated = must_update || p_inputs_updated;
	must_update = false;

	if (updated) {
		// just an example here, but implement updating data here..
	}

	return updated;
}

Variant::Type GDProcNode::get_input_property_type() const {
	return Variant::NIL;
}

String GDProcNode::get_input_property_hint() const {
	return "";
}

void GDProcNode::set_input(Variant p_input) {
	// nothing to do here
}

Variant GDProcNode::get_input() {
	return Variant();
}

void GDProcNode::set_hidden_input(bool p_set) {
	hidden_input = p_set;
}

bool GDProcNode::get_hidden_input() const {
	return hidden_input;
}

int GDProcNode::get_input_connector_count() const {
	return 0;
}

Variant::Type GDProcNode::get_input_connector_type(int p_slot) const {
	return Variant::NIL;
}

String GDProcNode::get_input_connector_name(int p_slot) const {
	return "";
}

String GDProcNode::get_connector_property_name(int p_slot) const {
	return "";
}

int GDProcNode::get_output_connector_count() const {
	// we should always have one output unless this is our final node
	return 1;
}

Variant::Type GDProcNode::get_output_connector_type(int p_slot) const {
	return Variant::NIL;
}

String GDProcNode::get_output_connector_name(int p_slot) const {
	return "Default";
}

Variant GDProcNode::get_output(int p_slot) const {
	return Variant();
}

Vector2 GDProcNode::get_position() const {
	return position;
}

void GDProcNode::set_position(Vector2 p_pos) {
	if (position != p_pos) {
		position = p_pos;

		// probably should send signal
	}
}
