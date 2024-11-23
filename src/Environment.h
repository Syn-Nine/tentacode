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

	void AssignToVariable(const std::string& var, TValue rhs);
	
	void AssignToVariableVectorIndex(const std::string& var, TValue idx, TValue rhs);
	

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

	TFunction GetFunction(Token* token, const std::string& func)
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

	/*

	void DefineUdt(std::string id, llvm::StructType* stype, std::vector<Token*> tokens, std::vector<std::string> names, std::vector<llvm::Type*> ty, std::vector<LiteralTypeEnum> vecTypes, std::vector<std::string> vecTypeIds)
	{
		m_udts[id] = stype;
		m_udt_tokens[id] = tokens;
		m_udt_names[id] = names;
		m_udt_ty[id] = ty;
		m_udt_vecTypes[id] = vecTypes;
		m_udt_vecTypeIds[id] = vecTypeIds;

		std::map<std::string, size_t> name_to_idx;
		for (size_t i = 0; i < names.size(); ++i)
		{
			name_to_idx.insert(std::make_pair(names[i], i));
		}
		m_udt_name_to_idx[id] = name_to_idx;

		if (2 <= Environment::GetDebugLevel()) printf("DefineUdt(%s)\n", id.c_str());
	}

	llvm::StructType* GetUdt(std::string id)
	{
		if (0 != m_udts.count(id)) return m_udts.at(id);
		if (m_parent) return m_parent->GetUdt(id);
		return nullptr;
	}

	bool IsUdt(std::string id)
	{
		if (0 != m_udts.count(id)) return true;
		if (m_parent) return m_parent->IsUdt(id);
		return false;
	}

	std::vector<Token*> GetUdtMemberTokens(std::string id)
	{
		if (0 != m_udt_tokens.count(id)) return m_udt_tokens.at(id);
		if (m_parent) return m_parent->GetUdtMemberTokens(id);
		std::vector<Token*> ret;
		return ret;
	}

	std::vector<LiteralTypeEnum> GetUdtMemberVecTypes(std::string id)
	{
		if (0 != m_udt_vecTypes.count(id)) return m_udt_vecTypes.at(id);
		if (m_parent) return m_parent->GetUdtMemberVecTypes(id);
		std::vector<LiteralTypeEnum> ret;
		return ret;
	}

	LiteralTypeEnum GetUdtMemberVecTypeAt(std::string id, size_t idx)
	{
		if (0 != m_udt_vecTypes.count(id) && idx < m_udt_vecTypes.at(id).size()) return m_udt_vecTypes.at(id).at(idx);
		if (m_parent) return m_parent->GetUdtMemberVecTypeAt(id, idx);
		return LITERAL_TYPE_INVALID;
	}

	std::vector<std::string> GetUdtMemberVecTypeIds(std::string id)
	{
		if (0 != m_udt_vecTypeIds.count(id)) return m_udt_vecTypeIds.at(id);
		if (m_parent) return m_parent->GetUdtMemberVecTypeIds(id);
		std::vector<std::string> ret;
		return ret;
	}

	std::string GetUdtMemberVecTypeIdAt(std::string id, size_t idx)
	{
		if (0 != m_udt_vecTypeIds.count(id) && idx < m_udt_vecTypeIds.at(id).size()) return m_udt_vecTypeIds.at(id).at(idx);
		if (m_parent) return m_parent->GetUdtMemberVecTypeIdAt(id, idx);
		return "";
	}

	Token* GetUdtMemberTokenAt(std::string id, size_t idx)
	{
		if (0 != m_udt_tokens.count(id) && idx < m_udt_tokens.at(id).size()) return m_udt_tokens.at(id).at(idx);
		if (m_parent) return m_parent->GetUdtMemberTokenAt(id, idx);
		return nullptr;
	}

	std::vector<std::string> GetUdtMemberNames(std::string id)
	{
		if (0 != m_udt_names.count(id)) return m_udt_names.at(id);
		if (m_parent) return m_parent->GetUdtMemberNames(id);
		std::vector<std::string> ret;
		return ret;
	}

	int GetUdtMemberIndex(std::string udt_name, std::string member_name)
	{
		if (0 != m_udt_name_to_idx.count(udt_name) && 0 != m_udt_name_to_idx.at(udt_name).count(member_name))
		{
			return m_udt_name_to_idx.at(udt_name).at(member_name);
		}
		if (m_parent) return m_parent->GetUdtMemberIndex(udt_name, member_name);
		return -1;
	}

	std::vector<llvm::Type*> GetUdtTy(std::string id)
	{
		if (0 != m_udt_ty.count(id)) return m_udt_ty.at(id);
		if (m_parent) return m_parent->GetUdtTy(id);
		std::vector<llvm::Type*> ret;
		return ret;
	}

private:

	std::map<std::string, llvm::StructType*> m_udts;
	std::map<std::string, std::vector<Token*> > m_udt_tokens;
	std::map<std::string, std::vector<std::string> > m_udt_names;
	std::map<std::string, std::map<std::string, size_t> > m_udt_name_to_idx;
	std::map<std::string, std::vector<llvm::Type*> > m_udt_ty;
	std::map<std::string, std::vector<LiteralTypeEnum> > m_udt_vecTypes;
	std::map<std::string, std::vector<std::string> > m_udt_vecTypeIds;

	
	*/

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

	bool IsVariable(const std::string& var)
	{
		return 0 != m_vars.count(var);
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

};

#endif // ENVIRONMENT_H