#pragma once

#include <stdio.h>
#include <string.h>

struct ErrorHandler
{
	static void printError(int lineNumber, const char* errorMessage, int lineOffset = 0);
};

extern std::string fileName;
extern int errorsCount;