#pragma once

#include "pch.h"
#include "SemanticCheck.h"

enum WordRegister
{
	AX = 0b000, 
	CX = 0b001,
	DX = 0b010,
	BX = 0b011,
	SP = 0b100,
	BP = 0b101,
	SI = 0b110,
	DI = 0b111
};

enum ByteRegister
{
	AL = 0b000,
	CL = 0b001,
	DL = 0b010,
	BL = 0b011,
	AH = 0b100,
	CH = 0b101,
	DH = 0b110,
	BH = 0b111
};

enum MemoryAddressing
{
	BX_SI = 0b000,
	BX_DI = 0b001,
	BP_SI = 0b010,
	BP_DI = 0b011,
	XX_SI = 0b100,
	XX_DI = 0b101,
	XX_BP = 0b110,
	XX_BX = 0b111
};

enum Mod
{
	Memory = 0,
	MemoryRelative = 0b10000000,
	Register = 0b11000000
};

enum CondJmp
{
	JE = 0x74,
	JNE = 0x75,
	JL = 0x7C,
	JG = 0x7F,
	JLE = 0x7E,
	JGE = 0x7D
};

struct Instruction
{
	std::vector<InstructionOperand_Ptr> Operands;
	int Size = 1;
	virtual std::vector<char> getBytes()
	{
		return Bytes;
	}
	virtual void print(){}

protected:
	std::vector<char> Bytes;

	const char* getWordRegisterName(int reg)
	{
		switch (reg)
		{
		case 0b000:
			return "ax";
		case 0b001:
			return "cx";
		case 0b010:
			return "dx";
		case 0b011:
			return "bx";
		case 0b100:
			return "sp";
		case 0b101:
			return "bp";
		case 0b110:
			return "si";
		case 0b111:
			return "di";
		default:
			return "";
		}
	}

	const char* getByteRegisterName(int reg)
	{
		switch (reg)
		{
		case 0b000:
			return "al";
		case 0b001:
			return "cl";
		case 0b010:
			return "dl";
		case 0b011:
			return "bl";
		case 0b100:
			return "ah";
		case 0b101:
			return "ch";
		case 0b110:
			return "dh";
		case 0b111:
			return "bh";
		default:
			return "";
		}
	}

	const char* getMemoryAddressing(int reg)
	{
		switch (reg)
		{
		case 0b000:
			return "bx+si";
		case 0b001:
			return "bx+di";
		case 0b010:
			return "bp+si";
		case 0b011:
			return "bp+di";
		case 0b100:
			return "si";
		case 0b101:
			return "di";
		case 0b110:
			return "bp";
		case 0b111:
			return "bx";
		default:
			return "";
		}
	}
}; TO_PTR(Instruction)


#pragma region 8086

struct JMPInstruction : Instruction
{
	JMPInstruction()
	{
		Size = 3;
		//1110 1001 posl.j.b. posl.v.b.
		Bytes.push_back(0xE9);
	}
	void print()
	{
		printf("jmp loc_%x", Operands[1]->value);
	}

	std::vector<char> getBytes()
	{
		auto jmpBy = Operands[1]->value - Operands[0]->value;
		Bytes.push_back(jmpBy);
		Bytes.push_back(jmpBy >> 8);
		return Bytes;
	}
}; TO_PTR(JMPInstruction)

struct CondJmpInstruction : Instruction
{
	CondJmpInstruction(CondJmp jmpType)
	{
		JmpType = jmpType;
		Size = 2;
		Bytes.push_back(jmpType);
	}

	void print() override
	{
		std::string command = getJmpCommandName(JmpType);
		command += " loc_%x";
		printf(command.c_str(), Operands[1]->value);
	}

	std::vector<char> getBytes()
	{
		auto jmpBy = Operands[1]->value - Operands[0]->value;
		Bytes.push_back(jmpBy);
		return Bytes;
	}

private:
	CondJmp JmpType;
	const char * getJmpCommandName(CondJmp jmp)
	{
		switch (jmp)
		{
		case JE:
			return "je";
		case JNE:
			return "jne";
		case JL:
			return "jl";
		case JG:
			return "jg";
		case JLE:
			return "jle";
		case JGE:
			return "jge";
		default:
			return "";
		}
	}
};

struct CallInstruction : Instruction
{
	CallInstruction()
	{
		Size = 3;
	}

	void print()
	{
		printf("call loc_%x", Operands[1]->value);
	}

	std::vector<char> getBytes() override
	{
		Bytes.push_back(0b11101000);
		int jmpBy = Operands[1]->value - Operands[0]->value;
		Bytes.push_back(jmpBy);
		Bytes.push_back(jmpBy >> 8);
		return Bytes;
	}

}; TO_PTR(CallInstruction)

