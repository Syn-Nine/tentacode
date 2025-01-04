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
	if (0 == name.substr(0, 5).compare("map::"))
	{
		return codegen_map(builder, module, env, callee);
	}
	else if (0 == name.substr(0, 5).compare("set::"))
	{
		return codegen_set(builder, module, env, callee);
	}
	else if (0 == name.substr(0, 5).compare("vec::"))
	{
		return codegen_vec(builder, module, env, callee);
	}
	if (0 == name.substr(0, 6).compare("file::"))
	{
		return codegen_file(builder, module, env, callee);
	}
	else if (0 == name.substr(0, 5).compare("str::"))
	{
		return codegen_str(builder, module, env, callee);
	}
	
	// math functions
	TValue ret = codegen_math(builder, module, env, callee);
	if (ret.IsValid()) return ret;

	if (0 == name.compare("parfor"))
	{
		if (m_arguments.size() < 2 || m_arguments.size() > 3)
		{
			env->Error(callee, "Argument count mismatch.");
			return TValue::NullInvalid();
		}

		TValue n_threads;
		TValue n_jobs;
		llvm::Function* ftn = nullptr;

		if (2 == m_arguments.size())
		{
			n_threads = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
			if (!n_threads.IsValid()) return TValue::NullInvalid();
			if (!n_threads.IsInteger())
			{
				env->Error(n_threads.GetToken(), "Expected integer number of threads.");
				return TValue::NullInvalid();
			}

			if (EXPRESSION_VARIABLE != m_arguments[1]->GetType())
			{
				env->Error(n_threads.GetToken(), "Expected function.");
				return TValue::NullInvalid();
			}

			Token* ftntok = static_cast<VariableExpr*>(m_arguments[1])->Operator();
			std::string ftnlex = ftntok->Lexeme();
			if (!env->GetFunction(ftntok, ftnlex).IsValid())
			{
				env->Error(ftntok, "Function not found.");
				return TValue::NullInvalid();
			}
			n_jobs = n_threads;
			ftn = module->getFunction(ftnlex);
		}
		else if (3 == m_arguments.size())
		{
			n_threads = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
			if (!n_threads.IsValid()) return TValue::NullInvalid();
			if (!n_threads.IsInteger())
			{
				env->Error(n_threads.GetToken(), "Expected integer number of threads.");
				return TValue::NullInvalid();
			}

			n_jobs = m_arguments[1]->codegen(builder, module, env).GetFromStorage();
			if (!n_jobs.IsValid()) return TValue::NullInvalid();
			if (!n_jobs.IsInteger())
			{
				env->Error(n_threads.GetToken(), "Expected integer number of jobs.");
				return TValue::NullInvalid();
			}

			if (EXPRESSION_VARIABLE != m_arguments[2]->GetType())
			{
				env->Error(n_threads.GetToken(), "Expected function.");
				return TValue::NullInvalid();
			}
			Token* ftntok = static_cast<VariableExpr*>(m_arguments[2])->Operator();
			std::string ftnlex = ftntok->Lexeme();
			if (!env->GetFunction(ftntok, ftnlex).IsValid())
			{
				env->Error(ftntok, "Function not found.");
				return TValue::NullInvalid();
			}
			ftn = module->getFunction(ftnlex);
		}

		builder->CreateCall(module->getFunction("__parfor"), { n_threads.Value(), n_jobs.Value(), ftn }, "calltmp");
		return TValue::NullInvalid();
	}
	
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
			return tval.EmitLen();
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
	}/****
	else if (0 == name.compare("input"))
	{
		llvm::Value* s = builder->CreateCall(module->getFunction("input"), {}, "calltmp");
		return TValue::MakeString(callee, s);
	}	*/
	else
	{
		TFunction tfunc = env->GetFunction(callee, name);
		if (!tfunc.IsValid())
		{
			env->Error(callee, "Invalid function call.");
			return TValue::NullInvalid();
		}

		llvm::Function* ftn = tfunc.GetLLVMFunc();
		std::vector<TType> params = tfunc.GetParamTypes();

		if (tfunc.IsInternal())
		{
			TType ret_type = tfunc.GetInternalReturnType();
			if (ret_type.IsValid()) ret = TValue::Construct(ret_type, "tmp", false);
			
			std::vector<llvm::Value*> args;

			if (!CheckArgSize(params.size())) return TValue::NullInvalid();
						
			for (size_t i = 0; i < m_arguments.size(); ++i)
			{
				TValue v = m_arguments[i]->codegen(builder, module, env).GetFromStorage();

				if (params[i].IsNumeric())
				{
					v = v.CastToMatchImplicit(params[i]);
				}
				else if (v.IsVecDynamic())
				{
					//v.SetValue(builder->CreateLoad(builder->getPtrTy(), v.Value(), "tmp_load"));
				}
				/*else if (v.IsVecFixed())
				{
					v = v.CastFixedToDynVec();
					v.SetValue(builder->CreateLoad(builder->getPtrTy(), v.Value(), "tmp_load"));
				}
				else if (v.IsUDT())
				{
					if (0 != v.GetToken()->Lexeme().compare(params[i].GetToken()->Lexeme()))
					{
						if (!tfunc.IsInternal())
						{
							env->Error(callee, "Parameter type mismatch in function call.");
							return TValue::NullInvalid();
						}
						else
						{
							//v.Value()->print(llvm::errs());
							v.SetValue(builder->CreateGEP(v.GetTy(), v.Value(), { builder->getInt32(0), builder->getInt32(0) }, "tmp_gep"));
							//v.Value()->print(llvm::errs());
						}
					}
				}*/
				args.push_back(v.Value());
			}
			llvm::Value* rval = builder->CreateCall(ftn, args, std::string("call_" + name).c_str());
			ret.SetValue(rval);
			ret.SetStorage(false);

			return ret;
		}
		else
		{
			std::vector<llvm::Value*> args;
			int start_idx = 0;

			TType rettype = tfunc.GetReturnType();
			if (rettype.IsValid())
			{
				llvm::Value* v = CreateEntryAlloca(builder, rettype.GetTy(), nullptr, "call_ret_ref");
				args.push_back(v);

				ret = TValue::Construct(rettype, "temp", false);
				ret.SetValue(v);
				ret.SetStorage(true);
				start_idx = 1;
			}

			for (size_t i = 0; i < m_arguments.size(); ++i)
			{
				// if not storage, put into storage because we're passing as reference
				TValue v = m_arguments[i]->codegen(builder, module, env).GetFromStorage().CastToMatchImplicit(params[i + start_idx]).MakeStorage();
				if (!v.IsValid())
				{
					env->Error(callee, "Error creating parameter reference for function call.");
					return TValue::NullInvalid();
				}

				args.push_back(v.Value());
			}
			/*llvm::Value* rval = builder->CreateCall(ftn, args, std::string("call_" + name).c_str());
			
			if (rettype.IsValid())
				ret = ret.GetFromStorage();
			else
				ret.SetValue(rval);
			
			return ret;*/
			
			builder->CreateCall(ftn, args, std::string("call_" + name).c_str());

			if (rettype.IsValid()) return ret.GetFromStorage();
			return TValue::NullInvalid();
		}
	}
	
	return TValue::NullInvalid();

}

