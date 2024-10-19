#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <map>
#include <string>
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "Literal.h"

using namespace llvm;

class Environment
{
public:
	Environment() = delete;
	Environment(Environment* parent = nullptr)
	{
		m_parent = parent;
	}

	void DefineVariable(LiteralTypeEnum type, std::string id, AllocaInst* alloca)
	{
		m_vars[id] = alloca;
		m_types[id] = type;
	}

	void DefineFunction(std::string id, Function* ftn, int arity, LiteralTypeEnum rettype)
	{
		m_ftns[id] = ftn;
		m_arity[id] = arity;
		m_rettypes[id] = rettype;
	}

	AllocaInst* GetVariable(std::string id)
	{
		if (0 != m_vars.count(id)) return m_vars.at(id);		
		if (m_parent) return m_parent->GetVariable(id);
		return nullptr;
	}

	LiteralTypeEnum GetVariableType(std::string id)
	{
		if (0 != m_vars.count(id)) return m_types.at(id);
		if (m_parent) return m_parent->GetVariableType(id);
		return LITERAL_TYPE_INVALID;
	}

	Function* GetFunction(std::string id)
	{
		if (0 != m_ftns.count(id)) return m_ftns.at(id);
		if (m_parent) return m_parent->GetFunction(id);
		return nullptr;
	}

	int GetFunctionArity(std::string id)
	{
		if (0 != m_ftns.count(id)) return m_arity.at(id);
		if (m_parent) return m_parent->GetFunctionArity(id);
		return 0;
	}

	LiteralTypeEnum GetFunctionReturnType(std::string id)
	{
		if (0 != m_rettypes.count(id)) return m_rettypes.at(id);
		if (m_parent) return m_parent->GetFunctionReturnType(id);
		return LITERAL_TYPE_INVALID;
	}



private:

	Environment* m_parent;
	std::map<std::string, AllocaInst*> m_vars;
	std::map<std::string, LiteralTypeEnum> m_types;
	std::map<std::string, Function*> m_ftns;
	std::map<std::string, int> m_arity;
	std::map<std::string, LiteralTypeEnum> m_rettypes;



};

#endif // ENVIRONMENT_H