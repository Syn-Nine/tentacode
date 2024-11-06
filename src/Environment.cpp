#include "Environment.h"

std::map<std::string, int> Environment::m_enumMap;
std::map<int, std::string> Environment::m_enumReflectMap;
int Environment::m_enumCounter = 0;
int Environment::m_debugLevel = 0;
ErrorHandler* Environment::m_errorHandler = nullptr;