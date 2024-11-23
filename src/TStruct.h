#ifndef TSTRUCT_H
#define TSTRUCT_H

//#include <stdint.h>
//#include <memory>
//#include "Literal.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/DerivedTypes.h"

#include "Token.h"
#include "TValue.h"

#include <map>
#include <vector>
#include <string>

class TStruct
{
public:
	TStruct()
	{
		m_valid = false;
		m_llvm_struc = nullptr;
	}

	~TStruct() {}

	static TStruct Construct(Token* name, void* in_vars);

	static void RegisterLLVM(llvm::IRBuilder<>* builder, llvm::Module* module)
	{
		m_builder = builder;
		m_module = module;
	}

	llvm::StructType* GetLLVMStruct() { return m_llvm_struc; }

	TValue GetMember(Token* token, const std::string& lex);
	llvm::Value* GetGEPLoc(Token* token, const std::string& lex);

	bool IsValid() const { return m_valid; }

private:

	static llvm::IRBuilder<>* m_builder;
	static llvm::Module* m_module;

	bool m_valid;
	
	Token* m_token;
	std::string m_name;
	llvm::StructType* m_llvm_struc;
	std::map<std::string, llvm::Value*> m_gep_loc;
	std::map<std::string, TValue > m_type_map;
	std::vector<TValue> m_member_types;
	std::vector<std::string> m_member_names;

};


#endif // TSTRUCT_H