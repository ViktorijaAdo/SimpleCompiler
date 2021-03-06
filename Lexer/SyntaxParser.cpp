#include "pch.h"
#include "SyntaxParser.h"

using namespace std;

WhileStatement* Node::findCycleParent()
{
	auto nodeParent = parent;
	while (nodeParent != nullptr)
	{
		if (typeid(*nodeParent) == typeid(struct WhileStatement))
		{
			return dynamic_cast<WhileStatement*>(&(*nodeParent));
		}
		nodeParent = nodeParent->parent;
	}

	return nullptr;
}

Function* Node::findFunctionParent()
{
	auto nodeParent = parent;
	while (nodeParent != nullptr)
	{
		if (typeid(*nodeParent) == typeid(Function))
		{
			return dynamic_cast<Function*>(&(*nodeParent));
		}
		nodeParent = nodeParent->parent;
	}

	return nullptr;
}

bool Parser::accept(LexemType type)
{
	if (lexems[offset]->type == type)
	{
		offset++;
		return true;
	}

	return false;
}

Identifier_Ptr Parser::parse_identifier()
{
	if (accept(Ident))
	{
		Identifier_Ptr ident = make_shared<Identifier>();
		ident->Value = lexems[offset - 1]->value_s;
		ident->LineNumber = lexems[offset - 1]->lineNumber;
		return ident;
	}

	return nullptr;
}

TypeKeyword_Ptr Parser::parse_type()
{
	TypeKeyword_Ptr result = nullptr;
	if (accept(Bool_type))
		result = make_shared<BoolKeyword>();
	else if (accept(Integer_type))
		result = make_shared<IntegerKeyword>();
	else if (accept(Float_type))
		result = make_shared<FloatKeyword>();
	else if (accept(String_type))
		result = make_shared<StringKeyword>();
	else if (accept(Char_type))
		result = make_shared<CharKeyword>();

	if (result != nullptr)
		result->LineNumber = lexems[offset - 1]->lineNumber;

	return result;
}

TypeKeyword_Ptr Parser::parse_return_type()
{
	if (accept(Void_type))
	{
		VoidKeyword_Ptr result = make_shared<VoidKeyword>();
		result->LineNumber = lexems[offset - 1]->lineNumber;
		return result;
	}
	else
		return parse_type();
}

IdentifierExpression_Ptr Parser::parse_identifier_expression()
{
	if (accept(Ident))
	{
		auto name = lexems[offset - 1]->value_s;
		IdentifierExpression_Ptr ident;
		if (accept(SQBracket_open))
		{
			auto memberAccess = make_shared<MemberAccessExpression>();
			ident = memberAccess;

			memberAccess->AccessPosition = parse_expression();

			if (!accept(SQBracket_close))
				ErrorHandler::printError(lexems[offset - 1]->lineNumber, "expected matching closing bracket");
		}
		else
		{
			ident = make_shared<IdentifierExpression>();
		}
		ident->Value = name;
		ident->LineNumber = lexems[offset - 1]->lineNumber;
		return ident;
	}

	return nullptr;
}

ParamDef_Ptr Parser::parse_parameter()
{
	ParamDef_Ptr param = make_shared<ParamDef>();
	if (accept(Seperator))
	{
		param->Seperator = make_shared<SeperatorToken>();
		param->Seperator->LineNumber = lexems[offset - 1]->lineNumber;
		param->LineNumber = param->Seperator->LineNumber;
	}

	param->Type = parse_type();
	if (accept(SQBracket_open))
	{
		param->Type->IsPointer = true;
		if (!accept(SQBracket_close))
			ErrorHandler::printError(lexems[offset - 1]->lineNumber, "Expected matching closing bracket");
	}
	param->Name = parse_identifier();
	if (param->Name == nullptr && param->Type == nullptr && param->Seperator == nullptr)
		return nullptr;

	if (param->Name == nullptr && param->Type == nullptr)
		ErrorHandler::printError(param->Seperator->LineNumber, "Unexpected extra , token");
	else if (param->Type == nullptr)
		ErrorHandler::printError(param->Name->LineNumber, "missing type specifier");
	else if (param->Name == nullptr)
		ErrorHandler::printError(param->Type->LineNumber, "missing parameter identifier");

	return param;
}

