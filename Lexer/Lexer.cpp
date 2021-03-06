#include "pch.h"
#include "Lexer.h"
#include <iostream>
#include <stdio.h>
#include <cctype>
#include <fstream>
#include <sstream>

using namespace std;

enum State
{
	Start,
	Ident_symbol,
	Less_sign,
	More_sign,
	Equal_sign,
	String_symbol,
	Char_start, 
	Character,
	Slash,
	Number,
	Comment_symbol,
	Comment_end,
	FloatScienStart,
	FloatScienNeg,
	FloatStart,
	FloatEndNumber
};

string LexemTypeStrings[] = { 
	"Ident",
	"Op_noteq",
	"Op_print",
	"Op_less",
	"Op_lesseq",
	"Op_moreeq",
	"Op_scan",
	"Op_more",
	"Op_assign",
	"Op_eq",
	"String",
	"Char",
	"Numbers",
	"Op_add",
	"Op_mult",
	"Op_minus",
	"Paran_open",
	"Paran_close",
	"Seperator",
	"Statement_end",
	"Block_start",
	"Block_end",
	"Op_and",
	"Op_or",
	"Op_not",
	"Const_true",
	"Const_false",
	"Bool_type",
	"Integer_type",
	"String_type",
	"Char_type",
	"Float_type",
	"Void_type",
	"If_keyword",
	"Else_keyword",
	"Return_keyword",
	"While_keyword",
	"Break_keyword",
	"Continue_keyword",
	"Print_keyword",
	"Scan_keyword",
	"Float",
	"SQBracket_open",
	"SQBracket_close",
	"EndOfFile"
};

//string input;
int lineNumber = 0;
int startLineNumber = 0;
int lexemsOffset = 0;
State curentState = Start;
string buffer = "";

vector<Lexem_ptr> lex_all(string fileName)
{
	vector<Lexem_ptr> lexems = vector<Lexem_ptr>();
	Lexem_ptr result = NULL;

	ifstream stream(fileName);
	string line = "";
	while (getline(stream, line)) {
		lineNumber++;
		lexemsOffset = 0;

		char current = line[lexemsOffset];
		lexemsOffset++;
		while (current != 0)
		{
			try {
				result = lex_next(current);
			}
			catch (exception error)
			{
				print_error(error.what(), fileName.c_str());
			}
			if (result != NULL)
				lexems.push_back(result);

			current = line[lexemsOffset];
			lexemsOffset++;
		}
		try {
			result = lex_next('\n');
		}
		catch (exception error)
		{
			errorsCount++;
			print_error(error.what(), fileName.c_str());
		}
		if (result != NULL)
			lexems.push_back(result);
	}
	try {
		result = lex_next(-1);
	}
	catch (exception error)
	{
		print_error(error.what(), fileName.c_str());
	}
	if (result != NULL)
		lexems.push_back(result);

	return lexems;
}

void print_error(const char* errorText, const char* fileName)
{
	ErrorHandler::printError(lineNumber, errorText, lexemsOffset);
}

