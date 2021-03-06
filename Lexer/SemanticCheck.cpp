#include "pch.h"
#include "SemanticCheck.h"

using namespace std;

std::string ExpressionTypeStrings[]
{
	"Unknown",
	"Bool",
	"Bool[]",
	"Integer",
	"Int[]",
	"Float",
	"Float[]",
	"String",
	"Char",
	"Char[]",
	"Void"
};

std::vector<Function_Ptr> Scope::RezervedFunctions;

void RegisterDiferentTypesError(ExpressionType leftType, ExpressionType rightType, int lineNumber)
{
	string error = "Both sides of operator should have the same type. Left side is: ";
	error.append(ExpressionTypeStrings[leftType]);
	error.append(" Right side is: ");
	error.append(ExpressionTypeStrings[rightType]);
	ErrorHandler::printError(lineNumber, error.c_str());
}

void RegisterIncorectTypeError(ExpressionType type, int lineNumber)
{
	string error = "Cannot perform operation with ";
	error.append(ExpressionTypeStrings[type]);
	error.append(" type");
	ErrorHandler::printError(lineNumber, error.c_str());
}

ExpressionType GetVariableType(Node* variable, int lineNumber)
{
	if (variable == nullptr)
		return Unknown;

	auto variableDeclaration = (dynamic_cast<VariableDeclaration *>(variable));
	if (variableDeclaration == nullptr)
	{
		auto functionDeclaration = dynamic_cast<Function *>(variable);
		string error = "Function ";
		error.append(functionDeclaration->Name->Value);
		error.append(" cannot be used as variable");
		ErrorHandler::printError(lineNumber, error.c_str());
		return Unknown;
	}

	return variableDeclaration->Type->getType();
}

void Scope::Add(std::string name, Node* node)
{
	auto currentScope = this;
	while (currentScope != nullptr)
	{
		if (variables.find(name) != variables.end())
		{
			string errorMessage = "Variable ";
			errorMessage.append(name.c_str());
			errorMessage.append(" is already defined in this scope");
			ErrorHandler::printError(node->LineNumber, errorMessage.c_str());

			return;
		}

		currentScope = &(*currentScope->parentScope);
	}

	variables.insert(std::pair<std::string, Node*>(name, node));
}

Node* Scope::Find(std::string name, int lineNumber)
{
	auto currentScope = this;
	auto previousScope = this;
	while (currentScope != nullptr)
	{
		if (currentScope->variables.find(name) != currentScope->variables.end())
		{
			return currentScope->variables[name];
		}
		previousScope = currentScope;
		currentScope = &(*currentScope->parentScope);
	}

	for (auto function : RezervedFunctions)
	{
		if (function->Name->Value == name)
			return &(*function);
	}

	if (name == "rand")
	{
		auto function = make_shared<Function>();
		RezervedFunctions.push_back(function);
		function->Parameters = make_shared<FunctionParamsDef>();
		auto param = make_shared<ParamDef>();
		function->Parameters->Parameters.push_back(param);
		param->Type = make_shared<IntegerKeyword>();
		function->HasReturn = true;
		function->Name = make_shared<Identifier>();
		function->Name->Value = name;
		function->ReturnType = make_shared<IntegerKeyword>();
		return &(*function);
	}

	if (name == "sleep")
	{
		auto function = make_shared<Function>();
		RezervedFunctions.push_back(function);
		function->Parameters = make_shared<FunctionParamsDef>();
		auto param = make_shared<ParamDef>();
		function->Parameters->Parameters.push_back(param);
		param->Type = make_shared<IntegerKeyword>();
		function->HasReturn = true;
		function->Name = make_shared<Identifier>();
		function->Name->Value = name;
		function->ReturnType = make_shared<VoidKeyword>();
		return &(*function);
	}

	if (name == "setscreen")
	{
		auto function = make_shared<Function>();
		RezervedFunctions.push_back(function);
		function->Parameters = make_shared<FunctionParamsDef>();
		auto param = make_shared<ParamDef>();
		function->Parameters->Parameters.push_back(param);
		param->Type = make_shared<IntegerKeyword>();
		param = make_shared<ParamDef>();
		function->Parameters->Parameters.push_back(param);
		param->Type = make_shared<IntegerKeyword>();
		function->HasReturn = true;
		function->Name = make_shared<Identifier>();
		function->Name->Value = name;
		function->ReturnType = make_shared<VoidKeyword>();
		return &(*function);
	}

	string errorMessage = "Variable ";
	errorMessage.append(name.c_str());
	errorMessage.append(" is not defined in this scope");
	ErrorHandler::printError(lineNumber, errorMessage.c_str());
	return nullptr;
}

