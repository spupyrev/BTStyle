#include "bib_parser.h"
#include "logger.h"
#include "string_utilities.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <algorithm>
#include <cassert>

using namespace string_utilities;

void BibParser::Read(const string& filename, BibDatabase& db) const
{
	if (filename != "")
	{
		ifstream fileStream;
		fileStream.open(filename.c_str(), ios::in);
		Logger::Error(fileStream != 0, "can't open input file '" + filename + "'");
		Read(fileStream, db);
		fileStream.close();

		db.inputFilename = filename;
	}
	else
	{
		Read(cin, db);
	}
}

void BibParser::Read(istream& is, BibDatabase& db) const
{
	int bytes = ParseItems(is, db);
	db.inputFilesize = bytes;
}

int BibParser::ParseItems(istream& is, BibDatabase& info) const
{
	char ch;
	string s = "";
	int brCount = 0;
	int lineNumber = 1;
	int symbolCount = 0;
	while (is.get(ch))
	{
		s += ch;
		symbolCount += sizeof(ch);
		if (ch == '\n') lineNumber++;

		if (ch == '{')
		{
			brCount++;
		}
		else if (ch == '}')
		{
			Logger::Error(brCount >= 1, "curly braces do not match at line " + to_string(lineNumber));

			brCount--;
			if (brCount == 0)
			{
				ParseItem(trim(s), info);
				s = "";
			}
		}
	}

	Logger::Error(brCount == 0, "curly braces do not match at line " + to_string(lineNumber));
	return symbolCount;
}

void BibParser::ParseItem(const string& s, BibDatabase& info) const
{
	if (s.length() == 0) return;

	if (s[0] != '@')
	{
		int startIndex = s.find('@');
		string ignored = s.substr(0, startIndex);
		Logger::Warning("unrecognized content '" + trim(ignored) + "'");
		ParseItem(trim(s.substr(startIndex)), info);
		return;
	}

	string type, content;
	ParseTypeContent(s, type, content);

	if (type == "string")
	{
		vector<string> kv = SplitTags(content);
		Logger::Error((int)kv.size() == 1, "invalid string abbreviation '" + s + "'");

		string tag, value;
		ParseTag("@string", kv[0], tag, value);

		BibAbbrv* abbrv = new BibAbbrv(tag, value);
		info.abbrv.push_back(abbrv);
	}
	else if (type == "comment")
	{
		BibComment* com = new BibComment(content);
		info.comments.push_back(com);
	}
	else if (type == "preamble")
	{
		BibPreamble* pr = new BibPreamble(content);
		info.preambles.push_back(pr);
	}
	else if (BibEntry::IsValidEntry(type))
	{
		vector<string> kv = SplitTags(content);
		Logger::Error((int)kv.size() >= 1, "invalid entry '" + s + "'");

		string key = kv[0];
		BibEntry* entry = new BibEntry(type, key);
		for (int i = 1; i < (int)kv.size(); i++)
		{
			string tag, value;
			ParseTag(key, kv[i], tag, value);

			Logger::Warning(!entry->fields.count(tag), "duplicate field '" + tag + "' in " + key );
			entry->fields[tag] = value;
		}
		info.entries.push_back(entry);
	}
	else
	{
		Logger::Warning("ignoring unrecognized entry `" + type + "'");
	}
}

void BibParser::ParseTypeContent(const string& s, string& type, string& content) const
{
	assert(s[0] == '@');

	int firstBracket = s.find('{');
	int lastBracket = s.find_last_of('}');

	type = trim(s.substr(1, firstBracket - 1));
	transform(type.begin(), type.end(), type.begin(), ::tolower);

	content = trim(s.substr(firstBracket + 1, lastBracket - firstBracket - 1));
}

