#ifndef EXTENSIONS_H
#define EXTENSIONS_H
#pragma once

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"

#include "Environment.h"

#include <string.h>
#include <stdlib.h>

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

extern "C" DLLEXPORT void ftoa2(double d, char* p)
{
	if (p) snprintf(p, 32, "%f", d);
}

extern "C" DLLEXPORT void itoa2(int d, char* p)
{
	if (p) snprintf(p, 32, "%d", d);
}

extern "C" DLLEXPORT void btoa2(int d, char* p)
{
	if (p)
	{
		if (0 == d) 
			snprintf(p, 32, "false", d);
		else
			snprintf(p, 32, "true", d);
	}
}

extern "C" DLLEXPORT double rand_impl()
{
	return (rand() % 10000) / 10000.0;
}

static void LoadExtensions(std::unique_ptr<IRBuilder<>>& builder, std::unique_ptr<Module>& module, Environment* env)
{
	// seed the rng with the time
	srand(time(nullptr));

	{
		std::vector<Type*> args(1, builder->getInt32Ty());
		FunctionType* FT = FunctionType::get(builder->getVoidTy(), args, false);
		Function::Create(FT, Function::InternalLinkage, "println", *module);
		Function::Create(FT, Function::InternalLinkage, "print", *module);
	}

	{
		std::vector<Type*> args(2, builder->getInt32Ty());
		FunctionType* FT = FunctionType::get(builder->getInt32Ty(), args, false);
		Function::Create(FT, Function::ExternalLinkage, "strcmp", *module);
	}

	{
		std::vector<Type*> args(2, builder->getInt32Ty());
		FunctionType* FT = FunctionType::get(builder->getInt32Ty(), args, false);
		Function::Create(FT, Function::ExternalLinkage, "strcat", *module);
	}

	{
		std::vector<Type*> args(3, builder->getInt32Ty());
		FunctionType* FT = FunctionType::get(builder->getInt32Ty(), args, false);
		Function::Create(FT, Function::ExternalLinkage, "strncpy", *module);
	}

	{
		std::vector<Type*> args(2, builder->getInt32Ty());
		FunctionType* FT = FunctionType::get(builder->getInt32Ty(), args, false);
		Function::Create(FT, Function::InternalLinkage, "itoa2", *module);
		Function::Create(FT, Function::InternalLinkage, "btoa2", *module);
	}

	{
		std::vector<Type*> args;
		args.push_back(builder->getDoubleTy());
		args.push_back(builder->getInt32Ty());
		FunctionType* FT = FunctionType::get(builder->getInt32Ty(), args, false);
		Function::Create(FT, Function::InternalLinkage, "ftoa2", *module);
	}

	{
		FunctionType* FT = FunctionType::get(builder->getDoubleTy(), {}, false);
		Function* ftn = Function::Create(FT, Function::InternalLinkage, "rand_impl", *module);
		env->DefineFunction("rand", ftn, 0, LITERAL_TYPE_DOUBLE);
	}
}


#endif // EXTENSIONS_H