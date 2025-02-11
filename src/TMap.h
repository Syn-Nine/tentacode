#ifndef TMAP_H
#define TMAP_H

#include <stdint.h>
#include <map>
#include "Literal.h"

class TMap
{
public:
	TMap() = delete;
	TMap(LiteralTypeEnum type, size_t bits, size_t span)
	{
		m_iter = 0;
		m_empty = true;
		m_bits = bits;
		m_span = span;
		m_is_integer = LITERAL_TYPE_INTEGER == type;
		m_is_string = LITERAL_TYPE_STRING == type;

		assert(m_is_integer || m_is_string);
		assert(16 == bits || 32 == bits || 64 == bits);

		// todo - pre-allocate storage so some data is contiguous

		/*m_vtype = vtype;
		m_span = span;
		m_count = 0;
		m_capacity = 10;
		m_empty = true;

		// pre-allocate capacity
		m_data = calloc(m_capacity, span);*/
	}

	~TMap()
	{
		/*DeleteStorage();
		if (m_data) free(m_data);
		m_data = nullptr;*/
	}

	bool ContainsKeyInt(int64_t key)
	{
		//printf("testing map for key: %ll\n", key);
		if (16 == m_bits) return m_map16.count(key) != 0;
		else if (32 == m_bits) return m_map32.count(key) != 0;
		else if (64 == m_bits) return m_map64.count(key) != 0;
		return false;
	}

	bool ContainsKeyString(std::string key)
	{
		//printf("testing map for key: %s\n", key.c_str());
		return m_mapStr.count(key.c_str()) != 0;
	}

	void InsertAtInt(int64_t key, void* data)
	{
		assert(m_is_integer);
		// allocate and memcopy data
		void* dst = malloc(m_span);
		memcpy(dst, data, m_span);
		// store
		if (16 == m_bits) m_map16.insert(std::make_pair(key, dst));
		else if (32 == m_bits) m_map32.insert(std::make_pair(key, dst));
		else if (64 == m_bits) m_map64.insert(std::make_pair(key, dst));
		m_empty = false;
	}

	void InsertAtString(std::string key, void* data)
	{
		assert(m_is_string);
		// allocate and memcopy data
		void* dst = malloc(m_span);
		memcpy(dst, data, m_span);
		// store
		m_mapStr.insert(std::make_pair(key, dst));
		m_empty = false;
	}

	int64_t Size() const
	{
		if (m_empty) return 0;

		if (m_is_integer)
		{
			if (16 == m_bits) return m_map16.size();
			else if (32 == m_bits) return m_map32.size();
			else if (64 == m_bits) return m_map64.size();
		}

		return m_mapStr.size();
	}

	int64_t Span() const { return m_span; }

	void StartIteration()
	{
		m_iter = 0;
		if (m_is_integer)
		{
			if (16 == m_bits) m_map16_iter = m_map16.begin();
			else if (32 == m_bits) m_map32_iter = m_map32.begin();
			else if (64 == m_bits) m_map64_iter = m_map64.begin();
		}
		else
		{
			m_mapStr_iter = m_mapStr.begin();
		}
	}

	int16_t GetIterKey_i16()
	{
		auto x = *m_map16_iter;
		return x.first;
	}

	int32_t GetIterKey_i32()
	{
		auto x = *m_map32_iter;
		return x.first;
	}

	int64_t GetIterKey_i64()
	{
		auto x = *m_map64_iter;
		return x.first;
	}

	std::string GetIterKey_str()
	{
		auto x = *m_mapStr_iter;
		return x.first;
	}

	void* GetIterValue()
	{
		if (m_is_integer)
		{
			if (16 == m_bits)
			{
				auto x = *m_map16_iter;
				if (m_iter < Size())
				{
					++m_iter;
					++m_map16_iter;
				}
				return x.second;
			}
			else if (32 == m_bits)
			{
				auto x = *m_map32_iter;
				if (m_iter < Size())
				{
					++m_iter;
					++m_map32_iter;
				}
				return x.second;
			}
			else if (64 == m_bits)
			{
				auto x = *m_map64_iter;
				if (m_iter < Size())
				{
					++m_iter;
					++m_map64_iter;
				}
				return x.second;
			}
		}
		else
		{
			auto x = *m_mapStr_iter;
			if (m_iter < Size())
			{
				++m_iter;
				++m_mapStr_iter;
			}
			return x.second;
		}
	}

	void GetAtInt(int64_t idx, void* dst)
	{
		assert(m_is_integer);
		if (16 == m_bits && 0 != m_map16.count(idx)) memcpy(dst, m_map16.at(idx), m_span);
		else if (32 == m_bits && 0 != m_map32.count(idx)) memcpy(dst, m_map32.at(idx), m_span);
		else if (64 == m_bits && 0 != m_map64.count(idx)) memcpy(dst, m_map64.at(idx), m_span);
		else
			printf("Invalid map index: %d\n", idx);
	}

	void SetAtInt(int64_t idx, void* src)
	{
		assert(m_is_integer);
		if (16 == m_bits && 0 != m_map16.count(idx)) memcpy(m_map16.at(idx), src, m_span);
		else if (32 == m_bits && 0 != m_map32.count(idx)) memcpy(m_map32.at(idx), src, m_span);
		else if (64 == m_bits && 0 != m_map64.count(idx)) memcpy(m_map64.at(idx), src, m_span);
		else
			InsertAtInt(idx, src);
	}

