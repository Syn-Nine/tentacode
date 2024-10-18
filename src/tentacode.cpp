#include <stdio.h>
#include <iostream>
#include <conio.h>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

#include "jit.h"

/*/#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Verifier.h"*/

#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"

#include "Token.h"
#include "Parser.h"
#include "Scanner.h"
#include "ErrorHandler.h"
#include "Extensions.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>


#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "llvm/ADT/STLExtras.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

ErrorHandler* errorHandler;

using namespace llvm;
using namespace llvm::orc;

static std::unique_ptr<LLVMContext> TheContext;
static std::unique_ptr<Module> TheModule;
static std::unique_ptr<IRBuilder<>> Builder;
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

		// walk statements to generate code
		printf("\nWalking AST...\n");
		for (auto& statement : stmts)
		{
			StatementTypeEnum stype = statement->GetType();
			Value* v = statement->codegen(TheContext, Builder, TheModule);
			if (v)
			{
				printf("LLVM IR: ");
				v->print(errs());
				printf("\n");
			}
		}


		if (errorHandler->HasErrors())
		{
			errorHandler->Print();
			errorHandler->Clear();
		}
	}

	return true;
}

void RunPrompt()
{
	for (;;)
	{
		char inbuf[256];
		std::cout << "> ";
		std::cin.getline(inbuf, 256);

		if (!Run(inbuf, "Console")) break;
	}
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

	TheContext = std::make_unique<LLVMContext>();
	TheModule = std::make_unique<Module>("tentajit", *TheContext);
	TheModule->setDataLayout(TheJIT->getDataLayout());
	Builder = std::make_unique<IRBuilder<>>(*TheContext);

	LoadExtensions(Builder, TheModule);

	FunctionType* funcType = FunctionType::get(Builder->getVoidTy(), {}, false);
	Function* mainFunc = Function::Create(funcType, Function::ExternalLinkage, "main", *TheModule);
	BasicBlock* entry = BasicBlock::Create(*TheContext, "entry", mainFunc);
	Builder->SetInsertPoint(entry);

	char* cmd = "println(\"hello tenta\");\nprintln((5+10.1)*2.6/1.1);";
	printf("%s\n", cmd);

	Run(cmd, "Console");

	Builder->CreateRetVoid();

	verifyFunction(*mainFunc);

	printf("\nLLVM IR Dump:\n");
	TheModule->print(errs(), nullptr);

	auto TSM = ThreadSafeModule(std::move(TheModule), std::move(TheContext));
	TheJIT->addModule(std::move(TSM));

	auto ExprSymbol = ExitOnErr(TheJIT->lookup("main"));
	void (*FP)() = ExprSymbol.getAddress().toPtr<void (*)()>();
	
	printf("\nOutput:\n");
	FP();


	_getch();
}
