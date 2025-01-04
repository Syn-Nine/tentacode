#ifndef TTYPE_H
#define TTYPE_H

#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"

#include <vector>
#include "Literal.h"
#include "Token.h"

class TType
{
public:
	typedef std::vector<TType> TTypeList;

	TType()
	{
		m_is_valid = false;
		m_token = nullptr;
		m_args = nullptr;
		m_literal_type = LITERAL_TYPE_INVALID;
		m_is_constant = false;
		m_is_reference = false;
		m_vec_sz = 0;
	}

	static void RegisterLLVM(llvm::IRBuilder<>* builder, llvm::Module* module)
	{
		m_builder = builder;
		m_module = module;
	}

	static TType Construct(Token* token, TokenPtrList* args);
	static TType Construct_Bool(Token* token);
	static TType Construct_Enum(Token* token);
	static TType Construct_Float(Token* token, int bits);
	static TType Construct_Int(Token* token, int bits);
	static TType Construct_String(Token* token);
	static TType Construct_Map(Token* token, TokenPtrList* args);
	static TType Construct_Set(Token* token, TokenPtrList* args);
	static TType Construct_Tuple(Token* token, TokenPtrList* args);
	static TType Construct_Vec(Token* token, TokenPtrList* args);
	static TType Construct_Ptr(Token* token);
	static TType Construct_Struct(Token* token);


	bool IsBool() { return m_literal_type == LITERAL_TYPE_BOOL; }
	bool IsEnum() { return m_literal_type == LITERAL_TYPE_ENUM; }
	bool IsFloat() { return m_literal_type == LITERAL_TYPE_FLOAT; }
	bool IsInteger() { return m_literal_type == LITERAL_TYPE_INTEGER; }
	bool IsString() { return m_literal_type == LITERAL_TYPE_STRING; }
	bool IsNumeric() { return m_literal_type == LITERAL_TYPE_FLOAT || m_literal_type == LITERAL_TYPE_INTEGER; }
	bool IsVecDynamic() { return m_literal_type == LITERAL_TYPE_VEC_DYNAMIC; }
	bool IsVecFixed() { return m_literal_type == LITERAL_TYPE_VEC_FIXED; }
	bool IsMap() { return m_literal_type == LITERAL_TYPE_MAP; }
	bool IsSet() { return m_literal_type == LITERAL_TYPE_SET; }
	bool IsTuple() { return m_literal_type == LITERAL_TYPE_TUPLE; }
	bool IsUDT() { return m_literal_type == LITERAL_TYPE_UDT; }

	bool CanIterate() {
		return m_literal_type == LITERAL_TYPE_VEC_DYNAMIC ||
			m_literal_type == LITERAL_TYPE_VEC_FIXED ||
			m_literal_type == LITERAL_TYPE_MAP ||
			m_literal_type == LITERAL_TYPE_SET;
	}
	bool CanGEP() {
		return IsVecFixed() || IsUDT();
	}
	bool CanIndex()
	{
		return IsVecAny() || IsMap() || IsTuple();
	}

	bool IsVecAny() { return m_literal_type == LITERAL_TYPE_VEC_DYNAMIC || m_literal_type == LITERAL_TYPE_VEC_FIXED; }

	bool IsValid() const { return m_is_valid; }
	bool IsConstant() const { return m_is_constant; }
	bool IsReference() const { return m_is_reference; }
	void SetConstant() { m_is_constant = true; }
	void SetReference() { m_is_reference = true; }

	Token* GetToken() { return m_token; }
	llvm::Type* GetTy() { return m_ty; }
	LiteralTypeEnum GetLiteralType() { return m_literal_type; }
	llvm::Value* GetSzBytes() { return m_ty_sz_bytes; }

	int NumBits() const { return m_bits; }

	TType GetInternal(size_t idx);
	int GetInternalCount() { return m_inner_types.size(); }
	
	size_t GetFixedVecLen() const { return m_vec_sz; }


	/****
	//bool IsPointer()    { return m_literal_type == LITERAL_TYPE_POINTER; }

	//bool IsInvalid() { return m_literal_type == LITERAL_TYPE_INVALID; }
		
	//LiteralTypeEnum GetVecType() { return m_vec_literal_type; }
	//std::string GetVecTypeName() { return m_vec_type_name; }
	
	void SetTy(llvm::Type* ty) { m_ty = ty; }*/


private:

	static llvm::IRBuilder<>* m_builder;
	static llvm::Module* m_module;

	bool m_is_valid;
	bool m_is_constant;
	bool m_is_reference;
	Token* m_token;
	std::string m_lexeme;
	TokenPtrList* m_args;
	LiteralTypeEnum m_literal_type;
	int m_bits;
	llvm::Type* m_ty;
	llvm::Value* m_ty_sz_bytes;
	size_t m_vec_sz;

	std::vector<TType> m_inner_types;

};

#endif // TTYPE_H