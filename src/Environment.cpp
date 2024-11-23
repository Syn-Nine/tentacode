#include "Environment.h"

std::map<std::string, int> Environment::m_enumMap;
std::map<int, std::string> Environment::m_enumReflectMap;
int Environment::m_enumCounter = 0;

std::vector<Environment*> Environment::m_stack;

int Environment::m_debugLevel = 0;
ErrorHandler* Environment::m_errorHandler = nullptr;

std::map<std::string, Environment::func_struct> Environment::m_func;



//-----------------------------------------------------------------------------
void Environment::AssignToVariable(const std::string& var, TValue rhs)
{
	TValue tval = GetVariable(rhs.GetToken(), var);
	if (tval.IsInvalid())
	{
		Error(rhs.GetToken(), "Variable not found in environment.");
		return;
	}
	
	if (!tval.IsVecAny())
	{
		rhs = rhs.CastToMatchImplicit(tval);
		if (rhs.IsInvalid()) return; // failed cast
	}

	tval.Store(rhs);
}


//-----------------------------------------------------------------------------
void Environment::AssignToVariableVectorIndex(const std::string& var, TValue idx, TValue rhs)
{
	TValue tval = GetVariable(rhs.GetToken(), var);
	if (tval.IsInvalid())
	{
		Error(rhs.GetToken(), "Variable not found in environment.");
		return;
	}

	rhs = rhs.CastToMatchImplicit(tval);
	if (rhs.IsInvalid()) return; // failed cast

	tval.StoreAtIndex(idx, rhs);
}
