#ifndef EXTENSIONS_H
#define EXTENSIONS_H
#pragma once

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"

#include "Environment.h"
#include "Literal.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <random>
#include <istream>
#include <chrono>
#include <algorithm>


#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

extern "C" DLLEXPORT void input(char* p)
{
	if (p) std::cin.getline(p, 255);
}

extern "C" DLLEXPORT void print(std::string* p)
{
	if (p) printf("%s", p->c_str());
}

extern "C" DLLEXPORT void println(std::string* p)
{
	if (p)
		printf("%s\n", p->c_str());
	else
		printf("\n");
}

extern "C" DLLEXPORT double clock_impl()
{
	const auto p = std::chrono::system_clock::now();
	return double(std::chrono::duration_cast<std::chrono::microseconds>(p.time_since_epoch()).count() / 1000000.0);
}

extern "C" DLLEXPORT double pow_impl(double base, double exponent)
{
	return powf(base, exponent);
}

extern "C" DLLEXPORT int32_t mod_impl(int32_t a, int32_t b)
{
	return a % b;
}

extern "C" DLLEXPORT double __sgn(double val)
{
	if (val < 0) return -1.0;
	return 1.0;
}


/*inline int32_t random(int32_t low, int32_t high) {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_int_distribution<> dist(low, high);
	return dist(gen);
}*/

extern "C" DLLEXPORT double rand_impl()
{
	// todo - go back and profile
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_real_distribution<> dist(0.0, 1.0);
	return dist(gen);
}

extern "C" DLLEXPORT int rand_range_impl(int lhs, int rhs)
{
	// todo - go back and profile
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_int_distribution<> dist(lhs, rhs);
	return dist(gen);
}

// file
extern "C" DLLEXPORT void* __file_readlines(std::string* filename)
{
	std::ifstream file(filename->c_str());

	llvm::SmallVector<std::string*>* ret = new llvm::SmallVector<std::string*>();

	if (file.is_open())
	{
		for (std::string line; std::getline(file, line); )
		{
			ret->push_back(new std::string(line));
		}
	}

	return static_cast<void*>(ret);
}

extern "C" DLLEXPORT void __file_writelines(std::string* filename, llvm::SmallVector<std::string*>* srcPtr)
{
	std::ofstream file(filename->c_str());

	if (file.is_open())
	{
		for (auto& s : *srcPtr)
		{
			file << *s << std::endl;
		}
		file.close();
	}
}

//-----------------------------------------------------------------------------
// string
extern "C" DLLEXPORT std::string* __bool_to_string(bool val)
{
	if (val) return new std::string("true");
	return new std::string("false");
}

extern "C" DLLEXPORT void __del_std_string_ptr(std::string* str)
{
	delete str;
}

extern "C" DLLEXPORT std::string* __double_to_string(double val)
{
	return new std::string(std::to_string(val));
}

extern "C" DLLEXPORT std::string* __int_to_string(int32_t val)
{
	return new std::string(std::to_string(val));
}

extern "C" DLLEXPORT std::string* __new_std_string_ptr(char* str)
{
	return new std::string(str);
}

extern "C" DLLEXPORT std::string* __new_std_string_void()
{
	return new std::string();
}

extern "C" DLLEXPORT std::string* __std_string_cat(std::string* a, std::string* b)
{
	return new std::string(*a + *b);
}

extern "C" DLLEXPORT void __str_assign(std::string* a, std::string* b)
{
	*a = *b;
}

extern "C" DLLEXPORT bool __str_cmp(std::string* a, std::string* b)
{
	return 0 == a->compare(*b);
}

extern "C" DLLEXPORT int32_t __str_to_int(std::string* a)
{
	return std::stoi(*a);
}

extern "C" DLLEXPORT double __str_to_double(std::string* a)
{
	return std::stod(*a);
}


// string utilities
extern "C" DLLEXPORT bool __str_contains(std::string* src, std::string* val)
{
	return std::string::npos != src->find(*val);
}

extern "C" DLLEXPORT std::string* __str_replace(std::string* src, std::string* from, std::string* to)
{
	return new std::string(StrReplace(*src, *from, *to));
}


extern "C" DLLEXPORT void* __str_split(std::string* src, std::string* del)
{
	llvm::SmallVector<std::string*>* ret = new llvm::SmallVector<std::string*>();
	auto ss = StrSplit(*src, *del);
	for (auto& s : ss)
	{
		ret->push_back(new std::string(s));
	}
	return static_cast<void*>(ret);
}

