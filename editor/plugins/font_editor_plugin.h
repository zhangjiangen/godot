/*************************************************************************/
/*  font_editor_plugin.h                                                 */
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

#ifndef FONT_EDITOR_PLUGIN_H
#define FONT_EDITOR_PLUGIN_H

#include "editor/editor_plugin.h"
#include "scene/resources/font.h"
#include "scene/resources/text_line.h"

class FontDataPreview : public Control {
	GDCLASS(FontDataPreview, Control);

protected:
	void _notification(int p_what);
	static void _bind_methods();

	Ref<TextLine> line;

public:
	virtual Size2 get_minimum_size() const override;

	void set_data(const Ref<FontData> &p_data);

	FontDataPreview();
};

/*************************************************************************/

class EditorInspectorPluginFont : public EditorInspectorPlugin {
	GDCLASS(EditorInspectorPluginFont, EditorInspectorPlugin);

public:
	virtual bool can_handle(Object *p_object) override;
	virtual void parse_begin(Object *p_object) override;
	virtual bool parse_property(Object *p_object, const Variant::Type p_type, const String &p_path, const PropertyHint p_hint, const String &p_hint_text, const uint32_t p_usage, const bool p_wide = false) override;
};

/*************************************************************************/

class FontEditorPlugin : public EditorPlugin {
	GDCLASS(FontEditorPlugin, EditorPlugin);

public:
	FontEditorPlugin();

	virtual String get_name() const override { return "Font"; }
};

#endif // FONT_EDITOR_PLUGIN_H
