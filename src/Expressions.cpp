#include "Expressions.h"


//-----------------------------------------------------------------------------
void copy_udt(llvm::IRBuilder<>* builder,
	llvm::Module* module,
	Environment* env,
	std::string udtname,
	llvm::Value* defval, llvm::Value* rdefval, llvm::Type* defty, llvm::Type* rdefty, std::vector<llvm::Value*> lhs_args, std::vector<llvm::Value*> rhs_args)
{
	/*
	if (!env->GetUdt(udtname)) return;

	std::vector<llvm::Type*> arg_ty = env->GetUdtTy(udtname);
	std::vector<Token*> arg_var_tokens = env->GetUdtMemberTokens(udtname);
	std::vector<std::string> arg_var_names = env->GetUdtMemberNames(udtname);

	for (size_t i = 0; i < arg_ty.size(); ++i)
	{
		std::vector<llvm::Value*> lhs_argtmp = lhs_args;
		std::vector<llvm::Value*> rhs_argtmp = rhs_args;
		lhs_argtmp.push_back(builder->getInt32(i));
		rhs_argtmp.push_back(builder->getInt32(i));

		if (TOKEN_IDENTIFIER == arg_var_tokens[i]->GetType())
		{
			copy_udt(builder, module, env, "struct." + arg_var_tokens[i]->Lexeme(), defval, rdefval, defty, rdefty, lhs_argtmp, rhs_argtmp);
		}
		else
		{
			llvm::Value* lhs_gep = builder->CreateGEP(defty, defval, lhs_argtmp, "lhs_gep_" + arg_var_tokens[i]->Lexeme());
			llvm::Value* rhs_gep = builder->CreateGEP(rdefty, rdefval, rhs_argtmp, "rhs_gep_" + arg_var_tokens[i]->Lexeme());
			if (TOKEN_VAR_I32 == arg_var_tokens[i]->GetType() ||
				TOKEN_VAR_ENUM == arg_var_tokens[i]->GetType()) builder->CreateStore(builder->CreateLoad(builder->getInt32Ty(), rhs_gep, "loadtmp"), lhs_gep);
			else if (TOKEN_VAR_BOOL == arg_var_tokens[i]->GetType()) builder->CreateStore(builder->CreateLoad(builder->getInt1Ty(), rhs_gep, "loadtmp"), lhs_gep);
			else if (TOKEN_VAR_F32 == arg_var_tokens[i]->GetType()) builder->CreateStore(builder->CreateLoad(builder->getDoubleTy(), rhs_gep, "loadtmp"), lhs_gep);
			else if (TOKEN_VAR_STRING == arg_var_tokens[i]->GetType())
			{
				llvm::Value* lhs_load = builder->CreateLoad(builder->getPtrTy(), lhs_gep, "lhs_load");
				llvm::Value* rhs_load = builder->CreateLoad(builder->getPtrTy(), rhs_gep, "rhs_load");
				builder->CreateCall(module->getFunction("__str_assign"), { lhs_load, rhs_load }, "calltmp");
			}
			/*else if (TOKEN_VAR_VEC == arg_var_tokens[i]->GetType())
			{
				llvm::Value* lhs_load = builder->CreateLoad(builder->getPtrTy(), lhs_gep, "lhs_load");
				llvm::Value* rhs_load = builder->CreateLoad(builder->getPtrTy(), rhs_gep, "rhs_load");
				//builder->CreateCall(module->getFunction("__str_assign"), { lhs_load, rhs_load }, "calltmp");
			}*/
	/*
			else
			{
				env->Error(arg_var_tokens[i], "Initializer not present for copy_udt.");
			}
		}
	}*/
}