extern "C" DLLEXPORT std::string* __str_join(void* src, std::string* del)
{
	llvm::SmallVector<std::string*>* scast = static_cast<llvm::SmallVector<std::string*>*>(src);
	llvm::SmallVector<std::string> tmp;
	for (auto& s : *scast)
	{
		tmp.push_back(*s);
	}
	return new std::string(StrJoin(tmp, *del));
}

extern "C" DLLEXPORT std::string* __str_substr(std::string* src, int32_t idx, int32_t len)
{
	if (idx < 0) idx += src->length();
	if (idx < 0) idx = 0;
	if (len < 0) return new std::string(src->substr(idx));
	
	if (idx + len <= src->length()) return new std::string(src->substr(idx, len));
	return new std::string(src->substr(idx));
}

extern "C" DLLEXPORT std::string* __str_toupper(std::string* src)
{
	std::string input = *src;
	std::transform(input.begin(), input.end(), input.begin(), ::toupper);
	return new std::string(input);
}

extern "C" DLLEXPORT std::string* __str_tolower(std::string* src)
{
	std::string input = *src;
	std::transform(input.begin(), input.end(), input.begin(), ::tolower);
	return new std::string(input);
}

extern "C" DLLEXPORT std::string* __str_ltrim(std::string* src)
{
	std::string input = *src;
	input.erase(0, input.find_first_not_of(" \t\n\r\f\v"));
	return new std::string(input);
}

extern "C" DLLEXPORT std::string* __str_rtrim(std::string* src)
{
	std::string input = *src;
	input.erase(input.find_last_not_of(" \t\n\r\f\v") + 1);
	return new std::string(input);
}

extern "C" DLLEXPORT std::string* __str_trim(std::string* src)
{
	std::string input = *src;
	input.erase(0, input.find_first_not_of(" \t\n\r\f\v")); // ltrim
	input.erase(input.find_last_not_of(" \t\n\r\f\v") + 1); // rtrim
	return new std::string(input);
}


//-----------------------------------------------------------------------------
// vector
extern "C" DLLEXPORT void* __vec_new(int vecType)
{
	LiteralTypeEnum vt = LiteralTypeEnum(vecType);

	void* ret = nullptr;

	if (LITERAL_TYPE_INTEGER == vt) ret = static_cast<void*>(new llvm::SmallVector<int32_t>());
	else if (LITERAL_TYPE_ENUM == vt) ret = static_cast<void*>(new llvm::SmallVector<int32_t>());
	else if (LITERAL_TYPE_BOOL == vt) ret = static_cast<void*>(new llvm::SmallVector<int8_t>());
	else if (LITERAL_TYPE_DOUBLE == vt) ret = static_cast<void*>(new llvm::SmallVector<double>());
	else if (LITERAL_TYPE_STRING == vt) ret = static_cast<void*>(new llvm::SmallVector<std::string*>());
	else if (LITERAL_TYPE_UDT == vt) ret = static_cast<void*>(new llvm::SmallVector<void*>());

	return ret;
}

extern "C" DLLEXPORT void* __vec_new_rep_i32(int32_t dstVal, int32_t dstQty)
{
	return new llvm::SmallVector<int32_t>(dstQty, dstVal);
}

extern "C" DLLEXPORT void* __vec_new_rep_enum(int32_t dstVal, int32_t dstQty)
{
	return __vec_new_rep_i32(dstQty, dstVal);
}

extern "C" DLLEXPORT void* __vec_new_rep_bool(bool dstVal, int32_t dstQty)
{
	return new llvm::SmallVector<int8_t>(dstQty, dstVal);
}

extern "C" DLLEXPORT void* __vec_new_rep_double(double dstVal, int32_t dstQty)
{
	return new llvm::SmallVector<double>(dstQty, dstVal);
}

extern "C" DLLEXPORT void __vec_del(int vecType, void* dstPtr)
{
	LiteralTypeEnum vt = LiteralTypeEnum(vecType);

	if (LITERAL_TYPE_INTEGER == vt) delete static_cast<llvm::SmallVector<int32_t>*>(dstPtr);
	if (LITERAL_TYPE_ENUM == vt) delete static_cast<llvm::SmallVector<int32_t>*>(dstPtr);
	if (LITERAL_TYPE_BOOL == vt) delete static_cast<llvm::SmallVector<bool>*>(dstPtr);
	if (LITERAL_TYPE_DOUBLE == vt) delete static_cast<llvm::SmallVector<double>*>(dstPtr);
	if (LITERAL_TYPE_STRING == vt)
	{
		llvm::SmallVector<std::string*>* dst = static_cast<llvm::SmallVector<std::string*>*>(dstPtr);
		for (auto& s : *dst)
		{
			delete s;
		}
		
	}
}

