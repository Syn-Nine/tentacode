#include "Statements.h"
#include "TFunction.h"
#include "TStruct.h"

//-----------------------------------------------------------------------------
TValue BlockStmt::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("BlockStmt::codegen()\n");

	llvm::Value* stack = builder->CreateStackSave("stksave");

	Environment* sub_env = Environment::Push();

	for (auto& statement : *m_block)
	{
		if (env->HasErrors()) break;
		if (STATEMENT_FUNCTION != statement->GetType() &&
			STATEMENT_STRUCT != statement->GetType())
		{
			statement->codegen(builder, module, sub_env);
		}
	}

	Environment::Pop();

	builder->CreateStackRestore(stack, "stkrest");

	return TValue::NullInvalid();
}


//-----------------------------------------------------------------------------
TValue BreakStmt::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("ExpressionStmt::codegen()\n");
	
	llvm::BasicBlock* bb = env->GetParentLoopBreak();
	if (bb)
	{
		// redirect
		builder->CreateBr(bb);

		// for follow-on non-redirected code insertion
		llvm::Function* ftn = builder->GetInsertBlock()->getParent();
		llvm::BasicBlock* tail = llvm::BasicBlock::Create(module->getContext(), "breaktail", ftn);
		builder->SetInsertPoint(tail);
	}
	else
	{
		env->Error(m_keyword, "No parent block for 'break'.");
	}
	
	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue ContinueStmt::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("ExpressionStmt::codegen()\n");
	
	llvm::BasicBlock* bb = env->GetParentLoopContinue();
	if (bb)
	{
		// redirect
		builder->CreateBr(bb);

		// for follow-on non-redirected code insertion
		llvm::Function* ftn = builder->GetInsertBlock()->getParent();
		llvm::BasicBlock* tail = llvm::BasicBlock::Create(module->getContext(), "conttail", ftn);
		builder->SetInsertPoint(tail);
	}
	else
	{
		env->Error(m_keyword, "No parent block for 'continue'.");
	}
	
	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue ExpressionStmt::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("ExpressionStmt::codegen()\n");
	return m_expr->codegen(builder, module, env);
}


