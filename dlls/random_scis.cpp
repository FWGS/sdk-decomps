/***
*
*	Scientist Slaughterhouse: random scientists.
*	The file name and the names marked "ours" are ours; the entity name and
*	the cvar come from the binary.
*
****/
/*

===== random_scis.cpp ========================================================

  random_scis: an invisible point that wanders the level and drops
  scientists while ssh_randomscis says there are too few.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "game.h"
#include "monsters.h"


//
// random_scis: not in the fgd, not placed by the maps; something creates it
// when ssh_randomscis is set. It flies about (MOVETYPE_BOUNCEMISSILE, a
// random velocity of up to 1000 per axis), jumping to the next entity of the
// entity list each time (unless that would leave it inside a solid). Every
// 1 to 5 seconds, while fewer than ssh_randomscis of its scientists live, it
// drops a monster_scientist or a monster_cleansuit_sci (half and half) at
// its origin, facing a random yaw, falling to the ground, pre-disaster half
// of the time, provided the spot is empty and the floor below isn't sky.
// The scientists are its children: their DeathNotice counts them down.
//
class CRandomScis : public CBaseEntity		// name ours
{
public:
	void Spawn( void );
	void Precache( void );
	int	ObjectCaps( void ) { return FCAP_DONT_SAVE; }
	void DeathNotice( entvars_t *pevChild );

	void SpawnThink( void );			// name ours
	void FindSpot( void );				// name ours

	int		m_cScis;					// name ours: scientists alive
};

LINK_ENTITY_TO_CLASS( random_scis, CRandomScis );


void CRandomScis :: Spawn( void )
{
	Precache();

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_BOUNCEMISSILE;
	pev->friction = 0;

	SET_MODEL( ENT( pev ), "models/scientist.mdl" );
	pev->effects = EF_NODRAW;
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 72 ) );

	FindSpot();

	m_cScis = 0;
	pev->nextthink = gpGlobals->time + 1;
	SetThink( SpawnThink );
}


void CRandomScis :: Precache( void )
{
	UTIL_PrecacheOther( "monster_scientist" );
	UTIL_PrecacheOther( "monster_cleansuit_sci" );
}


void CRandomScis :: SpawnThink( void )
{
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 1, 5 );

	if ( m_cScis >= ssh_randomscis.value )
	{
		FindSpot();
		return;
	}

	if ( ( pev->origin.x == 0 && pev->origin.y == 0 && pev->origin.z == 0 ) || UTIL_PointContents( pev->origin ) != CONTENTS_EMPTY )
	{
		FindSpot();
		return;
	}

	TraceResult tr;

	UTIL_TraceLine( pev->origin, Vector( pev->origin.x, pev->origin.y, -4096 ), ignore_monsters, ENT( pev ), &tr );

	if ( tr.fStartSolid || UTIL_PointContents( tr.vecEndPos ) == CONTENTS_SKY )
	{
		FindSpot();
		return;
	}

	char *pszClass = RANDOM_FLOAT( 0, 1 ) < 0.5 ? "monster_scientist" : "monster_cleansuit_sci";

	edict_t	*pent = CREATE_NAMED_ENTITY( MAKE_STRING( pszClass ) );
	if ( FNullEnt( pent ) )
	{
		ALERT ( at_console, "NULL Ent in RandomScis!\n" );
		return;
	}

	entvars_t *pevCreate = VARS( pent );
	pevCreate->origin = pev->origin;
	pevCreate->angles = Vector( 0, RANDOM_FLOAT( 0, 360 ), 0 );
	SetBits( pevCreate->spawnflags, SF_MONSTER_FALL_TO_GROUND );
	if ( RANDOM_FLOAT( 0, 1 ) > 0.5 )
		SetBits( pevCreate->spawnflags, SF_MONSTER_PREDISASTER );
	pevCreate->body = -1;
	pevCreate->owner = edict();

	DispatchSpawn( ENT( pevCreate ) );

	m_cScis++;
	ALERT( at_console, "RandomSci: Created %s at (%f, %f, %f)\n", pszClass, pev->origin.x, pev->origin.y, pev->origin.z );

	FindSpot();
}


void CRandomScis :: DeathNotice( entvars_t *pevChild )
{
	m_cScis--;
}


// name ours: the next entity of the entity list after index iIndex
CBaseEntity *FindNextEntity( int iIndex )
{
	for ( int i = iIndex + 1; i < gpGlobals->maxEntities; i++ )
	{
		edict_t *pent = INDEXENT( i );
		if ( pent && !pent->free )
			return CBaseEntity::Instance( pent );
	}

	return NULL;
}


// jump to the next entity in the list (pev->weapons keeps its index) and fly off from there
void CRandomScis :: FindSpot( void )
{
	CBaseEntity *pEntity = FindNextEntity( pev->weapons );

	if ( pEntity )
	{
		pev->weapons = ENTINDEX( pEntity->edict() );

		if ( !pEntity->IsBSPModel() )
		{
			Vector vecSpot = pEntity->pev->origin;
			if ( vecSpot == g_vecZero )
				vecSpot = VecBModelOrigin( pEntity->pev );	// sic: never used

			Vector vecOld = pev->origin;
			pev->owner = pEntity->edict();
			UTIL_SetOrigin( pev, pEntity->pev->origin );
			if ( IsInSolid() )
				UTIL_SetOrigin( pev, vecOld );
		}
	}
	else
	{
		pev->weapons = 0;
	}

	Vector vecDir = Vector( RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ) );
	pev->velocity = vecDir * 1000;
}
