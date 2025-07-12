#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <map>
#include <string>
#include <set>
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
		e->m_parent_function = nullptr;
		if (!m_stack.empty())
		{
			e->m_parent = m_stack.back();
			e->m_parent_function = e->m_parent->m_parent_function;
		}
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

	void DefineConstVariable(TValue val, const std::string& fqns, const std::string& lexeme)
	{
		if (lexeme.compare("_") == 0)
		{
			Error(val.GetToken(), "Constant must have a valid name.");
			return;
		}
		
		// is incoming variable namespaced?
		if (std::string::npos != lexeme.find_first_of(':'))
		{
			Error(val.GetToken(), "Namespace not allowed in variable definition.");
			return;
		}

		//m_const_vars.insert(fqns + ":" + lexeme);

		if (0 == m_ns_vars.count(fqns))
		{
			m_ns_vars.insert(std::make_pair(fqns, VarMap()));
		}
		m_ns_vars.at(fqns)[lexeme] = val;
	}

	void DefineVariable(TValue val, std::string lexeme)
	{
		if (lexeme.compare("_") == 0)
		{
			Error(val.GetToken(), "Variable must have a valid name.");
			return;
		}

		// is incoming variable namespaced?
		if (std::string::npos != lexeme.find_first_of(':'))
		{
			Error(val.GetToken(), "Namespace not allowed in variable definition.");
			return;
		}

		if (0 == m_ns_vars.count(m_fqns))
		{
			m_ns_vars.insert(std::make_pair(m_fqns, VarMap()));
		}
		m_ns_vars.at(m_fqns)[lexeme] = val;
	}

	void AssignToVariable(Token* token, const std::string& var, TValue rhs);
	
	void AssignToVariableIndex(Token* token, const std::string& var, TValue idx, TValue rhs);
	

	static TValue GetVariable(Token* token, std::string var, bool quiet=false)
	{
		if (m_stack.empty()) return TValue::NullInvalid();

		return m_stack.back()->GetVariable_Recursive(token, m_fqns, var, quiet);
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

	static void DefineFunction(TFunction func, std::string lexeme)
	{
		// is incoming function namespaced?
		if (std::string::npos != lexeme.find_first_of(':'))
		{
			Error(func.GetToken(), "Namespace not allowed in function definition.");
			return;
		}

		std::string fqns = func.GetFQNS();

		if (0 == m_ns_func.count(fqns))
		{
			m_ns_func.insert(std::make_pair(fqns, FuncMap()));
		}
		m_ns_func.at(fqns)[lexeme] = func;
	}

	static TFunction GetFunction(Token* token, std::string func, bool quiet=false)
	{
		std::string fqns = m_fqns;
		FixNamespacing(fqns, func);

		while (true)
		{
			std::string temp_fqns = fqns;
			std::string temp_func = func;
			Environment::FixNamespacing(temp_fqns, temp_func);

			if (0 != m_ns_func.count(temp_fqns))
			{
				FuncMap& fmap = m_ns_func.at(temp_fqns);
				if (0 != fmap.count(temp_func)) return fmap.at(temp_func);
			}
			size_t idx = fqns.find_last_of(':');
			if (std::string::npos == idx) break;
			fqns = fqns.substr(0, idx);
		}

		if (!quiet)
		{
			Error(token, "Function not found in environment.");
		}
		return TFunction();
	}

	TFunction* GetParentFunction()
	{
		return m_parent_function;
		/*if (m_parent_function.IsValid()) return m_parent_function;
		if (m_parent) return m_parent->GetParentFunction();
		return TFunction();*/
	}

	//void SetParentFunction(Token* token, const std::string& func)
	void SetParentFunction(TFunction* func)
	{
		//m_parent_function = GetFunction(token, func);
		m_parent_function = func;
	}


	//----------------------------------------------------------------------------

	static void DefineAnonFunction(TFunction func, std::string sig)
	{
		if (0 == m_anon_func.count(sig))
		{
			m_anon_func.insert(std::make_pair(sig, func));
		}
	}

	static TFunction GetAnonFunction(std::string sig)
	{
		if (0 == m_anon_func.count(sig)) return TFunction();
		return m_anon_func.at(sig);
	}


	//----------------------------------------------------------------------------

	static void DefineStruct(TStruct struc, std::string lexeme)
	{
		// is incoming struct namespaced?
		if (std::string::npos != lexeme.find_first_of(':'))
		{
			Error(struc.GetToken(), "Namespace not allowed in structure definition.");
			return;
		}

		std::string fqns = struc.GetFQNS();

		if (0 == m_ns_struc.count(fqns))
		{
			m_ns_struc.insert(std::make_pair(fqns, StrucMap()));
		}
		m_ns_struc.at(fqns)[lexeme] = struc;
	}

	static TStruct GetStruct(Token* token, std::string struc, bool quiet=false)
	{
		std::string fqns = m_fqns;
		FixNamespacing(fqns, struc);

		while (true)
		{
			std::string temp_fqns = fqns;
			std::string temp_struc = struc;
			Environment::FixNamespacing(temp_fqns, temp_struc);

			if (0 != m_ns_struc.count(temp_fqns))
			{
				StrucMap& smap = m_ns_struc.at(temp_fqns);
				if (0 != smap.count(temp_struc)) return smap.at(temp_struc);
			}
			size_t idx = fqns.find_last_of(':');
			if (std::string::npos == idx) break;
			fqns = fqns.substr(0, idx);
		}

		if (!quiet)
		{
			Error(token, "Structure not found in environment.");
		}
		return TStruct();
	}


	//----------------------------------------------------------------------------

	void PushNamespace(std::string name)
	{
		m_namespace.push_back(name);
		m_fqns = StrJoin(m_namespace, ":");
		if (0 < m_debugLevel) printf("Pushing Namespace, FQNS=%s\n", m_fqns.c_str());
	}

	void PopNamespace()
	{
		m_namespace.pop_back();
		m_fqns = StrJoin(m_namespace, ":");
		if (0 < m_debugLevel) printf("Popping Namespace, FQNS=%s\n", m_fqns.c_str());
	}

	static void FixNamespacing(std::string& fqns, std::string& var)
	{
		// todo clean this up for packages with :: namespacing
		if (var.length() > 5 && var.substr(0, 5).compare("ray::") == 0)
		{
			fqns = "ray::";
			var = var.substr(5);
			return;
		}

		// is incoming variable namespaced?
		size_t first = var.find_first_of(':');
		size_t last = var.find_last_of(':');
		if (std::string::npos != first)
		{
			if (0 == var.substr(0, first).compare("global"))
			{
				fqns = var.substr(0, last);
				var = var.substr(last + 1);
			}
			else
			{
				fqns.append(":" + var.substr(0, last));
				var = var.substr(last + 1);
			}
		}
	}

	static void PrintFQNS() { printf("FQNS: %s\n", m_fqns.c_str()); }


private:

	Environment()
	{
		m_parent = nullptr;
		m_loopBreak = nullptr;
		m_loopContinue = nullptr;
	}

	void EmitCleanup()
	{
		if (m_cleanup.empty()) return;

		for (auto& v : m_cleanup)
		{
			v.Cleanup();
		}
	}

	TValue GetVariable_Recursive(Token* token, const std::string& fqns, const std::string& var, bool quiet)
	{
		std::string temp_fqns = fqns;
		std::string temp_var = var;
		Environment::FixNamespacing(temp_fqns, temp_var);

		if (0 != m_ns_vars.count(temp_fqns))
		{
			VarMap& vmap = m_ns_vars.at(temp_fqns);
			if (0 != vmap.count(temp_var)) return vmap.at(temp_var);
		}

		if (m_parent) return m_parent->GetVariable_Recursive(token, fqns, var, quiet);
		if (!quiet) Error(token, "Variable not found in environment.");
		return TValue::NullInvalid();
	}


	Environment* m_parent;

	static std::vector<Environment*> m_stack;

	typedef std::map<std::string, TValue> VarMap;
	std::map<std::string, VarMap> m_ns_vars;
	
	std::vector<TValue> m_cleanup;

	static std::map<std::string, int> m_enumMap;
	static std::map<int, std::string> m_enumReflectMap;
	static int m_enumCounter;

	static ErrorHandler* m_errorHandler;
	static int m_debugLevel;

	llvm::BasicBlock* m_loopBreak;
	llvm::BasicBlock* m_loopContinue;

	TFunction* m_parent_function;
	typedef std::map<std::string, TFunction> FuncMap;
	static std::map<std::string, FuncMap> m_ns_func;
	static FuncMap m_anon_func;

	typedef std::map<std::string, TStruct> StrucMap;
	static std::map<std::string, StrucMap> m_ns_struc;

	static llvm::SmallVector<std::string> m_namespace;
	static std::string m_fqns;

	
};

#endif // ENVIRONMENT_H