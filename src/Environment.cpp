#include "Environment.h"

std::map<std::string, int> Environment::m_enumMap;
std::map<int, std::string> Environment::m_enumReflectMap;
int Environment::m_enumCounter = 0;

std::vector<Environment*> Environment::m_stack;

int Environment::m_debugLevel = 0;
ErrorHandler* Environment::m_errorHandler = nullptr;

std::map<std::string, Environment::FuncMap> Environment::m_ns_func;
std::map<std::string, Environment::StrucMap> Environment::m_ns_struc;

llvm::SmallVector<std::string> Environment::m_namespace;
std::string Environment::m_fqns;

Environment::FuncMap Environment::m_anon_func;


//-----------------------------------------------------------------------------
void Environment::AssignToVariable(Token* token, const std::string& var, TValue rhs)
{
	TValue tval = GetVariable(rhs.GetToken(), var);
	if (!tval.IsValid())
	{
		Error(token, "Variable not found in environment.");
		return;
	}
	if (tval.IsConstant())
	{
		if (tval.IsReference())
			Error(token, "Cannot assign to constant reference.");
		else 
			Error(token, "Cannot assign to constant variable.");
		
		return;
	}
	
	/*if (!tval.IsVecAny())
	{
		rhs = rhs.CastToMatchImplicit(tval);
		if (!rhs.IsValid()) return; // failed cast
	}*/

	tval.Store(rhs);
}


//-----------------------------------------------------------------------------
void Environment::AssignToVariableIndex(Token* token, const std::string& var, TValue idx, TValue rhs)
{
	TValue tval = GetVariable(rhs.GetToken(), var);
	if (!tval.IsValid())
	{
		Error(token, "Variable not found in environment.");
		return;
	}

	tval.StoreAtIndex(idx, rhs);
}
