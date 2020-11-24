#include "../include/interpreter.h"
#include "../include/night.h"
#include "../include/utils.h"
#include "../include/error.h"
#include "../include/token.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

Expression EvaluateExpression(const std::shared_ptr<Expression>& node, std::vector<NightVariable>& variables,
	const std::vector<NightFunction>& functions)
{
	assert(node != nullptr && "node should not be NULL");

	// if left and right node are NULL, then node must be a value
	if (node->left == nullptr && node->right == nullptr)
	{
		if (node->type == ValueType::VARIABLE)
		{
			const NightVariable* variableValue = GetContainer(variables, node->data);
			assert(variableValue != nullptr && "variableValue is not defined");

			return variableValue->data;
		}
		if (node->type == ValueType::CALL)
		{
			if (node->data == "input")
			{
				std::string uinput;
				getline(std::cin, uinput);

				return Expression{ ValueType::STRING, uinput };
			}

		    const NightFunction* function = GetContainer(functions, node->data);
			assert(function != nullptr && "function is not defined");

			// create variables from parameters
			assert(function->parameters.size() == node->extras.size() && "function parameters and function call don't match");
			for (std::size_t a = 0; a < function->parameters.size(); ++a)
			{
				variables.push_back(NightVariable{
					function->parameters[a],
					EvaluateExpression(node->extras[a], variables, functions)
				});
			}

			// interpret function body and extract return value
			std::unique_ptr<Expression> returnValue;
			Interpreter(function->body, &returnValue);

			assert(returnValue != nullptr && "function used in expression doesn't have a return statement");

			// remove variables since they are now out of scope
			variables.erase(
				variables.begin() + variables.size() - function->parameters.size(),
				variables.end()
			);

			return *returnValue;
		}

		return *node;
	}

	assert(node->type == ValueType::OPERATOR && "node should be operator");

	if (node->data == "+")
	{
		const Expression expr1 = EvaluateExpression(node->left, variables, functions);
		const Expression expr2 = EvaluateExpression(node->right, variables, functions);

		if (expr1.type == ValueType::STRING || expr2.type == ValueType::STRING)
		{
			return Expression{
				ValueType::STRING,
				expr1.data + expr2.data
			};
		}
		else
		{
			return Expression{
				expr1.type,
				std::to_string(std::stof(expr1.data) + std::stof(expr2.data))
			};
		}
	}
	if (node->data == "-")
	{
		if (node->left == nullptr)
		{
			Expression expression = EvaluateExpression(node->right, variables, functions);
			expression.data = std::to_string(-std::stof(expression.data));

			return expression;
		}

		EVAL_EXPR(std::to_string(std::stof(expr1.data) - std::stof(expr2.data)));
	}
	if (node->data == "*")
	{
		EVAL_EXPR(std::to_string(std::stof(expr1.data) * std::stof(expr2.data)));
	}
	if (node->data == "/")
	{
		EVAL_EXPR(std::to_string(std::stof(expr1.data) / std::stof(expr2.data)));
	}
	if (node->data == "%")
	{
		EVAL_EXPR(std::to_string(std::stoi(expr1.data) % std::stoi(expr2.data)));
	}
	if (node->data == ">")
	{
		EVAL_EXPR(std::stof(expr1.data) > std::stof(expr2.data) ? "true" : "false");
	}
	if (node->data == "<")
	{
		EVAL_EXPR(std::stof(expr1.data) < std::stof(expr2.data) ? "true" : "false");
	}
	if (node->data == ">=")
	{
		EVAL_EXPR(std::stof(expr1.data) >= std::stof(expr2.data) ? "true" : "false");
	}
	if (node->data == "<=")
	{
		EVAL_EXPR(std::stof(expr1.data) <= std::stof(expr2.data) ? "true" : "false");
	}
	if (node->data == "!")
	{
		Expression expression = EvaluateExpression(node->right, variables, functions);
		expression.data = expression.data == "true" ? "false" : "true";

		return expression;
	}
	if (node->data == "||")
	{
		EVAL_EXPR(expr1.data == "true" || expr2.data == "true" ? "true" : "false");
	}
	if (node->data == "&&")
	{
		EVAL_EXPR(expr1.data == "true" && expr2.data == "true" ? "true" : "false");
	}
	if (node->data == "==")
	{
		EVAL_EXPR(expr1.data == expr2.data ? "true" : "false");
	}
	if (node->data == "!=")
	{
		EVAL_EXPR(expr1.data != expr2.data ? "true" : "false");
	}
	if (node->data == "[]")
	{
		const Expression array = EvaluateExpression(node->right, variables, functions);
		const unsigned long index = std::stoul(EvaluateExpression(node->extras[0], variables, functions).data);

		if (array.extras.empty())
		{
			assert(index >= 0 && index < array.data.length() && "string subscript out of range");
			return Expression{ ValueType::STRING, std::string(1, array.data[index]) };
		}
		else
		{
			assert(index >= 0 && index < array.extras.size() && "array subscript out of range");
			return EvaluateExpression((array.extras[index]), variables, functions);
		}
	}
	if (node->data == "..")
	{
		const Expression start = EvaluateExpression(node->left, variables, functions);
		const Expression end = EvaluateExpression(node->right, variables, functions);

		Expression array{ ValueType::NUM_ARR };
		for (int a = std::stoi(start.data); a <= std::stoi(end.data); ++a)
		{
			array.extras.push_back(std::make_shared<Expression>(Expression{
				ValueType::NUM,
				std::to_string(a),
				std::vector<std::shared_ptr<Expression> >(),
				nullptr,
				nullptr
			}));
		}

		return array;
	}
	if (node->data == ":")
	{
		const Expression index = EvaluateExpression(node->left, variables, functions);
		const Expression value = EvaluateExpression(node->right, variables, functions);

		Expression coord{ ValueType::NUM };
		coord.data = index.data;
		coord.extras.push_back(std::make_shared<Expression>(value));

		return coord;
	}
	if (node->data == "<-")
	{
		const Expression index = EvaluateExpression(node->right, variables, functions);
		Expression array = EvaluateExpression(node->left, variables, functions);

		if (index.extras.empty())
		{
			array.extras.push_back(std::make_shared<Expression>(index));
		}
		else
		{
			array.extras.insert(
				array.extras.begin() + std::stoi(index.data),
				index.extras[0]
			);
		}

		return array;
	}
	if (node->data == "->")
	{
		const Expression index = EvaluateExpression(node->right, variables, functions);
		Expression array = EvaluateExpression(node->left, variables, functions);

		assert(!array.extras.empty() && "array shouldn't be empty");
		array.extras.erase(array.extras.begin() + std::stoi(index.data));

		return array;
	}

	assert_rtn(false && "operator missing", Expression());
}

