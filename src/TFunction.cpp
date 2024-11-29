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
	ret.m_name = new Token(TOKEN_IDENTIFIER, lexeme, -1, "");
	ret.m_body = nullptr;
	ret.m_llvm_func = ftn;
	ret.m_internal = true;

	int bits = 0;
	llvm::Type* ty;
	// construct null
	switch (rettype)
	{
	case LITERAL_TYPE_ENUM: // intentional fall-through
	case LITERAL_TYPE_INTEGER:
		bits = 32;
		ty = m_builder->getInt32Ty();
		break;

	case LITERAL_TYPE_FLOAT:
		bits = 64;
		ty = m_builder->getDoubleTy();
		break;

	case LITERAL_TYPE_BOOL:
		bits = 1;
		ty = m_builder->getIntNTy(1);
		break;

	case LITERAL_TYPE_POINTER: // intentional fallthrough
	case LITERAL_TYPE_STRING:
		bits = 64;
		ty = m_builder->getPtrTy();
		break;

	case LITERAL_TYPE_INVALID:
		break;

	default:
		Environment::Error(ret.m_name, "Unable to determine return type of internal function.");
		return TFunction();
	}

	if (LITERAL_TYPE_INVALID != rettype)
	{
		ret.m_retval = TValue::Construct_Explicit(
			ret.m_name,
			rettype,
			llvm::Constant::getNullValue(ty),
			ty,
			bits,
			false,
			0,
			LITERAL_TYPE_INVALID
		);
	}
		
		
	// figure out parameter types
	LiteralTypeEnum v_type;
	TValue v;

	for (size_t i = 0; i < types.size(); ++i)
	{
		v_type = types[i];
		switch (v_type)
		{
		case LITERAL_TYPE_ENUM: // intentional fall-through
		case LITERAL_TYPE_INTEGER:
			bits = 32;
			ty = m_builder->getInt32Ty();
			break;

		case LITERAL_TYPE_FLOAT:
			bits = 64;
			ty = m_builder->getDoubleTy();
			break;

		case LITERAL_TYPE_BOOL:
			bits = 1;
			ty = m_builder->getIntNTy(1);
			break;

		case LITERAL_TYPE_POINTER: // intentional fallthrough
		case LITERAL_TYPE_STRING:
			bits = 64;
			ty = m_builder->getPtrTy();
			break;

		default:
			Environment::Error(ret.m_name, "Unable to determine parameter type of internal function.");
			return TFunction();
		}

		v = TValue::Construct_Explicit(
			ret.m_name,
			v_type,
			llvm::Constant::getNullValue(ty),
			ty,
			bits,
			false,
			0,
			LITERAL_TYPE_INVALID
		);

		ret.m_param_names.push_back("");
		ret.m_param_types.push_back(v);

	}


	ret.m_valid = true;

	return ret;

}


