#ifndef TVEC_H
#define TVEC_H

#include <stdint.h>
#include <memory>
#include "Literal.h"

class TVec
{
public:
	TVec() = delete;
	TVec(LiteralTypeEnum vtype, size_t span)
	{
		m_vtype = vtype;
		m_span = span;
		m_count = 0;
		m_capacity = 10;
		m_empty = true;

		// pre-allocate capacity
		m_data = calloc(m_capacity, span);
	}

	~TVec()
	{
		DeleteStorage();
		if (m_data) free(m_data);
		m_data = nullptr;
	}

	int64_t Size() const { return m_count; }
	
	bool Empty() const { return m_empty; }

	void Clear()
	{
		if (m_empty) return;

		DeleteStorage();
		m_count = 0;
		m_empty = true;
		//printf("clear\n");
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

	void Replace(TVec* src)
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
	}

private:

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
	LiteralTypeEnum m_vtype;
};




#endif // TVEC_H