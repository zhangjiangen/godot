/*************************************************************************/
/*  memory.cpp                                                           */
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

#include "memory.h"

#include "core/error/error_macros.h"
#include "core/os/mutex.h"
#include "core/templates/safe_refcount.h"

#include "core/templates/hashfuncs.h"

//#include "core/os/tcmalloc/tcmalloc.cc"
#include <stdio.h>
#include <stdlib.h>
struct MemmoryInfo {
	size_t size;
	size_t count;
};
template <typename TKey, typename TData>
class MemoryHashMap {
#define MIN_HASH_TABLE_POWER 3
#define RELATIONSHIP 8
	friend class SmallMemoryManager;

public:
	struct Pair {
		TKey key;
		TData data;
		Pair() :
				key(nullptr) {}
		Pair(const TKey &p_key) :
				key(p_key),
				data() {}
		Pair(const TKey &p_key, const TData &p_data) :
				key(p_key),
				data(p_data) {
		}
	};

	struct Element {
	private:
		friend class MemoryHashMap;
		friend class SmallMemoryManager;

		uint64_t hash = 0;
		Element *next = nullptr;
		Element() {}
		Pair pair;

	public:
		const TKey &key() const {
			return pair.key;
		}

		TData &value() {
			return pair.data;
		}

		const TData &value() const {
			return pair.data;
		}

		Element(const TKey &p_key) :
				pair(p_key) {}
		Element(const Element &p_other) :
				hash(p_other.hash),
				pair(p_other.pair.key, p_other.pair.data) {}
	};

private:
	Element **hash_table = nullptr;
	uint32_t hash_table_size = 0;
	uint8_t hash_table_power = 0;
	uint32_t elements = 0;

	void make_hash_table() {
		ERR_FAIL_COND(hash_table);

		hash_table_power = MIN_HASH_TABLE_POWER;
		hash_table_size = (uint32_t)1 << hash_table_power;
		hash_table = new Element *[hash_table_size];

		elements = 0;
		for (int i = 0; i < (1 << MIN_HASH_TABLE_POWER); i++) {
			hash_table[i] = nullptr;
		}
	}

	void erase_hash_table() {
		ERR_FAIL_COND_MSG(elements, "Cannot erase hash table if there are still elements inside.");

		memdelete_arr(hash_table);
		hash_table = nullptr;
		hash_table_power = 0;
		elements = 0;
	}

	void check_hash_table() {
		int new_hash_table_power = -1;

		if ((int)elements > ((1 << hash_table_power) * RELATIONSHIP)) {
			/* rehash up */
			new_hash_table_power = hash_table_power + 1;

			while ((int)elements > ((1 << new_hash_table_power) * RELATIONSHIP)) {
				new_hash_table_power++;
			}

		} else if ((hash_table_power > (int)MIN_HASH_TABLE_POWER) && ((int)elements < ((1 << (hash_table_power - 1)) * RELATIONSHIP))) {
			/* rehash down */
			new_hash_table_power = hash_table_power - 1;

			while ((int)elements < ((1 << (new_hash_table_power - 1)) * RELATIONSHIP)) {
				new_hash_table_power--;
			}

			if (new_hash_table_power < (int)MIN_HASH_TABLE_POWER) {
				new_hash_table_power = MIN_HASH_TABLE_POWER;
			}
		}

		if (new_hash_table_power == -1) {
			return;
		}

		Element **new_hash_table = new Element *[((uint64_t)1 << new_hash_table_power)];
		if (!new_hash_table) {
			return;
		}

		for (int i = 0; i < (1 << new_hash_table_power); i++) {
			new_hash_table[i] = nullptr;
		}

		if (hash_table) {
			for (int i = 0; i < (1 << hash_table_power); i++) {
				while (hash_table[i]) {
					Element *se = hash_table[i];
					hash_table[i] = se->next;
					int new_pos = se->hash & ((1 << new_hash_table_power) - 1);
					se->next = new_hash_table[new_pos];
					new_hash_table[new_pos] = se;
				}
			}

			delete[] hash_table;
		}
		hash_table = new_hash_table;
		hash_table_size = ((uint64_t)1 << new_hash_table_power);
		hash_table_power = new_hash_table_power;
	}

