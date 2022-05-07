/*************************************************************************/
/*  ref_counted.h                                                        */
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

#ifndef REF_COUNTED_H
#define REF_COUNTED_H

#include "core/object/class_db.h"
#include "core/templates/safe_refcount.h"

#include <assert.h>
#include <stdint.h>

class RefCounted : public Object {
	GDCLASS(RefCounted, Object);
	SafeRefCount refcount;
	SafeRefCount refcount_init;

protected:
	static void _bind_methods();

public:
	_FORCE_INLINE_ bool is_referenced() const { return refcount_init.get() != 1; }
	bool init_ref();
	bool reference(); // returns false if refcount is at zero and didn't get increased
	bool unreference();
	int reference_get_count() const;

	RefCounted();
	~RefCounted() {}
};

template <class T>
class Ref {
	T *reference = nullptr;

	void ref(const Ref &p_from) {
		if (p_from.reference == reference) {
			return;
		}

		unref();

		reference = p_from.reference;
		if (reference) {
			reference->reference();
		}
	}

	void ref_pointer(T *p_ref) {
		ERR_FAIL_COND(!p_ref);

		if (p_ref->init_ref()) {
			reference = p_ref;
		}
	}

	//virtual RefCounted * get_reference() const { return reference; }
public:
	_FORCE_INLINE_ bool operator==(const T *p_ptr) const {
		return reference == p_ptr;
	}
	_FORCE_INLINE_ bool operator!=(const T *p_ptr) const {
		return reference != p_ptr;
	}

	_FORCE_INLINE_ bool operator<(const Ref<T> &p_r) const {
		return reference < p_r.reference;
	}
	_FORCE_INLINE_ bool operator==(const Ref<T> &p_r) const {
		return reference == p_r.reference;
	}
	_FORCE_INLINE_ bool operator!=(const Ref<T> &p_r) const {
		return reference != p_r.reference;
	}

	_FORCE_INLINE_ T *operator->() {
		return reference;
	}

	_FORCE_INLINE_ T *operator*() {
		return reference;
	}

	_FORCE_INLINE_ const T *operator->() const {
		return reference;
	}

	_FORCE_INLINE_ const T *ptr() const {
		return reference;
	}
	_FORCE_INLINE_ T *ptr() {
		return reference;
	}

	_FORCE_INLINE_ const T *operator*() const {
		return reference;
	}

	operator Variant() const {
		return Variant(reference);
	}

	void operator=(const Ref &p_from) {
		ref(p_from);
	}

	template <class T_Other>
	void operator=(const Ref<T_Other> &p_from) {
		RefCounted *refb = const_cast<RefCounted *>(static_cast<const RefCounted *>(p_from.ptr()));
		if (!refb) {
			unref();
			return;
		}
		Ref r;
		r.reference = Object::cast_to<T>(refb);
		ref(r);
		r.reference = nullptr;
	}

	void operator=(const Variant &p_variant) {
		Object *object = p_variant.get_validated_object();

		if (object == reference) {
			return;
		}

		unref();

		if (!object) {
			return;
		}

		T *r = Object::cast_to<T>(object);
		if (r && r->reference()) {
			reference = r;
		}
	}

	template <class T_Other>
	void reference_ptr(T_Other *p_ptr) {
		if (reference == p_ptr) {
			return;
		}
		unref();

		T *r = Object::cast_to<T>(p_ptr);
		if (r) {
			ref_pointer(r);
		}
	}

	Ref(const Ref &p_from) {
		ref(p_from);
	}

	template <class T_Other>
	Ref(const Ref<T_Other> &p_from) {
		RefCounted *refb = const_cast<RefCounted *>(static_cast<const RefCounted *>(p_from.ptr()));
		if (!refb) {
			unref();
			return;
		}
		Ref r;
		r.reference = Object::cast_to<T>(refb);
		ref(r);
		r.reference = nullptr;
	}

	Ref(T *p_reference) {
		if (p_reference) {
			ref_pointer(p_reference);
		}
	}

