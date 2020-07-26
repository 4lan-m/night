#pragma once

#include <vector>

#include "BitParser.h"
#include "IntParser.h"
#include "DecParser.h"
#include "StrParser.h"

#include "Squid.h"
#include "Output.h"

#include "Variable.h"
#include "Function.h"
#include "Token.h"

bool CheckVariable(const std::string& var, const std::vector<Variable>& vars)
{
	for (std::size_t a = 0; a < vars.size(); ++a)
	{
		if (var == vars[a].name)
			return false;
	}

	return true;
}

void ExtractVariables(std::size_t s, std::size_t e, std::vector<Token>& expr,
	const std::vector<Variable>& vars)
{
	bool isDefined;
	for (std::size_t a = s; a < e; ++a)
	{
		if (expr[a].type == TokenType::VARIABLE)
		{
			isDefined = false;
			for (std::size_t b = 0; b < vars.size(); ++b)
			{
				if (expr[a].token == vars[b].name)
				{
					expr[a].type = vars[b].type == "bit" ? TokenType::BIT_VALUE :
						vars[b].type == "syb" ? TokenType::SYB_VALUE :
						vars[b].type == "int" ? TokenType::INT_VALUE :
						vars[b].type == "dec" ? TokenType::DEC_VALUE :
						vars[b].type == "str" ? TokenType::STR_VALUE :
						throw DYNAMIC_SQUID(0);


					expr[a].token = vars[b].value;

					isDefined = true;
					break;
				}
			}

			if (!isDefined)
				throw _undefined_variable_("variable '" + expr[a].token + "' is not defined");
		}
	}
}

void SeparateTokens(std::vector<Token>& tokens, int s, int e);

