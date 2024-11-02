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
#include "TValue.h"

class Environment
{
public:
	Environment() = delete;
	Environment(Environment* parent = nullptr)
	{
		m_parent = parent;
		m_loopBreak = nullptr;
		m_loopContinue = nullptr;
	}

	void DefineVariable(LiteralTypeEnum type, std::string id, llvm::Value* value, llvm::Type* ty, Token* token, LiteralTypeEnum vecType = LITERAL_TYPE_INVALID)
	{
		m_vars[id] = value;
		m_ty[id] = ty;
		m_types[id] = type;
		m_vecTypes[id] = vecType;
		m_var_tokens[id] = token;
		if (2 <= Environment::GetDebugLevel()) printf("DefineVariable(%d, %s, %lu, %d)\n", type, id.c_str(), value, vecType);
	}

	void DefineFunction(std::string id, llvm::Function* ftn, std::vector<LiteralTypeEnum> types, std::vector<std::string> names, std::vector<llvm::Type*> args, std::vector<Token*> tokens, LiteralTypeEnum rettype)
	{
		m_ftns[id] = ftn;
		m_arity[id] = types.size();
		m_param_types[id] = types;
		m_param_tokens[id] = tokens;
		m_param_names[id] = names;
		m_rettypes[id] = rettype;
		m_ftn_argty[id] = args;
		if (2 <= Environment::GetDebugLevel()) printf("DefineFunction(%s)\n", id.c_str());
	}

	void DefineUdt(std::string id, llvm::StructType* stype, std::vector<Token*> tokens, std::vector<std::string> names, std::vector<llvm::Type*> ty)
	{
		m_udts[id] = stype;
		m_udt_tokens[id] = tokens;
		m_udt_names[id] = names;
		m_udt_ty[id] = ty;

		std::map<std::string, size_t> name_to_idx;
		for (size_t i = 0; i < names.size(); ++i)
		{
			name_to_idx.insert(std::make_pair(names[i], i));
		}
		m_udt_name_to_idx[id] = name_to_idx;

		if (2 <= Environment::GetDebugLevel()) printf("DefineUdt(%s)\n", id.c_str());
	}

	llvm::Value* GetVariable(std::string id)
	{
		if (0 != m_vars.count(id)) return m_vars.at(id);		
		if (m_parent) return m_parent->GetVariable(id);
		//printf("Error - unable to find variable `%s` in environment!\n", id.c_str());
		return nullptr;
	}

	Token* GetVariableToken(std::string id)
	{
		if (0 != m_var_tokens.count(id)) return m_var_tokens.at(id);
		if (m_parent) return m_parent->GetVariableToken(id);
		return nullptr;
	}

	llvm::Type* GetVariableTy(std::string id)
	{
		if (0 != m_ty.count(id)) return m_ty.at(id);
		if (m_parent) return m_parent->GetVariableTy(id);
		//printf("Error - unable to find variable `%s` in environment!\n", id.c_str());
		return nullptr;
	}

	LiteralTypeEnum GetVariableType(std::string id)
	{
		if (0 != m_vars.count(id)) return m_types.at(id);
		if (m_parent) return m_parent->GetVariableType(id);
		return LITERAL_TYPE_INVALID;
	}

	LiteralTypeEnum GetVecType(std::string id)
	{
		if (0 != m_vecTypes.count(id)) return m_vecTypes.at(id);
		if (m_parent) return m_parent->GetVecType(id);
		return LITERAL_TYPE_INVALID;
	}

	llvm::Function* GetFunction(std::string id)
	{
		if (0 != m_ftns.count(id)) return m_ftns.at(id);
		if (m_parent) return m_parent->GetFunction(id);
		printf("Error - unable to find function `%s` in environment!\n", id.c_str());
		return nullptr;
	}

	bool IsFunction(std::string id)
	{
		if (0 != m_ftns.count(id)) return true;
		if (m_parent) return m_parent->IsFunction(id);
		return false;
	}

	int GetFunctionArity(std::string id)
	{
		if (0 != m_ftns.count(id)) return m_arity.at(id);
		if (m_parent) return m_parent->GetFunctionArity(id);
		return 0;
	}

