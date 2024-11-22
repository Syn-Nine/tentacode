#ifndef TVALUE_H
#define TVALUE_H

#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"

#include <vector>

#include "Literal.h"
#include "Token.h"
#include "ErrorHandler.h"
#include "Utility.h"

class TValue
{
public:

	typedef std::vector<TValue> TValueList;

	TValue()
	{
		m_type = LITERAL_TYPE_INVALID;
		m_value = nullptr;
		m_ty = nullptr;
		m_bits = 0;
		m_is_storage = false;
	}

	static void RegisterErrorHandler(ErrorHandler* eh) { m_errorHandler = eh; }
	static void RegisterLLVM(llvm::IRBuilder<>* builder, llvm::Module* module)
	{
		m_builder = builder;
		m_module = module;
	}

	static TValue Construct(Token* token, TokenPtrList* args, std::string lexeme, bool global, TValueList* targs = nullptr);
	static TValue Construct_Null(Token* token, LiteralTypeEnum type, int bits);
	static TValue Construct_ReturnValue(Token* token, LiteralTypeEnum type, int bits, llvm::Value* a);
	

	size_t FixedVecLen() const { return m_fixed_vec_len; }
	static TValue FromLiteral(Token* token, const Literal& literal);

	static TValue NullInvalid()
	{
		return TValue(LITERAL_TYPE_INVALID, nullptr);
	}

	// type checking
	bool IsInteger()    { return m_type == LITERAL_TYPE_INTEGER; }
	bool IsFloat()      { return m_type == LITERAL_TYPE_FLOAT; }
	bool IsBool()       { return m_type == LITERAL_TYPE_BOOL; }
	bool IsString()     { return m_type == LITERAL_TYPE_STRING; }
	bool IsEnum()       { return m_type == LITERAL_TYPE_ENUM; }
	bool isNumeric()    { return m_type == LITERAL_TYPE_FLOAT || m_type == LITERAL_TYPE_INTEGER; }
	//bool IsPointer()    { return m_type == LITERAL_TYPE_POINTER; }
	//bool IsUDT()        { return m_type == LITERAL_TYPE_UDT; }

	bool IsVecAny()     { return m_type == LITERAL_TYPE_VEC_DYNAMIC || m_type == LITERAL_TYPE_VEC_FIXED; }
	bool IsVecDynamic() { return m_type == LITERAL_TYPE_VEC_DYNAMIC; }
	bool IsVecFixed()   { return m_type == LITERAL_TYPE_VEC_FIXED; }

	TValue As(TokenTypeEnum newType);

	// binary operations
	TValue Add(TValue rhs);
	TValue Divide(TValue rhs);
	TValue Modulus(TValue rhs);
	TValue Multiply(TValue rhs);
	TValue Subtract(TValue rhs);
	//
	TValue IsGreaterThan(TValue rhs, bool or_equal);
	TValue IsLessThan(TValue rhs, bool or_equal);
	TValue IsEqual(TValue rhs);
	TValue IsNotEqual(TValue rhs);
	
	TValue GetFromStorage();
	Token* GetToken() { return m_token; }
	llvm::Type* GetTy() { return m_ty; }
	LiteralTypeEnum GetVecType() { return m_vec_type; }

	bool IsInvalid() { return m_type == LITERAL_TYPE_INVALID; }

	llvm::Value* Value() { return m_value; }

	TValue CastToFloat(int bits);
	TValue CastToInt(int bits);
	TValue CastToMatchImplicit(TValue src);

	void Cleanup();

	TValue GetAtVectorIndex(TValue idx);

	void Store(TValue rhs);
	void StoreAtIndex(TValue idx, TValue rhs);

	static TValue MakeBool(Token* token, llvm::Value* value);
	static TValue MakeInt64(Token* token, llvm::Value* value);
	static TValue MakeFloat64(Token* token, llvm::Value* value);
	static TValue MakeString(Token* token, llvm::Value* value);
	static TValue MakeDynVec(Token* token, llvm::Value* value, LiteralTypeEnum vectype, int bits);