//-----------------------------------------------------------------------------
TValue AssignExpr::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("AssignExpr::codegen()\n");

	std::string var = m_token->Lexeme();
	//TType ttype_hint = env->GetVariable(m_token, var).GetTType();
	
	TValue rhs = m_right->codegen(builder, module, env);
	if (!rhs.IsValid())
	{
		env->Error(m_token, "Invalid assignment.");
		return TValue::NullInvalid();
	}

	rhs = rhs.GetFromStorage();

	if (m_vecIndex)
	{
		TValue vidx = m_vecIndex->codegen(builder, module, env);
		if (!vidx.IsValid())
		{
			env->Error(m_token, "Invalid vector index.");
			return TValue::NullInvalid();
		}

		vidx = vidx.GetFromStorage();
		env->AssignToVariableIndex(m_token, var, vidx, rhs);
	}
	else
	{
		env->AssignToVariable(m_token, var, rhs);
	}

	return env->GetVariable(m_token, var);
	
	/*
	
	llvm::Value* defval = env->GetVariable(var);
	llvm::Type* defty = env->GetVariableTy(var);
	LiteralTypeEnum varType = env->GetVariableType(var);

	else if (LITERAL_TYPE_UDT == varType)
	{
		if (EXPRESSION_VARIABLE == m_right->GetType())
		{
			VariableExpr* ve = static_cast<VariableExpr*>(m_right);
			std::string rvar = ve->Operator()->Lexeme();
			
			LiteralTypeEnum rvarType = env->GetVariableType(rvar);
			if (LITERAL_TYPE_UDT == rvarType)
			{
				llvm::Value* rdefval = env->GetVariable(rvar);
				llvm::Type* rdefty = env->GetVariableTy(rvar);

				if (defty == rdefty) // same type of data
				{
					std::string udtname = "struct." + env->GetVariableToken(rvar)->Lexeme();
					copy_udt(builder, module, env, udtname, defval, rdefval, defty, rdefty, { builder->getInt32(0) }, { builder->getInt32(0) } );
				}
			}
		}
	}
	*/
	return TValue::NullInvalid();
}


//-----------------------------------------------------------------------------
TValue BinaryExpr::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("BinaryExpr::codegen()\n");
	
	if (!m_left || !m_right) return TValue::NullInvalid();

	
	TValue lhs = m_left->codegen(builder, module, env);
	if (!lhs.IsValid()) return TValue::NullInvalid();
	/*if (EXPRESSION_GET == m_left->GetType())
	{
		llvm::Type* ty = nullptr;
		if (lhs.IsInteger() || lhs.IsEnum()) ty = builder->getInt32Ty();
		else if (lhs.IsDouble()) ty = builder->getDoubleTy();
		else if (lhs.IsBool()) ty = builder->getInt1Ty();
		
		if (ty) lhs = TValue(lhs.type, builder->CreateLoad(ty, lhs.value, "gep_load"));
	}
	*/
	
	// check for type conversion
	if (TOKEN_AS == m_token->GetType() && EXPRESSION_VARIABLE == m_right->GetType())
	{
		TokenTypeEnum new_type = (static_cast<VariableExpr*>(m_right))->Operator()->GetType();
		return lhs.As(new_type);
	}

	TValue rhs = m_right->codegen(builder, module, env);
	if (!rhs.IsValid()) return TValue::NullInvalid();

	// check for pair syntax
	if (TOKEN_COLON == m_token->GetType())
	{
		return TValue::Construct_Pair(lhs, rhs);
	}

	// upcase to double if one side is int and the other is float, or we are dividing
	if (lhs.IsInteger() && (rhs.IsFloat() || m_token->GetType() == TOKEN_SLASH))
	{
		lhs = lhs.GetFromStorage().CastToFloat(64);
	}
	if (rhs.IsInteger() && (lhs.IsFloat() || m_token->GetType() == TOKEN_SLASH))
	{
		rhs = rhs.GetFromStorage().CastToFloat(64);
	}


	// perform binary operation
	switch (m_token->GetType())
	{
	case TOKEN_PLUS:    return lhs.Add(rhs);
	case TOKEN_MINUS:   return lhs.Subtract(rhs);
	case TOKEN_STAR:    return lhs.Multiply(rhs);
	case TOKEN_SLASH:   return lhs.Divide(rhs);
	case TOKEN_PERCENT: return lhs.Modulus(rhs);
	case TOKEN_HAT:		return lhs.Power(rhs);

	//-------------------------------------------------------------------------//
	case TOKEN_GREATER:       return lhs.IsGreaterThan(rhs, false);
	case TOKEN_GREATER_EQUAL: return lhs.IsGreaterThan(rhs, true);
	case TOKEN_LESS:          return lhs.IsLessThan(rhs, false);
	case TOKEN_LESS_EQUAL:    return lhs.IsLessThan(rhs, true);
	case TOKEN_BANG_EQUAL:    return lhs.IsNotEqual(rhs);
	case TOKEN_EQUAL_EQUAL:   return lhs.IsEqual(rhs);
	//-------------------------------------------------------------------------//
	default:
		env->Error(m_token, "Invalid binary operation.");
	}

	/*
	if (EXPRESSION_GET == m_right->GetType())
	{
		llvm::Type* ty = nullptr;
		if (rhs.IsInteger() || rhs.IsEnum()) ty = builder->getInt32Ty();
		else if (rhs.IsDouble()) ty = builder->getDoubleTy();
		else if (rhs.IsBool()) ty = builder->getInt1Ty();
		
		if (ty) rhs = TValue(rhs.type, builder->CreateLoad(ty, rhs.value, "gep_load"));
	}
	
	*/
	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue BraceExpr::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)//, TType ttype_hint)
{
	if (2 <= Environment::GetDebugLevel()) printf("BraceExpr::codegen()\n");

	TValue::TValueList vals;

	for (Expr* arg : m_arguments)
	{
		// todo - go back and make replicate syntax work
		/*if (EXPRESSION_REPLICATE == arg->GetType())
		{
			ReplicateExpr* re = static_cast<ReplicateExpr*>(arg);
			Expr* lhs = re->Left();
			Expr* rhs = re->Right();

			if (EXPRESSION_LITERAL == rhs->GetType())
			{
				LiteralExpr* le = static_cast<LiteralExpr*>(rhs);
				if (LITERAL_TYPE_INTEGER == le->GetLiteral().GetType())
				{
					// inclue get from storage in case of storage type
					TValue val = lhs->codegen(builder, module, env).GetFromStorage();
					int qty = le->GetLiteral().IntValue();
					for (size_t i = 0; i < qty; ++i)
					{
						vals.push_back(val);
					}
				}
				else
				{
					env->Error(re->Operator(), "Expected integer for replicate size.");
					return TValue::NullInvalid();
				}
			}
			else
			{
				env->Error(re->Operator(), "Expected literal for replicate size.");
				return TValue::NullInvalid();
			}
		}
		else*/
		{
			// get from storage in case it's a variable
			TValue v = arg->codegen(builder, module, env).GetFromStorage();
			if (!v.IsValid()) return TValue::NullInvalid();
			vals.push_back(v);
		}
	}

	// to do, allow assigning to empty braces
	if (vals.empty()) return TValue::NullInvalid();

	return TValue::Construct_Brace(m_token, &vals);
}


