#include "bib_entry.h"
#include "string_utilities.h"
#include "logger.h"

#include <algorithm>
#include <cassert>

using namespace string_utilities;

const string BibEntry::TAG_ORDER[] = {
		"author", 
		"title", 
		"year",
		"journal",
		"booktitle",
		"volume",
		"number",
		"pages",
		"publisher",
		"editor",
		"series",
		"address",
		"month",
		"organization",
		"institution",
		"howpublished",
		"isbn",
		"url",
		"ee",
		"doi",
};

const map<string, vector<string> > BibEntry::REQUIRED_FIELDS = BibEntry::InitRequiredField();

map<string, vector<string> > BibEntry::InitRequiredField()
{
	map<string, vector<string> > m;
	m["article"] = vector_of_strings("author")("title")("journal")("year")();
	m["book"] = vector_of_strings("author|editor")("title")("publisher")("year")();
	m["booklet"] = vector_of_strings("title")();
	m["conference"] = vector_of_strings("author")("title")("booktitle")("year")();
	m["inbook"] = vector_of_strings("author|editor")("title")("chapter|pages")("publisher")("year")();
	m["incollection"] = vector_of_strings("author")("title")("booktitle")("publisher")("year")();
	m["inproceedings"] = vector_of_strings("author")("title")("booktitle")("year")();
	m["manual"] = vector_of_strings("title")();
	m["mastersthesis"] = vector_of_strings("author")("title")("school")("year")();
	m["misc"];
	m["phdthesis"] = vector_of_strings("author")("title")("school")("year")();
	m["proceedings"] = vector_of_strings("title")("year")();
	m["techreport"] = vector_of_strings("author")("title")("institution")("year")();
	m["unpublished"] = vector_of_strings("author")("title")("note")();

	return m;
}

bool BibEntry::IsValidEntry(const string& type)
{
	return REQUIRED_FIELDS.count(type) > 0;
}

vector<string> BibEntry::GetRequiredFields(const string& type)
{
	return REQUIRED_FIELDS.find(type)->second;
}

bool BibEntry::TagComparator(const string& s1, const string& s2)
{
	int rank1 = distance(TAG_ORDER, find(begin(TAG_ORDER), end(TAG_ORDER), s1));
	int rank2 = distance(TAG_ORDER, find(begin(TAG_ORDER), end(TAG_ORDER), s2));

	if (rank1 < rank2 || (rank1 == rank2 && s1 < s2)) return true;
	return false;
}

bool BibEntry::AuthorComparator(const BibEntry* e1, const BibEntry* e2)
{
	vector<Author> authors1 = e1->getAuthors();
	vector<Author> authors2 = e2->getAuthors();
	int n1 = (int)authors1.size(), n2 = (int)authors2.size();

	if (n1 == 0 || n2 == 0)
	{
		if (n1 == 0 && n2 == 0) return false;
		if (n1 == 0) return false;
		return true;
	}

	for (int i = 0; i < n1 && i < n2; i++)
	{
		if (authors1[i].last < authors2[i].last) return true;
		if (authors1[i].last > authors2[i].last) return false;
	}

	if (n1 < n2) return true;
	if (n1 > n2) return false;

	return false;
}

bool BibEntry::TitleComparator(const BibEntry* e1, const BibEntry* e2)
{
	string title1 = e1->getTitle();
	string title2 = e2->getTitle();
	if (title1 == title2)
		return YearAscComparator(e1, e2);

	return (title1 < title2);
}

bool BibEntry::YearAscComparator(const BibEntry* e1, const BibEntry* e2)
{
	string year1 = e1->getYear();
	string year2 = e2->getYear();

	if (year1 == year2) 
		return AuthorComparator(e1, e2);

	if (year1 == "" || year2 == "")
	{
		if (year1 == "") return false;
		if (year2 == "") return true;
	}

	return (year1 < year2);
}

bool BibEntry::YearDescComparator(const BibEntry* e1, const BibEntry* e2)
{
	string year1 = e1->getYear();
	string year2 = e2->getYear();

	if (year1 == year2) 
		return AuthorComparator(e1, e2);

	if (year1 == "" || year2 == "")
	{
		if (year1 == "") return false;
		if (year2 == "") return true;
	}

	return (year2 < year1);
}

