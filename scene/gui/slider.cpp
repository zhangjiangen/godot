/*************************************************************************/
/*  slider.cpp                                                           */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "slider.h"
#include "core/os/keyboard.h"
#include "scene/gui/label.h"

Size2 Slider::get_minimum_size() const {
	Ref<StyleBox> style = get_theme_stylebox(SNAME("slider"));
	Size2i ss = style->get_minimum_size() + style->get_center_size();

	Ref<Texture2D> grabber = get_theme_icon(SNAME("grabber"));
	Size2i rs = grabber->get_size();

	if (orientation == HORIZONTAL) {
		return Size2i(ss.width, MAX(ss.height, rs.height));
	} else {
		return Size2i(MAX(ss.width, rs.width), ss.height);
	}
}

void Slider::gui_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	if (!editable) {
		return;
	}

	Ref<InputEventMouseButton> mb = p_event;

	if (mb.is_valid()) {
		if (mb->get_button_index() == MouseButton::LEFT) {
			if (mb->is_pressed()) {
				Ref<Texture2D> grabber = get_theme_icon(mouse_inside || has_focus() ? "grabber_highlight" : "grabber");
				grab.pos = orientation == VERTICAL ? mb->get_position().y : mb->get_position().x;

				double grab_width = (double)grabber->get_size().width;
				double grab_height = (double)grabber->get_size().height;
				double max = orientation == VERTICAL ? get_size().height - grab_height : get_size().width - grab_width;
				if (orientation == VERTICAL) {
					set_as_ratio(1 - (((double)grab.pos - (grab_height / 2.0)) / max));
				} else {
					set_as_ratio(((double)grab.pos - (grab_width / 2.0)) / max);
				}
				grab.active = true;
				grab.uvalue = get_as_ratio();

				emit_signal(SNAME("drag_started"));
			} else {
				grab.active = false;

				const bool value_changed = !Math::is_equal_approx((double)grab.uvalue, get_as_ratio());
				emit_signal(SNAME("drag_ended"), value_changed);
			}
		} else if (scrollable) {
			if (mb->is_pressed() && mb->get_button_index() == MouseButton::WHEEL_UP) {
				grab_focus();
				set_value(get_value() + get_step());
			} else if (mb->is_pressed() && mb->get_button_index() == MouseButton::WHEEL_DOWN) {
				grab_focus();
				set_value(get_value() - get_step());
			}
		}
	}

	Ref<InputEventMouseMotion> mm = p_event;

	if (mm.is_valid()) {
		if (grab.active) {
			Size2i size = get_size();
			Ref<Texture2D> grabber = get_theme_icon(SNAME("grabber"));
			float motion = (orientation == VERTICAL ? mm->get_position().y : mm->get_position().x) - grab.pos;
			if (orientation == VERTICAL) {
				motion = -motion;
			}
			float areasize = orientation == VERTICAL ? size.height - grabber->get_size().height : size.width - grabber->get_size().width;
			if (areasize <= 0) {
				return;
			}
			float umotion = motion / float(areasize);
			set_as_ratio(grab.uvalue + umotion);
		}
	}

	if (!mm.is_valid() && !mb.is_valid()) {
		if (p_event->is_action_pressed("ui_left", true)) {
			if (orientation != HORIZONTAL) {
				return;
			}
			set_value(get_value() - (custom_step >= 0 ? custom_step : get_step()));
			accept_event();
		} else if (p_event->is_action_pressed("ui_right", true)) {
			if (orientation != HORIZONTAL) {
				return;
			}
			set_value(get_value() + (custom_step >= 0 ? custom_step : get_step()));
			accept_event();
		} else if (p_event->is_action_pressed("ui_up", true)) {
			if (orientation != VERTICAL) {
				return;
			}

			set_value(get_value() + (custom_step >= 0 ? custom_step : get_step()));
			accept_event();
		} else if (p_event->is_action_pressed("ui_down", true)) {
			if (orientation != VERTICAL) {
				return;
			}
			set_value(get_value() - (custom_step >= 0 ? custom_step : get_step()));
			accept_event();
		} else if (p_event->is_action("ui_home") && p_event->is_pressed()) {
			set_value(get_min());
			accept_event();
		} else if (p_event->is_action("ui_end") && p_event->is_pressed()) {
			set_value(get_max());
			accept_event();
		}
	}
}

