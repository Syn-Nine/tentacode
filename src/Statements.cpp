#include "Statements.h"


//-----------------------------------------------------------------------------
TValue BlockStmt::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("BlockStmt::codegen()\n");

	//llvm::Function* ftn = builder->GetInsertBlock()->getParent();
	//llvm::BasicBlock* after = llvm::BasicBlock::Create(*context, "after_block", ftn);

	Environment* sub_env = new Environment(env);

	for (auto& statement : *m_block)
	{
		if (env->HasErrors()) break;
		if (STATEMENT_FUNCTION != statement->GetType())
		{
			statement->codegen(context, builder, module, sub_env);
		}
	}

	sub_env->EmitCleanup(builder, module);
	delete sub_env;

	//builder->CreateBr(after);
	//builder->SetInsertPoint(after);

	return TValue::NullInvalid();
}


//-----------------------------------------------------------------------------
TValue BreakStmt::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("ExpressionStmt::codegen()\n");

	llvm::BasicBlock* bb = env->GetParentLoopBreak();
	if (bb)
	{
		// redirect
		builder->CreateBr(bb);

		// for follow-on non-redirected code insertion
		llvm::Function* ftn = builder->GetInsertBlock()->getParent();
		llvm::BasicBlock* tail = llvm::BasicBlock::Create(*context, "breaktail", ftn);
		builder->SetInsertPoint(tail);
	}
	else
	{
		env->Error(m_keyword, "No parent block for `break`.");
	}

	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue ContinueStmt::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("ExpressionStmt::codegen()\n");

	llvm::BasicBlock* bb = env->GetParentLoopContinue();
	if (bb)
	{
		// redirect
		builder->CreateBr(bb);

		// for follow-on non-redirected code insertion
		llvm::Function* ftn = builder->GetInsertBlock()->getParent();
		llvm::BasicBlock* tail = llvm::BasicBlock::Create(*context, "conttail", ftn);
		builder->SetInsertPoint(tail);
	}

	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue ExpressionStmt::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("ExpressionStmt::codegen()\n");
	return m_expr->codegen(context, builder, module, env);
}



//-----------------------------------------------------------------------------
TValue FunctionStmt::codegen_prototype(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("FunctionStmt::codegen_prototype()\n");

	std::string name = m_name->Lexeme();

	// check for existing function definition
	if (env->IsFunction(name))
	{
		printf("Function `%s` already defined!\n", name.c_str());
		return TValue::NullInvalid();
	}

	// build up the return and parameter types
	LiteralTypeEnum retliteral = LITERAL_TYPE_INVALID;
	llvm::Type* rettype = nullptr;
	if (!m_rettype)
	{
		rettype = builder->getVoidTy();
	}
	else
	{
		switch (m_rettype->GetType())
		{
		case TOKEN_VAR_I32:
			rettype = builder->getInt32Ty();
			retliteral = LITERAL_TYPE_INTEGER;
			break;
		case TOKEN_VAR_F32:
			rettype = builder->getDoubleTy();
			retliteral = LITERAL_TYPE_DOUBLE;
			break;
		case TOKEN_VAR_ENUM:
			rettype = builder->getInt32Ty();
			retliteral = LITERAL_TYPE_ENUM;
			break;
		case TOKEN_VAR_BOOL:
			rettype = builder->getInt1Ty();
			retliteral = LITERAL_TYPE_BOOL;
			break;
		}
	}

	std::vector<LiteralTypeEnum> types;
	std::vector<std::string> names;
	std::vector<Token*> tokens;

	std::vector<llvm::Type*> args;
	for (size_t i = 0; i < m_params.size(); ++i)
	{
		switch (m_types[i].GetType())
		{
		case TOKEN_VAR_I32:
			types.push_back(LITERAL_TYPE_INTEGER);
			tokens.push_back(new Token(m_params[i]));
			args.push_back(builder->getInt32Ty());
			break;

		case TOKEN_VAR_F32:
			types.push_back(LITERAL_TYPE_DOUBLE);
			tokens.push_back(new Token(m_params[i]));
			args.push_back(builder->getDoubleTy());
			break;

		case TOKEN_VAR_BOOL:
			types.push_back(LITERAL_TYPE_BOOL);
			tokens.push_back(new Token(m_params[i]));
			args.push_back(builder->getInt1Ty());
			break;

		case TOKEN_VAR_ENUM:
			types.push_back(LITERAL_TYPE_ENUM);
			tokens.push_back(new Token(m_params[i]));
			args.push_back(builder->getInt32Ty());
			break;
		}
		names.push_back(m_params[i].Lexeme());
	}

	llvm::FunctionType* FT = llvm::FunctionType::get(rettype, args, false);
	llvm::Function* ftn = llvm::Function::Create(FT, llvm::Function::InternalLinkage, name, *module);

	env->DefineFunction(name, ftn, types, names, args, tokens, retliteral);

	return TValue::NullInvalid();
}


