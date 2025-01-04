#include "Expressions.h"

//-----------------------------------------------------------------------------
TValue CallExpr::codegen_file(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env, Token* callee)
{
	std::string name = callee->Lexeme();

	if (0 == name.compare("file::readlines"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue arg = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		if (arg.IsString())
		{
			llvm::Value* v = builder->CreateCall(module->getFunction("__file_readlines"), { arg.Value() }, "calltmp");
			return TValue::MakeDynVec(callee, v, LITERAL_TYPE_STRING, 64);
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("file::readstring"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue arg = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		if (arg.IsString())
		{
			llvm::Value* s = builder->CreateCall(module->getFunction("__file_readstring"), { arg.Value() }, "calltmp");
			return TValue::MakeString(callee, s);
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("file::writelines"))
	{
		if (!CheckArgSize(2)) return TValue::NullInvalid();

		TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		TValue rhs = m_arguments[1]->codegen(builder, module, env);
		if (lhs.IsString() && rhs.IsVecAny())
		{
			if (rhs.IsVecDynamic())
			{
				llvm::Value* rhs_load = builder->CreateLoad(builder->getPtrTy(), rhs.Value(), "loadtmp");
				builder->CreateCall(module->getFunction("__file_writelines_dyn_vec"), { lhs.Value(), rhs_load }, "calltmp");
			}
			else
			{
				llvm::Value* gep = builder->CreateGEP(builder->getPtrTy(), rhs.Value(), builder->getInt32(0), "geptmp");
				llvm::Value* len = builder->getInt64(rhs.GetFixedVecLen());
				builder->CreateCall(module->getFunction("__file_writelines_fixed_vec"), { lhs.Value(), gep, len }, "calltmp");
			}
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("file::writestring"))
	{
		if (!CheckArgSize(2)) return TValue::NullInvalid();

		TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		TValue rhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage();
		if (lhs.IsString() && rhs.IsString())
		{
			builder->CreateCall(module->getFunction("__file_writestring"), { lhs.Value(), rhs.Value() }, "calltmp");
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	
	return TValue::NullInvalid();
}
