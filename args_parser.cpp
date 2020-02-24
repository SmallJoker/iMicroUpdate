#include "args_parser.h"

#include <cstdlib>
#include <iostream>
#include <vector>

std::vector<CLIArg *> g_args;

CLIArg::CLIArg(CONSTSTR prefix, CLIArgType type) :
	m_prefix("-" + prefix), m_type(type)
{
	g_args.emplace_back(this);
}

void CLIArg::parseArgs(int argc, char **argv)
{
	for (int i = 0; i < argc; ++i) {
		for (CLIArg *a : g_args) {
			if (a->m_prefix != argv[i])
				continue;

			if (a->getType() == CLIARG_FLAG) {
				a->parse(nullptr);
				break;
			}

			if (i + 1 < argc && a->parse(argv[i + 1]))
				++i;
			else
				std::cout << "Cannot parse argument '" << argv[i] << "'" << std::endl;

			break;
		}
	}
}

bool CLIArgS64::parse(const char *str)
{
	int status = sscanf(str, "%li", &m_value);
	return status == 1;
}

bool CLIArgStr::parse(const char *str)
{
	if (!str[0])
		return false;

	m_value = std::string(str);
	return true;
}
