#include "pch.h"
#include "CodeGenerator.h"
#include <fstream>

using namespace std;

const int COM_ENTRY_POINT = 0x100;

int GetTypeSize(ExpressionType type)
{
	switch (type)
	{
	case FloatType:
		return 4;
	case IntType:
	case StringType:
		return 2;
	case CharType:
	case BoolType:
		return 1;
	default:
		return 2;
	}
}

void ProgramCode::AddFunction(std::string name, Function* function)
{
	functions.insert(std::pair<std::string, Function*>(name, function));
}

void ProgramCode::AddVariable(string name, VariableDeclaration* variable, int additionalSpace = 0)
{
	functionVariables.insert(std::pair<std::string, VariableDeclaration*>(name, variable));
	currentRelativeAddress -= GetTypeSize(variable->Type->getType()) + additionalSpace;
	variable->relativeAddress = currentRelativeAddress;
}

void ProgramCode::AddParam(string name, VariableDeclaration* variable)
{
	functionVariables.insert(std::pair<std::string, VariableDeclaration*>(name, variable));
	paramRelativeAddress += 2;
	variable->relativeAddress = paramRelativeAddress;
}

void ProgramCode::AddInstruction(Instruction_Ptr instruction)
{
	currentAddress += instruction->Size;
	Instructions.push_back(instruction);
}

void ProgramCode::IncludeRezervedFunction(string name)
{
	std::ifstream file(name.c_str(), std::ios::binary);
	char c;
	while ((c = file.get()) != EOF)
	{
		currentAddress++;
		RezervedFunctionsCode.push_back(c);
	}
	file.close();
}

void ProgramCode::Print()
{
	int count = 0;
	vector<char> bytes;
	printf(".code\nCode:\n");
	for (auto instruction : Instructions)
	{
		for (auto instByte : instruction->getBytes())
		{
			bytes.push_back(instByte);
		}
		printf("loc_%x: ", count);
		instruction->print();
		printf("\n");
		count += instruction->Size;
	}
	printf("END Code\n.data\n");

	for (auto includedByte : RezervedFunctionsCode)
	{
		bytes.push_back(includedByte);
		count++;
	}

	for (auto floatConstant : FloatConstants)
	{
		auto constantValue = floatConstant->BinaryValue;
		bytes.push_back(constantValue);
		bytes.push_back(constantValue >> 8);
		bytes.push_back(constantValue >> 16);
		bytes.push_back(constantValue >> 24);
		printf("float_%x DD %f\n", count, floatConstant->FloatValue);
		count += 4;
	}
	for (auto stringConstant : StringConstants)
	{
		auto string = stringConstant->StringValue.c_str();
		auto stringLenght = stringConstant->StringValue.size();
		printf("string_%x db \"%s\"\n", stringConstant->value - COM_ENTRY_POINT, string);
	
		for (int i = 0; i < stringLenght; i++)
		{
			bytes.push_back(string[i]);
		}

		count += stringConstant->StringValue.size()+1;
	}
	printf("\n\n");

	string outputFile = fileName.substr(0, fileName.rfind('\\'));
	outputFile += "\\a.exe";

	std::ofstream file(outputFile.c_str(), std::ios::binary);
	file.write(&(bytes[0]), bytes.size());

	string output;
	output.assign(bytes.begin(), bytes.end());
	for (char byte : bytes)
	{
		printf("%02hhx ", byte);
	}
	file.close();
}

ConditionalNode* Node::findConditionalNodeParent()
{
	auto nodeParent = parent;
	while (nodeParent != nullptr)
	{
		auto conditionalNode = dynamic_cast<ConditionalNode*>(&(*nodeParent));
		if (conditionalNode != nullptr)
		{
			return conditionalNode;
		}
		nodeParent = nodeParent->parent;
	}

	return nullptr;
}

NotExpression* Node::findNotParent()
{
	auto nodeParent = parent;
	while (nodeParent != nullptr)
	{
		auto notExpression = dynamic_cast<NotExpression*>(&(*nodeParent));
		if (notExpression != nullptr)
		{
			return notExpression;
		}
		nodeParent = nodeParent->parent;
	}

	return nullptr;
}

