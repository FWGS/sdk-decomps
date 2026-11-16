// Link stand-ins, dropped when the real code comes: functions other
// files define: CBaseMonster::HasHintNodes (monsters.cpp 0x10087c90),
// CLaserPulse::Shoot (pulse.cpp 0x100b0860)
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"

bool CBaseMonster::HasHintNodes( void ) { return false; }

CLaserPulse *CLaserPulse::Shoot( Vector vecOrigin, edict_t *pOwner, const Vector &vecColor, int iTrailLife, float flScale, int iColor ) { return NULL; }