void Slider::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			update_minimum_size();
			update();
		} break;
		case NOTIFICATION_MOUSE_ENTER: {
			mouse_inside = true;
			update();
		} break;
		case NOTIFICATION_MOUSE_EXIT: {
			mouse_inside = false;
			update();
		} break;
		case NOTIFICATION_VISIBILITY_CHANGED: // fallthrough
		case NOTIFICATION_EXIT_TREE: {
			mouse_inside = false;
			grab.active = false;
		} break;
		case NOTIFICATION_DRAW: {
			RID ci = get_canvas_item();
			Size2i size = get_size();
			Ref<StyleBox> style = get_theme_stylebox(SNAME("slider"));
			bool highlighted = mouse_inside || has_focus();
			Ref<StyleBox> grabber_area = get_theme_stylebox(highlighted ? "grabber_area_highlight" : "grabber_area");
			Ref<Texture2D> grabber = get_theme_icon(editable ? (highlighted ? "grabber_highlight" : "grabber") : "grabber_disabled");
			Ref<Texture2D> tick = get_theme_icon(SNAME("tick"));
			double ratio = Math::is_nan(get_as_ratio()) ? 0 : get_as_ratio();

			if (orientation == VERTICAL) {
				int widget_width = style->get_minimum_size().width + style->get_center_size().width;
				float areasize = size.height - grabber->get_size().height;
				style->draw(ci, Rect2i(Point2i(size.width / 2 - widget_width / 2, 0), Size2i(widget_width, size.height)));
				grabber_area->draw(ci, Rect2i(Point2i((size.width - widget_width) / 2, size.height - areasize * ratio - grabber->get_size().height / 2), Size2i(widget_width, areasize * ratio + grabber->get_size().height / 2)));

				if (ticks > 1) {
					int grabber_offset = (grabber->get_size().height / 2 - tick->get_height() / 2);
					for (int i = 0; i < ticks; i++) {
						if (!ticks_on_borders && (i == 0 || i + 1 == ticks)) {
							continue;
						}
						int ofs = (i * areasize / (ticks - 1)) + grabber_offset;
						tick->draw(ci, Point2i((size.width - widget_width) / 2, ofs));
					}
				}
				grabber->draw(ci, Point2i(size.width / 2 - grabber->get_size().width / 2, size.height - ratio * areasize - grabber->get_size().height));
			} else {
				int widget_height = style->get_minimum_size().height + style->get_center_size().height;
				float areasize = size.width - grabber->get_size().width;

				style->draw(ci, Rect2i(Point2i(0, (size.height - widget_height) / 2), Size2i(size.width, widget_height)));
				grabber_area->draw(ci, Rect2i(Point2i(0, (size.height - widget_height) / 2), Size2i(areasize * ratio + grabber->get_size().width / 2, widget_height)));

				if (ticks > 1) {
					int grabber_offset = (grabber->get_size().width / 2 - tick->get_width() / 2);
					for (int i = 0; i < ticks; i++) {
						if ((!ticks_on_borders) && ((i == 0) || ((i + 1) == ticks))) {
							continue;
						}
						int ofs = (i * areasize / (ticks - 1)) + grabber_offset;
						tick->draw(ci, Point2i(ofs, (size.height - widget_height) / 2));
					}
				}
				grabber->draw(ci, Point2i(ratio * areasize, size.height / 2 - grabber->get_size().height / 2));
			}

		} break;
	}
}

void Slider::set_custom_step(float p_custom_step) {
	custom_step = p_custom_step;
}

float Slider::get_custom_step() const {
	return custom_step;
}

void Slider::set_ticks(int p_count) {
	ticks = p_count;
	update();
}

int Slider::get_ticks() const {
	return ticks;
}

bool Slider::get_ticks_on_borders() const {
	return ticks_on_borders;
}

void Slider::set_ticks_on_borders(bool _tob) {
	ticks_on_borders = _tob;
	update();
}