void Node::resolveNames(Scope_Ptr scope)
{
	throw std::exception("Node::resolveNames should never be called.");
}

void SyntaxTree::resolveNames(Scope_Ptr scope)
{
	Function_Ptr main = nullptr;
	for (Function_Ptr func : functions)
	{
		if (func->Name != nullptr)
		{
			scope->Add(func->Name->Value, &(*func));
			if (strcmp(func->Name->Value.c_str(), "main") == 0)
			{
				func->IsMain = true;
				main = func;
			}
		}
	}
	if (main == nullptr)
	{
		ErrorHandler::printError(0, "Main function does not exist");
		return;
	}
	if (main->ReturnType != nullptr && main->ReturnType->getType() != VoidType)
	{
		ErrorHandler::printError(main->LineNumber, "Main function should be void type function");
		return;
	}
	if (main->Parameters != nullptr && main->Parameters->Parameters.size() != 0)
	{
		ErrorHandler::printError(main->LineNumber, "Main function should be function without parameters");
		return;
	}
	for (Function_Ptr func : functions)
	{
		func->resolveNames(scope);
	}

	for (auto rezervedFunction : scope->RezervedFunctions)
	{
		rezervedFunctions.push_back(rezervedFunction);
	}
}

void Function::resolveNames(Scope_Ptr parentScope)
{
	Scope_Ptr functionScope = make_shared<Scope>();
	functionScope->parentScope = parentScope;

	if(Parameters != nullptr)
	for (auto param : Parameters->Parameters)
	{
		if (param->Name == nullptr)
			continue;

		functionScope->Add(param->Name->Value, &(*param));
	}

	if (Body != nullptr)
		Body->resolveNames(functionScope);

	if (!HasReturn && ReturnType != nullptr && ReturnType->getType() != VoidType)
		ErrorHandler::printError(LineNumber, "Function missing return statement");
}

void BlockBody::resolveNames(Scope_Ptr curentScope)
{
	for (auto statement : Statements)
		statement->resolveNames(curentScope);
}

void AssignmentStatement::resolveNames(Scope_Ptr currentScope)
{
	ExpressionType expressionType = Unknown;
	if (Variable != nullptr)
	{
		ExpressionType variableType = Variable->resolveType(currentScope);
		if (variableType == Unknown)
			return;
		expressionType = Variable->Type;
		if (expressionType == CharType && variableType == StringType)
		{
			ErrorHandler::printError(LineNumber, "cannot modify string type variable");
			return;
		}
	}

	if (Assignment != nullptr)
	{
		Assignment->CheckIsType(expressionType, currentScope);
	}
}

void DeclarationStatement::resolveNames(Scope_Ptr currentScope)
{
	ExpressionType variableType;
	if (Variable != nullptr)
	{
		currentScope->Add(Variable->Value, this);
		variableType = Type->getType();
		if (variableType == Unknown)
			return;
	}

	if (Assignment != nullptr)
		Assignment->CheckIsType(variableType, currentScope);
}

void AssignmentNode::CheckIsType(ExpressionType variableType, Scope_Ptr currentScope)
{
	if (Expression != nullptr)
	{
		Expression->resolveType(currentScope);
		auto expressionType = Expression->Type;
		if (variableType != expressionType && expressionType != Unknown)
		{
			string error = "Cannot assign ";
			error.append(ExpressionTypeStrings[expressionType]);
			error.append(" type expression to ");
			error.append(ExpressionTypeStrings[variableType]);
			error.append(" type variable");
			ErrorHandler::printError(Expression->LineNumber, error.c_str());
		}
	}
}

void FunctionCallStatement::resolveNames(Scope_Ptr curentScope)
{
	FunctionCall->resolveType(curentScope);
}

void ReturnStatement::resolveNames(Scope_Ptr currentScope)
{
	Function* parentFunction = findFunctionParent();
	if (parentFunction != nullptr)
	{
		parentFunction->HasReturn = true;
		auto returnType = parentFunction->ReturnType->getType();

		if (returnType == VoidType)
		{
			if (ReturnValue != nullptr)
			{
				ErrorHandler::printError(ReturnValue->LineNumber, "void type function cannot return value");
				return;
			}
			return;
		}
		else
		{
			if (ReturnValue == nullptr)
			{
				ErrorHandler::printError(Keyword->LineNumber, "function should return value");
				return;
			}
		}
		ReturnValue->resolveType(currentScope);
		auto actualType = ReturnValue->Type;
		if (returnType != Unknown && actualType != Unknown && actualType != returnType)
		{
			string error = "Cannot return ";
			error.append(ExpressionTypeStrings[actualType]);
			error.append(" type expression in function with return type ");
			error.append(ExpressionTypeStrings[returnType]);
			ErrorHandler::printError(ReturnValue->LineNumber, error.c_str());
		}
	}
	else
	{
		ErrorHandler::printError(LineNumber, "Return statement should be inside function.");
	}
}

