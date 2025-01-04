#ifndef EXTENSIONS_H
#define EXTENSIONS_H
#pragma once

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"

#include "Environment.h"
#include "Literal.h"
#include "TVec.h"
#include "TSet.h"
#include "TMap.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <random>
#include <istream>
#include <chrono>
#include <algorithm>
#include <assert.h>
#include <thread>

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

void __parjob(int32_t job_start, int32_t job_end, void (*f)(int32_t*))
{
	//printf("%d-%d\n", job_start, job_end);
	for (int32_t i = job_start; i <= job_end; ++i)
	{
		f(&i);
	}
}

extern "C" DLLEXPORT void __parfor(int32_t n_threads, int32_t n_jobs, void (*f)(int32_t*))
{
	if (0 == n_threads || 0 >= n_jobs) return;

	if (n_jobs < n_threads) n_threads = n_jobs;
	int n_cores = std::thread::hardware_concurrency();
	if (n_threads < 0) n_threads = std::max(1, n_cores - 1);
	if (n_threads > n_cores - 1) n_threads = std::max(1, n_cores - 1);

	int rem = n_jobs % n_threads;
	int stride = (n_jobs - rem) / n_threads;
	int j0 = 0;
	int j1 = stride - 1;

	std::vector<std::thread> threads;
	for (int32_t i = 0; i < n_threads; ++i)
	{
		//printf("%d\n", i);
		threads.push_back(std::thread(__parjob, j0, j1, f));
		j0 = j1 + 1;
		j1 = j0 + stride - 1;
		if (i == n_threads - 2) j1 = n_jobs - 1;
	}
	
	for (int32_t i = 0; i < threads.size(); ++i)
	{
		threads[i].join();
	}
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

extern "C" DLLEXPORT std::string* input()
{
	char p[256];
	memset(p, 0, 256);
	std::cin.getline(p, 255);
	return new std::string(p);
}


// strings
//-----------------------------------------------------------------------------
extern "C" DLLEXPORT void __del_string(std::string* str)
{
	if (!str) return; // pointer never set, nothing to delete

	if (-1 == reinterpret_cast<int64_t>(str))
	{
		//printf("Attempted to double-delete a string pointer.\n");
		return;
	}
	
	//printf("deleting: %s: %llu\n", str->c_str(), str);
	delete str;
}

extern "C" DLLEXPORT std::string* __new_string_from_literal(char* str)
{
	return new std::string(str);
}

extern "C" DLLEXPORT std::string* __new_string_from_string(std::string* str)
{
	return new std::string(*str);
}

extern "C" DLLEXPORT std::string* __new_string_void()
{
	//return new std::string("void_string");
	return new std::string("");
}

extern "C" DLLEXPORT std::string* __string_cat(std::string* a, std::string* b)
{
	return new std::string(*a + *b);
}

extern "C" DLLEXPORT bool __str_cmp(std::string* a, std::string* b)
{
	return 0 == a->compare(*b);
}

extern "C" DLLEXPORT void __str_assign(std::string* a, std::string* b)
{
	*a = *b; // clone
}

extern "C" DLLEXPORT std::string* __bool_to_string(bool val)
{
	if (val) return new std::string("true");
	return new std::string("false");
}


extern "C" DLLEXPORT std::string* __float_to_string(double val)
{
	return new std::string(std::to_string(val));
}

extern "C" DLLEXPORT std::string* __int_to_string(int64_t val)
{
	return new std::string(std::to_string(val));
}

extern "C" DLLEXPORT int64_t __str_to_int(std::string* a)
{
	return std::stol(*a);
}

extern "C" DLLEXPORT double __str_to_double(std::string* a)
{
	return std::stod(*a);
}

extern "C" DLLEXPORT int64_t __str_len(std::string* a)
{
	return a->length();
}


//-----------------------------------------------------------------------------

extern "C" DLLEXPORT int64_t __mod_impl(int64_t a, int64_t b)
{
	return a % b;
}


extern "C" DLLEXPORT double __clock_impl()
{
	const auto p = std::chrono::system_clock::now();
	return double(std::chrono::duration_cast<std::chrono::microseconds>(p.time_since_epoch()).count() / 1000000.0);
}

extern "C" DLLEXPORT int64_t __pow_impl(int64_t base, int64_t exponent)
{
	return pow(base, exponent);
}

extern "C" DLLEXPORT double __powf_impl(double base, double exponent)
{
	return powf(base, exponent);
}

extern "C" DLLEXPORT int64_t __max_impl(int64_t lhs, int64_t rhs)
{
	if (lhs > rhs) return lhs;
	return rhs;
}

extern "C" DLLEXPORT double __maxf_impl(double lhs, double rhs)
{
	if (lhs > rhs) return lhs;
	return rhs;
}

extern "C" DLLEXPORT int64_t __min_impl(int64_t lhs, int64_t rhs)
{
	if (lhs < rhs) return lhs;
	return rhs;
}

extern "C" DLLEXPORT double __minf_impl(double lhs, double rhs)
{
	if (lhs < rhs) return lhs;
	return rhs;
}

extern "C" DLLEXPORT double __sgn(double val)
{
	if (val < 0) return -1.0;
	return 1.0;
}

extern "C" DLLEXPORT double fract(double val)
{
	return val - floor(val);
}

extern "C" DLLEXPORT double __rand_impl()
{
	// todo - go back and profile
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_real_distribution<> dist(0.0, 1.0);
	return dist(gen);
}

extern "C" DLLEXPORT int64_t __rand_range_impl(int64_t lhs, int64_t rhs)
{
	// todo - go back and profile
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<> dist(lhs, rhs-1);
	return dist(gen);
}



// map
//-----------------------------------------------------------------------------

extern "C" DLLEXPORT void* __new_map(int64_t vtype, int32_t bits, int64_t span)
{
	LiteralTypeEnum vt = LiteralTypeEnum(vtype);
	return new TMap(vt, bits, span);
}

extern "C" DLLEXPORT void __map_insert_int_key(TMap* ptr, int64_t key, void* data)
{
	ptr->InsertAtInt(key, data);
}

extern "C" DLLEXPORT void __map_insert_string_key(TMap* ptr, std::string* key, void* data)
{
	ptr->InsertAtString(*key, data);
}

extern "C" DLLEXPORT int64_t __map_len(TMap* ptr)
{
	return ptr->Size();
}

extern "C" DLLEXPORT void __map_start_iter(TMap* ptr)
{
	return ptr->StartIteration();
}

extern "C" DLLEXPORT int16_t __get_map_iter_key_i16(TMap* ptr)
{
	return ptr->GetIterKey_i16();
}

extern "C" DLLEXPORT int32_t __get_map_iter_key_i32(TMap* ptr)
{
	return ptr->GetIterKey_i32();
}

extern "C" DLLEXPORT int64_t __get_map_iter_key_i64(TMap* ptr)
{
	return ptr->GetIterKey_i64();
}

extern "C" DLLEXPORT std::string* __get_map_iter_key_str(TMap* ptr)
{
	return new std::string(ptr->GetIterKey_str());
}

extern "C" DLLEXPORT void __get_map_iter_val(TMap* ptr, void* dst)
{
	void* src = ptr->GetIterValue();
	int sz_bytes = ptr->Span();
	memcpy(dst, src, sz_bytes);
}

extern "C" DLLEXPORT void __get_map_val_at_i64(TMap* ptr, int64_t idx, void* dst)
{
	ptr->GetAtInt(idx, dst);
}

extern "C" DLLEXPORT void __set_map_val_at_i64(TMap* ptr, int64_t idx, void* dst)
{
	ptr->SetAtInt(idx, dst);
}

extern "C" DLLEXPORT void __get_map_val_at_str(TMap* ptr, std::string* idx, void* dst)
{
	ptr->GetAtStr(*idx, dst);
}

extern "C" DLLEXPORT void __set_map_val_at_str(TMap* ptr, std::string* idx, void* dst)
{
	ptr->SetAtStr(*idx, dst);
}

// set
//-----------------------------------------------------------------------------

extern "C" DLLEXPORT void* __new_set(int64_t vtype, int32_t bits)
{
	LiteralTypeEnum vt = LiteralTypeEnum(vtype);
	return new TSet(vt, bits);
}

extern "C" DLLEXPORT void __set_insert_int(TSet* ptr, int64_t val)
{
	ptr->Insert_Int(val);
}

extern "C" DLLEXPORT void __set_insert_string(TSet* ptr, std::string* str)
{
	ptr->Insert_String(*str);
}

extern "C" DLLEXPORT int64_t __set_len(TSet* ptr)
{
	return ptr->Size();
}

extern "C" DLLEXPORT void __set_start_iter(TSet* ptr)
{
	return ptr->StartIteration();
}

extern "C" DLLEXPORT int16_t __get_set_iter_val_i16(TSet* ptr)
{
	return ptr->GetIterValue_i16();
}

extern "C" DLLEXPORT int32_t __get_set_iter_val_i32(TSet* ptr)
{
	return ptr->GetIterValue_i32();
}

extern "C" DLLEXPORT int64_t __get_set_iter_val_i64(TSet* ptr)
{
	return ptr->GetIterValue_i16();
}

extern "C" DLLEXPORT std::string* __get_set_iter_val_str(TSet* ptr)
{
	return new std::string(ptr->GetIterValue_str());
}

extern "C" DLLEXPORT void __set_assign(TSet* a, TSet* b)
{
	*a = *b; // clone
}


// fixed vec
//-----------------------------------------------------------------------------

extern "C" DLLEXPORT void* __fixed_vec_to_string(int32_t vecType, int32_t srcBits, void* srcPtr, int64_t sz)
{
	LiteralTypeEnum vt = LiteralTypeEnum(vecType);

	std::string array;
	std::string vtype = "invalid";

	if (LITERAL_TYPE_INTEGER == vt)
	{
		if (16 == srcBits)
		{
			vtype = "i16";
			int16_t* src = static_cast<int16_t*>(srcPtr);
			for (size_t i = 0; i < sz; ++i)
			{
				if (0 == i)
				{
					array.append(std::to_string(src[i]));
				}
				else
				{
					array.append(", " + std::to_string(src[i]));
				}
			}
		}
		else if (32 == srcBits)
		{
			vtype = "i32";
			int32_t* src = static_cast<int32_t*>(srcPtr);
			for (size_t i = 0; i < sz; ++i)
			{
				if (0 == i)
				{
					array.append(std::to_string(src[i]));
				}
				else
				{
					array.append(", " + std::to_string(src[i]));
				}
			}
		}
		else if (64 == srcBits)
		{
			vtype = "i64";
			int64_t* src = static_cast<int64_t*>(srcPtr);
			for (size_t i = 0; i < sz; ++i)
			{
				if (0 == i)
				{
					array.append(std::to_string(src[i]));
				}
				else
				{
					array.append(", " + std::to_string(src[i]));
				}
			}
		}
	}
	else if (LITERAL_TYPE_ENUM == vt)
	{
		vtype = "enum";
		int32_t* src = static_cast<int32_t*>(srcPtr);
		for (size_t i = 0; i < sz; ++i)
		{
			if (0 == i)
			{
				array.append(std::to_string(src[i]));
			}
			else
			{
				array.append(", " + std::to_string(src[i]));
			}
		}
	}
	else if (LITERAL_TYPE_BOOL == vt)
	{
		vtype = "bool";
		bool* src = static_cast<bool*>(srcPtr);
		for (size_t i = 0; i < sz; ++i)
		{
			bool b = src[i];
			if (0 == i)
			{
				if (b)
					array.append("true");
				else
					array.append("false");
			}
			else
			{
				if (b)
					array.append(", true");
				else
					array.append(", false");
			}
		}
	}
	else if (LITERAL_TYPE_FLOAT == vt)
	{
		if (32 == srcBits)
		{
			vtype = "f32";
			float* src = static_cast<float*>(srcPtr);
			for (size_t i = 0; i < sz; ++i)
			{
				if (0 == i)
				{
					array.append(std::to_string(src[i]));
				}
				else
				{
					array.append(", " + std::to_string(src[i]));
				}
			}
		}
		else if (64 == srcBits)
		{
			vtype = "f64";
			double* src = static_cast<double*>(srcPtr);
			for (size_t i = 0; i < sz; ++i)
			{
				if (0 == i)
				{
					array.append(std::to_string(src[i]));
				}
				else
				{
					array.append(", " + std::to_string(src[i]));
				}
			}
		}
	}
	else if (LITERAL_TYPE_STRING == vt)
	{
		vtype = "string";
		std::string** src = static_cast<std::string**>(srcPtr);
		for (size_t i = 0; i < sz; ++i)
		{
			if (0 == i)
			{
				array.append(*(src[i]));
			}
			else
			{
				array.append(", " + *(src[i]));
			}
		}
	}
	else if (LITERAL_TYPE_UDT == vt)
	{
		vtype = "UDT";
	}


	std::string ret = "<FixedVec," + vtype + ",Size:" + std::to_string(sz) + ">[" + array + "]";

	return new std::string(ret);
}


// dynamic vec
//-----------------------------------------------------------------------------

extern "C" DLLEXPORT void* __new_dyn_vec(int64_t vtype, int64_t span)
{
	LiteralTypeEnum vt = LiteralTypeEnum(vtype);
	return new TVec(vt, span);
}

extern "C" DLLEXPORT int64_t __dyn_vec_len(TVec* vec)
{
	return vec->Size();
}

extern "C" DLLEXPORT void __dyn_vec_append_i16(TVec* vec, int16_t val)
{
	int16_t* p = static_cast<int16_t*>(vec->PushBackPtr());
	*p = val;
}

extern "C" DLLEXPORT void __dyn_vec_append_i32(TVec* vec, int32_t val)
{
	int32_t* p = static_cast<int32_t*>(vec->PushBackPtr());
	*p = val;
}

extern "C" DLLEXPORT void __dyn_vec_append_i64(TVec* vec, int64_t val)
{
	int64_t* p = static_cast<int64_t*>(vec->PushBackPtr());
	*p = val;
}

extern "C" DLLEXPORT void __dyn_vec_append_f32(TVec* vec, float val)
{
	float* p = static_cast<float*>(vec->PushBackPtr());
	*p = val;
}

extern "C" DLLEXPORT void __dyn_vec_append_f64(TVec* vec, double val)
{
	double* p = static_cast<double*>(vec->PushBackPtr());
	*p = val;
}

extern "C" DLLEXPORT void __dyn_vec_append_enum(TVec* vec, int32_t val)
{
	int32_t* p = static_cast<int32_t*>(vec->PushBackPtr());
	*p = val;
}

extern "C" DLLEXPORT void __dyn_vec_append_bool(TVec* vec, bool val)
{
	bool* p = static_cast<bool*>(vec->PushBackPtr());
	*p = val;
}

extern "C" DLLEXPORT void __dyn_vec_append_string(TVec* vec, std::string* val)
{
	std::string** p = static_cast<std::string**>(vec->PushBackPtr());
	*p = new std::string(*val);
}

extern "C" DLLEXPORT void __dyn_vec_assert_idx(TVec* vec, int64_t idx)
{
	int64_t sz = vec->Size();
	if (idx >= vec->Size())
	{
		printf("Error! Vector index %d out of bounds %d.\n", idx, sz);
		assert(true);
		exit(0);
	}
}

extern "C" DLLEXPORT void __dyn_vec_clear_presize(TVec* dstVec, int64_t srcLen)
{
	dstVec->Clear();
	dstVec->Presize(srcLen);
}

extern "C" DLLEXPORT void* __dyn_vec_data_ptr(TVec* vec)
{
	return vec->Data();
}

extern "C" DLLEXPORT void __dyn_vec_delete(TVec* dstPtr)
{
	if (!dstPtr)
		printf("Attempted to delete null vec pointer.\n");
	else
		delete dstPtr;
}

extern "C" DLLEXPORT void __dyn_vec_replace(TVec* dstVec, TVec* srcVec)
{
	dstVec->Replace(srcVec);
}

extern "C" DLLEXPORT void* __dyn_vec_to_string(TVec* srcPtr)
{
	std::string array;
	std::string vtype = "invalid";

	LiteralTypeEnum vt = srcPtr->VecType();

	int sz = srcPtr->Size();
	int bits = srcPtr->Span() * 8;

	if (LITERAL_TYPE_INTEGER == vt)
	{
		if (16 == bits)
		{
			vtype = "i16";
			int16_t* src = nullptr;
			for (size_t i = 0; i < sz; ++i)
			{
				src = static_cast<int16_t*>(srcPtr->At(i));
				if (src)
				{
					if (0 == i)
					{
						array.append(std::to_string(*src));
					}
					else
					{
						array.append(", " + std::to_string(*src));
					}
				}
			}
		}
		else if (32 == bits)
		{
			vtype = "i32";
			int32_t* src = nullptr;
			for (size_t i = 0; i < sz; ++i)
			{
				src = static_cast<int32_t*>(srcPtr->At(i));
				if (src)
				{
					if (0 == i)
					{
						array.append(std::to_string(*src));
					}
					else
					{
						array.append(", " + std::to_string(*src));
					}
				}
			}
		}
		else if (64 == bits)
		{
			vtype = "i64";
			int64_t* src = nullptr;
			for (size_t i = 0; i < sz; ++i)
			{
				src = static_cast<int64_t*>(srcPtr->At(i));
				if (src)
				{
					if (0 == i)
					{
						array.append(std::to_string(*src));
					}
					else
					{
						array.append(", " + std::to_string(*src));
					}
				}
			}
		}
	}
	else if (LITERAL_TYPE_FLOAT == vt)
	{
		if (32 == bits)
		{
			vtype = "f32";
			float* src = nullptr;
			for (size_t i = 0; i < sz; ++i)
			{
				src = static_cast<float*>(srcPtr->At(i));
				if (src)
				{
					if (0 == i)
					{
						array.append(std::to_string(*src));
					}
					else
					{
						array.append(", " + std::to_string(*src));
					}
				}
			}
		}
		else if (64 == bits)
		{
			vtype = "f64";
			double* src = nullptr;
			for (size_t i = 0; i < sz; ++i)
			{
				src = static_cast<double*>(srcPtr->At(i));
				if (src)
				{
					if (0 == i)
					{
						array.append(std::to_string(*src));
					}
					else
					{
						array.append(", " + std::to_string(*src));
					}
				}
			}
		}
	}
	else if (LITERAL_TYPE_ENUM == vt)
	{
		vtype = "enum";
		int32_t* src = nullptr;
		for (size_t i = 0; i < sz; ++i)
		{
			src = static_cast<int32_t*>(srcPtr->At(i));
			if (src)
			{
				if (0 == i)
				{
					array.append(std::to_string(*src));
				}
				else
				{
					array.append(", " + std::to_string(*src));
				}
			}
		}
	}
	else if (LITERAL_TYPE_BOOL == vt)
	{
		vtype = "bool";
		bool* src = nullptr;
		for (size_t i = 0; i < sz; ++i)
		{
			src = static_cast<bool*>(srcPtr->At(i));
			bool b = *src;
			if (0 == i)
			{
				if (b)
					array.append("true");
				else
					array.append("false");
			}
			else
			{
				if (b)
					array.append(", true");
				else
					array.append(", false");
			}
		}
	}
	else if (LITERAL_TYPE_STRING == vt)
	{
		vtype = "string";
		std::string* src = nullptr;
		for (size_t i = 0; i < sz; ++i)
		{
			src = *(static_cast<std::string**>(srcPtr->At(i)));
			if (0 == i)
			{
				array.append(*src);
			}
			else
			{
				array.append(", " + *src);
			}
		}
	}
	else if (LITERAL_TYPE_UDT == vt)
	{
		vtype = "UDT,Span:" + std::to_string(srcPtr->Span());
	}

	std::string ret = "<DynVec," + vtype + ",Size:" + std::to_string(sz) + ">[" + array + "]";

	return new std::string(ret);

}

extern "C" DLLEXPORT void* __vec_new_rep_i32(int32_t dstVal, int32_t dstQty)
{
	TVec* ret = new TVec(LITERAL_TYPE_INTEGER, 4);
	ret->Fill_i32(dstVal, dstQty);
	return ret;
}

extern "C" DLLEXPORT void* __vec_new_rep_enum(int32_t dstVal, int32_t dstQty)
{
	TVec* ret = new TVec(LITERAL_TYPE_ENUM, 4);
	ret->Fill_i32(dstVal, dstQty);
	return ret;
}

extern "C" DLLEXPORT void* __vec_new_rep_bool(bool dstVal, int32_t dstQty)
{
	TVec* ret = new TVec(LITERAL_TYPE_BOOL, 1);
	ret->Fill_bool(dstVal, dstQty);
	return ret;
}

extern "C" DLLEXPORT void* __vec_new_rep_float(float dstVal, int32_t dstQty)
{
	TVec* ret = new TVec(LITERAL_TYPE_FLOAT, 4);
	ret->Fill_f32(dstVal, dstQty);
	return ret;
}

extern "C" DLLEXPORT bool __dyn_vec_contains_i16(TVec* srcPtr, int16_t val)
{
	return srcPtr->Contains_i16(val);
}

extern "C" DLLEXPORT bool __dyn_vec_contains_i32(TVec* srcPtr, int32_t val)
{
	return srcPtr->Contains_i32(val);
}

extern "C" DLLEXPORT bool __dyn_vec_contains_i64(TVec* srcPtr, int64_t val)
{
	return srcPtr->Contains_i64(val);
}






// file
//-----------------------------------------------------------------------------

extern "C" DLLEXPORT void* __file_readlines(std::string* filename)
{
	std::ifstream file(filename->c_str());

	TVec* ret = new TVec(LITERAL_TYPE_STRING, 8);

	if (file.is_open())
	{
		for (std::string line; std::getline(file, line); )
		{
			__dyn_vec_append_string(ret, new std::string(line));
		}
	}

	return static_cast<void*>(ret);
}

extern "C" DLLEXPORT void* __file_readstring(std::string* filename)
{
	std::ifstream file(filename->c_str());

	std::string ret;
	
	if (file.is_open())
	{
		std::ostringstream sstr;
		sstr << file.rdbuf();
		ret = sstr.str();
	}

	return new std::string(ret);
}


extern "C" DLLEXPORT void __file_writelines_dyn_vec(std::string* filename, TVec* srcPtr)
{
	std::ofstream file(filename->c_str());

	if (file.is_open())
	{
		std::string* src = nullptr;
		for (size_t i = 0; i < srcPtr->Size(); ++i)
		{
			src = *(static_cast<std::string**>(srcPtr->At(i)));
			file << *src << std::endl;
		}
		file.close();
	}
}

extern "C" DLLEXPORT void __file_writelines_fixed_vec(std::string* filename, std::string** srcPtr, int64_t len)
{
	std::ofstream file(filename->c_str());

	if (file.is_open())
	{
		std::string* src = nullptr;
		for (size_t i = 0; i < len; ++i)
		{
			src = srcPtr[i];
			file << *src << std::endl;
		}
		file.close();
	}
}

extern "C" DLLEXPORT void __file_writestring(std::string* filename, std::string* output)
{
	std::ofstream file(filename->c_str());

	if (file.is_open())
	{
		file << *output;
		file.close();
	}
}


//-----------------------------------------------------------------------------
// string

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
	TVec* ret = new TVec(LITERAL_TYPE_STRING, 8);

	llvm::SmallVector<std::string> ss = StrSplit(*src, *del);

	for (auto& s : ss)
	{
		__dyn_vec_append_string(ret, new std::string(s));
	}

	return static_cast<void*>(ret);
}