void Slider::set_editable(bool p_editable) {
	editable = p_editable;
	update();
}

bool Slider::is_editable() const {
	return editable;
}

void Slider::set_scrollable(bool p_scrollable) {
	scrollable = p_scrollable;
}

bool Slider::is_scrollable() const {
	return scrollable;
}

void Slider::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_ticks", "count"), &Slider::set_ticks);
	ClassDB::bind_method(D_METHOD("get_ticks"), &Slider::get_ticks);

	ClassDB::bind_method(D_METHOD("get_ticks_on_borders"), &Slider::get_ticks_on_borders);
	ClassDB::bind_method(D_METHOD("set_ticks_on_borders", "ticks_on_border"), &Slider::set_ticks_on_borders);

	ClassDB::bind_method(D_METHOD("set_editable", "editable"), &Slider::set_editable);
	ClassDB::bind_method(D_METHOD("is_editable"), &Slider::is_editable);
	ClassDB::bind_method(D_METHOD("set_scrollable", "scrollable"), &Slider::set_scrollable);
	ClassDB::bind_method(D_METHOD("is_scrollable"), &Slider::is_scrollable);

	ADD_SIGNAL(MethodInfo("drag_started"));
	ADD_SIGNAL(MethodInfo("drag_ended", PropertyInfo(Variant::BOOL, "value_changed")));

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "editable"), "set_editable", "is_editable");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "scrollable"), "set_scrollable", "is_scrollable");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "tick_count", PROPERTY_HINT_RANGE, "0,4096,1"), "set_ticks", "get_ticks");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "ticks_on_borders"), "set_ticks_on_borders", "get_ticks_on_borders");
}

Slider::Slider(Orientation p_orientation) {
	orientation = p_orientation;
	set_focus_mode(FOCUS_ALL);
}

MinMaxSlider::MinMaxSlider() {
	rect_min_size = Vector2(32, 28);
	rect_min_size = Vector2(32, 28);

	_label_value = memnew(Label);
	_label_value->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	_label_value->set_clip_text(true);
	_label_value->set_anchor(SIDE_TOP, 0);
	_label_value->set_anchor(SIDE_LEFT, 0);
	_label_value->set_anchor(SIDE_RIGHT, 1);
	_label_value->set_anchor(SIDE_BOTTOM, 1);
	_label_value->set_offset(SIDE_RIGHT, 8);
	_label_value->set_mouse_filter(MOUSE_FILTER_IGNORE);
	_label_value->add_theme_color_override("font_color_shadow", Color(0, 0, 0, 0.5));
	_label_value->add_theme_constant_override("shadow_offset_x", 1);
	_label_value->add_theme_constant_override("shadow_offset_y", 1);
	add_child(_label_value);
}

void MinMaxSlider::_draw() {
	int grabber_width = 3;
	int background_v_margin = 0;
	int foreground_margin = FG_MARGIN;
	Color grabber_color = Color(0.8, 0.8, 0.8);
	Color interval_color = Color(0.9, 0.4, 0.2);
	Color background_color = Color(0.1, 0.1, 0.1);

	Rect2 control_rect = Rect2(Vector2(), get_size());

	Rect2 bg_rect = Rect2(
			control_rect.position.x,
			control_rect.position.y + background_v_margin,
			control_rect.size.x,
			control_rect.size.y - 2 * background_v_margin);
	draw_rect(bg_rect, background_color);

	Rect2 fg_rect = control_rect.grow(-foreground_margin);

	float low_ratio = get_low_ratio();
	float high_ratio = get_high_ratio();
	//print("low",low_ratio,"high",high_ratio)

	float low_x = fg_rect.position.x + low_ratio * fg_rect.size.x;
	float high_x = fg_rect.position.x + high_ratio * fg_rect.size.x;

	Rect2 interval_rect = Rect2(
			low_x, fg_rect.position.y, high_x - low_x, fg_rect.size.y);
	draw_rect(interval_rect, interval_color);

	low_x = fg_rect.position.x + low_ratio * (fg_rect.size.x - grabber_width);
	high_x = fg_rect.position.x + high_ratio * (fg_rect.size.x - grabber_width);
	Rect2 grabber_rect = Rect2(
			low_x,
			fg_rect.position.y,
			grabber_width,
			fg_rect.size.y);
	draw_rect(grabber_rect, grabber_color);
	grabber_rect = Rect2(
			high_x,
			fg_rect.position.y,
			grabber_width,
			fg_rect.size.y);
	draw_rect(grabber_rect, grabber_color);
}
void MinMaxSlider::gui_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseButton> mm = p_event;
	if (mm.is_valid()) {
		if (mm->is_pressed()) {
			if (mm->get_button_index() == MouseButton::LEFT) {
				_grabbing = true;
				_set_from_pixel(mm->get_position().x);
			}
		} else {
			if (mm->get_button_index() == MouseButton::LEFT) {
				_grabbing = false;
			}
		}
	}

	Ref<InputEventMouseButton> move = p_event;
	if (move.is_valid()) {
		if (_grabbing) {
			_set_from_pixel(move->get_position().x);
		}
	}
}

