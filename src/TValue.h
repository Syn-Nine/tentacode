#ifndef TVALUE_H
#define TVALUE_H

#include "llvm/IR/Value.h"
#include "llvm/IR/Type.h"

#include "Literal.h"

using namespace llvm;

struct TValue
{
	LiteralTypeEnum type;
	Value* value;

	TValue(LiteralTypeEnum t, Value* v) : type(t), value(v)
	{}

	bool IsInteger()     { return type == LITERAL_TYPE_INTEGER; }
	bool IsDouble()  { return type == LITERAL_TYPE_DOUBLE; }
	bool IsBool()    { return type == LITERAL_TYPE_BOOL; }
	bool IsString()  { return type == LITERAL_TYPE_STRING; }

	bool IsIntegerTy()    { return value->getType()->isIntegerTy(); }
	bool IsDoubleTy() { return value->getType()->isDoubleTy();  }
	bool IsPtrTy()    { return value->getType()->isPointerTy(); }

	static TValue NullInvalid()
	{
		return TValue(LITERAL_TYPE_INVALID, nullptr);
	}

	static TValue Integer(Value* v)
	{
		return TValue(LITERAL_TYPE_INTEGER, v);
	}

	static TValue Double(Value* v)
	{
		return TValue(LITERAL_TYPE_DOUBLE, v);
	}

	static TValue Bool(Value* v)
	{
		return TValue(LITERAL_TYPE_BOOL, v);
	}

	static TValue String(Value* v)
	{
		return TValue(LITERAL_TYPE_STRING, v);
	}
};


#endif // TVALUE_H