	Ref(const Variant &p_variant) {
		Object *object = p_variant.get_validated_object();

		if (!object) {
			return;
		}

		T *r = Object::cast_to<T>(object);
		if (r && r->reference()) {
			reference = r;
		}
	}

	inline bool is_valid() const { return reference != nullptr; }
	inline bool is_null() const { return reference == nullptr; }

	void unref() {
		// TODO: this should be moved to mutexes, since this engine does not really
		// do a lot of referencing on references and stuff
		// mutexes will avoid more crashes?

		if (reference && reference->unreference()) {
			memdelete(reference);
		}
		reference = nullptr;
	}

	void instantiate(const char *file_name = __FILE__, int file_line = __LINE__) {
		ref(_post_initialize(new (file_name, file_line) T));
	}

	Ref() {}

	~Ref() {
		unref();
	}
};
#define New_instantiate(ins) ins.instantiate(__FILE__, __LINE__)

typedef Ref<RefCounted> REF;
template <class T>
class WeakPtr {
	ObjectID ref;
	T *reference = nullptr;

	void set(T *p_ptr) {
		if (p_ptr) {
			ref = p_ptr->get_instance_id();
		} else {
			ref = ObjectID();
		}
		reference = p_ptr;
	}
	void set(const T *p_ptr) {
		if (p_ptr) {
			ref = p_ptr->get_instance_id();
		} else {
			ref = ObjectID();
		}
		reference = (T *)p_ptr;
	}

	template <class T_Other>
	void set(T_Other *p_ptr) {
		if (!p_ptr) {
			reference = nullptr;
			ref = ObjectID();
			return;
		}
		reference = Object::cast_to<T>(p_ptr);
		ref = reference->get_instance_id();
	}
	template <class T_Other>
	void set(const T_Other *p_ptr) {
		if (!p_ptr) {
			reference = nullptr;
			ref = ObjectID();
			return;
		}
		reference = (T *)Object::cast_to<T>(p_ptr);
		ref = reference->get_instance_id();
	}

public:
	_FORCE_INLINE_ T *operator->() {
		if (ptr() != nullptr)
			return reference;
		return reference;
	}
	_FORCE_INLINE_ const T *operator->() const {
		if (ptr() != nullptr)
			return reference;
		return reference;
	}
	_FORCE_INLINE_ T *ptr() {
		if (reference != nullptr && (ObjectDB::get_instance(ref) == reference))
			return reference;
		return nullptr;
	}
	_FORCE_INLINE_ const T *ptr() const {
		if (reference != nullptr && (ObjectDB::get_instance(ref) == reference))
			return reference;
		return nullptr;
	}

	inline bool is_valid() const { return ptr() != nullptr; }
	inline bool is_null() const { return ptr() == nullptr; }
	void operator=(const Ref<T> &p_from) {
		set(p_from.ptr());
	}
	void operator=(T *p_from) {
		set(p_from);
	}
	template <class T_Other>
	void operator=(T_Other *p_from) {
		set(p_from);
	}
	void operator=(const WeakPtr<T> &p_from) {
		set(p_from.ptr());
	}
	template <class T_Other>
	void operator=(const Ref<T_Other> &p_from) {
		set(p_from.ptr());
	}
	template <class T_Other>
	void operator=(const WeakPtr<T_Other> &p_from) {
		set(p_from.ptr());
	}
	operator Ref<T>() {
		return Ref<T>(ptr());
	}

	WeakPtr() {}
	WeakPtr(T *object) {
		set(object);
	}
	template <class T_Other>
	WeakPtr(T_Other *object) {
		set(object);
	}
	WeakPtr(const Variant &p_variant) {
		Object *object = p_variant.get_validated_object();
		set(object);
	}

	WeakPtr(const WeakPtr<T> &p_from) {
		set(p_from.ptr());
	}
	WeakPtr(const Ref<T> &p_from) {
		set(p_from.ptr());
	}
};

class WeakRef : public RefCounted {
	GDCLASS(WeakRef, RefCounted);

