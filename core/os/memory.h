/*************************************************************************/
/*  memory.h                                                             */
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

#ifndef MEMORY_H
#define MEMORY_H

#include "core/error/error_macros.h"
#include "core/templates/safe_refcount.h"
#include <stdlib.h>
#include <stdexcept>

#include <stddef.h>
#include <new>

#ifndef PAD_ALIGN
#define PAD_ALIGN sizeof(uint64_t) //must always be greater than this at much
#endif
#define MEMORY_TAG_MALLOC 0xFBFBFBFB
#define MEMORY_TAG_NEW 0xBFBFBFBF
#define MEMORY_TAG_NEW_ARRAY 0xACACACAC

class Memory {
#ifdef DEBUG_ENABLED
	static SafeNumeric<uint64_t> mem_usage;
	static SafeNumeric<uint64_t> max_usage;
#endif

	static SafeNumeric<uint64_t> alloc_count;
	friend class DefaultAllocator;

	static void *alloc_static(size_t p_bytes, const char *p_description, int line, bool p_pad_align = false);
	static void *realloc_static(void *p_memory, size_t p_bytes, const char *p_description, int line, bool p_pad_align = false);
	static void free_static(void *p_ptr, bool p_pad_align = false);

public:
	static uint64_t get_mem_available();
	static uint64_t get_mem_usage();
	static uint64_t get_mem_max_usage();
};

// 这个类目前不能操作脚本创建出来的类Array，Variant，Dictctionary，
// 因为内存来源目前没有完全管理，下一步会整体应用内存管理 需要记录一下内存的大小
class DefaultAllocator {
	static void *alloc_manager(size_t p_memory);
	static void free_manager(void *ptr, size_t count);

public:
	static void record_memory_alloc(void *p_ptr, size_t p_memory, const char *file_name, int file_lne);
	static void record_memory_free(void *p_ptr, size_t size);
	friend class MallocAllocator;

public:
	// 是否管理
	_FORCE_INLINE_ static bool is_manager(size_t count) {
		return false;
		return count <= 1024;
	}
	_FORCE_INLINE_ static void *alloc(size_t p_memory, const char *file_name, int file_lne) {
		if (is_manager(p_memory)) {
			void *ptr = alloc_manager(p_memory);
			record_memory_alloc(ptr, p_memory, file_name, file_lne);
			return ptr;
		}
		void *ptr = Memory::alloc_static(p_memory, file_name, file_lne, false);
		record_memory_alloc(ptr, p_memory, file_name, file_lne);
		return ptr;
	}
	_FORCE_INLINE_ static void free(void *p_ptr, size_t p_memory) {
		record_memory_free(p_ptr, p_memory);
		if (is_manager(p_memory)) {
			return free_manager(p_ptr, p_memory);
		}
		Memory::free_static(p_ptr, false);
	}
	static void log_memory_info();
};
class MallocAllocator {
public:
	static void *alloc_memory(size_t p_memory, const char *file_name, int file_lne);
	static void free_memory(void *p_ptr);
	static void *realloc_memory(void *p_ptr, size_t p_size, const char *file_name, int file_lne);
};

void *operator new(size_t p_size, const char *p_description, size_t line); ///< operator new that takes a description and uses MemoryStaticPool
void *operator new(size_t p_size, void *(*p_allocfunc)(size_t p_size, const char *file_name, int file_lne), const char *p_description, size_t line); ///< operator new that takes a description and uses MemoryStaticPool

void *operator new(size_t p_size, void *p_pointer, size_t check, const char *p_description); ///< operator new that takes a description and uses a pointer to the preallocated memory

#ifdef _MSC_VER
// When compiling with VC++ 2017, the above declarations of placement new generate many irrelevant warnings (C4291).
// The purpose of the following definitions is to muffle these warnings, not to provide a usable implementation of placement delete.
void operator delete(void *p_mem, const char *p_description, size_t line);
void operator delete(void *p_mem, void *(*p_allocfunc)(size_t p_size, const char *file_name, int file_lne), const char *p_description, size_t line);
void operator delete(void *p_mem, void *p_pointer, size_t check, const char *p_description);
#endif

#define memalloc(m_size) MallocAllocator::alloc_memory(m_size, __FILE__, __LINE__)
#define memrealloc(m_mem, m_size) MallocAllocator::realloc_memory(m_mem, m_size, __FILE__, __LINE__)
#define memfree(m_mem) MallocAllocator::free_memory(m_mem)

_ALWAYS_INLINE_ void postinitialize_handler(void *) {}

template <class T>
_ALWAYS_INLINE_ T *_post_initialize(T *p_obj) {
	postinitialize_handler(p_obj);
	return p_obj;
}

#define memnew(m_class) _post_initialize(new (__FILE__, __LINE__) m_class)

