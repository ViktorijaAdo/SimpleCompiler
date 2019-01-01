#pragma once

#include "../Lexer/pch.h"
#include "SyntaxParser.h"

struct Scope
{
	static std::vector<Function_Ptr> RezervedFunctions;
	std::shared_ptr<Scope> parentScope = nullptr;

	void Add(std::string name, Node* node);
	Node* Find(std::string name, int lineNumber);

private:
	std::map<std::string, Node*> variables;
}; TO_PTR(Scope)