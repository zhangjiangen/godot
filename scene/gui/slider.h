/*************************************************************************/
/*  slider.h                                                             */
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

#ifndef SLIDER_H
#define SLIDER_H

#include "scene/gui/range.h"

class Slider : public Range {
	GDCLASS(Slider, Range);

	struct Grab {
		int pos = 0;
		float uvalue = 0.0;
		bool active = false;
	} grab;

	int ticks = 0;
	bool mouse_inside = false;
	Orientation orientation;
	float custom_step = -1.0;
	bool editable = true;
	bool scrollable = true;

protected:
	virtual void gui_input(const Ref<InputEvent> &p_event) override;
	void _notification(int p_what);
	static void _bind_methods();
	bool ticks_on_borders = false;

public:
	virtual Size2 get_minimum_size() const override;

	void set_custom_step(float p_custom_step);
	float get_custom_step() const;

	void set_ticks(int p_count);
	int get_ticks() const;

	void set_ticks_on_borders(bool);
	bool get_ticks_on_borders() const;

	void set_editable(bool p_editable);
	bool is_editable() const;

	void set_scrollable(bool p_scrollable);
	bool is_scrollable() const;

	Slider(Orientation p_orientation = VERTICAL);
};

class HSlider : public Slider {
	GDCLASS(HSlider, Slider);

public:
	HSlider() :
			Slider(HORIZONTAL) { set_v_size_flags(0); }
};

class VSlider : public Slider {
	GDCLASS(VSlider, Slider);

public:
	VSlider() :
			Slider(VERTICAL) { set_h_size_flags(0); }
};
class MinMaxSlider : public Control {
	GDCLASS(MinMaxSlider, Control);
	const int VALUE_LOW = 0;

	const int VALUE_HIGH = 1;
	const int VALUE_COUNT = 2;

	const int FG_MARGIN = 1;

	//signal value_changed(value)
	float _min_value = 0.0;
	float _max_value = 1.0;
	Vector2 _values = Vector2(0.2, 0.6);
	Vector2 rect_min_size = Vector2(32, 28);
	bool _grabbing = false;

	Label *_label_value;

protected:
	static void _bind_methods();
	virtual void gui_input(const Ref<InputEvent> &p_event) override;
	void _get_property_list(List<PropertyInfo> *p_list) const;
	bool _get(const StringName &p_name, Variant &r_ret) const;
	bool _set(const StringName &p_name, const Variant &p_value);
	float _ratio_to_value(float r) { return r * (_max_value - _min_value) +
		_min_value; }

	int _get_closest_index(float ratio);

	void _set_from_pixel(float px);
	void _set_value_x(float v, bool notify_change) {
		if (v != _values.x) {
			_values.x = v;
			update();
		}
		if (notify_change)
			emit_signal(SNAME("value_changed"), _values);
	}
	void _set_value_y(float v, bool notify_change) {
		if (v != _values.y) {
			_values.y = v;
			update();
		}
		if (notify_change)
			emit_signal(SNAME("value_changed"), _values);
	}
	void _notification(int p_what);
	void _draw();
	float _value_to_ratio(float v) {
		if (Math::abs(_max_value - _min_value) <
				0.001) {
			return 0.0;
		}
		return (v - _min_value) / (_max_value - _min_value);
	}
	void _set_value(int i, float v, bool notify_change) {
		v = Math::clamp(v, _min_value, _max_value);
		if (i == VALUE_LOW)
			_set_value_x(v, notify_change);
		if (i == VALUE_HIGH)
			_set_value_y(v, notify_change);
	}

public:
	MinMaxSlider();
	void set_range(float min_v, float max_v);
	void set_values(const Vector2 &v);
	Vector2 get_values() { return _values; }
	float get_low_ratio() { return _value_to_ratio(_values.x); }
	float get_high_ratio() { return _value_to_ratio(_values.y); }
};

#endif // SLIDER_H