extern "C" DLLEXPORT void __vec_assign(int dstType, void* dstPtr, int srcType, void* srcPtr)
{
	//printf("__vec_assign(%d, %ld, %d, %ld, %d)\n", dstType, dstPtr, srcType, srcPtr, srcQty);

	if (dstType == srcType)
	{
		if (LITERAL_TYPE_INTEGER == dstType || LITERAL_TYPE_ENUM == dstType)
		{
			llvm::SmallVector<int32_t>* dst = static_cast<llvm::SmallVector<int32_t>*>(dstPtr);
			llvm::SmallVector<int32_t>* src = static_cast<llvm::SmallVector<int32_t>*>(srcPtr);
			(*dst) = (*src);
		}
		else if (LITERAL_TYPE_BOOL == dstType)
		{
			llvm::SmallVector<int8_t>* dst = static_cast<llvm::SmallVector<int8_t>*>(dstPtr);
			llvm::SmallVector<int8_t>* src = static_cast<llvm::SmallVector<int8_t>*>(srcPtr);
			(*dst) = (*src);
		}
		else if (LITERAL_TYPE_DOUBLE == dstType)
		{
			llvm::SmallVector<double>* dst = static_cast<llvm::SmallVector<double>*>(dstPtr);
			llvm::SmallVector<double>* src = static_cast<llvm::SmallVector<double>*>(srcPtr);
			(*dst) = (*src);
		}
		else if (LITERAL_TYPE_STRING == dstType)
		{
			llvm::SmallVector<std::string*>* dst = static_cast<llvm::SmallVector<std::string*>*>(dstPtr);
			llvm::SmallVector<std::string*>* src = static_cast<llvm::SmallVector<std::string*>*>(srcPtr);
			if (!dst->empty())
			{
				for (auto& s : *dst)
				{
					delete s;
				}
				dst->clear();
			}
			for (size_t i = 0; i < src->size(); ++i)
			{
				dst->push_back(new std::string(*(*src)[i]));
			}
		}
	}
	else if (LITERAL_TYPE_INTEGER == dstType && LITERAL_TYPE_DOUBLE == srcType)
	{
		llvm::SmallVector<int32_t>* dst = static_cast<llvm::SmallVector<int32_t>*>(dstPtr);
		llvm::SmallVector<double>* src = static_cast<llvm::SmallVector<double>*>(srcPtr);
		if (!dst->empty()) dst->clear();
		for (size_t i = 0; i < src->size(); ++i)
		{
			dst->push_back(int32_t((*src)[i]));
		}
	}
	else if (LITERAL_TYPE_DOUBLE == dstType && LITERAL_TYPE_INTEGER == srcType)
	{
		llvm::SmallVector<double>* dst = static_cast<llvm::SmallVector<double>*>(dstPtr);
		llvm::SmallVector<int32_t>* src = static_cast<llvm::SmallVector<int32_t>*>(srcPtr);
		if (!dst->empty()) dst->clear();
		for (size_t i = 0; i < src->size(); ++i)
		{
			dst->push_back(double((*src)[i]));
		}
	}
}