void Parser(std::vector<Token>& tokens)
{
	static std::vector<Variable> variables;
	static std::vector<Function> functions;
	
	static bool inStatement = false;
	static bool ifElseStatement = false;

	// variable declaration
	if (tokens.size() == 3 && tokens[0].type != TokenType::PRINT && tokens[1].type == TokenType::VARIABLE &&
		tokens[2].type == TokenType::SEMICOLON)
	{
		switch (tokens[0].type)
		{
		case TokenType::BIT_TYPE:
			variables.push_back(Variable{ "bit", tokens[1].token, "false" });
			break;
		case TokenType::SYB_TYPE:
			variables.push_back(Variable{ "syb", tokens[1].token, " " });
			break;
		case TokenType::INT_TYPE:
			variables.push_back(Variable{ "int", tokens[1].token, "0" });
			break;
		case TokenType::DEC_TYPE:
			variables.push_back(Variable{ "dec", tokens[1].token, "0.0" });
			break;
		case TokenType::STR_TYPE:
			variables.push_back(Variable{ "str", tokens[1].token, "" });
			break;
		default:
			throw _undefined_data_type_("data type '" + tokens[0].token + "' is undefined");
		}
	}
	// variable initialization
	else if (tokens.size() >= 5 && tokens[1].type == TokenType::VARIABLE &&
		tokens[2].type == TokenType::ASSIGNMENT && tokens.back().type == TokenType::SEMICOLON)
	{
		if (!CheckVariable(tokens[1].token, variables))
			throw _variable_redefinition_("variable '" + tokens[1].token + "' already defined");

		ExtractVariables(3, tokens.size() - 1, tokens, variables);

		std::vector<Token> expression(tokens.begin() + 3, tokens.end() - 1);

		switch (tokens[0].type)
		{
		case TokenType::BIT_TYPE:
			variables.push_back(Variable{ "bit", tokens[1].token, BitParser(expression) });
			break;
		case TokenType::SYB_TYPE:
			if (tokens.size() > 5)
				throw _invalid_syb_expr_;

			variables.push_back(Variable{ "syb", tokens[1].token, tokens[3].token });
			break;
		case TokenType::INT_TYPE:
			variables.push_back(Variable{ "int", tokens[1].token, IntParser(expression) });
			break;
		case TokenType::DEC_TYPE:
			variables.push_back(Variable{ "dec", tokens[1].token, DecParser(expression) });
			break;
		case TokenType::STR_TYPE:
			variables.push_back(Variable{ "str", tokens[1].token, StrParser(expression) });
			break;
		default:
			throw _undefined_data_type_("data type '" + tokens[0].token + "' is undefined");
		}
	}
	// variable assignment
	else if (tokens.size() >= 4 && tokens[0].type == TokenType::VARIABLE &&
		tokens[1].type == TokenType::ASSIGNMENT && tokens.back().type == TokenType::SEMICOLON)
	{
		if (CheckVariable(tokens[0].token, variables))
			throw _undefined_variable_("variable '" + tokens[0].token + "' is undefined");

		ExtractVariables(2, tokens.size() - 1, tokens, variables);

		int variableIndex = -1;
		for (std::size_t a = 0; a < variables.size(); ++a)
		{
			if (tokens[0].token == variables[a].name)
			{
				variableIndex = a;
				break;
			}
		}

		if (variableIndex == -1)
			throw DYNAMIC_SQUID(0);

		if (variables[variableIndex].type == "bool")
		{
			std::vector<Token> temp(tokens.begin() + 2, tokens.end() - 1);
			variables[variableIndex].value = BitParser(temp);
		}
		else if (variables[variableIndex].type == "char")
		{
			if (tokens.size() == 4 && tokens[2].type == TokenType::SYB_VALUE)
				variables[variableIndex].value = tokens[2].token;
			else
				throw _invalid_syb_expr_;
		}
		else if (variables[variableIndex].type == "int")
		{
			std::vector<Token> temp(tokens.begin() + 2, tokens.end() - 1);
			variables[variableIndex].value = IntParser(temp);
		}
		else if (variables[variableIndex].type == "dec")
		{
			std::vector<Token> temp(tokens.begin() + 2, tokens.end() - 1);
			variables[variableIndex].value = DecParser(temp);
		}
		else if (variables[variableIndex].type == "str")
		{
			std::vector<Token> temp(tokens.begin() + 2, tokens.end() - 1);
			variables[variableIndex].value = StrParser(temp);
		}
		else
		{
			throw DYNAMIC_SQUID(0);
		}
	}
	// print
	else if (tokens.size() >= 3 && tokens[0].type == TokenType::PRINT &&
		tokens.back().type == TokenType::SEMICOLON)
	{
		ExtractVariables(1, tokens.size() - 1, tokens, variables);

		std::vector<Token> temp(tokens.begin() + 1, tokens.end() - 1);

		switch (tokens[1].type)
		{
		case TokenType::BIT_VALUE:
			storeOutput(BitParser(temp));
			break;
		case TokenType::SYB_VALUE:
			if (tokens.size() == 3)
				storeOutput(tokens[1].token);
			else
				throw _invalid_syb_expr_;

			break;
		case TokenType::INT_VALUE:
			storeOutput(IntParser(temp));
			break;
		case TokenType::DEC_VALUE:
			storeOutput(DecParser(temp));
			break;
		case TokenType::STR_VALUE:
			storeOutput(StrParser(temp));
			break;
		default:
			throw _invalid_print_;
		}
	}
	// if statement
	else if (tokens.size() >= 6 && tokens[0].type == TokenType::IF &&
		tokens[1].type == TokenType::OPEN_BRACKET && tokens.back().type == TokenType::CLOSE_CURLY)
	{
		inStatement = true;

		for (std::size_t a = 0; a < tokens.size(); ++a)
		{
			if (tokens[a].type == TokenType::OPEN_CURLY)
			{
				ExtractVariables(2, a - 1, tokens, variables);

				std::vector<Token> temp(tokens.begin() + 2, tokens.begin() + a - 1);
				if (BitParser(temp) == "true")
				{
					inStatement = false;

					SeparateTokens(tokens, a + 1, 1);

					inStatement = true;
					ifElseStatement = false;
				}
				else
				{
					ifElseStatement = true;
				}

				break;
			}
		}
	}
	// else if statement
	else if (tokens.size() >= 7 && tokens[0].type == TokenType::ELSE && tokens[1].type == TokenType::IF && 
		tokens[2].type == TokenType::OPEN_BRACKET && tokens.back().type == TokenType::CLOSE_CURLY)
	{
		if (!inStatement)
			throw _invalid_if_statement_;

		inStatement = true;

		if (ifElseStatement)
		{
			for (std::size_t a = 0; a < tokens.size(); ++a)
			{
				if (tokens[a].type == TokenType::OPEN_CURLY)
				{
					ExtractVariables(3, a - 1, tokens, variables);

					std::vector<Token> temp(tokens.begin() + 3, tokens.begin() + a - 1);
					if (BitParser(temp) == "true")
					{	
						inStatement = false;

						SeparateTokens(tokens, a + 1, 1);

						inStatement = true;
						ifElseStatement = false;
					}
					else
					{
						ifElseStatement = true;
					}

					break;
				}
			}
		}
	}
	// else statement
	else if (tokens.size() >= 3 && tokens[0].type == TokenType::ELSE &&
		tokens[1].type == TokenType::OPEN_CURLY && tokens.back().type == TokenType::CLOSE_CURLY)
	{
		if (!inStatement)
			throw _invalid_if_statement_;

		inStatement = true;

		if (ifElseStatement)
		{
			inStatement = false;

			SeparateTokens(tokens, 2, 1);

			inStatement = false;
			ifElseStatement = false;
		}
	}
	// function definition
	else if (tokens.size() >= 6 && tokens[0].type == TokenType::FUNC_TYPE &&
		tokens[1].type == TokenType::VARIABLE && tokens[2].type == TokenType::OPEN_BRACKET &&
		tokens.back().type == TokenType::CLOSE_CURLY)
	{
		functions.push_back(Function{tokens[1].token});

		int index = 5;
		for (std::size_t a = 3; tokens[a].type != TokenType::CLOSE_BRACKET; a += 2)
		{
			if (tokens[a].token == "bit" || tokens[a].token == "syb" || tokens[a].token == "int" ||
				tokens[a].token == "dec" || tokens[a].token == "str")
			{
				if (tokens[a + 1].type == TokenType::VARIABLE)
				{
					for (Variable param : functions.back().parameters)
					{
						if (tokens[a + 1].token == param.name)
							throw _invalid_parameter_;
					}

					functions.back().parameters.push_back(Variable{
						tokens[a].token, tokens[a + 1].token, "" });

					if (tokens[a + 2].type == TokenType::COMMA)
					{
						a += 1;
					}
					else if (tokens[a + 2].type == TokenType::CLOSE_BRACKET)
					{
						index = a + 4;
						break;
					}
					else
					{
						throw _invalid_parameter_;
					}
				}
				else
				{
					throw _invalid_parameter_;
				}
			}
			else if (tokens[a].type != TokenType::CLOSE_BRACKET)
			{
				throw _invalid_parameter_;;
			}
		}

		for (std::size_t a = index; a < tokens.size() - 1; ++a)
		{
			if (tokens[a].type == TokenType::FUNC_TYPE)
				throw _invalid_function_;

			functions.back().functionCode.push_back(tokens[a]);
		}
	}
	// function call
	else if (tokens.size() >= 4 && tokens[0].type == TokenType::VARIABLE &&
		tokens[1].type == TokenType::OPEN_BRACKET && tokens.back().type == TokenType::SEMICOLON)
	{
		int functionIndex = -1;
		for (std::size_t a = 0; a < functions.size(); ++a)
		{
			if (tokens[0].token == functions[a].name)
			{
				functionIndex = a;
				break;
			}
		}

		if (functionIndex == -1)
			throw _invalid_parameter_;

		int variableIndex = variables.size();

		int sC = 2;
		for (std::size_t a = 2; a < tokens.size() - 2; ++a)
		{
			if ((functions[functionIndex].parameters[a - sC].type == "bit" && 
					tokens[a].type == TokenType::BIT_VALUE) ||
				(functions[functionIndex].parameters[a - sC].type == "syb" &&
					tokens[a].type == TokenType::SYB_VALUE) ||
				(functions[functionIndex].parameters[a - sC].type == "int" &&
					tokens[a].type == TokenType::INT_VALUE) ||
				(functions[functionIndex].parameters[a - sC].type == "dec" &&
					tokens[a].type == TokenType::DEC_VALUE) ||
				(functions[functionIndex].parameters[a - sC].type == "str" &&
					tokens[a].type == TokenType::STR_VALUE))
			{
				functions[functionIndex].parameters[a - sC].value = tokens[a].token;
				variables.push_back(Variable{ functions[functionIndex].parameters[a - sC].type,
					functions[functionIndex].parameters[a - sC].name, tokens[a].token });

				if (tokens[a + 1].type == TokenType::CLOSE_BRACKET)
					break;

				if (tokens[a + 1].type != TokenType::COMMA)
					throw _invalid_parameter_;

				sC += 1;
				a += 1;
			}
			else if (tokens[a].type == TokenType::CLOSE_BRACKET)
			{
				break;
			}
			else
			{
				throw _invalid_parameter_;
			}
		}

		bool isDefined = false;
		for (std::size_t a = 0; a < functions.size(); ++a)
		{
			if (tokens[0].token == functions[a].name)
			{
				SeparateTokens(functions[a].functionCode, 0, 0);

				variables.erase(variables.begin() + variableIndex, variables.end());

				isDefined = true;
				break;
			}
		}

		if (!isDefined)
			throw _undefined_function_("function '" + tokens[0].token + "' is undefined");
	}
	// error
	else
	{
		throw _invalid_grammar_;
	}
}

void SeparateTokens(std::vector<Token>& tokens, int s, int e)
{
	std::vector<Token> temp;

	int openCurly = 0;
	for (std::size_t a = s; a < tokens.size() - e; ++a)
	{
		temp.push_back(tokens[a]);

		if (tokens[a].type == TokenType::OPEN_CURLY)
			openCurly += 1;
		else if (tokens[a].type == TokenType::CLOSE_CURLY)
			openCurly -= 1;

		if (openCurly == 0 && (tokens[a].type == TokenType::SEMICOLON ||
			tokens[a].type == TokenType::CLOSE_CURLY))
		{
			Parser(temp);
			temp.clear();
		}
	}
}