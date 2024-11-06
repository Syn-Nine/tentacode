#ifndef TVALUE_H
#define TVALUE_H

#include "Literal.h"
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <vector>

struct TValue
{
	LiteralTypeEnum type;
	llvm::Value* value;
	size_t fixed_vec_sz;
	LiteralTypeEnum fixed_vec_type;
	std::string vec_udt_name;
	std::string udt_name;
	llvm::Type* udt_ty;
	std::vector<llvm::Value*> udt_args;
	//void* addr;

	TValue() = delete;
	TValue(LiteralTypeEnum t, llvm::Value* v) : type(t), value(v)
	{}

	bool IsInteger()    { return type == LITERAL_TYPE_INTEGER; }
	bool IsDouble()		{ return type == LITERAL_TYPE_DOUBLE; }
	bool IsBool()		{ return type == LITERAL_TYPE_BOOL; }
	bool IsString()		{ return type == LITERAL_TYPE_STRING; }
	bool IsEnum()		{ return type == LITERAL_TYPE_ENUM; }
	bool IsVec()        { return type == LITERAL_TYPE_VEC; }
	bool IsFixedVec()   { return type == LITERAL_TYPE_VEC_FIXED; }
	bool IsPointer()    { return type == LITERAL_TYPE_POINTER; }
	bool IsUDT()		{ return type == LITERAL_TYPE_UDT; }
	bool IsInvalid()    { return type == LITERAL_TYPE_INVALID; }

	bool IsIntegerTy()  { return value->getType()->isIntegerTy(); }
	bool IsDoubleTy()	{ return value->getType()->isDoubleTy();  }
	bool IsPtrTy()		{ return value->getType()->isPointerTy(); }
	
	//llvm::Type* GetTy() { return value->getType(); }

	static TValue NullInvalid()
	{
		return TValue(LITERAL_TYPE_INVALID, nullptr);
	}

	static TValue Integer(llvm::Value* v)
	{
		return TValue(LITERAL_TYPE_INTEGER, v);
	}

	static TValue Enum(llvm::Value* v)
	{
		return TValue(LITERAL_TYPE_ENUM, v);
	}

	static TValue Double(llvm::Value* v)
	{
		return TValue(LITERAL_TYPE_DOUBLE, v);
	}

	static TValue Bool(llvm::Value* v)
	{
		return TValue(LITERAL_TYPE_BOOL, v);
	}

	static TValue String(llvm::Value* v)
	{
		return TValue(LITERAL_TYPE_STRING, v);
	}

	static TValue Pointer(llvm::Value* v)
	{
		return TValue(LITERAL_TYPE_POINTER, v);
	}

	static TValue FixedVec(llvm::Value* v, LiteralTypeEnum type, size_t sz)
	{
		TValue ret = TValue(LITERAL_TYPE_VEC_FIXED, v);
		ret.fixed_vec_sz = sz;
		ret.fixed_vec_type = type;
		return ret;
	}

	static TValue Vec(llvm::Value* v, LiteralTypeEnum type, std::string vec_udt_name = "")
	{
		TValue ret = TValue(LITERAL_TYPE_VEC, v);
		ret.fixed_vec_type = type;
		ret.vec_udt_name = vec_udt_name;
		return ret;
	}

	static TValue UDT(std::string udt_name, llvm::Type* ty, llvm::Value* v, std::vector<llvm::Value*> args)
	{
		TValue ret = TValue(LITERAL_TYPE_UDT, v);
		ret.udt_ty = ty;
		ret.udt_name = udt_name;
		ret.udt_args = args;
		return ret;
	}
};


#endif // TVALUE_H