vector<string> BibParser::SplitTags(const string& s) const
{
	vector<string> result;

	string cur;
	bool insideQuotes = false;
	char quoteSymbol = 0;
	int brCount = 0;
	for (int i = 0; i < (int)s.length(); i++)
	{
		if (s[i] == '{') brCount++;
		if (s[i] == '}') brCount--;

		if (!insideQuotes && (s[i] == '{' || s[i] == '"'))
		{
			insideQuotes = true;
			cur += s[i];
			quoteSymbol = s[i];
			continue;
		}

		// {
		if (insideQuotes && quoteSymbol == '{' && brCount == 0)
		{
			insideQuotes = false;
			cur += s[i];
			continue;
		}

		// "
		if (insideQuotes && quoteSymbol == '"' && s[i] == '"' && brCount == 0)
		{
			insideQuotes = false;
			cur += s[i];
			continue;
		}

		if (!insideQuotes && s[i] == ',')
		{
			Logger::Error(brCount == 0, "curly braces do not match in '" + s + "'");

			cur = trim(cur);
			if (cur.length() > 0) 
				result.push_back(cur);
			cur = "";
			continue;
		}

		cur += s[i];
	}

	//process last
	Logger::Error(brCount == 0, "curly braces do not match in '" + s + "'");
	Logger::Error(!insideQuotes, "invalid quotes in '" + s + "'");
	cur = trim(cur);
	if (cur.length() > 0) 
		result.push_back(cur);

	return result;
}

void BibParser::ParseTag(const string& key, const string& s, string& tag, string& value) const
{
	size_t equalIndex = s.find('=');
	Logger::Error(equalIndex != string::npos, "inavlid field '" + s + "' in " + key);

	tag = trim(s.substr(0, equalIndex));
	transform(tag.begin(), tag.end(), tag.begin(), ::tolower);

	value = trim(s.substr(equalIndex + 1));

	// check quotes
	int len = value.length();
	Logger::Error(len >= 1, "inavlid field '" + s + "' in " + key);

	if (value[0] == '{' && value[len - 1] == '}')
	{
		value = '{' + trim(value.substr(1, len - 2)) + '}';
	}
	else if (value[0] == '"' && value[len - 1] == '"')
	{
		value = '"' + trim(value.substr(1, len - 2)) + '"';
	}
}

void BibParser::Write(const string& filename, const BibDatabase& db) const
{
	if (filename != "")
	{
		if (db.inputFilename == filename)
		{
			ifstream src(filename.c_str(), ios::binary);
			ofstream dst((filename + ".orig").c_str(), ios::binary);
			dst << src.rdbuf();
		}

		ofstream fileStream;
		fileStream.open(filename.c_str(), ios::out);
		Logger::Error(fileStream != 0, "can't create output file '" + filename + "'");
		Write(fileStream, db);
		fileStream.close();
	}
	else
	{
		Write(cout, db);
	}
}

void BibParser::Write(ostream& os, const BibDatabase& db) const
{
	if (!db.preambles.empty())
	{
		for (auto i : db.preambles)
			Write(os, i);
	
		os << endl << endl;
	}

	if (!db.abbrv.empty())
	{
		for (auto i : db.abbrv)
			Write(os, i);
	
		os << endl << endl;
	}

	if (!db.entries.empty())
	{
		for (auto i : db.entries)
			Write(os, i);
	}

	if (!db.comments.empty())
	{
		os << endl;

		for (auto i : db.comments)
			Write(os, i);
	}
}

void BibParser::Write(ostream& os, const BibAbbrv* abbrv) const
{
	os << "@string ";
	os << "{" << abbrv->tag << " = " << abbrv->value << "}" << endl;
}

void BibParser::Write(ostream& os, const BibComment* com) const
{
	os << "@comment ";
	os << "{" << com->content << "}" << endl;
}

void BibParser::Write(ostream& os, const BibPreamble* pr) const
{
	os << "@preamble ";
	os << "{" << pr->content << "}" << endl;
}

void BibParser::Write(ostream& os, const BibEntry* entry) const
{
	os << "@" << entry->type << " ";
	os << "{" << entry->key << "," << endl;

	vector<string> tags;
	for (auto i : entry->fields)
		tags.push_back(i.first);

	sort(tags.begin(), tags.end(), BibEntry::TagComparator);

	for (auto tag : tags)
	{
		string value = entry->fields.find(tag)->second;
		os << "  " << setw(12) << left << tag << " = " << value << "," << endl;
	}

	os << "}" << endl << endl;
}