void MinMaxSlider::_get_property_list(List<PropertyInfo> *p_list) const {
	p_list->push_back(
			PropertyInfo(Variant::FLOAT, "min_value"));
	p_list->push_back(
			PropertyInfo(Variant::FLOAT, "max_value"));
	p_list->push_back(
			PropertyInfo(Variant::VECTOR2, "value"));
}

bool MinMaxSlider::_get(const StringName &p_name, Variant &r_ret) const {
	if (p_name == "min_value") {
		r_ret = _min_value;
		return true;
	}
	if (p_name == "max_value") {
		r_ret = _max_value;
		return true;
	}
	if (p_name == "value") {
		r_ret = _values;
		return true;
	}
	return false;
}
bool MinMaxSlider::_set(const StringName &p_name, const Variant &p_value) {
	if (p_name == "min_value") {
		_min_value = (p_value);
		update();
		return true;
	}
	if (p_name == "max_value") {
		_max_value = (p_value);
		update();
		return true;
	}
	if (p_name == "value") {
		_values = (p_value);
		update();
		return true;
	}
	return false;
}

void MinMaxSlider::set_range(float min_v, float max_v) {
	_min_value = MIN(min_v, max_v);
	_max_value = MAX(min_v, max_v);
	update();
}
void MinMaxSlider::set_values(const Vector2 &v) {
	_values = v;
	if (_values.x > _values.y)
		_values.x = _values.y;
	if (_values.y < _values.x)
		_values.y = _values.x;
	update();
}
void MinMaxSlider::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			update_minimum_size();
			update();
		} break;
		case NOTIFICATION_MOUSE_ENTER: {
			update();
		} break;
		case NOTIFICATION_MOUSE_EXIT: {
			update();
		} break;
		case NOTIFICATION_VISIBILITY_CHANGED: // fallthrough
		case NOTIFICATION_EXIT_TREE: {
			//grab.active = false;
			_grabbing = false;
		} break;
		case NOTIFICATION_DRAW: {
			_draw();
		}
	}
}
int MinMaxSlider::_get_closest_index(float ratio) {
	float lr = get_low_ratio();
	float hr = get_high_ratio();
	if (ratio < lr)
		return VALUE_LOW;
	else if (ratio > hr)
		return VALUE_HIGH;
	float distance_low = ratio - lr;
	float distance_high = hr - ratio;
	if (distance_low < distance_high) {
		return VALUE_LOW;
	}
	return VALUE_HIGH;
}

void MinMaxSlider::_set_from_pixel(float px) {
	float r = (px - FG_MARGIN) / (get_size().x - FG_MARGIN * 2.0);
	int i = _get_closest_index(r);
	float v = _ratio_to_value(r);
	_set_value(i, v, true);
}

void MinMaxSlider::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_values", "value"), &MinMaxSlider::set_values);
	ClassDB::bind_method(D_METHOD("get_values"), &MinMaxSlider::get_values);

	ClassDB::bind_method(D_METHOD("set_range", "min", "max"), &MinMaxSlider::set_range);

	ADD_SIGNAL(MethodInfo("value_changed", PropertyInfo(Variant::VECTOR2, "value")));

	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "values"), "set_values", "get_values");
}