void Interpreter(const std::vector<Statement>& statements, std::unique_ptr<Expression>* returnValue)
{
	static std::vector<NightVariable> variables;
	static std::vector<NightFunction> functions;

	const std::size_t variablesSize = variables.size();

	for (const Statement& statement : statements)
	{
		switch (statement.type)
		{
		case StatementType::VARIABLE: {
			variables.push_back(NightVariable{
				std::get<Variable>(statement.stmt).name,
				EvaluateExpression(std::get<Variable>(statement.stmt).value, variables, functions)
			});

			break;
		}
		case StatementType::ASSIGNMENT: {
			NightVariable* variable = GetContainer(variables, std::get<Assignment>(statement.stmt).name);
			assert(variable != nullptr && "definitions should be checked in the parser");

			variable->data = EvaluateExpression(std::get<Assignment>(statement.stmt).value, variables, functions);

			break;
		}
		case StatementType::CONDITIONAL: {
			if (EvaluateExpression(std::get<Conditional>(statement.stmt).condition, variables, functions).data == "true")
			{
				Interpreter(std::get<Conditional>(statement.stmt).body);
				break;
			}

			for (const Conditional& conditional : std::get<Conditional>(statement.stmt).chains)
			{
				if (conditional.condition == nullptr || EvaluateExpression(conditional.condition, variables, functions).data == "true")
				{
					Interpreter(conditional.body);
					break;
				}
			}

			break;
		}
		case StatementType::FUNCTION_DEF: {
			functions.push_back(NightFunction{
				std::get<FunctionDef>(statement.stmt).name,
				std::get<FunctionDef>(statement.stmt).parameters,
				std::get<FunctionDef>(statement.stmt).body
			});

			break;
		}
		case StatementType::FUNCTION_CALL: {
			if (std::get<FunctionCall>(statement.stmt).name == "print")
			{
				for (const std::shared_ptr<Expression>& parameter : std::get<FunctionCall>(statement.stmt).parameters)
					NightPrint(EvaluateExpression(parameter, variables, functions));

				break;
			}

			const NightFunction* function = GetContainer(functions, std::get<FunctionCall>(statement.stmt).name);
			assert(function != nullptr && "definitions should be checked in the parser");

			Interpreter(function->body);

			break;
		}
		case StatementType::RETURN: {
			assert(returnValue != nullptr && "returnValue should not be NULL");
			*returnValue = std::make_unique<Expression>(EvaluateExpression(std::get<Return>(statement.stmt).expression, variables, functions));

			variables.erase(variables.begin() + variablesSize, variables.end());
			return;
		}
		case StatementType::WHILE_LOOP: {
			while (EvaluateExpression(std::get<WhileLoop>(statement.stmt).condition, variables, functions).data == "true")
				Interpreter(std::get<WhileLoop>(statement.stmt).body);

			break;
		}
		case StatementType::FOR_LOOP: {
			const Expression range = EvaluateExpression(std::get<ForLoop>(statement.stmt).range, variables, functions);

			variables.push_back(NightVariable{ std::get<ForLoop>(statement.stmt).iterator, Expression() });
			NightVariable* index = &variables.back();

			for (const std::shared_ptr<Expression>& rangeValue : range.extras)
			{
				index->data = EvaluateExpression(rangeValue, variables, functions);
				Interpreter(std::get<ForLoop>(statement.stmt).body);
			}

			break;
		}
		case StatementType::ELEMENT: {
			NightVariable* variable = GetContainer(variables, std::get<Element>(statement.stmt).name);
			assert(variable != nullptr && "definitions should be checked in the parser");

			const std::size_t index = std::stoi(EvaluateExpression(std::get<Element>(statement.stmt).index, variables, functions).data);

			if (variable->data.type == ValueType::STRING)
				variable->data.data[index] = EvaluateExpression(std::get<Element>(statement.stmt).assign, variables, functions).data[0];
			else
				variable->data.extras[index] = std::make_shared<Expression>(EvaluateExpression(std::get<Element>(statement.stmt).assign, variables, functions));

			break;
		}
		default: {
			assert(false && "statement missing");
		}
		}
	}

	variables.erase(variables.begin() + variablesSize, variables.end());
}