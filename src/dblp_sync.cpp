#include "bib_database.h"
#include "bib_parser.h"
#include "dblp_database.h"
#include "string_utilities.h"

#include "logger.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cassert>
#include <algorithm>

#pragma comment(lib, "dblp/sqlite3.lib")

using namespace string_utilities;

void BibDatabase::SyncDBLP(const string& dbFile) const
{
	try
	{
		auto dblpDB = DBLPDatabase::Create(dbFile);
		Logger::Debug("connected to DBLP database with " + to_string(dblpDB->RowCount()) + " rows");

		auto parser = BibParser::Create();
		for (auto entry : entries)
			dblpDB->Sync(entry, *parser);
	}
	catch (const exception &e)
	{
		Logger::Warning("SQL error: " + string(e.what()));
	}
}


DBLPDatabase::DBLPDatabase(const string& dbFile): db(nullptr)
{
	if (!fstream(dbFile))
	{
		throw runtime_error("database file not found");
	}

	int rc = sqlite3_open(dbFile.c_str(), &db);
	if (rc != SQLITE_OK)
	{
		string msg = sqlite3_errmsg(db);
    	sqlite3_close(db);
    	throw runtime_error(msg);
  	}
}

DBLPDatabase::~DBLPDatabase()
{
	sqlite3_close(db);
}

int DBLPDatabase::RowCount() const
{
	sqlite3_stmt* stmt;
	string sql = "SELECT count(*) FROM BE;";

	runSQLite(sqlite3_prepare_v2(db, sql.c_str(), sql.length() + 1, &stmt, nullptr));
	runSQLite(sqlite3_step(stmt), SQLITE_ROW);
	int cnt = sqlite3_column_int(stmt, 0);
	runSQLite(sqlite3_finalize(stmt));

	return cnt;
}

void DBLPDatabase::Sync(BibEntry* entry, const BibParser& parser) const
{
	// get records from database
	vector<DBLPEntry> dblpEntry = findByTitle(entry->getTitle());
	//Logger::Debug("found entries for " + entry->key + ": " + to_string(dblpEntry.size()));

	if (dblpEntry.empty()) 
	{
		if (entry->type == "misc") return;

		Logger::Warning("no entries found for " + entry->key + " in dblp database");
		return;
	}

	// construct bib entries
	vector<BibEntry*> bibDBLPEntries;
	for (auto e : dblpEntry)
	{
		auto en = CreateBibEntry(e, parser);
		assert(en != nullptr);
		bibDBLPEntries.push_back(en);
	}

	// break ties (using author, year, etc)
	auto bibDBLPEntry = FilterDBLPEntries(bibDBLPEntries, entry);
	if (bibDBLPEntry == nullptr) return;

	// actual syncing
	for (auto f : bibDBLPEntry->fields)
	{
		string tag = f.first;
		if (!entry->fields.count(tag))
		{
			Logger::Debug("updating field '" + tag + "' for " + entry->key + " from DBLP database");
			entry->fields[tag] = bibDBLPEntry->fields[tag];
		}
	}

	// remove (TODO)
	for (auto en : bibDBLPEntries)
		delete en;
}

BibEntry* DBLPDatabase::FilterDBLPEntries(vector<BibEntry*> entries, BibEntry* entry) const
{
	assert(!entries.empty());

	if ((int)entries.size() == 1) 
	{
		return entries[0];
	}

	FilterDBLPEntriesByAuthor(entries, entry);
	if (entries.empty())
	{
		Logger::Warning("can't find dblp entry for " + entry->key + " with author=" + entry->fields["author"]);
		return nullptr;
	}
	else if ((int)entries.size() == 1)
	{
		return entries[0];
	}

	FilterDBLPEntriesByYear(entries, entry);
	if (entries.empty())
	{
		Logger::Warning("can't find dblp entry for " + entry->key + " with year=" + entry->fields["year"]);
		return nullptr;
	}
	else if ((int)entries.size() == 1)
	{
		return entries[0];
	}

	FilterDBLPEntriesByType(entries, entry);
	if (entries.empty())
	{
		Logger::Warning("can't find dblp entry for " + entry->key + " with type=" + entry->type);
		return nullptr;
	}
	else if ((int)entries.size() == 1)
	{
		return entries[0];
	}

	Logger::Warning(to_string(entries.size()) + " dblp entries for " + entry->key + " with author=" + entry->fields["author"] + " and year=" + entry->fields["year"]);
	return nullptr;
}

void DBLPDatabase::FilterDBLPEntriesByAuthor(vector<BibEntry*>& entries, BibEntry* entry) const
{
	auto authors = entry->getAuthors();
	entries.erase(remove_if(begin(entries), end(entries), 
		[&](BibEntry* en) 
		{
			auto authors2 = en->getAuthors();
			if (authors.size() != authors2.size()) 
				return true;

			for (int i = 0; i < (int)authors.size(); i++)
				if (to_lower(authors[i].last) != to_lower(authors2[i].last)) 
					return true;

			return false;
		}), end(entries));
}

void DBLPDatabase::FilterDBLPEntriesByYear(vector<BibEntry*>& entries, BibEntry* entry) const
{
	string year = entry->getYear();
	bool foundRecent = false;
	for (auto en : entries)
		if (en->getYear() > year)
		{
			foundRecent = true;
			break;
		}

	if (foundRecent)
	{
		Logger::Warning("found a newer version of the paper for '" + entry->key + "'");
	}

	entries.erase(remove_if(begin(entries), end(entries), 
		[&](BibEntry* en) 
		{
			return en->getYear() != year;
		}), end(entries));
}