TValue FunctionStmt::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("FunctionStmt::codegen()\n");

	std::string name = m_name->Lexeme();

	// check for existing function definition
	llvm::Function* ftn = env->GetFunction(name);
	if (!ftn)
	{
		printf("Function `%s` not defined!\n", name.c_str());
		return TValue::NullInvalid();
	}

	llvm::BasicBlock* body = llvm::BasicBlock::Create(*context, "entry", ftn);
	builder->SetInsertPoint(body);

	Environment* sub_env = new Environment(env);

	std::vector<LiteralTypeEnum> types = env->GetFunctionParamTypes(name);
	std::vector<std::string> names = env->GetFunctionParamNames(name);
	std::vector<llvm::Type*> args = env->GetFunctionArgTy(name);
	std::vector<Token*> tokens = env->GetFunctionParamTokens(name);

	int i = 0;
	for (auto& arg : ftn->args())
	{
		llvm::Value* defval = builder->CreateAlloca(args[i]);
		llvm::Type* defty = arg.getType();
		builder->CreateStore(&arg, defval);
		env->DefineVariable(types[i], names[i], defval, defty, tokens[i]);
		i = i + 1;
	}

	for (auto& statement : *m_body)
	{
		if (env->HasErrors()) break;
		if (STATEMENT_FUNCTION != statement->GetType())
		{
			statement->codegen(context, builder, module, sub_env);
		}
	}

	delete sub_env;

	LiteralTypeEnum rettype = env->GetFunctionReturnType(name);


	llvm::BasicBlock* tail = llvm::BasicBlock::Create(*context, "rettail", ftn);
	builder->CreateBr(tail);
	builder->SetInsertPoint(tail);

	if (LITERAL_TYPE_INVALID == rettype)
	{
		builder->CreateRetVoid();
	}
	else if (LITERAL_TYPE_INTEGER == rettype || LITERAL_TYPE_ENUM)
	{
		builder->CreateRet(llvm::Constant::getNullValue(builder->getInt32Ty()));
	}
	else if (LITERAL_TYPE_BOOL == rettype)
	{
		builder->CreateRet(llvm::Constant::getNullValue(builder->getInt1Ty()));
	}
	else if (LITERAL_TYPE_DOUBLE == rettype)
	{
		builder->CreateRet(llvm::Constant::getNullValue(builder->getDoubleTy()));
	}

	verifyFunction(*ftn);

	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue IfStmt::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("IfStmt::codegen()\n");
	TValue CondV = m_condition->codegen(context, builder, module, env);
	if (!CondV.value) return TValue::NullInvalid();

	llvm::Function* ftn = builder->GetInsertBlock()->getParent();

	llvm::BasicBlock* ThenBB = llvm::BasicBlock::Create(*context, "then", ftn);
	llvm::BasicBlock* ElseBB = llvm::BasicBlock::Create(*context, "else");
	llvm::BasicBlock* MergeBB = llvm::BasicBlock::Create(*context, "ifcont");

	builder->CreateCondBr(CondV.value, ThenBB, ElseBB);
	builder->SetInsertPoint(ThenBB);

	m_thenBranch->codegen(context, builder, module, env);

	builder->CreateBr(MergeBB);

	ftn->insert(ftn->end(), ElseBB);
	builder->SetInsertPoint(ElseBB);

	if (m_elseBranch) m_elseBranch->codegen(context, builder, module, env);

	builder->CreateBr(MergeBB);

	ftn->insert(ftn->end(), MergeBB);
	builder->SetInsertPoint(MergeBB);

	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue PrintStmt::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("PrintStmt::codegen()\n");

	if (!m_expr) // empty param list
	{
		if (m_newline)
			builder->CreateCall(module->getFunction("println"), { llvm::ConstantPointerNull::get(builder->getPtrTy()) }, "calltmp");
		else
			builder->CreateCall(module->getFunction("print"), { llvm::ConstantPointerNull::get(builder->getPtrTy()) }, "calltmp");

		return TValue::NullInvalid();
	}

	TValue v = m_expr->codegen(context, builder, module, env);

	// convert gep to raw value
	if (EXPRESSION_GET == m_expr->GetType())
	{
		llvm::Type* ty = nullptr;
		if (v.IsInteger() || v.IsEnum()) ty = builder->getInt32Ty();
		else if (v.IsDouble()) ty = builder->getDoubleTy();
		else if (v.IsBool()) ty = builder->getInt1Ty();
		
		if (ty) v = TValue(v.type, builder->CreateLoad(ty, v.value, "gep_load"));
	}

	if (!v.IsString())
	{
		if (v.IsInteger())
		{
			llvm::Value* s = builder->CreateCall(module->getFunction("__int_to_string"), { v.value }, "calltmp");
			llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
			builder->CreateStore(s, a);
			v = TValue::String(a);
			env->AddToCleanup(v);
		}
		else if (v.IsDouble())
		{
			llvm::Value* s = builder->CreateCall(module->getFunction("__double_to_string"), { v.value }, "calltmp");
			llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
			builder->CreateStore(s, a);
			v = TValue::String(a);
			env->AddToCleanup(v);
		}
		else if (v.IsBool())
		{
			llvm::Value* s = builder->CreateCall(module->getFunction("__bool_to_string"), { v.value }, "calltmp");
			llvm::Value* a = builder->CreateAlloca(builder->getPtrTy(), nullptr, "alloctmp");
			builder->CreateStore(s, a);
			v = TValue::String(a);
			env->AddToCleanup(v);
		}
	}

	llvm::Value* tmp = builder->CreateLoad(builder->getPtrTy(), v.value, "loadtmp");
	if (m_newline)
		builder->CreateCall(module->getFunction("println"), { tmp }, "calltmp");
	else
		builder->CreateCall(module->getFunction("print"), { tmp }, "calltmp");

	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue ReturnStmt::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("ReturnStmt::codegen()\n");

	TValue v = m_value->codegen(context, builder, module, env);

	llvm::Function* ftn = builder->GetInsertBlock()->getParent();
	llvm::Type* retty = ftn->getReturnType();

	if (retty->isIntegerTy() && v.IsDouble())
	{
		// convert rhs to int
		v = TValue::Integer(builder->CreateFPToSI(v.value, builder->getInt32Ty(), "int_cast_tmp"));
	}
	else if (retty->isDoubleTy() && v.IsInteger())
	{
		// convert rhs to double
		v = TValue::Double(builder->CreateSIToFP(v.value, builder->getDoubleTy(), "cast_to_dbl"));
	}

	builder->CreateRet(v.value);

	llvm::BasicBlock* tail = llvm::BasicBlock::Create(*context, "rettail", ftn);
	builder->SetInsertPoint(tail);

	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue StructStmt::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("StructStmt::codegen()\n");

	std::vector<llvm::Type*> arg_ty;
	std::vector<Token*> arg_types;
	std::vector<LiteralTypeEnum> arg_vec_types;
	std::vector<std::string> arg_vec_types_id;
	std::vector<std::string> arg_names;

	for (auto& statement : *m_vars)
	{
		if (env->HasErrors()) break;
		if (STATEMENT_VAR == statement->GetType())
		{
			VarStmt* vs = static_cast<VarStmt*>(statement);

			std::string id = vs->Operator()->Lexeme();
			TokenTypeEnum vtype = vs->VarType()->GetType();
			switch (vtype)
			{
			case TOKEN_VAR_ENUM: // intentional fall-through
			case TOKEN_VAR_I32:
				arg_ty.push_back(builder->getInt32Ty());
				break;
			case TOKEN_VAR_BOOL:
				arg_ty.push_back(builder->getInt1Ty());
				break;
			case TOKEN_VAR_F32:
				arg_ty.push_back(builder->getDoubleTy());
				break;
			case TOKEN_VAR_STRING:
				arg_ty.push_back(builder->getPtrTy());
				break;
			case TOKEN_IDENTIFIER:
			{
				llvm::Type* udt_ty = env->GetUdt("struct." + vs->VarType()->Lexeme());
				if (udt_ty)
					arg_ty.push_back(udt_ty);
				else
					printf("Invalid structure member UDT.\n");
				break;
			}
			case TOKEN_VAR_VEC:
				arg_ty.push_back(builder->getPtrTy());
				break;

			default:
				printf("Invalid structure member type.\n");
				break;
			}
			arg_vec_types.push_back(vs->VarVecType());
			arg_vec_types_id.push_back(vs->VarVecTypeId());
			arg_types.push_back(vs->VarType());
			arg_names.push_back(id);
		}
	}

	if (!arg_ty.empty())
	{
		std::string id = "struct." + m_name->Lexeme();
		llvm::StructType* stype = llvm::StructType::create(*context, arg_ty, id);
		env->DefineUdt(id, stype, arg_types, arg_names, arg_ty, arg_vec_types, arg_vec_types_id);
	}

	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