Lexem_ptr lex_next(char nextChar)
{
	switch (curentState)
	{
	case Start:
		startLineNumber = lineNumber;
		if (isalpha(nextChar) || nextChar == '_')
		{
			curentState = Ident_symbol;
			buffer.push_back(nextChar);
		}
		else if (nextChar == '<')
			curentState = Less_sign;
		else if (nextChar == '>')
			curentState = More_sign;
		else if (nextChar == '=')
			curentState = Equal_sign;
		else if (nextChar == '\"')
			curentState = String_symbol;
		else if (nextChar == '\'')
			curentState = Char_start;
		else if (nextChar == '/')
			curentState = Slash;
		else if (isdigit(nextChar))
		{
			curentState = Number;
			buffer.push_back(nextChar);
		}
		else if (nextChar == '+')
			return finish_lexem(Op_add);
		else if (nextChar == '*')
			return finish_lexem(Op_mult);
		else if (nextChar == '-')
			return finish_lexem(Op_minus);
		else if (nextChar == '(')
			return finish_lexem(Paran_open);
		else if (nextChar == ')')
			return finish_lexem(Paran_close);
		else if (nextChar == ',')
			return finish_lexem(Seperator);
		else if (nextChar == ';')
			return finish_lexem(Statement_end);
		else if (nextChar == '{')
			return finish_lexem(Block_start);
		else if (nextChar == '}')
			return finish_lexem(Block_end);
		else if (nextChar == '[')
			return finish_lexem(SQBracket_open);
		else if (nextChar == ']')
			return finish_lexem(SQBracket_close);
		else if (nextChar == -1)
			return finish_lexem(EndOfFile);
		else if (isspace(nextChar))
			return NULL;
		else throw exception("Unexpected char");

		return NULL;
	case Ident_symbol:
		if (isspace(nextChar))
			return finish_ident();
		if (!isalpha(nextChar) && !isdigit(nextChar) && nextChar != '_')
			return finish_ident();

		buffer.push_back(nextChar);
		return NULL;
	case Less_sign:
		if (nextChar == '=')
			return finish_lexem(Op_lesseq);
		if (nextChar == '>')
			return finish_lexem(Op_noteq);
		if (nextChar == '<')
			return finish_lexem(Op_print);
		
		return finish_lexem(Op_less, true);
	case More_sign:
		if (nextChar == '=')
			return finish_lexem(Op_moreeq);
		if (nextChar == '>')
			return finish_lexem(Op_scan);

		return finish_lexem(Op_more, true);
	case Equal_sign:
		if (nextChar == '=')
			return finish_lexem(Op_eq);

		return finish_lexem(Op_assign, true);
	case String_symbol:
	{
		size_t bufferLenght = buffer.length();
		if (bufferLenght >= 1 && buffer[bufferLenght - 1] == '\\')
		{
			if (nextChar == 'n')
				buffer[bufferLenght - 1] = '\n';
			else if (nextChar == 't')
				buffer[bufferLenght - 1] = '\t';
			else if (nextChar == '\"')
				buffer[bufferLenght - 1] = '\"';
			else if (nextChar != '\\')
			{
				buffer.push_back(nextChar);
				throw exception((string("Unknown escaped symbol found \\") + nextChar).c_str());
			}
		}
		else
		{
			if (nextChar == '\"')
				return finish_lexem(String);
			else if (nextChar == -1)
				throw exception("Unterminated string");

			buffer.push_back(nextChar);
		}

		return NULL;
	}
	case Char_start:
		if (nextChar == '\'' || nextChar == -1)
			throw exception("Empty char not allowed");

		curentState = Character;
		buffer.push_back(nextChar);
		return NULL;
	case Character:
	{
		size_t bufferLenght = buffer.length();
		if (bufferLenght >= 1 && buffer[bufferLenght - 1] == '\\')
		{
			if (nextChar == 'n')
				buffer[bufferLenght - 1] = '\n';
			else if (nextChar == 't')
				buffer[bufferLenght - 1] = '\t';
			else if (nextChar == '\"')
				buffer[bufferLenght - 1] = '\"';
			else if (nextChar == '\'')
				buffer[bufferLenght - 1] = '\'';
			else if (nextChar != '\\')
				throw exception((string("Unknown escaped symbol found \\") + nextChar).c_str());
		}
		else
		{
			if (nextChar == '\'')
				return finish_lexem(Char);
			else
			{
				finish_lexem(Char);
				throw exception("Unterminated char");
			}
		}
		return NULL;
	}
	case Slash:
		if (nextChar == '*')
			curentState = Comment_symbol;
		else
			throw exception((string("Expected comment start, but found ") + nextChar).c_str());

		return NULL;
	case Comment_symbol:
		if (nextChar == '*')
			curentState = Comment_end;
		else if(nextChar == -1)
			throw exception("Unterminated comment");

		return NULL;
	case Comment_end:
		if (nextChar == '/')
			curentState = Start;
		else if (nextChar == -1)
			throw exception("Unterminated comment");
		else if (nextChar != '*')
			curentState = Comment_symbol;

		return NULL;
	case Number:
		if (isspace(nextChar))
			return finish_lexem(Numbers);
		if (nextChar == 'E')
			curentState = FloatScienStart;
		else if (nextChar == '.')
			curentState = FloatStart;
		else if (isalpha(nextChar))
			throw exception((string("Unexpected constant prefix ") + nextChar).c_str());
		else if(!isdigit(nextChar))
			return finish_lexem(Numbers, true);

		buffer.push_back(nextChar);
		return NULL;
	case FloatScienStart:
		if (isspace(nextChar))
			return finish_lexem(Float);
		if (nextChar == '-')
			curentState = FloatScienNeg;
		else if (isdigit(nextChar))
			curentState = FloatEndNumber;
		else if (isalpha(nextChar))
			throw exception((string("Unexpected constant prefix ") + nextChar).c_str());
		else if (!isdigit(nextChar))
			return finish_lexem(Float, true);


		buffer.push_back(nextChar);
		return NULL;
	case FloatScienNeg:
	case FloatStart:
		if (isspace(nextChar))
			return finish_lexem(Float);
		if (isalpha(nextChar))
			throw exception((string("Unexpected constant prefix ") + nextChar).c_str());
		else if (!isdigit(nextChar))
			return finish_lexem(Float, true);

		curentState = FloatEndNumber;
		buffer.push_back(nextChar);
		return NULL;
	case FloatEndNumber:
		if (isspace(nextChar))
			return finish_lexem(Float);
		if (nextChar == 'E')
			curentState = FloatScienStart;
		else if (isalpha(nextChar))
			throw exception((string("Unexpected constant prefix ") + nextChar).c_str());
		else if (!isdigit(nextChar))
			return finish_lexem(Float, true);

		buffer.push_back(nextChar);
		return NULL;
	default:
		throw exception("Unexpected state");
	}
}

