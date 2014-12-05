#pragma once

#include <vector>

#include "bib_entry.h"

using namespace std;

class BibDatabase
{
	friend class BibParser;

	vector<BibEntry*> entries;
	vector<BibAbbrv*> abbrv;
	vector<BibComment*> comments;
	vector<BibPreamble*> preambles;

	int inputFilesize;
	string inputFilename;

	map<string, BibEntry*> keyEntryMap;

private:
	BibDatabase(const BibDatabase&);
	BibDatabase& operator = (const BibDatabase&);

	string GenerateKey(const string& option, const BibEntry* entry, const vector<Author>& authors) const;
	void FixPadding(BibEntry* entry, const string& tag) const;

public:
	BibDatabase();
	~BibDatabase();

	void LogDetails() const;

	void InitKeyEntryMap();
	void InitRefEntries() const;
	void CheckRequiredFields() const;

	void ConvertFieldDelimeters(const string& option) const;
	void ReplaceUnicodeCharacters() const;
	void FixPagesDash() const;
	void FixPadding() const;
	void ConvertKeys(const string& option, const string& texFile);
	void ConvertTexKeys(const string& texFile);
	void SortEntries(const string& option);
	void FormatAuthor(const string& option) const;
};