void PrintStatement::resolveNames(Scope_Ptr currentScope)
{
	for (auto print : Prints)
	{
		if (print->Expression != nullptr)
			print->Expression->resolveType(currentScope);
	}
}

void ScanStatement::resolveNames(Scope_Ptr currentScope)
{
	for (auto scan : Scans)
	{
		if (scan->Identifier != nullptr)
			currentScope->Find(scan->Identifier->Value, scan->Identifier->LineNumber);
	}
}

void WhileStatement::resolveNames(Scope_Ptr parentScope)
{
	if (Condition != nullptr)
	{
		auto conditionType = Condition->resolveType(parentScope);
		if (conditionType != BoolType && conditionType != Unknown)
			ErrorHandler::printError(Condition->LineNumber, "Condition should be boolean type");
	}

	Scope_Ptr whileScope = make_shared<Scope>();
	whileScope->parentScope = parentScope;

	if (Body != nullptr)
		Body->resolveNames(whileScope);
}

void IfStatement::resolveNames(Scope_Ptr parentScope)
{
	if (Condition != nullptr)
	{
		auto conditionType = Condition->resolveType(parentScope);
		if (conditionType != BoolType && conditionType != Unknown)
			ErrorHandler::printError(Condition->LineNumber, "Condition should be boolean type");
	}

	Scope_Ptr ifScope = make_shared<Scope>();
	ifScope->parentScope = parentScope;

	if (Body != nullptr)
		Body->resolveNames(ifScope);

	if (Else != nullptr)
		Else->resolveNames(parentScope);
}

void ElseIf::resolveNames(Scope_Ptr parentScope)
{
	if (If != nullptr)
		If->resolveNames(parentScope);
}

void Else::resolveNames(Scope_Ptr parentScope)
{
	Scope_Ptr elseScope = make_shared<Scope>();
	elseScope->parentScope = parentScope;

	if (ElseBody != nullptr)
		ElseBody->resolveNames(elseScope);
}

void FunctionCallExpression::resolveNames(Scope_Ptr currentScope)
{
}

void GroupedExpression::resolveNames(Scope_Ptr currentScope)
{
	if (Expression != nullptr)
		Expression->resolveNames(currentScope);
}

void BreakStatement::resolveNames(Scope_Ptr currentScope)
{
	if (findCycleParent() == nullptr)
	{
		ErrorHandler::printError(LineNumber, "Break statement should be inside a cycle");
	}
}

void ContinueStatement::resolveNames(Scope_Ptr currentScope)
{
	if (findCycleParent() == nullptr)
	{
		ErrorHandler::printError(LineNumber, "Break statement should be inside a cycle");
	}
}

void UnaryExpression::resolveNames(Scope_Ptr currentScope)
{
	if (Expression != nullptr)
		Expression->resolveNames(currentScope);
}

ExpressionType Expression::resolveType(Scope_Ptr currentScope)
{
	throw std::exception("Expression::getType should never be called.");
}

ExpressionType MultiplicationExpression::resolveType(Scope_Ptr currentScope)
{
	Left->resolveType(currentScope);
	ExpressionType leftType = Left->Type;
	Right->resolveType(currentScope);
	ExpressionType rightType = Right->Type;
	Type = leftType;

	bool isLeftTypeNotNumeric = (leftType != IntType && leftType != FloatType && leftType != Unknown);
	bool isRightTypeNotNumeric = (rightType != IntType && rightType != FloatType && rightType != Unknown);
	if (isLeftTypeNotNumeric)
	{
		RegisterIncorectTypeError(leftType, Left->LineNumber);
	}
	if (isRightTypeNotNumeric)
	{
		RegisterIncorectTypeError(rightType, Right->LineNumber);
	}
	if (isLeftTypeNotNumeric || isRightTypeNotNumeric)
		return Unknown;

	if (leftType != rightType && (leftType != Unknown || rightType != Unknown))
	{
		RegisterDiferentTypesError(leftType, rightType, Operator->LineNumber);
		return Unknown;
	}

	return leftType == Unknown ? rightType : leftType;
}

