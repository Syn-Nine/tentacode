#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <map>
#include <string>

#include "Literal.h"
#include "Token.h"
#include "ErrorHandler.h"
#include "Utility.h"

class Environment
{
public:
	Environment() = delete;
	Environment(ErrorHandler* errorHandler)
	{
		m_errorHandler = errorHandler;
		m_parent = nullptr;
	}

	Environment(Environment* parent, ErrorHandler* errorHandler)
	{
		m_errorHandler = errorHandler;
		m_parent = parent;
		parent->m_children.push_back(this);
		m_scopeLabel = m_parent->m_nextScopeLabel;
	}

	~Environment()
	{
		for (auto& p : m_parent->m_children)
		{
			if (p == this)
			{
				p = nullptr;
				break;
			}
		}

		for (auto& p : m_children)
		{
			if (p) delete p;
		}
	}

	void Clear()
	{
		for (size_t i = 0; i < m_children.size(); ++i)
		{
			if (m_children[i]) m_children[i]->Clear();
		}

		NameSpaceMap nsmap;

		for (auto& n : m_namespaces)
		{
			var_struct vs;
			for (auto& v : n.second.vars)
			{
				if (v.second.IsCallable())
				{
					vs.vars.insert(std::make_pair(v.first, v.second));
					vs.privacy.insert(std::make_pair(v.first, n.second.privacy.at(v.first)));
				}
			}
			if (!vs.vars.empty())
			{
				nsmap.insert(std::make_pair(n.first, vs));
			}
		}

		m_namespaces.clear();
		m_namespaces = nsmap;
		//m_fqns.clear();;
		//m_scopeLabel.clear();
		//m_nextScopeLabel.clear();
	}

	void Assign(std::string name, Literal value, int index, std::string fqns)
	{
		//printf("Environment::Assign: %s, %s, %d\n", name.c_str(), value.ToString().c_str(), index);

		if (std::string::npos != name.find_first_of("::"))
		{
			m_errorHandler->Error("", 0, "Cannot assign to a namespaced variable: '" + name + "'.");
			return;
		}

		if (0 == m_namespaces.count(fqns))
		{
			if (m_parent)
			{
				m_parent->Assign(name, value, index, fqns);
				return;
			}

			m_errorHandler->Error("", 0, "Undefined variable '" + name + "' in namespace '" + fqns + "'.");
			return;
		}

		VarMap& vars = m_namespaces.at(fqns).vars;

		if (vars.count(name) != 0)
		{
			// check type
			Literal& v = vars.at(name);
			if (v.IsRange())
			{
				m_errorHandler->Error("", 0, "Unable to assign range.");
			}
			else
			{
				if (v.IsInt() && value.IsDouble()) value = Literal(int32_t(value.DoubleValue()));
				if (v.IsDouble() && value.IsInt()) value = Literal(double(value.IntValue()));
				if (v.GetType() == value.GetType())
				{
					v = value;
				}
				else if (v.IsVector())
				{
					if (index < v.Len() && index != -1)
					{
						if (v.IsVecBool())
							v.SetValueAt(value.BoolValue(), index);
						else if (v.IsVecInteger())
							v.SetValueAt(value.IntValue(), index);
						else if (v.IsVecDouble())
							v.SetValueAt(value.DoubleValue(), index);
						else if (v.IsVecString())
							v.SetValueAt(value.StringValue(), index);
						else if (v.IsVecEnum())
							v.SetValueAt(value.EnumValue(), index);
						else if (v.IsVecStruct())
							v.SetValueAt(value, index);
					}
					else if (-1 == index)
					{
						m_errorHandler->Error("", 0, "No vector index provided.");
					}
					else
					{
						m_errorHandler->Error("", 0, "Vector index [" + std::to_string(index) + "] out of bounds during assignment (Size: " + std::to_string(v.Len()) + ").");
					}
				}
				else
				{
					m_errorHandler->Error("", 0, "Unable to cast between types during assignment (" + v.ToString() + ", " + value.ToString() + ").");
				}
			}
			return;
		}

		if (m_parent)
		{
			m_parent->Assign(name, value, index, fqns);
			return;
		}

		m_errorHandler->Error("", 0, "Undefined variable '" + name + "' in namespace '" + fqns + "'.");
	}

