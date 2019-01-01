#pragma once
#include "Lexer.h"
#include "pch.h"

#define TO_PTR(CLASS) typedef std::shared_ptr<CLASS> ## CLASS ## _Ptr;

struct Scope; TO_PTR(Scope)
struct ProgramCode; TO_PTR(ProgramCode)
struct InstructionOperand
{
	int value;
	InstructionOperand(){}
	InstructionOperand(int Value)
	{
		value = Value;
	}
}; TO_PTR(InstructionOperand)
struct WhileStatement;
struct Function;
struct ConditionalNode;
struct NotExpression;

enum ExpressionType
{
	Unknown,
	BoolType,	 
	BoolPointer, 
	IntType,	 
	IntPointer,  
	FloatType,	 
	FloatPointer,
	StringType,	 
	CharType,	 
	CharPointer, 
	VoidType	 
};

struct Node
{
	int LineNumber = 0;

	std::shared_ptr<Node> parent;

	virtual void resolveNames(Scope_Ptr scope);
	virtual void generateCode(ProgramCode_Ptr code);
	virtual void print(int level)
	{
		printf("%s%s\n", std::string(level*2, ' ').c_str(), std::string(typeid(*this).name()).substr(6).c_str());
	}

	WhileStatement* findCycleParent();

	Function* findFunctionParent();

	ConditionalNode* findConditionalNodeParent();
	NotExpression* findNotParent();
};
TO_PTR(Node)

struct Token : Node {}; TO_PTR(Token)

struct Identifier : Token 
{
	std::string Value;

	void print(int level) override
	{
		Token::print(level);
		printf("%sValue: %s\n", std::string((level + 1) * 2, ' ').c_str(), Value.c_str());
	}
}; TO_PTR(Identifier)

struct BlockStart : Token {}; TO_PTR(BlockStart)
struct BlockEnd : Token {}; TO_PTR(BlockEnd)
struct StatementEnd : Token {}; TO_PTR(StatementEnd)
struct SeperatorToken : Token {}; TO_PTR(SeperatorToken)
struct OpenParan : Token {}; TO_PTR(OpenParan)
struct CloseParan : Token {}; TO_PTR(CloseParan)
struct SQBracketOpen : Token {}; TO_PTR(SQBracketOpen)
struct SQBracketClose : Token {}; TO_PTR(SQBracketClose)
struct WhileKeyword : Token {}; TO_PTR(WhileKeyword)
struct IfKeyword : Token {}; TO_PTR(IfKeyword)
struct ElseKeyword : Token {}; TO_PTR(ElseKeyword)
struct ReturnKeyword : Token {}; TO_PTR(ReturnKeyword)
struct ScanKeyword : Token {}; TO_PTR(ScanKeyword)
struct PrintKeyword : Token {}; TO_PTR(PrintKeyword)
struct BreakKeyword : Token {}; TO_PTR(BreakKeyword)
struct ContinueKeyword : Token {}; TO_PTR(ContinueKeyword)
struct TrueKeyword : Token {}; TO_PTR(TrueKeyword)
struct FalseKeyword : Token {}; TO_PTR(FalseKeyword)

struct OpPrint : Token {}; TO_PTR(OpPrint)
struct OpScan : Token {}; TO_PTR(OpScan)
struct OpAssign : Token {}; TO_PTR(OpAssign)
struct OpMinus : Token {}; TO_PTR(OpMinus)
struct OpNot : Token {}; TO_PTR(OpNot)
struct OpAnd : Token {}; TO_PTR(OpAnd)
struct OpOr : Token {}; TO_PTR(OpOr)

struct OpComp : Token {}; TO_PTR(OpComp)
struct OpLess : OpComp {}; TO_PTR(OpLess)
struct OpMore : OpComp {}; TO_PTR(OpMore)
struct OpMoreEq : OpComp {}; TO_PTR(OpMoreEq)
struct OpLessEq : OpComp {}; TO_PTR(OpLessEq)
struct OpEq : OpComp {}; TO_PTR(OpEq)
struct OpNotEq : OpComp {}; TO_PTR(OpNotEq)

struct OpMult : Token {}; TO_PTR(OpMult)
struct OpAdd : Token {}; TO_PTR(OpAdd)

