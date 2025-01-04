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
#include "TType.h"


class TValue
{
public:

	typedef std::vector<TValue> TValueList;

	TValue()
	{
		m_token = nullptr;
		m_value = nullptr;
		m_is_storage = false;
		m_is_valid = false;
		m_is_map_pair = false;
		m_pair_lhs = nullptr;
		m_pair_rhs = nullptr;
	}

	static void RegisterErrorHandler(ErrorHandler* eh) { m_errorHandler = eh; }
	static void RegisterLLVM(llvm::IRBuilder<>* builder, llvm::Module* module)
	{
		m_builder = builder;
		m_module = module;
	}

	static TValue NullInvalid()
	{
		return TValue();
	}

	TValue As(TokenTypeEnum newType);
	TValue CastToFloat(int bits);
	TValue CastToInt(int bits);
	TValue CastToMatchImplicit(TValue src);
	TValue CastToMatchImplicit(TType ttype);

	void Cleanup();

	static TValue FromLiteral(Token* token, const Literal& literal);

	TValue GetFromStorage();
	Token* GetToken() { return m_token; }
	TType GetTType() { return m_ttype; }
	llvm::Value* GetPtrToStorage();
	//TValue GetPairLhs() { return *m_pair_lhs; }
	//TValue GetPairRhs() { return *m_pair_rhs; }

	static TValue Construct(TType ttype, std::string lexeme, bool global, TValueList* targs = nullptr);
	static TValue Construct_ConstInt(Token* type, std::string lexeme, Literal lit);
	static TValue Construct_Reference(TType ttype, std::string lexeme, llvm::Value* ptr);
	static TValue Construct_Pair(TValue lhs, TValue rhs);
	
	// type checking
	bool IsInteger() { return m_ttype.IsInteger(); }
	bool IsFloat() { return m_ttype.IsFloat(); }
	bool IsBool() { return m_ttype.IsBool(); }
	bool IsString() { return m_ttype.IsString(); }
	bool IsEnum() { return m_ttype.IsEnum(); }
	bool IsNumeric() { return m_ttype.IsNumeric(); }
	bool IsVecDynamic() { return m_ttype.IsVecDynamic(); }
	bool IsVecFixed() { return m_ttype.IsVecFixed(); }
	bool IsMap() { return m_ttype.IsMap(); }
	bool IsMapPair() { return m_is_map_pair; }
	bool IsSet() { return m_ttype.IsSet(); }
	bool IsTuple() { return m_ttype.IsTuple(); }
	bool IsUDT() { return m_ttype.IsUDT(); }
	//bool IsPointer()    { return m_type == LITERAL_TYPE_POINTER; }

	bool IsVecAny() { return m_ttype.IsVecAny(); }
	
	bool IsValid() { return m_is_valid; }//m_ttype.IsInvalid();
	bool IsConstant() { return m_ttype.IsConstant(); }
	bool IsReference() { return m_ttype.IsReference(); }
	
	bool CanIterate() { return m_ttype.CanIterate(); }
	bool CanIndex() { return m_ttype.CanIndex(); }

	
	static TValue MakeBool(Token* token, llvm::Value* value);
	static TValue MakeEnum(Token* token, llvm::Value* value);
	static TValue MakeFloat(Token* token, int bits, llvm::Value* value);
	static TValue MakeInt(Token* token, int bits, llvm::Value* value);
	static TValue MakeString(Token* token, llvm::Value* value);
	static TValue MakeDynVec(Token* token, llvm::Value* value, LiteralTypeEnum vectype, int bits);


	TValue MakeStorage();

	int NumBits() const { return m_ttype.NumBits(); }

	// binary operations
	TValue Add(TValue rhs);
	TValue Divide(TValue rhs);
	TValue Modulus(TValue rhs);
	TValue Multiply(TValue rhs);
	TValue Power(TValue rhs);
	TValue Subtract(TValue rhs);
	//
	TValue IsGreaterThan(TValue rhs, bool or_equal);
	TValue IsLessThan(TValue rhs, bool or_equal);
	TValue IsEqual(TValue rhs);
	TValue IsNotEqual(TValue rhs);

	TValue Negate();
	TValue Not();
	
	void SetStorage(bool val) { m_is_storage = val; }
	void Store(TValue rhs);

	llvm::Value* Value() { return m_value; }
	void SetValue(llvm::Value* val) { m_value = val; }


	TValue EmitLen();
	TValue EmitContains(TValue rhs);
	void EmitAppend(TValue rhs);
	void EmitMapInsert(TValue rhs);
	void EmitSetInsert(TValue rhs);
	void EmitStartIterator();

	TValue GetAtIndex(TValue idx);
	TValue GetIterKey();
	TValue GetIterValue();
	size_t GetFixedVecLen() const { return m_ttype.GetFixedVecLen(); }

	void StoreAtIndex(TValue idx, TValue rhs);

	TValue GetStructVariable(Token* token, llvm::Value* vec_idx, const std::string& name);

	/*
	
	TValue CastFixedToDynVec();	
	void SetTy(llvm::Type* ty) { m_ttype.SetTy(ty); }
	
	*/
private:

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
	
	
	TValue AsFloat(int bits);
	TValue AsInt(int bits);
	TValue AsString();

	static void CastToMaxBits(TValue& lhs, TValue& rhs);

	static void GetFromStorage(TValue& lhs, TValue& rhs);
	LiteralTypeEnum GetLiteralType() { return m_ttype.GetLiteralType(); }
	llvm::Type* GetTy() { return m_ttype.GetTy(); }
	
	static ErrorHandler* m_errorHandler;
	static llvm::IRBuilder<>* m_builder;
	static llvm::Module* m_module;

	llvm::Value* m_value;
	bool m_is_storage;
	bool m_is_valid;
	TType m_ttype;	
	Token* m_token;

	bool m_is_map_pair;
	TValue* m_pair_lhs;
	TValue* m_pair_rhs;
	
};


#endif // TVALUE_H