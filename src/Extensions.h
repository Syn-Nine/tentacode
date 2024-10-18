#ifndef EXTENSIONS_H
#define EXTENSIONS_H
#pragma once

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

using namespace llvm;

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

extern "C" DLLEXPORT void print(int32_t* p)
{
	if (p) printf("%s", p);
}

extern "C" DLLEXPORT void println(int32_t* p)
{
	if (p) printf("%s\n", p);
}

static void LoadExtensions(std::unique_ptr<IRBuilder<>>& builder, std::unique_ptr<Module>& module)
{
	{
		std::vector<Type*> args(1, builder->getInt32Ty());
		FunctionType* FT = FunctionType::get(builder->getVoidTy(), args, false);
		Function::Create(FT, Function::ExternalLinkage, "println", *module);
		Function::Create(FT, Function::ExternalLinkage, "print", *module);
	}
}


#endif // EXTENSIONS_H