void init_udt(std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env,
	std::string udtname,
	llvm::Value* defval, llvm::Type* defty, std::vector<llvm::Value*> args)
{
	if (!env->GetUdt(udtname)) return;
	
	std::vector<llvm::Type*> arg_ty = env->GetUdtTy(udtname);
	std::vector<Token*> arg_var_tokens = env->GetUdtMemberTokens(udtname);
	std::vector<std::string> arg_var_names = env->GetUdtMemberNames(udtname);
	std::vector<LiteralTypeEnum> arg_var_vec_types = env->GetUdtMemberVecTypes(udtname);
	std::vector<std::string> arg_var_vec_type_ids = env->GetUdtMemberVecTypeIds(udtname);
	
	for (size_t i = 0; i < arg_ty.size(); ++i)
	{
		std::vector<llvm::Value*> argtmp = args;
		argtmp.push_back(builder->getInt32(i));
		TokenTypeEnum argVarType = arg_var_tokens[i]->GetType();

		if (TOKEN_IDENTIFIER == argVarType)
		{
			init_udt(builder, module, env, "struct." + arg_var_tokens[i]->Lexeme(), defval, defty, argtmp);
		}
		else
		{
			llvm::Value* gep = builder->CreateGEP(defty, defval, argtmp, "gep_" + arg_var_tokens[i]->Lexeme());
			if (TOKEN_VAR_I32 == argVarType ||
				TOKEN_VAR_ENUM == argVarType) builder->CreateStore(builder->getInt32(0), gep);
			else if (TOKEN_VAR_BOOL == argVarType) builder->CreateStore(builder->getFalse(), gep);
			else if (TOKEN_VAR_F32 == argVarType) builder->CreateStore(llvm::Constant::getNullValue(builder->getDoubleTy()), gep);
			else if (TOKEN_VAR_STRING == argVarType)
			{
				llvm::Value* s = builder->CreateCall(module->getFunction("__new_std_string_void"), {}, "calltmp");
				builder->CreateStore(s, gep);
				TValue tmp = TValue::String(gep);
				env->AddToCleanup(tmp);
			}
			else if (TOKEN_VAR_VEC == argVarType)
			{
				LiteralTypeEnum vecType = arg_var_vec_types[i];
				std::string vecTypeId = arg_var_vec_type_ids[i];
				llvm::Value* addr = builder->CreateCall(module->getFunction("__vec_new"), { builder->getInt32(vecType) }, "calltmp");
				builder->CreateStore(addr, gep);
				TValue tmp = TValue::Vec(gep, vecType, vecTypeId);
				env->AddToCleanup(tmp);
			}
			else
			{
				printf("Initializer not present for VarStmt\n");
			}
		}
	}
}

