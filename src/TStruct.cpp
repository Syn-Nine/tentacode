#include "TStruct.h"
#include "Environment.h"
#include "Statements.h"

llvm::IRBuilder<>* TStruct::m_builder = nullptr;
llvm::Module* TStruct::m_module = nullptr;

TStruct TStruct::Construct(Token* name, void* in_vars)
{
	StmtList* vars = static_cast<StmtList*>(in_vars);

	TStruct ret;

	std::vector<llvm::Type*> arg_ty;
	
	int i = 0;
	for (auto& statement : *vars)
	{
		if (Environment::HasErrors()) break;
		if (STATEMENT_VAR == statement->GetType())
		{
			VarStmt* vs = static_cast<VarStmt*>(statement);
			std::string lexeme = vs->Id()->Lexeme();
			Token* vtype = vs->VarType();
			TValue var = TValue::Construct_Prototype(vtype);
			if (var.IsInvalid()) return TStruct();

			var.SetStorage(true);
			ret.m_member_types.push_back(var);
			ret.m_member_names.push_back(lexeme);
			arg_ty.push_back(var.GetTy());
			ret.m_gep_loc.insert(std::make_pair(lexeme, m_builder->getInt32(i)));
			ret.m_type_map.insert(std::make_pair(lexeme, var));
		}
		else
		{
			Environment::Error(name, "Expected variable statement in structure definition.");
			return TStruct();
		}
		i += 1;
	}

	if (!ret.m_member_types.empty())
	{
		ret.m_token = name;
		ret.m_name = name->Lexeme();
		ret.m_llvm_struc = llvm::StructType::create(m_module->getContext(), arg_ty, ret.m_name);
		if (!ret.m_llvm_struc)
		{
			Environment::Error(name, "Failed to compile structure.");
			return TStruct();
		}
		ret.m_valid = true;
	}
	else
	{
		Environment::Error(name, "No valid structure members.");
		return TStruct();
	}

	return ret;

	/*
	std::vector<llvm::Type*> arg_ty;
	std::vector<Token*> arg_types;
	std::vector<LiteralTypeEnum> arg_vec_types;
	std::vector<std::string> arg_vec_types_id;
	std::vector<std::string> arg_names;

	std::string id = vs->Id()->Lexeme();
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
					env->Error(vs->VarType(), "Invalid structure member type");
				break;
			}
			case TOKEN_VAR_VEC:
				arg_ty.push_back(builder->getPtrTy());
				break;

			default:
				env->Error(vs->VarType(), "Invalid structure member type");
				break;
			}
			arg_vec_types.push_back(vs->VarVecType());
			arg_vec_types_id.push_back(vs->VarVecTypeId());
			arg_types.push_back(vs->VarType());
			arg_names.push_back(id);

	if (!arg_ty.empty())
	{
		std::string id = "struct." + m_name->Lexeme();
		llvm::StructType* stype = llvm::StructType::create(*context, arg_ty, id);
		env->DefineUdt(id, stype, arg_types, arg_names, arg_ty, arg_vec_types, arg_vec_types_id);
	}
	*/
}


TValue TStruct::GetMember(Token* token, const std::string& lex)
{
	if (0 != m_type_map.count(lex)) return m_type_map.at(lex);
	Environment::Error(token, "Structure member not found");
	return TValue::NullInvalid();
}

llvm::Value* TStruct::GetGEPLoc(Token* token, const std::string& lex)
{
	if (0 != m_gep_loc.count(lex)) return m_gep_loc.at(lex);
	Environment::Error(token, "Structure member not found");
	return nullptr;
}