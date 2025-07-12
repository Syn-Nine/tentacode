#include "TStruct.h"
#include "Environment.h"
#include "Statements.h"

llvm::IRBuilder<>* TStruct::m_builder = nullptr;
llvm::Module* TStruct::m_module = nullptr;


//-----------------------------------------------------------------------------
TStruct TStruct::Construct(std::string fqns, Token* name, void* in_vars)
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

			TType type = TType::Construct(vs->VarType(), vs->TypeArgs());
			if (!type.IsValid()) return TStruct();

			TokenPtrList ids = vs->IDs();
			ArgList exprs = vs->Expressions();

			for (auto& id : ids)
			{
				std::string lexeme = id->Lexeme();
				ret.m_member_types.push_back(type);
				ret.m_member_names.push_back(lexeme);
				arg_ty.push_back(type.GetTy());
				ret.m_gep_loc.insert(std::make_pair(lexeme, m_builder->getInt32(i)));
				ret.m_type_map.insert(std::make_pair(lexeme, type));
				i++;
			}
		}
		else
		{
			Environment::Error(name, "Expected variable statement in structure definition.");
			return TStruct();
		}
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
		ret.m_fqns = fqns;
		ret.m_valid = true;
	}
	else
	{
		Environment::Error(name, "No valid structure members.");
		return TStruct();
	}

	return ret;
}


//-----------------------------------------------------------------------------
TType TStruct::GetMember(Token* token, const std::string& lex)
{
	if (0 != m_type_map.count(lex)) return m_type_map.at(lex);
	Environment::Error(token, "Structure member not found");
	return TType();
}


//-----------------------------------------------------------------------------
llvm::Value* TStruct::GetGEPLoc(Token* token, const std::string& lex)
{
	if (0 != m_gep_loc.count(lex)) return m_gep_loc.at(lex);
	Environment::Error(token, "Structure member not found");
	return nullptr;
}