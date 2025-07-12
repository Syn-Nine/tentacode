#include "Expressions.h"
#include "llvm/Support/Parallel.h"
//-----------------------------------------------------------------------------
TValue CallExpr::codegen_str(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env, Token* callee)
{
	std::string name = callee->Lexeme();

	if (0 == name.compare("str::tochar"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		lhs = lhs.CastToInt(32);
		if (lhs.IsInteger())
		{
			return TValue::MakeString(callee, builder->CreateCall(module->getFunction("__str_tochar"), { lhs.Value() }, "calltmp"));
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("str::toint"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		if (lhs.IsString())
		{
			return TValue::MakeInt(callee, 32, builder->CreateCall(module->getFunction("__str_toint"), { lhs.Value() }, "calltmp"));
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("str::contains"))
	{
		if (!CheckArgSize(2)) return TValue::NullInvalid();

		TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		TValue rhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage();
		if (lhs.IsString() && rhs.IsString())
		{
			llvm::Value* b = builder->CreateCall(module->getFunction("__str_contains"), { lhs.Value(), rhs.Value() }, "calltmp");
			return TValue::MakeBool(callee, b);
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("str::replace"))
	{
		if (!CheckArgSize(3)) return TValue::NullInvalid();

		TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		TValue mhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage();
		TValue rhs = m_arguments[2]->codegen(builder, module, env).GetFromStorage();
		if (lhs.IsString() && mhs.IsString() && rhs.IsString())
		{
			llvm::Value* s = builder->CreateCall(module->getFunction("__str_replace"), { lhs.Value(), mhs.Value(), rhs.Value() }, "calltmp");
			return TValue::MakeString(callee, s);
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
		}
	else if (0 == name.compare("str::split"))
	{
		if (!CheckArgSize(2)) return TValue::NullInvalid();

		TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		TValue rhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage();
		if (lhs.IsString() && rhs.IsString())
		{
			llvm::Value* v = builder->CreateCall(module->getFunction("__str_split"), { lhs.Value(), rhs.Value() }, "calltmp");
			return TValue::MakeDynVec(callee, v, LITERAL_TYPE_STRING, 64);
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
		}
	else if (0 == name.compare("str::join"))
	{
		if (!CheckArgSize(2)) return TValue::NullInvalid();

		TValue lhs = m_arguments[0]->codegen(builder, module, env);
		TValue rhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage();
		if (lhs.IsVecAny() && lhs.GetTType().GetInternal(0).IsString() && rhs.IsString())
		{
			llvm::Value* s = nullptr;
			if (lhs.IsVecDynamic())
			{
				llvm::Value* lhs_load = builder->CreateLoad(builder->getPtrTy(), lhs.Value(), "loadtmp");
				s = builder->CreateCall(module->getFunction("__str_join_dyn_vec"), { lhs_load, rhs.Value() }, "calltmp");
				return TValue::MakeString(callee, s);
			}
			else
			{
				llvm::Value* gep = builder->CreateGEP(builder->getPtrTy(), lhs.Value(), builder->getInt32(0), "geptmp");
				llvm::Value* len = builder->getInt64(lhs.GetFixedVecLen());
				s = builder->CreateCall(module->getFunction("__str_join_fixed_vec"), { gep, rhs.Value(), len }, "calltmp");
				return TValue::MakeString(callee, s);
			}
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
		}
	else if (0 == name.compare("str::substr"))
	{
		if (2 <= m_arguments.size())
		{
			TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
			TValue rhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage();
			if (lhs.IsString() && rhs.IsInteger())
			{
				llvm::Value* len;
				if (2 < m_arguments.size())
				{
					len = m_arguments[2]->codegen(builder, module, env).GetFromStorage().Value();
				}
				else
				{
					len = builder->getInt32(-1);
				}

				if (len->getType()->isIntegerTy())
				{
					llvm::Value* s = builder->CreateCall(module->getFunction("__str_substr"), { lhs.Value(), rhs.Value(), len }, "calltmp");
					return TValue::MakeString(callee, s);
				}
				else
				{
					env->Error(callee, "Argument type mismatch.");
					return TValue::NullInvalid();
				}
			}
			else
			{
				env->Error(callee, "Argument type mismatch.");
				return TValue::NullInvalid();
			}
		}
		else
		{
			env->Error(callee, "Argument count mismatch.");
			return TValue::NullInvalid();
		}
		}
	else if (0 == name.compare("str::toupper"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue arg = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		if (arg.IsString())
		{
			llvm::Value* s = builder->CreateCall(module->getFunction("__str_toupper"), { arg.Value() }, "calltmp");
			return TValue::MakeString(callee, s);
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
		}
	else if (0 == name.compare("str::tolower"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue arg = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		if (arg.IsString())
		{
			llvm::Value* s = builder->CreateCall(module->getFunction("__str_tolower"), { arg.Value() }, "calltmp");
			return TValue::MakeString(callee, s);
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
		}
	else if (0 == name.compare("str::ltrim"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue arg = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		if (arg.IsString())
		{
			llvm::Value* s = builder->CreateCall(module->getFunction("__str_ltrim"), { arg.Value() }, "calltmp");
			return TValue::MakeString(callee, s);
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
		}
	else if (0 == name.compare("str::rtrim"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue arg = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		if (arg.IsString())
		{
			llvm::Value* s = builder->CreateCall(module->getFunction("__str_rtrim"), { arg.Value() }, "calltmp");
			return TValue::MakeString(callee, s);
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
		}
	else if (0 == name.compare("str::trim"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue arg = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		if (arg.IsString())
		{
			llvm::Value* s = builder->CreateCall(module->getFunction("__str_trim"), { arg.Value() }, "calltmp");
			return TValue::MakeString(callee, s);
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else
	{
		env->Error(callee, "Function not found in namespace.");
		return TValue::NullInvalid();
	}
	
	return TValue::NullInvalid();
}