struct TypeKeyword : Token 
{
	virtual ExpressionType getType()
	{
		return Unknown;
	}
	virtual int getSize()
	{
		return 2;
	}
	bool IsPointer;
}; TO_PTR(TypeKeyword)
struct BoolKeyword : TypeKeyword 
{
	ExpressionType getType() override
	{
		return IsPointer ? BoolPointer : BoolType;
	}
	int getSize() override
	{
		return 1;
	}
}; TO_PTR(BoolKeyword)
struct IntegerKeyword : TypeKeyword 
{
	ExpressionType getType() override
	{
		return IsPointer ? IntPointer : IntType;
	}
	int getSize() override
	{
		return 2;
	}
}; TO_PTR(IntegerKeyword)
struct FloatKeyword : TypeKeyword 
{
	ExpressionType getType() override
	{
		return IsPointer ? FloatPointer : FloatType;
	}
	int getSize() override
	{
		return 4;
	}
}; TO_PTR(FloatKeyword)
struct StringKeyword : TypeKeyword 
{
	int Size;
	ExpressionType getType() override
	{
		return StringType;
	}	
	int getSize() override
	{
		return Size;
	}
}; TO_PTR(StringKeyword)
struct CharKeyword : TypeKeyword 
{
	ExpressionType getType() override
	{
		return IsPointer ? CharPointer : CharType;
	}
	int getSize() override
	{
		return 1;
	}
}; TO_PTR(CharKeyword)
struct VoidKeyword : TypeKeyword 
{
	ExpressionType getType() override
	{
		return VoidType;
	}
}; TO_PTR(VoidKeyword)

struct VariableDeclaration
{
	TypeKeyword_Ptr Type;
	int relativeAddress;
}; TO_PTR(VariableDeclaration)

struct ConditionalNode
{
	InstructionOperand_Ptr falseJmpAddress = std::make_shared<InstructionOperand>();
};

struct Expression : Node {
	ExpressionType Type;
	void resolveNames(Scope_Ptr) override {}
	virtual ExpressionType resolveType(Scope_Ptr);
}; TO_PTR(Expression)

struct Param : Node
{
	Expression_Ptr Expression;
	SeperatorToken_Ptr Seperator;

	void print(int level) override
	{
		Node::print(level);
		level++;
		if (Expression != nullptr)
			Expression->print(level);
		if (Seperator != nullptr)
			Seperator->print(level);
	}
}; TO_PTR(Param)

struct FunctionParams : Node
{
	OpenParan_Ptr ListStartToken;
	std::vector<Param_Ptr> Parameters;
	CloseParan_Ptr ListEndToken;

	void print(int level) override
	{
		Node::print(level);
		level++;
		if (ListStartToken != nullptr)
			ListStartToken->print(level);
		for (Param_Ptr param : Parameters)
		{
			param->print(level);
		}
		if (ListEndToken != nullptr)
			ListEndToken->print(level);
	}

}; TO_PTR(FunctionParams)

struct BinaryExpression : Expression
{
	Expression_Ptr Left;
	Expression_Ptr Right;

	void print(int level) override
	{
		Expression::print(level);
		level++;
		if (Left != nullptr)
			Left->print(level);
		if (Right != nullptr)
			Right->print(level);
	}
	void generateCode(ProgramCode_Ptr) override;
}; TO_PTR(BinaryExpression)

struct MultiplicationExpression : BinaryExpression
{
	OpMult_Ptr Operator;
	void print(int level) override
	{
		BinaryExpression::print(level);
		level++;
		if (Operator != nullptr)
			Operator->print(level);
	}
	ExpressionType resolveType(Scope_Ptr) override;
	void generateCode(ProgramCode_Ptr) override;
}; TO_PTR(MultiplicationExpression)

struct AddExpression : BinaryExpression
{
	OpAdd_Ptr Operator;

	void print(int level) override
	{
		BinaryExpression::print(level);
		level++;
		if (Operator != nullptr)
			Operator->print(level);
	}
	ExpressionType resolveType(Scope_Ptr) override;
	void generateCode(ProgramCode_Ptr) override;
}; TO_PTR(AddExpression)

