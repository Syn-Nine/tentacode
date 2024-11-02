#include "Expressions.h"
#include <deque>

#define PI 3.1415926535897932384626433832795028841971693993751058
#define RAD2DEG (180.0 / PI)
#define DEG2RAD (1 / RAD2DEG)

//-----------------------------------------------------------------------------
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
		builder->CreateCall(module->getFunction("__str_assign"), { defval, rhs.value }, "calltmp");
	}
	else if (LITERAL_TYPE_INTEGER == varType || LITERAL_TYPE_BOOL == varType || LITERAL_TYPE_DOUBLE == varType || LITERAL_TYPE_ENUM == varType || LITERAL_TYPE_POINTER == varType)
	{
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

		}
		else
		{
			// overwrite entire vector
			if (rhs.IsFixedVec())
			{
				TValue b = TValue(LITERAL_TYPE_INTEGER, builder->CreateLoad(defty, defval, "geptmp"));
				llvm::Value* dstType = llvm::ConstantInt::get(*context, llvm::APInt(32, vecType, true));
				llvm::Value* srcType = llvm::ConstantInt::get(*context, llvm::APInt(32, rhs.fixed_vec_type, true));
				llvm::Value* srcQty = llvm::ConstantInt::get(*context, llvm::APInt(32, rhs.fixed_vec_sz, true));
				llvm::Value* dstPtr = builder->CreateLoad(defty, defval, "vecdst");
				llvm::Value* srcPtr = rhs.value;
				builder->CreateCall(module->getFunction("__vec_assign_fixed"), { dstType, dstPtr, srcType, srcPtr, srcQty }, "calltmp");
			}
			else if (rhs.IsVec())
			{
				TValue b = TValue(LITERAL_TYPE_INTEGER, builder->CreateLoad(defty, defval, "geptmp"));
				llvm::Value* dstType = llvm::ConstantInt::get(*context, llvm::APInt(32, vecType, true));
				llvm::Value* srcType = llvm::ConstantInt::get(*context, llvm::APInt(32, rhs.fixed_vec_type, true));
				llvm::Value* dstPtr = b.value;
				llvm::Value* srcPtr = rhs.value;
				builder->CreateCall(module->getFunction("__vec_assign"), { dstType, dstPtr, srcType, srcPtr }, "calltmp");
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
		else if (lhs.IsString()) ty = builder->getPtrTy();
		
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
				return TValue::Integer(builder->CreateCall(module->getFunction("__str_to_int"), { lhs.value }, "calltmp"));
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
				return TValue::Double(builder->CreateCall(module->getFunction("__str_to_double"), { lhs.value }, "calltmp"));
			}
		}
		else if (TOKEN_VAR_STRING == new_type)
		{
			if (lhs.IsString()) return lhs;

			if (lhs.IsInteger())
			{
				TValue ret = TValue::String(builder->CreateCall(module->getFunction("__int_to_string"), { lhs.value }, "calltmp"));
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
				TValue ret = TValue::String(builder->CreateCall(module->getFunction("__double_to_string"), { lhs.value }, "calltmp"));
				env->AddToCleanup(ret);
				return ret;
			}
			else if (lhs.IsBool())
			{
				TValue ret = TValue::String(builder->CreateCall(module->getFunction("__bool_to_string"), { lhs.value }, "calltmp"));
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
		else if (rhs.IsString()) ty = builder->getPtrTy();

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
			TValue ret = TValue::String(builder->CreateCall(module->getFunction("__std_string_cat"), { lhs.value, rhs.value }, "calltmp"));
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
			llvm::Value* tmp = builder->CreateCall(module->getFunction("__str_cmp"), { lhs.value, rhs.value }, "calltmp");
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
			return TValue::Bool(builder->CreateCall(module->getFunction("__str_cmp"), { lhs.value, rhs.value }, "calltmp"));
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
		0 == name.compare("atan"))
	{
		TValue v = m_arguments[0]->codegen(context, builder, module, env);
		if (v.IsInteger()) v = TValue::Double(builder->CreateSIToFP(v.value, builder->getDoubleTy(), "cast_to_dbl"));

		return TValue::Double(builder->CreateCall(module->getFunction(name), { v.value }, "calltmp"));
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
		if (1 == m_arguments.size() && EXPRESSION_VARIABLE == m_arguments[0]->GetType())
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
				llvm::Value* srcType = llvm::ConstantInt::get(*context, llvm::APInt(32, vecType, true));
				return TValue::Integer(builder->CreateCall(module->getFunction("__vec_len"), { srcType, src }, "calltmp"));
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
				llvm::Value* dstType = llvm::ConstantInt::get(*context, llvm::APInt(32, vecType, true));

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
		}
		else
		{
			llvm::Value* gep = builder->CreateGEP(defty, defval, args, "get_member");
			if (TOKEN_VAR_I32 == ttype) ret = TValue::Integer(gep);
			else if (TOKEN_VAR_ENUM == ttype) ret = TValue::Enum(gep);
			else if (TOKEN_VAR_BOOL == ttype) ret = TValue::Bool(gep);
			else if (TOKEN_VAR_F32 == ttype) ret = TValue::Double(gep);
			else if (TOKEN_VAR_STRING == ttype) ret = TValue::String(gep);
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
		return TValue(lt, llvm::ConstantInt::get(*context, llvm::APInt(32, m_literal.IntValue(), true)));
	}
	else if (LITERAL_TYPE_ENUM == lt)
	{
		return TValue(lt, llvm::ConstantInt::get(*context, llvm::APInt(32, env->GetEnumAsInt(m_literal.EnumValue().enumValue), true)));
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
		TValue ret(lt, p);
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
		builder->CreateStore(rhs.value, lhs.value);
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

	}
	else if (LITERAL_TYPE_VEC == varType && !m_vecIndex)
	{
		return TValue::Vec(builder->CreateLoad(defty, defval, "loadtmp"), env->GetVecType(var));
	}
	else if (LITERAL_TYPE_POINTER == varType)
	{
		return TValue::Pointer(builder->CreateLoad(defty, defval, "loadtmp"));
	}
	else
	{
		return TValue(varType, builder->CreateLoad(defty, defval, "loadtmp"));
	}

	return TValue::NullInvalid();
}