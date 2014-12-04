#include "bib_database.h"

#include "logger.h"
#include "unicode_latex.h"
#include "string_utilities.h"

#include <cassert>
#include <algorithm>

using namespace string_utilities;

BibDatabase::BibDatabase()
{
}

BibDatabase::~BibDatabase()
{
	for (auto m : entries)
		delete m;
	for (auto m : abbrv)
		delete m;
	for (auto m : comments)
		delete m;
	for (auto m : preambles)
		delete m;
}

void BibDatabase::LogDetails() const
{
	int kb = int(double(inputFilesize)/1024.0 + 0.5);
	string msg = "Successfully parsed " + (inputFilename != "" ? inputFilename : "stdin") + " (" + to_string(kb) + "KB): \n";
	if (!preambles.empty())
		msg += "  " + to_string(preambles.size()) + " preambles\n";
	if (!abbrv.empty())
		msg += "  " + to_string(abbrv.size()) + " string abbreviations\n";
	if (!entries.empty())
		msg += "  " + to_string(entries.size()) + " bib entries\n";
	
	if (Logger::warningCount > 0 )
		msg += "There were " + to_string(Logger::warningCount) + " warnings\n";

	Logger::Info(trim(msg));
}

BibEntry* BibDatabase::FindEntryByKey(const string& key) const
{
	for (auto entry: entries)
		if (key == entry->key) return entry;

	return NULL;
}

void BibDatabase::ConvertFieldDelimeters(const string& option) const
{
	assert(option == "braces" || option == "quotes");

	char openQ, closeQ;
	if (option == "quotes")
		openQ = closeQ = '"';
	else
		openQ = '{', closeQ = '}';

	for (auto en: entries)
	{
		for (auto tag: en->fields)
		{
			string key = tag.first;
			string value = tag.second;

			int len = value.length();

			if ((value[0] == '{' || value[0] == '"') && (value[len - 1] == '}' || value[len - 1] == '"'))
			{
				value[0] = openQ;
				value[len - 1] = closeQ;

				en->fields[key] = value;
			}
			else if (isInteger(value))
			{
				value = openQ + value + closeQ;

				en->fields[key] = value;
			}
		}
	}
}

void BibDatabase::ReplaceUnicodeCharacters() const
{
	for (auto en: entries)
	{
		for (auto tag: en->fields)
		{
			string key = tag.first;
			string value = tag.second;

			string nvalue = unicode_latex::transform(value);
			if (value != nvalue)
			{
				en->fields[key] = nvalue;
				Logger::Debug("replaced unicode characters in " + en->key + " for '" + key + "'");
			}
		}
	}
}

void BibDatabase::FixPagesDash() const
{
	for (auto en: entries)
	{
		string tag = "pages";
		if (en->fields.count(tag))
		{
			string value = en->fields[tag];

			string openQ, closeQ;
			string v = unquote(value, openQ, closeQ);

			vector<string> tmp = split(v, " -");
			if ((int)tmp.size() == 2 && isInteger(tmp[0]) && isInteger(tmp[1]))
			{
				string nvalue = openQ + tmp[0] + "--" + tmp[1] + closeQ;
				if (value != nvalue)
				{
					en->fields[tag] = nvalue;
					Logger::Debug("fixed page dashes in " + en->key);
				}
			}
		}
	}
}

void BibDatabase::FixPadding() const
{
	for (auto entry: entries)
	{
		FixPadding(entry, "title");
		FixPadding(entry, "journal");
		FixPadding(entry, "booktitle");
		FixPadding(entry, "publisher");
		FixPadding(entry, "series");
		FixPadding(entry, "address");
		FixPadding(entry, "organization");
		FixPadding(entry, "institution");
		FixPadding(entry, "howpublished");
	}
}

void BibDatabase::FixPadding(BibEntry* entry, const string& tag) const
{
	if (!entry->fields.count(tag)) return;

	string value = entry->fields[tag];
	string openQ, closeQ;
	string v = unquote(value, openQ, closeQ);
		
	v = replace(v, "\n", " ");
	v = replace(v, "\t", " ");
	v = replace(v, "\r", " ");
	v = replace(v, "  ", " ");
	v = trim(v);

	string nvalue = openQ + v + closeQ;
	if (value != nvalue)
	{
		entry->fields[tag] = nvalue;
		Logger::Debug("fixed padding for " + tag + " in " + entry->key);
	}
}