	TValue Negate();
	TValue Not();

	void EmitAppend(TValue rhs);
	TValue EmitLen();

	int NumBits() const { return m_bits; }

	void SetValue(llvm::Value* val) { m_value = val; }
	void SetStorage(bool val) { m_is_storage = val; }
	
	/*
	static TValue UDT(std::string udt_name, llvm::Type* ty, llvm::Value* v, std::vector<llvm::Value*> args)
	{
		TValue ret = TValue(LITERAL_TYPE_UDT, v);
		ret.udt_ty = ty;
		ret.udt_name = udt_name;
		ret.udt_args = args;
		return ret;
	}

	std::string vec_udt_name;
	std::string udt_name;
	llvm::Type* udt_ty;
	std::vector<llvm::Value*> udt_args;
	*/

private:

	TValue(LiteralTypeEnum t, llvm::Value* v) : m_type(t), m_value(v)
	{
		if (v) m_ty = v->getType();
		m_token = nullptr;
		m_bits = 0;
		m_fixed_vec_len = 0;
		m_is_storage = false;
		m_vec_type = LITERAL_TYPE_INVALID;
	}
	
	TValue(Token* tk, LiteralTypeEnum t, llvm::Value* v) : m_token(tk), m_type(t), m_value(v)
	{
		if (v) m_ty = v->getType();
		m_bits = 0;
		m_fixed_vec_len = 0;
		m_is_storage = false;
		m_vec_type = LITERAL_TYPE_INVALID;
	}

	static void Error(Token* token, const std::string& err)
	{
		if (token)
		{
			m_errorHandler->Error(token->Filename(), token->Line(), "at '" + token->Lexeme() + "'", err);
		}
		else
		{
			m_errorHandler->Error("<File>", -1, "at '<at>'", err);
		}
	}

	TValue AsInt(int bits);
	TValue AsFloat(int bits);
	TValue AsString();

	static void CastToMaxBits(TValue& lhs, TValue& rhs);

	static TValue Construct_Int(Token* type, int bits, std::string lexeme, bool global);
	static TValue Construct_Float(Token* type, int bits, std::string lexeme, bool global);
	static TValue Construct_Bool(Token* type, std::string lexeme, bool global);
	static TValue Construct_Enum(Token* type, std::string lexeme, bool global);
	static TValue Construct_String(Token* type, std::string lexeme, bool global);
	static TValue Construct_Vec_Dynamic(Token* type, TokenPtrList* args, std::string lexeme, bool global, TValueList* targs);
	static TValue Construct_Vec_Fixed(Token* type, TokenPtrList* args, std::string lexeme, bool global, TValueList* targs);
	static TValue Construct_Explicit(
		Token* token,
		LiteralTypeEnum type,
		llvm::Value* value,
		llvm::Type* ty,
		int bits,
		bool is_storage,
		int fixed_vec_len,
		LiteralTypeEnum vec_type)
	{
		TValue ret;
		ret.m_token = token;
		ret.m_type = type;
		ret.m_value = value;
		ret.m_ty = ty;
		ret.m_bits = bits;
		ret.m_is_storage = is_storage;
		ret.m_fixed_vec_len = fixed_vec_len;
		ret.m_vec_type = vec_type;
		return ret;
	}

	void GetFromStorage(TValue& lhs, TValue& rhs);

	static llvm::Type* MakeTy(LiteralTypeEnum type, int bits);

	static ErrorHandler* m_errorHandler;
	static llvm::IRBuilder<>* m_builder;
	static llvm::Module* m_module;

	llvm::Value* m_value;
	llvm::Type* m_ty;
	LiteralTypeEnum m_type;
	int m_bits;
	Token* m_token;
	bool m_is_storage;
	int m_fixed_vec_len;
	LiteralTypeEnum m_vec_type;
};


#endif // TVALUE_H