struct CompExpression : BinaryExpression, ConditionalNode
{
	CompExpression()
	{
		falseJmpAddress->value = -1;
	}
	OpComp_Ptr Operator;

	void print(int level) override
	{
		BinaryExpression::print(level);
		level++;
		if (Operator != nullptr)
			Operator->print(level);
	}
	ExpressionType resolveType(Scope_Ptr) override;
	void generateCode(ProgramCode_Ptr) override;
}; TO_PTR(CompExpression)

struct OrExpression : BinaryExpression, ConditionalNode
{
	OrExpression()
	{
		falseJmpAddress->value = 1;
	}
	OpOr_Ptr Operator;

	void print(int level) override
	{
		BinaryExpression::print(level);
		level++;
		if (Operator != nullptr)
			Operator->print(level);
	}
	ExpressionType resolveType(Scope_Ptr) override;
	void generateCode(ProgramCode_Ptr) override;
}; TO_PTR(OrExpression)

struct AndExpression : BinaryExpression, ConditionalNode
{
	AndExpression()
	{
		falseJmpAddress->value = 0x100;
	}

	OpAnd_Ptr Operator;

	void print(int level) override
	{
		BinaryExpression::print(level);
		level++;
		if (Operator != nullptr)
			Operator->print(level);
	}
	ExpressionType resolveType(Scope_Ptr) override;
	void generateCode(ProgramCode_Ptr) override;
}; TO_PTR(AndExpression)

struct IdentifierExpression : Expression
{
	std::string Value;

	void print(int level) override
	{
		Expression::print(level);
		printf("%sValue: %s\n", std::string((level+1)*2, ' ').c_str(), Value.c_str());
	}
	ExpressionType resolveType(Scope_Ptr) override;
	void generateCode(ProgramCode_Ptr) override;
	virtual void setData(ProgramCode_Ptr code, ExpressionType type);

	int AccessRegister;
	InstructionOperand_Ptr Offset;
	int AccessMod;
};
TO_PTR(IdentifierExpression)

struct MemberAccessExpression : IdentifierExpression
{
	Expression_Ptr AccessPosition; //TODO: make sure it is it

	void print(int level) override
	{
		IdentifierExpression::print(level);
		AccessPosition->print(level);
	}
	ExpressionType resolveType(Scope_Ptr) override;
	void setData(ProgramCode_Ptr code, ExpressionType type) override;
}; TO_PTR(MemberAccessExpression)

struct FunctionCallExpression : Expression
{
	IdentifierExpression_Ptr Name;
	FunctionParams_Ptr Parameters;

	void print(int level) override
	{
		Expression::print(level);
		level++;
		if (Name != nullptr)
			Name->print(level);
		if (Parameters != nullptr)
			Parameters->print(level);
	}

	void resolveNames(Scope_Ptr) override;
	ExpressionType resolveType(Scope_Ptr) override;
	void generateCode(ProgramCode_Ptr) override;
}; TO_PTR(FunctionCallExpression)

struct StringExpression : Expression
{
	std::string Value;

	void print(int level) override
	{
		Expression::print(level);
		printf("%sValue: %s\n", std::string((level + 1) * 2, ' ').c_str(), Value.c_str());
	}
	ExpressionType resolveType(Scope_Ptr) override;
	void generateCode(ProgramCode_Ptr) override;
}; TO_PTR(StringExpression)

struct CharExpression : Expression
{
	char Value;
	void print(int level) override
	{
		Expression::print(level);
		printf("%sValue: %c\n", std::string((level + 1) * 2, ' ').c_str(), Value);
	}
	ExpressionType resolveType(Scope_Ptr) override;
	void generateCode(ProgramCode_Ptr) override;
}; TO_PTR(CharExpression)

struct IntegerExpression : Expression
{
	int Value;
	void print(int level) override
	{
		Expression::print(level);
		printf("%sValue: %d\n", std::string((level + 1) * 2, ' ').c_str(), Value);
	}
	ExpressionType resolveType(Scope_Ptr) override;
	void generateCode(ProgramCode_Ptr) override;
}; TO_PTR(IntegerExpression)