extern "C" DLLEXPORT std::string* __str_join_fixed_vec(std::string** srcPtr, std::string* del, int64_t len)
{
	llvm::SmallVector<std::string> tmp;
	
	std::string* src = nullptr;
	for (size_t i = 0; i < len; ++i)
	{
		src = srcPtr[i];
		tmp.push_back(*src);
	}
	
	return new std::string(StrJoin(tmp, *del));
}

extern "C" DLLEXPORT std::string* __str_join_dyn_vec(TVec* srcPtr, std::string* del)
{
	llvm::SmallVector<std::string> tmp;
	
	size_t sz = srcPtr->Size();
	std::string* src = nullptr;
	for (size_t i = 0; i < sz; ++i)
	{
		src = *(static_cast<std::string**>(srcPtr->At(i)));
		tmp.push_back(*src);
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
/*
extern "C" DLLEXPORT void __vec_append_udt(int dstType, void* dstPtr, void* val)
{
	if (LITERAL_TYPE_UDT == dstType)
	{
		llvm::SmallVector<void*>* dst = static_cast<llvm::SmallVector<void*>*>(dstPtr);
		dst->push_back(val);
	}
}

*/

//-----------------------------------------------------------------------------
// math
extern "C" DLLEXPORT double mix(double a, double b, double ratio) {
	// lerp
	return a + (b - a) * ratio;
}

// from wikipedia
extern "C" DLLEXPORT double clamp(double x, double lowerlimit = 0.0f, double upperlimit = 1.0f) {
	if (x < lowerlimit) return lowerlimit;
	if (x > upperlimit) return upperlimit;
	return x;
}

extern "C" DLLEXPORT double smoothstep(double a, double b, double ratio) {
	// Scale, and clamp x to 0..1 range
	ratio = clamp((ratio - a) / (b - a));
	return ratio * ratio * (3.0f - 2.0f * ratio);
}

extern "C" DLLEXPORT double smootherstep(double a, double b, double ratio) {
	// Scale, and clamp x to 0..1 range
	ratio = clamp((ratio - a) / (b - a));
	return ratio * ratio * ratio * (ratio * (6.0f * ratio - 15.0f) + 10.0f);
}



static void LoadExtensions(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	{	// void = ftn(ptr)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), { builder->getPtrTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "println", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "print", *module);
		//
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__del_string", *module);
	}
	{ // void = f(i32, ptr)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getInt32Ty());
		args.push_back(builder->getInt32Ty());
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__parfor", *module);
	}
	{	// ptr = ftn(ptr, ptr)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), { builder->getPtrTy(), builder->getPtrTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__string_cat", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_split", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_join_dyn_vec", *module);
	}

	{	// ptr = ftn(ptr, ptr, i64)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), { builder->getPtrTy(), builder->getPtrTy(), builder->getInt64Ty() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_join_fixed_vec", *module);
	}

	{	// ptr = ftn(void)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), { }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__new_string_void", *module);
	}

	{	// ptr = ftn(ptr)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), { builder->getPtrTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__new_string_from_literal", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__new_string_from_string", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__file_readlines", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__file_readstring", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_toupper", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_tolower", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_ltrim", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_rtrim", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_trim", *module);
	}

	{	// bool = ftn(ptr, ptr)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt1Ty(), { builder->getPtrTy(), builder->getPtrTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_cmp", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_contains", *module);
	}

	{	// void = ftn(ptr, ptr)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), { builder->getPtrTy(), builder->getPtrTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_assign", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__file_writelines_dyn_vec", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__file_writestring", *module);
	}
	{	// void = ftn(ptr, ptr, i64)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), { builder->getPtrTy(), builder->getPtrTy(), builder->getInt64Ty() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__file_writelines_fixed_vec", *module);
	}

	{	// ptr = ftn(int)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), { builder->getInt64Ty() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__int_to_string", *module);
	}

	{	// ptr = ftn(double)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), { builder->getDoubleTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__float_to_string", *module);
	}

	{	// ptr = ftn(bool)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), { builder->getInt1Ty() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__bool_to_string", *module);
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
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "ceil", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "sqrt", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "fract", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "round", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__sgn", *module);

	}

	{	// int = ftn(int)
		std::vector<llvm::Type*> args(1, builder->getInt64Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt64Ty(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "abs", *module);
	}

	{
		std::vector<llvm::Type*> args(2, builder->getInt64Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt64Ty(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__mod_impl", *module);
	}

	{	// double = ftn(double, double, double)
		std::vector<llvm::Type*> args(3, builder->getDoubleTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getDoubleTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "mix", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "smoothstep", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "smootherstep", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "clamp", *module);
	}
	{	// double = ftn(double, double)
		std::vector<llvm::Type*> args(2, builder->getDoubleTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getDoubleTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__powf_impl", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__minf_impl", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__maxf_impl", *module);
	}
	{	// i64 = ftn(i64, i64)
		std::vector<llvm::Type*> args(2, builder->getInt64Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt64Ty(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__pow_impl", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__min_impl", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__max_impl", *module);
	}

	{	// double = ftn()
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getDoubleTy(), {}, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__clock_impl", *module);
	}

	{
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getDoubleTy(), {}, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__rand_impl", *module);
	}

	{
		std::vector<llvm::Type*> args(2, builder->getInt64Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt64Ty(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__rand_range_impl", *module);
	}

	{	// int = ftn(ptr)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt64Ty(), { builder->getPtrTy(), builder->getPtrTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_to_int", *module);
	}

	{	// double = ftn(ptr)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getDoubleTy(), { builder->getPtrTy(), builder->getPtrTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_to_double", *module);
	}

	{	// ptr = ftn(ptr, ptr, ptr)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), { builder->getPtrTy(), builder->getPtrTy(), builder->getPtrTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_replace", *module);
	}

	{	// ptr = ftn(ptr, int, int)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), { builder->getPtrTy(), builder->getInt32Ty(), builder->getInt32Ty() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_substr", *module);
	}

	{	// double = ftn(double, double)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getDoubleTy(), { builder->getDoubleTy(), builder->getDoubleTy() }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "atan2", *module);
	}
	
	{	// ptr = ftn(void)
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), { }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "input", *module);
	}

	
	// vectors
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getInt16Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt1Ty(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__dyn_vec_contains_i16", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getInt32Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt1Ty(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__dyn_vec_contains_i32", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getInt64Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt1Ty(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__dyn_vec_contains_i64", *module);
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
		args.push_back(builder->getFloatTy()); // val
		args.push_back(builder->getInt32Ty());  // qty
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__vec_new_rep_float", *module);
	}

	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getInt32Ty());
		args.push_back(builder->getInt32Ty());
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getInt64Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__fixed_vec_to_string", *module);
	}

	{ // ptr = f(i64, i64)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getInt64Ty());
		args.push_back(builder->getInt64Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__new_dyn_vec", *module);
	}

	{ // void = f(ptr)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__dyn_vec_delete", *module);
	}

	{ // i64 = f(ptr)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt64Ty(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__dyn_vec_len", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__set_len", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__map_len", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__str_len", *module);
	}

	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getInt64Ty());
		args.push_back(builder->getInt64Ty());
		args.push_back(builder->getInt64Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__dyn_vec_replace_fixed", *module);
	}

	{ // void = f(ptr, i64)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getInt64Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__dyn_vec_clear_presize", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__dyn_vec_assert_idx", *module);
	}

	{ // ptr = f(ptr)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__dyn_vec_to_string", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__dyn_vec_data_ptr", *module);
	}

	{ // void = f(ptr, i64)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__dyn_vec_replace", *module);
	}

	////////////// vector appends

	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getInt16Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__dyn_vec_append_i16", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getInt32Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__dyn_vec_append_i32", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getInt64Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__dyn_vec_append_i64", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getFloatTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__dyn_vec_append_f32", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getDoubleTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__dyn_vec_append_f64", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getInt32Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__dyn_vec_append_enum", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getInt1Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__dyn_vec_append_bool", *module);
	}
	{
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__dyn_vec_append_string", *module);
	}

	// Sets
	//-------------------------------------------------------------------------

	{ // ptr = f(i64)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getInt64Ty());
		args.push_back(builder->getInt32Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__new_set", *module);
	}
	{ // void = f(ptr, ptr)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__set_insert_string", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__set_assign", *module);
	}
	{ // void = f(ptr, i64)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getInt64Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__set_insert_int", *module);
	}
	{ // void = f()
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), { }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__set_start_iter", *module);
	}
	{ // i16 = f(ptr)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt16Ty(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__get_set_iter_val_i16", *module);
	}
	{ // i32 = f(ptr)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt32Ty(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__get_set_iter_val_i32", *module);
	}
	{ // i64 = f(ptr)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt64Ty(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__get_set_iter_val_i64", *module);
	}
	{ // ptr = f(ptr)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__get_set_iter_val_str", *module);
	}


	// Maps
	//-------------------------------------------------------------------------

	{ // ptr = f(i64, i64)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getInt64Ty());
		args.push_back(builder->getInt32Ty());
		args.push_back(builder->getInt64Ty());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__new_map", *module);
	}
	{ // void = f(ptr, i64, ptr, i64)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getInt64Ty());
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__map_insert_int_key", *module);
	}
	{ // void = f(ptr, ptr, ptr, i64)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__map_insert_string_key", *module);
	}
	{ // void = f()
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), { }, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__map_start_iter", *module);
	}
	{ // i16 = f(ptr)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt16Ty(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__get_map_iter_key_i16", *module);
	}
	{ // i32 = f(ptr)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt32Ty(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__get_map_iter_key_i32", *module);
	}
	{ // i64 = f(ptr)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getInt64Ty(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__get_map_iter_key_i64", *module);
	}
	{ // ptr = f(ptr)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getPtrTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__get_map_iter_key_str", *module);
	}
	{ // void = f(ptr, ptr)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__get_map_iter_val", *module);
	}
	{ // ptr = f(ptr, i64)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getInt64Ty());
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__get_map_val_at_i64", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__set_map_val_at_i64", *module);
	}
	{ // ptr = f(ptr, ptr)
		std::vector<llvm::Type*> args;
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getPtrTy());
		args.push_back(builder->getPtrTy());
		llvm::FunctionType* FT = llvm::FunctionType::get(builder->getVoidTy(), args, false);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__get_map_val_at_str", *module);
		llvm::Function::Create(FT, llvm::Function::InternalLinkage, "__set_map_val_at_str", *module);
	}



	
	
}


#endif // EXTENSIONS_H