	ObjectID ref;

protected:
	static void _bind_methods();

public:
	Variant get_ref() const;
	void set_obj(Object *p_object);
	void set_ref(const Ref<RefCounted> &p_ref);
	ObjectID get_obj_id() {
		return ref;
	}
	WeakRef() {}
};

template <class T>
struct PtrToArg<Ref<T>> {
	_FORCE_INLINE_ static Ref<T> convert(const void *p_ptr) {
		return Ref<T>(const_cast<T *>(reinterpret_cast<const T *>(p_ptr)));
	}

	typedef Ref<T> EncodeT;

	_FORCE_INLINE_ static void encode(Ref<T> p_val, const void *p_ptr) {
		*(const_cast<Ref<RefCounted> *>(reinterpret_cast<const Ref<RefCounted> *>(p_ptr))) = p_val;
	}
};

template <class T>
struct PtrToArg<const Ref<T> &> {
	typedef Ref<T> EncodeT;

	_FORCE_INLINE_ static Ref<T> convert(const void *p_ptr) {
		return Ref<T>((T *)p_ptr);
	}
};
enum SharedPtrFreeMethod {
	/// Use OGRE_DELETE to free the memory
	SPFM_DELETE_ARRAY,
	/// Use OGRE_DELETE_T to free (only MEMCATEGORY_GENERAL supported)
	SPFM_DELETE_T,
	/// Use OGRE_FREE to free (only MEMCATEGORY_GENERAL supported)
	SPFM_FREE,
	/// Don`t free resource at all, lifetime controlled externally.
	SPFM_NONE
};

struct SharedPtrInfo {
	inline SharedPtrInfo() {
		useCount.ref();
	}

	virtual ~SharedPtrInfo() {}
	SafeRefCount useCount;
};

struct SharedPtrInfoNone : public SharedPtrInfo {
	virtual ~SharedPtrInfoNone() {}
};

template <class T>
class SharedPtrInfoDeleteArray : public SharedPtrInfo {
	T *mObject;

public:
	inline SharedPtrInfoDeleteArray(T *o) :
			mObject(o) {}

	virtual ~SharedPtrInfoDeleteArray() {
		memdelete_arr(mObject);
	}
};

template <class T>
class SharedPtrInfoDeleteT : public SharedPtrInfo {
	T *mObject;

public:
	inline SharedPtrInfoDeleteT(T *o) :
			mObject(o) {}

	virtual ~SharedPtrInfoDeleteT() {
		memdelete(mObject);
	}
};

template <class T>
class SharedPtrInfoFree : public SharedPtrInfo {
	T *mObject;

public:
	inline SharedPtrInfoFree(T *o) :
			mObject(o) {}

	virtual ~SharedPtrInfoFree() {
		memfree(mObject);
	}
};

/** Reference-counted shared pointer, used for objects where implicit destruction is
		required.
	@remarks
		This is a standard shared pointer implementation which uses a reference
		count to work out when to delete the object.
	@par
		If OGRE_THREAD_SUPPORT is defined to be 1, use of this class is thread-safe.
	*/
template <class T>
class SharedPtr {
	template <typename Y>
	friend class SharedPtr;

protected:
	/* DO NOT ADD MEMBERS TO THIS CLASS!
	 *
	 * The average Ogre application has *thousands* of them. Every extra
	 * member causes extra memory use in general, and causes extra padding
	 * to be added to a multitude of structures.
	 *
	 * Everything you need to do can be acomplished by creatively working
	 * with the SharedPtrInfo object.
	 *
	 * There is no reason for this object to ever have more than two members.
	 */

	T *pRep;
	SharedPtrInfo *pInfo;

