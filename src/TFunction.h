#ifndef TFUNCTION_H
#define TFUNCTION_H

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"


#include "TValue.h"
#include "Token.h"
#include "Literal.h"
#include "ErrorHandler.h"

#include <vector>

class TFunction
{
public:
	TFunction()
	{
		m_valid = false;
		m_body = nullptr;
		m_llvm_func = nullptr;
	}

	static void RegisterErrorHandler(ErrorHandler* eh) { m_errorHandler = eh; }
	static void RegisterLLVM(llvm::IRBuilder<>* builder, llvm::Module* module)
	{
		m_builder = builder;
		m_module = module;
	}


	static TFunction Construct_Internal(
		std::string lexeme,
		llvm::Function* ftn,
		std::vector<LiteralTypeEnum> types,
		std::vector<llvm::Type*> args,
		LiteralTypeEnum rettype);

	static TFunction Construct_Prototype(Token* name, Token* rettype, TokenList types, TokenList params, void* bodyPtr);
	
	void Construct_Body();

	bool IsValid() const { return m_valid; }

	llvm::Function* GetLLVMFunc() { return m_llvm_func; }

	std::vector<TValue> GetParams() { return m_param_types; }
	TValue GetReturn() { return m_retval; }

private:

	static ErrorHandler* m_errorHandler;
	static llvm::IRBuilder<>* m_builder;
	static llvm::Module* m_module;

	bool m_valid;
	void* m_body;

	Token* m_name;
	TValue m_retval;
	llvm::Function* m_llvm_func;

	std::vector<TValue> m_param_types;
	std::vector<std::string> m_param_names;
};


#endif // TFUNCTION_H