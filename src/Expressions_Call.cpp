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

	Token* callee = nullptr;
	TValue lhs_object;

	ExpressionTypeEnum callee_type = m_callee->GetType();

	if (EXPRESSION_VARIABLE == callee_type)
	{
		callee = static_cast<VariableExpr*>(m_callee)->Operator();
	}
	else if (EXPRESSION_GET == callee_type)
	{
		GetExpr* ge = static_cast<GetExpr*>(m_callee);
		callee = ge->Operator();
		lhs_object = ge->Object()->codegen(builder, module, env);
		if (!lhs_object.IsValid())
		{
			Environment::Error(lhs_object.GetToken(), "Invalid reference variable.");
			return TValue::NullInvalid();
		}
	}
	else
	{
		Environment::Error(nullptr, "Invalid call expression.");
		return TValue::NullInvalid();
	}

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

		TValue v = m_arguments[0]->codegen(builder, module, env);
		return v.EmitLen();
	}
	else if (0 == name.compare("console_input"))
	{
		llvm::Value* s = builder->CreateCall(module->getFunction("console_input"), {}, "calltmp");
		return TValue::MakeString(callee, s);
	}
	else
	{
		TFunction tfunc = env->GetFunction(callee, name, true); // call quietly
		llvm::FunctionType* ft;
		llvm::Value* ftn;
		if (tfunc.IsValid())
		{
			//builder->CreateStore(tfunc.GetLLVMFunc(), ftn);
			ftn = tfunc.GetLLVMFunc();
			ft = tfunc.GetLLVMFuncType();
		}
		else
		{
			// function not found, is this a functor?
			TValue functor = env->GetVariable(callee, name, true); // call quietly
			if (!functor.IsValid())
			{
				env->Error(callee, "Function not found in environment.");
				return TValue::NullInvalid();
			}

			// get the function for this functor
			tfunc = env->GetAnonFunction(functor.GetFunctorSig());
			if (!tfunc.IsValid())
			{
				env->Error(callee, "Anonymous function not found in environment.");
				return TValue::NullInvalid();
			}

			ftn = CreateEntryAlloca(builder, builder->getPtrTy(), nullptr, "ftn_ptr");

			// store the llvm function pointer that's in functor over into ftn
			TValue temp = functor.GetFromStorage();
			builder->CreateStore(temp.Value(), ftn);
			
			//ftn = builder->CreateLoad(builder->getPtrTy(), functor.Value());
			ftn = builder->CreateLoad(builder->getPtrTy(), ftn);

			ft = tfunc.GetLLVMFuncType();

		}
		
		//return TValue::NullInvalid();
		//llvm::Value* ftn_load = builder->CreateLoad(builder->getPtrTy(), ftn);
		//ftn = builder->CreateLoad(builder->getPtrTy(), ftn);

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
					if (0 != v.GetLexeme().compare(params[i].GetLexeme()))
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
			//llvm::Value* rval = builder->CreateCall(ft, ftn, args, std::string("call_" + name).c_str());
			llvm::Value* rval = builder->CreateCall(static_cast<llvm::Function*>(ftn), args, std::string("call_" + name).c_str());
			ret.SetValue(rval);
			ret.SetStorage(false);

			return ret;
		}
		else
		{
			std::vector<llvm::Value*> args;
			int start_idx = 0;
			int lhs_object_offset = 0;
			
			TType rettype = tfunc.GetReturnType();
			if (rettype.IsValid())
			{
				ret = TValue::Construct(rettype, "ret_ref", false);
				if (!ret.IsStorage() && !ret.IsVecFixed() && !ret.IsTuple() && !ret.IsUDT()) ret = ret.MakeStorage();
				args.push_back(ret.Value());
				start_idx += 1;
			}

			if (lhs_object.IsValid())
			{
				TType param_type = params[start_idx];
				TValue v = lhs_object;

				// first parameter must matched type of the lhs object
				if (!v.IsTypeMatched(param_type))
				{
					env->Error(v.GetToken(), "Type mismatch with function first parameter.");
					return TValue::NullInvalid();
				}

				// is parameter on a register?
				if (!v.IsStorage() && !v.IsVecFixed() && !v.IsTuple() && !v.IsUDT())
				{
					// put into storage because we're passing as reference
					if (!v.IsTypeMatched(param_type)) v = v.CastToMatchImplicit(param_type);
					v = v.MakeStorage();
				}
				
				if (!v.IsValid())
				{
					env->Error(callee, "Error creating parameter reference for function call.");
					return TValue::NullInvalid();
				}

				args.push_back(v.Value());
				start_idx += 1;
				lhs_object_offset = 1;
			}

			std::vector<TType> params = tfunc.GetParamTypes();
			size_t num_defaults = tfunc.GetNumDefaults();
			if (0 == num_defaults)
			{
				if (!CheckArgSize(params.size() - start_idx)) return TValue::NullInvalid();
			}
			else
			{
				if (params.size() - start_idx > m_arguments.size() + num_defaults)
				{
					Token* callee = (static_cast<VariableExpr*>(m_callee))->Operator();
					Environment::Error(callee, "Argument count mismatch.");
					return TValue::NullInvalid();
				}
			}

			// container for temporary values passed in as function arguments
			std::vector<TValue> param_vals;
			std::vector<TValue> param_storage;
			std::vector<Expr*> defaults = tfunc.GetDefaults();

			for (size_t i = 0; i < params.size() - start_idx; ++i)
			{
				TType param_type = params[i + start_idx];

				TValue v;
				if (m_arguments.size() > i)
				{
					v = m_arguments[i]->codegen(builder, module, env);
				}
				else
				{
					v = defaults[i + lhs_object_offset]->codegen(builder, module, env);
				}
				
				// is parameter on a register?
				if (!v.IsStorage() && !v.IsVecFixed() && !v.IsTuple() && !v.IsUDT())
				{
					// put into storage because we're passing as reference
					if (!v.IsTypeMatched(param_type)) v = v.CastToMatchImplicit(param_type);
					v = v.MakeStorage();
				}
				else
				{ // parameter value is in storage
					
					// is parameter immutable?
					if (param_type.IsConstant())
					{
						// do we need to cast the value before sending it?
						if (!v.IsTypeMatched(param_type))
						{
							v = v.GetFromStorage().CastToMatchImplicit(param_type).MakeStorage();
						}
					}
					else
					{
						// parameter is mutable so types must be matched
						if (!v.IsTypeMatched(param_type))
						{
							env->Error(v.GetToken(), "Type mismatch for mutable parameter.");
							return TValue::NullInvalid();
						}
					}
				}
				
				if (!v.IsValid())
				{
					env->Error(callee, "Error creating parameter reference for function call.");
					return TValue::NullInvalid();
				}

				args.push_back(v.Value());
			}
			
			builder->CreateCall(ft, ftn, args, std::string("call_" + name).c_str());

			if (lhs_object.IsValid()) return lhs_object;
			if (rettype.IsValid()) return ret.GetFromStorage();
			return TValue::NullInvalid();
		}
	}
	
	return TValue::NullInvalid();

}