	SharedPtr(T *rep, SharedPtrInfo *info) :
			pRep(rep), pInfo(info) {
	}

public:
	typedef T DataType;
	/** Constructor, does not initialise the SharedPtr.
			@remarks
				<b>Dangerous!</b> You have to call bind() before using the SharedPtr.
		*/
	SharedPtr() :
			pRep(0), pInfo(0) {}

private:
	static SharedPtrInfo *createInfoForMethod(T *rep, SharedPtrFreeMethod method) {
		switch (method) {
			case SPFM_DELETE_ARRAY:
				return memnew(SharedPtrInfoDeleteArray<T>(rep));
			case SPFM_DELETE_T:
				return memnew(SharedPtrInfoDeleteT<T>(rep));
			case SPFM_FREE:
				return memnew(SharedPtrInfoFree<T>(rep));
			case SPFM_NONE:
				return memnew(SharedPtrInfoNone());
		}
		return 0;
	}

public:
	/** Constructor.
		@param rep The pointer to take ownership of
		@param inFreeMethod The mechanism to use to free the pointer
		*/
	template <class Y>
	explicit SharedPtr(Y *rep, SharedPtrFreeMethod inFreeMethod = SPFM_DELETE_T) :
			pRep(rep), pInfo(rep ? createInfoForMethod(rep, inFreeMethod) : 0) {
	}

	SharedPtr(const SharedPtr &r) :
			pRep(r.pRep), pInfo(r.pInfo) {
		if (pRep) {
			pInfo->useCount.ref();
		}
	}
	void instantiate(const char *file_name = __FILE__, int file_line = __LINE__, int array_count = 1, SharedPtrFreeMethod inFreeMethod = SPFM_DELETE_T) {
		T *ptr = nullptr;
		switch (inFreeMethod) {
			case SPFM_DELETE_ARRAY:
				if (array_count > 0) {
					ptr = memnew_arr(DataType, array_count);
				}
				break;
			case SPFM_DELETE_T:
				ptr = _post_initialize(new (file_name, file_line) T);
				break;
			case SPFM_FREE:
				break;
			/// Don`t free resource at all, lifetime controlled externally.
			case SPFM_NONE:
				break;
		}
		reset(ptr);
	}

	SharedPtr &operator=(const SharedPtr &r) {
		// One resource could have several non-controlling control blocks but only one controlling.
		assert(pRep != r.pRep || pInfo == r.pInfo || dynamic_cast<SharedPtrInfoNone *>(pInfo) || dynamic_cast<SharedPtrInfoNone *>(r.pInfo));
		if (pInfo == r.pInfo)
			return *this;

		// Swap current data into a local copy
		// this ensures we deal with rhs and this being dependent
		SharedPtr<T> tmp(r);
		swap(tmp);
		return *this;
	}

	/* For C++11 compilers, use enable_if to only expose functions when viable
	 *
	 * MSVC 2012 and earlier only claim conformance to C++98. This is fortunate,
	 * because they don't support default template parameters
	 */
#if __cplusplus >= 201103L && !defined(__APPLE__)
	template <class Y,
			class = typename std::enable_if<std::is_convertible<Y *, T *>::value>::type>
#else
	template <class Y>
#endif
	SharedPtr(const SharedPtr<Y> &r) :
			pRep(r.pRep), pInfo(r.pInfo) {
		if (pRep) {
			pInfo->useCount.ref();
		}
	}

#if __cplusplus >= 201103L && !defined(__APPLE__)
	template <class Y,
			class = typename std::enable_if<std::is_assignable<T *, Y *>::value>::type>
#else
	template <class Y>
#endif
	SharedPtr &operator=(const SharedPtr<Y> &r) {
		// One resource could have several non-controlling control blocks but only one controlling.
		assert(pRep != r.pRep || pInfo == r.pInfo || dynamic_cast<SharedPtrInfoNone *>(pInfo) || dynamic_cast<SharedPtrInfoNone *>(r.pInfo));
		if (pInfo == r.pInfo)
			return *this;

		// Swap current data into a local copy
		// this ensures we deal with rhs and this being dependent
		SharedPtr<T> tmp(r);
		swap(tmp);
		return *this;
	}

	~SharedPtr() {
		release();
	}