#define memnew_allocator(m_class, m_allocator) _post_initialize(new (m_allocator::alloc, __FILE__, __LINE__) m_class)
#define memnew_placement(m_placement, m_class) _post_initialize(new (m_placement) m_class)

_ALWAYS_INLINE_ bool predelete_handler(void *) {
	return true;
}

template <class T>
void memdelete(T *p_class) {
	if (!predelete_handler(p_class)) {
		return; // doesn't want to be deleted
	}
	if (!__has_trivial_destructor(T)) {
		p_class->~T();
	}
	// 偏移到内存起始地址
	uint32_t *base = (uint32_t *)p_class;
	base -= 2;
	uint32_t size = *(base + 1);
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
	uint32_t tag = *base;
	if (tag != MEMORY_TAG_NEW) {
		abort();
	}
#endif
	// 检测数据大小是否相等,类型不相等有可能内存大小不一致
	//    uint32_t type_size = sizeof(T);
	//	if (size != type_size) {
	//		throw std::runtime_error("memory delete type size error!");
	//	}

	DefaultAllocator::free(base, size + sizeof(uint64_t));
	//Memory::free_static(p_class, false);
}

template <class T, class A>
void memdelete_allocator(T *p_class) {
	if (!predelete_handler(p_class)) {
		return; // doesn't want to be deleted
	}
	if (!__has_trivial_destructor(T)) {
		p_class->~T();
	}

	A::free(p_class, sizeof(T));
}

#define memdelete_notnull(m_v) \
	{                          \
		if (m_v) {             \
			memdelete(m_v);    \
		}                      \
	}

#define memnew_arr(m_class, m_count) memnew_arr_template<m_class>(m_count, __FILE__, __LINE__)

template <typename T>
T *memnew_arr_template(size_t p_elements, const char *p_description, size_t line) {
	if (p_elements == 0) {
		return nullptr;
	}
	/** overloading operator new[] cannot be done , because it may not return the real allocated address (it may pad the 'element count' before the actual array). Because of that, it must be done by hand. This is the
	same strategy used by std::vector, and the Vector class, so it should be safe.*/

	size_t len = sizeof(T) * p_elements;
	uint32_t *mem = (uint32_t *)DefaultAllocator::alloc(len + sizeof(uint64_t), p_description, line);
	T *failptr = nullptr; //get rid of a warning
	ERR_FAIL_COND_V(!mem, failptr);
	// 标记为数组和记录内存大小
	*mem = MEMORY_TAG_NEW_ARRAY;
	++mem;
	*mem = len;
	++mem;

	if (!__has_trivial_constructor(T)) {
		T *elems = (T *)mem;

		/* call operator new */
		for (size_t i = 0; i < p_elements; i++) {
			new (&elems[i]) T;
		}
	}

	return (T *)mem;
}

/**
 * Wonders of having own array functions, you can actually check the length of
 * an allocated-with memnew_arr() array
 */

template <typename T>
size_t memarr_len(const T *p_class) {
	uint32_t *ptr = (uint32_t *)p_class;
	ptr -= 2;
	uint32_t tag = *ptr;
	uint32_t size = *(ptr + 1);
	if (tag != MEMORY_TAG_NEW_ARRAY) {
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
		abort();
#endif
	}
	int count = size / sizeof(T);
	if (count * sizeof(T) != size) {
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
		abort();
#endif
	}

	return count;
}

template <typename T>
void memdelete_arr(T *p_class) {
	uint32_t *ptr = (uint32_t *)p_class;
	ptr -= 2;
	uint32_t size = *(ptr + 1);
	int count = size / sizeof(T);

	if (!__has_trivial_destructor(T)) {
		for (int i = 0; i < count; i++) {
			p_class[i].~T();
		}
	}
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)

	uint32_t tag = *ptr;
	// 错误验证
	if (tag != MEMORY_TAG_NEW_ARRAY) {
		abort();
	}
	if (count * sizeof(T) != size) {
		abort();
	}
#endif
	// 删除内存
	DefaultAllocator::free(ptr, size + sizeof(uint64_t));

	//Memory::free_static(ptr, true);
}

struct _GlobalNil {
	int color = 1;
	_GlobalNil *right = nullptr;
	_GlobalNil *left = nullptr;
	_GlobalNil *parent = nullptr;

	_GlobalNil();
};

struct _GlobalNilClass {
	static _GlobalNil _nil;
};

template <class T>
class DefaultTypedAllocator {
public:
	template <class... Args>
	_FORCE_INLINE_ T *new_allocation(const Args &&...p_args) { return memnew(T(p_args...)); }
	_FORCE_INLINE_ void delete_allocation(T *p_allocation) { memdelete(p_allocation); }
};

#endif // MEMORY_H
