#include <stdio.h>
#include <fstream>
#include <iostream>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

#include "jit.h"

#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Verifier.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"

#include "TStruct.h"
#include "TFunction.h"
#include "TValue.h"
#include "Token.h"
#include "Parser.h"
#include "Scanner.h"
#include "ErrorHandler.h"
#include "Extensions.h"
#include "Extensions_Raylib.h"
#include "Environment.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>


ErrorHandler* errorHandler;

static std::unique_ptr<llvm::LLVMContext> context;
static std::unique_ptr<llvm::Module> module;
static std::unique_ptr<llvm::IRBuilder<>> builder;
static std::unique_ptr<llvm::orc::KaleidoscopeJIT> TheJIT;
static llvm::ExitOnError ExitOnErr;

bool Run(const char* buf, const char* filename)
{
	if (std::string(buf).compare("quit") == 0) return false;
	if (std::string(buf).compare("q") == 0) return false;


	Scanner scanner(buf, errorHandler, filename);
	TokenList tokens = scanner.ScanTokens();

	if (!tokens.empty() && tokens.at(0).GetType() != TOKEN_END_OF_FILE)
	{
		Parser parser(tokens, errorHandler);
		StmtList stmts = parser.Parse();

		// push global environment
		Environment* env = Environment::Push();
		env->RegisterErrorHandler(errorHandler);

		TValue::RegisterErrorHandler(errorHandler);
		TValue::RegisterLLVM(builder.get(), module.get());
		TFunction::RegisterLLVM(builder.get(), module.get());
		TStruct::RegisterLLVM(builder.get(), module.get());
		
		LoadExtensions(builder.get(), module.get(), env);
		LoadExtensions_Raylib(builder.get(), module.get(), env);

		if (2 <= Environment::GetDebugLevel()) printf("\nWalking AST...\n");

		// walk function and struct definitions
		for (auto& statement : stmts)
		{
			if (env->HasErrors()) break;
			if (STATEMENT_STRUCT == statement->GetType())
			{
				statement->codegen(builder.get(), module.get(), env);
			}

			if (STATEMENT_FUNCTION == statement->GetType())
			{
				static_cast<FunctionStmt*>(statement)->codegen_prototype(builder.get(), module.get(), env);
			}
		}

		llvm::FunctionType* funcType = llvm::FunctionType::get(builder->getVoidTy(), {}, false);
		llvm::Function* mainFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "jit_main", module.get());
		llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context, "entry", mainFunc);
		builder->SetInsertPoint(entry);

		// push main ftn environment
		Environment::Push();

		// walk main code
		for (auto& statement : stmts)
		{
			if (env->HasErrors()) break;
			if (STATEMENT_FUNCTION != statement->GetType() && STATEMENT_STRUCT != statement->GetType())
			{
				statement->codegen(builder.get(), module.get(), env);
			}
		}

		// pop main ftn environment
		Environment::Pop();

		builder->CreateRetVoid();
		verifyFunction(*mainFunc);

		// fill in functions
		for (auto& statement : stmts)
		{
			if (env->HasErrors()) break;
			if (STATEMENT_FUNCTION == statement->GetType())
			{
				statement->codegen(builder.get(), module.get(), env);
			}
		}

		// pop global environment
		Environment::Pop();


		if (1 <= Environment::GetDebugLevel())
		{
			printf("\nLLVM IR Dump:\n");
			module->print(llvm::errs(), nullptr);
		}
	}

	return true;
}

void RunInternal()
{
	char* cmd = "println(\"a\");\n{ if 1 < 2 { println(\"c\"); } }\nprintln(\"b\");";
	
	printf("%s\n", cmd);	
	Run(cmd, "Internal");
}

void RunFile(const char* filename)
{
	std::ifstream f;
	f.open(filename, std::ios::in | std::ios::binary | std::ios::ate);

	if (!f.is_open())
	{
		printf("Failed to open file: %s\n", filename);
		RunInternal();
		return;
	}

	const std::size_t n = f.tellg();
	char* buffer = new char[n + 1];
	buffer[n] = '\0';

	f.seekg(0, std::ios::beg);
	f.read(buffer, n);
	f.close();
	
	Run(buffer, filename);
	
	delete[] buffer;
}


int main(int nargs, char* argsv[])
{
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();

	const char* version = "0.2.1";
	printf("Launching Tentacode JIT Compiler v%s\n", version);

	Environment::SetDebugLevel(1);

	errorHandler = new ErrorHandler();

	TheJIT = ExitOnErr(llvm::orc::KaleidoscopeJIT::Create());

	context = std::make_unique<llvm::LLVMContext>();
	module = std::make_unique<llvm::Module>("tentajit", *context);
	module->setDataLayout(TheJIT->getDataLayout());
	builder = std::make_unique<llvm::IRBuilder<>>(*context);

	if (1 == nargs)
	{
		std::ifstream f;
		f.open("autoplay.tt", std::ios::in | std::ios::binary | std::ios::ate);
		if (f.is_open())
		{
			RunFile("autoplay.tt");
		}
		else
		{
			RunInternal();
		}
	}
	else if (2 == nargs)
	{
		printf("Run File: %s\n", argsv[1]);
		RunFile(argsv[1]);
	}
	else
	{
		printf("Usage: interp [script]\nOmit [script] to run prompt\n");
	}

	if (errorHandler->HasErrors())
	{
		errorHandler->Print();
	}
	else
	{
		auto TSM = llvm::orc::ThreadSafeModule(std::move(module), std::move(context));
		TheJIT->addModule(std::move(TSM));

		auto ExprSymbol = ExitOnErr(TheJIT->lookup("jit_main"));
		void (*FP)() = ExprSymbol.getAddress().toPtr<void (*)()>();

		printf("\nOutput:\n");
		FP();
	}

}
