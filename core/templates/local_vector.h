/*************************************************************************/
/*  local_vector.h                                                       */
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

#ifndef LOCAL_VECTOR_H
#define LOCAL_VECTOR_H

#include "core/error/error_macros.h"
#include "core/os/memory.h"
#include "core/os/mutex.h"
#include "core/templates/sort_array.h"
#include "core/templates/vector.h"

#include <initializer_list>

// If tight, it grows strictly as much as needed.
// Otherwise, it grows exponentially (the default and what you want in most cases).
template <class T, class U = uint32_t, bool force_trivial = false, bool tight = false>
class LocalVector {
private:
	U count = 0;
	U capacity = 0;
	T *data = nullptr;

public:
	T *ptr() {
		return data;
	}

	const T *ptr() const {
		return data;
	}

	_FORCE_INLINE_ T &push_back(const T &p_elem) {
		if (unlikely(count == capacity)) {
			if (capacity == 0) {
				capacity = 1;
			} else {
				capacity <<= 1;
			}
			data = (T *)memrealloc(data, capacity * sizeof(T));
			CRASH_COND_MSG(!data, "Out of memory");
		}

		if (!__has_trivial_constructor(T) && !force_trivial) {
			memnew_placement(&data[count++], T(p_elem));
		} else {
			data[count++] = p_elem;
		}
		return data[count - 1];
	}

	void remove_at(U p_index) {
		ERR_FAIL_UNSIGNED_INDEX(p_index, count);
		count--;
		for (U i = p_index; i < count; i++) {
			data[i] = data[i + 1];
		}
		if (!__has_trivial_destructor(T) && !force_trivial) {
			data[count].~T();
		}
	}

	/// Removes the item copying the last value into the position of the one to
	/// remove. It's generally faster than `remove`.
	void remove_at_unordered(U p_index) {
		ERR_FAIL_INDEX(p_index, count);
		count--;
		if (count > p_index) {
			data[p_index] = data[count];
		}
		if (!__has_trivial_destructor(T) && !force_trivial) {
			data[count].~T();
		}
	}
	T &back() {
		return data[count - 1];
	}
	const T &back() const {
		return data[count - 1];
	}
	void pop_back() {
		remove_at(count - 1);
	}

	void erase(const T &p_val) {
		int64_t idx = find(p_val);
		if (idx >= 0) {
			remove_at(idx);
		}
	}

	void invert() {
		for (U i = 0; i < count / 2; i++) {
			SWAP(data[i], data[count - i - 1]);
		}
	}

	_FORCE_INLINE_ void clear() { resize(0); }
	_FORCE_INLINE_ void reset() {
		clear();
		if (data) {
			memfree(data);
			data = nullptr;
			capacity = 0;
		}
	}
	_FORCE_INLINE_ bool is_empty() const { return count == 0; }
	_FORCE_INLINE_ U get_capacity() const { return capacity; }
	_FORCE_INLINE_ void reserve(U p_size) {
		p_size = tight ? p_size : nearest_power_of_2_templated(p_size);
		if (p_size > capacity) {
			capacity = p_size;
			data = (T *)memrealloc(data, capacity * sizeof(T));
			CRASH_COND_MSG(!data, "Out of memory");
		}
	}

	_FORCE_INLINE_ U size() const { return count; }
	void resize(U p_size) {
		if (p_size < count) {
			if (!__has_trivial_destructor(T) && !force_trivial) {
				for (U i = p_size; i < count; i++) {
					data[i].~T();
				}
			}
			count = p_size;
		} else if (p_size > count) {
			if (unlikely(p_size > capacity)) {
				if (capacity == 0) {
					capacity = 1;
				}
				while (capacity < p_size) {
					capacity <<= 1;
				}
				data = (T *)memrealloc(data, capacity * sizeof(T));
				CRASH_COND_MSG(!data, "Out of memory");
			}
			if (!__has_trivial_constructor(T) && !force_trivial) {
				for (U i = count; i < p_size; i++) {
					memnew_placement(&data[i], T);
				}
			}
			count = p_size;
		}
	}
	_FORCE_INLINE_ const T &operator[](U p_index) const {
		CRASH_BAD_UNSIGNED_INDEX(p_index, count);
		return data[p_index];
	}
	_FORCE_INLINE_ T &operator[](U p_index) {
		CRASH_BAD_UNSIGNED_INDEX(p_index, count);
		return data[p_index];
	}

	void insert(U p_pos, const T &p_val) {
		ERR_FAIL_UNSIGNED_INDEX(p_pos, count + 1);
		if (p_pos == count) {
			push_back(p_val);
		} else {
			resize(count + 1);
			for (U i = count - 1; i > p_pos; i--) {
				data[i] = data[i - 1];
			}
			data[p_pos] = p_val;
		}
	}

	int64_t find(const T &p_val, U p_from = 0) const {
		for (U i = p_from; i < count; i++) {
			if (data[i] == p_val) {
				return int64_t(i);
			}
		}
		return -1;
	}