void DBLPDatabase::FilterDBLPEntriesByType(vector<BibEntry*>& entries, BibEntry* entry) const
{
	entries.erase(remove_if(begin(entries), end(entries), 
		[&](BibEntry* en) 
		{
			return en->type != entry->type;
		}), end(entries));
}

BibEntry* DBLPDatabase::CreateBibEntry(const DBLPEntry& dblpEntry, const BibParser& parser) const
{
	try
	{
		BibEntry* entry = parser.ParseBibEntry(dblpEntry.content);
		if (entry->fields.count("crossref"))
		{
			string ref = unquote(entry->fields["crossref"]);
			DBLPEntry dblpEntry;
			try 
			{
				dblpEntry = findByKey(ref);
			}
			catch (...)
			{
				Logger::Warning("no reference entry in dblp database for crossref='" + ref + "'");
				//TODO!!!
				return entry;
			}

			auto bibDBLPEntry = unique_ptr<BibEntry>(CreateBibEntry(dblpEntry, parser));

			// syncing from referenced entry
			for (auto f : bibDBLPEntry->fields)
				if (!entry->fields.count(f.first))
					entry->fields[f.first] = bibDBLPEntry->fields[f.first];

			entry->fields.erase("crossref");
		}

		//FixLocalURL(entry, "url");
		entry->fields.erase("url");
		FixLocalURL(entry, "ee");
		ExtractDOI(entry);

		return entry;
	}
	catch (const exception& e)
	{
		throw runtime_error("can't parse content of dblp entry: " + string(e.what()));
	}
}

void DBLPDatabase::FixLocalURL(BibEntry* entry, const string& field) const
{
	if (!entry->fields.count(field)) return;
	string v = unquote(entry->fields[field]);
	if (!startsWith(v, "http:") && !startsWith(v, "https:") && !startsWith(v, "ftp:"))
	{
		entry->fields[field] = "\"http://dblp.uni-trier.de/" + v + "\"";
	}
}

void DBLPDatabase::ExtractDOI(BibEntry* entry) const
{
	if (entry->fields.count("doi")) return;
	if (!entry->fields.count("ee")) return;

	string openQ, closeQ;
	string ee = unquote(entry->fields["ee"], openQ, closeQ);
	if (ee.find("doi") == string::npos) return;

	vector<string> prefixes = vector_of_strings("http://dx.doi.org/")("http://doi.acm.org/")("http://doi.ieeecomputersociety.org/")();
	for (auto& prefix : prefixes)
	{
		if (startsWith(ee, prefix))
		{
			entry->fields["doi"] = openQ + ee.substr(prefix.length()) + closeQ;
			//entry->fields.erase("ee");
			return;
		}
	}

	Logger::Warning("cannot extract doi from string '" + ee + "'");
}

vector<DBLPEntry> DBLPDatabase::findByTitle(const string& title) const
{
	string trunc_title;
	for (int i = 0; i < (int)title.length() && i < 256; i++)
	{
		auto ch = static_cast<unsigned char>(title[i]);
		if (isalpha(ch))
			trunc_title += tolower(ch);
	}

	if (trunc_title.length() > DBLPEntry::MAX_TITLE_SIZE) trunc_title = trunc_title.substr(0, DBLPEntry::MAX_TITLE_SIZE);

    sqlite3_stmt* stmt;
	string sql = "SELECT dblp_key, title, content FROM BE WHERE title=?";
	runSQLite(sqlite3_prepare_v2(db, sql.c_str(), sql.length() + 1, &stmt, nullptr));
	runSQLite(sqlite3_bind_text(stmt, 1, trunc_title.c_str(), trunc_title.length() + 1, SQLITE_STATIC));

	vector<DBLPEntry> result;
    while (true) 
	{
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) 
		{
			DBLPEntry en;
            en.dblp_key = string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            en.title = string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            en.content = string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));

			result.push_back(en);
        }
        else if (rc == SQLITE_DONE) 
            break;
        else 
			throw runtime_error(sqlite3_errmsg(db));
	}

	return result;
}

DBLPEntry DBLPDatabase::findByKey(const string& key) const
{
    sqlite3_stmt* stmt;
	string sql = "SELECT dblp_key, title, content FROM BE WHERE dblp_key=?";
	runSQLite(sqlite3_prepare_v2(db, sql.c_str(), sql.length() + 1, &stmt, nullptr));
	runSQLite(sqlite3_bind_text(stmt, 1, key.c_str(), key.length() + 1, SQLITE_STATIC));

    while (true) 
	{
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) 
		{
			DBLPEntry en;
            en.dblp_key = string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            en.title = string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            en.content = string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));

			return en;
        }
        else if (rc == SQLITE_DONE) 
			throw runtime_error("crossref entry '" + key + "' not found in DBLP database");
        else 
			throw runtime_error(sqlite3_errmsg(db));
	}
}

void DBLPDatabase::runSQLite(int rc, int ec) const
{
	if (rc != ec)
	{
		throw runtime_error(sqlite3_errmsg(db));
	}
}

void DBLPDatabase::runSQLite(int rc) const
{
	runSQLite(rc, SQLITE_OK);
}