extern "C" DLLEXPORT void __vec_assign_fixed(int dstType, void* dstPtr, int srcType, void* srcPtr, int srcQty)
{
	//printf("__vec_assign(%d, %ld, %d, %ld, %d)\n", dstType, dstPtr, srcType, srcPtr, srcQty);
	if (dstType == srcType)
	{
		if (LITERAL_TYPE_INTEGER == dstType || LITERAL_TYPE_ENUM == dstType)
		{
			llvm::SmallVector<int32_t>* dst = static_cast<llvm::SmallVector<int32_t>*>(dstPtr);
			int32_t* src = static_cast<int32_t*>(srcPtr);
			if (!dst->empty()) dst->clear();
			if (dst->capacity() < srcQty) dst->reserve(srcQty);
			for (size_t i = 0; i < srcQty; ++i)
			{
				dst->push_back(src[i]);
				//printf("%d\n", (*dst)[i]);
			}
		}
		else if (LITERAL_TYPE_BOOL == dstType)
		{
			llvm::SmallVector<int8_t>* dst = static_cast<llvm::SmallVector<int8_t>*>(dstPtr);
			int8_t* src = static_cast<int8_t*>(srcPtr);
			if (!dst->empty()) dst->clear();
			if (dst->capacity() < srcQty) dst->reserve(srcQty);
			for (size_t i = 0; i < srcQty; ++i)
			{
				dst->push_back(src[i]);
				//printf("%d\n", (*dst)[i]);
			}
		}
		else if (LITERAL_TYPE_DOUBLE == dstType)
		{
			llvm::SmallVector<double>* dst = static_cast<llvm::SmallVector<double>*>(dstPtr);
			double* src = static_cast<double*>(srcPtr);
			if (!dst->empty()) dst->clear();
			if (dst->capacity() < srcQty) dst->reserve(srcQty);
			for (size_t i = 0; i < srcQty; ++i)
			{
				dst->push_back(src[i]);
				//printf("%d\n", (*dst)[i]);
			}
		}
		else if (LITERAL_TYPE_STRING == dstType)
		{
			llvm::SmallVector<std::string*>* dst = static_cast<llvm::SmallVector<std::string*>*>(dstPtr);
			std::string** src = static_cast<std::string**>(srcPtr);
			if (!dst->empty()) dst->clear();
			if (dst->capacity() < srcQty) dst->reserve(srcQty);
			for (size_t i = 0; i < srcQty; ++i)
			{
				dst->push_back(new std::string(*src[i]));
				//printf("%d\n", (*dst)[i]);
			}
		}
	}
	else if (LITERAL_TYPE_INTEGER == dstType && LITERAL_TYPE_DOUBLE == srcType)
	{
		llvm::SmallVector<int32_t>* dst = static_cast<llvm::SmallVector<int32_t>*>(dstPtr);
		double* src = static_cast<double*>(srcPtr);
		if (!dst->empty()) dst->clear();
		if (dst->capacity() < srcQty) dst->reserve(srcQty);
		for (size_t i = 0; i < srcQty; ++i)
		{
			dst->push_back(int32_t(src[i]));
			//printf("%d\n", (*dst)[i]);
		}
	}
	else if (LITERAL_TYPE_DOUBLE == dstType && LITERAL_TYPE_INTEGER == srcType)
	{
		llvm::SmallVector<double>* dst = static_cast<llvm::SmallVector<double>*>(dstPtr);
		int32_t* src = static_cast<int32_t*>(srcPtr);
		if (!dst->empty()) dst->clear();
		if (dst->capacity() < srcQty) dst->reserve(srcQty);
		for (size_t i = 0; i < srcQty; ++i)
		{
			dst->push_back(int32_t(src[i]));
			//printf("%d\n", (*dst)[i]);
		}
	}
}


extern "C" DLLEXPORT bool __vec_contains_i32(void* srcPtr, int32_t val)
{
	llvm::SmallVector<int32_t>* src = static_cast<llvm::SmallVector<int32_t>*>(srcPtr);
	return std::find(src->begin(), src->end(), val) != src->end();
}


extern "C" DLLEXPORT int32_t __vec_len(int srcType, void* srcPtr)
{
	//printf("__vec_len(%d, %ld)\n", srcType, srcPtr);
	if (LITERAL_TYPE_INTEGER == srcType || LITERAL_TYPE_ENUM == srcType)
	{
		llvm::SmallVector<int32_t>* src = static_cast<llvm::SmallVector<int32_t>*>(srcPtr);
		return src->size();
	}
	else if (LITERAL_TYPE_BOOL == srcType)
	{
		llvm::SmallVector<int8_t>* src = static_cast<llvm::SmallVector<int8_t>*>(srcPtr);
		return src->size();
	}
	else if (LITERAL_TYPE_DOUBLE == srcType)
	{
		llvm::SmallVector<double>* src = static_cast<llvm::SmallVector<double>*>(srcPtr);
		return src->size();
	}
	else if (LITERAL_TYPE_STRING == srcType)
	{
		llvm::SmallVector<std::string*>* src = static_cast<llvm::SmallVector<std::string*>*>(srcPtr);
		return src->size();
	}
	else if (LITERAL_TYPE_UDT == srcType)
	{
		llvm::SmallVector<void*>* src = static_cast<llvm::SmallVector<void*>*>(srcPtr);
		return src->size();
	}
	
	return 0;
}