void SyntaxTree::generateCode(ProgramCode_Ptr programCode)
{
	Function_Ptr mainFunction = nullptr;
	auto callInstruction = make_shared<CallInstruction>();
	programCode->AddInstruction(callInstruction); 
	callInstruction->Operands.push_back(make_shared<InstructionOperand>(programCode->currentAddress));

	Instruction_Ptr instruction = make_shared<MovWordConstInstruction>(Register);
	programCode->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(AX));
	instruction->Operands.push_back(make_shared<InstructionOperand>(0x4C00));

	instruction = make_shared<InteruptInstruction>();
	programCode->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(0x21));
	for (auto function : functions)
	{
		programCode->AddFunction(function->Name->Value, &(*function));
	}
	for (auto rezervedFunction : rezervedFunctions)
	{
		programCode->AddFunction(rezervedFunction->Name->Value, &(*rezervedFunction));
	}
	for (auto function : functions)
	{
		programCode->currentRelativeAddress = 0;
		programCode->paramRelativeAddress = 2;
		programCode->functionVariables.clear();
		if (function->IsMain)
		{
			callInstruction->Operands.push_back(function->functionStartAddress);
		}
		function->functionStartAddress->value = programCode->currentAddress;
		function->generateCode(programCode);
	}

	for (auto rezervedFunction : rezervedFunctions)
	{
		rezervedFunction->functionStartAddress->value = programCode->currentAddress;
		programCode->IncludeRezervedFunction(rezervedFunction->Name->Value);
	}

	for (auto floatContant : programCode->FloatConstants)
	{
		floatContant->value = programCode->currentAddress + COM_ENTRY_POINT;
		programCode->currentAddress += 4;
	}
	for (auto stringConstant : programCode->StringConstants)
	{
		stringConstant->value = programCode->currentAddress + COM_ENTRY_POINT;
		programCode->currentAddress += stringConstant->StringValue.size();
	}
}

void Node::generateCode(ProgramCode_Ptr program)
{
	throw std::exception("Node::generateCode should never be called.");
}

void Function::generateCode(ProgramCode_Ptr program)
{
	Instruction_Ptr instruction = make_shared<PushInstruction>();
	program->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(BP));

	instruction = make_shared<MovWordInstruction>(Register);
	program->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(BP));
	instruction->Operands.push_back(make_shared<InstructionOperand>(SP));
	instruction = make_shared<SubConstInstruction>();
	program->AddInstruction(instruction);
	auto label = make_shared<InstructionOperand>();
	instruction->Operands.push_back(label);
	for(auto param = Parameters->Parameters.rbegin(); param != Parameters->Parameters.rend(); param++)
	{
		program->AddParam((*param)->Name->Value, dynamic_cast<VariableDeclaration *>(&(**param)));
	}
	Body->generateCode(program);
	label->value = -(program->currentRelativeAddress);

	functionEndAddress->value = program->currentAddress;
	instruction = make_shared<MovWordInstruction>(Register);
	program->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(SP));
	instruction->Operands.push_back(make_shared<InstructionOperand>(BP));
	instruction = make_shared<PopInstruction>();
	program->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(BP));
	instruction = make_shared<ReturnInstruction>();
	program->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(program->paramRelativeAddress - 2));
}

void BlockBody::generateCode(ProgramCode_Ptr code)
{
	for (auto statement : Statements)
	{
		statement->generateCode(code);
	}
}

void AssignmentStatement::generateCode(ProgramCode_Ptr code)
{
	Assignment->Expression->generateCode(code);
	Variable->setData(code, Assignment->Expression->Type);
	auto reg = make_shared<InstructionOperand>(Variable->AccessRegister);
	auto mod = (Mod)Variable->AccessMod;

	if (Assignment->Expression->Type == BoolType)
	{
		//TODO: [optimization] if assigning identifier use it directly
		Instruction_Ptr instruction = make_shared<MovByteConstInstruction>(mod);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(reg);
		if (Variable->Offset != nullptr)
			instruction->Operands.push_back(Variable->Offset);
		instruction->Operands.push_back(make_shared<InstructionOperand>(1));

		instruction = make_shared<JMPInstruction>();
		code->AddInstruction(instruction);
		instruction->Operands.push_back(falseJmpAddress);
		falseJmpAddress->value = code->currentAddress;
		auto jumpOutOp = make_shared<InstructionOperand>();
		instruction->Operands.push_back(jumpOutOp);

		instruction = make_shared<MovByteConstInstruction>(mod);
		code->AddInstruction(instruction);
		jumpOutOp->value = code->currentAddress;
		instruction->Operands.push_back(reg);
		if (Variable->Offset != nullptr)
			instruction->Operands.push_back(Variable->Offset);
		instruction->Operands.push_back(make_shared<InstructionOperand>(0));
		
	}
	else if (Assignment->Expression->Type == CharType)
	{
		auto instruction = make_shared<MovByteInstruction>(mod);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(reg);
		if (Variable->Offset != nullptr)
			instruction->Operands.push_back(Variable->Offset);
		instruction->Operands.push_back(make_shared<InstructionOperand>(AL));
	}
	else
	{
		auto instruction = make_shared<MovWordInstruction>(mod);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(reg);
		if (Variable->Offset != nullptr)
			instruction->Operands.push_back(Variable->Offset);
		instruction->Operands.push_back(make_shared<InstructionOperand>(AX));
	}
}