set<string> BibEntry::getFields() const
{
	set<string> res;
	for (auto f : fields)
		res.insert(f.first);

	if (refEntry != nullptr)
	{
		for (auto f : refEntry->fields)
			res.insert(f.first);
	}

	return res;
}

string BibEntry::getYear() const
{
	if (fields.count("year")) 
		return unquote(fields.find("year")->second);

	if (refEntry != nullptr && refEntry->fields.count("year"))
		return unquote(refEntry->fields.find("year")->second);

	return "";
}

string BibEntry::getTitle() const
{
	if (fields.count("title")) 
		return unquote(fields.find("title")->second);

	return "";
}

vector<Author> BibEntry::getAuthors() const
{
	if (!fields.count("author")) 
		return authors;

	if (authors.empty())
	{
		authors = ParseAuthors();
	}

	return authors;
}

vector<Author> BibEntry::ParseAuthors() const
{
	vector<Author> result;

	string s = unquote(fields.find("author")->second);
	s = replace(s, "\n", " ");
	s = replace(s, "\t", " ");
	s = replace(s, "\r", " ");
	int brCount = 0;
	int i = 0, len = s.length();
	string cur = "";

	while (i < len)
	{
		if (s[i] == '{') brCount++;
		if (s[i] == '}') brCount--;

		if (brCount == 0 && i+4 < len && s[i] == ' ' && s[i+1] == 'a' && s[i+2] == 'n' && s[i+3] == 'd' && s[i+4] == ' ')
		{
			result.push_back(ParseAuthor(trim(cur)));
			cur = "";
			i += 5;
			continue;
		}

		cur += s[i];
		i++;
	}

	assert(brCount == 0);
	result.push_back(ParseAuthor(trim(cur)));
	return result;
}

Author BibEntry::ParseAuthor(const string& input) const
{
	string ss = replace(input, "~", " ");

	string s;
	// adding missing white spaces: replacing "." with ". "
	for (int i = 0; i < (int)ss.length(); i++)
		if (ss[i] == '.' && i+1 < (int)ss.length() && isalpha(ss[i + 1])) {s += ss[i]; s += " ";}
		else s += ss[i];

	Author author;

	if (s.find(',') != string::npos)
	{
		
		vector<string> parts = split(s, ",");
		Logger::Warning((int)parts.size() == 2, "cannot parse author '" + s + "' in " + key);
		//warn if more than 2
		author.first = trim(parts[1]);

		vector<string> tokens = split(parts[0], " \n\r\t");
		int nn = (int)tokens.size();
		int endVon = -1;
		for (int i = 0; i < nn; i++)
			if (!islower(tokens[i][0])) {endVon = i - 1; break;}

		if (endVon != -1)
		{
			for (int i = 0; i <= endVon; i++)
				author.von += tokens[i] + " ";
			for (int i = endVon + 1; i < nn; i++)
				author.last += tokens[i] + " ";
		}
		else
		{
			for (int i = 0; i < nn; i++)
				author.last += tokens[i] + " ";
		}
	}
	else
	{
		vector<string> tokens = split(s, " \n\r\t");
		int startVon = -1, endVon = -1;
		int nn = (int)tokens.size();
		for (int i = 0; i < nn; i++)
			if (islower(tokens[i][0])) {startVon = i; break;}
		for (int i = startVon + 1; i < nn; i++)
			if (!islower(tokens[i][0])) {endVon = i - 1; break;}

		if (startVon != -1)
		{
			for (int i = 0; i < startVon; i++)
				author.first += tokens[i] + " ";
			for (int i = startVon; i <= endVon; i++)
				author.von += tokens[i] + " ";
			for (int i = endVon + 1; i < nn; i++)
				author.last += tokens[i] + " ";
		}
		else
		{
			for (int i = 0; i + 1 < nn; i++)
				author.first += tokens[i] + " ";
			if (nn > 0) 
				author.last = tokens[nn - 1];
		}
	}

	author.last = replace(trim(author.last), "  ", " ");
	author.von = replace(trim(author.von), "  ", " ");
	author.first = replace(trim(author.first), "  ", " ");

	Logger::Warning(author.last.length() > 0, "empty last name for author '" + ss + "' in "+ key);
	Logger::Warning(author.first.length() > 0 || author.last == "others", "empty first name for author '" + ss + "' in "+ key);
	return author;
}