extern "C" DLLEXPORT void __vec_set_i32(void* srcPtr, int32_t idx, int32_t val)
{
	llvm::SmallVector<int32_t>* src = static_cast<llvm::SmallVector<int32_t>*>(srcPtr);
	(*src)[idx] = val;
}

extern "C" DLLEXPORT void __vec_set_enum(void* srcPtr, int32_t idx, int32_t val)
{
	__vec_set_i32(srcPtr, idx, val);
}

extern "C" DLLEXPORT void __vec_set_bool(void* srcPtr, int32_t idx, bool val)
{
 	llvm::SmallVector<int8_t>* src = static_cast<llvm::SmallVector<int8_t>*>(srcPtr);
	if (val)
		(*src)[idx] = 1;
	else
		(*src)[idx] = 0;
	
}

extern "C" DLLEXPORT void __vec_set_double(void* srcPtr, int32_t idx, double val)
{
	llvm::SmallVector<double>* src = static_cast<llvm::SmallVector<double>*>(srcPtr);
	(*src)[idx] = val;
}

extern "C" DLLEXPORT void __vec_set_str(void* srcPtr, int32_t idx, std::string* val)
{
	llvm::SmallVector<std::string*>* src = static_cast<llvm::SmallVector<std::string*>*>(srcPtr);
	delete (*src)[idx]; // cleanup previous value
	(*src)[idx] = new std::string(*val);
}

extern "C" DLLEXPORT int32_t __vec_get_i32(void* srcPtr, int32_t idx)
{
	llvm::SmallVector<int32_t>* src = static_cast<llvm::SmallVector<int32_t>*>(srcPtr);
	return (*src)[idx];
}

extern "C" DLLEXPORT int32_t __vec_get_enum(void* srcPtr, int32_t idx)
{
	return __vec_get_i32(srcPtr, idx);
}

extern "C" DLLEXPORT bool __vec_get_bool(void* srcPtr, int32_t idx)
{
	llvm::SmallVector<int8_t>* src = static_cast<llvm::SmallVector<int8_t>*>(srcPtr);
	return (*src)[idx] != 0;
}

extern "C" DLLEXPORT double __vec_get_double(void* srcPtr, int32_t idx)
{
	llvm::SmallVector<double>* src = static_cast<llvm::SmallVector<double>*>(srcPtr);
	return (*src)[idx];
}

extern "C" DLLEXPORT std::string* __vec_get_str(void* srcPtr, int32_t idx)
{
	llvm::SmallVector<std::string*>* src = static_cast<llvm::SmallVector<std::string*>*>(srcPtr);
	return new std::string(*(*src)[idx]);
}

extern "C" DLLEXPORT void __vec_append_i32(int dstType, void* dstPtr, int32_t val)
{
	if (LITERAL_TYPE_INTEGER == dstType)
	{
		llvm::SmallVector<int32_t>* dst = static_cast<llvm::SmallVector<int32_t>*>(dstPtr);
		dst->push_back(val);
	}
}

extern "C" DLLEXPORT void __vec_append_enum(int dstType, void* dstPtr, int32_t val)
{
	if (LITERAL_TYPE_ENUM == dstType)
	{
		llvm::SmallVector<int32_t>* dst = static_cast<llvm::SmallVector<int32_t>*>(dstPtr);
		dst->push_back(val);
	}
}

extern "C" DLLEXPORT void __vec_append_bool(int dstType, void* dstPtr, bool val)
{
	if (LITERAL_TYPE_BOOL == dstType)
	{
		llvm::SmallVector<int8_t>* dst = static_cast<llvm::SmallVector<int8_t>*>(dstPtr);
		dst->push_back(val);
	}
}

extern "C" DLLEXPORT void __vec_append_double(int dstType, void* dstPtr, double val)
{
	if (LITERAL_TYPE_DOUBLE == dstType)
	{
		llvm::SmallVector<double>* dst = static_cast<llvm::SmallVector<double>*>(dstPtr);
		dst->push_back(val);
	}
}

extern "C" DLLEXPORT void __vec_append_str(int dstType, void* dstPtr, std::string* val)
{
	if (LITERAL_TYPE_STRING == dstType)
	{
		llvm::SmallVector<std::string*>* dst = static_cast<llvm::SmallVector<std::string*>*>(dstPtr);
		dst->push_back(new std::string(*val));
	}
}