void DeclarationStatement::generateCode(ProgramCode_Ptr code)
{
	int size = 0;
	if(Size != nullptr)
		size = Size->Value * Type->getSize();

	code->AddVariable(Variable->Value, this, size);
	auto reg = make_shared<InstructionOperand>(XX_BP);
	auto op = make_shared<InstructionOperand>();
	op->value = relativeAddress;
	if (Type->IsPointer)
	{
		Instruction_Ptr instruction = make_shared<LeaInstruction>(MemoryRelative);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(reg);
		instruction->Operands.push_back(make_shared<InstructionOperand>(op->value + 2));
		instruction->Operands.push_back(make_shared<InstructionOperand>(AX));

		instruction = make_shared<MovWordInstruction>(MemoryRelative, true);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(reg);
		instruction->Operands.push_back(op);
		instruction->Operands.push_back(make_shared<InstructionOperand>(AX));
	}
	if (Assignment != nullptr)
	{
		Assignment->Expression->generateCode(code);
		if (Type->getType() == BoolType)
		{
			Instruction_Ptr instruction = make_shared<MovByteConstInstruction>(MemoryRelative);
			code->AddInstruction(instruction);
			instruction->Operands.push_back(reg);
			instruction->Operands.push_back(op);
			instruction->Operands.push_back(make_shared<InstructionOperand>(1));

			instruction = make_shared<JMPInstruction>();
			code->AddInstruction(instruction);
			instruction->Operands.push_back(make_shared<InstructionOperand>(code->currentAddress));
			falseJmpAddress->value = code->currentAddress;
			auto jumpOutOp = make_shared<InstructionOperand>();
			instruction->Operands.push_back(jumpOutOp);

			instruction = make_shared<MovByteConstInstruction>(MemoryRelative);
			code->AddInstruction(instruction);
			jumpOutOp->value = code->currentAddress;
			instruction->Operands.push_back(reg);
			instruction->Operands.push_back(op);
			instruction->Operands.push_back(make_shared<InstructionOperand>(0));
		}
		else if (Type->getType() == CharType)
		{
			auto instruction = make_shared<MovByteInstruction>(MemoryRelative);
			code->AddInstruction(instruction);
			instruction->Operands.push_back(reg);
			instruction->Operands.push_back(op);
			instruction->Operands.push_back(make_shared<InstructionOperand>(AL));
		}
		else
		{
			auto instruction = make_shared<MovWordInstruction>(MemoryRelative);
			code->AddInstruction(instruction);
			instruction->Operands.push_back(reg);
			instruction->Operands.push_back(op);
			instruction->Operands.push_back(make_shared<InstructionOperand>(AX));
		}
	}
}

void FunctionCallStatement::generateCode(ProgramCode_Ptr code)
{
	FunctionCall->generateCode(code);
}

void ReturnStatement::generateCode(ProgramCode_Ptr code)
{
	if (ReturnValue != nullptr)
	{
		ReturnValue->generateCode(code);
		if (ReturnValue->Type == BoolType)
		{
			Instruction_Ptr instruction = make_shared<MovWordConstInstruction>(Register);
			code->AddInstruction(instruction);
			instruction->Operands.push_back(make_shared<InstructionOperand>(AX));
			instruction->Operands.push_back(make_shared<InstructionOperand>(1)),

			instruction = make_shared<JMPInstruction>();
			code->AddInstruction(instruction);
			instruction->Operands.push_back(make_shared<InstructionOperand>(code->currentAddress));
			auto jumpOutOp = make_shared<InstructionOperand>();
			falseJmpAddress->value = code->currentAddress;
			instruction->Operands.push_back(jumpOutOp);

			instruction = make_shared<MovWordConstInstruction>(Register);
			code->AddInstruction(instruction);
			jumpOutOp->value = code->currentAddress;
			instruction->Operands.push_back(make_shared<InstructionOperand>(AX));
			instruction->Operands.push_back(make_shared<InstructionOperand>(0));
		}
	}
	auto instruction = make_shared<JMPInstruction>();
	code->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(code->currentAddress));
	instruction->Operands.push_back(findFunctionParent()->functionEndAddress);
}

