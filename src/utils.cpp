#include "../include/utils.hpp"
#include "../include/token.hpp"

#include <vector>

std::vector<std::vector<Token> > SplitCode(const std::vector<Token>& tokens)
{
	std::vector<std::vector<Token> > code;
	for (std::size_t a = 0, openCurlyCount = 0; a < tokens.size(); ++a)
	{
		if (a == 0)
			code.push_back(std::vector<Token>());

		if (tokens[a].type == TokenType::EOL && openCurlyCount == 0)
		{
			if (a < tokens.size() - 1)
				code.push_back(std::vector<Token>());

			continue;
		}

		if (tokens[a].type == TokenType::OPEN_CURLY)
			openCurlyCount++;
		else if (tokens[a].type == TokenType::CLOSE_CURLY)
			openCurlyCount--;

		code.back().push_back(tokens[a]);
	}

	return code;
}

bool night::find_type(const std::vector<VariableType>& container, const VariableType& var_type)
{
	assert(var_type.type != VariableType::CLASS && "shitihisuhf");

	for (const VariableType& contain_type : container)
	{
		if (var_type == contain_type.type)
			return true;
	}

	return false;
}