struct FloatExpression : Expression
{
	double Value;
	void print(int level) override
	{
		Expression::print(level);
		printf("%sValue: %f\n", std::string((level + 1) * 2, ' ').c_str(), Value);
	}
	ExpressionType resolveType(Scope_Ptr) override;
	void generateCode(ProgramCode_Ptr) override;
}; TO_PTR(FloatExpression)

struct BoolExpression : Expression
{
	ExpressionType resolveType(Scope_Ptr) override;
}; TO_PTR(BoolExpression)

struct TrueExpresion : BoolExpression
{
	TrueKeyword_Ptr Keyword;

	void print(int level) override
	{
		Expression::print(level);
		level++;
		if (Keyword != nullptr)
			Keyword->print(level);
	}
	void generateCode(ProgramCode_Ptr) override;
}; TO_PTR(TrueExpresion)

struct FalseExpresion : BoolExpression
{
	FalseKeyword_Ptr Keyword;

	void print(int level) override
	{
		Expression::print(level);
		level++;
		if (Keyword != nullptr)
			Keyword->print(level);
	}
	void generateCode(ProgramCode_Ptr) override;
}; TO_PTR(FalseExpresion)

struct GroupedExpression : Expression
{
	OpenParan_Ptr GroupStart;
	Expression_Ptr Expression;
	CloseParan_Ptr GroupFinish;

	void print(int level) override
	{
		Expression::print(level);
		level++;
		if (GroupStart != nullptr)
			GroupStart->print(level);
		if (Expression != nullptr)
			Expression->print(level);
		if (GroupFinish != nullptr)
			GroupFinish->print(level);
	}
	void resolveNames(Scope_Ptr) override;
	ExpressionType resolveType(Scope_Ptr) override;
	void generateCode(ProgramCode_Ptr) override;
}; TO_PTR(GroupedExpression)

struct UnaryExpression : Expression
{
	Expression_Ptr Expression;

	void print(int level) override
	{
		Expression::print(level);
		level++;
		if (Expression != nullptr)
			Expression->print(level);
	}
	void resolveNames(Scope_Ptr) override;
	void generateCode(ProgramCode_Ptr) override;
}; TO_PTR(UnaryExpression)

struct SubExpression : UnaryExpression
{
	OpMinus_Ptr Prefix;

	void print(int level) override
	{
		UnaryExpression::print(level);
		level++;
		if (Prefix != nullptr)
			Prefix->print(level);
	}
	ExpressionType resolveType(Scope_Ptr) override;
	void generateCode(ProgramCode_Ptr) override;
}; TO_PTR(SubExpression)

struct NotExpression : UnaryExpression
{
	OpNot_Ptr Prefix;

	void print(int level) override
	{
		UnaryExpression::print(level);
		level++;
		if (Prefix != nullptr)
			Prefix->print(level);
	}
	ExpressionType resolveType(Scope_Ptr) override;
	void generateCode(ProgramCode_Ptr) override;
}; TO_PTR(NotExpression)

struct Body : Node {}; TO_PTR(Body)

struct Statement : Body {}; TO_PTR(Statement)

struct BlockBody : Body
{
	BlockStart_Ptr Start;
	std::vector<Statement_Ptr> Statements;
	BlockEnd_Ptr End;

	void print(int level) override
	{
		Body::print(level);
		level++;
		Start->print(level);
		for (Statement_Ptr stmt : Statements)
		{
			stmt->print(level);
		}
		if (End != nullptr)
			End->print(level);
	}

	void resolveNames(Scope_Ptr scope) override;
	void generateCode(ProgramCode_Ptr code) override;
};
TO_PTR(BlockBody)

struct EmptyStatement : Statement
{
	StatementEnd_Ptr EndToken;
	void print(int level) override
	{
		Statement::print(level);
		level++;
		if(EndToken != nullptr)
			EndToken->print(level);;
	}

	void resolveNames(Scope_Ptr scope) override {}
	void generateCode(ProgramCode_Ptr code) override {}
}; TO_PTR(EmptyStatement)

struct AssignmentNode : Node
{
	OpAssign_Ptr Operator;
	Expression_Ptr Expression;

	void print(int level) override
	{
		Node::print(level);
		level++;
		if (Operator != nullptr)
			Operator->print(level);
		if (Expression != nullptr)
			Expression->print(level);
	}
	void CheckIsType(ExpressionType, Scope_Ptr);
}; TO_PTR(AssignmentNode)

