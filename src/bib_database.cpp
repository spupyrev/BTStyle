#include "bib_database.h"

#include "logger.h"
#include "unicode_latex.h"
#include "string_utilities.h"

#include <cassert>
#include <algorithm>
#include <fstream>
#include <regex>

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

void BibDatabase::InitKeyEntryMap()
{
	keyEntryMap.clear();
	for (auto entry: entries)
	{
		string lkey = to_lower(entry->key);
		if (keyEntryMap.count(lkey))
			Logger::Warning("duplicate key " + entry->key);
		else
			keyEntryMap[lkey] = entry;
	}
}

void BibDatabase::InitRefEntries() const
{
	for (auto entry: entries)
	{
		if (entry->fields.count("crossref"))
		{
			string ref = unquote(entry->fields["crossref"]);
			if (!keyEntryMap.count(ref))
			{
				Logger::Warning("non-existing crossref '" + ref + "' in " + entry->key);
			}
			else
			{
				entry->refEntry = keyEntryMap.find(ref)->second;
			}
		}
	}
}

void BibDatabase::CheckRequiredFields() const
{
	for (auto entry: entries)
	{
		vector<string> required = BibEntry::GetRequiredFields(entry->type);
		set<string> listed = entry->getFields();
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
		for (auto field: en->fields)
		{
			string tag = field.first;
			string value = field.second;

			string nvalue;
			if (tag == "title")
				nvalue = openQ + unquote(unquote(value)) + closeQ;
			else
			{
				string oldOQ, oldCQ;
				string unq = unquote(value, oldOQ, oldCQ);
				if (oldOQ != "" && oldCQ != "")
					nvalue = openQ + unq + closeQ;
				else
					nvalue = value;
			}

			en->fields[tag] = nvalue;
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

void BibDatabase::ConvertKeys(const string& option, const string& texFile)
{
	assert(option == "alpha" || option == "abstract");

	map<string, vector<BibEntry*> > key2Entries;
	for (auto entry: entries)
	{
		if (!entry->fields.count("author")) 
		{
			key2Entries[entry->key].push_back(entry);
			continue;
		}

		string nvalue = GenerateKey(option, entry, entry->getAuthors());
		key2Entries[nvalue].push_back(entry);
	}

	for (auto it: key2Entries)
	{
		string key = it.first;
		vector<BibEntry*> entries = it.second;
		for (int i = 0; i < (int)entries.size(); i++)
		{
			BibEntry* entry = entries[i];
			string nvalue = key;
			if (entries.size() > 1) nvalue += char('a' + i);

			if (entry->key != nvalue)
			{
				Logger::Debug("modified key in " + entry->key + " to '" + nvalue + "'");
				entry->key = nvalue;
			}
		}
	}

	if (texFile != "")
		ConvertTexKeys(texFile);

	InitKeyEntryMap();
}

string BibDatabase::GenerateKey(const string& option, const BibEntry* entry, const vector<Author>& authors) const
{
	if (option == "alpha")
	{
		string year = entry->getYear();
		if ((int)year.length() == 4) year = year.substr(2, 2);
		string author;
		if (authors.empty()) author = "";
		else if ((int)authors.size() == 1) 
		{
			size_t i = 0;
			string s = "";
			while (i < authors[0].last.length() && s.length() < 3)
			{
				if (isalpha(authors[0].last[i]))
					s += authors[0].last[i];
				i++;
			}
			author = s;
		}
		else
		{
			for (int i = 0; i < (int)authors.size() && i < 5; i++)
				if (authors[i].last != "others")	
					author += authors[i].last[0];
			if ((int)authors.size() > 6) author += "+";
			else if ((int)authors.size() == 6) author += authors[5].last[0];
		}

		return author + year;
	}
	else if (option == "abstract")
	{
		string year = entry->getYear();
		string first_author = (authors.empty() ? "" : split(authors[0].last, " ")[0]);

		if (year != "")
			return first_author + "-" + year;
		return first_author;
	}

	return "";
}

void BibDatabase::ConvertTexKeys(const string& texFile)
{
	// reading
	ifstream is;
	is.open(texFile.c_str(), ios::in);
	Logger::Error(is != 0, "can't open tex file '" + texFile + "'");
	string s((istreambuf_iterator<char>(is)), istreambuf_iterator<char>());
	is.close();

	// replacing
	int replacedCount = 0, keptCount = 0;
	regex re("\\\\cite(\\s*)\\{([^\\{\\}])*\\}");
    string result;
    auto callback = [&](const string& m) 
	{
		if (!regex_match(m, re))
		{
			result += m;
			return;
		}

		result += "\\cite{";

		int firstBracket = m.find_first_of('{');
		int lastBracket = m.find_last_of('}');
		string refString = m.substr(firstBracket + 1, lastBracket - firstBracket - 1);
		vector<string> refs = split(refString, ",");
		for (int i = 0; i < (int)refs.size(); i++)
		{
			if (i != 0) result += ",";

			string t = to_lower(trim(refs[i]));
			if (keyEntryMap.count(t))
			{
				result += keyEntryMap[t]->key;
				if (trim(refs[i]) != keyEntryMap[t]->key)
					replacedCount++;
				else
					keptCount++;
			}
			else
			{
				result += refs[i];
				Logger::Warning("citation " + refs[i] + " from " + texFile + " not found in the database ");
			}
		}

		result += "}";
    };

    sregex_token_iterator begin(s.begin(), s.end(), re, vector_of_ints(-1)(0)()), end;
    for_each(begin, end, callback);
	s = result;
	Logger::Debug("replaced " + to_string(replacedCount) + ", unchanged " + to_string(keptCount) + " citation keys in " + texFile);

	// output
	ofstream os;
	os.open((texFile + ".new").c_str(), ios::out);
	os << s;
	os.close();
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
	assert(option == "author" || option == "title" || option == "year-asc" || option == "year-desc");

	if (option == "author")
		stable_sort(entries.begin(), entries.end(), BibEntry::AuthorComparator);
	else if (option == "title")
		stable_sort(entries.begin(), entries.end(), BibEntry::TitleComparator);
	else if (option == "year-asc")
		stable_sort(entries.begin(), entries.end(), BibEntry::YearAscComparator);
	else if (option == "year-desc")
		stable_sort(entries.begin(), entries.end(), BibEntry::YearDescComparator);
}