	template <class C>
	void sort_custom() {
		U len = count;
		if (len == 0) {
			return;
		}

		SortArray<T, C> sorter;
		sorter.sort(data, len);
	}

	void sort() {
		sort_custom<_DefaultComparator<T>>();
	}

	void ordered_insert(T p_val) {
		U i;
		for (i = 0; i < count; i++) {
			if (p_val < data[i]) {
				break;
			}
		}
		insert(i, p_val);
	}

	operator Vector<T>() const {
		Vector<T> ret;
		ret.resize(size());
		T *w = ret.ptrw();
		memcpy(w, data, sizeof(T) * count);
		return ret;
	}

	Vector<uint8_t> to_byte_array() const { //useful to pass stuff to gpu or variant
		Vector<uint8_t> ret;
		ret.resize(count * sizeof(T));
		uint8_t *w = ret.ptrw();
		memcpy(w, data, sizeof(T) * count);
		return ret;
	}

	_FORCE_INLINE_ LocalVector() {}
	_FORCE_INLINE_ LocalVector(std::initializer_list<T> p_init) {
		reserve(p_init.size());
		for (const T &element : p_init) {
			push_back(element);
		}
	}
	_FORCE_INLINE_ LocalVector(const LocalVector &p_from) {
		resize(p_from.size());
		for (U i = 0; i < p_from.count; i++) {
			data[i] = p_from.data[i];
		}
	}
	inline void operator=(const LocalVector &p_from) {
		resize(p_from.size());
		for (U i = 0; i < p_from.count; i++) {
			data[i] = p_from.data[i];
		}
	}
	inline void operator=(const Vector<T> &p_from) {
		resize(p_from.size());
		for (U i = 0; i < count; i++) {
			data[i] = p_from[i];
		}
	}

	_FORCE_INLINE_ ~LocalVector() {
		if (data) {
			reset();
		}
	}
};
// 只能操作alloca分配的内存，生命期是在函数调用结束
template <class T>
class TempAllocaVector {
public:
    TempAllocaVector():count(0),capacity(0),data(nullptr)
    {
        
    }
	TempAllocaVector(T *p_memmory, uint32_t p_capacity) :
			count(0), capacity(p_capacity), data(p_memmory) {
	}
	TempAllocaVector(TempAllocaVector &other) :
			count(other), capacity(other.capacity), data(other.data) {
	}
	~TempAllocaVector() {
	}
	T *ptr() {
		return data;
	}
	void operator=(TempAllocaVector &other) {
		count = other.count;
		capacity = other.capacity;
		data = other.data;
	}

	const T *ptr() const {
		return data;
	}

	_FORCE_INLINE_ T &push_back(const T &p_elem) {
		if (unlikely(count == capacity)) {
			CRASH_COND_MSG(!data, "Out of memory");
		}

		if (!__has_trivial_constructor(T)) {
			memnew_placement(&data[count++], T(p_elem));
		} else {
			data[count++] = p_elem;
		}
		return data[count - 1];
	}

	int64_t find(const T &p_val, uint32_t p_from = 0) const {
		for (uint32_t i = p_from; i < count; i++) {
			if (data[i] == p_val) {
				return int64_t(i);
			}
		}
		return -1;
	}
	void remove_at(uint32_t p_index) {
		ERR_FAIL_UNSIGNED_INDEX(p_index, count);
		count--;
		for (uint32_t i = p_index; i < count; i++) {
			data[i] = data[i + 1];
		}
		if (!__has_trivial_destructor(T)) {
			data[count].~T();
		}
	}
	_FORCE_INLINE_ T &operator[](uint32_t p_index) {
		CRASH_BAD_UNSIGNED_INDEX(p_index, count);
		return data[p_index];
	}
	_FORCE_INLINE_ uint32_t size() const { return count; }
	void clear() {
		if (!__has_trivial_destructor(T)) {
			for (uint32_t i = 0; i < count; i++) {
				data[i].~T();
			}
		}
		count = 0;
	}
	void resize(uint32_t p_size) {
		if (p_size < count) {
			if (!__has_trivial_destructor(T) ) {
				for (uint32_t i = p_size; i < count; i++) {
					data[i].~T();
				}
			}
			count = p_size;
		} else if (p_size > count) {
			if (unlikely(p_size > capacity)) {
				CRASH_COND_MSG(!data, "Out of memory");
			}
			if (!__has_trivial_constructor(T) ) {
				for (uint32_t i = count; i < p_size; i++) {
					memnew_placement(&data[i], T);
				}
			}
			count = p_size;
		}
	}

private:
	uint32_t count = 0;
	uint32_t capacity = 0;
	T *data = nullptr;
};
#define DEF_TEMP_ALLOCA_VECTOR(type, var, count) TempAllocaVector<type> var(count > 0 ? (type *)alloca(sizeof(type) * (count)) : nullptr, (count))

template <class T, class U = uint32_t, bool force_trivial = false>
using TightLocalVector = LocalVector<T, U, force_trivial, true>;

#endif // LOCAL_VECTOR_H
