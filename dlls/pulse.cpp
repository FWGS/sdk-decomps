/***
*
*	Scientist Slaughterhouse: the laser pulse.
*	The file name and the names marked "ours" are ours; class, entity and
*	exported member names come from the binary.
*
****/
/*

===== pulse.cpp ========================================================

  laser_pulse (CLaserPulse, declared in weapons.h): a glowing sprite that
  flies straight and hurts what it touches (pev->dmg, damage type
  pev->iuser2). The shotgun's laser rounds, the laser pistol and several
  monsters fire it through CLaserPulse::Shoot; special_laser (lpistol.cpp)
  derives from it.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"

#define SF_PULSE_EXPLODE		4	// names ours: explodes on touch (CGrenade::ExplodeTouch)
#define SF_PULSE_EXDETONATE		24	// with a color: a CGrenade::ExDetonate blast instead

// trail sprites of the four colors (PrecacheColor); names ours, chosen so that VC6 lays them out in
// .bss as retail has them (red, blue, green, light red: the order follows the names' hash)
short g_sTrailSpriteRed;
short g_sTrailSpriteBlue;
short g_sTrailSpriteGreen;
short g_sTrailSpriteLightRed;

LINK_ENTITY_TO_CLASS( laser_pulse, CLaserPulse );

const char *CLaserPulse::GetKillVerb( BOOL fGibbed, int &iHitGroup )
{
	if ( pev->spawnflags & SF_PULSE_EXPLODE )
	{
		iHitGroup = 0;
		return DamageVerb( DMG_BLAST, fGibbed );
	}

	if ( !fGibbed )
		return "shot";

	iHitGroup = 0;
	return DamageVerb( pev->iuser2, fGibbed );
}

void CLaserPulse::Spawn( void )
{
	Precache( );

	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_FLY;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;
	pev->movetype = MOVETYPE_FLY;	// sic

	SetTouch( LaserTouch );
	pev->iuser2 = DMG_ENERGYBEAM;

	// pev->weapons picks the color; the plain pulse is a glow with the laser beam as its trail
	short sTrail = g_sModelIndexLaser;

	switch ( pev->weapons )
	{
	case 1:
		if ( g_sTrailSpriteRed )
		{
			SET_MODEL( ENT( pev ), "sprites/laserglow_red.spr" );
			sTrail = g_sTrailSpriteRed;
		}
		else
			SET_MODEL( ENT( pev ), "sprites/glow01.spr" );
		break;
	case 2:
		if ( g_sTrailSpriteGreen )
		{
			SET_MODEL( ENT( pev ), "sprites/laserglow_green.spr" );
			sTrail = g_sTrailSpriteGreen;
		}
		else
			SET_MODEL( ENT( pev ), "sprites/glow01.spr" );
		break;
	case 3:
		if ( g_sTrailSpriteBlue )
		{
			SET_MODEL( ENT( pev ), "sprites/laserglow_blue.spr" );
			sTrail = g_sTrailSpriteBlue;
		}
		else
			SET_MODEL( ENT( pev ), "sprites/glow01.spr" );
		break;
	case 4:
		if ( g_sTrailSpriteLightRed )
		{
			SET_MODEL( ENT( pev ), "sprites/laserglow_n.spr" );
			sTrail = g_sTrailSpriteLightRed;
		}
		else
			SET_MODEL( ENT( pev ), "sprites/glow01.spr" );
		break;
	default:
		SET_MODEL( ENT( pev ), "sprites/glow01.spr" );
		break;
	}

	UTIL_SetSize( pev, g_vecZero, g_vecZero );

	// pev->iuser1: the trail's life in tenths of a second
	if ( pev->iuser1 > 0 )
	{
		int iWidth = pev->scale * 10;
		if ( sTrail != g_sModelIndexLaser )
			iWidth *= 2;
		BeamFollow( sTrail, pev->iuser1, iWidth, pev->rendercolor, 255 );
	}
}

void CLaserPulse::Precache( void )
{
	PRECACHE_MODEL( "sprites/glow01.spr" );
}

short CLaserPulse::PrecacheColor( int iColor )
{
	switch ( iColor )
	{
	case 1:
		PRECACHE_MODEL( "sprites/laserglow_red.spr" );
		return g_sTrailSpriteRed = PRECACHE_MODEL( "sprites/lasertrail_red.spr" );
	case 2:
		PRECACHE_MODEL( "sprites/laserglow_green.spr" );
		return g_sTrailSpriteGreen = PRECACHE_MODEL( "sprites/lasertrail_green.spr" );
	case 3:
		PRECACHE_MODEL( "sprites/laserglow_blue.spr" );
		return g_sTrailSpriteBlue = PRECACHE_MODEL( "sprites/ionbeam.spr" );
	case 4:
		PRECACHE_MODEL( "sprites/laserglow_n.spr" );
		return g_sTrailSpriteLightRed = PRECACHE_MODEL( "sprites/lred.spr" );
	}

	return g_sModelIndexLaser;
}

void CLaserPulse::LaserTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	SetThink( NULL );

	if ( pev->spawnflags & SF_PULSE_EXPLODE )
	{
		if ( ( pev->spawnflags & SF_PULSE_EXDETONATE ) && pev->weapons > 0 )
		{
			pev->rendercolor = pev->movedir;	// sic
			pev->movedir = pev->velocity.Normalize();
			SetThink( ExDetonate );
			pev->nextthink = gpGlobals->time + gpGlobals->frametime;
		}
		else
		{
			ExplodeTouch( pOther );
		}
		return;
	}

	TraceResult tr = UTIL_GetGlobalTrace( );

	if ( pOther->pev->takedamage )
	{
		entvars_t *pevOwner = VARS( pev->owner );

		ClearMultiDamage( );
		pOther->TraceAttack( pevOwner, pev->dmg, pev->velocity.Normalize(), &tr, pev->iuser2 );
		ApplyMultiDamage( pev, pevOwner );
	}
	else if ( UTIL_PointContents( pev->origin ) != CONTENTS_WATER )
	{
		UTIL_Sparks( pev->origin );
	}

	if ( pOther->IsBSPModel() )
		UTIL_GunshotDecalTrace( &tr, RANDOM_LONG( 0, 4 ) );

	UTIL_Remove( this );
}

CLaserPulse *CLaserPulse::Shoot( Vector vecOrigin, edict_t *pOwner, const Vector &vecColor, int iTrailLife, float flScale, int iColor )
{
	CLaserPulse *pPulse = GetClassPtr( (CLaserPulse *)NULL );

	UTIL_SetOrigin( pPulse->pev, vecOrigin );
	pPulse->pev->iuser1 = iTrailLife;
	pPulse->pev->owner = pOwner;
	pPulse->pev->rendercolor = vecColor;
	pPulse->pev->scale = flScale;
	pPulse->pev->weapons = iColor;
	pPulse->Spawn( );
	pPulse->pev->classname = MAKE_STRING( "laser_pulse" );

	return pPulse;
}
