#pragma once

#include <vector>
#include <memory>

#include "bib_entry.h"
#include "dblp/sqlite3.h"

using namespace std;

struct DBLPEntry
{
	static const int MAX_TITLE_SIZE = 256;

	string dblp_key;
	string title;
	string content;
};

class DBLPDatabase
{
	sqlite3* db;

private:
	DBLPDatabase(const DBLPDatabase&);
	DBLPDatabase& operator = (const DBLPDatabase&);
	DBLPDatabase(const string& dbFile);

	void runSQLite(int rc, int ec) const;
	void runSQLite(int rc) const;

	vector<DBLPEntry> findByTitle(const string& title) const;
	DBLPEntry findByKey(const string& key) const;
	BibEntry* CreateBibEntry(const DBLPEntry& dblpEntry, const BibParser& parser) const;
	BibEntry* FilterDBLPEntries(vector<BibEntry*> entries, BibEntry* entry) const;
	void FilterDBLPEntriesByAuthor(vector<BibEntry*>& entries, BibEntry* entry) const;
	void FilterDBLPEntriesByYear(vector<BibEntry*>& entries, BibEntry* entry) const;
	void FilterDBLPEntriesByType(vector<BibEntry*>& entries, BibEntry* entry) const;
	void FixLocalURL(BibEntry* entry, const string& field) const;
	void ExtractDOI(BibEntry* entry) const;
public:
	static unique_ptr<DBLPDatabase> Create(const string& dbFile)
	{
		return unique_ptr<DBLPDatabase>(new DBLPDatabase(dbFile));
	}

	~DBLPDatabase();

	void Sync(BibEntry* entry, const BibParser& parser) const;
	int RowCount() const;

};