//-----------------------------------------------------------------------------
TValue DestructExpr::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("DestructExpr::codegen()\n");

	std::string var = m_token->Lexeme();

	if (!m_left) return TValue::NullInvalid();
	if (!m_right) return TValue::NullInvalid();

	if (EXPRESSION_COLLECT != m_left->GetType()) return TValue::NullInvalid();

	CollectExpr* ce = static_cast<CollectExpr*>(m_left);
	ArgList lhs = ce->GetArguments();

	TValue rhs;
	if (EXPRESSION_COLLECT == m_right->GetType())
	{
		TValue::TValueList vals;

		ArgList args = static_cast<CollectExpr*>(m_right)->GetArguments();

		for (Expr* arg : args)
		{
			// get from storage in case it's a variable
			TValue v = arg->codegen(builder, module, env).GetFromStorage();
			if (!v.IsValid()) return TValue::NullInvalid();
			vals.push_back(v);
		}

		if (vals.empty()) return TValue::NullInvalid();

		rhs = TValue::Construct_Brace(m_token, &vals);
	}
	else
	{
		rhs = m_right->codegen(builder, module, env);
	}

	if (!rhs.IsBrace())
	{
		env->Error(rhs.GetToken(), "Expected brace or collect expression.");
		return TValue::NullInvalid();
	}

	// create rhs values
	TValue::TValueList rvals = rhs.GetBraceArgs();

	if (lhs.size() != rvals.size())
	{
		env->Error(rhs.GetToken(), "Assignment count mismatch in destructure.");
		return TValue::NullInvalid();
	}
	
	// assign to left side
	for (size_t i = 0; i < lhs.size(); ++i)
	{
		if (EXPRESSION_VARIABLE != lhs[i]->GetType())
		{
			env->Error(m_token, "Expected variable in left side collection.");
			return TValue::NullInvalid();
		}
		
		VariableExpr* ve = static_cast<VariableExpr*>(lhs[i]);
		std::string var = ve->Operator()->Lexeme();

		TValue lval = ve->codegen(builder, module, env);
		if (!lval.IsValid()) return TValue::NullInvalid();

		Expr* idx_expr = ve->VecIndex();
		if (idx_expr)
		{
			TValue vidx = idx_expr->codegen(builder, module, env);
			if (!vidx.IsValid())
			{
				env->Error(ve->Operator(), "Invalid vector index.");
				return TValue::NullInvalid();
			}
			vidx = vidx.GetFromStorage();
			env->AssignToVariableIndex(m_token, var, vidx, rvals[i]);
		}
		else
		{
			env->AssignToVariable(m_token, var, rvals[i]);
		}

	}

	return TValue::NullInvalid();
}


