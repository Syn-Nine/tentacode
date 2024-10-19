#ifndef LITERAL_H
#define LITERAL_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <stdint.h>
#include <cfloat>
#include <cmath>
#include <memory>

#ifndef NO_RAYLIB
#include <raylib.h>
#endif

class FunctionStmt;
class StructStmt;
class FunctorExpr;
class Interpreter;
class Literal;

typedef std::vector<Literal> LiteralList;

enum LiteralTypeEnum
{
	LITERAL_TYPE_INVALID,
	LITERAL_TYPE_DOUBLE,
	LITERAL_TYPE_INTEGER,
	LITERAL_TYPE_STRING,
	LITERAL_TYPE_BOOL,
};

class Literal
{
public:
	

	Literal()
	{
		m_type = LITERAL_TYPE_INVALID;
		//m_isInstance = false;
	}

	Literal(double val)
	{
		m_doubleValue = val;
		m_intValue = int32_t(val);
		m_type = LITERAL_TYPE_DOUBLE;
	}

	Literal(int32_t val)
	{
		m_intValue = val;
		m_doubleValue = double(val);
		m_type = LITERAL_TYPE_INTEGER;
	}

	Literal(std::string val)
	{
		m_stringValue = val;
		m_type = LITERAL_TYPE_STRING;
	}

	Literal(bool val)
	{
		m_boolValue = val;
		m_type = LITERAL_TYPE_BOOL;
	}

	bool IsDouble() const { return m_type == LITERAL_TYPE_DOUBLE; }
	bool IsInt() const { return m_type == LITERAL_TYPE_INTEGER; }
	bool IsNumeric() const { return (m_type == LITERAL_TYPE_DOUBLE || m_type == LITERAL_TYPE_INTEGER); }
	bool IsString() const { return m_type == LITERAL_TYPE_STRING; }
	bool IsBool() const { return m_type == LITERAL_TYPE_BOOL; }
	bool IsInvalid() const { return m_type == LITERAL_TYPE_INVALID; }

	LiteralTypeEnum GetType() const { return m_type; }
	
	bool BoolValue() const { return m_boolValue; }
	std::string StringValue() const { return m_stringValue; }
	double DoubleValue() const{ return m_doubleValue; }
	int32_t IntValue() const{ return m_intValue; }

	std::string ToString() const;


private:

	double m_doubleValue;
	int32_t m_intValue;
	std::string m_stringValue;
	
	bool m_boolValue;
	LiteralTypeEnum m_type;
	
};

#endif // LITERAL_H