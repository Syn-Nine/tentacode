#include <stdio.h>
#include <fstream>
#include <iostream>
#include <conio.h>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

#include "jit.h"

#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Verifier.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"

#include "Token.h"
#include "Parser.h"
#include "Scanner.h"
#include "ErrorHandler.h"
#include "Extensions.h"
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

using namespace llvm;
using namespace llvm::orc;

static std::unique_ptr<LLVMContext> context;
static std::unique_ptr<Module> module;
static std::unique_ptr<IRBuilder<>> builder;
static std::unique_ptr<llvm::orc::KaleidoscopeJIT> TheJIT;
static ExitOnError ExitOnErr;

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
		Environment* env = new Environment(nullptr);

		LoadExtensions(builder, module, env);

		printf("\nWalking AST...\n");

		// walk function definitions
		for (auto& statement : stmts)
		{
			if (STATEMENT_FUNCTION == statement->GetType())
			{
				Value* v = statement->codegen(context, builder, module, env);
				if (v)
				{
					printf("LLVM IR: ");
					v->print(errs());
					printf("\n");
				}
			}
		}

		FunctionType* funcType = FunctionType::get(builder->getVoidTy(), {}, false);
		Function* mainFunc = Function::Create(funcType, Function::ExternalLinkage, "main", *module);
		BasicBlock* entry = BasicBlock::Create(*context, "entry", mainFunc);
		builder->SetInsertPoint(entry);

		// walk main code
		for (auto& statement : stmts)
		{
			if (STATEMENT_FUNCTION != statement->GetType())
			{
				Value* v = statement->codegen(context, builder, module, env);
				if (v)
				{
					printf("LLVM IR: ");
					v->print(errs());
					printf("\n");
				}
			}
		}

		builder->CreateRetVoid();
		verifyFunction(*mainFunc);

		delete env;

		if (errorHandler->HasErrors())
		{
			errorHandler->Print();
			errorHandler->Clear();
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
	InitializeNativeTarget();
	InitializeNativeTargetAsmPrinter();
	InitializeNativeTargetAsmParser();


	const char* version = "0.2.0";
	printf("Launching Tentacode JIT Compiler v%s\n", version);

	errorHandler = new ErrorHandler();

	TheJIT = ExitOnErr(KaleidoscopeJIT::Create());

	context = std::make_unique<LLVMContext>();
	module = std::make_unique<Module>("tentajit", *context);
	module->setDataLayout(TheJIT->getDataLayout());
	builder = std::make_unique<IRBuilder<>>(*context);

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


	printf("\nLLVM IR Dump:\n");
	module->print(errs(), nullptr);

	auto TSM = ThreadSafeModule(std::move(module), std::move(context));
	TheJIT->addModule(std::move(TSM));

	auto ExprSymbol = ExitOnErr(TheJIT->lookup("main"));
	void (*FP)() = ExprSymbol.getAddress().toPtr<void (*)()>();
	
	printf("\nOutput:\n");
	FP();


	_getch();
}
