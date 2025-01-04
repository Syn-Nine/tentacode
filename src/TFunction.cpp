#include "TFunction.h"
#include "Statements.h"
#include "Environment.h"

llvm::IRBuilder<>* TFunction::m_builder = nullptr;
llvm::Module* TFunction::m_module = nullptr;


//-----------------------------------------------------------------------------
TFunction TFunction::Construct_Internal(
	std::string lexeme,
	llvm::Function* ftn,
	std::vector<LiteralTypeEnum> types,
	std::vector<llvm::Type*> args,
	LiteralTypeEnum rettype)
{
	TFunction ret;
	ret.m_body = nullptr;
	ret.m_llvm_func = ftn;
	ret.m_internal = true;
	ret.m_name = new Token(TOKEN_IDENTIFIER, lexeme, -1, "");

	
	// figure out return type
	TType retType;

	switch (rettype)
	{
	case LITERAL_TYPE_ENUM:
		retType = TType::Construct_Enum(ret.m_name);
		break;

	case LITERAL_TYPE_INTEGER:
		retType = TType::Construct_Int(ret.m_name, 32);
		break;

	case LITERAL_TYPE_FLOAT:
		retType = TType::Construct_Float(ret.m_name, 64);
		break;

	case LITERAL_TYPE_BOOL:
		retType = TType::Construct_Bool(ret.m_name);
		break;

	case LITERAL_TYPE_POINTER:
		retType = TType::Construct_Ptr(ret.m_name);
		break;

	case LITERAL_TYPE_STRING:
		retType = TType::Construct_String(ret.m_name);
		break;

	case LITERAL_TYPE_INVALID:
		break;

	default:
		Environment::Error(ret.m_name, "Unable to determine parameter type of internal function.");
		return TFunction();
	}

	ret.m_rettype_internal = retType;
		
		
	// figure out parameter types
	LiteralTypeEnum v_type;

	for (size_t i = 0; i < types.size(); ++i)
	{
		TType t;
		v_type = types[i];
		switch (v_type)
		{
		case LITERAL_TYPE_ENUM:
			t = TType::Construct_Enum(ret.m_name);
			break;

		case LITERAL_TYPE_INTEGER:
			t = TType::Construct_Int(ret.m_name, 32);
			break;

		case LITERAL_TYPE_FLOAT:
			t = TType::Construct_Float(ret.m_name, 64);
			break;

		case LITERAL_TYPE_BOOL:
			t = TType::Construct_Bool(ret.m_name);
			break;

		case LITERAL_TYPE_POINTER:
			t = TType::Construct_Ptr(ret.m_name);
			break;

		case LITERAL_TYPE_STRING:
			t = TType::Construct_String(ret.m_name);
			break;

		default:
			Environment::Error(ret.m_name, "Unable to determine parameter type of internal function.");
			return TFunction();
		}

		if (!t.IsValid()) { return TFunction(); }
		ret.m_paramTypes.push_back(t);
		ret.m_paramNames.push_back("anon");
	}

	ret.m_valid = true;

	return ret;

}


//-----------------------------------------------------------------------------
TFunction TFunction::Construct_Prototype(Token* name, Token* rettype, TokenList types, TokenList params, std::vector<bool> mut, void* bodyPtr)
{
	TFunction ret;
	std::vector<llvm::Type*> args;

	TType retType = TType::Construct(rettype, nullptr);

	if (retType.IsValid())
	{
		retType.SetReference();
		ret.m_has_return_ref = true;
		//ret.m_retType = retType;
		ret.m_paramTypes.push_back(retType);
		ret.m_paramNames.push_back("__ret_ref");
		args.push_back(llvm::PointerType::get(retType.GetTy(), 0));
	}

	// all parameters are passed by const reference unless specified
	for (size_t i = 0; i < params.size(); ++i)
	{
		Token* type_clone = new Token(types[i]);
		TType t = TType::Construct(type_clone, nullptr);
		if (!t.IsValid()) { delete type_clone; return TFunction(); }
		t.SetReference();
		if (!mut[i]) t.SetConstant();

		ret.m_paramTypes.push_back(t);
		ret.m_paramNames.push_back(params[i].Lexeme());
		args.push_back(llvm::PointerType::get(t.GetTy(), 0));
	}

	// create type definition and link
	llvm::FunctionType* FT = llvm::FunctionType::get(m_builder->getVoidTy(), args, false);

	ret.m_llvm_func = llvm::Function::Create(FT, llvm::Function::InternalLinkage, name->Lexeme(), *m_module);
	if (!ret.m_llvm_func)
	{
		Environment::Error(name, "Failed compile function prototype.");
		return TFunction();
	}

	ret.m_name = name;
	ret.m_body = bodyPtr;
	ret.m_valid = true;

	return ret;
}


