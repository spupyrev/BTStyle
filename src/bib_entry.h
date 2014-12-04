#pragma once

#include <string>
#include <vector>
#include <set>
#include <map>

using namespace std;

class Author
{
public:
	string first;
	string last;
	string von;
};

class BibEntry
{
	friend class BibParser;
	friend class BibDatabase;

	string type;
	string key;
	map<string, string> fields;

	mutable vector<Author> authors;

private:
	BibEntry(const BibEntry&);
	BibEntry& operator = (const BibEntry&);

	vector<Author> ParseAuthors() const;
	Author ParseAuthor(const string& s) const;

public:
	BibEntry(const string& type, const string& key): type(type), key(key) {}
	~BibEntry() {}

	set<string> getFields() const;
	string getYear() const;
	vector<Author> getAuthors() const;

// static section
private:
	static const string TAG_ORDER[];
	static const map<string, vector<string> > REQUIRED_FIELDS;
	static map<string, vector<string> > InitRequiredField();

public:
	static bool IsValidEntry(const string& type);
	static vector<string> GetRequiredFields(const string& type);

	static bool TagComparator(const string& s1, const string& s2);
	static bool AuthorComparator(const BibEntry* s1, const BibEntry* s2);
	static bool YearAscComparator(const BibEntry* s1, const BibEntry* s2);
	static bool YearDescComparator(const BibEntry* s1, const BibEntry* s2);
};

class BibAbbrv
{
	friend class BibParser;

	string tag;
	string value;

private:
	BibAbbrv(const BibAbbrv&);
	BibAbbrv& operator = (const BibAbbrv&);

public:
	BibAbbrv(const string& tag, const string& value): tag(tag), value(value) {}
	~BibAbbrv() {}
};

class BibComment
{
	friend class BibParser;

	string content;

private:
	BibComment(const BibComment&);
	BibComment& operator = (const BibComment&);

public:
	BibComment(const string& content): content(content) {}
	~BibComment() {}
};

class BibPreamble
{
	friend class BibParser;

	string content;

private:
	BibPreamble(const BibPreamble&);
	BibPreamble& operator = (const BibPreamble&);

public:
	BibPreamble(const string& content): content(content) {}
	~BibPreamble() {}
};