	/// @deprecated use Ogre::static_pointer_cast instead
	template <typename Y>
	SharedPtr<Y> staticCast() const {
		if (pRep) {
			pInfo->useCount.ref();
			return SharedPtr<Y>(static_cast<Y *>(pRep), pInfo);
		} else
			return SharedPtr<Y>();
	}

	/// @deprecated use Ogre::dynamic_pointer_cast instead
	template <typename Y>
	SharedPtr<Y> dynamicCast() const {
		Y *rep = dynamic_cast<Y *>(pRep);
		if (rep) {
			pInfo->useCount.ref();
			return SharedPtr<Y>(rep, pInfo);
		} else
			return SharedPtr<Y>();
	}

	inline T &operator*() const {
		assert(pRep);
		return *pRep;
	}
	inline T *operator->() const {
		assert(pRep);
		return pRep;
	}
	inline T *get() const {
		return pRep;
	}

	/** Binds rep to the SharedPtr.
			@remarks
				Assumes that the SharedPtr is uninitialised!

			@warning
				The object must not be bound into a SharedPtr elsewhere

			@deprecated this api will be dropped. use reset(T*) instead
		*/
	void bind(T *rep, SharedPtrFreeMethod inFreeMethod = SPFM_DELETE_T) {
		assert(!pRep && !pInfo);
		pInfo = createInfoForMethod(rep, inFreeMethod);
		pRep = rep;
	}

	inline bool unique() const {
		return pInfo->useCount.get() == 1;
	}

	unsigned int use_count() const {
		return pInfo->useCount.get();
	}

	/// @deprecated use get() instead
	T *getPointer() const {
		return pRep;
	}

	static void unspecified_bool(SharedPtr ***) {
	}

	typedef void (*unspecified_bool_type)(SharedPtr ***);

	operator unspecified_bool_type() const {
		return pRep == 0 ? 0 : unspecified_bool;
	}

	/// @deprecated use SharedPtr::operator unspecified_bool_type() instead
	bool isNull(void) const {
		return pRep == 0;
	}

	/// @deprecated use reset() instead
	void setNull() {
		reset();
	}

	void reset(void) {
		release();
	}

	void reset(T *rep) {
		SharedPtr(rep).swap(*this);
	}

protected:
	inline void release(void) {
		if (pRep) {
			if (pInfo->useCount.unrefval() == 0)
				destroy();
		}

		pRep = 0;
		pInfo = 0;
	}

	/** IF YOU GET A CRASH HERE, YOU FORGOT TO FREE UP POINTERS
		 BEFORE SHUTTING OGRE DOWN
		 Use reset() before shutdown or make sure your pointer goes
		 out of scope before OGRE shuts down to avoid this. */
	inline void destroy(void) {
		assert(pRep && pInfo);
		memdelete(pInfo);
	}

	inline void swap(SharedPtr<T> &other) {
		std::swap(pRep, other.pRep);
		std::swap(pInfo, other.pInfo);
	}
};

// 对象类型智能指针
template <class T>
class TObjectPtr {
	template <typename Y>
	friend class TObjectPtr;

protected:
	/* DO NOT ADD MEMBERS TO THIS CLASS!
	 *
	 * The average Ogre application has *thousands* of them. Every extra
	 * member causes extra memory use in general, and causes extra padding
	 * to be added to a multitude of structures.
	 *
	 * Everything you need to do can be acomplished by creatively working
	 * with the SharedPtrInfo object.
	 *
	 * There is no reason for this object to ever have more than two members.
	 */

	T *pRep;

public:
	TObjectPtr(T *rep) :
			pRep(nullptr) {
		reset(rep);
	}

public:
	typedef T DataType;
	/** Constructor, does not initialise the SharedPtr.
			@remarks
				<b>Dangerous!</b> You have to call bind() before using the SharedPtr.
		*/
	TObjectPtr() :
			pRep(0) {}

private:
public:
	/** Constructor.
		@param rep The pointer to take ownership of
		@param inFreeMethod The mechanism to use to free the pointer
		*/
	template <class Y>
	explicit TObjectPtr(Y *rep) :
			pRep(nullptr) {
		reset(rep);
	}

