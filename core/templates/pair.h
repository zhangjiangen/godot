/*************************************************************************/
/*  pair.h                                                               */
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

#ifndef PAIR_H
#define PAIR_H

#include "core/templates/hashfuncs.h"
#include "core/typedefs.h"
template <class F, class S>
struct Pair {
	F first;
	S second;

	Pair() :
			first(),
			second() {
	}

	Pair(F p_first, const S &p_second) :
			first(p_first),
			second(p_second) {
	}
};

template <class F, class S>
bool operator==(const Pair<F, S> &pair, const Pair<F, S> &other) {
	return (pair.first == other.first) && (pair.second == other.second);
}

template <class F, class S>
bool operator!=(const Pair<F, S> &pair, const Pair<F, S> &other) {
	return (pair.first != other.first) || (pair.second != other.second);
}

template <class F, class S>
struct PairSort {
	bool operator()(const Pair<F, S> &A, const Pair<F, S> &B) const {
		if (A.first != B.first) {
			return A.first < B.first;
		}
		return A.second < B.second;
	}
};

template <class F, class S>
struct PairHash {
	static uint32_t hash(const Pair<F, S> &P) {
		uint64_t h1 = HashMapHasherDefault::hash(P.first);
		uint64_t h2 = HashMapHasherDefault::hash(P.second);
		return hash_one_uint64((h1 << 32) | h2);
	}
};

// template <class K, class V>
// struct KeyValue {
// 	const K key;
// 	V value;

// 	void operator=(const KeyValue &p_kv) = delete;
// 	_FORCE_INLINE_ KeyValue(const KeyValue &p_kv) :
// 			key(p_kv.key),
// 			value(p_kv.value) {
// 	}
// 	_FORCE_INLINE_ KeyValue(const K &p_key, const V &p_value) :
// 			key(p_key),
// 			value(p_value) {
// 	}
// };

// template <class K, class V>
// bool operator==(const KeyValue<K, V> &pair, const KeyValue<K, V> &other) {
// 	return (pair.key == other.key) && (pair.value == other.value);
// }

// template <class K, class V>
// bool operator!=(const KeyValue<K, V> &pair, const KeyValue<K, V> &other) {
// 	return (pair.key != other.key) || (pair.value != other.value);
// }

// A custom pair implementation is used in the map because std::pair is not is_trivially_copyable,
// which means it would  not be allowed to be used in std::memcpy. This struct is copyable, which is
// also tested.
template <typename T1, typename T2>
struct KeyValue {
	using first_type = T1;
	using second_type = T2;

	template <typename U1 = T1, typename U2 = T2,
			typename = typename std::enable_if<std::is_default_constructible<U1>::value &&
					std::is_default_constructible<U2>::value>::type>
	constexpr KeyValue() noexcept(noexcept(U1()) &&noexcept(U2())) :
			key(), value() {}

	// KeyValue constructors are explicit so we don't accidentally call this ctor when we don't have to.
	explicit constexpr KeyValue(std::pair<T1, T2> const &o) noexcept(
			noexcept(T1(std::declval<T1 const &>())) &&noexcept(T2(std::declval<T2 const &>()))) :
			key(o.first), value(o.second) {}

	// KeyValue constructors are explicit so we don't accidentally call this ctor when we don't have to.
	explicit constexpr KeyValue(std::pair<T1, T2> &&o) noexcept(noexcept(
			T1(std::move(std::declval<T1 &&>()))) &&noexcept(T2(std::move(std::declval<T2 &&>())))) :
			key(std::move(o.first)), value(std::move(o.second)) {}

	constexpr KeyValue(T1 &&a, T2 &&b) noexcept(noexcept(
			T1(std::move(std::declval<T1 &&>()))) &&noexcept(T2(std::move(std::declval<T2 &&>())))) :
			key(std::move(a)), value(std::move(b)) {}

