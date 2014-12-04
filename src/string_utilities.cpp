#include "string_utilities.h"

#include <sstream>

namespace string_utilities {

string trim(const string& line)
{
	if (line.length() == 0) return line;

	int i = 0;
	while (i < (int)line.length())
	{
		if (line[i] == ' ' || line[i] == '\n' || line[i] == '\t' || line[i] == '\r')
		{
			i++;
			continue;
		}
		break;
	}

	int j = (int)line.length() - 1;
	while (j >= 0)
	{
		if (line[j] == ' ' || line[j] == '\n' || line[j] == '\t' || line[j] == '\r')
		{
			j--;
			continue;
		}
		break;
	}

	if (i > j) return "";
	return line.substr(i, j - i + 1);
}

string replace(const string& s, const string& search, const string& replace) 
{
	string res = s;
    size_t pos = 0;
    while ((pos = res.find(search, pos)) != string::npos) 
	{
         res.replace(pos, search.length(), replace);
    }
    return res;
}

string unquote(const string& s, string& openQ, string& closeQ)
{
	openQ = closeQ = "";
	int len = s.length();
	if (len <= 1) return s;

	if (s[0] == '{' && s[len - 1] == '}')
	{
		openQ = '{';
		closeQ = '}';
		return s.substr(1, len - 2);
	}

	if (s[0] == '"' && s[len - 1] == '"')
	{
		openQ = '"';
		closeQ = '"';
		return s.substr(1, len - 2);
	}

	return s;
}

string unquote(const string& s)
{
	string openQ, closeQ;
	return unquote(s, openQ, closeQ);
}

vector<string> split(const string& ss, const string& c)
{
	string s = ss + c;
	vector<string> result;
	string tec = "";
	for (int i = 0; i < (int)s.length(); i++)
	{
		if (c.find(s[i]) != string::npos)
		{
			if ((int)tec.length() > 0) result.push_back(tec);
			tec = "";
		}
		else tec += s[i];
	}

	return result;
}

bool isInteger(const string& s)
{
	int n;
	stringstream ss(s);
	ss >> n;
	if (!ss) return false;
	return (ss.rdbuf()->in_avail() == 0);
	//return all_of(s.begin(), s.end(), ::isdigit);
}

};