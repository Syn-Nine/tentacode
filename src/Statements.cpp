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
		env->Error(m_keyword, "No parent block for `break`.");
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
		env->Error(m_keyword, "No parent block for `continue`.");
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
TValue FunctionStmt::codegen_prototype(llvm::IRBuilder<>* builder,
	llvm::Module* module,
	Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("FunctionStmt::codegen_prototype()\n");

	std::string name = m_name->Lexeme();

	// check for existing function definition
	if (env->IsFunction(name))
	{
		env->Error(m_name, "Function already defined in environment.");
		return TValue::NullInvalid();
	}

	TFunction func = TFunction::Construct_Prototype(m_name, m_rettype, m_types, m_params, m_body);
	
	if (func.IsValid()) env->DefineFunction(func, name);
	
	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue FunctionStmt::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("FunctionStmt::codegen()\n");
	
	std::string name = m_name->Lexeme();

	// check for existing function definition
	TFunction func = env->GetFunction(m_name, name);
	
	if (func.IsValid()) func.Construct_Body();
	
	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue IfStmt::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("IfStmt::codegen()\n");
	TValue CondV = m_condition->codegen(builder, module, env);
	if (CondV.IsInvalid()) return TValue::NullInvalid();

	llvm::Function* ftn = builder->GetInsertBlock()->getParent();
	llvm::LLVMContext& context = builder->getContext();
	llvm::BasicBlock* ThenBB = llvm::BasicBlock::Create(context, "then", ftn);
	llvm::BasicBlock* ElseBB = llvm::BasicBlock::Create(context, "else");
	llvm::BasicBlock* MergeBB = llvm::BasicBlock::Create(context, "ifcont");

	builder->CreateCondBr(CondV.GetFromStorage().Value(), ThenBB, ElseBB);
	builder->SetInsertPoint(ThenBB);

	m_thenBranch->codegen(builder, module, env);

	builder->CreateBr(MergeBB);

	ftn->insert(ftn->end(), ElseBB);
	builder->SetInsertPoint(ElseBB);

	if (m_elseBranch) m_elseBranch->codegen(builder, module, env);

	builder->CreateBr(MergeBB);

	ftn->insert(ftn->end(), MergeBB);
	builder->SetInsertPoint(MergeBB);

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
	if (v.IsInvalid()) return TValue::NullInvalid();

	// convert gep to raw value
	/*if (EXPRESSION_GET == m_expr->GetType())
	{
		llvm::Type* ty = nullptr;
		if (v.IsInteger() || v.IsEnum()) ty = builder->getInt32Ty();
		else if (v.IsDouble()) ty = builder->getDoubleTy();
		else if (v.IsBool()) ty = builder->getInt1Ty();
		
		if (ty) v = TValue(v.type, builder->CreateLoad(ty, v.value, "gep_load"));
	}

	*/

	v = v.As(TOKEN_VAR_STRING);
	if (v.IsInvalid()) return TValue::NullInvalid();
	
	//llvm::Value* tmp = v.Value();//builder->CreateLoad(builder->getPtrTy(), v.Value(), "loadtmp");
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

	TValue v = m_value->codegen(builder, module, env).GetFromStorage();

	TFunction tfunc = env->GetParentFunction();
	TValue ret = tfunc.GetReturn();
	if (ret.IsInvalid())
	{
		env->Error(ret.GetToken(), "Function does not have a return type declared.");
	}

	llvm::Function* ftn = tfunc.GetLLVMFunc();

	if (v.isNumeric()) v = v.CastToMatchImplicit(ret);
	
	builder->CreateRet(v.Value());

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
	if (env->IsStruct(name))
	{
		env->Error(m_name, "Structure already defined in environment.");
		return TValue::NullInvalid();
	}

	TStruct struc = TStruct::Construct(m_name, m_vars);

	if (struc.IsValid()) env->DefineStruct(struc, name);

	return TValue::NullInvalid();
}



//-----------------------------------------------------------------------------
TValue VarStmt::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("VarStmt::codegen()\n");

	bool global = !env->HasParent();
	std::string lexeme = m_id->Lexeme();

	TValue val = TValue::Construct(m_type, m_vecArgs, lexeme, global); // if val is null then gets default initializer
	
	if (val.Value())
	{
		env->DefineVariable(val, lexeme);
		if (m_expr && EXPRESSION_ASSIGN == m_expr->GetType())
		{
			// run assignment expression
			m_expr->codegen(builder, module, env);
		}
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