	void Define(std::string name, Literal value, std::string fqns, bool internal = false)
	{
		if (std::string::npos != name.find_first_of("::"))
		{
			m_errorHandler->Error("", 0, "Cannot explicitly define a namespaced variable: '" + name + "'.");
			return;
		}

		if (0 == m_namespaces.count(fqns))
		{
			var_struct vs;
			m_namespaces.insert(std::make_pair(fqns, vs));
		}

		VarMap& vars = m_namespaces.at(fqns).vars;
		PrivacyMap& privacy = m_namespaces.at(fqns).privacy;

		if (!value.IsRange())
		{
			// check for redefinition
			if (vars.count(name) != 0)
			{
				if (vars.at(name).IsFunctionDef())
				{
					printf("Warning. Overwriting Function Definition for '%s'.", name.c_str());
				}
				else if (vars.at(name).IsStructDef())
				{
					printf("Warning. Overwriting Struct Definition for '%s'.", name.c_str());
				}
				vars.at(name) = value;
				privacy.at(name) = internal;
			}
			else
			{
				vars.insert(std::make_pair(name, value));
				privacy.insert(std::make_pair(name, internal));
			}
		}
		else
		{
			m_errorHandler->Error("", 0, "Unable to assign range type.");
		}
	}

	Literal Get(Token* token, std::string fqns)
	{
		std::string name = token->Lexeme();
		bool external_access = false;
		std::string base_ns = fqns;

		std::string subnamespace;

		size_t pos = name.find_last_of(":");
		if (std::string::npos != pos)
		{
			external_access = true;

			//m_errorHandler->Error("", 0, "Get: '" + name + "', fqns: '" + fqns + "'.");
			int global = name.find("global::");
			int ray = name.find("ray::");
			if (0 == global || 0 == ray)
			{
				fqns = name.substr(0, pos + 1);
				name = name.substr(pos + 1);
			}
			else
			{
				subnamespace = name.substr(0, pos + 1);
				name = name.substr(pos + 1);
			}
		}

		// nothing here, go to parent
		if (m_namespaces.empty() && m_parent)
		{
			return m_parent->Get(token, fqns);
		}

		// local search
		if (0 != m_namespaces.count(fqns + subnamespace) && 0 != m_namespaces.at(fqns + subnamespace).vars.count(name))
		{
			fqns = fqns + subnamespace;
		}
		else
		{
			// deep search
			bool found = false;
			std::vector<std::string> namestack = StrSplit(fqns, "::");
			std::vector<std::string> options;
			for (auto& s : namestack)
			{
				if (s.empty()) break;
				if (options.empty())
				{
					options.push_back(s + "::");
				}
				else
				{
					options.push_back(options.back() + s + "::");
				}
			}
			for (int i = options.size() - 1; i >= 0; --i)
			{
				if (0 != m_namespaces.count(options[i] + subnamespace) && 0 != m_namespaces.at(options[i] + subnamespace).vars.count(name))
				{
					fqns = options[i] + subnamespace;
					found = true;
					break;
				}
			}

			if (!found)
			{
				if (m_parent) return m_parent->Get(token, fqns);

				m_errorHandler->Error(token->Filename(), token->Line(), "Undefined variable '" + name + "' in namespace '" + fqns + "'.");
				return Literal();
			}
		}

		if (external_access && 0 == fqns.compare(base_ns) && m_namespaces.at(fqns).privacy.at(name))
		{
			m_errorHandler->Error(token->Filename(), token->Line(), "Cannot access internal '" + name + "' from '" + fqns + "'.");
			return Literal();
		}

		VarMap& vars = m_namespaces.at(fqns).vars;

		if (vars.count(name))
		{
			return vars.at(name);
		}

		if (m_parent) return m_parent->Get(token, fqns);

		m_errorHandler->Error(token->Filename(), token->Line(), "Undefined variable '" + name + "' in namespace '" + fqns + "'.");

		return Literal();
	}

	/*void Print(std::string t = "")
	{
		std::string pre = "-" + t;
		if (m_parent) m_parent->Print(pre);

		for (auto& v : m_vars)
		{
			std::string name = v.first;
			std::string val = v.second.ToString();

			printf("%s %s = %s\n", pre.c_str(), name.c_str(), val.c_str());
		}
	}*/

	std::string GetNearestScopeLabel() const
	{
		if (!m_scopeLabel.empty()) return m_scopeLabel;
		if (m_parent) return m_parent->GetNearestScopeLabel();
		return "";
	}
	
	void SetNextScopeLabel(std::string label)
	{
		m_nextScopeLabel = label;
	}


private:
	typedef std::map<std::string, Literal> VarMap;
	typedef std::map<std::string, bool> PrivacyMap;
	
	struct var_struct {
		VarMap vars;
		PrivacyMap privacy;
	};

	typedef std::map<std::string, var_struct> NameSpaceMap;

	NameSpaceMap m_namespaces;
	//std::map<std::string, std::string> m_fqns;
	std::string m_scopeLabel;
	std::string m_nextScopeLabel;

	ErrorHandler* m_errorHandler;
	Environment* m_parent;
	std::vector<Environment*> m_children;

};

#endif // ENVIRONMENT_H