FunctionParamsDef_Ptr Parser::parse_parameters_list()
{
	if (!accept(Paran_open))
		return nullptr;

	FunctionParamsDef_Ptr result = make_shared<FunctionParamsDef>();
	result->ListStartToken = make_shared<OpenParan>();
	result->ListStartToken->LineNumber = lexems[offset - 1]->lineNumber;
	result->LineNumber = result->ListStartToken->LineNumber;

	while (true)
	{
		ParamDef_Ptr parameter = parse_parameter();
		if (parameter == nullptr)
			break;

		result->Parameters.push_back(parameter);
		if (result->Parameters.size() == 1)
		{
			if (parameter->Seperator != nullptr)
				ErrorHandler::printError(parameter->Seperator->LineNumber, "Unexpected seperator token");
		}
		else
		{
			if (parameter->Seperator == NULL)
				ErrorHandler::printError(parameter->LineNumber, "Expected separator token");
		}
	}
	if (accept(Paran_close))
	{
		result->ListEndToken = make_shared<CloseParan>();
		result->ListEndToken->LineNumber = lexems[offset - 1]->lineNumber;
	}
	else
	{
		ErrorHandler::printError(result->LineNumber, "Expected ), but found something else");
	}

	return result;
}

StatementEnd_Ptr Parser::parse_statement_end_token()
{
	StatementEnd_Ptr result = nullptr;
	if (!accept(Statement_end))
	{
		ErrorHandler::printError(lexems[offset]->lineNumber, "Unterminated statement. Expected ;");
	}
	else
	{
		result = make_shared<StatementEnd>();
		result->LineNumber = lexems[offset - 1]->lineNumber;
	}

	return result;
}

StringExpression_Ptr Parser::parse_string_expression()
{
	StringExpression_Ptr result = nullptr;
	if (!accept(String))
		return result;

	result = make_shared<StringExpression>();
	result->Value = lexems[offset - 1]->value_s;
	result->LineNumber = lexems[offset - 1]->lineNumber;

	return result;
}

CharExpression_Ptr Parser::parse_char_expression()
{
	CharExpression_Ptr result = nullptr;
	if (!accept(Char))
		return result;

	result = make_shared<CharExpression>();
	result->Value = lexems[offset - 1]->value_s[0];
	result->LineNumber = lexems[offset - 1]->lineNumber;

	return result;
}

IntegerExpression_Ptr Parser::parse_integer_expression()
{
	IntegerExpression_Ptr result = nullptr;
	if (!accept(Numbers))
		return result;

	result = make_shared<IntegerExpression>();
	result->Value = lexems[offset - 1]->value_i;
	result->LineNumber = lexems[offset - 1]->lineNumber;

	return result;
}

FloatExpression_Ptr Parser::parse_float_expression()
{
	FloatExpression_Ptr result = nullptr;
	if (!accept(Float))
		return result;

	result = make_shared<FloatExpression>();
	result->Value = lexems[offset - 1]->value_f;
	result->LineNumber = lexems[offset - 1]->lineNumber;

	return result;
}

TrueExpresion_Ptr Parser::parse_true_expression()
{
	TrueExpresion_Ptr result = nullptr;
	if (!accept(Const_true))
		return result;

	result = make_shared<TrueExpresion>();
	result->LineNumber = lexems[offset - 1]->lineNumber;
	return result;
}

FalseExpresion_Ptr Parser::parse_false_expression()
{
	FalseExpresion_Ptr result = nullptr;
	if (!accept(Const_false))
		return result;

	result = make_shared<FalseExpresion>();
	result->LineNumber = lexems[offset - 1]->lineNumber;
	return result;
}

GroupedExpression_Ptr Parser::parse_grouped_expression()
{
	GroupedExpression_Ptr result = nullptr;
	if (!accept(Paran_open))
		return result;

	result = make_shared<GroupedExpression>();
	result->GroupStart = make_shared<OpenParan>();
	result->GroupStart->LineNumber = lexems[offset - 1]->lineNumber;
	result->LineNumber = result->GroupStart->LineNumber;

	result->Expression = parse_expression();
	if (result->Expression == nullptr)
		ErrorHandler::printError(lexems[offset - 1]->lineNumber, "Expected expression");
	else
		result->Expression->parent = result;

	while (!accept(Paran_close))
	{
		if (accept(EndOfFile))
		{
			ErrorHandler::printError(result->GroupStart->LineNumber, "Unmatched paranthesis");
			offset--;
			return nullptr;
		}
		offset++;
	}

	result->GroupFinish = make_shared<CloseParan>();
	result->GroupFinish->LineNumber = lexems[offset - 1]->lineNumber;

	return result;
}