	TObjectPtr(const TObjectPtr &r) :
			pRep(nullptr) {
		reset(r.pRep);
	}
	void instantiate() {
		T *ptr = memnew(T);
		reset(ptr);
	}

	TObjectPtr &operator=(const TObjectPtr &r) {
		// One resource could have several non-controlling control blocks but only one controlling.
		assert(pRep != r.pRep);

		// Swap current data into a local copy
		// this ensures we deal with rhs and this being dependent
		TObjectPtr<T> tmp(r);
		swap(tmp);
		return *this;
	}

	/* For C++11 compilers, use enable_if to only expose functions when viable
	 *
	 * MSVC 2012 and earlier only claim conformance to C++98. This is fortunate,
	 * because they don't support default template parameters
	 */
#if __cplusplus >= 201103L && !defined(__APPLE__)
	template <class Y,
			class = typename std::enable_if<std::is_convertible<Y *, T *>::value>::type>
#else
	template <class Y>
#endif
	TObjectPtr(const TObjectPtr<Y> &r) :
			pRep(nullptr) {

		reset(r.pRep);
	}

#if __cplusplus >= 201103L && !defined(__APPLE__)
	template <class Y,
			class = typename std::enable_if<std::is_assignable<T *, Y *>::value>::type>
#else
	template <class Y>
#endif
	TObjectPtr &operator=(const TObjectPtr<Y> &r) {
		// One resource could have several non-controlling control blocks but only one controlling.
		assert(pRep != r.pRep);

		// Swap current data into a local copy
		// this ensures we deal with rhs and this being dependent
		TObjectPtr<T> tmp(r);
		swap(tmp);
		return *this;
	}

	~TObjectPtr() {
		release();
	}

	/// @deprecated use Ogre::static_pointer_cast instead
	template <typename Y>
	TObjectPtr<Y> staticCast() const {
		if (pRep) {
			return TObjectPtr<Y>(static_cast<Y *>(pRep));
		} else
			return TObjectPtr<Y>();
	}

	/// @deprecated use Ogre::dynamic_pointer_cast instead
	template <typename Y>
	TObjectPtr<Y> dynamicCast() const {
		Y *rep = dynamic_cast<Y *>(pRep);
		if (rep) {
			return TObjectPtr<Y>(rep);
		} else
			return TObjectPtr<Y>();
	}

	inline T &operator*() const {
		assert(pRep);
		return *pRep;
	}
	inline T *operator->() const {
		assert(pRep);
		return pRep;
	}
	inline T *get() const {
		return pRep;
	}

	/** Binds rep to the SharedPtr.
			@remarks
				Assumes that the SharedPtr is uninitialised!

			@warning
				The object must not be bound into a SharedPtr elsewhere

			@deprecated this api will be dropped. use reset(T*) instead
		*/
	void bind(T *rep) {
		assert(!pRep);
		pRep = rep;
	}

	inline bool unique() const {
		return pRep->get_ref_count() == 1;
	}

	unsigned int use_count() const {
		return pRep->get_ref_count();
	}

	/// @deprecated use get() instead
	T *getPointer() const {
		return pRep;
	}

	static void unspecified_bool(TObjectPtr ***) {
	}

	typedef void (*unspecified_bool_type)(TObjectPtr ***);

	operator unspecified_bool_type() const {
		return pRep == 0 ? 0 : unspecified_bool;
	}

	/// @deprecated use SharedPtr::operator unspecified_bool_type() instead
	bool isNull(void) const {
		return pRep == 0;
	}

	/// @deprecated use reset() instead
	void setNull() {
		reset();
	}

	void reset(void) {
		release();
	}
	void reset(T *rep) {
		release();
		pRep = rep;
		if (pRep != nullptr) {
			pRep->add_ref_count();
		}
	}
	template <class Y>
	void reset(Y *rep) {
		release();
		pRep = static_cast<T *>(rep);
		if (pRep != nullptr) {
			pRep->add_ref_count();
		}
	}

protected:
	inline void release(void) {
		if (pRep) {
			int32_t count = pRep->inv_ref_count();
			if (count == 0)
				destroy();
		}

		pRep = nullptr;
	}

