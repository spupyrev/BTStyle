#pragma once

#include <string>
#include <memory>

#include "bib_database.h"

using namespace std;

class BibParser
{
	BibParser(const BibParser&);
	BibParser& operator = (const BibParser&);
	BibParser() {}

public:
	static unique_ptr<BibParser> Create()
	{
		return unique_ptr<BibParser>(new BibParser());
	}

	void Read(const string& filename, BibDatabase& db) const;
	void Write(const string& filename, const BibDatabase& db) const;

	BibEntry* ParseBibEntry(const string& s) const;

private:
	//reading
	void Read(istream& is, BibDatabase& db) const;
	int ParseItems(istream& is, BibDatabase& info) const;
	void ParseItem(const string& s, BibDatabase& info) const;
	void ParseTypeContent(const string& s, string& type, string& content) const;
	vector<string> SplitTags(const string& s) const;
	void ParseTag(const string& key, const string& s, string& tag, string& value) const;
	
	//writing
	void Write(ostream& os, const BibDatabase& info) const;
	void Write(ostream& os, const BibAbbrv* info) const;
	void Write(ostream& os, const BibComment* info) const;
	void Write(ostream& os, const BibPreamble* info) const;
	void Write(ostream& os, const BibEntry* info) const;
};