	/* I want to have only one function.. */
	_FORCE_INLINE_ const Element *get_element(const TKey &p_key) const {
		uint64_t hash = HashMapHasherDefault::hash(p_key);
		uint32_t index = hash & ((1 << hash_table_power) - 1);

		Element *e = hash_table[index];

		while (e) {
			/* checking hash first avoids comparing key, which may take longer */
			if (e->hash == hash && e->key() == p_key) {
				/* the pair exists in this hashtable, so just update data */
				return e;
			}

			e = e->next;
		}

		return nullptr;
	}

	Element *create_element(const TKey &p_key) {
		/* if element doesn't exist, create it */
		Element *e = new Element(p_key);
		if (!e) {
			return nullptr;
		}
		uint64_t hash = HashMapHasherDefault::hash(p_key);
		uint32_t index = hash & ((1 << hash_table_power) - 1);
		e->next = hash_table[index];
		e->hash = hash;

		hash_table[index] = e;
		elements++;

		return e;
	}

	void copy_from(const MemoryHashMap &p_t) {
		if (&p_t == this) {
			return; /* much less bother with that */
		}

		clear();

		if (!p_t.hash_table || p_t.hash_table_power <= 0) {
			return; /* not copying from empty table */
		}

		hash_table_power = p_t.hash_table_power;
		hash_table_size = (uint32_t)1 << p_t.hash_table_power;
		hash_table = new Element *[hash_table_size];
		elements = p_t.elements;

		for (int i = 0; i < (1 << hash_table_power); i++) {
			hash_table[i] = nullptr;

			const Element *e = p_t.hash_table[i];

			while (e) {
				Element *le = new Element(*e); /* local element */

				/* add to list and reassign pointers */
				le->next = hash_table[i];
				hash_table[i] = le;

				e = e->next;
			}
		}
	}

public:
	Element *set(const TKey &p_key, const TData &p_data) {
		return set(Pair(p_key, p_data));
	}

	Element *set(const Pair &p_pair) {
		Element *e = nullptr;
		if (!hash_table) {
			make_hash_table(); // if no table, make one
		} else {
			e = const_cast<Element *>(get_element(p_pair.key));
		}

		/* if we made it up to here, the pair doesn't exist, create and assign */

		if (!e) {
			e = create_element(p_pair.key);
			if (!e) {
				return nullptr;
			}
			check_hash_table(); // perform mantenience routine
		}

		e->pair.data = p_pair.data;
		return e;
	}

	bool has(const TKey &p_key) const {
		return getptr(p_key) != nullptr;
	}

	bool try_get(const TKey &p_key, const TData *&data) const {
		data = getptr(p_key);
		return data != nullptr;
	}

	bool try_get(const TKey &p_key, TData *&data) {
		data = getptr(p_key);
		return data != nullptr;
	}

	/**
	 * Same as get, except it can return nullptr when item was not found.
	 * This is mainly used for speed purposes.
	 */

	_FORCE_INLINE_ TData *getptr(const TKey &p_key) {
		if (unlikely(!hash_table)) {
			return nullptr;
		}

		Element *e = const_cast<Element *>(get_element(p_key));

		if (e) {
			return &e->pair.data;
		}

		return nullptr;
	}

	_FORCE_INLINE_ const TData *getptr(const TKey &p_key) const {
		if (unlikely(!hash_table)) {
			return nullptr;
		}

		const Element *e = const_cast<Element *>(get_element(p_key));

		if (e) {
			return &e->pair.data;
		}

		return nullptr;
	}
	/**
	 * Erase an item, return true if erasing was successful
	 */

