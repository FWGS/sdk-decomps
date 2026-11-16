// Stand-ins until pulse.cpp has the real code: empty definitions of CLaserPulse::Shoot
// (0x100b0860) and CLaserPulse::PrecacheColor (0x100b0530), so that hl.dll links
// without them
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"

CLaserPulse *CLaserPulse::Shoot( Vector vecOrigin, edict_t *pOwner, const Vector &vecColor, int iTrailLife, float flScale, int iColor ) { return NULL; }

short CLaserPulse::PrecacheColor( int iColor ) { return 0; }