//-----------------------------------------------------------------------------
void TFunction::Construct_Body()
{
	llvm::Function* ftn = m_llvm_func;

	llvm::BasicBlock* body = llvm::BasicBlock::Create(m_module->getContext(), "entry", ftn);
	m_builder->SetInsertPoint(body);

	Environment* sub_env = Environment::Push();
	sub_env->SetParentFunction(m_name, m_name->Lexeme());

	// all variables come in as references
	int i = 0;
	for (auto& arg : ftn->args())
	{
		TValue val = TValue::Construct_Reference(m_paramTypes[i], m_paramNames[i], &arg);
		if (!val.IsValid())
		{
			Environment::Error(m_name, "Failed to add function parameters to environment on entry.");
			m_valid = false;
			return;
		}
		sub_env->DefineVariable(val, m_paramNames[i]);
		i = i + 1;
	}

	/****
	// make local variables 
	int i = 0;
	for (auto& arg : ftn->args())
	{
		TValue v = m_param_types[i];
		if (v.IsUDT() && (!m_has_return_ref || 0 < i))
		{
			llvm::Type* defty = v.GetTy();
			llvm::Value* defval = CreateEntryAlloca(m_builder, defty, nullptr, m_param_names[i]);

			TStruct tstruc = Environment::GetStruct(v.GetToken(), v.GetToken()->Lexeme());

			std::vector<std::string>& member_names = tstruc.GetMemberNames();
			std::vector<TValue>& members = tstruc.GetMemberVec();

			llvm::Value* gep_zero = m_builder->getInt32(0);

			for (size_t i = 0; i < members.size(); ++i)
			{
				llvm::Value* lhs_gep = m_builder->CreateGEP(v.GetTy(), defval, { gep_zero, m_builder->getInt32(i) }, member_names[i]);
				llvm::Value* rhs_gep = m_builder->CreateGEP(v.GetTy(), &arg, { gep_zero, m_builder->getInt32(i) }, member_names[i]);
				llvm::Value* tmp = m_builder->CreateLoad(members[i].GetTy(), rhs_gep);
				// shallow copy
				m_builder->CreateStore(tmp, lhs_gep);
			}
			v.SetValue(defval);
		}
		else
		{
			llvm::Type* defty = arg.getType();
			llvm::Value* defval = CreateEntryAlloca(m_builder, defty, nullptr, m_param_names[i]);
			m_builder->CreateStore(&arg, defval);
			v.SetValue(defval);
			v.SetStorage(true);
		}
		sub_env->DefineVariable(v, m_param_names[i]);
		i = i + 1;
	}*/

	StmtList* stmtBody = static_cast<StmtList*>(m_body);

	bool found_return = false;

	for (auto& statement : *stmtBody)
	{
		if (Environment::HasErrors()) break;
		if (STATEMENT_FUNCTION != statement->GetType())
		{
			TValue v = statement->codegen(m_builder, m_module, sub_env);
			if (STATEMENT_RETURN == statement->GetType()) found_return = true;
		}
	}

	Environment::Pop();

	llvm::BasicBlock* tail = llvm::BasicBlock::Create(m_module->getContext(), "rettail", ftn);
	m_builder->CreateBr(tail);
	m_builder->SetInsertPoint(tail);
	
	if (m_has_return_ref && !found_return)
	{
		Environment::Error(m_name, "Function requires a return value.");
		m_valid = false;
		return;
	}

	m_builder->CreateRetVoid();

	m_valid = verifyFunction(*ftn);
}