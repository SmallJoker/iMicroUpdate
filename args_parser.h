#pragma once

#include <cctype>
#include <string>

#define CONSTSTR const std::string &

enum CLIArgType {
	CLIARG_NONE,
	CLIARG_S64,
	CLIARG_STR
};

// Meta container

class CLIArg {
public:
	CLIArg() = delete;
	CLIArgType getType() const { return m_type; }

	static void parseArgs(int argc, char **argv);
protected:
	CLIArg(CONSTSTR prefix, CLIArgType type);
	virtual bool parse(const char *str) { return false; }

	std::string m_prefix;
	CLIArgType m_type;
};

// Various data types

class CLIArgS64 : public CLIArg {
public:
	CLIArgS64(CONSTSTR prefix, int64_t def = 0) :
		CLIArg(prefix, CLIARG_S64), m_value(def) {}
	int64_t get() const { return m_value; }
protected:
	bool parse(const char *str);
	int64_t m_value;
};

class CLIArgStr : public CLIArg {
public:
	CLIArgStr(CONSTSTR prefix, CONSTSTR def = "") :
		CLIArg(prefix, CLIARG_STR), m_value(def) {}
	CONSTSTR get() const { return m_value; }
protected:
	bool parse(const char *str);
	std::string m_value;
};