	/** IF YOU GET A CRASH HERE, YOU FORGOT TO FREE UP POINTERS
		 BEFORE SHUTTING OGRE DOWN
		 Use reset() before shutdown or make sure your pointer goes
		 out of scope before OGRE shuts down to avoid this. */
	inline void destroy(void) {
		memdelete(pRep);
	}

	inline void swap(TObjectPtr<T> &other) {
		std::swap(pRep, other.pRep);
	}
};

template <class T, class U>
inline bool operator==(TObjectPtr<T> const &a, TObjectPtr<U> const &b) {
	return a.get() == b.get();
}

template <class T, class U>
inline bool operator!=(TObjectPtr<T> const &a, TObjectPtr<U> const &b) {
	return a.get() != b.get();
}

template <class T, class U>
inline bool operator<(TObjectPtr<T> const &a, TObjectPtr<U> const &b) {
	return std::less<const void *>()(a.get(), b.get());
}

template <class T, class U>
inline TObjectPtr<T> static_pointer_cast(TObjectPtr<U> const &r) {
	return r.template staticCast<T>();
}

template <class T, class U>
inline TObjectPtr<T> dynamic_pointer_cast(TObjectPtr<U> const &r) {
	return r.template dynamicCast<T>();
}

template <class T, class U>
inline bool operator==(SharedPtr<T> const &a, SharedPtr<U> const &b) {
	return a.get() == b.get();
}

template <class T, class U>
inline bool operator!=(SharedPtr<T> const &a, SharedPtr<U> const &b) {
	return a.get() != b.get();
}

template <class T, class U>
inline bool operator<(SharedPtr<T> const &a, SharedPtr<U> const &b) {
	return std::less<const void *>()(a.get(), b.get());
}

template <class T, class U>
inline SharedPtr<T> static_pointer_cast(SharedPtr<U> const &r) {
	return r.template staticCast<T>();
}

template <class T, class U>
inline SharedPtr<T> dynamic_pointer_cast(SharedPtr<U> const &r) {
	return r.template dynamicCast<T>();
}

template <class T>
class SalfRefVector {
public:
	SalfRefVector() {
		ArrayData = memnew(Vector<Ref<T>>());
		TempArrayData = memnew(Vector<Ref<T>>());
	}
	~SalfRefVector() {
		memdelete(ArrayData);
		memdelete(TempArrayData);
		ArrayData = nullptr;
		TempArrayData = nullptr;
	}

	// 这个函数只能在线程里面调用，并且只能在一个线程调用，不支持多个线程同时调用
	const Vector<Ref<T>> &GetData() {
		_mutex.lock();
		if (AddData.size() > 0 || RemoveData.size() > 0) {
			for (int i = 0; i < AddData.size(); ++i) {
				ArrayData->push_back(AddData[i]);
			}
			bool is_change = RemoveData.size() > 0;
			Ref<T> nullref;
			for (int i = 0; i < RemoveData.size(); ++i) {
				for (int j = 0; j < (*ArrayData).size(); ++j) {
					if ((*ArrayData)[j] == RemoveData[i]) {
						ArrayData->write[j] = nullref;
						break;
					}
				}
			}
			if (is_change) {
				for (int i = 0; i < ArrayData->size(); ++i) {
					if ((*ArrayData)[i].ptr() != nullptr) {
						TempArrayData->push_back((*ArrayData)[i]);
					}
				}
				ArrayData->clear();
				Vector<Ref<T>> *t = ArrayData;
				ArrayData = TempArrayData;
				TempArrayData = t;
			}
		}
		_mutex.unlock();
		return *ArrayData;
	}
	// 这个函数只能在线程里面调用，调用之前必须调用GetData，支持多个线程同时调用
	const Vector<Ref<T>> &OnlyGetData() {
		return *ArrayData;
	}
	// 增加一个数据
	void Add(const Ref<T> &p_data) {
		_mutex.lock();
		AddData.push_back(p_data);
		_mutex.unlock();
	}
	// 删除一个数据
	void Remove(const Ref<T> &p_data) {
		_mutex.lock();
		RemoveData.push_back(p_data);
		_mutex.unlock();
	}

private:
	Mutex _mutex;
	Vector<Ref<T>> AddData;
	Vector<Ref<T>> RemoveData;
	Vector<Ref<T>> *ArrayData;
	Vector<Ref<T>> *TempArrayData;
};
template <class T>
class SalfSharedVector {
public:
	SalfSharedVector() {
		ArrayData = memnew(Vector<SharedPtr<T>>());
		TempArrayData = memnew(Vector<SharedPtr<T>>());
	}
	~SalfSharedVector() {
		memdelete(ArrayData);
		memdelete(TempArrayData);
		ArrayData = nullptr;
		TempArrayData = nullptr;
	}

