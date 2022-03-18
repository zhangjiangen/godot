#include "voxel_graph_editor_node.h"
#include "../../generators/graph/voxel_graph_node_db.h"
#include "voxel_graph_editor_node_preview.h"

#include <editor/editor_scale.h>
#include <scene/gui/box_container.h>
#include <scene/gui/label.h>

namespace zylann::voxel {

VoxelGraphEditorNode *VoxelGraphEditorNode::create(const VoxelGeneratorGraph &graph, uint32_t node_id) {
	const uint32_t node_type_id = graph.get_node_type_id(node_id);
	const VoxelGraphNodeDB::NodeType &node_type = VoxelGraphNodeDB::get_singleton()->get_type(node_type_id);

	VoxelGraphEditorNode *node_view = memnew(VoxelGraphEditorNode);
	node_view->set_position_offset(graph.get_node_gui_position(node_id) * EDSCALE);

	StringName node_name = graph.get_node_name(node_id);
	node_view->update_title(node_name, node_type.name);

	node_view->_node_id = node_id;
	//node_view.resizable = true
	//node_view.rect_size = Vector2(200, 100)

	// We artificially hide output ports if the node is an output.
	// These nodes have an output for implementation reasons, some outputs can process the data like any other node.
	const bool hide_outputs = node_type.category == VoxelGraphNodeDB::CATEGORY_OUTPUT;

	const unsigned int row_count = math::max(node_type.inputs.size(), hide_outputs ? 0 : node_type.outputs.size());
	const Color port_color(0.4, 0.4, 1.0);
	const Color hint_label_modulate(0.6, 0.6, 0.6);

	//const int middle_min_width = EDSCALE * 32.0;

	for (unsigned int i = 0; i < row_count; ++i) {
		const bool has_left = i < node_type.inputs.size();
		const bool has_right = (i < node_type.outputs.size()) && !hide_outputs;

		HBoxContainer *property_control = memnew(HBoxContainer);
		property_control->set_custom_minimum_size(Vector2(0, 24 * EDSCALE));

		if (has_left) {
			Label *label = memnew(Label);
			label->set_text(node_type.inputs[i].name);
			property_control->add_child(label);

			Label *hint_label = memnew(Label);
			hint_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			hint_label->set_modulate(hint_label_modulate);
			//hint_label->set_clip_text(true);
			//hint_label->set_custom_minimum_size(Vector2(middle_min_width, 0));
			property_control->add_child(hint_label);
			VoxelGraphEditorNode::InputHint input_hint;
			input_hint.label = hint_label;
			node_view->_input_hints.push_back(input_hint);
		}

		if (has_right) {
			if (property_control->get_child_count() < 2) {
				Control *spacer = memnew(Control);
				spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
				//spacer->set_custom_minimum_size(Vector2(middle_min_width, 0));
				property_control->add_child(spacer);
			}

			Label *label = memnew(Label);
			label->set_text(node_type.outputs[i].name);
			// Pass filter is required to allow tooltips to work
			label->set_mouse_filter(Control::MOUSE_FILTER_PASS);
			property_control->add_child(label);

			node_view->_output_labels.push_back(label);
		}

		node_view->add_child(property_control);
		node_view->set_slot(i, has_left, Variant::FLOAT, port_color, has_right, Variant::FLOAT, port_color);
	}

	if (node_type_id == VoxelGeneratorGraph::NODE_SDF_PREVIEW) {
		node_view->_preview = memnew(VoxelGraphEditorNodePreview);
		node_view->add_child(node_view->_preview);
	}

	return node_view;
}

void VoxelGraphEditorNode::update_title(StringName node_name, String node_type_name) {
	if (node_name == StringName()) {
		set_title(node_type_name);
	} else {
		set_title(String("{0} ({1})").format(varray(node_name, node_type_name)));
	}
}

void VoxelGraphEditorNode::poll_default_inputs(const VoxelGeneratorGraph &graph) {
	ProgramGraph::PortLocation src_loc_unused;
	const String prefix = ": ";

	for (unsigned int input_index = 0; input_index < _input_hints.size(); ++input_index) {
		VoxelGraphEditorNode::InputHint &input_hint = _input_hints[input_index];
		const ProgramGraph::PortLocation loc{ _node_id, input_index };

		if (graph.try_get_connection_to(loc, src_loc_unused)) {
			// There is an inbound connection, don't show the default value
			if (input_hint.last_value != Variant()) {
				input_hint.label->set_text("");
				input_hint.last_value = Variant();
			}

		} else {
			// There is no inbound connection, show the default value
			const Variant current_value = graph.get_node_default_input(loc.node_id, loc.port_index);
			// Only update when it changes so we don't spam editor redraws
			if (input_hint.last_value != current_value) {
				input_hint.label->set_text(prefix + current_value.to_json_string());
				input_hint.last_value = current_value;
			}
		}
	}
}

void VoxelGraphEditorNode::update_range_analysis_tooltips(
		const VoxelGeneratorGraph &graph, const VoxelGraphRuntime::State &state) {
	for (unsigned int port_index = 0; port_index < _output_labels.size(); ++port_index) {
		ProgramGraph::PortLocation loc;
		loc.node_id = get_generator_node_id();
		loc.port_index = port_index;
		uint32_t address;
		if (!graph.try_get_output_port_address(loc, address)) {
			continue;
		}
		const math::Interval range = state.get_range(address);
		Control *label = _output_labels[port_index];
		label->set_tooltip(String("Min: {0}\nMax: {1}").format(varray(range.min, range.max)));
	}
}

void VoxelGraphEditorNode::clear_range_analysis_tooltips() {
	for (unsigned int i = 0; i < _output_labels.size(); ++i) {
		Control *oc = _output_labels[i];
		oc->set_tooltip("");
	}
}

} // namespace zylann::voxel