struct MovWordConstInstruction : Instruction
{
	MovWordConstInstruction(Mod mod)
	{
		Mod = mod;
		Size = (mod == MemoryRelative) ? 6 : 4;
		//1100 011w mod 000 r/m [poslinkis] bet.op1 [bet.op.2, jei w = 1]
		Bytes.push_back(0b11000111);
	}

	void print()
	{
		if (Mod == MemoryRelative)
		{
			printf("mov [bp%+d], %d", Operands[0]->value, Operands[1]->value);
		}
		else
		{
			std::string command = "mov ";
			command += getWordRegisterName(Operands[0]->value);
			command += ", %02xh";
			printf(command.c_str(), Operands[1]->value);
		}
	}

	std::vector<char> getBytes()
	{
		//1100 011w mod 000 r/m [poslinkis] bet.op1 [bet.op.2, jei w = 1]
		if (Mod == MemoryRelative)
		{
			Bytes.push_back(Mod + XX_BP);
			Bytes.push_back(Operands[0]->value);
			Bytes.push_back(Operands[0]->value >> 8);
		}
		else
		{
			Bytes.push_back(Mod + Operands[0]->value);
		}
		Bytes.push_back(Operands[1]->value);
		Bytes.push_back(Operands[1]->value >> 8);
		return Bytes;
	}

private:
	Mod Mod;
};

struct MovByteConstInstruction : Instruction
{
	MovByteConstInstruction(Mod mod)
	{
		Mod = mod;
		Size = (mod == MemoryRelative) ? 5 : 3;

		//1100 011w mod 000 r/m [poslinkis] bet.op1 [bet.op.2, jei w = 1]
		Bytes.push_back(0b11000110);
	}

	void print() override
	{
		std::string command = "mov ";
		if (Mod == MemoryRelative)
		{
			command += "byte ptr [";
			command += getMemoryAddressing(Operands[0]->value);
			command += "%+d], %d";
			printf(command.c_str(), Operands[1]->value, Operands[2]->value);
		}
		else if (Mod == Memory)
		{
			command += "byte ptr [";
			command += getMemoryAddressing(Operands[0]->value);
			command += "], %d";
			printf(command.c_str(), Operands[1]->value);
		}
		else
		{
			command += getByteRegisterName(Operands[0]->value);
			command += ", %d";
			printf(command.c_str(), Operands[1]->value);
		}
	}

	std::vector<char> getBytes()
	{
		//TODO: support not only bp + %d and register
		////1100 011w mod 000 r/m [poslinkis] bet.op1 [bet.op.2, jei w = 1]
		if (Mod == MemoryRelative)
		{
			Bytes.push_back(Mod + Operands[0]->value);
			Bytes.push_back(Operands[1]->value);
			Bytes.push_back(Operands[1]->value >> 8);
			Bytes.push_back(Operands[2]->value);
		}
		else
		{
			Bytes.push_back(Mod + Operands[0]->value);
			Bytes.push_back(Operands[1]->value);
		}
		
		return Bytes;
	}
private:
	Mod Mod;
};

struct MovWordInstruction : Instruction
{
	MovWordInstruction(Mod mod, bool toMemory = true)
	{
		ToMemory = toMemory;
		Mod = mod;
		Size = (mod == MemoryRelative) ? 4 : 2;
		//1000 10dw mod reg r/m [poslinkis]
		auto byte = 0b10001001;
		if (!ToMemory)
			byte += 0b10;
		Bytes.push_back(byte);

	}

	void print() override
	{
		std::string command = "mov ";
		if (Mod == MemoryRelative)
		{
			if (!ToMemory)
			{
				command += getWordRegisterName(Operands[2]->value);
				command += ", ";
				command += "[";
				command += getMemoryAddressing(Operands[0]->value);
				command += "%+d]";
			}
			else
			{
				command += "[";
				command += getMemoryAddressing(Operands[0]->value);
				command += "%+d],";
				command += getWordRegisterName(Operands[2]->value);
			}
		}
		else if (Mod == Memory)
		{
			if (!ToMemory)
			{
				command += getWordRegisterName(Operands[1]->value);
				command += ", ";
				command += "[";
				command += getMemoryAddressing(Operands[0]->value);
				command += "]";
			}
			else
			{
				command += "[";
				command += getMemoryAddressing(Operands[0]->value);
				command += "],";
				command += getWordRegisterName(Operands[1]->value);
			}
		}
		else
		{
			command += getWordRegisterName(Operands[0]->value);
			command += ", ";
			command += getWordRegisterName(Operands[1]->value);
		}
		printf(command.c_str(), Operands[1]->value);
	}

