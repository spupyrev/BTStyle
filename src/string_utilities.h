#pragma once

#include <string>
#include <vector>
#include <sstream>

using namespace std;

namespace string_utilities {

bool startsWith(const string& s, const string& prefix);
string trim(const string& line);
string replace(const string& s, const string& search, const string& replace);
vector<string> split(const string& s, const string& c); 
bool isInteger(const string& s); 
string unquote(const string& s, string& openQ, string& closeQ); 
string unquote(const string& s); 
string to_lower(const string& s); 

template <typename T> 
string to_string(const T& n)
{
	ostringstream ss;
    ss << n;
    return ss.str();
}

template <typename T>
class vector_of_type
{
private:
	vector<T> v;
public:
    vector_of_type(const T& r) 
	{
		(*this)(move(r));
	}
    vector_of_type& operator()(const T& r) 
	{
		v.push_back(move(r));
        return *this;
    }
    vector<T>&& operator()() 
	{
		return move(v);
    }
};

typedef vector_of_type<string> vector_of_strings;
typedef vector_of_type<int> vector_of_ints;

}; // namespace string_utilities