//-----------------------------------------------------------------------------
TValue FunctorExpr::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("FunctorExpr::codegen()\n");

	TFunction func = env->GetAnonFunction(m_sig);
	if (!func.IsValid())
	{
		env->Error(m_token, "Failed to get anonymous prototype.");
		return TValue::NullInvalid();
	}

	TType type = TType::Construct_Functor(m_token);

	TValue val = TValue::Construct_Functor(type, m_sig, func.GetLLVMFunc());
	if (val.IsValid()) return val;

	//return TValue::Construct_Functor(TType::Construct_Functor(m_token, nullptr));
	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue GetExpr::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("GetExpr::codegen()\n");

	VariableExpr* vs = static_cast<VariableExpr*>(m_object); // parent object

	Token* struct_token = vs->Operator();
	std::string var_name = struct_token->Lexeme();
	std::string mem_name = m_token->Lexeme();

	TValue obj = env->GetVariable(struct_token, var_name);
	if (!obj.IsValid()) return TValue::NullInvalid();

	Expr* e0 = vs->VecIndex();
	if (e0)
	{
		TValue idx0 = e0->codegen(builder, module, env).GetFromStorage();
		if (!idx0.IsValid())
		{
			env->Error(m_token, "Invalid index.");
			return TValue::NullInvalid();
		}
		obj = obj.GetAtIndex(idx0);
	}

	TValue idx1;
	if (m_right) idx1 = m_right->codegen(builder, module, env).GetFromStorage();
	obj = obj.GetStructVariable(m_token, idx1.Value(), mem_name);

	return obj;
}


//-----------------------------------------------------------------------------
TValue LogicalExpr::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("LogicalExpr::codegen()\n");
	if (!m_left || !m_right) return TValue::NullInvalid();

	// always evaluate lhs
	TValue lhs = m_left->codegen(builder, module, env).GetFromStorage();
	if (!lhs.Value()) return TValue::NullInvalid();

	llvm::Function* ftn = builder->GetInsertBlock()->getParent();
	llvm::LLVMContext& context = builder->getContext();
	llvm::BasicBlock* rhsBB = llvm::BasicBlock::Create(context, "rhs", ftn);
	llvm::BasicBlock* shortBB = llvm::BasicBlock::Create(context, "short");

	llvm::Value* ret = builder->CreateAlloca(builder->getInt1Ty(), nullptr, "tmp");

	if (TOKEN_AND == m_token->GetType())
	{
		// if lhs is already false, short circuit false
		builder->CreateStore(builder->getFalse(), ret);
		builder->CreateCondBr(lhs.Value(), rhsBB, shortBB);
	}
	else if (TOKEN_OR == m_token->GetType())
	{
		// if lhs is already true, short circuit true
		builder->CreateStore(builder->getTrue(), ret);
		builder->CreateCondBr(builder->CreateNot(lhs.Value(), "nottmp"), rhsBB, shortBB);
	}
	else
	{
		// invalid token
		return TValue::NullInvalid();
	}

	// only evaluate rhs if not short circuiting
	builder->SetInsertPoint(rhsBB);
	TValue rhs = m_right->codegen(builder, module, env).GetFromStorage();
	if (!rhs.Value()) return TValue::NullInvalid();

	// store rhs value
	builder->CreateStore(rhs.Value(), ret);
	builder->CreateBr(shortBB);

	// short circuit merge
	ftn->insert(ftn->end(), shortBB);
	builder->SetInsertPoint(shortBB);

	return TValue::MakeBool(m_token, builder->CreateLoad(builder->getInt1Ty(), ret));
}



