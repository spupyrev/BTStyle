#pragma once

#include <string>
#include <vector>
#include <sstream>

using namespace std;

namespace string_utilities {

string trim(const string& line);
string replace(const string& s, const string& search, const string& replace);
vector<string> split(const string& s, const string& c); 
bool isInteger(const string& s); 
string unquote(const string& s, string& openQ, string& closeQ); 
string unquote(const string& s); 

template <typename T> 
string to_string(const T& n)
{
	ostringstream ss;
    ss << n;
    return ss.str();
}

class vector_of_strings 
{
private:
	vector<string> v;
public:
    vector_of_strings(const string& r) 
	{
		(*this)(move(r));
	}
    vector_of_strings& operator()(const string& r) 
	{
		v.push_back(move(r));
        return *this;
    }
    vector<string>&& operator()() 
	{
		return move(v);
    }
};

}; // namespace string_utilities