void PrintStatement::generateCode(ProgramCode_Ptr code)
{
	//TODO: not only char
	for (auto print : Prints)
	{
		print->Expression->generateCode(code);
		auto expressionType = print->Expression->Type;
		auto ax_register = make_shared<InstructionOperand>(AX);

		Instruction_Ptr instruction = make_shared<MovWordInstruction>(Register);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(make_shared<InstructionOperand>(DX));
		instruction->Operands.push_back(ax_register);

		instruction = make_shared<MovWordConstInstruction>(Register);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(ax_register);
		instruction->Operands.push_back(make_shared<InstructionOperand>(0x0200));

		instruction = make_shared<InteruptInstruction>();
		code->AddInstruction(instruction);
		instruction->Operands.push_back(make_shared<InstructionOperand>(0x21));
	}
}

void ScanStatement::generateCode(ProgramCode_Ptr code)
{
	//TODO: not only char
	for (auto scan : Scans)
	{
		auto expressionType = code->functionVariables[scan->Identifier->Value]->Type->getType();
		auto ax_register = make_shared<InstructionOperand>(AX);

		//TODO: interupt to check if char available

		Instruction_Ptr instruction = make_shared<MovWordConstInstruction>(Register);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(ax_register);
		instruction->Operands.push_back(make_shared<InstructionOperand>(0x0B00));

		instruction = make_shared<InteruptInstruction>();
		code->AddInstruction(instruction);
		instruction->Operands.push_back(make_shared<InstructionOperand>(0x21));

		instruction = make_shared<MovByteConstInstruction>(Register);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(make_shared<InstructionOperand>(AH));
		instruction->Operands.push_back(make_shared<InstructionOperand>(0));

		instruction = make_shared<CmpByteInstruction>();
		code->AddInstruction(instruction);
		instruction->Operands.push_back(make_shared<InstructionOperand>(AL));
		instruction->Operands.push_back(make_shared<InstructionOperand>(AH));

		auto jmp_address = make_shared<InstructionOperand>();
		instruction = make_shared<CondJmpInstruction>(JE);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(make_shared<InstructionOperand>(code->currentAddress));
		instruction->Operands.push_back(jmp_address);

		instruction = make_shared<MovWordConstInstruction>(Register);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(ax_register);
		instruction->Operands.push_back(make_shared<InstructionOperand>(0x0700));

		instruction = make_shared<InteruptInstruction>();
		code->AddInstruction(instruction);
		instruction->Operands.push_back(make_shared<InstructionOperand>(0x21));

		/*
		Instruction_Ptr instruction = make_shared<MovWordConstInstruction>(Register);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(ax_register);
		instruction->Operands.push_back(make_shared<InstructionOperand>(0x0600));
		
		instruction = make_shared<MovByteConstInstruction>(Register);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(make_shared<InstructionOperand>(DL));
		instruction->Operands.push_back(make_shared<InstructionOperand>(0xFF));

		instruction = make_shared<InteruptInstruction>();
		code->AddInstruction(instruction);
		instruction->Operands.push_back(make_shared<InstructionOperand>(0x21));
		*/
		/*
		Instruction_Ptr instruction = make_shared<MovWordConstInstruction>(Register);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(ax_register);
		instruction->Operands.push_back(make_shared<InstructionOperand>(0x0700));

		instruction = make_shared<InteruptInstruction>();
		code->AddInstruction(instruction);
		instruction->Operands.push_back(make_shared<InstructionOperand>(0x21));
		*/

		scan->Identifier->setData(code, expressionType);

		jmp_address->value = code->currentAddress;

		if (expressionType == BoolType || expressionType == CharType)
		{
			instruction = make_shared<MovByteInstruction>((Mod)scan->Identifier->AccessMod);
			code->AddInstruction(instruction);
			instruction->Operands.push_back(make_shared<InstructionOperand>(scan->Identifier->AccessRegister));
			if (scan->Identifier->Offset != nullptr)
				instruction->Operands.push_back(scan->Identifier->Offset);
			instruction->Operands.push_back(make_shared<InstructionOperand>(AL));
		}
		else
		{
			instruction = make_shared<MovByteConstInstruction>(Register);
			code->AddInstruction(instruction);
			instruction->Operands.push_back(make_shared<InstructionOperand>(AH));
			instruction->Operands.push_back(make_shared<InstructionOperand>(0));

			instruction = make_shared<MovWordInstruction>((Mod)scan->Identifier->AccessMod);
			code->AddInstruction(instruction);
			instruction->Operands.push_back(make_shared<InstructionOperand>(scan->Identifier->AccessRegister));
			if (scan->Identifier->Offset != nullptr)
				instruction->Operands.push_back(scan->Identifier->Offset);
			instruction->Operands.push_back(ax_register);
		}
	}
}