struct AssignmentStatement : EmptyStatement, ConditionalNode
{
	AssignmentStatement()
	{
		falseJmpAddress->value = 0x1000;
	}

	IdentifierExpression_Ptr Variable;
	AssignmentNode_Ptr Assignment;

	void print(int level) override
	{
		EmptyStatement::print(level);
		level++;
		if (Variable != nullptr)
			Variable->print(level);
		if (Assignment != nullptr)
			Assignment->print(level);
	}

	void resolveNames(Scope_Ptr scope) override;
	void generateCode(ProgramCode_Ptr code) override;
}; TO_PTR(AssignmentStatement)

struct DeclarationStatement : EmptyStatement, VariableDeclaration, ConditionalNode
{
	DeclarationStatement()
	{
		falseJmpAddress->value = 0x111;
	}

	Identifier_Ptr Variable;
	AssignmentNode_Ptr Assignment;
	IntegerExpression_Ptr Size;

	void print(int level) override
	{
		EmptyStatement::print(level);
		level++;
		if (Type != nullptr)
			Type->print(level);
		if (Variable != nullptr)
			Variable->print(level);
		if (Assignment != nullptr)
			Assignment->print(level);
	}

	void resolveNames(Scope_Ptr scope) override;
	void generateCode(ProgramCode_Ptr code) override;
}; TO_PTR(DeclarationStatement)

struct PointerDeclarationStatement : DeclarationStatement
{
	Expression_Ptr Size;
	TypeKeyword_Ptr ItemType;
};

struct FunctionCallStatement : EmptyStatement
{
	FunctionCallExpression_Ptr FunctionCall;
	void print(int level) override
	{
		EmptyStatement::print(level);
		level++;
		if (FunctionCall != nullptr)
			FunctionCall->print(level);
	}

	void resolveNames(Scope_Ptr scope) override;
	void generateCode(ProgramCode_Ptr code) override;
}; TO_PTR(FunctionCallStatement)

struct ReturnStatement : EmptyStatement, ConditionalNode
{
	ReturnStatement()
	{
		falseJmpAddress->value = 0x11110;
	}

	ReturnKeyword_Ptr Keyword;
	Expression_Ptr ReturnValue;

	void print(int level)
	{
		EmptyStatement::print(level);
		level++;
		if (Keyword != nullptr)
			Keyword->print(level);
		if (ReturnValue != nullptr)
			ReturnValue->print(level);
	}

	void resolveNames(Scope_Ptr scope) override;
	void generateCode(ProgramCode_Ptr code) override;
}; TO_PTR(ReturnStatement)

struct PrintNode : Node
{
	OpPrint_Ptr Operator;
	Expression_Ptr Expression;

	void print(int level) override
	{
		Node::print(level);
		level++;
		if (Operator != nullptr)
			Operator->print(level);
		if (Expression != nullptr)
			Expression->print(level);
	}
}; TO_PTR(PrintNode)

struct PrintStatement : EmptyStatement
{
	PrintKeyword_Ptr Keyword;
	std::vector<PrintNode_Ptr> Prints;

	void print(int level) override
	{
		Node::print(level);
		level++;
		if (Keyword != nullptr)
			Keyword->print(level);
		for (PrintNode_Ptr print : Prints)
		{
			print->print(level);
		}
	}

	void resolveNames(Scope_Ptr scope) override;
	void generateCode(ProgramCode_Ptr code) override;
}; TO_PTR(PrintStatement)

struct ScanNode : Node
{
	OpScan_Ptr Operator;
	IdentifierExpression_Ptr Identifier;

	void print(int level) override
	{
		Node::print(level);
		level++;
		if (Operator != nullptr)
			Operator->print(level);
		if (Identifier != nullptr)
			Identifier->print(level);
	}
}; TO_PTR(ScanNode)

struct ScanStatement : EmptyStatement
{
	ScanKeyword_Ptr Keyword;
	std::vector<ScanNode_Ptr> Scans;