void BibDatabase::CheckRequiredFields() const
{
	for (auto entry: entries)
	{
		vector<string> required = BibEntry::GetRequiredFields(entry->type);
		set<string> listed = entry->getFields();
		if (entry->fields.count("crossref"))
		{
			string ref = unquote(entry->fields["crossref"]);
			BibEntry* refEntry = FindEntryByKey(ref);
			if (refEntry == NULL)
			{
				Logger::Warning("non-existing crossref '" + ref + "' in " + entry->key);
				continue;
			}
			set<string> listedRef = refEntry->getFields();
			listed.insert(listedRef.begin(), listedRef.end());
		}

		for (string f : required)
		{
			vector<string> fs = split(f, "|");
			bool fieldFound = false;
			for (string s : fs)
				fieldFound |= (listed.count(s) > 0);

			Logger::Warning(fieldFound, "missing required field '" + f + "' in " + entry->key);
		}
	}
}

void BibDatabase::ConvertKeys(const string& option) const
{
	assert(option == "alpha" || option == "abstract");

	set<string> allKeys;
	for (auto entry: entries)
	{
		if (!entry->fields.count("author")) continue;
		string nvalue = ConvertKey(option, entry, entry->getAuthors());

		if (allKeys.count(nvalue) > 0)
		{
			for (char c = 'a'; c <= 'z'; c++)
			{
				string ss = nvalue + c;
				if (!allKeys.count(ss))
				{
					nvalue = ss;
					break;
				}
			}
		}

		allKeys.insert(nvalue);
		if (entry->key != nvalue)
		{
			Logger::Debug("modified key in " + entry->key + " to '" + nvalue + "'");
			entry->key = nvalue;
		}
	}
}

string BibDatabase::ConvertKey(const string& option, const BibEntry* entry, const vector<Author>& authors) const
{
	if (option == "alpha")
	{
		string year = GetYear(entry);
		if ((int)year.length() == 4) year = year.substr(2, 2);
		string author;
		if (authors.empty()) author = "";
		else if ((int)authors.size() == 1) author = authors[0].last.substr(0, 3);
		else
		{
			for (int i = 0; i < (int)authors.size() && i < 5; i++)
				if (authors[i].last != "others")	
					author += authors[i].last[0];
			if ((int)authors.size() > 5) author += "+";
		}

		return author + year;
	}
	else if (option == "abstract")
	{
		string year = GetYear(entry);
		string first_author = (authors.empty() ? "" : split(authors[0].last, " ")[0]);

		if (year != "")
			return first_author + "-" + year;
		return first_author;
	}

	return "";
}

string BibDatabase::GetYear(const BibEntry* entry) const
{
	if (entry->fields.count("year") > 0)
		return entry->getYear();

	if (entry->fields.count("crossref") > 0)
	{
		string ref = unquote(entry->fields.find("crossref")->second);
		BibEntry* refEntry = FindEntryByKey(ref);
		if (refEntry != NULL && refEntry->fields.count("year") > 0)
			return refEntry->getYear();
	}

	return "";
}

void BibDatabase::FormatAuthor(const string& option) const
{
	assert(option == "space" || option == "comma");

	for (auto entry: entries)
	{
		if (!entry->fields.count("author")) continue;
		string openQ, closeQ;
		string value = unquote(entry->fields["author"], openQ, closeQ);
		vector<Author> authors = entry->getAuthors();

		string nvalue = "";
		for (Author a : authors)
			if (a.first != "" || a.von != "" || a.last != "")
			{
				if (nvalue != "") nvalue += " and ";
				if (option == "space") 
				{
					nvalue += a.first + " " + a.von + " " + a.last;
				}
				else
				{
					nvalue += a.von + " " + a.last;
					if (a.first != "") nvalue += ", " + a.first;
				}
			}

		nvalue = replace(trim(nvalue), "  ", " ");
		if (value != nvalue)
		{
			Logger::Debug("modified format of author in " + entry->key + " to '" + nvalue + "'");
			entry->fields["author"] = openQ + nvalue + closeQ;;
		}
	}
}

void BibDatabase::SortEntries(const string& option)
{
	assert(option == "author" || option == "year-asc" || option == "year-desc");

	if (option == "author")
		stable_sort(entries.begin(), entries.end(), BibEntry::AuthorComparator);
	else if (option == "year-asc")
		stable_sort(entries.begin(), entries.end(), BibEntry::YearAscComparator);
	else if (option == "year-desc")
		stable_sort(entries.begin(), entries.end(), BibEntry::YearDescComparator);
}