Lexem_ptr finish_lexem(LexemType type, bool backtrack)
{
	curentState = Start;
	if (backtrack)
		lexemsOffset--;

	Lexem_ptr lexem = make_shared<Lexem>();
	lexem->lineNumber = startLineNumber;
	lexem->type = type;
	switch (type)
	{
	case Numbers:
		lexem->value_i = atoi(buffer.c_str());
		break;
	case Float:
		lexem->value_f = atof(buffer.c_str());
		break;
	default:
		lexem->value_s = buffer;
		break;
	}

	buffer = "";
	return lexem;
}

Lexem_ptr finish_ident()
{
	LexemType lexemType = Ident;
	if (buffer == "and")
		lexemType = Op_and;
	if (buffer == "or")
		lexemType = Op_or;
	if (buffer == "not")
		lexemType = Op_not;
	if (buffer == "true")
		lexemType = Const_true;
	if (buffer == "false")
		lexemType = Const_false;
	if (buffer == "bool")
		lexemType = Bool_type;
	if (buffer == "int")
		lexemType = Integer_type;
	if (buffer == "string")
		lexemType = String_type;
	if (buffer == "char")
		lexemType = Char_type;
	if (buffer == "float")
		lexemType = Float_type;
	if (buffer == "void")
		lexemType = Void_type;
	if (buffer == "if")
		lexemType = If_keyword;
	if (buffer == "else")
		lexemType = Else_keyword;
	if (buffer == "return")
		lexemType = Return_keyword;
	if (buffer == "while")
		lexemType = While_keyword;
	if (buffer == "break")
		lexemType = Break_keyword;
	if (buffer == "continue")
		lexemType = Continue_keyword;
	if (buffer == "print")
		lexemType = Print_keyword;
	if (buffer == "scan")
		lexemType = Scan_keyword;
	if (lexemType != Ident)
		buffer = "";

	return finish_lexem(lexemType, true);
}

void print_lexems(vector<Lexem_ptr> lexems)
{
	printf("%15s | %8s | %s\n", "Lexem type", "Line nr", "Value");
	for (Lexem_ptr lex : lexems)
	{
		printf("%15s | %8d | ", LexemTypeStrings[lex->type].c_str(), lex->lineNumber);
		switch (lex->type)
		{
		case Numbers:
			printf("%d\n", lex->value_i);
			break;
		case Float:
			printf("%f\n", lex->value_f);
			break;
		default:
			printf("%s\n", lex->value_s.c_str());
			break;
		}
	}
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