	std::vector<char> getBytes()
	{
		//1000 10dw mod reg r/m [poslinkis]
		if (Mod == MemoryRelative)
		{
			Bytes.push_back(Mod + Operands[2]->value * 0b1000 + Operands[0]->value);
			Bytes.push_back(Operands[1]->value);
			Bytes.push_back(Operands[1]->value >> 8);
		}
		else
		{
			Bytes.push_back(Mod + Operands[1]->value * 0b1000 + Operands[0]->value);
		}
		return Bytes;
	}
private:
	Mod Mod;
	bool ToMemory;
};

struct LeaInstruction : Instruction
{
	LeaInstruction(Mod mod)
{
	Mod = mod;
	Size = (mod == MemoryRelative) ? 4 : 2;
	//1000 1101 mod reg r/m [poslinkis]
	auto byte = 0b10001101;
	Bytes.push_back(byte);

}

void print() override
{
	std::string command = "lea ";
	if (Mod == MemoryRelative)
	{
			command += getWordRegisterName(Operands[2]->value);
			command += ", ";
			command += "[";
			command += getMemoryAddressing(Operands[0]->value);
			command += "%+d]";
	}
	else if (Mod == Memory)
	{
			command += getWordRegisterName(Operands[1]->value);
			command += ", ";
			command += "[";
			command += getMemoryAddressing(Operands[0]->value);
			command += "]";
	}
	printf(command.c_str(), Operands[1]->value);
}

std::vector<char> getBytes()
{
	//1000 1101 mod reg r/m [poslinkis]
	if (Mod == MemoryRelative)
	{
		Bytes.push_back(Mod + Operands[2]->value * 0b1000 + Operands[0]->value);
		Bytes.push_back(Operands[1]->value);
		Bytes.push_back(Operands[1]->value >> 8);
	}
	else
	{
		Bytes.push_back(Mod + Operands[1]->value * 0b1000 + Operands[0]->value);
	}
	return Bytes;
}
private:
	Mod Mod;
};

struct MovByteInstruction : Instruction
{
	MovByteInstruction(Mod mod, bool toMemory = true)
	{
		ToMemory = toMemory;
		Mod = mod;
		Size = (mod == MemoryRelative) ? 4 : 2;
		//1000 10dw mod reg r/m [poslinkis]
		auto byte = 0b10001000;
		if (!toMemory)
			byte += 0b10;
		Bytes.push_back(byte);

	}

	void print() override
	{
		std::string command = "mov ";
		if (Mod == MemoryRelative)
		{
			if (!ToMemory)
			{
				command += getByteRegisterName(Operands[2]->value);
				command += ", ";
				command += "[";
				command += getMemoryAddressing(Operands[0]->value);
				command += "%+d]";
			}
			else
			{
				command += "[";
				command += getMemoryAddressing(Operands[0]->value);
				command += "%+d], ";
				command += getByteRegisterName(Operands[2]->value);
			}
		}
		else if (Mod == Memory)
		{
			if (!ToMemory)
			{
				command += getByteRegisterName(Operands[1]->value);
				command += ", ";
				command += "[";
				command += getMemoryAddressing(Operands[0]->value);
				command += "]";
			}
			else
			{
				command += "[";
				command += getMemoryAddressing(Operands[0]->value);
				command += "], ";
				command += getByteRegisterName(Operands[1]->value);
			}
		}
		else
		{
			command += getByteRegisterName(Operands[0]->value);
			command += ", ";
			command += getByteRegisterName(Operands[1]->value);
		}
		printf(command.c_str(), Operands[1]->value);
	}

	std::vector<char> getBytes()
	{
		//1000 10dw mod reg r/m [poslinkis]
		if (Mod == MemoryRelative)
		{
			Bytes.push_back(Mod + Operands[2]->value * 0b1000 + Operands[0]->value);
			Bytes.push_back(Operands[1]->value);
			Bytes.push_back(Operands[1]->value >> 8);
		}
		else
		{
			Bytes.push_back(Mod + Operands[1]->value * 0b1000 + Operands[0]->value);
		}
		return Bytes;
	}
private:
	Mod Mod;
	bool ToMemory;
};

struct SubConstInstruction : Instruction
{
	SubConstInstruction()
	{
		//1000 00sw mod 101 r/m [poslinkis] bet.op.j.b. [bet.op.v.b]
		Size = 4;
		Bytes.push_back(0b10000001);
		//sub sp, smth TODO: make more generic
		Bytes.push_back(Register + 0b101000 + SP);
	}
	void print() override
	{
		printf("sub sp, %d", Operands[0]->value);
	}