	void GetAtStr(std::string idx, void* dst)
	{
		assert(m_is_string);
		if (0 != m_mapStr.count(idx)) memcpy(dst, m_mapStr.at(idx), m_span);
		else
			printf("Invalid map index: %s\n", idx.c_str());
	}

	void SetAtStr(std::string idx, void* src)
	{
		assert(m_is_string);
		if (0 != m_mapStr.count(idx))
			memcpy(m_mapStr.at(idx), src, m_span);
		else
			InsertAtString(idx, src);
	}
	
	/*
	bool IsEmpty() const { return m_empty; }

	void Clear()
	{
		if (m_empty) return;

		DeleteStorage();
		m_count = 0;
		m_empty = true;
		//printf("clear\n");
	}

	bool Contains_i16(int16_t val)
	{
		if (m_empty) return false;
		if (LITERAL_TYPE_INTEGER != m_vtype) return false;
		if (m_span != 2) return false;

		int16_t* p = static_cast<int16_t*>(m_data);
		for (size_t i = 0; i < m_count; ++i)
		{
			if (p[i] == val) return true;
		}
		return false;
	}

	bool Contains_i32(int32_t val)
	{
		if (m_empty) return false;
		if (LITERAL_TYPE_INTEGER != m_vtype) return false;
		if (m_span != 4) return false;

		int32_t* p = static_cast<int32_t*>(m_data);
		for (size_t i = 0; i < m_count; ++i)
		{
			if (p[i] == val) return true;
		}
		return false;
	}

	bool Contains_i64(int64_t val)
	{
		if (m_empty) return false;
		if (LITERAL_TYPE_INTEGER != m_vtype) return false;
		if (m_span != 8) return false;

		int64_t* p = static_cast<int64_t*>(m_data);
		for (size_t i = 0; i < m_count; ++i)
		{
			if (p[i] == val) return true;
		}
		return false;
	}

	void Fill_bool(bool val, size_t qty)
	{
		Clear();
		Reserve(qty);
		m_count = qty;
		m_empty = false;
		bool* p = static_cast<bool*>(m_data);
		for (size_t i = 0; i < qty; ++i)
		{
			p[i] = val;
		}
	}

	void Fill_i32(int32_t val, size_t qty)
	{
		Clear();
		Reserve(qty);
		m_count = qty;
		m_empty = false;
		int32_t* p = static_cast<int32_t*>(m_data);
		for (size_t i = 0; i < qty; ++i)
		{
			p[i] = val;
		}
	
	}
	void Fill_f32(float val, size_t qty)
	{
		Clear();
		Reserve(qty);
		m_count = qty;
		m_empty = false;
		float* p = static_cast<float*>(m_data);
		for (size_t i = 0; i < qty; ++i)
		{
			p[i] = val;
		}
	}

	void Reserve(size_t capacity)
	{
		// over-allocate
		size_t new_capacity = capacity * 1.2;
		if (new_capacity <= m_capacity) return;

		void* p = calloc(new_capacity, m_span);
		memcpy(p, m_data, m_capacity * m_span);
		free(m_data);
		m_data = p;
		m_capacity = new_capacity;
	}

	// about to fill vector, move the count to the new size
	void Presize(size_t size)
	{
		Reserve(size);
		m_count = size;
		m_empty = false;
	}

	LiteralTypeEnum VecType() const { return m_vtype; }
	
	
	void* At(size_t idx)
	{
		if (m_empty) return nullptr;
		if (idx < m_count)
		{
			return static_cast<uint8_t*>(m_data) + idx * m_span;
		}
		return nullptr;
	}

	void* Data() { return m_data; }

	void Replace(TMap* src)
	{
		Clear();
		Presize(src->m_count);

		if (LITERAL_TYPE_STRING == m_vtype)
		{
			// deep copy
			std::string** dstPtr = static_cast<std::string**>(m_data);
			std::string** srcPtr = static_cast<std::string**>(src->m_data);
			for (size_t i = 0; i < src->m_count; ++i)
			{
				dstPtr[i] = new std::string(*(srcPtr[i]));
			}
		}
		else
		{
			memcpy(m_data, src->m_data, src->m_count * m_span);
		}
	}

	void* PushBackPtr()
	{
		if (m_count == m_capacity) Reserve(m_capacity * 2.2);
		m_count += 1;
		m_empty = false;
		return static_cast<uint8_t*>(m_data) + (m_count - 1) * m_span;
	}*/

private:
	/*
	void DeleteStorage()
	{
		if (LITERAL_TYPE_STRING == m_vtype)
		{
			for (size_t i = 0; i < m_count; ++i)
			{
				std::string* p = *(static_cast<std::string**>(At(i)));
				delete p;
			}
		}
	}

	void* m_data;         // pointer to data
	int64_t m_span;       // size of one element in bytes
	int64_t m_count;      // number of elements in vector
	int64_t m_capacity;   // capacity in element counts
	bool m_empty;
	LiteralTypeEnum m_vtype;*/

	bool m_is_string;
	bool m_is_integer;
	bool m_empty;
	int m_bits;
	int m_iter;
	int64_t m_span;

	std::map<int16_t, void*> m_map16;
	std::map<int16_t, void*>::iterator m_map16_iter;

	std::map<int32_t, void*> m_map32;
	std::map<int32_t, void*>::iterator m_map32_iter;

	std::map<int64_t, void*> m_map64;
	std::map<int64_t, void*>::iterator m_map64_iter;

	std::map<std::string, void*> m_mapStr;
	std::map<std::string, void*>::iterator m_mapStr_iter;
};


#endif // TMAP_H