//-----------------------------------------------------------------------------
TValue ForEachStmt::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("ForEachStmt::codegen()\n");

	BlockStmt* body = static_cast<BlockStmt*>(m_body);

	// for-each is wrapped in a block statement so it comes wrapped in a local environment scope

	// todo clean up redundant code
	// create loop blocks
	llvm::Function* ftn = builder->GetInsertBlock()->getParent();
	llvm::LLVMContext& context = builder->getContext();
	llvm::BasicBlock* LoopBB = llvm::BasicBlock::Create(context, "loop", ftn);
	llvm::BasicBlock* BodyBB = llvm::BasicBlock::Create(context, "body");
	llvm::BasicBlock* PostBB = llvm::BasicBlock::Create(context, "post");
	llvm::BasicBlock* MergeBB = llvm::BasicBlock::Create(context, "loopcont");

	env->PushLoopBreakContinue(MergeBB, PostBB);

	if (EXPRESSION_RANGE == m_expr->GetType()) // rhs is range expression
	{
		// make local variable
		Token* var = m_val;
		std::string lexeme = var->Lexeme();
		if (lexeme.compare("_") == 0) lexeme.append("%");
		TType ttype = TType::Construct_Int(var, 32);
		TValue val = TValue::Construct(ttype, lexeme, false);
		if (!val.IsValid()) return TValue::NullInvalid();

		env->DefineVariable(val, lexeme);

		// evalue range
		TValue rngLeft = static_cast<RangeExpr*>(m_expr)->Left()->codegen(builder, module, env).GetFromStorage();
		if (!rngLeft.IsValid() || !rngLeft.IsInteger())
		{
			env->Error(rngLeft.GetToken(), "Invalid range left side bound.");
			return TValue::NullInvalid();
		}
		TValue rngRight = static_cast<RangeExpr*>(m_expr)->Right()->codegen(builder, module, env).GetFromStorage();
		if (!rngRight.IsValid() || !rngRight.IsInteger())
		{
			env->Error(rngRight.GetToken(), "Invalid range right side bound.");
			return TValue::NullInvalid();
		}
		rngRight = rngRight.CastToMatchImplicit(rngLeft);

		// assign vector value at index to val variable
		env->AssignToVariable(var, lexeme, rngLeft);

		// add outer loop
		builder->CreateBr(LoopBB);
		builder->SetInsertPoint(LoopBB);

		// make a comparison expression
		TValue CondV = TValue::MakeBool(var, builder->CreateICmpSLT(val.GetFromStorage().Value(), rngRight.Value(), "cmptmp"));
		if (!CondV.Value()) return TValue::NullInvalid();
		builder->CreateCondBr(CondV.GetFromStorage().Value(), BodyBB, MergeBB);

		// add inner block
		ftn->insert(ftn->end(), BodyBB);
		builder->SetInsertPoint(BodyBB);
		if (m_body) m_body->codegen(builder, module, env);

		// add increment section
		builder->CreateBr(PostBB);
		ftn->insert(ftn->end(), PostBB); // continue statements arrive here or fall-through from inner block
		builder->SetInsertPoint(PostBB);

		TValue plus = val.GetFromStorage();
		plus.SetValue(builder->CreateAdd(plus.Value(), builder->getInt64(1), "addtmp"));
		val.Store(plus);

	}
	else
	{
		// evaluate the container
		TValue rhs = m_expr->codegen(builder, module, env);
		if (!rhs.IsValid()) return TValue::NullInvalid();
		
		if (!rhs.CanIterate()) // normal for loop with counter
		{
			if (!rhs.IsInteger())
			{
				env->Error(rhs.GetToken(), "Cannot iterate over type.");
				return TValue::NullInvalid();
			}

			// rhs evaluated to an integer so do normal loop
			// make local variable
			Token* var = m_val;
			std::string lexeme = var->Lexeme();
			if (lexeme.compare("_") == 0) lexeme.append("%");
			TType ttype = TType::Construct_Int(var, 32);
			TValue val = TValue::Construct(ttype, lexeme, false);
			if (!val.IsValid()) return TValue::NullInvalid();

			env->DefineVariable(val, lexeme);
			builder->CreateStore(builder->getInt32(0), val.Value()); // re-initialize counter to zero

			// evaluate range
			TValue rngLeft = TValue::MakeInt(var, 32, builder->getInt32(0));
			TValue rngRight = rhs.GetFromStorage().CastToMatchImplicit(rngLeft);

			// assign vector value at index to val variable
			env->AssignToVariable(var, lexeme, rngLeft);

			// add outer loop
			builder->CreateBr(LoopBB);
			builder->SetInsertPoint(LoopBB);

			// make a comparison expression
			TValue CondV = TValue::MakeBool(var, builder->CreateICmpSLT(val.GetFromStorage().Value(), rngRight.Value(), "cmptmp"));
			if (!CondV.Value()) return TValue::NullInvalid();
			builder->CreateCondBr(CondV.GetFromStorage().Value(), BodyBB, MergeBB);

			// add inner block
			ftn->insert(ftn->end(), BodyBB);
			builder->SetInsertPoint(BodyBB);
			if (m_body) m_body->codegen(builder, module, env);

			// add increment section
			builder->CreateBr(PostBB);
			ftn->insert(ftn->end(), PostBB); // continue statements arrive here or fall-through from inner block
			builder->SetInsertPoint(PostBB);

			TValue plus = val.GetFromStorage();
			plus.SetValue(builder->CreateAdd(plus.Value(), builder->getInt64(1), "addtmp"));
			val.Store(plus);

		}
		else
		{
			TType ttype = rhs.GetTType();
			if (ttype.IsVecAny())
			{
				// len variable
				TValue len = rhs.EmitLen().GetFromStorage();

				// key variable
				Token* keytoken = m_key;
				if (!keytoken) keytoken = new Token(TOKEN_IDENTIFIER, "__loop_idx%", m_val->Line(), m_val->Filename()); // anonymous key		
				TType keyttype = TType::Construct_Int(keytoken, 64);
				std::string key_lex = keytoken->Lexeme();
				if (key_lex.compare("_") == 0) key_lex.append("%");
				TValue key = TValue::Construct(keyttype, key_lex, false);
				if (!key.IsValid()) return TValue::NullInvalid();
				env->DefineVariable(key, key_lex);
				builder->CreateStore(builder->getInt64(0), key.Value()); // re-initialize counter to zero incase loop is inside a loop

				// val variable
				std::string val_lex = m_val->Lexeme();
				if (val_lex.compare("_") == 0) val_lex.append("%");
				TValue val = TValue::Construct(ttype.GetInternal(0), val_lex, false);
				if (!val.IsValid()) return TValue::NullInvalid();
				env->DefineVariable(val, val_lex);

				// add outer loop
				builder->CreateBr(LoopBB);
				builder->SetInsertPoint(LoopBB);

				// make a comparison expression
				TValue CondV = TValue::MakeBool(keytoken, builder->CreateICmpSLT(key.GetFromStorage().Value(), len.Value(), "cmptmp"));
				if (!CondV.Value()) return TValue::NullInvalid();
				builder->CreateCondBr(CondV.GetFromStorage().Value(), BodyBB, MergeBB);

				// add inner block
				ftn->insert(ftn->end(), BodyBB);
				builder->SetInsertPoint(BodyBB);

				// assign vector value at index to val variable
				env->AssignToVariable(m_val, val_lex, rhs.GetAtIndex(key.GetFromStorage()));

				if (m_body) m_body->codegen(builder, module, env);

				// add increment section
				builder->CreateBr(PostBB);
				ftn->insert(ftn->end(), PostBB); // continue statements arrive here or fall-through from inner block
				builder->SetInsertPoint(PostBB);

				TValue plus = key.GetFromStorage();
				plus.SetValue(builder->CreateAdd(plus.Value(), builder->getInt64(1), "addtmp"));
				key.Store(plus);

				if (!m_key) delete keytoken;

			}
			else if (ttype.IsSet())
			{
				if (m_key)
				{
					env->Error(m_key, "Set type does not support key index.");
					return TValue::NullInvalid();
				}

				// len variable
				TValue len = rhs.EmitLen().GetFromStorage();

				// loop counter
				Token* cnttoken = new Token(TOKEN_IDENTIFIER, "__loop_idx%", m_val->Line(), m_val->Filename()); // anonymous counter
				TType cntttype = TType::Construct_Int(cnttoken, 64);
				TValue cnt = TValue::Construct(cntttype, cnttoken->Lexeme(), false);
				if (!cnt.IsValid()) return TValue::NullInvalid();
				builder->CreateStore(builder->getInt64(0), cnt.Value()); // re-initialize counter to zero incase loop is inside a loop

				// val variable
				std::string val_lex = m_val->Lexeme();
				if (val_lex.compare("_") == 0) val_lex.append("%");
				TValue val = TValue::Construct(ttype.GetInternal(0), val_lex, false);
				if (!val.IsValid()) return TValue::NullInvalid();
				env->DefineVariable(val, val_lex);

				rhs.EmitStartIterator();

				// add outer loop
				builder->CreateBr(LoopBB);
				builder->SetInsertPoint(LoopBB);

				// make a comparison expression
				TValue CondV = TValue::MakeBool(cnttoken, builder->CreateICmpSLT(cnt.GetFromStorage().Value(), len.Value(), "cmptmp"));
				if (!CondV.Value()) return TValue::NullInvalid();
				builder->CreateCondBr(CondV.GetFromStorage().Value(), BodyBB, MergeBB);

				// add inner block
				ftn->insert(ftn->end(), BodyBB);
				builder->SetInsertPoint(BodyBB);

				// assign vector value at index to val variable
				env->AssignToVariable(m_val, val_lex, rhs.GetIterValue());

				if (m_body) m_body->codegen(builder, module, env);

				// add increment section
				builder->CreateBr(PostBB);
				ftn->insert(ftn->end(), PostBB); // continue statements arrive here or fall-through from inner block
				builder->SetInsertPoint(PostBB);

				TValue plus = cnt.GetFromStorage();
				plus.SetValue(builder->CreateAdd(plus.Value(), builder->getInt64(1), "addtmp"));
				cnt.Store(plus);

				delete cnttoken;
			}
			else if (ttype.IsMap())
			{
				// len variable
				TValue len = rhs.EmitLen().GetFromStorage();

				// loop counter
				Token* cnttoken = new Token(TOKEN_IDENTIFIER, "__loop_idx%", m_val->Line(), m_val->Filename()); // anonymous counter
				TType cntttype = TType::Construct_Int(cnttoken, 64);
				TValue cnt = TValue::Construct(cntttype, cnttoken->Lexeme(), false);
				if (!cnt.IsValid()) return TValue::NullInvalid();
				builder->CreateStore(builder->getInt64(0), cnt.Value()); // re-initialize counter to zero incase loop is inside a loop

				// always populate key and value, set to "_" var if not requested
				
				// key variable
				std::string key_lex = "_";
				if (m_key) key_lex = m_key->Lexeme();
				if (key_lex.compare("_") == 0) key_lex.append("%");
				TValue key = TValue::Construct(ttype.GetInternal(0), key_lex, false);
				if (!key.IsValid()) return TValue::NullInvalid();
				env->DefineVariable(key, key_lex);

				// val variable
				std::string val_lex = m_val->Lexeme(); // always valid
				if (val_lex.compare("_") == 0) val_lex.append("%");
				TValue val = TValue::Construct(ttype.GetInternal(1), val_lex, false);
				if (!val.IsValid()) return TValue::NullInvalid();
				env->DefineVariable(val, val_lex);

				rhs.EmitStartIterator();

				// add outer loop
				builder->CreateBr(LoopBB);
				builder->SetInsertPoint(LoopBB);

				// make a comparison expression
				TValue CondV = TValue::MakeBool(cnttoken, builder->CreateICmpSLT(cnt.GetFromStorage().Value(), len.Value(), "cmptmp"));
				if (!CondV.Value()) return TValue::NullInvalid();
				builder->CreateCondBr(CondV.GetFromStorage().Value(), BodyBB, MergeBB);

				// add inner block
				ftn->insert(ftn->end(), BodyBB);
				builder->SetInsertPoint(BodyBB);

				// assign map value at index to val variable
				key = rhs.GetIterKey();
				if (m_key) env->AssignToVariable(m_key, key_lex, key);
				env->AssignToVariable(m_val, val_lex, rhs.GetAtIndex(key.GetFromStorage()));


				if (m_body) m_body->codegen(builder, module, env);

				// add increment section
				builder->CreateBr(PostBB);
				ftn->insert(ftn->end(), PostBB); // continue statements arrive here or fall-through from inner block
				builder->SetInsertPoint(PostBB);

				TValue plus = cnt.GetFromStorage();
				plus.SetValue(builder->CreateAdd(plus.Value(), builder->getInt64(1), "addtmp"));
				cnt.Store(plus);

				delete cnttoken;
			}
		}
	}

	// loop back to top or exit
	builder->CreateBr(LoopBB); // go back to the top

	ftn->insert(ftn->end(), MergeBB); // break statements and exit conditions arrive here
	builder->SetInsertPoint(MergeBB);

	env->PopLoopBreakContinue();

	return TValue::NullInvalid();
}