TValue VarStmt::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("VarStmt::codegen()\n");

	TokenTypeEnum varType = m_type->GetType();
	LiteralTypeEnum vecType = m_vecType;

	llvm::Value* defval = nullptr;
	llvm::Type* defty = nullptr;

	bool global = !env->HasParent();

	if (TOKEN_VAR_I32 == varType)
	{
		defty = builder->getInt32Ty();
		if (global)
		{
			defval = new llvm::GlobalVariable(*module, defty, false, llvm::GlobalValue::InternalLinkage, builder->getInt32(0), m_token->Lexeme());
		}
		else
		{
			defval = builder->CreateAlloca(defty, nullptr, "alloc_i32");
			if (!m_expr) builder->CreateStore(builder->getInt32(0), defval);
		}
		env->DefineVariable(LITERAL_TYPE_INTEGER, m_token->Lexeme(), defval, defty, m_type);
	}
	else if (TOKEN_VAR_ENUM == varType)
	{
		defty = builder->getInt32Ty();
		if (global)
		{
			defval = new llvm::GlobalVariable(*module, defty, false, llvm::GlobalValue::InternalLinkage, builder->getInt32(0), m_token->Lexeme());
		}
		else
		{
			defval = builder->CreateAlloca(defty, nullptr, "alloc_enum_i32");
			if (!m_expr) builder->CreateStore(builder->getInt32(0), defval);
		}
		env->DefineVariable(LITERAL_TYPE_ENUM, m_token->Lexeme(), defval, defty, m_type);
	}
	else if (TOKEN_VAR_BOOL == varType)
	{
		defty = builder->getInt1Ty();
		if (global)
		{
			defval = new llvm::GlobalVariable(*module, defty, false, llvm::GlobalValue::InternalLinkage, builder->getFalse(), m_token->Lexeme());
		}
		else
		{
			defval = builder->CreateAlloca(defty, nullptr, "alloc_bool");
			if (!m_expr) builder->CreateStore(builder->getFalse(), defval);
		}
		env->DefineVariable(LITERAL_TYPE_BOOL, m_token->Lexeme(), defval, defty, m_type);
	}
	else if (TOKEN_VAR_F32 == varType)
	{
		defty = builder->getDoubleTy();
		if (global)
		{
			defval = new llvm::GlobalVariable(*module, defty, false, llvm::GlobalValue::InternalLinkage, llvm::ConstantFP::get(*context, llvm::APFloat(0.0)), m_token->Lexeme());
		}
		else
		{
			defval = builder->CreateAlloca(defty, nullptr, "alloc_f64");
			if (!m_expr) builder->CreateStore(llvm::Constant::getNullValue(defty), defval);
		}
		env->DefineVariable(LITERAL_TYPE_DOUBLE, m_token->Lexeme(), defval, defty, m_type);
	}
	else if (TOKEN_VAR_STRING == varType)
	{
		defty = builder->getPtrTy();
		if (global)
		{
			defval = new llvm::GlobalVariable(*module, defty, false, llvm::GlobalValue::InternalLinkage, llvm::ConstantPointerNull::get(builder->getPtrTy()), m_token->Lexeme());
		}
		else
		{
			defval = builder->CreateAlloca(defty, nullptr, "alloc_ptr");

		}
		env->DefineVariable(LITERAL_TYPE_STRING, m_token->Lexeme(), defval, defty, m_type);
		llvm::Value* addr = builder->CreateCall(module->getFunction("__new_std_string_void"), { }, "calltmp");
		builder->CreateStore(addr, defval);
		env->AddToCleanup(TValue::String(defval));
	}
	else if (TOKEN_VAR_TEXTURE == varType || TOKEN_VAR_IMAGE == varType || TOKEN_VAR_SOUND == varType || TOKEN_VAR_FONT == varType)
	{
		defty = builder->getPtrTy();
		if (global)
		{
			defval = new llvm::GlobalVariable(*module, defty, false, llvm::GlobalValue::InternalLinkage, llvm::ConstantPointerNull::get(builder->getPtrTy()), m_token->Lexeme());
		}
		else
		{
			defval = builder->CreateAlloca(defty, nullptr, "alloc_ptr");

		}
		env->DefineVariable(LITERAL_TYPE_POINTER, m_token->Lexeme(), defval, defty, m_type);
	}
	else if (TOKEN_VAR_VEC == varType)
	{
		defty = builder->getPtrTy();
		if (global)
		{
			defval = new llvm::GlobalVariable(*module, defty, false, llvm::GlobalValue::InternalLinkage, llvm::ConstantPointerNull::get(builder->getPtrTy()), m_token->Lexeme());
		}
		else
		{
			defval = builder->CreateAlloca(defty, nullptr, "alloc_vec_ptr");

		}
		env->DefineVariable(LITERAL_TYPE_VEC, m_token->Lexeme(), defval, defty, m_type, vecType);
		llvm::Value* addr = builder->CreateCall(module->getFunction("__vec_new"), { builder->getInt32(vecType) }, "calltmp");
		builder->CreateStore(addr, defval);
		env->AddToCleanup(TValue::Vec(defval, vecType));
	}
	else if (TOKEN_IDENTIFIER == varType)
	{
		std::string udtname = "struct." + m_type->Lexeme();
		if (env->IsUdt(udtname))
		{
			defty = env->GetUdt(udtname);
			if (global)
			{
				//defval = new llvm::GlobalVariable(*module, defty, false, llvm::GlobalValue::InternalLinkage, builder->getInt64(0), m_token->Lexeme());
			}
			else
			{
				defval = builder->CreateAlloca(defty, nullptr, "alloc_udt");
				if (!m_expr)
				{
					init_udt(builder, module, env, udtname, defval, defty, { builder->getInt32(0) });
				}

			}
			env->DefineVariable(LITERAL_TYPE_UDT, m_token->Lexeme(), defval, defty, m_type);
		}
		else
		{
			env->Error(m_type, "Invalid identifier type.");
		}
	}

	if (defval && m_expr && EXPRESSION_ASSIGN == m_expr->GetType())
	{
		// run assignment expression
		m_expr->codegen(context, builder, module, env);
	}

	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue WhileStmt::codegen(std::unique_ptr<llvm::LLVMContext>& context,
	std::unique_ptr<llvm::IRBuilder<>>& builder,
	std::unique_ptr<llvm::Module>& module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("WhileStmt::codegen()\n");

	llvm::Function* ftn = builder->GetInsertBlock()->getParent();
	llvm::BasicBlock* LoopBB = llvm::BasicBlock::Create(*context, "loop", ftn);
	llvm::BasicBlock* BodyBB = llvm::BasicBlock::Create(*context, "body");
	llvm::BasicBlock* PostBB = llvm::BasicBlock::Create(*context, "post");
	llvm::BasicBlock* MergeBB = llvm::BasicBlock::Create(*context, "loopcont");

	env->PushLoopBreakContinue(MergeBB, PostBB);

	builder->CreateBr(LoopBB);
	builder->SetInsertPoint(LoopBB);

	TValue CondV = m_condition->codegen(context, builder, module, env);
	if (!CondV.value) return TValue::NullInvalid();

	builder->CreateCondBr(CondV.value, BodyBB, MergeBB);

	ftn->insert(ftn->end(), BodyBB);
	builder->SetInsertPoint(BodyBB);

	if (m_body) m_body->codegen(context, builder, module, env);

	builder->CreateBr(PostBB); // go back to the top

	ftn->insert(ftn->end(), PostBB);
	builder->SetInsertPoint(PostBB);

	if (m_post) m_post->codegen(context, builder, module, env);

	builder->CreateBr(LoopBB); // go back to the top

	ftn->insert(ftn->end(), MergeBB);
	builder->SetInsertPoint(MergeBB);

	env->PopLoopBreakContinue();

	return TValue::NullInvalid();
}