ExpressionType AddExpression::resolveType(Scope_Ptr currentScope)
{
	Left->resolveType(currentScope);
	ExpressionType leftType = Left->Type;
	Right->resolveType(currentScope);
	ExpressionType rightType = Right->Type;
	Type = leftType;

	bool isLeftTypeNotNumeric = (leftType != IntType && leftType != FloatType && leftType != Unknown);
	bool isRightTypeNotNumeric = (rightType != IntType && rightType != FloatType && rightType != Unknown);
	if (isLeftTypeNotNumeric)
	{
		RegisterIncorectTypeError(leftType, Left->LineNumber);
	}
	if (isRightTypeNotNumeric)
	{
		RegisterIncorectTypeError(rightType, Right->LineNumber);
	}
	if (isLeftTypeNotNumeric || isRightTypeNotNumeric)
		return Unknown;

	if (leftType != rightType && (leftType != Unknown || rightType != Unknown))
	{
		RegisterDiferentTypesError(leftType, rightType, Operator->LineNumber);
		return Unknown;
	}

	return leftType == Unknown ? rightType : leftType;
}

ExpressionType CompExpression::resolveType(Scope_Ptr currentScope)
{
	Left->resolveType(currentScope);
	ExpressionType leftType = Left->Type;
	Right->resolveType(currentScope);
	ExpressionType rightType = Right->Type;
	Type = BoolType;

	if (leftType != rightType && (leftType != Unknown || rightType != Unknown))
	{
		RegisterDiferentTypesError(leftType, rightType, Operator->LineNumber);
		return Unknown;
	}
	if ((leftType != Unknown || rightType != Unknown) && leftType != IntType && leftType != FloatType)
	{
		if (typeid(*Operator) != typeid(OpEq) && typeid(*Operator) != typeid(OpNotEq))
		{
			RegisterIncorectTypeError(leftType, Operator->LineNumber);
			return Unknown;
		}
	}

	return BoolType;
}

ExpressionType OrExpression::resolveType(Scope_Ptr currentScope)
{
	Left->resolveType(currentScope);
	ExpressionType leftType = Left->Type;
	Right->resolveType(currentScope);
	ExpressionType rightType = Right->Type;
	Type = leftType;

	bool isLeftBool = (leftType != BoolType && leftType != Unknown);
	bool isRightBool = (rightType != BoolType && rightType != Unknown);
	if (isLeftBool)
	{
		RegisterIncorectTypeError(leftType, Left->LineNumber);
	}
	if (isRightBool)
	{
		RegisterIncorectTypeError(rightType, Right->LineNumber);
	}

	if (!isLeftBool || !isRightBool)
		return Unknown;

	if (leftType != rightType && (leftType != Unknown || rightType != Unknown))
	{
		RegisterDiferentTypesError(leftType, rightType, Operator->LineNumber);
		return Unknown;
	}

	return BoolType;
}

ExpressionType AndExpression::resolveType(Scope_Ptr currentScope)
{
	Left->resolveType(currentScope);
	ExpressionType leftType = Left->Type;
	Right->resolveType(currentScope);
	ExpressionType rightType = Right->Type;
	Type = leftType;

	bool isLeftBool = (leftType != BoolType && leftType != Unknown);
	bool isRightBool = (rightType != BoolType && rightType != Unknown);
	if (isLeftBool)
	{
		RegisterIncorectTypeError(leftType, Left->LineNumber);
	}
	if (isRightBool)
	{
		RegisterIncorectTypeError(rightType, Right->LineNumber);
	}

	if (!isLeftBool || !isRightBool)
		return Unknown;

	if (leftType != rightType && (leftType != Unknown || rightType != Unknown))
	{
		RegisterDiferentTypesError(leftType, rightType, Operator->LineNumber);
		return Unknown;
	}

	return BoolType;
}

ExpressionType IdentifierExpression::resolveType(Scope_Ptr currentScope)
{
	Type = GetVariableType(currentScope->Find(Value, LineNumber), LineNumber);
	return Type;
}

