#include "logger.h"

#include <iostream>
#include <cassert>

#if defined _WIN32 || defined __CYGWIN__
#include <windows.h>
#endif

int Logger::warningCount = 0;
Logger::LEVEL Logger::logLevel = info;

void Logger::SetLogLevel(const string& level)
{
	if (level == "debug") logLevel = debug;
	else if (level == "info") logLevel = info;
	else if (level == "warning") logLevel = warning;
	else if (level == "error") logLevel = error;
	else assert(false);
}

void Logger::Error(bool condition, const string& msg)
{
	if (!condition)
		Error(msg);
}

void Logger::Error(const string& msg)
{
	SetColor(12);
	cerr << "Error: " << flush;

	SetColor(7);
	cerr << msg << endl;

	throw 1;
}

void Logger::Warning(bool condition, const string& msg)
{
	if (!condition)
		Warning(msg);
}

void Logger::Warning(const string& msg)
{
	if (logLevel > warning) return;

	warningCount++;

	//SetColor(9);
	SetColor(13);
	cerr << "Warning: "<< flush;

	SetColor(7);
	cerr << msg << endl;
}

void Logger::Debug(bool condition, const string& msg)
{
	if (!condition)
		Debug(msg);
}

void Logger::Debug(const string& msg)
{
	if (logLevel > debug) return;

	SetColor(8);
	cerr << "Debug: "<< flush;

	SetColor(7);
	cerr << msg << endl;
}

void Logger::Info(const string& msg)
{
	if (logLevel > info) return;

	cerr << msg << endl;
}

void Logger::SetColor(int color)
{
#if defined _WIN32 || defined __CYGWIN__
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
	SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), color);
#endif
}