	// 这个函数只能在线程里面调用，并且只能在一个线程调用，不支持多个线程同时调用
	const Vector<SharedPtr<T>> &OnlyGetData() {
		return *ArrayData;
	}
	// 这个函数只能在线程里面调用，并且只能在一个线程调用，不支持多个线程同时调用
	const Vector<SharedPtr<T>> &GetData() {
		_mutex.lock();
		if (AddData.size() > 0 || RemoveData.size() > 0) {
			for (int i = 0; i < AddData.size(); ++i) {
				ArrayData->push_back(AddData[i]);
			}
			bool is_change = RemoveData.size() > 0;
			for (int i = 0; i < RemoveData.size(); ++i) {
				for (int j = 0; j < (*ArrayData).size(); ++j) {
					if ((*ArrayData)[j] == RemoveData[i]) {
						ArrayData->write[j] = nullptr;
						break;
					}
				}
			}
			if (is_change) {
				for (int i = 0; i < ArrayData->size(); ++i) {
					if ((*ArrayData)[i].get() != nullptr) {
						TempArrayData->push_back((*ArrayData)[i]);
					}
				}
				ArrayData->clear();
				Vector<Ref<T>> *t = ArrayData;
				ArrayData = TempArrayData;
				TempArrayData = t;
			}
		}
		_mutex.unlock();
		return *ArrayData;
	}
	// 增加一个数据
	void Add(const SharedPtr<T> &p_data) {
		_mutex.lock();
		AddData.push_back(p_data);
		_mutex.unlock();
	}
	// 删除一个数据
	void Remove(const SharedPtr<T> &p_data) {
		_mutex.lock();
		RemoveData.push_back(p_data);
		_mutex.unlock();
	}

private:
	Mutex _mutex;
	Vector<SharedPtr<T>> AddData;
	Vector<SharedPtr<T>> RemoveData;
	Vector<SharedPtr<T>> *ArrayData;
	Vector<SharedPtr<T>> *TempArrayData;
};

template <class T>
struct GetTypeInfo<Ref<T>> {
	static const Variant::Type VARIANT_TYPE = Variant::OBJECT;
	static const GodotTypeInfo::Metadata METADATA = GodotTypeInfo::METADATA_NONE;

	static inline PropertyInfo get_class_info() {
		return PropertyInfo(Variant::OBJECT, String(), PROPERTY_HINT_RESOURCE_TYPE, T::get_class_static());
	}
};

template <class T>
struct GetTypeInfo<const Ref<T> &> {
	static const Variant::Type VARIANT_TYPE = Variant::OBJECT;
	static const GodotTypeInfo::Metadata METADATA = GodotTypeInfo::METADATA_NONE;

	static inline PropertyInfo get_class_info() {
		return PropertyInfo(Variant::OBJECT, String(), PROPERTY_HINT_RESOURCE_TYPE, T::get_class_static());
	}
};

#endif // REF_COUNTED_H