NotExpression_Ptr Parser::parse_not_expression()
{
	NotExpression_Ptr result = nullptr;
	if (!accept(Op_not))
		return result;

	result = make_shared<NotExpression>();
	result->Prefix = make_shared<OpNot>();
	result->Prefix->LineNumber = lexems[offset - 1]->lineNumber;
	result->LineNumber = result->Prefix->LineNumber;

	result->Expression = parse_value_expression();
	if (result->Expression == nullptr)
		ErrorHandler::printError(lexems[offset]->lineNumber, "Expected expression");
	else
		result->Expression->parent = result;

	return result;
}

SubExpression_Ptr Parser::parse_sub_expression()
{
	SubExpression_Ptr result = nullptr;
	if (!accept(Op_minus))
		return result;

	result = make_shared<SubExpression>();
	result->Prefix = make_shared<OpMinus>();
	result->Prefix->LineNumber = lexems[offset - 1]->lineNumber;
	result->LineNumber = result->Prefix->LineNumber;

	result->Expression = parse_value_expression();
	if (result->Expression == nullptr)
		ErrorHandler::printError(lexems[offset]->lineNumber, "Expected expression");
	else
		result->Expression->parent = result;

	return result;
}

Expression_Ptr Parser::parse_value_expression()
{
	Expression_Ptr expression = parse_string_expression();
	if (expression != nullptr)
		return expression;

	expression = parse_char_expression();
	if (expression != nullptr)
		return expression;

	expression = parse_integer_expression();
	if (expression != nullptr)
		return expression;

	expression = parse_float_expression();
	if (expression != nullptr)
		return expression;

	expression = parse_true_expression();
	if (expression != nullptr)
		return expression;

	expression = parse_false_expression();
	if (expression != nullptr)
		return expression;

	expression = parse_grouped_expression();
	if (expression != nullptr)
		return expression;

	expression = parse_not_expression();
	if (expression != nullptr)
		return expression;

	expression = parse_sub_expression();
	if (expression != nullptr)
		return expression;

	expression = parse_function_call_expression();
	if (expression != nullptr)
		return expression;

	expression = parse_identifier_expression();
	if (expression != nullptr)
		return expression;

	return expression;
}

Expression_Ptr Parser::parse_multiplication_expression()
{
	Expression_Ptr left = nullptr;
	left = parse_value_expression();

	while (accept(Op_mult))
	{
		MultiplicationExpression_Ptr multiplication = make_shared<MultiplicationExpression>();
		if (left == nullptr)
		{
			ErrorHandler::printError(lexems[offset - 1]->lineNumber, "Missing left side");
		}
		else
		{
			multiplication->LineNumber = left->LineNumber;
			left->parent = multiplication;
		}

		multiplication->Left = left;
		multiplication->Operator = make_shared<OpMult>();

		multiplication->Right = parse_value_expression();
		if (multiplication->Right == nullptr)
			ErrorHandler::printError(multiplication->Operator->LineNumber, "Missing right side");
		else
			multiplication->Right->parent = multiplication;

		left = multiplication;
	}

	return left;
}

Expression_Ptr Parser::parse_add_expression()
{
	Expression_Ptr expression = nullptr;
	expression = parse_multiplication_expression();

	SubExpression_Ptr sub = nullptr;
	while (accept(Op_add) || (sub = parse_sub_expression()) != nullptr)
	{
		AddExpression_Ptr add = make_shared<AddExpression>();
		if (expression == nullptr)
			ErrorHandler::printError(lexems[offset - 1]->lineNumber, "Missing left side");
		else
		{
			add->LineNumber = expression->LineNumber;
			expression->parent = add;
		}

		add->Left = expression;
		add->Operator = make_shared<OpAdd>();
		if (sub == nullptr)
		{
			add->LineNumber = lexems[offset - 1]->lineNumber;

			add->Right = parse_multiplication_expression();
			if (add->Right == nullptr)
				ErrorHandler::printError(add->Operator->LineNumber, "Missing right side");
			else
				add->Right->parent = add;
		}
		else
		{
			add->Right = sub;
			add->Operator->LineNumber = sub->Prefix->LineNumber;
			sub->parent = add;
			sub = nullptr;
		}

		expression = add;
	}

	return expression;
}

