#include "Expressions.h"
#include <deque>

#define PI 3.1415926535897932384626433832795028841971693993751058
#define RAD2DEG (180.0 / PI)
#define DEG2RAD (1 / RAD2DEG)

//-----------------------------------------------------------------------------
void copy_udt(std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env,
	std::string udtname,
	llvm::Value* defval, llvm::Value* rdefval, llvm::Type* defty, llvm::Type* rdefty, std::vector<llvm::Value*> lhs_args, std::vector<llvm::Value*> rhs_args)
{
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
			else
			{
				printf("Initializer not present for copy_udt\n");
			}
		}
	}
}

TValue AssignExpr::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("AssignExpr::codegen()\n");
	TValue rhs = m_right->codegen(context, builder, module, env);

	std::string var = m_token->Lexeme();
	llvm::Value* defval = env->GetVariable(var);
	llvm::Type* defty = env->GetVariableTy(var);
	LiteralTypeEnum varType = env->GetVariableType(var);

	if (LITERAL_TYPE_STRING == varType)
	{
		llvm::Value* lhs_load = builder->CreateLoad(builder->getPtrTy(), defval, "loadtmp");
		llvm::Value* rhs_load = builder->CreateLoad(builder->getPtrTy(), rhs.value, "loadtmp");
		builder->CreateCall(module->getFunction("__str_assign"), { lhs_load, rhs_load }, "calltmp");
	}
	else if (LITERAL_TYPE_INTEGER == varType || LITERAL_TYPE_BOOL == varType || LITERAL_TYPE_DOUBLE == varType || LITERAL_TYPE_ENUM == varType || LITERAL_TYPE_POINTER == varType)
	{
		if (LITERAL_TYPE_INTEGER == varType && rhs.IsDouble())
		{
			rhs = TValue(LITERAL_TYPE_INTEGER, builder->CreateFPToSI(rhs.value, builder->getInt32Ty(), "cast_to_int"));
		}
		else if (LITERAL_TYPE_DOUBLE == varType && rhs.IsInteger())
		{
			rhs = TValue(LITERAL_TYPE_DOUBLE, builder->CreateSIToFP(rhs.value, builder->getDoubleTy(), "cast_to_dbl"));
		}
		builder->CreateStore(rhs.value, defval);
	}
	else if (LITERAL_TYPE_VEC == varType)
	{
		LiteralTypeEnum vecType = env->GetVecType(var);

		if (nullptr != m_vecIndex)
		{
			// assign to index
			TValue idx = m_vecIndex->codegen(context, builder, module, env);

			llvm::Value* src = builder->CreateLoad(defty, defval, "vecsrc");
			if (LITERAL_TYPE_INTEGER == vecType)
			{
				if (LITERAL_TYPE_DOUBLE == rhs.type)
				{
					// convert rhs to int
					rhs = TValue::Integer(builder->CreateFPToSI(rhs.value, builder->getInt32Ty(), "int_cast_tmp"));
				}
				builder->CreateCall(module->getFunction("__vec_set_i32"), { src, idx.value, rhs.value }, "calltmp");
			}
			else if (LITERAL_TYPE_DOUBLE == vecType)
			{
				if (LITERAL_TYPE_INTEGER == rhs.type)
				{
					// convert rhs to double
					rhs = TValue::Double(builder->CreateSIToFP(rhs.value, builder->getDoubleTy(), "cast_to_dbl"));
				}
				builder->CreateCall(module->getFunction("__vec_set_double"), { src, idx.value, rhs.value }, "calltmp");
			}
			else if (LITERAL_TYPE_ENUM == vecType)
			{
				builder->CreateCall(module->getFunction("__vec_set_enum"), { src, idx.value, rhs.value }, "calltmp");
			}
			else if (LITERAL_TYPE_BOOL == vecType)
			{
				rhs = TValue::Integer(builder->CreateIntCast(rhs.value, builder->getInt8Ty(), true));
				builder->CreateCall(module->getFunction("__vec_set_bool"), { src, idx.value, rhs.value }, "calltmp");
			}
			else if (LITERAL_TYPE_STRING == vecType)
			{
				llvm::Value* tmp = builder->CreateLoad(builder->getPtrTy(), rhs.value, "loadtmp");
				builder->CreateCall(module->getFunction("__vec_set_str"), { src, idx.value, tmp }, "calltmp");
			}

		}
		else
		{
			// overwrite entire vector
			if (rhs.IsFixedVec())
			{
				llvm::Value* dstType = builder->getInt32(vecType);
				llvm::Value* srcType = builder->getInt32(rhs.fixed_vec_type);
				llvm::Value* srcQty = builder->getInt32(rhs.fixed_vec_sz);
				llvm::Value* dstPtr = builder->CreateLoad(defty, defval, "vecdst");
				llvm::Value* srcPtr = rhs.value; // no load required, address already loaded in local variable
				builder->CreateCall(module->getFunction("__vec_assign_fixed"), { dstType, dstPtr, srcType, srcPtr, srcQty }, "calltmp");
			}
			else if (rhs.IsVec())
			{
				llvm::Value* dstType = builder->getInt32(vecType);
				llvm::Value* srcType = builder->getInt32(rhs.fixed_vec_type);
				llvm::Value* dstPtr = builder->CreateLoad(defty, defval, "vecdst");
				llvm::Value* srcPtr = builder->CreateLoad(defty, rhs.value, "vecdst");
				builder->CreateCall(module->getFunction("__vec_assign"), { dstType, dstPtr, srcType, srcPtr }, "calltmp");
			}

		}
	}
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

	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue BinaryExpr::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("BinaryExpr::codegen()\n");

	if (!m_left || !m_right) return TValue::NullInvalid();
	TValue lhs = m_left->codegen(context, builder, module, env);
	if (!lhs.value) return TValue::NullInvalid();
	if (EXPRESSION_GET == m_left->GetType())
	{
		llvm::Type* ty = nullptr;
		if (lhs.IsInteger() || lhs.IsEnum()) ty = builder->getInt32Ty();
		else if (lhs.IsDouble()) ty = builder->getDoubleTy();
		else if (lhs.IsBool()) ty = builder->getInt1Ty();
		
		if (ty) lhs = TValue(lhs.type, builder->CreateLoad(ty, lhs.value, "gep_load"));
	}

	if (TOKEN_AS == m_token->GetType() && EXPRESSION_VARIABLE == m_right->GetType())
	{
		TokenTypeEnum new_type = (static_cast<VariableExpr*>(m_right))->Operator()->GetType();

		if (TOKEN_VAR_I32 == new_type)
		{
			if (lhs.IsInteger()) return lhs;
			if (lhs.IsDouble())
			{
				return TValue::Integer(builder->CreateFPToSI(lhs.value, builder->getInt32Ty(), "int_cast_tmp"));
			}
			if (lhs.IsString())
			{
				llvm::Value* tmp = builder->CreateLoad(builder->getPtrTy(), lhs.value);
				return TValue::Integer(builder->CreateCall(module->getFunction("__str_to_int"), { tmp }, "calltmp"));
			}
		}
		else if (TOKEN_VAR_F32 == new_type)
		{
			if (lhs.IsDouble()) return lhs;
			if (lhs.IsInteger())
			{
				return TValue::Double(builder->CreateSIToFP(lhs.value, builder->getDoubleTy(), "cast_to_dbl"));
			}
			if (lhs.IsString())
			{
				llvm::Value* tmp = builder->CreateLoad(builder->getPtrTy(), lhs.value);
				return TValue::Double(builder->CreateCall(module->getFunction("__str_to_double"), { tmp }, "calltmp"));
			}
		}
		else if (TOKEN_VAR_STRING == new_type)
		{
			if (lhs.IsString()) return lhs;

			if (lhs.IsInteger())
			{
				llvm::Value* s = builder->CreateCall(module->getFunction("__int_to_string"), { lhs.value }, "calltmp");
				llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
				builder->CreateStore(s, a);
				TValue ret = TValue::String(a);
				env->AddToCleanup(ret);
				return ret;
			}
			else if (lhs.IsEnum())
			{
				// to do
				//builder->CreateCall(module->getFunction("strcat"), { lhs.value, b.value }, "calltmp");
			}
			else if (lhs.IsDouble())
			{
				llvm::Value* s = builder->CreateCall(module->getFunction("__double_to_string"), { lhs.value }, "calltmp");
				llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
				builder->CreateStore(s, a);
				TValue ret = TValue::String(a);
				env->AddToCleanup(ret);
				return ret;
			}
			else if (lhs.IsBool())
			{
				llvm::Value* s = builder->CreateCall(module->getFunction("__bool_to_string"), { lhs.value }, "calltmp");
				llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
				builder->CreateStore(s, a);
				TValue ret = TValue::String(a);
				env->AddToCleanup(ret);
				return ret;
			}
			printf("Unable to cast to string\n");
		}
	}


	TValue rhs = m_right->codegen(context, builder, module, env);
	if (!lhs.value || !rhs.value) return TValue::NullInvalid();
	if (EXPRESSION_GET == m_right->GetType())
	{
		llvm::Type* ty = nullptr;
		if (rhs.IsInteger() || rhs.IsEnum()) ty = builder->getInt32Ty();
		else if (rhs.IsDouble()) ty = builder->getDoubleTy();
		else if (rhs.IsBool()) ty = builder->getInt1Ty();
		
		if (ty) rhs = TValue(rhs.type, builder->CreateLoad(ty, rhs.value, "gep_load"));
	}
	
	if (lhs.IsInteger() && (rhs.IsDouble() || m_token->GetType() == TOKEN_SLASH))
	{
		// convert lhs to double
		lhs = TValue::Double(builder->CreateSIToFP(lhs.value, builder->getDoubleTy(), "cast_to_dbl"));
	}
	if (rhs.IsInteger() && (lhs.IsDouble() || m_token->GetType() == TOKEN_SLASH))
	{
		// convert rhs to double
		rhs = TValue::Double(builder->CreateSIToFP(rhs.value, builder->getDoubleTy(), "cast_to_int"));
	}

	switch (m_token->GetType())
	{
	case TOKEN_PERCENT:
		if (lhs.IsInteger())
		{
			return TValue::Integer(builder->CreateCall(module->getFunction("mod_impl"), { lhs.value, rhs.value }, "calltmp"));
		}
		break;

	case TOKEN_MINUS:
		if (lhs.IsDouble()) return TValue::Double(builder->CreateFSub(lhs.value, rhs.value, "subtmp"));
		if (lhs.IsInteger()) return TValue::Integer(builder->CreateSub(lhs.value, rhs.value, "subtmp"));
		break;

	case TOKEN_PLUS:
		if (lhs.IsDouble()) return TValue::Double(builder->CreateFAdd(lhs.value, rhs.value, "addtmp"));
		if (lhs.IsInteger()) return TValue::Integer(builder->CreateAdd(lhs.value, rhs.value, "addtmp"));
		if (lhs.IsString() && rhs.IsString())
		{
			llvm::Value* lhs_load = builder->CreateLoad(builder->getPtrTy(), lhs.value);
			llvm::Value* rhs_load = builder->CreateLoad(builder->getPtrTy(), rhs.value);
			llvm::Value* s = builder->CreateCall(module->getFunction("__std_string_cat"), { lhs_load, rhs_load }, "calltmp");
			llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
			builder->CreateStore(s, a);
			TValue ret = TValue::String(a);
			env->AddToCleanup(ret);
			return ret;
		}
		break;

	case TOKEN_STAR:
		if (lhs.IsDouble()) return TValue::Double(builder->CreateFMul(lhs.value, rhs.value, "multmp"));
		if (lhs.IsInteger()) return TValue::Integer(builder->CreateMul(lhs.value, rhs.value, "multmp"));
		break;

	case TOKEN_SLASH:
		if (lhs.IsDouble()) return TValue::Double(builder->CreateFDiv(lhs.value, rhs.value, "divtmp"));
		//if (lhs.IsInteger()) return TValue::Double(builder->CreateSDiv(lhs.value, rhs.value, "divtmp"));
		break;

	case TOKEN_GREATER:
		if (lhs.IsDouble()) return TValue::Bool(builder->CreateFCmpUGT(lhs.value, rhs.value, "cmptmp"));
		if (lhs.IsInteger()) return TValue::Bool(builder->CreateICmpSGT(lhs.value, rhs.value, "cmptmp"));
		break;

	case TOKEN_GREATER_EQUAL:
		if (lhs.IsDouble()) return TValue::Bool(builder->CreateFCmpUGE(lhs.value, rhs.value, "cmptmp"));
		if (lhs.IsInteger()) return TValue::Bool(builder->CreateICmpSGE(lhs.value, rhs.value, "cmptmp"));
		break;

	case TOKEN_LESS:
		if (lhs.IsDouble()) return TValue::Bool(builder->CreateFCmpULT(lhs.value, rhs.value, "cmptmp"));
		if (lhs.IsInteger()) return TValue::Bool(builder->CreateICmpSLT(lhs.value, rhs.value, "cmptmp"));
		break;

	case TOKEN_LESS_EQUAL:
		if (lhs.IsDouble()) return TValue::Bool(builder->CreateFCmpULE(lhs.value, rhs.value, "cmptmp"));
		if (lhs.IsInteger()) return TValue::Bool(builder->CreateICmpSLE(lhs.value, rhs.value, "cmptmp"));
		break;

	case TOKEN_BANG_TILDE: // intentional fall-through
	case TOKEN_BANG_EQUAL:
		if (lhs.IsDouble())
		{
			if (TOKEN_BANG_TILDE == m_token->GetType())
			{
				llvm::Value* tmp = builder->CreateCall(module->getFunction("fabs"), { builder->CreateFSub(lhs.value, rhs.value) }, "calltmp");
				return TValue::Bool(builder->CreateFCmpUGT(tmp, llvm::ConstantFP::get(*context, llvm::APFloat(1.0e-12)), "cmptmp"));
			}
			return TValue::Bool(builder->CreateFCmpUNE(lhs.value, rhs.value, "cmptmp"));
		}
		if (lhs.IsInteger() || lhs.IsEnum() || lhs.IsBool()) return TValue::Bool(builder->CreateICmpNE(lhs.value, rhs.value, "cmptmp"));
		if (lhs.IsString() && rhs.IsString())
		{
			llvm::Value* lhs_load = builder->CreateLoad(builder->getPtrTy(), lhs.value, "loadtmp");
			llvm::Value* rhs_load = builder->CreateLoad(builder->getPtrTy(), rhs.value, "loadtmp");
			llvm::Value* tmp = builder->CreateCall(module->getFunction("__str_cmp"), { lhs_load, rhs_load }, "calltmp");
			return TValue::Bool(builder->CreateICmpNE(tmp, builder->getTrue(), "boolcmp"));
		}
		break;

	case TOKEN_TILDE_TILDE: // intentional fall-through
	case TOKEN_EQUAL_EQUAL:
		if (lhs.IsDouble())
		{
			if (TOKEN_TILDE_TILDE == m_token->GetType())
			{
				llvm::Value* tmp = builder->CreateCall(module->getFunction("fabs"), { builder->CreateFSub(lhs.value, rhs.value) }, "calltmp");
				return TValue::Bool(builder->CreateFCmpULT(tmp, llvm::ConstantFP::get(*context, llvm::APFloat(1.0e-12)), "cmptmp"));
			}
			return TValue::Bool(builder->CreateFCmpUEQ(lhs.value, rhs.value, "cmptmp"));
		}
		if (lhs.IsInteger() || lhs.IsEnum() || lhs.IsBool()) return TValue::Bool(builder->CreateICmpEQ(lhs.value, rhs.value, "cmptmp"));
		if (lhs.IsString() && rhs.IsString())
		{
			llvm::Value* lhs_load = builder->CreateLoad(builder->getPtrTy(), lhs.value, "loadtmp");
			llvm::Value* rhs_load = builder->CreateLoad(builder->getPtrTy(), rhs.value, "loadtmp");
			return TValue::Bool(builder->CreateCall(module->getFunction("__str_cmp"), { lhs_load, rhs_load }, "calltmp"));
		}
		break;	
	}

	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue BracketExpr::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("BracketExpr::codegen()\n");

	std::vector<TValue> vals;

	bool has_int = false;
	bool has_double = false;

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
					TValue val = lhs->codegen(context, builder, module, env);
					if (val.IsString()) val = TValue::String(builder->CreateLoad(builder->getPtrTy(), val.value, "loadtmp"));

					int qty = le->GetLiteral().IntValue();
					for (size_t i = 0; i < qty; ++i)
					{
						vals.push_back(val);
					}

					if (val.IsInteger()) has_int = true;
					if (val.IsDouble()) has_double = true;
				}
			}
		}
		else
		{
			TValue v = arg->codegen(context, builder, module, env);
			if (v.IsString()) v = TValue::String(builder->CreateLoad(builder->getPtrTy(), v.value, "loadtmp"));

			vals.push_back(v);
			if (v.IsInteger()) has_int = true;
			if (v.IsDouble()) has_double = true;
		}
	}

	if (has_int && has_double)
	{
		// convert everything to double
		for (auto& v : vals)
		{
			if (v.IsInteger())
			{
				v = TValue::Double(builder->CreateSIToFP(v.value, builder->getDoubleTy(), "cast_to_dbl"));
			}
		}
	}

	if (vals.empty()) return TValue::NullInvalid();

	llvm::AllocaInst* a = nullptr;
	if (vals[0].IsInteger() || vals[0].IsEnum()) a = builder->CreateAlloca(builder->getInt32Ty(), builder->getInt32(vals.size()), "alloc_vec_tmp");
	if (vals[0].IsBool()) a = builder->CreateAlloca(builder->getInt8Ty(), builder->getInt32(vals.size()), "alloc_vec_tmp");
	if (vals[0].IsDouble()) a = builder->CreateAlloca(builder->getDoubleTy(), builder->getInt32(vals.size()), "alloc_vec_tmp");
	if (vals[0].IsString()) a = builder->CreateAlloca(builder->getPtrTy(), builder->getInt32(vals.size()), "alloc_vec_tmp");

	if (a)
	{
		TValue ret = TValue::NullInvalid();
		for (size_t i = 0; i < vals.size(); ++i)
		{
			llvm::Value* b = builder->CreateGEP(a->getAllocatedType(), a, builder->getInt32(i), "geptmp");
			if (0 == i) ret = TValue::FixedVec(b, vals[0].type, vals.size());

			// convert bool to int8
			/*if (vals[i].IsBool())
			{
				vals[i] = TValue::Bool(builder->CreateIntCast(vals[i].value, builder->getInt8Ty(), true, "bool_cast"));
			}*/
			builder->CreateStore(vals[i].value, b);
		}
		return ret;
	}


	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue CallExpr::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("CallExpr::codegen()\n");
	if (!m_callee) return TValue::NullInvalid();
	if (EXPRESSION_VARIABLE != m_callee->GetType()) return TValue::NullInvalid();

	std::string name = (static_cast<VariableExpr*>(m_callee))->Operator()->Lexeme();
	//printf("  lexeme=%s\n", name.c_str());

	if (0 == name.compare("abs") && 1 == m_arguments.size())
	{
		TValue v = m_arguments[0]->codegen(context, builder, module, env);
		if (v.IsInteger())
			return TValue::Integer(builder->CreateCall(module->getFunction("abs"), { v.value }, "calltmp"));
		else if (v.IsDouble())
			return TValue::Double(builder->CreateCall(module->getFunction("fabs"), { v.value }, "calltmp"));
	}
	else if (0 == name.compare("deg2rad") && 1 == m_arguments.size())
	{
		TValue v = m_arguments[0]->codegen(context, builder, module, env);
		if (v.IsInteger()) v = TValue::Double(builder->CreateSIToFP(v.value, builder->getDoubleTy(), "cast_to_dbl"));
		llvm::Value* tmp = llvm::ConstantFP::get(*context, llvm::APFloat(DEG2RAD));
		return TValue::Double(builder->CreateFMul(v.value, tmp));
	}
	else if (0 == name.compare("rad2deg") && 1 == m_arguments.size())
	{
		TValue v = m_arguments[0]->codegen(context, builder, module, env);
		if (v.IsInteger()) v = TValue::Double(builder->CreateSIToFP(v.value, builder->getDoubleTy(), "cast_to_dbl"));
		llvm::Value* tmp = llvm::ConstantFP::get(*context, llvm::APFloat(RAD2DEG));
		return TValue::Double(builder->CreateFMul(v.value, tmp));
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
		TValue v = m_arguments[0]->codegen(context, builder, module, env);
		if (v.IsInteger()) v = TValue::Double(builder->CreateSIToFP(v.value, builder->getDoubleTy(), "cast_to_dbl"));

		return TValue::Double(builder->CreateCall(module->getFunction(name), { v.value }, "calltmp"));
	}
	else if (1 == m_arguments.size() && 0 == name.compare("sgn"))
	{
		TValue v = m_arguments[0]->codegen(context, builder, module, env);
		if (v.IsInteger()) v = TValue::Double(builder->CreateSIToFP(v.value, builder->getDoubleTy(), "cast_to_dbl"));

		return TValue::Double(builder->CreateCall(module->getFunction("__sgn"), { v.value }, "calltmp"));
	}
	else if (2 == m_arguments.size() && 0 == name.compare("atan2"))
	{
		TValue lhs = m_arguments[0]->codegen(context, builder, module, env);
		TValue rhs = m_arguments[1]->codegen(context, builder, module, env);
		if (lhs.IsInteger()) lhs = TValue::Double(builder->CreateSIToFP(lhs.value, builder->getDoubleTy(), "cast_to_dbl"));
		if (rhs.IsInteger()) rhs = TValue::Double(builder->CreateSIToFP(rhs.value, builder->getDoubleTy(), "cast_to_dbl"));

		return TValue::Double(builder->CreateCall(module->getFunction(name), { lhs.value, rhs.value }, "calltmp"));
	}
	else if (0 == name.compare("input"))
	{
		llvm::AllocaInst* a = builder->CreateAlloca(builder->getInt8Ty(), builder->getInt32(256), "alloctmp");
		TValue b = TValue::String(builder->CreateGEP(a->getAllocatedType(), a, builder->getInt8(0), "geptmp"));
		builder->CreateStore(builder->getInt8(0), b.value);
		builder->CreateCall(module->getFunction("input"), { b.value }, "calltmp");
		return b;
	}
	else if (0 == name.compare("clock"))
	{
		if (m_arguments.empty())
		{
			return TValue::Double(builder->CreateCall(module->getFunction("clock_impl"), {}, "calltmp"));
		}
	}
	else if (0 == name.compare("rand"))
	{
		if (m_arguments.empty())
		{
			return TValue::Double(builder->CreateCall(module->getFunction("rand_impl"), {}, "calltmp"));
		}
		else if (1 == m_arguments.size() && EXPRESSION_RANGE == m_arguments[0]->GetType())
		{
			RangeExpr* re = static_cast<RangeExpr*>(m_arguments[0]);
			TValue lhs = re->Left()->codegen(context, builder, module, env);
			TValue rhs = re->Right()->codegen(context, builder, module, env);

			if (lhs.IsDouble()) return TValue::Integer(builder->CreateFPToSI(lhs.value, builder->getInt32Ty(), "int_cast_tmp"));
			if (rhs.IsDouble()) return TValue::Integer(builder->CreateFPToSI(rhs.value, builder->getInt32Ty(), "int_cast_tmp"));

			return TValue::Integer(builder->CreateCall(module->getFunction("rand_range_impl"), { lhs.value, rhs.value }, "calltmp"));
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
				llvm::Value* defval = env->GetVariable(var);
				llvm::Type* defty = env->GetVariableTy(var);
				LiteralTypeEnum varType = env->GetVariableType(var);
				LiteralTypeEnum vecType = env->GetVecType(var);

				//("%lu, %d, %d\n", defval, varType, vecType);

				if (LITERAL_TYPE_VEC == varType)
				{
					llvm::Value* src = builder->CreateLoad(defty, defval);
					llvm::Value* srcType = builder->getInt32(vecType);
					return TValue::Integer(builder->CreateCall(module->getFunction("__vec_len"), { srcType, src }, "calltmp"));
				}
			}
			else if (EXPRESSION_GET == m_arguments[0]->GetType())
			{
				GetExpr* ge = static_cast<GetExpr*>(m_arguments[0]);
				TValue v = m_arguments[0]->codegen(context, builder, module, env);

				if (v.IsVec())
				{
					llvm::Value* src = builder->CreateLoad(builder->getPtrTy(), v.value);
					llvm::Value* srcType = builder->getInt32(v.fixed_vec_type);
					return TValue::Integer(builder->CreateCall(module->getFunction("__vec_len"), { srcType, src }, "calltmp"));
				}
			}
		}
	}
	else if (0 == name.compare("pow"))
	{
		if (2 == m_arguments.size())
		{
			std::vector<llvm::Value*> args;
			for (size_t i = 0; i < m_arguments.size(); ++i)
			{
				TValue v = m_arguments[i]->codegen(context, builder, module, env);
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
	}
	else if (0 == name.compare("file::readlines"))
	{
		if (1 == m_arguments.size())
		{
			TValue arg = m_arguments[0]->codegen(context, builder, module, env);
			if (arg.IsString())
			{
				llvm::Value* tmp = builder->CreateLoad(builder->getPtrTy(), arg.value, "loadtmp");
				llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
				builder->CreateStore(builder->CreateCall(module->getFunction("__file_readlines"), { tmp }, "calltmp"), a);
				TValue ret = TValue::Vec(a, LITERAL_TYPE_STRING);
				env->AddToCleanup(ret);
				return ret;
			}
		}
	}
	else if (0 == name.compare("file::writelines"))
	{
		if (2 == m_arguments.size())
		{
			TValue lhs = m_arguments[0]->codegen(context, builder, module, env);
			TValue rhs = m_arguments[1]->codegen(context, builder, module, env);
			if (lhs.IsString() && rhs.IsVec())
			{
				llvm::Value* lhs_load = builder->CreateLoad(builder->getPtrTy(), lhs.value, "loadtmp");
				llvm::Value* rhs_load = builder->CreateLoad(builder->getPtrTy(), rhs.value, "loadtmp");
				builder->CreateCall(module->getFunction("__file_writelines"), { lhs_load, rhs_load }, "calltmp");
			}
		}
	}
	else if (0 == name.compare("str::contains"))
	{
		if (2 == m_arguments.size())
		{
			TValue lhs = m_arguments[0]->codegen(context, builder, module, env);
			TValue rhs = m_arguments[1]->codegen(context, builder, module, env);
			if (lhs.IsString() && rhs.IsString())
			{
				llvm::Value* lhs_load = builder->CreateLoad(builder->getPtrTy(), lhs.value, "loadtmp");
				llvm::Value* rhs_load = builder->CreateLoad(builder->getPtrTy(), rhs.value, "loadtmp");
				return TValue::Bool(builder->CreateCall(module->getFunction("__str_contains"), { lhs_load, rhs_load }, "calltmp"));
			}
		}
	}
	else if (0 == name.compare("str::replace"))
	{
		if (3 == m_arguments.size())
		{
			TValue lhs = m_arguments[0]->codegen(context, builder, module, env);
			TValue mhs = m_arguments[1]->codegen(context, builder, module, env);
			TValue rhs = m_arguments[2]->codegen(context, builder, module, env);
			if (lhs.IsString() && mhs.IsString() && rhs.IsString())
			{
				llvm::Value* lhs_load = builder->CreateLoad(builder->getPtrTy(), lhs.value, "loadtmp");
				llvm::Value* mhs_load = builder->CreateLoad(builder->getPtrTy(), mhs.value, "loadtmp");
				llvm::Value* rhs_load = builder->CreateLoad(builder->getPtrTy(), rhs.value, "loadtmp");
				llvm::Value* s = builder->CreateCall(module->getFunction("__str_replace"), { lhs_load, mhs_load, rhs_load }, "calltmp");
				llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
				builder->CreateStore(s, a);
				TValue ret = TValue::String(a);
				env->AddToCleanup(ret);
				return ret;
			}
		}
	}
	else if (0 == name.compare("str::split"))
	{
		if (2 == m_arguments.size())
		{
			TValue lhs = m_arguments[0]->codegen(context, builder, module, env);
			TValue rhs = m_arguments[1]->codegen(context, builder, module, env);
			if (lhs.IsString() && rhs.IsString())
			{
				llvm::Value* lhs_load = builder->CreateLoad(builder->getPtrTy(), lhs.value, "loadtmp");
				llvm::Value* rhs_load = builder->CreateLoad(builder->getPtrTy(), rhs.value, "loadtmp");
				llvm::Value* s = builder->CreateCall(module->getFunction("__str_split"), { lhs_load, rhs_load }, "calltmp");
				llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
				builder->CreateStore(s, a);
				TValue ret = TValue::Vec(a, LITERAL_TYPE_STRING);
				env->AddToCleanup(ret);
				return ret;
			}
		}
	}
	else if (0 == name.compare("str::join"))
	{
		if (2 == m_arguments.size())
		{
			TValue lhs = m_arguments[0]->codegen(context, builder, module, env);
			TValue rhs = m_arguments[1]->codegen(context, builder, module, env);
			if (lhs.IsVec() && LITERAL_TYPE_STRING == lhs.fixed_vec_type && rhs.IsString())
			{
				llvm::Value* lhs_load = builder->CreateLoad(builder->getPtrTy(), lhs.value, "loadtmp");
				llvm::Value* rhs_load = builder->CreateLoad(builder->getPtrTy(), rhs.value, "loadtmp");
				llvm::Value* s = builder->CreateCall(module->getFunction("__str_join"), { lhs_load, rhs_load }, "calltmp");
				llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
				builder->CreateStore(s, a);
				TValue ret = TValue::String(a);
				env->AddToCleanup(ret);
				return ret;
			}
		}
	}
	else if (0 == name.compare("str::substr"))
	{
		if (2 <= m_arguments.size())
		{
			TValue lhs = m_arguments[0]->codegen(context, builder, module, env);
			TValue rhs = m_arguments[1]->codegen(context, builder, module, env);
			if (lhs.IsString() && rhs.IsInteger())
			{
				TValue len = 2 < m_arguments.size() ? m_arguments[2]->codegen(context, builder, module, env) : TValue::Integer(builder->getInt32(-1));
				if (len.IsInteger())
				{
					llvm::Value* lhs_load = builder->CreateLoad(builder->getPtrTy(), lhs.value, "loadtmp");
					llvm::Value* s = builder->CreateCall(module->getFunction("__str_substr"), { lhs_load, rhs.value, len.value }, "calltmp");
					llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
					builder->CreateStore(s, a);
					TValue ret = TValue::String(a);
					env->AddToCleanup(ret);
					return ret;
				}
			}
		}
	}
	else if (0 == name.compare("str::toupper"))
	{
		if (1 == m_arguments.size())
		{
			TValue arg = m_arguments[0]->codegen(context, builder, module, env);
			if (arg.IsString())
			{
				llvm::Value* arg_load = builder->CreateLoad(builder->getPtrTy(), arg.value, "loadtmp");
				llvm::Value* s = builder->CreateCall(module->getFunction("__str_toupper"), { arg_load }, "calltmp");
				llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
				builder->CreateStore(s, a);
				TValue ret = TValue::String(a);
				env->AddToCleanup(ret);
				return ret;
			}
		}
	}
	else if (0 == name.compare("str::tolower"))
	{
		if (1 == m_arguments.size())
		{
			TValue arg = m_arguments[0]->codegen(context, builder, module, env);
			if (arg.IsString())
			{
				llvm::Value* arg_load = builder->CreateLoad(builder->getPtrTy(), arg.value, "loadtmp");
				llvm::Value* s = builder->CreateCall(module->getFunction("__str_tolower"), { arg_load }, "calltmp");
				llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
				builder->CreateStore(s, a);
				TValue ret = TValue::String(a);
				env->AddToCleanup(ret);
				return ret;
			}
		}
	}
	else if (0 == name.compare("str::ltrim"))
	{
		if (1 == m_arguments.size())
		{
			TValue arg = m_arguments[0]->codegen(context, builder, module, env);
			if (arg.IsString())
			{
				llvm::Value* arg_load = builder->CreateLoad(builder->getPtrTy(), arg.value, "loadtmp");
				llvm::Value* s = builder->CreateCall(module->getFunction("__str_ltrim"), { arg_load }, "calltmp");
				llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
				builder->CreateStore(s, a);
				TValue ret = TValue::String(a);
				env->AddToCleanup(ret);
				return ret;
			}
		}
		}
	else if (0 == name.compare("str::rtrim"))
	{
		if (1 == m_arguments.size())
		{
			TValue arg = m_arguments[0]->codegen(context, builder, module, env);
			if (arg.IsString())
			{
				llvm::Value* arg_load = builder->CreateLoad(builder->getPtrTy(), arg.value, "loadtmp");
				llvm::Value* s = builder->CreateCall(module->getFunction("__str_rtrim"), { arg_load }, "calltmp");
				llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
				builder->CreateStore(s, a);
				TValue ret = TValue::String(a);
				env->AddToCleanup(ret);
				return ret;
			}
		}
		}
	else if (0 == name.compare("str::trim"))
	{
		if (1 == m_arguments.size())
		{
			TValue arg = m_arguments[0]->codegen(context, builder, module, env);
			if (arg.IsString())
			{
				llvm::Value* arg_load = builder->CreateLoad(builder->getPtrTy(), arg.value, "loadtmp");
				llvm::Value* s = builder->CreateCall(module->getFunction("__str_trim"), { arg_load }, "calltmp");
				llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
				builder->CreateStore(s, a);
				TValue ret = TValue::String(a);
				env->AddToCleanup(ret);
				return ret;
			}
		}
		}
	else if (0 == name.compare("vec::append"))
	{
		if (2 == m_arguments.size() && EXPRESSION_VARIABLE == m_arguments[0]->GetType())
		{
			VariableExpr* ve = static_cast<VariableExpr*>(m_arguments[0]);
			std::string var = ve->Operator()->Lexeme();
			llvm::Value* defval = env->GetVariable(var);
			llvm::Type* defty = env->GetVariableTy(var);
			LiteralTypeEnum varType = env->GetVariableType(var);
			LiteralTypeEnum vecType = env->GetVecType(var);

			//printf("%lu, %d, %d\n", defval, varType, vecType);

			if (LITERAL_TYPE_VEC == varType)
			{
				TValue rhs = m_arguments[1]->codegen(context, builder, module, env);

				llvm::Value* dst = builder->CreateLoad(defty, defval);
				llvm::Value* dstType = builder->getInt32(vecType);

				if (LITERAL_TYPE_INTEGER == vecType)
				{
					if (LITERAL_TYPE_DOUBLE == rhs.type)
					{
						// convert rhs to int
						rhs = TValue::Integer(builder->CreateFPToSI(rhs.value, builder->getInt32Ty(), "int_cast_tmp"));
					}
					builder->CreateCall(module->getFunction("__vec_append_i32"), { dstType, dst, rhs.value }, "calltmp");
				}
				else if (LITERAL_TYPE_DOUBLE == vecType)
				{
					if (LITERAL_TYPE_INTEGER == rhs.type)
					{
						// convert rhs to double
						rhs = TValue::Double(builder->CreateSIToFP(rhs.value, builder->getDoubleTy(), "cast_to_dbl"));
					}
					builder->CreateCall(module->getFunction("__vec_append_double"), { dstType, dst, rhs.value }, "calltmp");
				}
				else if (LITERAL_TYPE_ENUM == vecType)
				{
					builder->CreateCall(module->getFunction("__vec_append_enum"), { dstType, dst, rhs.value }, "calltmp");
				}
				else if (LITERAL_TYPE_BOOL == vecType)
				{
					builder->CreateCall(module->getFunction("__vec_append_bool"), { dstType, dst, rhs.value }, "calltmp");
				}
				else if (LITERAL_TYPE_STRING == vecType)
				{
					llvm::Value* tmp = builder->CreateLoad(builder->getPtrTy(), rhs.value, "loadtmp");
					builder->CreateCall(module->getFunction("__vec_append_str"), { dstType, dst, tmp }, "calltmp");
				}
				else if (LITERAL_TYPE_UDT == vecType)
				{
					llvm::Value* tmp = builder->CreateLoad(builder->getPtrTy(), rhs.value, "loadtmp");
					builder->CreateCall(module->getFunction("__vec_append_udt"), { dstType, dst, tmp }, "calltmp");
				}

			}
		}
	}
	else if (0 == name.compare("vec::contains"))
	{
		if (2 == m_arguments.size() && EXPRESSION_VARIABLE == m_arguments[0]->GetType())
		{
			VariableExpr* ve = static_cast<VariableExpr*>(m_arguments[0]);
			std::string var = ve->Operator()->Lexeme();
			llvm::Value* defval = env->GetVariable(var);
			llvm::Type* defty = env->GetVariableTy(var);
			LiteralTypeEnum varType = env->GetVariableType(var);
			LiteralTypeEnum vecType = env->GetVecType(var);

			//printf("%lu, %d, %d\n", defval, varType, vecType);

			if (LITERAL_TYPE_VEC == varType)
			{
				TValue rhs = m_arguments[1]->codegen(context, builder, module, env);

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
		}
	}
	else if (0 == name.compare("vec::fill"))
	{
		if (2 == m_arguments.size())
		{
			TValue lhs = m_arguments[0]->codegen(context, builder, module, env);
			TValue rhs = m_arguments[1]->codegen(context, builder, module, env);

			if (rhs.IsInteger())
			{
				if (lhs.IsInteger())
				{
					llvm::Value* v = builder->CreateCall(module->getFunction("__vec_new_rep_i32"), { lhs.value, rhs.value }, "calltmp");
					llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
					builder->CreateStore(v, a);
					TValue ret = TValue::Vec(a, LITERAL_TYPE_INTEGER, "");
					env->AddToCleanup(ret);
					return ret;
				}
				else if (lhs.IsBool())
				{
					llvm::Value* v = builder->CreateCall(module->getFunction("__vec_new_rep_bool"), { lhs.value, rhs.value }, "calltmp");
					llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
					builder->CreateStore(v, a);
					TValue ret = TValue::Vec(a, LITERAL_TYPE_BOOL, "");
					env->AddToCleanup(ret);
					return ret;
				}
				if (lhs.IsEnum())
				{
					llvm::Value* v = builder->CreateCall(module->getFunction("__vec_new_rep_enum"), { lhs.value, rhs.value }, "calltmp");
					llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
					builder->CreateStore(v, a);
					TValue ret = TValue::Vec(a, LITERAL_TYPE_ENUM, "");
					env->AddToCleanup(ret);
					return ret;
				}
				else if (lhs.IsDouble())
				{
					llvm::Value* v = builder->CreateCall(module->getFunction("__vec_new_rep_double"), { lhs.value, rhs.value }, "calltmp");
					llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
					builder->CreateStore(v, a);
					TValue ret = TValue::Vec(a, LITERAL_TYPE_DOUBLE, "");
					env->AddToCleanup(ret);
					return ret;
				}
			}
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
				printf("Argument count mismatch for call: %s\n", name.c_str());
				return TValue::NullInvalid();
			}

			std::vector<llvm::Value*> args;
			for (size_t i = 0; i < m_arguments.size(); ++i)
			{
				TValue v = m_arguments[i]->codegen(context, builder, module, env);
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
				if (v.IsString() || v.IsVec()) v.value = builder->CreateLoad(builder->getPtrTy(), v.value, "loadtmp");
				args.push_back(v.value);
			}
			llvm::Value* rval = builder->CreateCall(ftn, args, std::string("call_" + name).c_str());
			LiteralTypeEnum retType = env->GetFunctionReturnType(name);
			switch (retType)
			{
			case LITERAL_TYPE_INTEGER:
				return TValue::Integer(rval);
			case LITERAL_TYPE_DOUBLE:
				return TValue::Double(rval);
			case LITERAL_TYPE_BOOL:
				return TValue::Bool(rval);
			case LITERAL_TYPE_ENUM:
				return TValue::Enum(rval);
			}

			return TValue(LITERAL_TYPE_INVALID, rval);
		}
	}

	return TValue::NullInvalid();

}



//-----------------------------------------------------------------------------
TValue GetExpr::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("GetExpr::codegen()\n");

	TValue ret = TValue::NullInvalid();

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
			printf("Error parsing UDT members\n");
			return TValue::NullInvalid();
		}

		args.push_back(builder->getInt32(idx));

		Token* token = env->GetUdtMemberTokenAt(udt_name, idx);
		if (!token)
		{
			printf("Invalid UDT member token\n");
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

	return ret;
}


