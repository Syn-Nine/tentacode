#include "Expressions.h"


//-----------------------------------------------------------------------------
bool CallExpr::CheckArgSize(int count)
{
	if (count != m_arguments.size())
	{
		Token* callee = (static_cast<VariableExpr*>(m_callee))->Operator();
		Environment::Error(callee, "Argument count mismatch.");
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
TValue CallExpr::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("CallExpr::codegen()\n");
	
	if (!m_callee) return TValue::NullInvalid();
	if (EXPRESSION_VARIABLE != m_callee->GetType()) return TValue::NullInvalid();

	Token* callee = (static_cast<VariableExpr*>(m_callee))->Operator();
	std::string name = callee->Lexeme();

	// namespaced
	if (0 == name.substr(0, 6).compare("file::"))
	{
		return codegen_file(builder, module, env, callee);
	}
	else if (0 == name.substr(0, 5).compare("str::"))
	{
		return codegen_str(builder, module, env, callee);
	}
	else if (0 == name.substr(0, 5).compare("vec::"))
	{
		return codegen_vec(builder, module, env, callee);
	}

	// math functions
	TValue ret = codegen_math(builder, module, env, callee);
	if (!ret.IsInvalid()) return ret;

	// everything else
	if (0 == name.compare("clock"))
	{
		if (!CheckArgSize(0)) return TValue::NullInvalid();

		return TValue::MakeFloat(callee, 64, builder->CreateCall(module->getFunction("__clock_impl"), {}, "calltmp"));
	}
	else if (0 == name.compare("rand"))
	{
		if (m_arguments.empty())
		{
			return TValue::MakeFloat(callee, 64, builder->CreateCall(module->getFunction("__rand_impl"), {}, "calltmp"));
		}
		else if (EXPRESSION_RANGE == m_arguments[0]->GetType())
		{
			if (!CheckArgSize(1)) return TValue::NullInvalid();

			RangeExpr* re = static_cast<RangeExpr*>(m_arguments[0]);
			TValue lhs = re->Left()->codegen(builder, module, env).GetFromStorage().CastToInt(64);
			TValue rhs = re->Right()->codegen(builder, module, env).GetFromStorage().CastToInt(64);
			return TValue::MakeInt(callee, 64, builder->CreateCall(module->getFunction("__rand_range_impl"), { lhs.Value(), rhs.Value()}, "calltmp"));
		}
		else
		{
			env->Error(callee, "Argument count mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("len"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		if (EXPRESSION_VARIABLE == m_arguments[0]->GetType())
		{
			VariableExpr* ve = static_cast<VariableExpr*>(m_arguments[0]);
			std::string var = ve->Operator()->Lexeme();
			TValue tval = env->GetVariable(callee, var);
			if (tval.IsVecAny()) return tval.EmitLen();

			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
		/*else if (EXPRESSION_GET == m_arguments[0]->GetType())
		{
			GetExpr* ge = static_cast<GetExpr*>(m_arguments[0]);
			TValue v = m_arguments[0]->codegen(builder, module, env);

			if (v.IsVec())
			{
				llvm::Value* src = builder->CreateLoad(builder->getPtrTy(), v.value);
				llvm::Value* srcType = builder->getInt32(v.fixed_vec_type);
				return TValue::Integer(builder->CreateCall(module->getFunction("__vec_len"), { srcType, src }, "calltmp"));
			}
			else
			{
				env->Error(callee, "Argument type mismatch.");
				return TValue::NullInvalid();
			}
		}*/
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("input"))
	{
		llvm::Value* s = builder->CreateCall(module->getFunction("input"), {}, "calltmp");
		return TValue::MakeString(callee, s);
	}	
	else
	{
		TFunction tfunc = env->GetFunction(callee, name);
		if (tfunc.IsValid())
		{
			llvm::Function* ftn = tfunc.GetLLVMFunc();

			std::vector<TValue> params = tfunc.GetParams();
			TValue ret = tfunc.GetReturn();

			if (!CheckArgSize(params.size())) return TValue::NullInvalid();
			
			std::vector<llvm::Value*> args;
			for (size_t i = 0; i < m_arguments.size(); ++i)
			{
				TValue v = m_arguments[i]->codegen(builder, module, env).GetFromStorage();

				if (params[i].isNumeric())
				{
					v = v.CastToMatchImplicit(params[i]);
				}
				else if (v.IsVecDynamic())
				{
					v.SetValue(builder->CreateLoad(builder->getPtrTy(), v.Value(), "tmp_load"));
				}
				else if (v.IsVecFixed())
				{
					v = v.CastFixedToDynVec();
					v.SetValue(builder->CreateLoad(builder->getPtrTy(), v.Value(), "tmp_load"));
				}
				args.push_back(v.Value());
			}
			llvm::Value* rval = builder->CreateCall(ftn, args, std::string("call_" + name).c_str());
			ret.SetValue(rval);
			return ret;
		}
		else
		{
			env->Error(callee, "Invalid function call.");
			return TValue::NullInvalid();
		}
	}
	
	return TValue::NullInvalid();

}