	void print(int level) override
	{
		Node::print(level);
		level++;
		if (Keyword != nullptr)
			Keyword->print(level);
		for (ScanNode_Ptr scan : Scans)
		{
			scan->print(level);
		}
	}

	void resolveNames(Scope_Ptr scope) override;
	void generateCode(ProgramCode_Ptr code) override;
}; TO_PTR(ScanStatement)

struct BreakStatement : EmptyStatement
{
	BreakKeyword_Ptr Keyword;

	void print(int level) override
	{
		EmptyStatement::print(level);
		level++;
		if (Keyword != nullptr)
			Keyword->print(level);
	}
	void resolveNames(Scope_Ptr) override;
	void generateCode(ProgramCode_Ptr code) override;
}; TO_PTR(BreakStatement)

struct ContinueStatement : EmptyStatement
{
	ContinueKeyword_Ptr Keyword;

	void print(int level) override
	{
		EmptyStatement::print(level);
		level++;
		if (Keyword != nullptr)
			Keyword->print(level);
	}

	void resolveNames(Scope_Ptr) override;
	void generateCode(ProgramCode_Ptr code) override;
}; TO_PTR(ContinueStatement)

struct  WhileStatement : Statement, ConditionalNode
{
	WhileStatement()
	{
		falseJmpAddress->value = 0x11001;
	}

	WhileKeyword_Ptr Keyword;
	Expression_Ptr Condition;
	Body_Ptr Body;
	InstructionOperand_Ptr statementStartAddress = std::make_shared<InstructionOperand>();
	InstructionOperand_Ptr statementEndAddress = falseJmpAddress;

	void print(int level) override
	{
		Statement::print(level);
		level++;
		if (Keyword != nullptr)
			Keyword->print(level);
		if (Condition != nullptr)
			Condition->print(level);
		if (Body != nullptr)
			Body->print(level);
	}

	void resolveNames(Scope_Ptr scope) override;
	void generateCode(ProgramCode_Ptr code) override;
}; TO_PTR(WhileStatement)

struct ElseNode : Node 
{
	ElseKeyword_Ptr ElseKeyword;

	void print(int level) override
	{
		Node::print(level);
		level++;
		if (ElseKeyword != nullptr)
			ElseKeyword->print(level);
	}
}; TO_PTR(ElseNode)

struct IfStatement : Statement, ConditionalNode
{
	IfStatement()
	{
		falseJmpAddress->value = 0x11110000;
	}

	IfKeyword_Ptr IfKeyword;
	Expression_Ptr Condition;
	Body_Ptr Body;
	ElseNode_Ptr Else;

	void print(int level) override
	{
		Node::print(level);
		level++;
		if (IfKeyword != nullptr)
			IfKeyword->print(level);
		if (Condition != nullptr)
			Condition->print(level);
		if (Body != nullptr)
			Body->print(level);
		if (Else != nullptr)
			Else->print(level);
	}

	void resolveNames(Scope_Ptr scope) override;
	void generateCode(ProgramCode_Ptr code) override;
}; TO_PTR(IfStatement)

struct ElseIf : ElseNode
{
	IfStatement_Ptr If;

	void print(int level) override
	{
		ElseNode::print(level);
		level++;
		if (If != nullptr)
			If->print(level);
	}

	void resolveNames(Scope_Ptr scope) override;
	void generateCode(ProgramCode_Ptr code) override;
}; TO_PTR(ElseIf)

struct Else : ElseNode
{
	Body_Ptr ElseBody;

	void print(int level) override
	{
		ElseNode::print(level);
		level++;
		if (ElseBody != nullptr)
			ElseBody->print(level);
	}

	void resolveNames(Scope_Ptr scope) override;
	void generateCode(ProgramCode_Ptr code) override;
}; TO_PTR(Else)

struct ParamDef : Node, VariableDeclaration
{
	Identifier_Ptr Name;
	SeperatorToken_Ptr Seperator;

	void print(int level) override
	{
		Node::print(level);
		level++;
		if (Seperator != nullptr)
			Seperator->print(level);
		if (Type != nullptr)
			Type->print(level);
		if (Name != nullptr)
			Name->print(level);
	}
}; TO_PTR(ParamDef)

