#include "pch.h"
#include "ErrorHandler.h"

int errorsCount = 0;
std::string fileName = "";
void ErrorHandler::printError(int lineNumber, const char* errorMessage, int lineOffset)
{
	errorsCount++;
	if (lineOffset != 0)
		fprintf(stderr, "%s:%d:%d error: %s\n", fileName.c_str(), lineNumber, lineOffset, errorMessage);
	else
		fprintf(stderr, "%s:%d: error: %s\n", fileName.c_str(), lineNumber, errorMessage);
}