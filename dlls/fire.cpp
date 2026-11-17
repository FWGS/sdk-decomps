/***
*
*	Scientist Slaughterhouse: fire on the ground.
*	The file name and the names marked "ours" are ours; class, entity and
*	exported member names come from the binary.
*
****/
/*

===== fire.cpp ========================================================

  flame (CFire): a patch of burning ground. FireSpread, the incendiary
  grenade's (ggrenade.cpp), lays a grid of them around a spot: each one
  burns what touches it twice a second for its pev->dmg, scorches the floor
  once it has landed and swallows the flames that overlap it (taking over
  their remaining time), goes out in water and fades after 20 seconds.

*/
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "soundent.h"
#include "decals.h"

class CFire : public CSprite
{
public:
	void Spawn( void );
	void Precache( void );
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return "incinerated"; }
	const char *GetKillDesc( void ) { return "a fire field"; }

	void EXPORT TimerThink( void );
	void EXPORT HurtTouch( CBaseEntity *pOther );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	EHANDLE	m_hOwner;
	float	m_flStartTime;
};

LINK_ENTITY_TO_CLASS( flame, CFire );

TYPEDESCRIPTION	CFire::m_SaveData[] =
{
	DEFINE_FIELD( CFire, m_hOwner, FIELD_EHANDLE ),
	DEFINE_FIELD( CFire, m_flStartTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CFire, CSprite );

void CFire::Spawn( void )
{
	pev->renderamt = 255;
	pev->rendermode = kRenderTransAdd;
	pev->scale = 1.5;
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_TOSS;
	pev->effects = 0;
	pev->frame = 0;
	pev->framerate = RANDOM_FLOAT( 9, 11 );

	Precache( );
	SET_MODEL( ENT( pev ), "sprites/fire.spr" );
	UTIL_SetSize( pev, Vector( -32, -32, -64 ), Vector( 32, 32, 0 ) );
	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;

	m_flStartTime = gpGlobals->time;
	SetThink( TimerThink );
	SetTouch( HurtTouch );
	pev->nextthink = gpGlobals->time + 0.1;
	pev->dmgtime = gpGlobals->time;
}

void CFire::Precache( void )
{
	PRECACHE_MODEL( "sprites/fire.spr" );
}

void CFire::TimerThink( void )
{
	AnimateThink( );

	if ( UTIL_PointContents( pev->origin ) == CONTENTS_SOLID )
		pev->origin.z += 65;

	if ( m_flStartTime + 20 < gpGlobals->time )
	{
		SUB_StartFadeOut( );
		pev->rendermode = kRenderTransAdd;
	}

	if ( pev->waterlevel > 0 )
	{
		// put out
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( (pev->dmg - 50) * .80 ); // scale * 10
			WRITE_BYTE( 12  ); // framerate
		MESSAGE_END();

		UTIL_Remove( this );
		return;
	}

	// landed: scorch the floor once and take over the flames this one covers
	if ( !( pev->spawnflags & 1 ) && ( pev->flags & FL_ONGROUND ) )
	{
		TraceResult tr;

		UTIL_TraceLine( pev->origin, pev->origin - Vector( 0, 0, 256 ), ignore_monsters, ENT( pev ), &tr );

		if ( RANDOM_FLOAT( 0, 1 ) < 0.5 )
			UTIL_DecalTrace( &tr, DECAL_SCORCH1 );
		else
			UTIL_DecalTrace( &tr, DECAL_SCORCH2 );

		pev->spawnflags |= 1;

		CBaseEntity *pEntity = NULL;
		while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 30 ) ) != NULL )
		{
			if ( pEntity != this && FClassnameIs( pEntity->pev, STRING( pev->classname ) ) )
			{
				m_flStartTime += 20 - ( gpGlobals->time - ( (CFire *)pEntity )->m_flStartTime );
				pEntity->pev->spawnflags |= 1;
				pEntity->Killed( pev, GIB_NEVER );
			}
		}
	}

	if ( pev->dmgtime < gpGlobals->time )
		gpGlobals->force_retouch = 1;
}

void CFire::HurtTouch( CBaseEntity *pOther )
{
	if ( !pOther->pev->takedamage )
		return;

	if ( pev->dmgtime > gpGlobals->time )
	{
		if ( gpGlobals->time != pev->pain_finished )
		{// too early to hurt again, and not same frame with a different entity
			return;
		}
	}

	entvars_t *pevOwner = pev;
	if ( m_hOwner != NULL )
		pevOwner = m_hOwner->pev;

	pOther->TakeDamage( pev, pevOwner, pev->dmg, DMG_SLOWBURN | DMG_NEVERGIB );

	pev->pain_finished = gpGlobals->time;
	pev->dmgtime = gpGlobals->time + 0.5;
}

// where FireSpread puts its flames, around the spot (name ours)
static Vector g_vecFireSpread[] =
{
	Vector( -64, 128, 0 ),
	Vector( 0, 128, 0 ),
	Vector( 64, 128, 0 ),
	Vector( -128, 64, 0 ),
	Vector( -64, 64, 0 ),
	Vector( 0, 64, 0 ),
	Vector( 64, 64, 0 ),
	Vector( 128, 64, 0 ),
	Vector( -128, 0, 0 ),
	Vector( -64, 0, 0 ),
	Vector( 0, 0, 0 ),
	Vector( 64, 0, 0 ),
	Vector( 128, 0, 0 ),
	Vector( -128, -64, 0 ),
	Vector( -64, -64, 0 ),
	Vector( 0, -64, 0 ),
	Vector( 64, -64, 0 ),
	Vector( 128, -64, 0 ),
	Vector( -64, -128, 0 ),
	Vector( 0, -128, 0 ),
	Vector( 64, -128, 0 ),
};

//=========================================================
// FireSpread - lay flames burning flDamage around
// vecOrigin, on every spot of the grid it can see (name ours)
//=========================================================
void FireSpread( const Vector &vecOrigin, float flDamage, entvars_t *pevOwner, edict_t *pentIgnore )
{
	if ( UTIL_PointContents( vecOrigin ) == CONTENTS_WATER )
		return;

	TraceResult tr;

	UTIL_TraceLine( vecOrigin, vecOrigin - Vector( 0, 0, 2048 ), ignore_monsters, NULL, &tr );
	if ( tr.flFraction < 1.0 && !tr.fInWater )
		CSoundEnt::InsertSound( bits_SOUND_DANGER, tr.vecEndPos, 256, 20 );

	for ( int i = 0; i < ARRAYSIZE( g_vecFireSpread ); i++ )
	{
		Vector vecSpot = vecOrigin + g_vecFireSpread[i];

		if ( vecOrigin != vecSpot )
		{
			UTIL_TraceLine( vecOrigin, vecSpot, ignore_monsters, pentIgnore, &tr );
			if ( tr.flFraction < 1.0 || tr.fInWater )
				continue;
		}

		CFire *pFire = (CFire *)CBaseEntity::Create( "flame", vecSpot, g_vecZero, NULL );
		if ( pevOwner && ENT( pevOwner ) && !ENT( pevOwner )->free )
			pFire->m_hOwner.Set( ENT( pevOwner ) );
		pFire->pev->dmg = flDamage;
	}
}

#endif