void BreakStatement::generateCode(ProgramCode_Ptr code)
{
	auto instruction = make_shared<JMPInstruction>();
	code->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(code->currentAddress));
	auto whileStatement = findCycleParent();
	instruction->Operands.push_back(whileStatement->statementEndAddress);
}

void ContinueStatement::generateCode(ProgramCode_Ptr code)
{
	auto instruction = make_shared<JMPInstruction>();
	code->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(code->currentAddress));
	auto whileStatement = findCycleParent();
	instruction->Operands.push_back(whileStatement->statementStartAddress);
}

void WhileStatement::generateCode(ProgramCode_Ptr code)
{
	falseJmpAddress = statementEndAddress;
	statementStartAddress->value = code->currentAddress;
	Condition->generateCode(code);

	Body->generateCode(code);
	Instruction_Ptr instruction = make_shared<JMPInstruction>();
	code->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(code->currentAddress));
	instruction->Operands.push_back(statementStartAddress);
	statementEndAddress->value = code->currentAddress;
}

void IfStatement::generateCode(ProgramCode_Ptr code)
{
	Condition->generateCode(code);
	Body->generateCode(code);
	falseJmpAddress->value = code->currentAddress;

	if (Else != nullptr)
	{
		auto elseEnd = make_shared<InstructionOperand>();
		if (typeid(*code->Instructions.front()) != typeid(JMPInstruction))
		{
			auto instruction = make_shared<JMPInstruction>();
			code->AddInstruction(instruction);
			instruction->Operands.push_back(make_shared<InstructionOperand>(code->currentAddress));
			instruction->Operands.push_back(elseEnd);
			falseJmpAddress->value = code->currentAddress;
		}
		Else->generateCode(code);
		elseEnd->value = code->currentAddress;
	}
}

void ElseIf::generateCode(ProgramCode_Ptr code)
{
	If->generateCode(code);
}

void Else::generateCode(ProgramCode_Ptr code)
{
	ElseBody->generateCode(code);
}

void BinaryExpression::generateCode(ProgramCode_Ptr code)
{
	Left->generateCode(code);
	Instruction_Ptr instruction = make_shared<PushInstruction>();
	code->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(AX));

	Right->generateCode(code);
	instruction = make_shared<PopInstruction>();
	code->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(BX));
	//TODO floats
	//Left->generateCode(code);
	//Instruction_Ptr instruction = make_shared<PushFloatInstruction>(Memory);
	//code->AddInstruction(instruction);
	//instruction->Operands.push_back(make_shared<InstructionOperand>(XX_SI));

	//Right->generateCode(code);
	//instruction = make_shared<PushFloatInstruction>(Memory);
	//code->AddInstruction(instruction);
	//instruction->Operands.push_back(make_shared<InstructionOperand>(XX_SI));
}

void MultiplicationExpression::generateCode(ProgramCode_Ptr code)
{
	//TODO: [possible optimization] then working with contants use constant
	BinaryExpression::generateCode(code);
	auto instruction = make_shared<MultInstruction>();
	code->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(BX));
	//else
	//{
	//	auto instruction = make_shared<MulFloatInstruction>(Memory);
	//	code->AddInstruction(instruction);
	//}
}

void AddExpression::generateCode(ProgramCode_Ptr code)
{
	//TODO: [possible optimization] then working with contants use constant
	BinaryExpression::generateCode(code);
	auto instruction = make_shared<AddInstruction>();
	code->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(BX));
	//TODO: add floats
}

