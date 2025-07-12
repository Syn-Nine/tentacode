#ifndef TFUNCTION_H
#define TFUNCTION_H

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"

#include "TValue.h"
#include "TType.h"
#include "Token.h"
#include "Literal.h"

#include <vector>
#include <string>

class Expr;

class TFunction
{
public:
	TFunction()
	{
		m_valid = false;
		m_body = nullptr;
		m_llvm_func = nullptr;
		m_name = nullptr;
		m_has_return_ref = false;
		m_internal = false;
	}

	static void RegisterLLVM(llvm::IRBuilder<>* builder, llvm::Module* module)
	{
		m_builder = builder;
		m_module = module;
	}


	static TFunction Construct_Internal(
		std::string fqns,
		std::string lexeme,
		llvm::Function* ftn,
		std::vector<LiteralTypeEnum> types,
		std::vector<llvm::Type*> args,
		LiteralTypeEnum rettype);

	static TFunction Construct_Prototype(std::string fqns, Token* name, TypeToken* rettype, TypeTokenList types, TokenList params, std::vector<bool> mut, std::vector<Expr*> defaults, void* bodyPtr);

	static TFunction Construct_AnonPrototype(const std::string& sig, void* bodyPtr);
	
	void Construct_Body();

	bool IsInternal() const { return m_internal; }
	bool IsValid() const { return m_valid; }

	llvm::Function* GetLLVMFunc() { return m_llvm_func; }
	llvm::FunctionType* GetLLVMFuncType() { return m_llvm_ft; }

	std::vector<TType> GetParamTypes() const { return m_paramTypes; }
	std::vector<std::string> GetParamNames() const { return m_paramNames; }

	TType GetReturnType()
	{
		if (m_has_return_ref) return m_paramTypes[0];
		return TType();
	}

	TType GetInternalReturnType() { return m_rettype_internal; }

	Token* GetToken() { return m_name; }

	std::string GetFQNS() { return m_fqns; }

	std::vector<Expr*> GetDefaults() { return m_defaults; }
	size_t GetNumDefaults() { return m_num_defaults; }

private:

	static llvm::IRBuilder<>* m_builder;
	static llvm::Module* m_module;

	bool m_internal;
	bool m_valid;
	void* m_body;

	Token* m_name;
	bool m_has_return_ref;
	TType m_rettype_internal;

	TValue m_retref;
	llvm::Function* m_llvm_func;
	llvm::FunctionType* m_llvm_ft;

	std::vector<TType> m_paramTypes;
	std::vector<std::string> m_paramNames;
	std::vector<Expr*> m_defaults;
	size_t m_num_defaults;

	std::string m_fqns;

};


#endif // TFUNCTION_H