	bool erase(const TKey &p_key) {
		if (unlikely(!hash_table)) {
			return false;
		}

		uint64_t hash = HashMapHasherDefault::hash(p_key);
		uint32_t index = hash & ((1 << hash_table_power) - 1);

		Element *e = hash_table[index];
		Element *p = nullptr;
		while (e) {
			/* checking hash first avoids comparing key, which may take longer */
			if (e->hash == hash && e->key() == p_key) {
				if (p) {
					p->next = e->next;
				} else {
					//begin of list
					hash_table[index] = e->next;
				}

				delete e;
				elements--;

				if (elements == 0) {
					erase_hash_table();
				} else {
					check_hash_table();
				}
				return true;
			}

			p = e;
			e = e->next;
		}

		return false;
	}

	inline const TData &operator[](const TKey &p_key) const { //constref

		return get(p_key);
	}
	inline TData &operator[](const TKey &p_key) { //assignment

		Element *e = nullptr;
		if (!hash_table) {
			make_hash_table(); // if no table, make one
		} else {
			e = const_cast<Element *>(get_element(p_key));
		}

		/* if we made it up to here, the pair doesn't exist, create */
		if (!e) {
			e = create_element(p_key);
			//CRASH_COND(!e);
			check_hash_table(); // perform mantenience routine
		}

		return e->pair.data;
	}

	/**
	 * Get the next key to p_key, and the first key if p_key is null.
	 * Returns a pointer to the next key if found, nullptr otherwise.
	 * Adding/Removing elements while iterating will, of course, have unexpected results, don't do it.
	 *
	 * Example:
	 *
	 * 	const TKey *k=nullptr;
	 *
	 * 	while( (k=table.next(k)) ) {
	 *
	 * 		print( *k );
	 * 	}
	 *
	 */
	const TKey *next(const TKey *p_key) const {
		if (unlikely(!hash_table)) {
			return nullptr;
		}

		if (!p_key) { /* get the first key */

			for (int i = 0; i < (1 << hash_table_power); i++) {
				if (hash_table[i]) {
					return &hash_table[i]->pair.key;
				}
			}

		} else { /* get the next key */

			const Element *e = get_element(*p_key);
			if (!e) {
				return nullptr;
			}
			if (e->next) {
				/* if there is a "next" in the list, return that */
				return &e->next->pair.key;
			} else {
				/* go to next elements */
				uint32_t index = e->hash & ((1 << hash_table_power) - 1);
				index++;
				for (int i = index; i < (1 << hash_table_power); i++) {
					if (hash_table[i]) {
						return &hash_table[i]->pair.key;
					}
				}
			}

			/* nothing found, was at end */
		}

		return nullptr; /* nothing found */
	}

	inline unsigned int size() const {
		return elements;
	}

	inline bool is_empty() const {
		return elements == 0;
	}

	void clear() {
		/* clean up */
		if (hash_table) {
			int size = 1 << hash_table_power;
			for (int i = 0; i < size; i++) {
				while (hash_table[i]) {
					Element *e = hash_table[i];
					hash_table[i] = e->next;
					delete e;
				}
			}

			delete[] hash_table;
		}

		hash_table = nullptr;
		hash_table_size = 0;
		hash_table_power = 0;
		elements = 0;
	}

	void operator=(const MemoryHashMap &p_table) {
		copy_from(p_table);
	}

	MemoryHashMap() {}

	MemoryHashMap(const MemoryHashMap &p_table) {
		copy_from(p_table);
	}

	~MemoryHashMap() {
		clear();
	}
};
class SmallMemoryManager {
	// 存储的数据，内部用的联合，如果被回收了，那么里面的数据就没有意义了，就会变成链表的指针
	template <int Count>
	struct Data {
		union {
			Data *Next = nullptr;
			int8_t data[Count];
		};
	};
#define DECL_MEMUBER_TYPE(Count)         \
	Data<Count> *_data##Count = nullptr; \
	int _count##Count = 0;               \
	int _curr_count##Count = 0;          \
	::Mutex _mutex##Count
	DECL_MEMUBER_TYPE(8);
	DECL_MEMUBER_TYPE(16);
	DECL_MEMUBER_TYPE(32);
	DECL_MEMUBER_TYPE(48);
	DECL_MEMUBER_TYPE(64);
	DECL_MEMUBER_TYPE(96);
	DECL_MEMUBER_TYPE(128);