//-----------------------------------------------------------------------------
TValue FunctionStmt::codegen_prototype(llvm::IRBuilder<>* builder,
	llvm::Module* module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("FunctionStmt::codegen_prototype()\n");

	std::string name = m_name->Lexeme();

	// check for existing function definition
	if (env->GetFunction(m_name, name, true).IsValid())
	{
		env->Error(m_name, "Function already defined in environment.");
		return TValue::NullInvalid();
	}

	TFunction func = TFunction::Construct_Prototype(m_fqns, m_name, m_rettype, m_types, m_params, m_mutable, m_defaults, m_body);
	
	if (func.IsValid()) env->DefineFunction(func, name);
	
	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue FunctionStmt::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("FunctionStmt::codegen()\n");
	
	std::string name = m_name->Lexeme();

	// check for existing function definition
	TFunction func = env->GetFunction(m_name, m_fqns + ":" + name);
	if (func.IsValid()) func.Construct_Body();
	
	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue IfStmt::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	return codegen(builder, module, env, nullptr);
}

TValue IfStmt::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env, llvm::BasicBlock* elseifmerge)
{
	if (2 <= Environment::GetDebugLevel()) printf("IfStmt::codegen()\n");
	TValue CondV = m_condition->codegen(builder, module, env);
	if (!CondV.IsBool())
	{
		env->Error(m_token, "Expression is not boolean.");
		return TValue::NullInvalid();
	}
	if (!CondV.IsValid()) return TValue::NullInvalid();

	llvm::Function* ftn = builder->GetInsertBlock()->getParent();
	llvm::LLVMContext& context = builder->getContext();
	llvm::BasicBlock* ThenBB = llvm::BasicBlock::Create(context, "then", ftn);
	llvm::BasicBlock* ElseBB = llvm::BasicBlock::Create(context, "else");
	llvm::BasicBlock* MergeBB = elseifmerge;
	if (!MergeBB) MergeBB = llvm::BasicBlock::Create(context, "ifcont");

	builder->CreateCondBr(CondV.GetFromStorage().Value(), ThenBB, ElseBB);
	builder->SetInsertPoint(ThenBB);

	m_thenBranch->codegen(builder, module, env);

	builder->CreateBr(MergeBB);

	ftn->insert(ftn->end(), ElseBB);
	builder->SetInsertPoint(ElseBB);

	if (m_elseBranch)
	{
		if (STATEMENT_IF == m_elseBranch->GetType())
		{
			static_cast<IfStmt*>(m_elseBranch)->codegen(builder, module, env, MergeBB);
		}
		else
		{
			m_elseBranch->codegen(builder, module, env);
		}
	}

	if (!elseifmerge) {
		builder->CreateBr(MergeBB);
		ftn->insert(ftn->end(), MergeBB);
		builder->SetInsertPoint(MergeBB);
	}

	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue PrintStmt::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("PrintStmt::codegen()\n");

	if (!m_expr) // empty param list
	{
		llvm::Value* nullval = llvm::Constant::getNullValue(builder->getPtrTy());
		if (m_newline)
			builder->CreateCall(module->getFunction("println"), { nullval }, "calltmp");
		else
			builder->CreateCall(module->getFunction("print"), { nullval }, "calltmp");
		return TValue::NullInvalid();
	}

	TValue v = m_expr->codegen(builder, module, env);
	if (!v.IsValid()) return TValue::NullInvalid();

	v = v.As(TOKEN_VAR_STRING);
	if (!v.IsValid()) return TValue::NullInvalid();
	
	llvm::Value* tmp = v.GetFromStorage().Value();

	if (m_newline)
		builder->CreateCall(module->getFunction("println"), { tmp }, "calltmp");
	else
		builder->CreateCall(module->getFunction("print"), { tmp }, "calltmp");
	

	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue ReturnStmt::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("ReturnStmt::codegen()\n");

	TFunction* tfunc = env->GetParentFunction();
	llvm::Function* ftn = tfunc->GetLLVMFunc();
	
	TType rettype = tfunc->GetReturnType();
	
	if (m_value)
	{
		TValue v = m_value->codegen(builder, module, env).GetFromStorage();
		if (!v.IsValid()) return v;

		if (!rettype.IsValid())
		{
			env->Error(v.GetToken(), "Function does not have a return type.");
			return TValue::NullInvalid();
		}

		TValue retvar = env->GetVariable(v.GetToken(), "__ret_ref");
		retvar.Store(v);
	}
	else
	{
		if (rettype.IsValid())
		{
			env->Error(rettype.GetToken(), "Expected return value.");
			return TValue::NullInvalid();
		}
	}
	
	env->EmitReturnCleanup();

	builder->CreateRetVoid();

	llvm::BasicBlock* tail = llvm::BasicBlock::Create(builder->getContext(), "rettail", ftn);
	builder->SetInsertPoint(tail);

	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue StructStmt::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("StructStmt::codegen()\n");

	std::string name = m_name->Lexeme();

	// check for existing function definition
	if (env->GetStruct(m_name, name, true).IsValid())
	{
		env->Error(m_name, "Structure already defined in environment.");
		return TValue::NullInvalid();
	}

	TStruct struc = TStruct::Construct(m_fqns, m_name, m_vars);
	if (struc.IsValid()) env->DefineStruct(struc, name);

	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue VarStmt::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("VarStmt::codegen()\n");

	bool global = !env->HasParent();

	std::vector<TValue> tvals;
	for (auto& expr : m_exprs)
	{
		TValue temp;
		if (expr) temp = expr->codegen(builder, module, env).GetFromStorage();
		tvals.push_back(temp);
	}

	// if type token is auto, need to use the first expression to determine the type
	TType type;
	if (TOKEN_VAR_AUTO == m_type_token.type->GetType())
	{
		if (tvals.empty())
		{
			Environment::Error(m_type_token.type, "Assignment expression expected for type 'auto'.");
			return TValue::NullInvalid();
		}
		else
		{
			if (!tvals[0].IsValid()) return TValue::NullInvalid();
			type = tvals[0].GetTType();
			type.RemoveConstant();
		}
	}
	else
	{
		type = TType::Construct(m_type_token.type, m_type_token.args);
	}
	if (!type.IsValid()) return TValue::NullInvalid();

	for (size_t i = 0; i < m_ids.size(); ++i)
	{
		Token* id = m_ids[i];
		std::string lexeme = id->Lexeme();

		TValue val = TValue::Construct(type, lexeme, global);
		if (val.IsValid())
		{
			if (type.IsFunctor())
			{
				TFunction func = env->GetAnonFunction(m_anon_sig);
				if (!func.IsValid())
				{
					env->Error(id, "Failed to get anonymous prototype.");
					return TValue::NullInvalid();
				}

				val.SetFunctorSig(m_anon_sig);
				val.StoreLLVMValue(func.GetLLVMFunc());
			}

			// check if aleady exists
			TValue temp = env->GetVariable(val.GetToken(), lexeme, true);
			if (temp.IsValid() && temp.IsConstant())
			{
				env->Error(id, "Attempting to shadow a constant.");
				return TValue::NullInvalid();
			}

			env->DefineVariable(val, lexeme);
			if (tvals[i].IsValid())
			{
				env->AssignToVariable(id, lexeme, tvals[i]);
			}
		}
	}
	
	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue VarConstStmt::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("VarStmt::codegen()\n");

	for (size_t i = 0; i < m_ids.size(); ++i)
	{
		Token* id = m_ids[i];
		Expr* expr = m_exprs[i];

		if (!expr) return TValue::NullInvalid();

		LiteralExpr* le = static_cast<LiteralExpr*>(expr);
		Literal lit = le->GetLiteral();
		std::string lexeme = id->Lexeme();

		//// check for existing function definition
		//if (env->IsConstVariableDefined(m_fqns, lexeme))
		//{
		//	env->Error(id, "Constant already defined in environment.");
		//	return TValue::NullInvalid();
		//}

		TValue val = TValue::Construct_ConstInt(m_type, lexeme, lit);
		//if (val.Value()) env->DefineVariable(val, lexeme);
		if (val.Value()) env->DefineConstVariable(val, m_fqns, lexeme);
	}
	
	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue WhileStmt::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("WhileStmt::codegen()\n");

	llvm::Function* ftn = builder->GetInsertBlock()->getParent();
	llvm::LLVMContext& context = builder->getContext();
	llvm::BasicBlock* LoopBB = llvm::BasicBlock::Create(context, "loop", ftn);
	llvm::BasicBlock* BodyBB = llvm::BasicBlock::Create(context, "body");
	llvm::BasicBlock* PostBB = llvm::BasicBlock::Create(context, "post");
	llvm::BasicBlock* MergeBB = llvm::BasicBlock::Create(context, "loopcont");
	
	env->PushLoopBreakContinue(MergeBB, PostBB);

	builder->CreateBr(LoopBB);
	builder->SetInsertPoint(LoopBB);

	TValue CondV = m_condition->codegen(builder, module, env);
	if (!CondV.Value()) return TValue::NullInvalid();

	builder->CreateCondBr(CondV.GetFromStorage().Value(), BodyBB, MergeBB);

	ftn->insert(ftn->end(), BodyBB);
	builder->SetInsertPoint(BodyBB);

	if (m_body) m_body->codegen(builder, module, env);

	builder->CreateBr(PostBB); // go back to the top

	ftn->insert(ftn->end(), PostBB);
	builder->SetInsertPoint(PostBB);

	if (m_post) m_post->codegen(builder, module, env);

	builder->CreateBr(LoopBB); // go back to the top

	ftn->insert(ftn->end(), MergeBB);
	builder->SetInsertPoint(MergeBB);

	env->PopLoopBreakContinue();
	
	return TValue::NullInvalid();
}
