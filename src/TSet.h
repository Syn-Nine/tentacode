#ifndef TSET_H
#define TSET_H

#include <assert.h>
#include <stdint.h>
#include <set>
#include "Literal.h"

class TSet
{
public:
	TSet() = delete;
	TSet(LiteralTypeEnum type, int bits)
	{
		m_iter = 0;
		m_empty = true;
		m_bits = bits;
		m_is_integer = LITERAL_TYPE_INTEGER == type;
		m_is_string = LITERAL_TYPE_STRING == type;

		assert(m_is_integer || m_is_string);
		assert(16 == bits || 32 == bits || 64 == bits);

		/*
		m_span = span;
		m_count = 0;
		m_capacity = 10;
		m_empty = true;

		// pre-allocate capacity
		m_data = calloc(m_capacity, span);*/

	}

	~TSet()
	{
		/*DeleteStorage();
		if (m_data) free(m_data);
		m_data = nullptr;*/
	}

	void Insert_Int(int64_t val)
	{
		assert(m_is_integer);
		if (16 == m_bits) m_set16.insert(val);
		else if (32 == m_bits) m_set32.insert(val);
		else if (64 == m_bits) m_set64.insert(val);
		m_empty = false;
	}

	void Insert_String(std::string val)
	{
		assert(m_is_string);
		m_setStr.insert(val);
		m_empty = false;
	}
	
	int64_t Size() const
	{
		if (m_empty) return 0;

		if (m_is_integer)
		{
			if (16 == m_bits) return m_set16.size();
			else if (32 == m_bits) return m_set32.size();
			else if (64 == m_bits) return m_set64.size();
		}
		
		return m_setStr.size();
	}

	void StartIteration()
	{
		m_iter = 0;
		if (m_is_integer)
		{
			if (16 == m_bits) m_set16_iter = m_set16.begin();
			else if (32 == m_bits) m_set32_iter = m_set32.begin();
			else if (64 == m_bits) m_set64_iter = m_set64.begin();
		}
		else
		{
			m_setStr_iter = m_setStr.begin();
		}
	}

	int16_t GetIterValue_i16()
	{
		int16_t ret = *m_set16_iter;
		if (m_iter < Size())
		{
			++m_iter;
			++m_set16_iter;
		}
		return ret;
	}

	int32_t GetIterValue_i32()
	{
		int32_t ret = *m_set32_iter;
		if (m_iter < Size())
		{
			++m_iter;
			++m_set32_iter;
		}
		return ret;
	}

	int64_t GetIterValue_i64()
	{
		int64_t ret = *m_set64_iter;
		if (m_iter < Size())
		{
			++m_iter;
			++m_set64_iter;
		}
		return ret;
	}

	std::string GetIterValue_str()
	{
		std::string ret = *m_setStr_iter;
		if (m_iter < Size())
		{
			++m_iter;
			++m_setStr_iter;
		}
		return ret;
	}
	
	/*
	bool Empty() const { return m_empty; }

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
	int64_t Span() const { return m_span; }
	
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

	void Replace(TSet* src)
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
	
	
	*/
	bool m_is_string;
	bool m_is_integer;
	bool m_empty;
	int m_bits;
	int m_iter;
	
	std::set<int16_t> m_set16;
	std::set<int16_t>::iterator m_set16_iter;

	std::set<int32_t> m_set32;
	std::set<int32_t>::iterator m_set32_iter;

	std::set<int64_t> m_set64;
	std::set<int64_t>::iterator m_set64_iter;

	std::set<std::string> m_setStr;
	std::set<std::string>::iterator m_setStr_iter;

};




#endif // TSET_H