void CompExpression::generateCode(ProgramCode_Ptr code)
{
	//TODO: compare chars
	if (Right->Type == BoolType)
	{
		Left->generateCode(code);
		auto usedRegister = make_shared<InstructionOperand>(AX);
		Instruction_Ptr instruction = make_shared<MovWordConstInstruction>(Register);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(usedRegister);
		instruction->Operands.push_back(make_shared<InstructionOperand>(1));
		
		instruction = make_shared<PushInstruction>();
		code->AddInstruction(instruction);
		instruction->Operands.push_back(usedRegister);

		instruction = make_shared<JMPInstruction>();
		code->AddInstruction(instruction);
		instruction->Operands.push_back(make_shared<InstructionOperand>(code->currentAddress));
		falseJmpAddress->value = code->currentAddress;
		auto jumpOutOp = make_shared<InstructionOperand>();
		instruction->Operands.push_back(jumpOutOp);

		instruction = make_shared<MovWordConstInstruction>(Register);
		code->AddInstruction(instruction);
		jumpOutOp->value = code->currentAddress;
		instruction->Operands.push_back(usedRegister);
		instruction->Operands.push_back(make_shared<InstructionOperand>(0));

		instruction = make_shared<PushInstruction>();
		code->AddInstruction(instruction);
		instruction->Operands.push_back(usedRegister);

		falseJmpAddress = make_shared<InstructionOperand>();
		Right->generateCode(code);
		instruction = make_shared<MovWordConstInstruction>(Register);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(usedRegister);
		instruction->Operands.push_back(make_shared<InstructionOperand>(1));

		instruction = make_shared<JMPInstruction>();
		code->AddInstruction(instruction);
		instruction->Operands.push_back(make_shared<InstructionOperand>(code->currentAddress));
		falseJmpAddress->value = code->currentAddress;
		jumpOutOp = make_shared<InstructionOperand>();
		instruction->Operands.push_back(jumpOutOp);

		instruction = make_shared<MovWordConstInstruction>(Register);
		code->AddInstruction(instruction);
		jumpOutOp->value = code->currentAddress;
		instruction->Operands.push_back(usedRegister);
		instruction->Operands.push_back(make_shared<InstructionOperand>(0));
		
		instruction = make_shared<PopInstruction>();
		code->AddInstruction(instruction);
		instruction->Operands.push_back(make_shared<InstructionOperand>(BX));
	}
	else
	{
		BinaryExpression::generateCode(code);
	}

	Instruction_Ptr instruction = make_shared<CmpWordInstruction>();
	code->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(AX));
	instruction->Operands.push_back(make_shared<InstructionOperand>(BX));

	bool doInvert = findNotParent() != nullptr;
	if (typeid(*Operator) == typeid(OpEq))
	{
		if (doInvert)
			instruction = make_shared<CondJmpInstruction>(JE);
		else
			instruction = make_shared<CondJmpInstruction>(JNE);
	}
	else if (typeid(*Operator) == typeid(OpNotEq))
	{
		if (doInvert)
			instruction = make_shared<CondJmpInstruction>(JNE);
		else
			instruction = make_shared<CondJmpInstruction>(JE);
	}
	else if (typeid(*Operator) == typeid(OpLess))
	{
		if (doInvert)
			instruction = make_shared<CondJmpInstruction>(JL);
		else
			instruction = make_shared<CondJmpInstruction>(JGE);
	}
	else if (typeid(*Operator) == typeid(OpLessEq))
	{
		if (doInvert)
			instruction = make_shared<CondJmpInstruction>(JLE);
		else
			instruction = make_shared<CondJmpInstruction>(JG);
	}
	else if (typeid(*Operator) == typeid(OpMore))
	{
		if (doInvert)
			instruction = make_shared<CondJmpInstruction>(JG);
		else
			instruction = make_shared<CondJmpInstruction>(JLE);
	}
	else
	{
		if (doInvert)
			instruction = make_shared<CondJmpInstruction>(JGE);
		else
			instruction = make_shared<CondJmpInstruction>(JL);
	}

	code->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(code->currentAddress));
	auto jmpFalseLoc = make_shared<InstructionOperand>();
	instruction->Operands.push_back(jmpFalseLoc);

	instruction = make_shared<JMPInstruction>();
	code->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(code->currentAddress));
	auto trueLoc = make_shared<InstructionOperand>();
	instruction->Operands.push_back(trueLoc);

	jmpFalseLoc->value = code->currentAddress;
	instruction = make_shared<JMPInstruction>();
	code->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(code->currentAddress));
	auto condParent = findConditionalNodeParent();
	instruction->Operands.push_back(condParent->falseJmpAddress);
	trueLoc->value = code->currentAddress;
}

void or_generate_code(BinaryExpression* orExpression, ConditionalNode* condNode, ProgramCode_Ptr code)
{
	orExpression->Left->generateCode(code);
	auto instruction = make_shared<JMPInstruction>(); //jump to the end of or
	code->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(code->currentAddress));
	auto orEndOp = make_shared<InstructionOperand>();
	instruction->Operands.push_back(orEndOp);
	condNode->falseJmpAddress->value = code->currentAddress;

	auto condParent = orExpression->findConditionalNodeParent();
	condNode->falseJmpAddress = condParent->falseJmpAddress;
	orExpression->Right->generateCode(code);
	orEndOp->value = code->currentAddress;
}

