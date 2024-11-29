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
	static TValue Construct_Prototype(Token* token);
	static TValue Construct_ReturnValue(Token* token, LiteralTypeEnum type, int bits, llvm::Value* a);
	static TValue Construct_Explicit(Token* token, LiteralTypeEnum type, llvm::Value* value, llvm::Type* ty, int bits, bool is_storage, int fixed_vec_len, LiteralTypeEnum vec_type);


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
	bool IsUDT()        { return m_type == LITERAL_TYPE_UDT; }
	//bool IsPointer()    { return m_type == LITERAL_TYPE_POINTER; }

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
	std::string GetVecTypeName() { return m_vec_type_name; }
	TValue GetStructVariable(Token* token, llvm::Value* vec_idx, const std::string& name);

	bool IsInvalid() { return m_type == LITERAL_TYPE_INVALID; }

	llvm::Value* Value() { return m_value; }

	TValue CastFixedToDynVec();
	TValue CastToFloat(int bits);
	TValue CastToInt(int bits);
	TValue CastToMatchImplicit(TValue src);

	void Cleanup();

	TValue GetAtVectorIndex(TValue idx);

	void Store(TValue rhs);
	void StoreAtIndex(TValue idx, TValue rhs);

	static TValue MakeBool(Token* token, llvm::Value* value);
	static TValue MakeEnum(Token* token, llvm::Value* value);
	static TValue MakeInt(Token* token, int bits, llvm::Value* value);
	static TValue MakeFloat(Token* token, int bits, llvm::Value* value);
	static TValue MakeString(Token* token, llvm::Value* value);
	static TValue MakeDynVec(Token* token, llvm::Value* value, LiteralTypeEnum vectype, int bits);

	TValue Negate();
	TValue Not();

	void EmitAppend(TValue rhs);
	TValue EmitContains(TValue rhs);
	TValue EmitLen();

	int NumBits() const { return m_bits; }

	void SetTy(llvm::Type* ty) { m_ty = ty; }
	void SetValue(llvm::Value* val) { m_value = val; }
	void SetStorage(bool val) { m_is_storage = val; }
	

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
	static TValue Construct_Pointer(Token* type, std::string lexeme, bool global);
	static TValue Construct_String(Token* type, std::string lexeme, bool global);
	static TValue Construct_Vec_Dynamic(Token* type, TokenPtrList* args, std::string lexeme, bool global, TValueList* targs);
	static TValue Construct_Vec_Fixed(Token* type, TokenPtrList* args, std::string lexeme, bool global, TValueList* targs);
	static TValue Construct_Struct(Token* type, std::string lexeme, bool global);
	
	void GetFromStorage(TValue& lhs, TValue& rhs);

	static llvm::Type* MakeTy(LiteralTypeEnum type, int bits);
	static llvm::Type* MakeFloatTy(int bits);
	static inline llvm::Type* MakeIntTy(int bits) { return m_builder->getIntNTy(bits); }
	static inline llvm::Type* MakePointerTy() { return m_builder->getPtrTy(); }
	static inline llvm::Type* MakeVoidTy() { return m_builder->getVoidTy(); }
	
	static bool TokenToType(Token* token, LiteralTypeEnum& type, int& bits, llvm::Value*& sz_bytes, llvm::Type*& ty);

	static ErrorHandler* m_errorHandler;
	static llvm::IRBuilder<>* m_builder;
	static llvm::Module* m_module;

	Token* m_token;
	LiteralTypeEnum m_type;
	llvm::Value* m_value;
	llvm::Type* m_ty;
	int m_bits;
	bool m_is_storage;
	int m_fixed_vec_len;
	LiteralTypeEnum m_vec_type;
	std::string m_vec_type_name;
};


#endif // TVALUE_H