OpComp_Ptr Parser::parse_comparison_operator()
{
	OpComp_Ptr result = nullptr;
	if (accept(Op_less))
		result = make_shared<OpLess>();
	else if (accept(Op_more))
		result = make_shared<OpMore>();
	else if (accept(Op_moreeq))
		result = make_shared<OpMoreEq>();
	else if (accept(Op_lesseq))
		result = make_shared<OpLessEq>();
	else if (accept(Op_eq))
		result = make_shared<OpEq>();
	else if (accept(Op_noteq))
		result = make_shared<OpNotEq>();

	if (result != nullptr)
		result->LineNumber = lexems[offset - 1]->lineNumber;

	return result;
}

Expression_Ptr Parser::parse_comparison_expression()
{
	Expression_Ptr expression = nullptr;
	expression = parse_add_expression();

	OpComp_Ptr comparison = nullptr;
	while (comparison = parse_comparison_operator())
	{
		CompExpression_Ptr comp = make_shared<CompExpression>();
		if (expression == nullptr)
			ErrorHandler::printError(lexems[offset - 1]->lineNumber, "Missing left side");
		else
		{
			comp->LineNumber = expression->LineNumber;
			expression->parent = comp;
		}

		comp->Left = expression;
		comp->Operator = comparison;

		comp->Right = parse_add_expression();
		if (comp->Right == nullptr)
			ErrorHandler::printError(comp->Operator->LineNumber, "Missing right side");
		else
			comp->Right->parent = comp;

		expression = comp;
	}

	return expression;
}

Expression_Ptr Parser::parse_or_expression()
{
	Expression_Ptr expression = nullptr;
	expression = parse_comparison_expression();

	while (accept(Op_or))
	{
		OrExpression_Ptr orExpression = make_shared<OrExpression>();
		if (expression == nullptr)
			ErrorHandler::printError(lexems[offset - 1]->lineNumber, "Missing left side");
		else
		{
			orExpression->LineNumber = expression->LineNumber;
			expression->parent = orExpression;
		}

		orExpression->Left = expression;
		orExpression->Operator = make_shared<OpOr>();

		orExpression->Right = parse_comparison_expression();
		if (orExpression->Right == nullptr)
			ErrorHandler::printError(orExpression->Operator->LineNumber, "Missing right side");
		else
			orExpression->Right->parent = orExpression;

		expression = orExpression;
	}
	return expression;
}

Expression_Ptr Parser::parse_and_expression()
{
	Expression_Ptr expression = nullptr;
	expression = parse_or_expression();

	while (accept(Op_and))
	{
		AndExpression_Ptr andExpression = make_shared<AndExpression>();
		if (expression == nullptr)
			ErrorHandler::printError(lexems[offset - 1]->lineNumber, "Missing left side");
		else
		{
			andExpression->LineNumber = expression->LineNumber;
			expression->parent = andExpression;
		}

		andExpression->Left = expression;
		andExpression->Operator = make_shared<OpAnd>();

		andExpression->Right = parse_or_expression();
		if (andExpression->Right == nullptr)
			ErrorHandler::printError(andExpression->Operator->LineNumber, "Missing right side");
		else
			andExpression->Right->parent = andExpression;

		expression = andExpression;
	}

	return expression;
}

Expression_Ptr Parser::parse_expression()
{
	return parse_and_expression();
}

AssignmentNode_Ptr Parser::parse_assignment()
{
	if (!accept(Op_assign))
		return nullptr;

	AssignmentNode_Ptr node = make_shared<AssignmentNode>();
	node->Operator = make_shared<OpAssign>();
	node->Operator->LineNumber = lexems[offset - 1]->lineNumber;
	node->LineNumber = node->Operator->LineNumber;

	node->Expression = parse_expression();
	if (node->Expression == nullptr)
		ErrorHandler::printError(lexems[offset]->lineNumber, "Incorect right side expression");
	else
		node->Expression->parent = node;

	return node;
}