	std::vector<char> getBytes()
	{
		Bytes.push_back(Operands[0]->value);
		Bytes.push_back(Operands[0]->value >> 8);
		return Bytes;
	}
};

struct InteruptInstruction : Instruction
{
	InteruptInstruction()
	{
		Size = 2;
		Bytes.push_back(0xCD);
	}

	void print() override
	{
		printf("int %xh", Operands[0]->value);
	}

	std::vector<char> getBytes()
	{
		Bytes.push_back(Operands[0]->value);
		return Bytes;
	}
};

struct PushInstruction : Instruction
{
	PushInstruction()
	{
	}

	void print() override
	{
		std::string command = "push ";
		command += getWordRegisterName(Operands[0]->value);
		printf(command.c_str());
	}

	std::vector<char> getBytes()
	{
		//0101 0reg 
		Bytes.push_back(0b01010000 + Operands[0]->value);
		return Bytes;
	}
};

struct PopInstruction : Instruction
{
	PopInstruction()
	{
	}

	void print() override
	{
		std::string command = "pop ";
		command += getWordRegisterName(Operands[0]->value);
		printf(command.c_str());
	}

	std::vector<char> getBytes()
	{
		//01011 reg 
		Bytes.push_back(0b01011000 + Operands[0]->value);
		return Bytes;
	}
};

struct ReturnInstruction : Instruction
{
	ReturnInstruction()
	{
		Size = 3;
		//1100 0010 bet.op.j.b. bet.op.v.b.
		Bytes.push_back(0xC2);
	}

	void print() override
	{
		printf("ret %d", Operands[0]->value);
	}

	std::vector<char> getBytes() override
	{
		Bytes.push_back(Operands[0]->value);
		Bytes.push_back(Operands[0]->value >> 8);
		return Bytes;
	}
};

struct MultInstruction : Instruction
{
	MultInstruction()
	{
		Size = 2;
		//1111 011w mod 100 r/m [poslinkis]
		Bytes.push_back(0xF7);
	}

	void print() override
	{
		std::string command = "mul ";
		command += getWordRegisterName(Operands[0]->value);
		printf(command.c_str());
	}

	std::vector<char> getBytes()
	{
		Bytes.push_back(Register + 0b100000 + Operands[0]->value);
		return Bytes;
	}
};

struct AddInstruction : Instruction
{
	AddInstruction()
	{
		Size = 2;
		//0000 00dw mod reg r/m [poslinkis]
		Bytes.push_back(0b00000011);
	}

	void print() override
	{
		//TODO: support other registers too
		std::string command = "add ax, ";
		command += getWordRegisterName(Operands[0]->value);
		printf(command.c_str());
	}

	std::vector<char> getBytes()
	{
		Bytes.push_back(Register + AX*0b1000 + Operands[0]->value);
		return Bytes;
	}
};

struct CmpWordInstruction : Instruction
{
	CmpWordInstruction()
	{
		Size = 2;
		//0011 10dw mod reg r/m [poslinkis]
		Bytes.push_back(0b00111001);
	}

	void print() override
	{
		std::string command = "cmp ";
		command += getWordRegisterName(Operands[0]->value);
		command += ", ";
		command += getWordRegisterName(Operands[1]->value);
		printf(command.c_str());
	}

	std::vector<char> getBytes()
	{
		Bytes.push_back(Register + Operands[0]->value + Operands[1]->value);
		return Bytes;
	}
};

struct CmpByteInstruction : Instruction
{
	CmpByteInstruction()
	{
		Size = 2;
		//0011 10dw mod reg r/m [poslinkis]
		Bytes.push_back(0x38);
	}

	void print() override
	{
		std::string command = "cmp ";
		command += getByteRegisterName(Operands[0]->value);
		command += ", ";
		command += getByteRegisterName(Operands[1]->value);
		printf(command.c_str());
	}

	std::vector<char> getBytes()
	{
		Bytes.push_back(Register + Operands[0]->value + Operands[1]->value);
		return Bytes;
	}
};

struct TestByteInstruction : Instruction
{
	TestByteInstruction()
	{
		Size = 2;
		//1000 010w mod reg r/m [poslinkis]
		Bytes.push_back(0b10000100);
	}

	void print() override
	{
		std::string command = "test ";
		command += getByteRegisterName(Operands[0]->value);
		command += ", ";
		command += getByteRegisterName(Operands[1]->value);
		printf(command.c_str());
	}

	std::vector<char> getBytes()
	{
		Bytes.push_back(Register + Operands[0]->value + Operands[1]->value);
		return Bytes;
	}
};