void OrExpression::generateCode(ProgramCode_Ptr code)
{
	
	if (findNotParent() != nullptr)
	{
		Left->generateCode(code);
		Right->generateCode(code);
	}
	else
	{
		or_generate_code(this, this, code);
	}
}

void AndExpression::generateCode(ProgramCode_Ptr code)
{
	falseJmpAddress = findConditionalNodeParent()->falseJmpAddress;
	if(findNotParent() != nullptr)
	{
		or_generate_code(this, this, code);
	}
	else
	{
		Left->generateCode(code);
		Right->generateCode(code);
	}
}

void IdentifierExpression::setData(ProgramCode_Ptr code, ExpressionType type)
{
	AccessRegister = XX_BP;
	AccessMod = MemoryRelative;
	Offset = make_shared<InstructionOperand>();
	auto variable = code->functionVariables[Value];
	Offset->value = variable->relativeAddress;
}

void IdentifierExpression::generateCode(ProgramCode_Ptr code)
{
	setData(code, Type);
	auto mod = (Mod)AccessMod;
	auto reg = make_shared<InstructionOperand>(AccessRegister);
	Instruction_Ptr instruction;
	if (Type == BoolType)
	{
		auto condParent = findConditionalNodeParent();
		instruction = make_shared<MovByteInstruction>(mod, false);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(reg);
		if (Offset != nullptr)
			instruction->Operands.push_back(Offset);
		instruction->Operands.push_back(make_shared<InstructionOperand>(AL));

		instruction = make_shared<TestByteInstruction>();
		code->AddInstruction(instruction);
		auto operand = make_shared<InstructionOperand>(AL);
		instruction->Operands.push_back(operand);
		instruction->Operands.push_back(operand);

		if (findNotParent() != nullptr)
			instruction = make_shared<CondJmpInstruction>(JNE);
		else
			instruction = make_shared<CondJmpInstruction>(JE);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(make_shared<InstructionOperand>(code->currentAddress));
		instruction->Operands.push_back(condParent->falseJmpAddress);
	}
	else if (Type == CharType)
	{
		instruction = make_shared<MovByteInstruction>(mod, false);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(reg);
		if (Offset != nullptr)
			instruction->Operands.push_back(Offset);
		instruction->Operands.push_back(make_shared<InstructionOperand>(AL));

		instruction = make_shared<MovByteConstInstruction>(Register);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(make_shared<InstructionOperand>(AH));
		instruction->Operands.push_back(make_shared<InstructionOperand>(0));
	}
	else
	{
		instruction = make_shared<MovWordInstruction>(mod, false);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(reg);
		if (Offset != nullptr)
			instruction->Operands.push_back(Offset);
		instruction->Operands.push_back(make_shared<InstructionOperand>(AX));
	}
}

void MemberAccessExpression::setData(ProgramCode_Ptr code, ExpressionType type)
{
	AccessRegister = BX_SI;
	AccessMod = Memory;

	//save data to not destroy
	auto ax_register = make_shared<InstructionOperand>(AX);
	Instruction_Ptr instruction = make_shared<PushInstruction>();
	code->AddInstruction(instruction);
	instruction->Operands.push_back(ax_register);

	AccessPosition->generateCode(code);
	int typeSize = GetTypeSize(type);
	if (typeSize != 1)
	{
		auto bx_register = make_shared<InstructionOperand>(BX);
		instruction = make_shared<MovWordConstInstruction>(Register);
		code->AddInstruction(instruction);
		instruction->Operands.push_back(bx_register);
		instruction->Operands.push_back(make_shared<InstructionOperand>(typeSize));

		instruction = make_shared<MultInstruction>();
		code->AddInstruction(instruction);
		instruction->Operands.push_back(bx_register);
	}
	instruction = make_shared<MovWordInstruction>(Register);
	code->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(SI));
	instruction->Operands.push_back(ax_register);
		
	instruction = make_shared<MovWordInstruction>(MemoryRelative, false);
	code->AddInstruction(instruction);
	Offset = nullptr;
	auto variable = code->functionVariables[Value];
	instruction->Operands.push_back(make_shared<InstructionOperand>(XX_BP));
	instruction->Operands.push_back(make_shared<InstructionOperand>(variable->relativeAddress));
	instruction->Operands.push_back(make_shared<InstructionOperand>(BX));

	//restore data
	instruction = make_shared<PopInstruction>();
	code->AddInstruction(instruction);
	instruction->Operands.push_back(ax_register);
}