DeclarationStatement_Ptr Parser::parse_declaration()
{
	DeclarationStatement_Ptr result = nullptr;

	TypeKeyword_Ptr keyword = parse_type();
	if (keyword == nullptr)
		return result;

	result = make_shared<DeclarationStatement>();
	result->Size = 0;
	result->Type = keyword;
	result->LineNumber = result->Type->LineNumber;

	result->Variable = parse_identifier();
	if (result->Variable == nullptr)
		ErrorHandler::printError(lexems[offset]->lineNumber, "Expected variable declaration identifier");
	else
		result->Variable->parent = result;

	if (accept(SQBracket_open))
	{
		result->Type->IsPointer = true;
		auto size = parse_integer_expression();
		if (size == nullptr)
		{
			ErrorHandler::printError(lexems[offset - 1]->lineNumber, "expected array size");
		}
		else
		{
			result->Size = size;

		}
		if(!accept(SQBracket_close))
			ErrorHandler::printError(lexems[offset - 1]->lineNumber, "expected matching closing bracket");
	}

	result->Assignment = parse_assignment();
	if (result->Assignment != nullptr)
		result->Assignment->parent = result;
	result->EndToken = parse_statement_end_token();
	return result;
}

AssignmentStatement_Ptr Parser::parse_assignment_statement()
{
	AssignmentStatement_Ptr result = nullptr;
	int startOffset = offset;
	IdentifierExpression_Ptr name = parse_identifier_expression();
	if (name == nullptr)
		return result;

	AssignmentNode_Ptr assignment = parse_assignment();
	if (assignment == nullptr)
	{
		offset = startOffset;
		return result;
	}

	result = make_shared<AssignmentStatement>();
	result->Variable = name;
	result->Variable->parent = result;
	result->Assignment = assignment;
	result->Assignment->parent = result;
	result->LineNumber = result->Variable->LineNumber;

	result->EndToken = parse_statement_end_token();

	return result;
}

EmptyStatement_Ptr Parser::parse_continue_or_break_statement()
{
	EmptyStatement_Ptr result = nullptr;
	if (accept(Continue_keyword))
	{
		ContinueStatement_Ptr statement = make_shared<ContinueStatement>();
		statement->Keyword = make_shared<ContinueKeyword>();
		statement->Keyword->LineNumber = lexems[offset - 1]->lineNumber;
		statement->LineNumber = statement->Keyword->LineNumber;
		result = statement;
	}
	else if (accept(Break_keyword))
	{
		BreakStatement_Ptr statement = make_shared<BreakStatement>();
		statement->Keyword = make_shared<BreakKeyword>();
		statement->Keyword->LineNumber = lexems[offset - 1]->lineNumber;
		statement->LineNumber = statement->Keyword->LineNumber;
		result = statement;
	}
	else
	{
		return result;
	}

	result->EndToken = parse_statement_end_token();
	return result;
}

ReturnStatement_Ptr Parser::parse_return_statement()
{
	ReturnStatement_Ptr result = nullptr;
	if (!accept(Return_keyword))
		return result;

	result = make_shared<ReturnStatement>();
	result->Keyword = make_shared<ReturnKeyword>();
	result->Keyword->LineNumber = lexems[offset - 1]->lineNumber;
	result->LineNumber = result->Keyword->LineNumber;

	if (accept(Statement_end))
	{
		offset--;
		result->EndToken = parse_statement_end_token();
		return result;
	}

	result->ReturnValue = parse_expression();
	if (result->ReturnValue != nullptr)
		result->ReturnValue->parent = result;
	result->EndToken = parse_statement_end_token();
	return result;
}

ScanNode_Ptr Parser::parse_scan()
{
	ScanNode_Ptr result = nullptr;
	if (!accept(Op_scan))
		return result;

	result = make_shared<ScanNode>();
	result->Operator = make_shared<OpScan>();
	result->Operator->LineNumber = lexems[offset - 1]->lineNumber;
	result->LineNumber = result->Operator->LineNumber;

	result->Identifier = parse_identifier_expression();
	if (result->Identifier == nullptr)
		ErrorHandler::printError(lexems[offset]->lineNumber, "Missing variable identifier");
	else
		result->Identifier->parent = result;

	return result;
}