extern "C" DLLEXPORT void __vec_append_udt(int dstType, void* dstPtr, void* val)
{
	if (LITERAL_TYPE_UDT == dstType)
	{
		llvm::SmallVector<void*>* dst = static_cast<llvm::SmallVector<void*>*>(dstPtr);
		dst->push_back(val);
	}
}

extern "C" DLLEXPORT void* __vec_to_string(int vecType, void* srcPtr)
{
	LiteralTypeEnum vt = LiteralTypeEnum(vecType);

	int sz = 0;
	std::string array;
	std::string vtype = "invalid";

	if (LITERAL_TYPE_INTEGER == vt)
	{
		llvm::SmallVector<int32_t>* src = static_cast<llvm::SmallVector<int32_t>*>(srcPtr);
		sz = src->size();
		vtype = "i32";
		for (size_t i = 0; i < sz; ++i)
		{
			if (0 == i)
			{
				array.append(std::to_string((*src)[i]));
			}
			else
			{
				array.append(", " + std::to_string((*src)[i]));
			}
		}
	}
	else if (LITERAL_TYPE_ENUM == vt)
	{
		llvm::SmallVector<int32_t>* src = static_cast<llvm::SmallVector<int32_t>*>(srcPtr);
		sz = src->size();
		vtype = "enum";
		for (size_t i = 0; i < sz; ++i)
		{
			if (0 == i)
			{
				array.append(std::to_string((*src)[i]));
			}
			else
			{
				array.append(", " + std::to_string((*src)[i]));
			}
		}
	}
	else if (LITERAL_TYPE_BOOL == vt)
	{
		llvm::SmallVector<bool>* src = static_cast<llvm::SmallVector<bool>*>(srcPtr);
		sz = src->size();
		vtype = "bool";
		for (size_t i = 0; i < sz; ++i)
		{
			if (0 == i)
			{
				array.append((*src)[i] ? "true" : "false");
			}
			else
			{
				std::string s = (*src)[i] ? "true" : "false";
				array.append(", " + s);
			}
		}
	}
	else if (LITERAL_TYPE_DOUBLE == vt)
	{
		llvm::SmallVector<double>* src = static_cast<llvm::SmallVector<double>*>(srcPtr);
		sz = src->size();
		vtype = "f32";
		for (size_t i = 0; i < sz; ++i)
		{
			if (0 == i)
			{
				array.append(std::to_string((*src)[i]));
			}
			else
			{
				array.append(", " + std::to_string((*src)[i]));
			}
		}
	}
	else if (LITERAL_TYPE_STRING == vt)
	{
		llvm::SmallVector<std::string*>* src = static_cast<llvm::SmallVector<std::string*>*>(srcPtr);
		sz = src->size();
		vtype = "string";
		for (size_t i = 0; i < sz; ++i)
		{
			if (0 == i)
			{
				array.append(*(*src)[i]);
			}
			else
			{
				array.append(", " + *(*src)[i]);
			}
		}
	}

	std::string ret = "<Vec," + vtype + ",Size:" + std::to_string(sz) + ">[" + array + "]";

	return new std::string(ret);
}


