#include "Expressions.h"
#include <deque>

#define PI 3.1415926535897932384626433832795028841971693993751058
#define RAD2DEG (180.0 / PI)
#define DEG2RAD (1 / RAD2DEG)


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
		env->AssignToVariableVectorIndex(var, vidx, rhs);
	}
	else
	{
		env->AssignToVariable(var, rhs);
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
TValue CallExpr::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("CallExpr::codegen()\n");
	
	if (!m_callee) return TValue::NullInvalid();
	if (EXPRESSION_VARIABLE != m_callee->GetType()) return TValue::NullInvalid();

	
	Token* callee = (static_cast<VariableExpr*>(m_callee))->Operator();
	std::string name = callee->Lexeme();

	if (0 == name.compare("abs") && 1 == m_arguments.size())
	{
		TValue v = m_arguments[0]->codegen(builder, module, env);
		if (v.IsInvalid()) return TValue::NullInvalid();
		v = v.GetFromStorage();
		if (v.IsInteger())
		{
			v = v.CastToInt(64);
			return TValue::MakeInt64(callee, builder->CreateCall(module->getFunction("abs"), { v.Value() }, "calltmp"));
		}
		else if (v.IsFloat())
		{
			v = v.CastToFloat(64);
			return TValue::MakeFloat64(callee, builder->CreateCall(module->getFunction("fabs"), { v.Value() }, "calltmp"));
		}
		else
		{
			env->Error(callee, "Invalid parameter type.");
		}
	}
	else if (0 == name.compare("deg2rad") && 1 == m_arguments.size())
	{
		TValue v = m_arguments[0]->codegen(builder, module, env);
		if (v.IsInvalid()) return TValue::NullInvalid();
		v = v.GetFromStorage().CastToFloat(64);
		llvm::Value* tmp = llvm::ConstantFP::get(builder->getContext(), llvm::APFloat(DEG2RAD));
		return TValue::MakeFloat64(callee, builder->CreateFMul(v.Value(), tmp));
	}
	else if (0 == name.compare("rad2deg") && 1 == m_arguments.size())
	{
		TValue v = m_arguments[0]->codegen(builder, module, env);
		if (v.IsInvalid()) return TValue::NullInvalid();
		v = v.GetFromStorage().CastToFloat(64);
		llvm::Value* tmp = llvm::ConstantFP::get(builder->getContext(), llvm::APFloat(RAD2DEG));
		return TValue::MakeFloat64(callee, builder->CreateFMul(v.Value(), tmp));
	}
	else if (1 == m_arguments.size() &&
		0 == name.compare("cos") ||
		0 == name.compare("sin") ||
		0 == name.compare("tan") ||
		0 == name.compare("acos") ||
		0 == name.compare("asin") ||
		0 == name.compare("atan") ||
		0 == name.compare("floor") ||
		0 == name.compare("sqrt"))
	{
		TValue v = m_arguments[0]->codegen(builder, module, env);
		if (v.IsInvalid()) return TValue::NullInvalid();
		v = v.GetFromStorage().CastToFloat(64);
		return TValue::MakeFloat64(callee, builder->CreateCall(module->getFunction(name), { v.Value()}, "calltmp"));
	}
	else if (0 == name.compare("clock"))
	{
		if (m_arguments.empty())
		{
			return TValue::MakeFloat64(callee, builder->CreateCall(module->getFunction("__clock_impl"), {}, "calltmp"));
		}
		else
		{
			env->Error(callee, "Argument count mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("rand"))
	{
		if (m_arguments.empty())
		{
			return TValue::MakeFloat64(callee, builder->CreateCall(module->getFunction("__rand_impl"), {}, "calltmp"));
		}
		else if (1 == m_arguments.size() && EXPRESSION_RANGE == m_arguments[0]->GetType())
		{
			RangeExpr* re = static_cast<RangeExpr*>(m_arguments[0]);
			TValue lhs = re->Left()->codegen(builder, module, env).CastToInt(64);
			TValue rhs = re->Right()->codegen(builder, module, env).CastToInt(64);
			return TValue::MakeInt64(callee, builder->CreateCall(module->getFunction("__rand_range_impl"), { lhs.Value(), rhs.Value()}, "calltmp"));
		}
	}
	else if (0 == name.compare("len"))
	{
		if (1 == m_arguments.size())
		{
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
		else
		{
			env->Error(callee, "Argument count mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("vec::append"))
	{
		if (2 == m_arguments.size())
		{
			if (EXPRESSION_VARIABLE == m_arguments[0]->GetType())
			{
				VariableExpr* ve = static_cast<VariableExpr*>(m_arguments[0]);
				std::string var = ve->Operator()->Lexeme();
				TValue tval = env->GetVariable(callee, var);
				TValue rhs = m_arguments[1]->codegen(builder, module, env);
				if (tval.IsVecDynamic())
				{
					tval.EmitAppend(rhs);
				}
				else
				{
					env->Error(callee, "Argument type mismatch.");
				}
				return TValue::NullInvalid();
			}
		}
		else
		{
			env->Error(callee, "Argument count mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("file::readlines"))
	{
		if (1 == m_arguments.size())
		{
			TValue arg = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
			if (arg.IsString())
			{
				llvm::Value* a = CreateEntryAlloca(builder, builder->getPtrTy(), nullptr, "alloctmp");
				llvm::Value* v = builder->CreateCall(module->getFunction("__file_readlines"), { arg.Value() }, "calltmp");
				builder->CreateStore(v, a);
				TValue ret = TValue::ConstructExplicit(
					callee,                   // Token * token,
					LITERAL_TYPE_VEC_DYNAMIC, // LiteralTypeEnum type,
					a,                        // llvm::Value * value,
					builder->getPtrTy(),      // llvm::Type * ty,
					64,                       // int bits,
					false,                    // bool is_storage,
					0,                        // int fixed_vec_len,
					LITERAL_TYPE_STRING       // LiteralTypeEnum vec_type
					);
				env->AddToCleanup(ret);
				return ret;
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
	else if (0 == name.compare("file::readstring"))
	{
		if (1 == m_arguments.size())
		{
			TValue arg = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
			if (arg.IsString())
			{
				llvm::Value* a = CreateEntryAlloca(builder, builder->getPtrTy(), nullptr, "alloctmp");
				llvm::Value* s = builder->CreateCall(module->getFunction("__file_readstring"), { arg.Value() }, "calltmp");
				builder->CreateStore(s, a);
				TValue ret = TValue::ConstructExplicit(
					callee,                   // Token * token,
					LITERAL_TYPE_STRING,      // LiteralTypeEnum type,
					a,                        // llvm::Value * value,
					builder->getPtrTy(),      // llvm::Type * ty,
					64,                       // int bits,
					true,                     // bool is_storage,
					0,                        // int fixed_vec_len,
					LITERAL_TYPE_INVALID      // LiteralTypeEnum vec_type
				);
				env->AddToCleanup(ret);
				return ret;
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
	else if (0 == name.compare("file::writelines"))
	{
		if (2 == m_arguments.size())
		{
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
					llvm::Value* len = builder->getInt64(rhs.FixedVecLen());
					builder->CreateCall(module->getFunction("__file_writelines_fixed_vec"), { lhs.Value(), gep, len }, "calltmp");
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
	else if (0 == name.compare("file::writestring"))
	{
		if (2 == m_arguments.size())
		{
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
		else
		{
			env->Error(callee, "Argument count mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("str::contains"))
	{
		if (2 == m_arguments.size())
		{
			TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
			TValue rhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage();
			if (lhs.IsString() && rhs.IsString())
			{
				llvm::Value* b = builder->CreateCall(module->getFunction("__str_contains"), { lhs.Value(), rhs.Value() }, "calltmp");
				TValue ret = TValue::ConstructExplicit(
					callee,                   // Token * token,
					LITERAL_TYPE_BOOL,        // LiteralTypeEnum type,
					b,                        // llvm::Value * value,
					builder->getInt1Ty(),     // llvm::Type * ty,
					1,                        // int bits,
					false,                    // bool is_storage,
					0,                        // int fixed_vec_len,
					LITERAL_TYPE_INVALID      // LiteralTypeEnum vec_type
				);
				return ret;
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
	else if (0 == name.compare("str::replace"))
	{
		if (3 == m_arguments.size())
		{
			TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
			TValue mhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage();
			TValue rhs = m_arguments[2]->codegen(builder, module, env).GetFromStorage();
			if (lhs.IsString() && mhs.IsString() && rhs.IsString())
			{
				llvm::Value* s = builder->CreateCall(module->getFunction("__str_replace"), { lhs.Value(), mhs.Value(), rhs.Value() }, "calltmp");
				llvm::Value* a = CreateEntryAlloca(builder, builder->getPtrTy(), nullptr, "alloctmp");
				builder->CreateStore(s, a);
				TValue ret = TValue::ConstructExplicit(
					callee,                   // Token * token,
					LITERAL_TYPE_STRING,      // LiteralTypeEnum type,
					a,                        // llvm::Value * value,
					builder->getPtrTy(),      // llvm::Type * ty,
					64,                       // int bits,
					true,                     // bool is_storage,
					0,                        // int fixed_vec_len,
					LITERAL_TYPE_INVALID      // LiteralTypeEnum vec_type
				);
				env->AddToCleanup(ret);
				return ret;
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
	else if (0 == name.compare("str::split"))
	{
		if (2 == m_arguments.size())
		{
			TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
			TValue rhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage();
			if (lhs.IsString() && rhs.IsString())
			{
				llvm::Value* s = builder->CreateCall(module->getFunction("__str_split"), { lhs.Value(), rhs.Value() }, "calltmp");
				llvm::Value* a = CreateEntryAlloca(builder, builder->getPtrTy(), nullptr, "alloctmp");
				builder->CreateStore(s, a);
				TValue ret = TValue::ConstructExplicit(
					callee,                   // Token * token,
					LITERAL_TYPE_VEC_DYNAMIC, // LiteralTypeEnum type,
					a,                        // llvm::Value * value,
					builder->getPtrTy(),      // llvm::Type * ty,
					64,                       // int bits,
					false,                    // bool is_storage,
					0,                        // int fixed_vec_len,
					LITERAL_TYPE_STRING       // LiteralTypeEnum vec_type
				);
				env->AddToCleanup(ret);
				return ret;
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
	else if (0 == name.compare("str::join"))
	{
		if (2 == m_arguments.size())
		{
			TValue lhs = m_arguments[0]->codegen(builder, module, env);
			TValue rhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage();
			if (lhs.IsVecAny() && LITERAL_TYPE_STRING == lhs.GetVecType() && rhs.IsString())
			{
				llvm::Value* s = nullptr;
				if (lhs.IsVecDynamic())
				{
					llvm::Value* lhs_load = builder->CreateLoad(builder->getPtrTy(), lhs.Value(), "loadtmp");
					s = builder->CreateCall(module->getFunction("__str_join_dyn_vec"), { lhs_load, rhs.Value() }, "calltmp");
				}
				else
				{
					llvm::Value* gep = builder->CreateGEP(builder->getPtrTy(), lhs.Value(), builder->getInt32(0), "geptmp");
					llvm::Value* len = builder->getInt64(lhs.FixedVecLen());
					s = builder->CreateCall(module->getFunction("__str_join_fixed_vec"), { gep, rhs.Value(), len }, "calltmp");
				}

				if (s)
				{
					llvm::Value* a = CreateEntryAlloca(builder, builder->getPtrTy(), nullptr, "alloctmp");
					builder->CreateStore(s, a);
					TValue ret = TValue::ConstructExplicit(
						callee,                   // Token * token,
						LITERAL_TYPE_STRING,      // LiteralTypeEnum type,
						a,                        // llvm::Value * value,
						builder->getPtrTy(),      // llvm::Type * ty,
						64,                       // int bits,
						true,                     // bool is_storage,
						0,                        // int fixed_vec_len,
						LITERAL_TYPE_INVALID      // LiteralTypeEnum vec_type
					);
					env->AddToCleanup(ret);
					return ret;
				}
				else
				{
					env->Error(callee, "Failed to join strings.");
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
	else if (0 == name.compare("str::substr"))
	{
		if (2 <= m_arguments.size())
		{
			TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
			TValue rhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage();
			if (lhs.IsString() && rhs.IsInteger())
			{
				TValue len;
				if (2 < m_arguments.size())
				{
					len = m_arguments[2]->codegen(builder, module, env).GetFromStorage();
				}
				else
				{
					len = TValue::ConstructExplicit(
						callee,                   // Token * token,
						LITERAL_TYPE_INTEGER,     // LiteralTypeEnum type,
						builder->getInt32(-1),    // llvm::Value * value,
						builder->getInt32Ty(),    // llvm::Type * ty,
						32,                       // int bits,
						false,                    // bool is_storage,
						0,                        // int fixed_vec_len,
						LITERAL_TYPE_INVALID      // LiteralTypeEnum vec_type
					);
				}

				if (len.IsInteger())
				{
					llvm::Value* s = builder->CreateCall(module->getFunction("__str_substr"), { lhs.Value(), rhs.Value(), len.Value() }, "calltmp");
					llvm::Value* a = CreateEntryAlloca(builder, builder->getPtrTy(), nullptr, "alloctmp");
					builder->CreateStore(s, a);
					TValue ret = TValue::ConstructExplicit(
						callee,                   // Token * token,
						LITERAL_TYPE_STRING,      // LiteralTypeEnum type,
						a,                        // llvm::Value * value,
						builder->getPtrTy(),      // llvm::Type * ty,
						64,                       // int bits,
						true,                     // bool is_storage,
						0,                        // int fixed_vec_len,
						LITERAL_TYPE_INVALID      // LiteralTypeEnum vec_type
					);
					env->AddToCleanup(ret);
					return ret;
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
		if (1 == m_arguments.size())
		{
			TValue arg = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
			if (arg.IsString())
			{
				llvm::Value* s = builder->CreateCall(module->getFunction("__str_toupper"), { arg.Value() }, "calltmp");
				llvm::Value* a = CreateEntryAlloca(builder, builder->getPtrTy(), nullptr, "alloctmp");
				builder->CreateStore(s, a);
				TValue ret = TValue::ConstructExplicit(
					callee,                   // Token * token,
					LITERAL_TYPE_STRING,      // LiteralTypeEnum type,
					a,                        // llvm::Value * value,
					builder->getPtrTy(),      // llvm::Type * ty,
					64,                       // int bits,
					true,                     // bool is_storage,
					0,                        // int fixed_vec_len,
					LITERAL_TYPE_INVALID      // LiteralTypeEnum vec_type
				);
				env->AddToCleanup(ret);
				return ret;
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
	else if (0 == name.compare("str::tolower"))
	{
		if (1 == m_arguments.size())
		{
			TValue arg = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
			if (arg.IsString())
			{
				llvm::Value* s = builder->CreateCall(module->getFunction("__str_tolower"), { arg.Value() }, "calltmp");
				llvm::Value* a = CreateEntryAlloca(builder, builder->getPtrTy(), nullptr, "alloctmp");
				builder->CreateStore(s, a);
				TValue ret = TValue::ConstructExplicit(
					callee,                   // Token * token,
					LITERAL_TYPE_STRING,      // LiteralTypeEnum type,
					a,                        // llvm::Value * value,
					builder->getPtrTy(),      // llvm::Type * ty,
					64,                       // int bits,
					true,                     // bool is_storage,
					0,                        // int fixed_vec_len,
					LITERAL_TYPE_INVALID      // LiteralTypeEnum vec_type
				);
				env->AddToCleanup(ret);
				return ret;
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
	else if (0 == name.compare("str::ltrim"))
	{
		if (1 == m_arguments.size())
		{
			TValue arg = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
			if (arg.IsString())
			{
				llvm::Value* s = builder->CreateCall(module->getFunction("__str_ltrim"), { arg.Value() }, "calltmp");
				llvm::Value* a = CreateEntryAlloca(builder, builder->getPtrTy(), nullptr, "alloctmp");
				builder->CreateStore(s, a);
				TValue ret = TValue::ConstructExplicit(
					callee,                   // Token * token,
					LITERAL_TYPE_STRING,      // LiteralTypeEnum type,
					a,                        // llvm::Value * value,
					builder->getPtrTy(),      // llvm::Type * ty,
					64,                       // int bits,
					true,                     // bool is_storage,
					0,                        // int fixed_vec_len,
					LITERAL_TYPE_INVALID      // LiteralTypeEnum vec_type
				);
				env->AddToCleanup(ret);
				return ret;
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
	else if (0 == name.compare("str::rtrim"))
	{
		if (1 == m_arguments.size())
		{
			TValue arg = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
			if (arg.IsString())
			{
				llvm::Value* s = builder->CreateCall(module->getFunction("__str_rtrim"), { arg.Value() }, "calltmp");
				llvm::Value* a = CreateEntryAlloca(builder, builder->getPtrTy(), nullptr, "alloctmp");
				builder->CreateStore(s, a);
				TValue ret = TValue::ConstructExplicit(
					callee,                   // Token * token,
					LITERAL_TYPE_STRING,      // LiteralTypeEnum type,
					a,                        // llvm::Value * value,
					builder->getPtrTy(),      // llvm::Type * ty,
					64,                       // int bits,
					true,                     // bool is_storage,
					0,                        // int fixed_vec_len,
					LITERAL_TYPE_INVALID      // LiteralTypeEnum vec_type
				);
				env->AddToCleanup(ret);
				return ret;
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
	else if (0 == name.compare("str::trim"))
	{
		if (1 == m_arguments.size())
		{
			TValue arg = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
			if (arg.IsString())
			{
				llvm::Value* s = builder->CreateCall(module->getFunction("__str_trim"), { arg.Value() }, "calltmp");
				llvm::Value* a = CreateEntryAlloca(builder, builder->getPtrTy(), nullptr, "alloctmp");
				builder->CreateStore(s, a);
				TValue ret = TValue::ConstructExplicit(
					callee,                   // Token * token,
					LITERAL_TYPE_STRING,      // LiteralTypeEnum type,
					a,                        // llvm::Value * value,
					builder->getPtrTy(),      // llvm::Type * ty,
					64,                       // int bits,
					true,                     // bool is_storage,
					0,                        // int fixed_vec_len,
					LITERAL_TYPE_INVALID      // LiteralTypeEnum vec_type
				);
				env->AddToCleanup(ret);
				return ret;
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

	/*else if (0 == name.compare("vec::contains"))
	{
		if (2 == m_arguments.size() && EXPRESSION_VARIABLE == m_arguments[0]->GetType())
		{
			VariableExpr* ve = static_cast<VariableExpr*>(m_arguments[0]);
			std::string var = ve->Operator()->Lexeme();
			llvm::Value* defval = env->GetVariable(var);
			llvm::Type* defty = env->GetVariableTy(var);
			LiteralTypeEnum varType = env->GetVariableType(var);
			LiteralTypeEnum vecType = env->GetVecType(var);

			if (LITERAL_TYPE_VEC == varType)
			{
				TValue rhs = m_arguments[1]->codegen(builder, module, env);

				llvm::Value* dst = builder->CreateLoad(defty, defval);

				if (LITERAL_TYPE_INTEGER == vecType)
				{
					if (LITERAL_TYPE_DOUBLE == rhs.type)
					{
						// convert rhs to int
						rhs = TValue::Integer(builder->CreateFPToSI(rhs.value, builder->getInt32Ty(), "int_cast_tmp"));
					}
					return TValue::Bool(builder->CreateCall(module->getFunction("__vec_contains_i32"), { dst, rhs.value }, "calltmp"));
				}
			}
			else
			{
				env->Error(callee, "Argument type mismatch.");
				return TValue::NullInvalid();
			}
		}

		if (2 != m_arguments.size())
		{
			env->Error(callee, "Argument count mismatch.");
			return TValue::NullInvalid();
		}
		}

	else if (1 == m_arguments.size() && 0 == name.compare("sgn"))
	{
		TValue v = m_arguments[0]->codegen(builder, module, env);
		if (v.IsInteger()) v = TValue::Double(builder->CreateSIToFP(v.value, builder->getDoubleTy(), "cast_to_dbl"));

		return TValue::Double(builder->CreateCall(module->getFunction("__sgn"), { v.value }, "calltmp"));
	}
	else if (2 == m_arguments.size() && 0 == name.compare("atan2"))
	{
		TValue lhs = m_arguments[0]->codegen(builder, module, env);
		TValue rhs = m_arguments[1]->codegen(builder, module, env);
		if (lhs.IsInteger()) lhs = TValue::Double(builder->CreateSIToFP(lhs.value, builder->getDoubleTy(), "cast_to_dbl"));
		if (rhs.IsInteger()) rhs = TValue::Double(builder->CreateSIToFP(rhs.value, builder->getDoubleTy(), "cast_to_dbl"));

		return TValue::Double(builder->CreateCall(module->getFunction(name), { lhs.value, rhs.value }, "calltmp"));
	}
	else if (0 == name.compare("input"))
	{
		llvm::Value* a = CreateEntryAlloca(builder, builder->getInt8Ty(), builder->getInt32(256), "alloctmp");
		TValue b = TValue::String(builder->CreateGEP(static_cast<llvm::AllocaInst*>(a)->getAllocatedType(), a, builder->getInt8(0), "geptmp"));
		builder->CreateStore(builder->getInt8(0), b.value);
		builder->CreateCall(module->getFunction("input"), { b.value }, "calltmp");
		return b;
	}
	else if (0 == name.compare("pow"))
	{
		if (2 == m_arguments.size())
		{
			std::vector<llvm::Value*> args;
			for (size_t i = 0; i < m_arguments.size(); ++i)
			{
				TValue v = m_arguments[i]->codegen(builder, module, env);
				if (LITERAL_TYPE_INTEGER == v.type)
				{
					// convert rhs to double
					v = TValue::Double(builder->CreateSIToFP(v.value, builder->getDoubleTy(), "cast_to_dbl"));
				}
				args.push_back(v.value);
			}
			llvm::Value* rval = builder->CreateCall(module->getFunction("pow_impl"), args, std::string("call_" + name).c_str());
			return TValue::Double(rval);
		}
		else
		{
			env->Error(callee, "Argument count mismatch.");
			return TValue::NullInvalid();
		}
	}
	
	
	else if (0 == name.compare("vec::fill"))
	{
		if (2 == m_arguments.size())
		{
			TValue lhs = m_arguments[0]->codegen(builder, module, env);
			TValue rhs = m_arguments[1]->codegen(builder, module, env);

			if (rhs.IsInteger())
			{
				if (lhs.IsInteger())
				{
					llvm::Value* v = builder->CreateCall(module->getFunction("__vec_new_rep_i32"), { lhs.value, rhs.value }, "calltmp");
					llvm::Value* a = CreateEntryAlloca(builder, builder->getPtrTy(), nullptr, "alloctmp");
					builder->CreateStore(v, a);
					TValue ret = TValue::Vec(a, LITERAL_TYPE_INTEGER, "");
					env->AddToCleanup(ret);
					return ret;
				}
				else if (lhs.IsBool())
				{
					llvm::Value* v = builder->CreateCall(module->getFunction("__vec_new_rep_bool"), { lhs.value, rhs.value }, "calltmp");
					llvm::Value* a = CreateEntryAlloca(builder, builder->getPtrTy(), nullptr, "alloctmp");
					builder->CreateStore(v, a);
					TValue ret = TValue::Vec(a, LITERAL_TYPE_BOOL, "");
					env->AddToCleanup(ret);
					return ret;
				}
				if (lhs.IsEnum())
				{
					llvm::Value* v = builder->CreateCall(module->getFunction("__vec_new_rep_enum"), { lhs.value, rhs.value }, "calltmp");
					llvm::Value* a = CreateEntryAlloca(builder, builder->getPtrTy(), nullptr, "alloctmp");
					builder->CreateStore(v, a);
					TValue ret = TValue::Vec(a, LITERAL_TYPE_ENUM, "");
					env->AddToCleanup(ret);
					return ret;
				}
				else if (lhs.IsDouble())
				{
					llvm::Value* v = builder->CreateCall(module->getFunction("__vec_new_rep_double"), { lhs.value, rhs.value }, "calltmp");
					llvm::Value* a = CreateEntryAlloca(builder, builder->getPtrTy(), nullptr, "alloctmp");
					builder->CreateStore(v, a);
					TValue ret = TValue::Vec(a, LITERAL_TYPE_DOUBLE, "");
					env->AddToCleanup(ret);
					return ret;
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
	else
	{
		llvm::Function* ftn = env->GetFunction(name);
		if (ftn)
		{
			std::vector<LiteralTypeEnum> types = env->GetFunctionParamTypes(name);
			if (m_arguments.size() != types.size())
			{
				env->Error(callee, "Argument count mismatch.");
				return TValue::NullInvalid();
			}

			std::vector<llvm::Value*> args;
			for (size_t i = 0; i < m_arguments.size(); ++i)
			{
				TValue v = m_arguments[i]->codegen(builder, module, env);
				if (LITERAL_TYPE_INTEGER == types[i] && LITERAL_TYPE_DOUBLE == v.type)
				{
					// convert rhs to int
					v = TValue::Integer(builder->CreateFPToSI(v.value, builder->getInt32Ty(), "int_cast_tmp"));
				}
				else if (LITERAL_TYPE_DOUBLE == types[i] && LITERAL_TYPE_INTEGER == v.type)
				{
					// convert rhs to double
					v = TValue::Double(builder->CreateSIToFP(v.value, builder->getDoubleTy(), "cast_to_dbl"));
				}
				else if (v.IsFixedVec())
				{
					// convert to regular vec
					LiteralTypeEnum vecType = v.fixed_vec_type;
					llvm::Value* a = CreateEntryAlloca(builder, builder->getPtrTy(), nullptr, "alloctmp");
					llvm::Value* addr = builder->CreateCall(module->getFunction("__vec_new"), { builder->getInt32(vecType) }, "calltmp");
					builder->CreateCall(module->getFunction("__vec_assign_fixed"), { builder->getInt32(vecType), addr, builder->getInt32(vecType), v.value, builder->getInt32(v.fixed_vec_sz) }, "calltmp");
					builder->CreateStore(addr, a);
					v = TValue::Vec(a, vecType);
					env->AddToCleanup(v);
				}
				if (v.IsString() || v.IsVec()) v.value = builder->CreateLoad(builder->getPtrTy(), v.value, "loadtmp");
				args.push_back(v.value);
			}
			llvm::Value* rval = builder->CreateCall(ftn, args, std::string("call_" + name).c_str());
			LiteralTypeEnum retType = env->GetFunctionReturnType(name);
			return TValue(retType, rval);
		}
		else
		{
			env->Error(callee, "Function not found.");
		}
	}
	*/
	return TValue::NullInvalid();

}



//-----------------------------------------------------------------------------
TValue GetExpr::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("GetExpr::codegen()\n");
	
	TValue ret = TValue::NullInvalid();
	/*
	// need the root defval and defty
	// need to build arg path

	std::vector<std::string> names;

	Expr* expr = this;

	while (EXPRESSION_GET == expr->GetType())
	{
		GetExpr* ge = static_cast<GetExpr*>(expr);
		names.insert(names.begin(), ge->Operator()->Lexeme());
		expr = ge->Object();
	}

	VariableExpr* vs = static_cast<VariableExpr*>(expr);
	std::string var_name = vs->Operator()->Lexeme();

	llvm::Value* defval = env->GetVariable(var_name);
	llvm::Type* defty = env->GetVariableTy(var_name);
	
	std::string udt_name = "struct." + env->GetVariableToken(var_name)->Lexeme();

	std::vector<llvm::Value*> args(1, builder->getInt32(0));

	for (size_t i = 0; i < names.size(); ++i)
	{
		int idx = env->GetUdtMemberIndex(udt_name, names[i]);
		if (-1 == idx)
		{
			env->Error(env->GetVariableToken(var_name), "Error parsing UDT members");
			return TValue::NullInvalid();
		}

		args.push_back(builder->getInt32(idx));

		Token* token = env->GetUdtMemberTokenAt(udt_name, idx);
		if (!token)
		{
			env->Error(env->GetVariableToken(var_name), "Invalid UDT member token");
			return TValue::NullInvalid();
		}

		TokenTypeEnum ttype = token->GetType();
		if (TOKEN_IDENTIFIER == ttype)
		{
			udt_name = "struct." + token->Lexeme();
			ret = TValue::UDT(udt_name, defty, defval, args);
		}
		else
		{
			llvm::Value* gep = builder->CreateGEP(defty, defval, args, "get_member");
			if (TOKEN_VAR_I32 == ttype) ret = TValue::Integer(gep);
			else if (TOKEN_VAR_ENUM == ttype) ret = TValue::Enum(gep);
			else if (TOKEN_VAR_BOOL == ttype) ret = TValue::Bool(gep);
			else if (TOKEN_VAR_F32 == ttype) ret = TValue::Double(gep);
			else if (TOKEN_VAR_STRING == ttype) ret = TValue::String(gep);
			else if (TOKEN_VAR_VEC == ttype)
			{
				ret = TValue::Vec(gep, env->GetUdtMemberVecTypeAt(udt_name, idx), env->GetUdtMemberVecTypeIdAt(udt_name, idx));
			}
		}
	}
	*/
	return ret;
}



//-----------------------------------------------------------------------------
TValue LogicalExpr::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("LogicalExpr::codegen()\n");
	
	if (!m_left || !m_right) return TValue::NullInvalid();
	TValue lhs = m_left->codegen(builder, module, env);
	TValue rhs = m_right->codegen(builder, module, env);
	if (!lhs.Value() || !rhs.Value()) return TValue::NullInvalid();

	switch (m_token->GetType())
	{
	case TOKEN_AND:
		return TValue::MakeBool(m_token, builder->CreateAnd(lhs.Value(), rhs.Value(), "logical_and_cmp"));
		break;

	case TOKEN_OR:
		return TValue::MakeBool(m_token, builder->CreateOr(lhs.Value(), rhs.Value(), "logical_or_cmp"));
		break;
	}
	
	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue RangeExpr::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("RangeExpr::codegen()\n");
	
	if (!m_left || !m_right) return TValue::NullInvalid();
	TValue lhs = m_left->codegen(builder, module, env);
	TValue rhs = m_right->codegen(builder, module, env);
	if (!lhs.Value() || !rhs.Value()) return TValue::NullInvalid();
	
	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue SetExpr::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("SetExpr::codegen()\n");
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
	return TValue::NullInvalid();
}