//-----------------------------------------------------------------------------
TValue LiteralExpr::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("LiteralExpr::codegen()\n");

	LiteralTypeEnum lt = m_literal.GetType();

	if (LITERAL_TYPE_INTEGER == lt)
	{
		return TValue(lt, builder->getInt32(m_literal.IntValue()));
	}
	else if (LITERAL_TYPE_ENUM == lt)
	{
		return TValue(lt, builder->getInt32(env->GetEnumAsInt(m_literal.EnumValue().enumValue)));
	}
	else if (LITERAL_TYPE_BOOL == lt)
	{
		if (m_literal.BoolValue())
			return TValue(lt, builder->getTrue());
		else
			return TValue(lt, builder->getFalse());
	}
	else if (LITERAL_TYPE_DOUBLE == lt)
	{
		return TValue(lt, llvm::ConstantFP::get(*context, llvm::APFloat(m_literal.DoubleValue())));
	}
	else if (LITERAL_TYPE_STRING == lt)
	{
		llvm::Value* global_string = builder->CreateGlobalStringPtr(m_literal.ToString(), "strtmp");
		llvm::Value* p = builder->CreateCall(module->getFunction("__new_std_string_ptr"), { global_string }, "strptr");
		llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
		builder->CreateStore(p, a);
		TValue ret(lt, a);
		env->AddToCleanup(ret);
		return ret;
	}

	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue LogicalExpr::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("LogicalExpr::codegen()\n");

	if (!m_left || !m_right) return TValue::NullInvalid();
	TValue lhs = m_left->codegen(context, builder, module, env);
	TValue rhs = m_right->codegen(context, builder, module, env);
	if (!lhs.value || !rhs.value) return TValue::NullInvalid();

	switch (m_token->GetType())
	{
	case TOKEN_AND:
		return TValue::Bool(builder->CreateAnd(lhs.value, rhs.value, "logical_and_cmp"));
		break;

	case TOKEN_OR:
		return TValue::Bool(builder->CreateOr(lhs.value, rhs.value, "logical_or_cmp"));
		break;
	}

	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue RangeExpr::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("RangeExpr::codegen()\n");

	if (!m_left || !m_right) return TValue::NullInvalid();
	TValue lhs = m_left->codegen(context, builder, module, env);
	TValue rhs = m_right->codegen(context, builder, module, env);
	if (!lhs.value || !rhs.value) return TValue::NullInvalid();

	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue SetExpr::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("SetExpr::codegen()\n");

	TValue lhs = m_object->codegen(context, builder, module, env);
	
	TValue rhs = m_value->codegen(context, builder, module, env);
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
		printf("Type mismatch in SetExpr\n");
	}
	

	return TValue::NullInvalid();
}