ScanStatement_Ptr Parser::parse_scan_statement()
{
	ScanStatement_Ptr result = nullptr;
	if (!accept(Scan_keyword))
		return result;

	result = make_shared<ScanStatement>();
	result->Keyword = make_shared<ScanKeyword>();
	result->Keyword->LineNumber = lexems[offset - 1]->lineNumber;
	result->LineNumber = result->Keyword->LineNumber;

	while (true)
	{
		ScanNode_Ptr scan = parse_scan();
		if (scan == nullptr)
			break;

		scan->parent = result;
		result->Scans.push_back(scan);
	}

	result->EndToken = parse_statement_end_token();
	if (result->EndToken == nullptr)
		ErrorHandler::printError(lexems[offset]->lineNumber, "Expected statement end ; token");

	return result;
}

PrintNode_Ptr Parser::parse_print()
{
	PrintNode_Ptr result = nullptr;
	if (!accept(Op_print))
		return result;

	result = make_shared<PrintNode>();
	result->Operator = make_shared<OpPrint>();
	result->Operator->LineNumber = lexems[offset - 1]->lineNumber;
	result->LineNumber = result->Operator->LineNumber;

	result->Expression = parse_expression();
	if (result->Expression == nullptr)
		ErrorHandler::printError(lexems[offset]->lineNumber, "Missing expression");
	else
		result->Expression->parent = result;

	return result;
}

PrintStatement_Ptr Parser::parse_print_statement()
{
	PrintStatement_Ptr result = nullptr;
	if (!accept(Print_keyword))
		return result;

	result = make_shared<PrintStatement>();
	result->Keyword = make_shared<PrintKeyword>();
	result->Keyword->LineNumber = lexems[offset - 1]->lineNumber;
	result->LineNumber = result->Keyword->LineNumber;

	while (true)
	{
		PrintNode_Ptr print = parse_print();
		if (print == nullptr)
			break;

		print->parent = result;
		result->Prints.push_back(print);
	}

	result->EndToken = parse_statement_end_token();
	if (result->EndToken == nullptr)
		ErrorHandler::printError(lexems[offset]->lineNumber, "Expected statement end ; token");

	return result;
}

Param_Ptr Parser::parse_call_parameter()
{
	Param_Ptr result = make_shared<Param>();
	if (accept(Seperator))
	{
		result->Seperator = make_shared<SeperatorToken>();
		result->Seperator->LineNumber = lexems[offset - 1]->lineNumber;
		result->LineNumber = result->Seperator->LineNumber;
	}

	result->Expression = parse_expression();
	if (result->Expression == nullptr && result->Seperator == nullptr)
		return nullptr;

	if (result->Expression == nullptr)
		ErrorHandler::printError(result->Seperator->LineNumber, "Unexpected extra , token");
	else
		result->Expression->parent = result;

	if (result->LineNumber == 0)
		result->LineNumber = result->Expression->LineNumber;

	return result;
}

FunctionParams_Ptr Parser::parse_call_parameters()
{
	FunctionParams_Ptr result = nullptr;

	if (!accept(Paran_open))
		return result;

	result = make_shared<FunctionParams>();
	result->ListStartToken = make_shared<OpenParan>();
	result->ListStartToken->LineNumber = lexems[offset - 1]->lineNumber;
	result->LineNumber = result->ListStartToken->LineNumber;

	while (true)
	{
		Param_Ptr parameter = parse_call_parameter();
		if (parameter == nullptr)
			break;

		parameter->parent = result;
		result->Parameters.push_back(parameter);
		if (result->Parameters.size() == 1)
		{
			if (parameter->Seperator != nullptr)
				ErrorHandler::printError(parameter->Seperator->LineNumber, "Unexpected seperator token");
		}
		else
		{
			if (parameter->Seperator == NULL)
				ErrorHandler::printError(parameter->LineNumber, "Expected separator token");
		}
	}

	if (accept(Paran_close))
	{
		result->ListEndToken = make_shared<CloseParan>();
		result->ListEndToken->LineNumber = lexems[offset - 1]->lineNumber;
	}
	else
	{
		ErrorHandler::printError(result->LineNumber, "Expected ), but found something else");
	}

	return result;
}

FunctionCallExpression_Ptr Parser::parse_function_call_expression()
{
	int startoffset = offset;
	FunctionCallExpression_Ptr result = nullptr;

	IdentifierExpression_Ptr ident = parse_identifier_expression();
	if (ident == nullptr)
		return result;

	FunctionParams_Ptr parameters = parse_call_parameters();
	if (parameters == nullptr)
	{
		offset = startoffset;
		return result;
	}

	result = make_shared<FunctionCallExpression>();
	result->Name = ident;
	result->Name->parent = result;
	result->Parameters = parameters;
	result->Parameters->parent = result;
	result->LineNumber = result->Name->LineNumber;

	return result;
}

