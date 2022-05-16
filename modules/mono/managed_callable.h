/*************************************************************************/
/*  managed_callable.h                                                   */
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

#ifndef MANAGED_CALLABLE_H
#define MANAGED_CALLABLE_H

#include <mono/metadata/object.h>

#include "core/os/mutex.h"
#include "core/templates/self_list.h"
#include "core/variant/callable.h"

#include "mono_gc_handle.h"
#include "mono_gd/gd_mono_method.h"

class ManagedCallable : public CallableCustom {
	friend class CSharpLanguage;
	MonoGCHandleData delegate_handle;
	GDMonoMethod *delegate_invoke = nullptr;

#ifdef GD_MONO_HOT_RELOAD
	SelfList<ManagedCallable> self_instance = this;
	static SelfList<ManagedCallable>::List instances;
	static RBMap<ManagedCallable *, Array> instances_pending_reload;
	static Mutex instances_mutex;
#endif

public:
	uint32_t hash() const override;
	String get_as_text() const override;
	CompareEqualFunc get_compare_equal_func() const override;
	CompareLessFunc get_compare_less_func() const override;
	ObjectID get_object() const override;
	void call(const Variant **p_arguments, int p_argcount, Variant &r_return_value, Callable::CallError &r_call_error) const override;

	_FORCE_INLINE_ MonoDelegate *get_delegate() { return (MonoDelegate *)delegate_handle.get_target(); }

	void set_delegate(MonoDelegate *p_delegate);

	static bool compare_equal(const CallableCustom *p_a, const CallableCustom *p_b);
	static bool compare_less(const CallableCustom *p_a, const CallableCustom *p_b);

	static constexpr CompareEqualFunc compare_equal_func_ptr = &ManagedCallable::compare_equal;
	static constexpr CompareEqualFunc compare_less_func_ptr = &ManagedCallable::compare_less;

	ManagedCallable(MonoDelegate *p_delegate);
	~ManagedCallable();
};

#endif // MANAGED_CALLABLE_H
