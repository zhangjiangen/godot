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

template <class T, class U = uint32_t, bool force_trivial = false>
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
		p_size = nearest_power_of_2_templated(p_size);
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
// 智能用在临时变量，不能返回给成员变量永远持有,并且如果复制给其他变量，当前变量就会清空，用的时候要小心
template <class T>
class LocalVectorPool {
public:
	class V {
	private:
		mutable LocalVector<T> *_vector;
		LocalVectorPool *Owenr;

	public:
		V(LocalVectorPool *owenr, LocalVector<T> *v) :
				_vector(v),
				Owenr(owenr) {
		}
		V() :
				_vector(nullptr), Owenr(nullptr) {
		}
		V(const V &v) :
				_vector(v._vector),
				Owenr(v.Owenr) {
			v._vector = nullptr;
		}
		~V() {
			if (_vector && Owenr)
				Owenr->add(_vector);
		}
		void operator=(const V &o) {
			_vector = o._vector;
			Owenr = o.Owenr;
			o._vector = nullptr;
		}
		_FORCE_INLINE_ const T &operator[](size_t p_index) const {
			return (*_vector)[p_index];
		}
		_FORCE_INLINE_ T &operator[](size_t p_index) {
			return (*_vector)[p_index];
		}
		_FORCE_INLINE_ void push_back(T &p_elem) {
			_vector->push_back(p_elem);
		}
		T *ptr() {
			return _vector->ptr();
		}

		T &back() {
			return _vector->back();
		}
		const T &back() const {
			return _vector->back();
		}
		const T *ptr() const {
			return _vector->ptr();
		}
		uint32_t size() const {
			return _vector->size();
		}
		void clear() {
			_vector->clear();
		}
		LocalVector<T> &get() {
			return *_vector;
		}
		const LocalVector<T> &get() const {
			return *_vector;
		}
	};

private:
	void add(LocalVector<T> *v) {
		mutex.lock();
		v->clear();
		pool.push_back(v);
		mutex.unlock();
	}
	LocalVector<LocalVector<T> *> pool;
	::Mutex mutex;

public:
	static LocalVectorPool &instance() {
		static LocalVectorPool ins;

		return ins;
	}
	V get() {
		mutex.lock();
		if (pool.size()) {
			LocalVector<T> *ret = pool.back();
			pool.pop_back();
			mutex.unlock();
			return V(this, ret);
		}
		mutex.unlock();
		return V(this, memnew(LocalVector<T>));
	}
};

#endif // LOCAL_VECTOR_H