FunctionCallStatement_Ptr Parser::parse_function_call_statement()
{
	FunctionCallStatement_Ptr result = nullptr;
	FunctionCallExpression_Ptr functionCall = parse_function_call_expression();
	if (functionCall == nullptr)
		return result;

	result = make_shared<FunctionCallStatement>();
	result->FunctionCall = functionCall;
	functionCall->parent = result;
	result->LineNumber = result->FunctionCall->LineNumber;

	result->EndToken = parse_statement_end_token();
	if(result->EndToken == nullptr)
		ErrorHandler::printError(lexems[offset]->lineNumber, "Expected statement end ; token");

	return result;
}

Body_Ptr Parser::parse_body()
{
	Body_Ptr result = nullptr;

	result = parse_statement();
	if (result != nullptr)
		return result;

	return parse_block();
}

WhileStatement_Ptr Parser::parse_while_statement()
{
	WhileStatement_Ptr result = nullptr;
	if (!accept(While_keyword))
		return result;

	result = make_shared<WhileStatement>();
	result->Keyword = make_shared<WhileKeyword>();
	result->Keyword->LineNumber = lexems[offset - 1]->lineNumber;
	result->LineNumber = result->Keyword->LineNumber;

	result->Condition = parse_expression();
	if (result->Condition == nullptr)
		ErrorHandler::printError(lexems[offset - 1]->lineNumber, "Expected bool expression");
	else
		result->Condition->parent = result;

	result->Body = parse_body();
	if (result->Body == nullptr)
	{
		if (result->Condition != nullptr)
			ErrorHandler::printError(lexems[offset - 1]->lineNumber, "Expected body");
	}
	else
	{
		result->Body->parent = result;
	}

	return result;
}

ElseNode_Ptr Parser::parse_else_node()
{
	ElseNode_Ptr result = nullptr;
	if (!accept(Else_keyword))
		return result;

	ElseKeyword_Ptr elseKeyword = make_shared<ElseKeyword>();
	elseKeyword->LineNumber = lexems[offset - 1]->lineNumber;

	IfStatement_Ptr ifNode = parse_if_statement();
	if (ifNode != nullptr)
	{
		ElseIf_Ptr elseIf= make_shared<ElseIf>();
		result = elseIf;

		elseIf->If = ifNode;
		elseIf->If->parent = elseIf;
	}
	else
	{

		Else_Ptr elseNode = make_shared<Else>();
		result = elseNode;

		elseNode->ElseBody = parse_body();
		if (elseNode->ElseBody == nullptr)
			ErrorHandler::printError(lexems[offset - 1]->lineNumber, "expected else body");
		else
			elseNode->ElseBody->parent = elseNode;
	}

	result->ElseKeyword = elseKeyword;
	result->LineNumber = result->ElseKeyword->LineNumber;
	return result;
}

IfStatement_Ptr Parser::parse_if_statement()
{
	IfStatement_Ptr result = nullptr;
	if (!accept(If_keyword))
		return result;

	result = make_shared<IfStatement>();
	result->IfKeyword = make_shared<IfKeyword>();
	result->IfKeyword->LineNumber = lexems[offset - 1]->lineNumber;
	result->LineNumber = result->IfKeyword->LineNumber;

	result->Condition = parse_expression();
	if (result->Condition == nullptr)
		ErrorHandler::printError(lexems[offset - 1]->lineNumber, "Expected bool expression");
	else
		result->Condition->parent = result;

	result->Body = parse_body();
	if (result->Body == nullptr)
	{
		if (result->Condition != nullptr)
			ErrorHandler::printError(lexems[offset - 1]->lineNumber, "Expected body");
	}
	else
	{
		result->Body->parent = result;
	}

	result->Else = parse_else_node();
	if (result->Else != nullptr)
		result->Else->parent = result;

	return result;
}