	std::vector<LiteralTypeEnum> GetFunctionParamTypes(std::string id)
	{
		if (0 != m_ftns.count(id)) return m_param_types.at(id);
		if (m_parent) return m_parent->GetFunctionParamTypes(id);
		std::vector<LiteralTypeEnum> ret;
		return ret;
	}

	std::vector<Token*> GetFunctionParamTokens(std::string id)
	{
		if (0 != m_param_tokens.count(id)) return m_param_tokens.at(id);
		if (m_parent) return m_parent->GetFunctionParamTokens(id);
		std::vector<Token*> ret;
		return ret;
	}

	std::vector<std::string> GetFunctionParamNames(std::string id)
	{
		if (0 != m_param_names.count(id)) return m_param_names.at(id);
		if (m_parent) return m_parent->GetFunctionParamNames(id);
		std::vector<std::string> ret;
		return ret;
	}

	std::vector<llvm::Type*> GetFunctionArgTy(std::string id)
	{
		if (0 != m_ftn_argty.count(id)) return m_ftn_argty.at(id);
		if (m_parent) return m_parent->GetFunctionArgTy(id);
		std::vector<llvm::Type*> ret;
		return ret;
	}

	LiteralTypeEnum GetFunctionReturnType(std::string id)
	{
		if (0 != m_rettypes.count(id)) return m_rettypes.at(id);
		if (m_parent) return m_parent->GetFunctionReturnType(id);
		return LITERAL_TYPE_INVALID;
	}


	llvm::StructType* GetUdt(std::string id)
	{
		if (0 != m_udts.count(id)) return m_udts.at(id);
		if (m_parent) return m_parent->GetUdt(id);
		printf("Error - unable to find UDT `%s` in environment!\n", id.c_str());
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

	int GetEnumAsInt(const EnumLiteral& e)
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

	std::string GetEnumAsString(int val)
	{
		if (0 == m_enumReflectMap.count(val)) return "<Enum Invalid>";
		return m_enumReflectMap.at(val);
	}

	void AddToCleanup(TValue v)
	{
		m_cleanup.push_back(v);
	}

	void EmitCleanup(std::unique_ptr<llvm::IRBuilder<>>& builder, std::unique_ptr<llvm::Module>& module)
	{
		if (m_cleanup.empty()) return;

		for (auto& v : m_cleanup)
		{
			if (v.IsString())
			{
				builder->CreateCall(module->getFunction("__del_std_string_ptr"), { v.value }, "delstr");
			}
		}
	}
	std::vector<TValue> GetCleanup() { return m_cleanup; }

	static void SetDebugLevel(int level) { m_debugLevel = level; }
	static int GetDebugLevel() { return m_debugLevel; }


private:

	Environment* m_parent;
	std::map<std::string, llvm::Value*> m_vars;
	std::map<std::string, llvm::Type*> m_ty;
	std::map<std::string, Token*> m_var_tokens;
	std::map<std::string, LiteralTypeEnum> m_types;
	std::map<std::string, LiteralTypeEnum> m_vecTypes;
	std::map<std::string, llvm::Function*> m_ftns;
	std::map<std::string, int> m_arity;
	std::map<std::string, std::vector<LiteralTypeEnum> > m_param_types;
	std::map<std::string, std::vector<Token*> > m_param_tokens;
	std::map<std::string, std::vector<std::string> > m_param_names;
	std::map<std::string, LiteralTypeEnum> m_rettypes;
	std::map<std::string, std::vector<llvm::Type*> > m_ftn_argty;
	std::map<std::string, llvm::StructType*> m_udts;
	std::map<std::string, std::vector<Token*> > m_udt_tokens;
	std::map<std::string, std::vector<std::string> > m_udt_names;
	std::map<std::string, std::map<std::string, size_t> > m_udt_name_to_idx;
	std::map<std::string, std::vector<llvm::Type*> > m_udt_ty;

	std::vector<TValue> m_cleanup;

	static std::map<std::string, int> m_enumMap;
	static std::map<int, std::string> m_enumReflectMap;
	static int m_enumCounter;
	static int m_debugLevel;

	llvm::BasicBlock* m_loopBreak;
	llvm::BasicBlock* m_loopContinue;

};

#endif // ENVIRONMENT_H