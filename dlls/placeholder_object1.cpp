// Placeholder, not Gunman's code.
//
// Gunman's hl.dsp compiled two more C++ sources than this project:
// retail's Rich header counts 124 C++ objects (Utc12 8168) where the files
// the server's code shows give 122, and LINK sizes the PE header by the
// number of objects it loads (e_lfanew 0xe0 in retail, 0xe8 with two
// fewer). Those sources left no code and no data in gunman.dll, so their
// names, contents and places in the project can't be recovered. This empty
// file and placeholder_object2.cpp stand in for them (relinking the same
// objects with two empty ones gives retail's Rich header and PE header and
// leaves every section as it was).
//
// They come last in the project: a source with no #include writes an empty
// .pch that every later file would reuse, which moves header inlines in
// all of them; nothing in retail's objects shows that, so nothing follows.
