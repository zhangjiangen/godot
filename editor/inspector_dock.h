/*************************************************************************/
/*  inspector_dock.h                                                     */
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

#ifndef INSPECTOR_DOCK_H
#define INSPECTOR_DOCK_H

#include "editor/create_dialog.h"
#include "editor/editor_data.h"
#include "editor/editor_inspector.h"
#include "editor/editor_path.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/tree.h"

class EditorFileDialog;

class InspectorDock : public VBoxContainer {
	GDCLASS(InspectorDock, VBoxContainer);

	enum MenuOptions {
		RESOURCE_LOAD,
		RESOURCE_SAVE,
		RESOURCE_SAVE_AS,
		RESOURCE_MAKE_BUILT_IN,
		RESOURCE_COPY,
		RESOURCE_EDIT_CLIPBOARD,
		OBJECT_COPY_PARAMS,
		OBJECT_PASTE_PARAMS,
		OBJECT_UNIQUE_RESOURCES,
		OBJECT_REQUEST_HELP,

		COLLAPSE_ALL,
		EXPAND_ALL,

		// Matches `EditorPropertyNameProcessor::Style`.
		PROPERTY_NAME_STYLE_RAW,
		PROPERTY_NAME_STYLE_CAPITALIZED,
		PROPERTY_NAME_STYLE_LOCALIZED,

		OBJECT_METHOD_BASE = 500
	};

	EditorData *editor_data = nullptr;

	EditorInspector *inspector = nullptr;

	Object *current = nullptr;

	Button *backward_button = nullptr;
	Button *forward_button = nullptr;

	EditorFileDialog *load_resource_dialog = nullptr;
	CreateDialog *new_resource_dialog = nullptr;
	Button *resource_new_button = nullptr;
	Button *resource_load_button = nullptr;
	MenuButton *resource_save_button = nullptr;
	MenuButton *resource_extra_button = nullptr;
	MenuButton *history_menu = nullptr;
	LineEdit *search = nullptr;

	Button *open_docs_button = nullptr;
	MenuButton *object_menu = nullptr;
	EditorPath *editor_path = nullptr;

	Button *warning = nullptr;
	AcceptDialog *warning_dialog = nullptr;

	int current_option = -1;
	ConfirmationDialog *unique_resources_confirmation = nullptr;
	Tree *unique_resources_list_tree = nullptr;

	EditorPropertyNameProcessor::Style property_name_style;

	void _prepare_menu();
	void _menu_option(int p_option);
	void _menu_confirm_current();
	void _menu_option_confirm(int p_option, bool p_confirmed);

	void _new_resource();
	void _load_resource(const String &p_type = "");
	void _open_resource_selector() { _load_resource(); }; // just used to call from arg-less signal
	void _resource_file_selected(String p_file);
	void _save_resource(bool save_as);
	void _unref_resource();
	void _copy_resource();
	void _paste_resource();
	void _prepare_resource_extra_popup();

	void _warning_pressed();
	void _resource_created();
	void _resource_selected(const Ref<Resource> &p_res, const String &p_property);
	void _edit_forward();
	void _edit_back();
	void _menu_collapseall();
	void _menu_expandall();
	void _select_history(int p_idx);
	void _prepare_history();

private:
	static InspectorDock *singleton;

public:
	static InspectorDock *get_singleton() { return singleton; }
	static EditorInspector *get_inspector_singleton() { return singleton->inspector; }

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void go_back();
	void edit_resource(const Ref<Resource> &p_resource);
	void open_resource(const String &p_type);
	void clear();
	void set_warning(const String &p_message);
	void update(Object *p_object);
	Container *get_addon_area();
	EditorInspector *get_inspector() { return inspector; }

	EditorPropertyNameProcessor::Style get_property_name_style() const;

	InspectorDock(EditorData &p_editor_data);
	~InspectorDock();
};

#endif