	DECL_MEMUBER_TYPE(196);

	DECL_MEMUBER_TYPE(256);
	DECL_MEMUBER_TYPE(320);
	DECL_MEMUBER_TYPE(384);
	DECL_MEMUBER_TYPE(440);
	DECL_MEMUBER_TYPE(512);

	DECL_MEMUBER_TYPE(640);
	DECL_MEMUBER_TYPE(960);
	DECL_MEMUBER_TYPE(1024);
#undef DECL_MEMUBER_TYPE

public:
#define OP_MEMORY_NEW(Count)                                                \
	if (p_memory <= Count) {                                                \
		MutexLock<::Mutex> lock(_mutex##Count);                             \
		if (_data##Count) {                                                 \
			void *ret = (void *)&_data##Count->data;                        \
			_data##Count = _data##Count->Next;                              \
			--_count##Count;                                                \
			return ret;                                                     \
		} else {                                                            \
			Data<Count> *data = (Data<Count> *)malloc(sizeof(Data<Count>)); \
			data->Next = nullptr;                                           \
			++_curr_count##Count;                                           \
			return (void *)&data->data;                                     \
		}                                                                   \
	}
	void *alloc_manager(int p_memory) {
		OP_MEMORY_NEW(8)
		//
		else OP_MEMORY_NEW(16)
				//
				else OP_MEMORY_NEW(32)
				//
				else OP_MEMORY_NEW(48)
				//
				else OP_MEMORY_NEW(64)
				//
				else OP_MEMORY_NEW(96)
				//
				else OP_MEMORY_NEW(128)
				//
				else OP_MEMORY_NEW(196)
				//
				else OP_MEMORY_NEW(256)
				//
				else OP_MEMORY_NEW(320)
				//
				else OP_MEMORY_NEW(384)
				//
				else OP_MEMORY_NEW(440)
				//
				else OP_MEMORY_NEW(512)
				//
				else OP_MEMORY_NEW(640)
				//
				else OP_MEMORY_NEW(960)
				//
				else OP_MEMORY_NEW(1024)
				//
				return nullptr;
	}
#undef OP_MEMORY_NEW
#define OP_MEMORY_FREE(Count)                                                       \
	if (p_memory <= Count) {                                                        \
		MutexLock<::Mutex> lock(_mutex##Count);                                     \
		/*Data<Count> *data = (Data<Count> *)(((uint8_t *)ptr) - sizeof(void *));*/ \
		Data<Count> *data = (Data<Count> *)ptr;                                     \
		if (_count##Count < 512) {                                                  \
			data->Next = _data##Count;                                              \
			_data##Count = data;                                                    \
			++_count##Count;                                                        \
			return;                                                                 \
		}                                                                           \
		--_curr_count##Count;                                                       \
		free(data);                                                                 \
		return;                                                                     \
	}
	void free_manager(void *ptr, size_t p_memory) {
		if (ptr == nullptr || p_memory == 0) {
			return;
		}
		OP_MEMORY_FREE(8)
		//
		else OP_MEMORY_FREE(16)
				//
				else OP_MEMORY_FREE(32)
				//
				else OP_MEMORY_FREE(48)
				//
				else OP_MEMORY_FREE(64)
				//
				else OP_MEMORY_FREE(96)
				//
				else OP_MEMORY_FREE(128)
				//
				else OP_MEMORY_FREE(196)
				//
				else OP_MEMORY_FREE(256)
				//
				else OP_MEMORY_FREE(320)
				//
				else OP_MEMORY_FREE(384)
				//
				else OP_MEMORY_FREE(440)
				//
				else OP_MEMORY_FREE(512)
				//
				else OP_MEMORY_FREE(640)
				//
				else OP_MEMORY_FREE(960)
				//
				else OP_MEMORY_FREE(1024)
		//
	}
#undef OP_MEMORY_FREE
	static SmallMemoryManager &get() {
		static SmallMemoryManager *instance = new SmallMemoryManager;
		return *instance;
	}
	void record_memory_alloc(void *ptr, size_t p_memory, const char *file_name, int file_lne) {
		MemoryDebugInfo key;
		key.file_name = file_name;
		key.line = file_lne;
		MutexLock<::Mutex> lock(_memory_info_mutex);
		MemmoryInfo *info = nullptr;
		if (_memory_info.try_get(key, info)) {
			info->size += p_memory;
			info->count += 1;
		} else {
			MemmoryInfo t;
			t.size = p_memory;
			t.count = 1;
			_memory_info.set(key, t);
		}
		// 存储指针映射
		point_map.set(ptr, key);
		++TestCount;
		if (TestCount > 50000) {
			TestCount = 0;
			log_memory_info();
		}
	}
	void record_memory_free(void *ptr, size_t size) {
		MemoryDebugInfo *key = nullptr;
		MutexLock<::Mutex> lock(_memory_info_mutex);
		if (point_map.try_get(ptr, key)) {
			MemmoryInfo *info = nullptr;
			if (_memory_info.try_get(*key, info)) {
				info->size -= size;
				info->count -= 1;
				_memory_info.erase(*key);
			}
			point_map.erase(ptr);
		} else {
			printf("error : not fund ptr!!");
			point_map.try_get(ptr, key);
		}
	}
	struct MemoryLog {
		const char *file_name;
		int file_line;
		int total_memory;
		int total_count;
		MemoryLog *Next;
	};
	void log_memory_info() {
		MemoryLog *Root = nullptr;
		int count = 0;
		int total = 0;
		{
			MutexLock<::Mutex> lock(_memory_info_mutex);
			int size = 1 << _memory_info.hash_table_power;
			MemoryHashMap<MemoryDebugInfo, MemmoryInfo>::Element *e;
			for (int i = 0; i < size; i++) {
				e = _memory_info.hash_table[i];
				while (e != nullptr) {
					MemoryLog *info = (MemoryLog *)alloca(sizeof(MemoryLog));
					info->file_name = e->key().file_name;
					info->file_line = e->key().line;
					info->total_count = e->value().count;
					info->total_memory = e->value().size;
					total += info->total_memory;
					info->Next = Root;
					Root = info;
					e = e->next;
					++count;
				}
			}
		}
		// 下面开始打印日志
		float mb = ((float)total) / 1024.0f;
		if (mb < 1024.0f)
			printf("--------------begin log memory count:%d size: %0.3f KB------------\n", count, mb);
		else {
			mb /= 1024.0f;
			printf("--------------begin log memory count:%d size: %0.3fMB------------\n", count, mb);
		}
		while (Root != nullptr) {
			if (Root->total_count > 100) {
				mb = ((float)Root->total_memory) / 1024.0f;
				if (mb < 1024.0f)
					printf("    source: %s ,line : %d ,memory size: %0.3fKB,count: %d\n", Root->file_name, (int)Root->file_line, mb, (int)Root->total_count);
				else {
					mb /= 1024.0f;
					printf("    source: %s ,line : %d ,memory size: %0.3fMB,count: %d\n", Root->file_name, (int)Root->file_line, mb, (int)Root->total_count);
				}
			}
			Root = Root->Next;
		}
		printf("------------------end log memory ----------------\n\n\n\n");
	}

private:
	MemoryHashMap<MemoryDebugInfo, MemmoryInfo> _memory_info;
	MemoryHashMap<void *, MemoryDebugInfo> point_map;
	int TestCount = 0;
	::Mutex _memory_info_mutex;

	SmallMemoryManager() {
	}
};

void DefaultAllocator::record_memory_alloc(void *p_ptr, size_t p_memory, const char *file_name, int file_lne) {
	//SmallMemoryManager::get().record_memory_alloc(p_ptr, p_memory, file_name, file_lne);
}
void DefaultAllocator::record_memory_free(void *ptr, size_t size) {
	//SmallMemoryManager::get().record_memory_free(ptr, size);
}
void DefaultAllocator::log_memory_info() {
	//SmallMemoryManager::get().log_memory_info();
}
void *
DefaultAllocator::alloc_manager(size_t p_memory) {
	return SmallMemoryManager::get().alloc_manager(p_memory);
}
void DefaultAllocator::free_manager(void *ptr, size_t count) {
	SmallMemoryManager::get().free_manager(ptr, count);
}
void *operator new(size_t p_size, const char *p_description, size_t line) {
	size_t len = p_size + sizeof(uint64_t);
	uint32_t *ptr = (uint32_t *)DefaultAllocator::alloc(len, p_description, line);
	// 记录内存大小
	*ptr = MEMORY_TAG_NEW;
	++ptr;
	*ptr = (uint32_t)p_size;
	++ptr;
	return ptr;
}

void *operator new(size_t p_size, void *(*p_allocfunc)(size_t p_size, const char *file_name, int file_lne), const char *p_description, size_t line) {
	return p_allocfunc(p_size, p_description, line);
}

#ifdef _MSC_VER
void operator delete(void *p_mem, const char *p_description) {
	CRASH_NOW_MSG("Call to placement delete should not happen.");
}

void operator delete(void *p_mem, void *(*p_allocfunc)(size_t p_size)) {
	CRASH_NOW_MSG("Call to placement delete should not happen.");
}

void operator delete(void *p_mem, void *p_pointer, size_t check, const char *p_description) {
	CRASH_NOW_MSG("Call to placement delete should not happen.");
}
#endif

void *MallocAllocator::alloc_memory(size_t p_memory, const char *file_name, int file_lne) {
	size_t size = p_memory + sizeof(uint64_t);
	uint32_t *p = (uint32_t *)DefaultAllocator::alloc(size, file_name, file_lne);

	*p = MEMORY_TAG_MALLOC;
	uint32_t tag = *p;
	++p;
	*p = p_memory;
	++p;
	return p;
}
void MallocAllocator::free_memory(void *p_ptr) {
	uint32_t *base = (uint32_t *)p_ptr;
	base -= 2;
	uint32_t tag, size;
	tag = *base;
	size = *(base + 1);
	if (tag != MEMORY_TAG_MALLOC) {
		throw std::runtime_error("memory free error!");
	}
	DefaultAllocator::free(base, size + sizeof(uint64_t));
}
void *MallocAllocator::realloc_memory(void *p_ptr, size_t p_new_size, const char *file_name, int file_lne) {
	if (!p_ptr) {
		if (p_new_size == 0) {
			return nullptr;
		}
		// NULL ptr. realloc should act like malloc.
		return alloc_memory(p_new_size, file_name, file_lne);
	}
	if (p_new_size == 0) {
		free_memory(p_ptr);
		return nullptr;
	}
	uint32_t *base = (uint32_t *)p_ptr;
	base -= 2;
	uint32_t tag, size;
	tag = *base;
	size = *(base + 1);
	if (tag != MEMORY_TAG_MALLOC) {
		throw std::runtime_error("memory free error!");
	}
	void *new_ptr;
	new_ptr = alloc_memory(p_new_size, file_name, file_lne);
	if (!new_ptr) {
		free_memory(p_ptr);
		return NULL; // TODO: set errno on failure.
	}
	// 下面拷贝旧的数据
	if (size > p_new_size) {
		size = p_new_size;
	}
	memcpy(new_ptr, p_ptr, size);
	free_memory(p_ptr);
	return new_ptr;
}

#ifdef DEBUG_ENABLED
SafeNumeric<uint64_t> Memory::mem_usage;
SafeNumeric<uint64_t> Memory::max_usage;
#endif

SafeNumeric<uint64_t> Memory::alloc_count;

void *Memory::alloc_static(size_t p_bytes, const char *p_description, int line, bool p_pad_align) {
	//if (line > 0)
	//printf("memory new size %d ,source %s ,line : %d\n", (int)p_bytes, p_description, (int)line);
#ifdef DEBUG_ENABLED
	bool prepad = true;
#else
	bool prepad = p_pad_align;
#endif

	void *mem = malloc(p_bytes + (prepad ? PAD_ALIGN : 0));
	//void *mem = TCMallocInternalMalloc(p_bytes + (prepad ? PAD_ALIGN : 0));

	ERR_FAIL_COND_V(!mem, nullptr);

	alloc_count.increment();

	if (prepad) {
		uint64_t *s = (uint64_t *)mem;
		*s = p_bytes;

		uint8_t *s8 = (uint8_t *)mem;

		//#ifdef DEBUG_ENABLED
		//		uint64_t new_mem_usage = mem_usage.add(p_bytes);
		//		max_usage.exchange_if_greater(new_mem_usage);
		//#endif
		return s8 + PAD_ALIGN;
	} else {
		return mem;
	}
}

void *Memory::realloc_static(void *p_memory, size_t p_bytes, const char *p_description, int line, bool p_pad_align) {
	if (p_memory == nullptr) {
		return alloc_static(p_bytes, p_description, line, p_pad_align);
	}

	//if (line > 0)
	//printf("memory new size %d ,source %s ,line : %d\n", (int)p_bytes, p_description, (int)line);
	uint8_t *mem = (uint8_t *)p_memory;

#ifdef DEBUG_ENABLED
	bool prepad = true;
#else
	bool prepad = p_pad_align;
#endif

	if (prepad) {
		mem -= PAD_ALIGN;
		uint64_t *s = (uint64_t *)mem;

		//#ifdef DEBUG_ENABLED
		//		if (p_bytes > *s) {
		//			uint64_t new_mem_usage = mem_usage.add(p_bytes - *s);
		//			max_usage.exchange_if_greater(new_mem_usage);
		//		} else {
		//			mem_usage.sub(*s - p_bytes);
		//		}
		//#endif

		if (p_bytes == 0) {
			free(mem);
			//TCMallocInternalFree(mem);
			return nullptr;
		} else {
			*s = p_bytes;

			mem = (uint8_t *)realloc(mem, p_bytes + PAD_ALIGN);
			//mem = TCMallocInternalRealloc(mem, p_bytes + PAD_ALIGN);
			ERR_FAIL_COND_V(!mem, nullptr);

			s = (uint64_t *)mem;

			*s = p_bytes;

			return mem + PAD_ALIGN;
		}
	} else {
		mem = (uint8_t *)realloc(mem, p_bytes);
		//mem = (uint8_t *)TCMallocInternalRealloc(mem, p_bytes);

		ERR_FAIL_COND_V(mem == nullptr && p_bytes > 0, nullptr);

		return mem;
	}
}

void Memory::free_static(void *p_ptr, bool p_pad_align) {
	ERR_FAIL_COND(p_ptr == nullptr);

	uint8_t *mem = (uint8_t *)p_ptr;

#ifdef DEBUG_ENABLED
	bool prepad = true;
#else
	bool prepad = p_pad_align;
#endif

	alloc_count.decrement();

	if (prepad) {
		mem -= PAD_ALIGN;

		//#ifdef DEBUG_ENABLED
		//		uint64_t *s = (uint64_t *)mem;
		//		mem_usage.sub(*s);
		//#endif

		free(mem);
		//TCMallocInternalFree(mem);
	} else {
		free(mem);
		//TCMallocInternalFree(mem);
	}
}

uint64_t Memory::get_mem_available() {
	return -1; // 0xFFFF...
}

uint64_t Memory::get_mem_usage() {
#ifdef DEBUG_ENABLED
	return mem_usage.get();
#else
	return 0;
#endif
}

uint64_t Memory::get_mem_max_usage() {
#ifdef DEBUG_ENABLED
	return max_usage.get();
#else
	return 0;
#endif
}

_GlobalNil::_GlobalNil() {
	left = this;
	right = this;
	parent = this;
}

_GlobalNil _GlobalNilClass::_nil;
