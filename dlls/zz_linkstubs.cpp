// Stand-ins until monsters.cpp and pulse.cpp have the real code: empty definitions
// of CBaseMonster::HasHintNodes (monsters.cpp, 0x10087c90) and CLaserPulse::Shoot
// (pulse.cpp, 0x100b0860), so that hl.dll links without them
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"

bool CBaseMonster::HasHintNodes( void ) { return false; }

CLaserPulse *CLaserPulse::Shoot( Vector vecOrigin, edict_t *pOwner, const Vector &vecColor, int iTrailLife, float flScale, int iColor ) { return NULL; }
