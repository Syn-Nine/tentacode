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

	}

	~TSet()
	{
	}

	void Clear()
	{
		if (m_empty) return;
		if (m_is_integer)
		{
			if (16 == m_bits) m_set16.clear();
			else if (32 == m_bits) m_set32.clear();
			else if (64 == m_bits) m_set64.clear();
		}
		else
		{
			m_setStr.clear();
		}
		m_empty = true;
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
	

private:
	
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