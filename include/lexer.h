#pragma once

#include "token.h"

#include <string>
#include <vector>

// matches token with keyword
void FindKeyword(
	const std::string& file,
	const int          line,

	std::vector<Token>& tokens,
	std::string& token
);

// scans code
std::vector<Token> Lexer(
	const std::string& file,
	const int          line,

	const std::string& fileLine
);