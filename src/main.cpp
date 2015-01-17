#include "bib_parser.h"
#include "cmd_options.h"
#include "logger.h"

void PrepareCMDOptions(int argc, char** argv, CMDOptions& args)
{
	string msg;
	msg += "Usage: BTStyle [options] input.bib\n";
	msg += "       BTStyle [options] < input.bib > output.bib\n\n";
	msg += "When processing a specific bib file, the result is saved in a new file with the suffix \".new\"\n";
	args.SetUsageMessage(msg);

	args.AddAllowedOption("", "", "Input file name");

	args.AddAllowedOption("--field-delimeters", "", "Convert outer delimeters in field values to either braces or double quotes");
	args.AddAllowedValue("--field-delimeters", "braces");
	args.AddAllowedValue("--field-delimeters", "quotes");

	args.AddAllowedOption("--replace-unicode", "Replace special UTF-8 symbols with the corresponding LaTeX command");

	args.AddAllowedOption("--fix-pages", "Replace single dash with double one in pages");

	args.AddAllowedOption("--fix-padding", "Remove line breaks and adjust white spaces in field values");

	args.AddAllowedOption("--keys", "", "Modify entry keys according to the specified style");
	args.AddAllowedValue("--keys", "alpha");
	args.AddAllowedValue("--keys", "abstract");

	args.AddAllowedOption("--keys-tex", "", "If --keys option is specified, then all the Bib entries are modified in the provided TeX file");

	args.AddAllowedOption("--sort", "", "Sort entries according to the specified style");
	args.AddAllowedValue("--sort", "author");
	args.AddAllowedValue("--sort", "title");
	args.AddAllowedValue("--sort", "year-asc");
	args.AddAllowedValue("--sort", "year-desc");

	args.AddAllowedOption("--format-author", "", "Format author names to either space-separated (First Last) or comma-separated (Last, First) format");
	args.AddAllowedValue("--format-author", "space");
	args.AddAllowedValue("--format-author", "comma");

	args.AddAllowedOption("--sync-dblp", "", "Synchronize entries with the specified dblp database");

	args.AddAllowedOption("--log-level", "info", "Log level");
	args.AddAllowedValue("--log-level", "debug");
	args.AddAllowedValue("--log-level", "info");
	args.AddAllowedValue("--log-level", "warning");
	args.AddAllowedValue("--log-level", "error");

	args.AddAllowedOption("--default", "Apply default options");

	args.Parse(argc, argv);

	if (args.hasOption("--default"))
	{
		args.SetOption("--field-delimeters=quotes");
		args.SetOption("--replace-unicode");
		args.SetOption("--fix-pages");
		args.SetOption("--fix-padding");
		args.SetOption("--log-level=debug");
		args.SetOption("--format-author=space");
		args.SetOption("--sort=year-desc");
	}
} 

void ProcessBibInfo(const CMDOptions& options, BibDatabase& db)
{
	db.InitKeyEntryMap();
	db.InitRefEntries();
	db.CheckRequiredFields();

	string dblpDBFile = options.getOption("--sync-dblp");
	if (dblpDBFile != "")
		db.SyncDBLP(dblpDBFile);

	string fieldDelimeters = options.getOption("--field-delimeters");
	if (fieldDelimeters != "")
		db.ConvertFieldDelimeters(fieldDelimeters);

	if (options.hasOption("--replace-unicode"))
		db.ReplaceUnicodeCharacters();

	if (options.hasOption("--fix-pages"))
		db.FixPagesDash();

	if (options.hasOption("--fix-padding"))
		db.FixPadding();

	string authorFormat = options.getOption("--format-author");
	if (authorFormat != "")
		db.FormatAuthor(authorFormat);

	string keys = options.getOption("--keys");
	if (keys != "")
		db.ConvertKeys(keys, options.getOption("--keys-tex"));

	string sort = options.getOption("--sort");
	if (sort != "")
		db.SortEntries(sort);

	db.LogDetails();
}

int main(int argc, char** argv)
{
	auto options = CMDOptions::Create();
	auto parser = BibParser::Create();
	auto db = BibDatabase::Create();

	int returnCode = 0;
	try
	{
		PrepareCMDOptions(argc, argv, *options);
		Logger::SetLogLevel(options->getOption("--log-level"));

		parser->Read(options->getOption(""), *db);

		ProcessBibInfo(*options, *db);

		parser->Write(options->getOption(""), *db);
	}
	catch (int code)
	{
		returnCode = code;
	}

	return returnCode;
}
