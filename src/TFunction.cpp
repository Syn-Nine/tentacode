#include "TFunction.h"
#include "Statements.h"
#include "Environment.h"

ErrorHandler* TFunction::m_errorHandler = nullptr;
llvm::IRBuilder<>* TFunction::m_builder = nullptr;
llvm::Module* TFunction::m_module = nullptr;



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

	int bits = 0;
	if (LITERAL_TYPE_INTEGER) bits = 32;
	else if (LITERAL_TYPE_FLOAT) bits = 64;
	
	ret.m_retval = TValue::Construct_Null(ret.m_name, rettype, bits);

	// figure out parameter types
	int v_bits;
	LiteralTypeEnum v_type;
	TValue v;

	for (size_t i = 0; i < types.size(); ++i)
	{
		v_type = types[i];
		switch (v_type)
		{
		case LITERAL_TYPE_ENUM:
		case LITERAL_TYPE_INTEGER:
			v_bits = 32;
			break;
		case LITERAL_TYPE_FLOAT:
			v_bits = 64;
			break;
		case LITERAL_TYPE_BOOL:
			v_bits = 1;
			break;
		case LITERAL_TYPE_STRING:
			v_bits = 64;
			break;
		case LITERAL_TYPE_POINTER:
			v_bits = 64;
			break;
		default:
			Environment::Error(ret.m_name, "Failed to set parameter type when constructing internal function.");
			return TFunction();
		}
		v = TValue::Construct_Null(ret.m_name, v_type, v_bits);
		ret.m_param_names.push_back("");
		ret.m_param_types.push_back(v);

	}


	ret.m_valid = true;

	return ret;

}


TFunction TFunction::Construct_Prototype(Token* name, Token* rettype, TokenList types, TokenList params, void* bodyPtr)
{
	TFunction ret;
	ret.m_name = name;
	ret.m_body = bodyPtr;

	int ret_bits;
	LiteralTypeEnum ret_type;

	// figure out return type	
	if (!rettype)
	{
		ret_type = LITERAL_TYPE_INVALID;
		ret_bits = 0;
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
		default:
			Environment::Error(name, "Failed to set return type.");
			return TFunction();
		}
	}

	ret.m_retval = TValue::Construct_Null(rettype, ret_type, ret_bits);


	// figure out parameter types
	std::vector<llvm::Type*> args;
	int v_bits;
	LiteralTypeEnum v_type;
	TValue v;

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
		default:
			Environment::Error(name, "Failed to set parameter type.");
			return TFunction();
		}
		v = TValue::Construct_Null(rettype, v_type, v_bits);
		args.push_back(v.GetTy());
		ret.m_param_names.push_back(params[i].Lexeme());
		ret.m_param_types.push_back(v);

	}

	// create type definition and link
	llvm::FunctionType* FT = llvm::FunctionType::get(ret.m_retval.GetTy(), args, false);

	ret.m_llvm_func = llvm::Function::Create(FT, llvm::Function::InternalLinkage, name->Lexeme(), *m_module);
	
	ret.m_valid = true;
	
	return ret;
}


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
		llvm::Type* defty = arg.getType();
		llvm::Value* defval = CreateEntryAlloca(m_builder, defty);
		m_builder->CreateStore(&arg, defval);
		TValue v = m_param_types[i];
		v.SetValue(defval);
		v.SetStorage(true);
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
	
	if (m_retval.IsInvalid())
	{
		m_builder->CreateRetVoid();
	}
	else
	{
		m_builder->CreateRet(llvm::Constant::getNullValue(m_retval.GetTy()));
	}
	
	verifyFunction(*ftn);
}