static void LoadExtensions(
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{

	{	// void = ftn(int)
		std::vector<llvm::Type*> args(1, builder->getInt32Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "input", *module);
	}

	{	// void = ftn(ptr)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), { builder->getPtrTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "println", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "print", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__del_std_string_ptr", *module);
	}

	{	// void = ftn(ptr, ptr)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), { builder->getPtrTy(), builder->getPtrTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_assign", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__file_writelines", *module);
	}

	{	// int = ftn(int)
		std::vector<llvm::Type*> args(1, builder->getInt32Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt32Ty(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "atoi", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "abs", *module);
	}

	{	// double = ftn(int)
		std::vector<llvm::Type*> args(1, builder->getInt32Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getDoubleTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "atof", *module);
	}

	{	// double = ftn(double)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getDoubleTy(), { builder->getDoubleTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "fabs", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "cos", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "sin", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "tan", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "acos", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "asin", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "atan", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "floor", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "sqrt", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__sgn", *module);
	}

	{	// double = ftn(double, double)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getDoubleTy(), { builder->getDoubleTy(), builder->getDoubleTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "atan2", *module);
	}

	{	// ptr = ftn(void)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), { }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__new_std_string_void", *module);
	}

	{	// ptr = ftn(ptr)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), { builder->getPtrTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__new_std_string_ptr", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__file_readlines", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_toupper", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_tolower", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_ltrim", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_rtrim", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_trim", *module);
	}

	{	// ptr = ftn(ptr, ptr)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), { builder->getPtrTy(), builder->getPtrTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__std_string_cat", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_split", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_join", *module);
	}

	{	// ptr = ftn(ptr, ptr, ptr)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), { builder->getPtrTy(), builder->getPtrTy(), builder->getPtrTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_replace", *module);
	}

	{	// ptr = ftn(ptr, int, int)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), { builder->getPtrTy(), builder->getInt32Ty(), builder->getInt32Ty() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_substr", *module);
	}
	
	{	// bool = ftn(ptr, ptr)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt1Ty(), { builder->getPtrTy(), builder->getPtrTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_cmp", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_contains", *module);
	}

	{	// ptr = ftn(int)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), { builder->getInt32Ty(), builder->getPtrTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__int_to_string", *module);
	}

	{	// ptr = ftn(double)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), { builder->getDoubleTy(), builder->getPtrTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__double_to_string", *module);
	}

	{	// ptr = ftn(bool)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), { builder->getInt1Ty(), builder->getPtrTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__bool_to_string", *module);
	}

	{	// int = ftn(ptr)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt32Ty(), { builder->getPtrTy(), builder->getPtrTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_to_int", *module);
	}

	{	// double = ftn(ptr)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getDoubleTy(), { builder->getPtrTy(), builder->getPtrTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_to_double", *module);
	}



	{
		std::vector<llvm::Type*> args(2, builder->getInt32Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt32Ty(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "mod_impl", *module);
	}

	{	// double = ftn(double, double)
		std::vector<llvm::Type*> args(2, builder->getDoubleTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getDoubleTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "pow_impl", *module);
	}


	{	// double = ftn()
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getDoubleTy(), {}, false);
		llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "clock_impl", *module);
	}

	{
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getDoubleTy(), {}, false);
		llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "rand_impl", *module);
	}

	{
		std::vector<llvm::Type*> args(2, builder->getInt32Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt32Ty(), args, false);
		llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, "rand_range_impl", *module);
	}

	// vectors
	{
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), { builder->getInt32Ty() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_new", *module);
	}
	{
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), { builder->getInt32Ty(), builder->getPtrTy()}, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_del", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getInt32Ty()); // val
		args.push_back(builder->getInt32Ty()); // qty
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_new_rep_i32", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_new_rep_enum", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getInt8Ty()); // val
		args.push_back(builder->getInt32Ty()); // qty
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_new_rep_bool", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getDoubleTy()); // val
		args.push_back(builder->getInt32Ty());  // qty
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_new_rep_double", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getInt32Ty());
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getInt32Ty());
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_assign", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getInt32Ty());
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getInt32Ty());
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getInt32Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_assign_fixed", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getInt32Ty());
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt32Ty(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_len", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy()); // addr
		args.push_back(builder->getInt32Ty()); // idx
		args.push_back(builder->getInt32Ty()); // val
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_set_i32", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_set_enum", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy()); // addr
		args.push_back(builder->getInt32Ty()); // idx
		args.push_back(builder->getInt8Ty()); // val
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_set_bool", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getInt32Ty());
		args.push_back(builder->getDoubleTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_set_double", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getInt32Ty());
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_set_str", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy()); // addr
		args.push_back(builder->getInt32Ty()); // idx
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt32Ty(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_get_i32", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_get_enum", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy()); // addr
		args.push_back(builder->getInt32Ty()); // idx
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt1Ty(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_get_bool", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy()); // addr
		args.push_back(builder->getInt32Ty()); // idx
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getDoubleTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_get_double", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy()); // addr
		args.push_back(builder->getInt32Ty()); // idx
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_get_str", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getInt32Ty()); // type
		args.push_back(builder->getPtrTy()); // addr
		args.push_back(builder->getInt32Ty()); // val
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_append_i32", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_append_enum", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getInt32Ty()); // type
		args.push_back(builder->getPtrTy()); // addr
		args.push_back(builder->getInt8Ty()); // val
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_append_bool", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getInt32Ty());
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getDoubleTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_append_double", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getInt32Ty());
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_append_str", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_append_udt", *module);
	}

	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getInt32Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt1Ty(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_contains_i32", *module);
	}

	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getInt32Ty());
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_to_string", *module);
	}

}


#endif // EXTENSIONS_H