//-----------------------------------------------------------------------------
TValue UnaryExpr::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("UnaryExpr::codegen()\n");

	TValue rhs = m_right->codegen(context, builder, module, env);

	switch (m_token->GetType())
	{
	case TOKEN_MINUS:
		if (rhs.IsInteger())
			return TValue::Integer(builder->CreateNeg(rhs.value, "negtmp"));
		else if (rhs.IsDouble())
			return TValue::Double(builder->CreateFNeg(rhs.value, "negtmp"));
		break;

	case TOKEN_BANG:
		// convert gep to raw value
		if (EXPRESSION_GET == m_right->GetType())
		{
			llvm::Type* ty = nullptr;
			if (rhs.IsInteger()) ty = builder->getInt32Ty();
			else if (rhs.IsBool()) ty = builder->getInt1Ty();

			if (ty) rhs = TValue(rhs.type, builder->CreateLoad(ty, rhs.value, "gep_load"));
		}

		if (rhs.IsInteger())
		{
			return TValue::Integer(builder->CreateICmpEQ(rhs.value, llvm::Constant::getNullValue(builder->getInt32Ty()), "zero_check"));
		}
		else if (rhs.IsBool())
		{
			return TValue::Bool(builder->CreateNot(rhs.value, "nottmp"));
		}
		break;
	}

	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue VariableExpr::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("VariableExpr::codegen()\n");
	std::string var = m_token->Lexeme();
	llvm::Value* defval = env->GetVariable(var);
	llvm::Type* defty = env->GetVariableTy(var);
	LiteralTypeEnum varType = env->GetVariableType(var);

	if (LITERAL_TYPE_STRING == varType)
	{
		return TValue(varType, builder->CreateGEP(defty, defval, builder->getInt8(0), "geptmp"));
	}
	else if (LITERAL_TYPE_VEC == varType && nullptr != m_vecIndex)
	{
		LiteralTypeEnum vecType = env->GetVecType(var);

		TValue idx = m_vecIndex->codegen(context, builder, module, env);

		llvm::Value* src = builder->CreateLoad(defty, defval, "vecsrc");

		if (LITERAL_TYPE_INTEGER == vecType)
		{
			return TValue(vecType, builder->CreateCall(module->getFunction("__vec_get_i32"), { src, idx.value }, "calltmp"));
		}
		else if (LITERAL_TYPE_DOUBLE == vecType)
		{
			return TValue(vecType, builder->CreateCall(module->getFunction("__vec_get_double"), { src, idx.value }, "calltmp"));
		}
		else if (LITERAL_TYPE_ENUM == vecType)
		{
			return TValue(vecType, builder->CreateCall(module->getFunction("__vec_get_enum"), { src, idx.value }, "calltmp"));
		}
		else if (LITERAL_TYPE_BOOL == vecType)
		{
			return TValue(vecType, builder->CreateCall(module->getFunction("__vec_get_bool"), { src, idx.value }, "calltmp"));
		}
		else if (LITERAL_TYPE_STRING == vecType)
		{
			llvm::Value* s = builder->CreateCall(module->getFunction("__vec_get_str"), { src, idx.value }, "calltmp");
			llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
			builder->CreateStore(s, a);
			TValue ret = TValue::String(a);
			env->AddToCleanup(ret);
			return ret;
		}

	}
	else if (LITERAL_TYPE_VEC == varType && !m_vecIndex)
	{
		//return TValue::Vec(builder->CreateLoad(defty, defval, "loadtmp"), env->GetVecType(var));
		return TValue::Vec(defval, env->GetVecType(var));
	}
	else if (LITERAL_TYPE_POINTER == varType)
	{
		return TValue::Pointer(builder->CreateLoad(defty, defval, "loadtmp"));
	}
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

	return TValue::NullInvalid();
}