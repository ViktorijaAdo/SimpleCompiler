#pragma once

#include "../ErrorsHandler/ErrorHandler.h"
#include "pch.h"

enum LexemType
{
	Ident,
	Op_noteq,
	Op_print,
	Op_less,
	Op_lesseq,
	Op_moreeq,
	Op_scan,
	Op_more,
	Op_assign,
	Op_eq,
	String,
	Char,
	Numbers,
	Op_add,
	Op_mult,
	Op_minus,
	Paran_open,
	Paran_close,
	Seperator,
	Statement_end,
	Block_start,
	Block_end,
	Op_and,
	Op_or,
	Op_not,
	Const_true,
	Const_false,
	Bool_type,
	Integer_type,
	String_type,
	Char_type,
	Float_type,
	Void_type,
	If_keyword,
	Else_keyword,
	Return_keyword,
	While_keyword,
	Break_keyword,
	Continue_keyword,
	Print_keyword,
	Scan_keyword,
	Float,
	SQBracket_open,
	SQBracket_close,
	EndOfFile
};

struct Lexem
{
	LexemType type;
	int lineNumber;
	int value_i;
	std::string value_s;
	double value_f;
};
typedef std::shared_ptr<Lexem> Lexem_ptr;

std::vector<Lexem_ptr> lex_all(std::string fileName);
void print_lexems(std::vector<Lexem_ptr> lexems);
Lexem_ptr lex_next(char nextChar);
Lexem_ptr finish_lexem(LexemType type, bool backtrack = false);
Lexem_ptr finish_ident();
void print_error(const char* errorText, const char* fileName);