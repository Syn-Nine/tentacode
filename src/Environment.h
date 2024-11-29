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
private:
	struct var_struct
	{
		TValue tvalue;
		std::string lexeme;

		var_struct() {}
		var_struct(TValue val, std::string lex) : tvalue(val), lexeme(lex)
		{}
	};

	struct func_struct
	{
		TFunction tfunc;
		std::string lexeme;

		func_struct() {}
		func_struct(TFunction func, std::string lex) : tfunc(func), lexeme(lex)
		{}
	};

	struct struct_struct
	{
		TStruct tstruc;
		std::string lexeme;

		struct_struct() {}
		struct_struct(TStruct struc, std::string lex) : tstruc(struc), lexeme(lex)
		{}
	};


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
		m_vars[lexeme] = var_struct(val, lexeme);
	}

	void AssignToVariable(Token* token, const std::string& var, TValue rhs);
	
	void AssignToVariableVectorIndex(Token* token, const std::string& var, TValue idx, TValue rhs);
	

	TValue GetVariable(Token* token, const std::string& var)
	{
		if (IsVariable(var)) return m_vars.at(var).tvalue;
		if (m_parent) return m_parent->GetVariable(token, var);
		Error(token, "Variable not found in environment.");
		return TValue::NullInvalid();
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
		m_func[lexeme] = func_struct(func, lexeme);
	}

	static TFunction GetFunction(Token* token, const std::string& func)
	{
		if (IsFunction(func)) return m_func.at(func).tfunc;
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
		m_struc[lexeme] = struct_struct(struc, lexeme);
	}

	static TStruct GetStruct(Token* token, const std::string& struc)
	{
		if (IsStruct(struc)) return m_struc.at(struc).tstruc;
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

	Environment* m_parent;

	static std::vector<Environment*> m_stack;

	std::map<std::string, var_struct> m_vars;
	
	std::vector<TValue> m_cleanup;

	static std::map<std::string, int> m_enumMap;
	static std::map<int, std::string> m_enumReflectMap;
	static int m_enumCounter;

	static ErrorHandler* m_errorHandler;
	static int m_debugLevel;

	llvm::BasicBlock* m_loopBreak;
	llvm::BasicBlock* m_loopContinue;

	TFunction m_parent_function;
	static std::map<std::string, func_struct> m_func;
	
	static std::map<std::string, struct_struct> m_struc;

};

#endif // ENVIRONMENT_H