// Placeholder, not Wanted's code.
//
// Wanted's hl.dsp compiled eight more C++ sources than this project:
// retail's Rich header counts 146 C++ objects (Utc12 8168) where the files
// Wanted's code shows give 138, and LINK sizes the PE header by the number
// of objects it loads. Those sources left no code and no data in
// wanted.dll, so their names, places and contents can't be recovered; this
// file and its seven siblings (untraced1.cpp to untraced8.cpp) stand in for
// them. They come last in hl.dsp and include exactly what wtownb.cpp, the
// file before them, includes: with /YX each reuses the precompiled header
// wtownb.cpp left, so nothing compiled before or after them changes (the
// last build session, tools/release_build.py, starts from that header).

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"animation.h"
#include	"soundent.h"