//-----------------------------------------------------------------------------
TFunction TFunction::Construct_Prototype(Token* name, Token* rettype, TokenList types, TokenList params, void* bodyPtr)
{
	TFunction ret;
	ret.m_name = name;
	ret.m_body = bodyPtr;

	int ret_bits = 0;
	LiteralTypeEnum ret_type;
	TValue ret_param;

	// figure out return type	
	if (!rettype)
	{
		ret_type = LITERAL_TYPE_INVALID;
	}
	else
	{
		switch (rettype->GetType())
		{
		case TOKEN_VAR_I16:
			ret_type = LITERAL_TYPE_INTEGER;
			ret_bits = 16;
			break;
		case TOKEN_VAR_I32:
			ret_type = LITERAL_TYPE_INTEGER;
			ret_bits = 32;
			break;
		case TOKEN_VAR_I64:
			ret_type = LITERAL_TYPE_INTEGER;
			ret_bits = 32;
			break;
		case TOKEN_VAR_F32:
			ret_type = LITERAL_TYPE_FLOAT;
			ret_bits = 32;
			break;
		case TOKEN_VAR_F64:
			ret_type = LITERAL_TYPE_FLOAT;
			ret_bits = 64;
			break;
		case TOKEN_VAR_ENUM:
			ret_type = LITERAL_TYPE_ENUM;
			ret_bits = 32;
			break;
		case TOKEN_VAR_BOOL:
			ret_type = LITERAL_TYPE_BOOL;
			ret_bits = 1;
			break;
		case TOKEN_VAR_STRING:
			ret_type = LITERAL_TYPE_STRING;
			ret_bits = 64;
			break;
		case TOKEN_VAR_VEC:
			ret_type = LITERAL_TYPE_VEC_DYNAMIC;
			ret_bits = 64;
			break;
		case TOKEN_IDENTIFIER:
			ret_type = LITERAL_TYPE_UDT;
			ret_bits = 64;
			break;

		default:
			Environment::Error(name, "Failed to set return type.");
			return TFunction();
		}

		Token* rettype_clone = new Token(*rettype); // need to clone because going to hold on to pointer after this function
		ret_param = TValue::Construct_Null(rettype_clone, ret_type, ret_bits);
		ret.m_has_return_ref = true;
		if (ret_param.IsInvalid())
		{
			Environment::Error(name, "Invalid return type.");
			return TFunction();
		}
	}


	// figure out parameter types
	std::vector<llvm::Type*> args;
	int v_bits;
	LiteralTypeEnum v_type;
	TValue v;

	if (ret.m_has_return_ref)
	{
		args.push_back(llvm::PointerType::get(ret_param.GetTy(), 0));
		ret.m_param_names.push_back("__ret_ref");
		ret.m_param_types.push_back(ret_param);
	}

	for (size_t i = 0; i < params.size(); ++i)
	{
		switch (types[i].GetType())
		{
		case TOKEN_VAR_I16:
			v_type = LITERAL_TYPE_INTEGER;
			v_bits = 16;
			break;
		case TOKEN_VAR_I32:
			v_type = LITERAL_TYPE_INTEGER;
			v_bits = 32;
			break;
		case TOKEN_VAR_I64:
			v_type = LITERAL_TYPE_INTEGER;
			v_bits = 64;
			break;
		case TOKEN_VAR_F32:
			v_type = LITERAL_TYPE_FLOAT;
			v_bits = 32;
			break;
		case TOKEN_VAR_F64:
			v_type = LITERAL_TYPE_FLOAT;
			v_bits = 64;
			break;
		case TOKEN_VAR_ENUM:
			v_type = LITERAL_TYPE_ENUM;
			v_bits = 32;
			break;
		case TOKEN_VAR_BOOL:
			v_type = LITERAL_TYPE_BOOL;
			v_bits = 1;
			break;
		case TOKEN_VAR_STRING:
			v_type = LITERAL_TYPE_STRING;
			v_bits = 64;
			break;
		case TOKEN_IDENTIFIER:
			v_type = LITERAL_TYPE_UDT;
			v_bits = 64;
			break;

		default:
			Environment::Error(name, "Failed to set parameter type.");
			return TFunction();
		}

		Token* type_clone = new Token(types[i]); // need to clone because going to hold on to pointer after this function
		v = TValue::Construct_Null(type_clone, v_type, v_bits);
		if (LITERAL_TYPE_UDT == v_type)
		{
			args.push_back(llvm::PointerType::get(v.GetTy(), 0));
		}
		else
		{
			args.push_back(v.GetTy());
		}
		ret.m_param_names.push_back(params[i].Lexeme());
		ret.m_param_types.push_back(v);

	}

	// create type definition and link
	llvm::FunctionType* FT = llvm::FunctionType::get(m_builder->getVoidTy(), args, false);

	ret.m_llvm_func = llvm::Function::Create(FT, llvm::Function::InternalLinkage, name->Lexeme(), *m_module);
	if (!ret.m_llvm_func)
	{
		Environment::Error(name, "Failed compile function prototype.");
		return TFunction();
	}
	
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
	}

	StmtList* stmtBody = static_cast<StmtList*>(m_body);

	for (auto& statement : *stmtBody)
	{
		if (Environment::HasErrors()) break;
		if (STATEMENT_FUNCTION != statement->GetType())
		{
			statement->codegen(m_builder, m_module, sub_env);
		}
	}

	Environment::Pop();

	llvm::BasicBlock* tail = llvm::BasicBlock::Create(m_module->getContext(), "rettail", ftn);
	m_builder->CreateBr(tail);
	m_builder->SetInsertPoint(tail);
	
	m_builder->CreateRetVoid();

	verifyFunction(*ftn);
}