void FunctionCallExpression::generateCode(ProgramCode_Ptr code)
{
	for (auto param : Parameters->Parameters)
	{
		param->Expression->generateCode(code);
		Instruction_Ptr instruction = make_shared<PushInstruction>();
		code->AddInstruction(instruction);
		instruction->Operands.push_back(make_shared<InstructionOperand>(AX));
	}
	Instruction_Ptr instruction = make_shared<CallInstruction>();
	code->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(code->currentAddress));
	instruction->Operands.push_back(code->functions[Name->Value]->functionStartAddress);

	auto condParent = findConditionalNodeParent();
	if (Type == BoolType && condParent != nullptr)
	{
		instruction = make_shared<TestWordInstruction>();
		code->AddInstruction(instruction);
		instruction->Operands.push_back(make_shared<InstructionOperand>(AX));
		instruction->Operands.push_back(make_shared<InstructionOperand>(AX));

		Instruction_Ptr jmpInstruction;
		if (findNotParent() != nullptr)
			jmpInstruction = make_shared<CondJmpInstruction>(JNE);
		else
			jmpInstruction = make_shared<CondJmpInstruction>(JE);
		code->AddInstruction(jmpInstruction);
		jmpInstruction->Operands.push_back(make_shared<InstructionOperand>(code->currentAddress));
		jmpInstruction->Operands.push_back(condParent->falseJmpAddress);
	}
}

void StringExpression::generateCode(ProgramCode_Ptr code)
{
	auto string = make_shared<StringConstant>(Value);
	code->StringConstants.push_back(string);

	auto instruction = make_shared<MovWordConstInstruction>(Register);
	code->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(AX));
	instruction->Operands.push_back(string);
}

void CharExpression::generateCode(ProgramCode_Ptr code)
{
	auto instruction = make_shared<MovWordConstInstruction>(Register);
	code->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(AX));
	instruction->Operands.push_back(make_shared<InstructionOperand>(Value));
}

void IntegerExpression::generateCode(ProgramCode_Ptr code)
{
	auto instruction = make_shared<MovWordConstInstruction>(Register);
	code->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(AX));
	instruction->Operands.push_back(make_shared<InstructionOperand>(Value));
}

void FloatExpression::generateCode(ProgramCode_Ptr code)
{
	//TODO: floats
	//auto floatAddress = make_shared<FloatConstant>(Value);
	//code->FloatConstants.push_back(floatAddress);

	//auto instruction = make_shared<MovWordConstInstruction>(Register);
	//code->AddInstruction(instruction);
	//instruction->Operands.push_back(make_shared<InstructionOperand>(SI));
	//instruction->Operands.push_back(floatAddress);
}

void generate_false(ProgramCode_Ptr code, ConditionalNode* condNode)
{
	auto instruction = make_shared<JMPInstruction>();
	code->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(code->currentAddress));
	instruction->Operands.push_back(condNode->falseJmpAddress);
}

void TrueExpresion::generateCode(ProgramCode_Ptr code)
{
	if (findNotParent() != nullptr)
		generate_false(code, findConditionalNodeParent());
}

void FalseExpresion::generateCode(ProgramCode_Ptr code)
{
	if (findNotParent() == nullptr)
		generate_false(code, findConditionalNodeParent());
}

void GroupedExpression::generateCode(ProgramCode_Ptr code)
{
	Expression->generateCode(code);
}

void UnaryExpression::generateCode(ProgramCode_Ptr code)
{
	Expression->generateCode(code);
}

void SubExpression::generateCode(ProgramCode_Ptr code)
{
	UnaryExpression::generateCode(code);
	auto instruction = make_shared<NegInstruction>();
	code->AddInstruction(instruction);
	instruction->Operands.push_back(make_shared<InstructionOperand>(AX));
}

void NotExpression::generateCode(ProgramCode_Ptr code)
{
	Expression->generateCode(code);
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("Incorect number of arguments");
		return -1;
	}

	fileName = argv[1];
	vector<Lexem_ptr> lexems = lex_all(fileName);
	//print_lexems(lexems);
	Parser parser = Parser();
	SyntaxTree_Ptr tree = parser.build_syntax_tree(lexems);
	parser.print_tree(tree);

	Scope_Ptr programScope = make_shared<Scope>();
	tree->resolveNames(programScope);
	if (errorsCount > 0)
	{
		printf("Program cannot be compiled. Errors counted: %d\n", errorsCount);
		return -1;
	}

	ProgramCode_Ptr code = make_shared<ProgramCode>();
	try {
		tree->generateCode(code);
	}
	catch (exception)
	{

	}

	code->Print();

	return 0;
}