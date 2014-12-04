BTStyle
=======

BibTex formatter

Usage: BTStyle [options] input.bib
       BTStyle [options] < input.bib > output.bib

When processing a specific bib file, the result OVERWRITES the input file;
the original file is renamed by adding a suffix ".orig".

Allowed options:
  
  Input file name

  --field-delimeters=[braces|quotes]
  Convert outer delimeters in field values to either braces or double quotes

  --replace-unicode
  Replace special UTF-8 symbols with the corresponding LaTeX command

  --fix-pages
  Replace single dash with double one in pages

  --fix-padding
  Remove line breaks and adjust white spaces in field values

  --keys=[alpha|abstract]
  Modify entry keys according to the specified style

  --sort=[author|year-asc|year-desc]
  Sort entries according to the specified style

  --format-author=[space|comma]
  Format author names to either space-separated (First Last) or comma-separated (Last, First) format

  --log-level=[debug|info|warning|error]
  Log level

  --default
  Apply default options
