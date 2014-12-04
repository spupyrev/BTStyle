#pragma once

#include <string>

using namespace std;

class Logger
{
private:
	Logger() {};

	static void SetColor(int color);

	enum LEVEL {debug = 0, info = 1, warning = 2, error = 3};
	static LEVEL logLevel;

public:
	static int warningCount;

	static void SetLogLevel(const string& level);

	static void Error(bool condition, const string& msg);
	static void Error(const string& msg);

	static void Warning(const string& msg);
	static void Warning(bool condition, const string& msg);

	static void Debug(const string& msg);
	static void Debug(bool condition, const string& msg);

	static void Info(const string& msg);
};