struct TestWordInstruction : Instruction
{
	TestWordInstruction()
	{
		Size = 2;
		//1000 010w mod reg r/m [poslinkis]
		Bytes.push_back(0b10000101);
	}

	void print() override
	{
		std::string command = "test ";
		command += getWordRegisterName(Operands[0]->value);
		command += ", ";
		command += getWordRegisterName(Operands[1]->value);
		printf(command.c_str());
	}

	std::vector<char> getBytes()
	{
		Bytes.push_back(Register + Operands[0]->value + Operands[1]->value);
		return Bytes;
	}
};

struct NegInstruction : Instruction
{
	NegInstruction()
	{
		Size = 2;
		//1111 011w mod 011 r/m [poslinkis]
		Bytes.push_back(0xF7);
	}

	void print() override
	{
		std::string command = "neg ";
		command += getWordRegisterName(Operands[0]->value);
		printf(command.c_str());
	}

	std::vector<char> getBytes()
	{
		Bytes.push_back(Register + 0b011000 + Operands[0]->value);
		return Bytes;
	}
};

#pragma endregion

//#pragma region 8087
//
//struct FloatInstruction : Instruction
//{
//	FloatInstruction()
//	{
//		Size = 1;
//		Bytes.push_back(0x9B); //Wait instruction
//	}
//
//	void printCommand(std::string command)
//	{
//		if (CommandMod == MemoryRelative)
//		{
//			command += '[';
//			command += getWordRegisterName(Operands[0]->value);
//			command += "%+d]";
//			printf(command.c_str(), Operands[1]->value);
//		}
//		else
//		{
//			command += '[';
//			command += getWordRegisterName(Operands[0]->value);
//			command += ']';
//			printf(command.c_str());
//		}
//	}
//
//protected:
//	int const ESC = 0b11011000;
//	Mod CommandMod;
//};
//
//struct PushFloatInstruction : FloatInstruction
//{
//	PushFloatInstruction(Mod mod) : FloatInstruction()
//	{
//		CommandMod = mod;
//		Size += mod == MemoryRelative ? 4 : 2;
//	}
//
//	void print() override
//	{
//		printCommand("fld ");
//	}
//
//	std::vector<char> getBytes() override
//	{
//		Bytes.push_back(ESC + 1);
//		Bytes.push_back(CommandMod + 000 + Operands[0]->value);
//		if (CommandMod == MemoryRelative)
//		{
//			Bytes.push_back(Operands[1]->value);
//			Bytes.push_back(Operands[1]->value >> 8);
//		}
//		return Bytes;
//	}
//};
//
//struct MulFloatInstruction : FloatInstruction
//{
//	MulFloatInstruction(Mod mod) : FloatInstruction()
//	{
//		CommandMod = mod;
//		Size += mod == MemoryRelative ? 4 : 2;
//	}
//
//	void print() override
//	{
//		printf("fmulp ST(1)");
//	}
//
//	std::vector<char> getBytes() override
//	{
//		//ESC d P 0 11000 ST(i)
//		Bytes.push_back(ESC + 0b10);
//		Bytes.push_back(0b11000001);
//		if (CommandMod == MemoryRelative)
//		{
//			Bytes.push_back(Operands[1]->value);
//			Bytes.push_back(Operands[1]->value >> 8);
//		}
//		return Bytes;
//	}
//};

#pragma endregion

struct FloatConstant : InstructionOperand
{
	FloatConstant(float value)
	{
		FloatValue = value;
	}
	union
	{
		float FloatValue;
		long BinaryValue;
	};
}; TO_PTR(FloatConstant)

struct StringConstant : InstructionOperand
{
	StringConstant(std::string value)
	{
		StringValue = value;
	}
	std::string StringValue;
}; TO_PTR(StringConstant)

struct ProgramCode
{
	std::vector<Instruction_Ptr> Instructions;
	std::vector<FloatConstant_Ptr> FloatConstants;
	std::vector<StringConstant_Ptr> StringConstants;
	int currentAddress = 0;
	int currentRelativeAddress = 0;
	int paramRelativeAddress = 2;
	std::map<std::string, VariableDeclaration*> functionVariables;
	std::map<std::string, Function*> functions;
	std::vector<char> RezervedFunctionsCode;

	void AddFunction(std::string name, Function* function);
	void AddParam(std::string, VariableDeclaration*);
	void AddVariable(std::string, VariableDeclaration*, int additionalSpace);
	void AddInstruction(Instruction_Ptr);
	void IncludeRezervedFunction(std::string name);

	void Print();
};
