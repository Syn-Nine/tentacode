#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <map>
#include <string>
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "Literal.h"
#include "Token.h"
#include "TFunction.h"
#include "TStruct.h"
#include "TValue.h"
#include "ErrorHandler.h"


class Environment
{

public:
	
	static Environment* Push()
	{
		Environment* e = new Environment();
		if (!m_stack.empty()) e->m_parent = m_stack.back();
		m_stack.push_back(e);
		return e;
	}

	static void Pop()
	{
		if (!m_stack.empty())
		{
			m_stack.back()->EmitCleanup();
			delete m_stack.back();
			m_stack.pop_back();
		}
		else
		{
			Error(nullptr, "Attempting to pop empty environment.");
		}
	}

	static void AddToCleanup(TValue v)
	{
		if (!m_stack.empty())
		{
			m_stack.back()->m_cleanup.push_back(v);
		}
		else
		{
			Error(nullptr, "Attempting to add cleanup to empty environment.");
		}
	}

	void EmitReturnCleanup()
	{
		if (m_parent) m_parent->EmitCleanup();
		EmitCleanup();
	}

	//-------------------------------------------------------------------------

	bool HasParent() const { return m_parent; }
	static bool HasErrors()
	{
		if (m_errorHandler) return m_errorHandler->HasErrors();
		return false;
	}

	static void RegisterErrorHandler(ErrorHandler* eh)
	{
		m_errorHandler = eh;
	}

	static void Error(Token* token, const std::string& err)
	{
		if (token)
		{
			m_errorHandler->Error(token->Filename(), token->Line(), "at '" + token->Lexeme() + "'", err);
		}
		else
		{
			m_errorHandler->Error("<File>", -1, "at '<at>'", err);
		}
	}

	void DefineVariable(TValue val, std::string lexeme)
	{
		m_vars[lexeme] = val;
	}

	void AssignToVariable(Token* token, const std::string& var, TValue rhs);
	
	void AssignToVariableIndex(Token* token, const std::string& var, TValue idx, TValue rhs);
	

	static TValue GetVariable(Token* token, const std::string& var)
	{
		if (m_stack.empty()) return TValue::NullInvalid();
		return m_stack.back()->GetVariable_Recursive(token, var);
	}


	static int GetEnumAsInt(const EnumLiteral& e)
	{
		std::string val = e.enumValue;

		if (0 == m_enumMap.count(val))
		{
			m_enumCounter++;
			m_enumMap.insert(std::make_pair(val, m_enumCounter));
			m_enumReflectMap.insert(std::make_pair(m_enumCounter, val));
			return m_enumCounter;
		}

		return m_enumMap.at(val);
	}

	static std::string GetEnumAsString(int val)
	{
		if (0 == m_enumReflectMap.count(val)) return "<Enum Invalid>";
		return m_enumReflectMap.at(val);
	}

	static void SetDebugLevel(int level) { m_debugLevel = level; }
	static int GetDebugLevel() { return m_debugLevel; }


	llvm::BasicBlock* GetParentLoopBreak()
	{
		if (!m_parent) return nullptr;
		if (m_parent->m_loopBreak) return m_parent->m_loopBreak;
		return m_parent->GetParentLoopBreak();
	}

	llvm::BasicBlock* GetParentLoopContinue()
	{
		if (!m_parent) return nullptr;
		if (m_parent->m_loopContinue) return m_parent->m_loopContinue;
		return m_parent->GetParentLoopContinue();
	}

	void PushLoopBreakContinue(llvm::BasicBlock* b, llvm::BasicBlock* c)
	{
		m_loopBreak = b;
		m_loopContinue = c;
	}

	void PopLoopBreakContinue()
	{
		PushLoopBreakContinue(nullptr, nullptr);
	}

	//----------------------------------------------------------------------------

	static bool IsFunction(std::string id)
	{
		return 0 != m_func.count(id);
	}


	static void DefineFunction(TFunction func, std::string lexeme)
	{
		m_func[lexeme] = func;
	}

	static TFunction GetFunction(Token* token, const std::string& func)
	{
		if (IsFunction(func)) return m_func.at(func);
		Error(token, "Function not found in environment.");
		return TFunction();
	}

	TFunction GetParentFunction()
	{
		if (m_parent_function.IsValid()) return m_parent_function;
		if (m_parent) return m_parent->GetParentFunction();
		return TFunction();
	}

	void SetParentFunction(Token* token, const std::string& func)
	{
		m_parent_function = GetFunction(token, func);
	}


	//----------------------------------------------------------------------------

	static bool IsStruct(std::string id)
	{
		return 0 != m_struc.count(id);
	}


	static void DefineStruct(TStruct struc, std::string lexeme)
	{
		m_struc[lexeme] = struc;
	}

	static TStruct GetStruct(Token* token, const std::string& struc)
	{
		if (IsStruct(struc)) return m_struc.at(struc);
		Error(token, "Structure not found in environment.");
		return TStruct();
	}


private:

	Environment()
	{
		m_parent = nullptr;
		m_loopBreak = nullptr;
		m_loopContinue = nullptr;
	}

	bool IsVariable(const std::string& var)
	{
		return 0 != m_vars.count(var);
	}

	void EmitCleanup()
	{
		if (m_cleanup.empty()) return;

		for (auto& v : m_cleanup)
		{
			v.Cleanup();
		}
	}

	TValue GetVariable_Recursive(Token* token, const std::string& var)
	{
		if (IsVariable(var)) return m_vars.at(var);
		if (m_parent) return m_parent->GetVariable_Recursive(token, var);
		Error(token, "Variable not found in environment.");
		return TValue::NullInvalid();
	}

	Environment* m_parent;

	static std::vector<Environment*> m_stack;

	std::map<std::string, TValue> m_vars;
	
	std::vector<TValue> m_cleanup;

	static std::map<std::string, int> m_enumMap;
	static std::map<int, std::string> m_enumReflectMap;
	static int m_enumCounter;

	static ErrorHandler* m_errorHandler;
	static int m_debugLevel;

	llvm::BasicBlock* m_loopBreak;
	llvm::BasicBlock* m_loopContinue;

	TFunction m_parent_function;
	static std::map<std::string, TFunction> m_func;
	static std::map<std::string, TStruct> m_struc;

};

#endif // ENVIRONMENT_H