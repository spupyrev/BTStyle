#pragma once

#include <string>
#include <map>

using namespace std;

class unicode_latex
{
	static const map<unsigned long, string> substMap;
	static map<unsigned long, string> initMap();

public:
	static string transform(char c1, char c2);
	static string transform(const string& s);
};