//-----------------------------------------------------------------------------
TValue RangeExpr::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("RangeExpr::codegen()\n");
	
	if (!m_left || !m_right) return TValue::NullInvalid();
	TValue lhs = m_left->codegen(builder, module, env).GetFromStorage();
	TValue rhs = m_right->codegen(builder, module, env).GetFromStorage();
	if (!lhs.Value() || !rhs.Value()) return TValue::NullInvalid();
	
	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue SetExpr::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("SetExpr::codegen()\n");
	
	TValue lhs = m_object->codegen(builder, module, env);
	if (!lhs.IsValid()) return TValue::NullInvalid();

	TValue rhs = m_value->codegen(builder, module, env).GetFromStorage();
	if (!rhs.IsValid()) return TValue::NullInvalid();

	rhs = rhs.CastToMatchImplicit(lhs);
	if (!rhs.IsValid()) return TValue::NullInvalid();

	if (!lhs.IsUDT())
	{
		if (lhs.IsString()) // deep copy strings
		{
			lhs = lhs.GetFromStorage();
			builder->CreateCall(module->getFunction("__str_assign"), { lhs.Value(), rhs.Value() }, "calltmp");
		}
		else
		{
			builder->CreateStore(rhs.Value(), lhs.Value());
		}
	}

	return TValue::NullInvalid();

	/*


	VariableExpr* vs = static_cast<VariableExpr*>(m_object); // parent object

	std::string var_name = vs->Operator()->Lexeme();
	TValue var = env->GetVariable(m_token, var_name);

	std::string udt_name = var.GetLexeme();
	TStruct struc = env->GetStruct(m_token, udt_name);
	if (!struc.IsValid()) return TValue::NullInvalid();

	std::string mem_name = m_token->Lexeme();
	TValue ret = struc.GetMember(m_token, mem_name);
	if (ret.IsInvalid()) return TValue::NullInvalid();

	llvm::Value* gep_loc = struc.GetGEPLoc(m_token, mem_name);
	if (!gep_loc) return TValue::NullInvalid();

	llvm::Value* gep = builder->CreateGEP(ret.GetTy(), var.Value(), gep_loc, mem_name);
	ret.SetValue(builder->CreateLoad(ret.GetTy(), gep, "load_tmp"));
	*/


	/*
	TValue lhs = m_object->codegen(builder, module, env);
	
	TValue rhs = m_value->codegen(builder, module, env);
	if (EXPRESSION_GET == m_value->GetType())
	{
		llvm::Type* ty = nullptr;
		if (rhs.IsInteger() || rhs.IsEnum()) ty = builder->getInt32Ty();
		else if (rhs.IsDouble()) ty = builder->getDoubleTy();
		else if (rhs.IsBool()) ty = builder->getInt1Ty();
		
		if (ty) rhs = TValue(rhs.type, builder->CreateLoad(ty, rhs.value, "gep_load"));
	}


	if (lhs.IsInteger() && rhs.IsDouble())
	{
		// convert rhs to int
		rhs = TValue::Integer(builder->CreateFPToSI(rhs.value, builder->getInt32Ty(), "int_cast_tmp"));
	}
	else if (lhs.IsDouble() && rhs.IsInteger())
	{
		// convert rhs to double
		rhs = TValue::Double(builder->CreateSIToFP(rhs.value, builder->getDoubleTy(), "cast_to_dbl"));
	}
	
	
	if (lhs.type == rhs.type)
	{
		if (LITERAL_TYPE_UDT != lhs.type)
		{
			if (lhs.IsString())
			{
				llvm::Value* lhs_load = builder->CreateLoad(builder->getPtrTy(), lhs.value, "lhs_load");
				llvm::Value* rhs_load = builder->CreateLoad(builder->getPtrTy(), rhs.value, "rhs_load");
				builder->CreateCall(module->getFunction("__str_assign"), { lhs_load, rhs_load }, "calltmp");
			}
			else if (lhs.IsVec())
			{
				llvm::Value* dstType = builder->getInt32(lhs.fixed_vec_type);
				llvm::Value* srcType = builder->getInt32(rhs.fixed_vec_type);
				llvm::Value* srcQty = builder->getInt32(rhs.fixed_vec_sz);
				llvm::Value* dstPtr = builder->CreateLoad(builder->getPtrTy(), lhs.value, "lhs_load");
				llvm::Value* srcPtr = builder->CreateLoad(builder->getPtrTy(), rhs.value, "lhs_load");
				builder->CreateCall(module->getFunction("__vec_assign"), { dstType, dstPtr, srcType, srcPtr, srcQty }, "calltmp");
			}
			else
			{
				builder->CreateStore(rhs.value, lhs.value);
			}
		}
		else
		{
			llvm::Value* defval = lhs.value;
			llvm::Type* defty = lhs.udt_ty;
			std::vector<llvm::Value*> lhs_args = lhs.udt_args;

			if (EXPRESSION_VARIABLE == m_value->GetType())
			{
				VariableExpr* ve = static_cast<VariableExpr*>(m_value);
				std::string rvar = ve->Operator()->Lexeme();

				LiteralTypeEnum rvarType = env->GetVariableType(rvar);
				if (LITERAL_TYPE_UDT == rvarType)
				{
					llvm::Value* rdefval = env->GetVariable(rvar);
					llvm::Type* rdefty = env->GetVariableTy(rvar);

					std::string udtname = "struct." + env->GetVariableToken(rvar)->Lexeme();
					if (0 == lhs.udt_name.compare(udtname))
					{
						// same type of data
						copy_udt(builder, module, env, udtname, defval, rdefval, defty, rdefty, lhs_args, { builder->getInt32(0) });
					}
				}
			}

		}
	}
	else if (lhs.IsVec() && rhs.IsFixedVec())
	{
		llvm::Value* dstType = builder->getInt32(lhs.fixed_vec_type);
		llvm::Value* srcType = builder->getInt32(rhs.fixed_vec_type);
		llvm::Value* srcQty = builder->getInt32(rhs.fixed_vec_sz);
		llvm::Value* dstPtr = builder->CreateLoad(builder->getPtrTy(), lhs.value, "lhs_load");
		llvm::Value* srcPtr = rhs.value; // no load required, address already loaded in local variable
		builder->CreateCall(module->getFunction("__vec_assign_fixed"), { dstType, dstPtr, srcType, srcPtr, srcQty }, "calltmp");
	}
	else
	{
		env->Error(static_cast<GetExpr*>(m_object)->Operator(), "Type mismatch in SetExpr\n");
	}
	
	*/
	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue UnaryExpr::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("UnaryExpr::codegen()\n");
	
	TValue rhs = m_right->codegen(builder, module, env);
	if (!rhs.IsValid())
	{
		env->Error(m_token, "Unable to perform unary operation.");
		return TValue::NullInvalid();
	}
	
	switch (m_token->GetType())
	{
	case TOKEN_MINUS:
		return rhs.Negate();
	
	case TOKEN_BANG:
		return rhs.Not();
	
	default:
		break;
	}
	
	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue VariableExpr::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("VariableExpr::codegen()\n");
	
	// returns the value stored at a TValue memory location
	TValue ret = env->GetVariable(m_token, m_token->Lexeme());
	if (m_vecIndex)
	{
		TValue vidx = m_vecIndex->codegen(builder, module, env);
		if (!vidx.IsValid())
		{
			env->Error(m_token, "Invalid index.");
			return TValue::NullInvalid();
		}
		else if (!ret.CanIndex())
		{
			env->Error(m_token, "Variable cannot be indexed.");
			return TValue::NullInvalid();
		}

		// get index from storage in case it's a variable
		vidx = vidx.GetFromStorage();

		return ret.GetAtIndex(vidx);
	}

	return ret;


	/*
	
	else if (LITERAL_TYPE_UDT == varType)
	{
		std::string udt_name = "struct." + env->GetVariableToken(var)->Lexeme();
		std::vector<llvm::Value*> args(1, builder->getInt32(0));
		return TValue::UDT(udt_name, defty, defval, args);
	}
	else
	{
		return TValue(varType, builder->CreateLoad(defty, defval, "loadtmp"));
	}
	*/

}