ExpressionType MemberAccessExpression::resolveType(Scope_Ptr currentScope)
{
	ExpressionType variableType = GetVariableType(currentScope->Find(Value, LineNumber), LineNumber);
	if (variableType != StringType && variableType != CharPointer && variableType != IntPointer && variableType != FloatPointer && variableType != BoolPointer)
	{
		ErrorHandler::printError(LineNumber, "Cannot access member for non array type variable");
		return Unknown;
	}
	auto accesorType = AccessPosition->resolveType(currentScope);
	if (accesorType != IntType)
	{
		ErrorHandler::printError(LineNumber, "array index should Integer type expression");
		return Unknown;
	}

	switch (variableType)
	{
	case StringType:
	case CharPointer:
		Type = CharType;
		break;
	case IntPointer:
		Type = IntType;
		break;
	case FloatPointer:
		Type = FloatType;
		break;
	case BoolPointer:
		Type = BoolType;
		break;
	default:
		Type = Unknown;
		break;
	}

	return variableType;
}

ExpressionType FunctionCallExpression::resolveType(Scope_Ptr currentScope)
{
	if (Name == nullptr)
		return Unknown;

	auto functionVariable = currentScope->Find(Name->Value, LineNumber);

	if (functionVariable == nullptr)
		return Unknown;

	Function * functionDefinition = (dynamic_cast<Function *>(&(*functionVariable)));

	if (functionDefinition == nullptr)
	{
		string error = "Variable ";
		error.append(Name->Value);
		error.append(" used as function");
		ErrorHandler::printError(LineNumber, error.c_str());
		return Unknown;
	}

	if (Parameters != nullptr && functionDefinition->Parameters != nullptr)
	{
		auto parameters = Parameters->Parameters;
		auto definitionParameters = functionDefinition->Parameters->Parameters;
		size_t parametersCount = definitionParameters.size();
		if (parametersCount != parameters.size())
		{
			ErrorHandler::printError(Parameters->LineNumber, "missmatched parameters count");
		}
		else
		{
			for (int i = 0; i < parametersCount; i++)
			{
				if (definitionParameters[i]->Type != nullptr && parameters[i]->Expression != nullptr)
				{
					ExpressionType definitionType = definitionParameters[i]->Type->getType();
					parameters[i]->Expression->resolveType(currentScope);
					ExpressionType type = parameters[i]->Expression->Type;
					if (type != Unknown && definitionType != type)
					{
						if ((definitionType != CharPointer && definitionType != StringType) || (type != CharPointer && type != StringType))
						{
							string error = "Cannot use ";
							error.append(ExpressionTypeStrings[type]);
							error.append(" type expression as ");
							error.append(ExpressionTypeStrings[definitionType]);
							error.append(" type parameter");
							ErrorHandler::printError(parameters[i]->Expression->LineNumber, error.c_str());
						}
					}
				}
			}
		}
	}

	Type = functionDefinition->ReturnType->getType();
	return Type;
}

ExpressionType StringExpression::resolveType(Scope_Ptr currentScope)
{
	Type = StringType;
	return Type;
}

ExpressionType CharExpression::resolveType(Scope_Ptr currentScope)
{
	Type = CharType;
	return Type;
}

ExpressionType IntegerExpression::resolveType(Scope_Ptr currentScope)
{
	Type = IntType;
	return Type;
}

ExpressionType FloatExpression::resolveType(Scope_Ptr currentScope)
{
	Type = FloatType;
	return Type;
}

ExpressionType BoolExpression::resolveType(Scope_Ptr currentScope)
{
	Type = BoolType;
	return Type;
}

ExpressionType GroupedExpression::resolveType(Scope_Ptr currentScope)
{
	if (Expression != nullptr)
	{
		Type = Expression->resolveType(currentScope);
		return Type;
	}

	return Unknown;
}

ExpressionType SubExpression::resolveType(Scope_Ptr currentScope)
{
	Expression->resolveType(currentScope);
	auto expressionType = Expression->Type;
	if (expressionType != Unknown && expressionType != IntType && expressionType != FloatType)
	{
		string error = "Cannot apply operator for not numeric type: ";
		error.append(ExpressionTypeStrings[expressionType]);
		ErrorHandler::printError(Expression->LineNumber, error.c_str());
		return Unknown;
	}

	Type = expressionType;
	return expressionType;
}

ExpressionType NotExpression::resolveType(Scope_Ptr currentScope)
{
	Expression->resolveType(currentScope);
	auto expressionType = Expression->Type;
	if (expressionType != Unknown && expressionType != BoolType)
	{
		string error = "Cannot apply operator for not boolean type: ";
		error.append(ExpressionTypeStrings[expressionType]);
		ErrorHandler::printError(Expression->LineNumber, error.c_str());
		return Unknown;
	}

	Type = expressionType;
	return expressionType;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
