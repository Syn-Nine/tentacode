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
	
	TValue rhs = m_right->codegen(builder, module, env);
	if (rhs.IsInvalid())
	{
		env->Error(m_token, "Invalid assignment.");
		return TValue::NullInvalid();
	}

	rhs = rhs.GetFromStorage();
	
	std::string var = m_token->Lexeme();

	if (m_vecIndex)
	{
		TValue vidx = m_vecIndex->codegen(builder, module, env);
		if (vidx.IsInvalid())
		{
			env->Error(m_token, "Invalid vector index.");
			return TValue::NullInvalid();
		}

		vidx = vidx.GetFromStorage();
		env->AssignToVariableVectorIndex(m_token, var, vidx, rhs);
	}
	else
	{
		env->AssignToVariable(m_token, var, rhs);
	}

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
	if (lhs.IsInvalid()) return TValue::NullInvalid();
	/*if (EXPRESSION_GET == m_left->GetType())
	{
		llvm::Type* ty = nullptr;
		if (lhs.IsInteger() || lhs.IsEnum()) ty = builder->getInt32Ty();
		else if (lhs.IsDouble()) ty = builder->getDoubleTy();
		else if (lhs.IsBool()) ty = builder->getInt1Ty();
		
		if (ty) lhs = TValue(lhs.type, builder->CreateLoad(ty, lhs.value, "gep_load"));
	}
	*/

	if (TOKEN_AS == m_token->GetType() && EXPRESSION_VARIABLE == m_right->GetType())
	{
		TokenTypeEnum new_type = (static_cast<VariableExpr*>(m_right))->Operator()->GetType();
		return lhs.As(new_type);
	}
	

	TValue rhs = m_right->codegen(builder, module, env);
	if (rhs.IsInvalid()) return TValue::NullInvalid();

	
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
TValue BracketExpr::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("BracketExpr::codegen()\n");
	
	TokenPtrList* args = new TokenPtrList();
	std::vector<TValue> vals;

	bool has_int = false;
	bool has_double = false;
	int max_bits = 0;

	for (Expr* arg : m_arguments)
	{
		if (EXPRESSION_REPLICATE == arg->GetType())
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

					if (val.IsInteger()) has_int = true;
					if (val.IsFloat()) has_double = true;
					max_bits = std::max(val.NumBits(), max_bits);
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
		else
		{
			// get from storage in case it's a variable
			TValue v = arg->codegen(builder, module, env).GetFromStorage();

			vals.push_back(v);
			if (v.IsInteger()) has_int = true;
			if (v.IsFloat()) has_double = true;
			max_bits = std::max(v.NumBits(), max_bits);
		}
	}

	// to do, allow assigning to empty brackets to clear dynamic vectors
	if (vals.empty()) return TValue::NullInvalid();

	TokenTypeEnum vectype;

	if (has_double)
	{
		// convert to max common bitsize
		if (16 == max_bits) max_bits = 32;

		if (32 == max_bits) vectype = TOKEN_VAR_F32;
		else if (64 == max_bits) vectype = TOKEN_VAR_F64;

		// convert everything to double at max common bitsize
		for (auto& v : vals)
		{
			v = v.CastToFloat(max_bits);
		}
	}
	else if (has_int) // has int, but not float
	{
		// intentionally different than double
		if (16 == max_bits) vectype = TOKEN_VAR_I16;
		else if (32 == max_bits) vectype = TOKEN_VAR_I32;
		else if (64 == max_bits) vectype = TOKEN_VAR_I64;

		// convert everything to double at max common bitsize
		for (auto& v : vals)
		{
			v = v.CastToInt(max_bits);
		}
	}
	else
	{
		if (vals[0].IsBool()) vectype = TOKEN_VAR_BOOL;
		else if (vals[0].IsString()) vectype = TOKEN_VAR_STRING;
		else if (vals[0].IsEnum()) vectype = TOKEN_VAR_ENUM;
		else
		{
			env->Error(vals[0].GetToken(), "Type not supported in fixed length vector.");
			return TValue::NullInvalid();
		}
	}

	Token* vec = new Token(TOKEN_VAR_VEC, "<anon>", m_token->Line(), m_token->Filename());
	args->push_back(new Token(vectype, "<anon>", m_token->Line(), m_token->Filename()));
	args->push_back(new Token(TOKEN_INTEGER, "<anon>", vals.size(), vals.size(), m_token->Line(), m_token->Filename()));
	
	return TValue::Construct(vec, args, "<anon>", false, &vals);
}


//-----------------------------------------------------------------------------
TValue GetExpr::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("GetExpr::codegen()\n");
		
	VariableExpr* vs = static_cast<VariableExpr*>(m_object); // parent object
	
	std::string var_name = vs->Operator()->Lexeme();
	std::string mem_name = m_token->Lexeme();

	TValue vidx;
	Expr* temp = vs->VecIndex();
	if (temp) vidx = temp->codegen(builder, module, env).GetFromStorage();

	TValue obj = env->GetVariable(m_token, var_name);
	return obj.GetStructVariable(m_token, vidx.Value(), mem_name);
}



//-----------------------------------------------------------------------------
TValue LogicalExpr::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("LogicalExpr::codegen()\n");
	
	if (!m_left || !m_right) return TValue::NullInvalid();
	TValue lhs = m_left->codegen(builder, module, env).GetFromStorage();
	TValue rhs = m_right->codegen(builder, module, env).GetFromStorage();
	if (!lhs.Value() || !rhs.Value()) return TValue::NullInvalid();

	switch (m_token->GetType())
	{
	case TOKEN_AND:
		return TValue::MakeBool(m_token, builder->CreateAnd(lhs.Value(), rhs.Value(), "logical_and_cmp"));

	case TOKEN_OR:
		return TValue::MakeBool(m_token, builder->CreateOr(lhs.Value(), rhs.Value(), "logical_or_cmp"));

	default:
		break;
	}
	
	return TValue::NullInvalid();
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
	if (lhs.IsInvalid()) return TValue::NullInvalid();

	TValue rhs = m_value->codegen(builder, module, env).GetFromStorage();
	if (rhs.IsInvalid()) return TValue::NullInvalid();

	rhs = rhs.CastToMatchImplicit(lhs);
	if (rhs.IsInvalid()) return TValue::NullInvalid();

	builder->CreateStore(rhs.Value(), lhs.Value());

	return TValue::NullInvalid();
	/*


	VariableExpr* vs = static_cast<VariableExpr*>(m_object); // parent object

	std::string var_name = vs->Operator()->Lexeme();
	TValue var = env->GetVariable(m_token, var_name);

	std::string udt_name = var.GetToken()->Lexeme();
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
	if (rhs.IsInvalid())
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
		if (vidx.IsInvalid())
		{
			env->Error(m_token, "Invalid vector index.");
			return TValue::NullInvalid();
		}
		else if (!ret.IsVecAny())
		{
			env->Error(m_token, "Variable is not a vector.");
			return TValue::NullInvalid();
		}

		// get from storage in case it's a variable
		vidx = vidx.GetFromStorage();

		return ret.GetAtVectorIndex(vidx);
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