	template <typename U1, typename U2>
	constexpr KeyValue(U1 &&a, U2 &&b) noexcept(noexcept(T1(std::forward<U1>(
			std::declval<U1 &&>()))) &&noexcept(T2(std::forward<U2>(std::declval<U2 &&>())))) :
			key(std::forward<U1>(a)), value(std::forward<U2>(b)) {}

	template <typename... U1, typename... U2>
	// MSVC 2015 produces error "C2476: ‘constexpr’ constructor does not initialize all members"
	// if this constructor is constexpr
	// #if !ROBIN_HOOD(BROKEN_CONSTEXPR)
	// 	constexpr
	// #endif
	KeyValue(std::piecewise_construct_t /*unused*/, std::tuple<U1...> a,
			std::tuple<U2...>
					b) noexcept(noexcept(KeyValue(std::declval<std::tuple<U1...> &>(),
			std::declval<std::tuple<U2...> &>(),
			std::index_sequence_for<U1...>(),
			std::index_sequence_for<U2...>()))) :
			KeyValue(a, b, std::index_sequence_for<U1...>(),
					std::index_sequence_for<U2...>()) {
	}

	// constructor called from the std::piecewise_construct_t ctor
	template <typename... U1, size_t... I1, typename... U2, size_t... I2>
	KeyValue(std::tuple<U1...> &a, std::tuple<U2...> &b, std::index_sequence<I1...> /*unused*/, std::index_sequence<I2...> /*unused*/) noexcept(
			noexcept(T1(std::forward<U1>(std::get<I1>(
					std::declval<std::tuple<
							U1...> &>()))...)) &&noexcept(T2(std::
							forward<U2>(std::get<I2>(
									std::declval<std::tuple<U2...> &>()))...))) :
			key(std::forward<U1>(std::get<I1>(a))...), value(std::forward<U2>(std::get<I2>(b))...) {
		// make visual studio compiler happy about warning about unused a & b.
		// Visual studio's pair implementation disables warning 4100.
		(void)a;
		(void)b;
	}

	void swap(KeyValue<T1, T2> &o) {
		using std::swap;
		swap(key, o.key);
		swap(value, o.value);
	}

	T1 key; // NOLINT(misc-non-private-member-variables-in-classes)
	T2 value; // NOLINT(misc-non-private-member-variables-in-classes)
};

template <typename A, typename B>
inline void swap(KeyValue<A, B> &a, KeyValue<A, B> &b) noexcept(
		noexcept(std::declval<KeyValue<A, B> &>().swap(std::declval<KeyValue<A, B> &>()))) {
	a.swap(b);
}

template <typename A, typename B>
inline constexpr bool operator==(KeyValue<A, B> const &x, KeyValue<A, B> const &y) {
	return (x.key == y.key) && (x.value == y.value);
}
template <typename A, typename B>
inline constexpr bool operator!=(KeyValue<A, B> const &x, KeyValue<A, B> const &y) {
	return !(x == y);
}
template <typename A, typename B>
inline constexpr bool operator<(KeyValue<A, B> const &x, KeyValue<A, B> const &y) noexcept(noexcept(
		std::declval<A const &>() < std::declval<A const &>()) &&noexcept(std::declval<B const &>() <
		std::declval<B const &>())) {
	return x.key < y.key || (!(y.key < x.key) && x.value < y.value);
}
template <typename A, typename B>
inline constexpr bool operator>(KeyValue<A, B> const &x, KeyValue<A, B> const &y) {
	return y < x;
}
template <typename A, typename B>
inline constexpr bool operator<=(KeyValue<A, B> const &x, KeyValue<A, B> const &y) {
	return !(x > y);
}
template <typename A, typename B>
inline constexpr bool operator>=(KeyValue<A, B> const &x, KeyValue<A, B> const &y) {
	return !(x < y);
}

template <class K, class V>
struct KeyValueSort {
	bool operator()(const KeyValue<K, V> &A, const KeyValue<K, V> &B) const {
		return A.key < B.key;
	}
};

#endif // PAIR_H