Statement_Ptr Parser::parse_statement()
{
	if (accept(Statement_end))
	{
		EmptyStatement_Ptr statement = make_shared<EmptyStatement>();
		statement->EndToken = make_shared<StatementEnd>();
		statement->EndToken->LineNumber = lexems[offset - 1]->lineNumber;
		statement->LineNumber = statement->EndToken->LineNumber;
		return statement;
	}
	
	Statement_Ptr statement = parse_continue_or_break_statement();
	if (statement != nullptr)
		return statement;

	statement = parse_return_statement();
	if (statement != nullptr)
		return statement;

	statement = parse_scan_statement();
	if (statement != nullptr)
		return statement;

	statement = parse_print_statement();
	if (statement != nullptr)
		return statement;

	statement = parse_declaration();
	if (statement != nullptr)
		return statement;

	statement = parse_assignment_statement();
	if (statement != nullptr)
		return statement;

	statement = parse_function_call_statement();
	if (statement != nullptr)
		return statement;

	statement = parse_while_statement();
	if (statement != nullptr)
		return statement;

	statement = parse_if_statement();
	if (statement != nullptr)
		return statement;

	return nullptr;
}

BlockBody_Ptr Parser::parse_block()
{
	if (!accept(Block_start))
		return nullptr;
	
	BlockBody_Ptr result = make_shared<BlockBody>();
	result->Start = make_shared<BlockStart>();
	result->Start->LineNumber = lexems[offset - 1]->lineNumber;
	result->LineNumber = result->Start->LineNumber;

	while (!accept(Block_end))
	{
		if (accept(EndOfFile))
		{
			offset--;
			ErrorHandler::printError(lexems[offset]->lineNumber, "Unmatched Block start");
			return result;
		}
		Statement_Ptr statement = parse_statement();
		if (statement == nullptr)
		{
			ErrorHandler::printError(lexems[offset]->lineNumber, "Unexpected identifier");
			offset++;
			continue;
		}
		statement->parent = result;
		result->Statements.push_back(statement);
	}
	result->End = make_shared<BlockEnd>();

	return result;
}

Function_Ptr Parser::build_function()
{
	int startOffset = offset;
	Function_Ptr function = make_shared<Function>();

	function->ReturnType = parse_return_type();
	if (accept(SQBracket_open))
	{
		function->ReturnType->IsPointer = true;
		if (!accept(SQBracket_close))
			ErrorHandler::printError(lexems[offset - 1]->lineNumber, "expected matching closing bracket");
	}
	bool hasReturnType = function->ReturnType != nullptr;

	function->Name = parse_identifier();
	bool hasName = function->Name != nullptr;
	if (!hasReturnType && !hasName)
	{
		offset = startOffset + 1;
		ErrorHandler::printError(lexems[offset-1]->lineNumber, "Expected function declaration");
		return nullptr;
	}

	function->Parameters = parse_parameters_list();
	bool hasParameters = function->Parameters != nullptr;
	if (!hasParameters && (!hasReturnType || !hasName))
	{
		offset = startOffset + 1;
		ErrorHandler::printError(lexems[offset-1]->lineNumber, "Expected function declaration");
		return nullptr;
	}

	function->Body = parse_block();
	bool hasBody = function->Body != nullptr;
	if (hasBody)
		function->Body->parent = function;
	if(!hasBody && (!hasReturnType || !hasName || !hasParameters))
	{
		offset = startOffset + 1;
		ErrorHandler::printError(lexems[offset-1]->lineNumber, "Expected function declaration, but found some trash");
		return nullptr;
	}

	if (!hasReturnType)
		ErrorHandler::printError(function->Name->LineNumber, "Missing function return type");

	if(!hasName)
		ErrorHandler::printError(function->ReturnType->LineNumber, "Missing function name");

	if (!hasParameters)
		ErrorHandler::printError(function->Name->LineNumber, "Missing parameters");

	if (!hasBody)
		ErrorHandler::printError(lexems[offset]->lineNumber, "Missing function body");

	if (hasReturnType)
		function->LineNumber = function->ReturnType->LineNumber;
	else
		function->LineNumber = function->Name->LineNumber;
		

	return function;
}

SyntaxTree_Ptr Parser::build_syntax_tree(std::vector<Lexem_ptr> _lexems)
{
	lexems = _lexems;
	SyntaxTree_Ptr tree = make_shared<SyntaxTree>();
	while (lexems[offset]->type != EndOfFile)
	{
		Function_Ptr func = build_function();
		if (func != nullptr)
			tree->functions.push_back(func);
	}
	return tree;
}

void Parser::print_tree(SyntaxTree_Ptr tree)
{
	for (Function_Ptr node : tree->functions)
	{
		node->print(0);
	}
}