struct FunctionParamsDef : Node
{
	OpenParan_Ptr ListStartToken;
	std::vector<ParamDef_Ptr> Parameters;
	CloseParan_Ptr ListEndToken;

	void print(int level) override
	{
		Node::print(level);
		level++;
		if(ListStartToken != nullptr)
			ListStartToken->print(level);

		for (ParamDef_Ptr param : Parameters)
		{
			param->print(level);
		}

		if (ListEndToken != nullptr)
			ListEndToken->print(level);
	}
}; TO_PTR(FunctionParamsDef)

struct Function : Node
{
	TypeKeyword_Ptr ReturnType;
	Identifier_Ptr Name;
	FunctionParamsDef_Ptr Parameters;
	BlockBody_Ptr Body;
	bool HasReturn = false;
	bool IsMain = false;

	InstructionOperand_Ptr functionEndAddress = std::make_shared<InstructionOperand>();
	InstructionOperand_Ptr functionStartAddress = std::make_shared<InstructionOperand>();

	void print(int level) override
	{
		Node::print(level);
		level++;
		if (ReturnType != NULL)
			ReturnType->print(level);
		if (Name != NULL)
			Name->print(level);
		if (Parameters != NULL)
			Parameters->print(level);
		if (Body != NULL)
			Body->print(level);
	}

	void resolveNames(Scope_Ptr scope) override;
	void generateCode(ProgramCode_Ptr code) override;
}; TO_PTR(Function)

struct SyntaxTree
{
	std::vector<Function_Ptr> functions;
	std::vector<Function_Ptr> rezervedFunctions;
	void resolveNames(Scope_Ptr Scope);
	void generateCode(ProgramCode_Ptr program);
}; TO_PTR(SyntaxTree)

struct Parser
{
	SyntaxTree_Ptr build_syntax_tree(std::vector<Lexem_ptr>);
	void print_tree(SyntaxTree_Ptr);

private:
	int offset = 0;
	std::vector<Lexem_ptr> lexems;

	bool accept(LexemType type);
	Identifier_Ptr parse_identifier();
	TypeKeyword_Ptr parse_type();
	TypeKeyword_Ptr parse_return_type();
	IdentifierExpression_Ptr parse_identifier_expression();
	ParamDef_Ptr parse_parameter();
	FunctionParamsDef_Ptr parse_parameters_list();
	StatementEnd_Ptr parse_statement_end_token();
	StringExpression_Ptr parse_string_expression();
	CharExpression_Ptr parse_char_expression();
	IntegerExpression_Ptr parse_integer_expression();
	FloatExpression_Ptr parse_float_expression();
	TrueExpresion_Ptr parse_true_expression();
	FalseExpresion_Ptr parse_false_expression();
	GroupedExpression_Ptr parse_grouped_expression();
	Expression_Ptr parse_expression();
	AssignmentNode_Ptr parse_assignment();
	DeclarationStatement_Ptr parse_declaration();
	AssignmentStatement_Ptr parse_assignment_statement();
	EmptyStatement_Ptr parse_continue_or_break_statement();
	ReturnStatement_Ptr parse_return_statement();
	ScanNode_Ptr parse_scan();
	ScanStatement_Ptr parse_scan_statement();
	PrintNode_Ptr parse_print();
	PrintStatement_Ptr parse_print_statement();
	Param_Ptr parse_call_parameter();
	FunctionParams_Ptr parse_call_parameters();
	FunctionCallExpression_Ptr parse_function_call_expression();
	FunctionCallStatement_Ptr parse_function_call_statement();
	Body_Ptr parse_body();
	WhileStatement_Ptr parse_while_statement();
	ElseNode_Ptr parse_else_node();
	IfStatement_Ptr parse_if_statement();
	Statement_Ptr parse_statement();
	BlockBody_Ptr parse_block();
	Function_Ptr build_function();
	NotExpression_Ptr parse_not_expression();
	SubExpression_Ptr parse_sub_expression();
	Expression_Ptr parse_value_expression();
	Expression_Ptr parse_multiplication_expression();
	Expression_Ptr parse_add_expression();
	OpComp_Ptr parse_comparison_operator();
	Expression_Ptr parse_comparison_expression();
	Expression_Ptr parse_or_expression();
	Expression_Ptr parse_and_expression();
};