/***
*
*	Scientist Slaughterhouse: the black hole.
*	The file name and the names marked "ours" are ours; class, entity and
*	exported member names come from the binary.
*
****/
/*

===== black_hole.cpp ========================================================

  black_hole (CBlackHole): a sprite that fades in over 2 seconds, then for
  24 seconds pulls everything within 512 units towards itself (140 units/s
  per tenth of a second), hurts what comes within 256 units (30 damage a
  tenth) and gibs whatever touches it (300 damage); gibs that touch it
  vanish. It shakes the screen and sends an implosion every second and
  throws random beams around. Its state lives in pev: scale (the fade-in),
  frags (seconds since it opened), pain_finished (next implosion) and
  dmgtime (next beam).

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "customentity.h"
#include "effects.h"
#include "soundent.h"
#include "player.h"

class CBlackHole : public CSprite
{
public:
	void Spawn( void );
	void Precache( void );
	const char *GetKillDesc( void ) { return "a black hole"; }

	void EXPORT SuckThink( void );
	void Suck( void );	// name ours: pulls and hurts what is near
	void EXPORT HurtTouch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( black_hole, CBlackHole );

void CBlackHole::Spawn( void )
{
	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;
	pev->rendermode = kRenderTransAlpha;

	Precache( );
	SET_MODEL( ENT( pev ), "sprites/blackhole.spr" );
	UTIL_SetSize( pev, Vector( -8, -8, -8 ), Vector( 8, 8, 8 ) );
	pev->framerate = 14;

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
	TurnOn( );

	SetTouch( HurtTouch );
	SetThink( SuckThink );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBlackHole::Precache( void )
{
	PRECACHE_MODEL( "sprites/blackhole.spr" );
	PRECACHE_MODEL( "sprites/obeam1.spr" );
	PRECACHE_SOUND( "ambience/particle_suck2.wav" );
	PRECACHE_SOUND( "ambience/alienwind1.wav" );
}

void CBlackHole::SuckThink( void )
{
	AnimateThink( );

	if ( pev->scale < 1 )
	{
		// fading in
		if ( pev->scale == 0 )
		{
			EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "ambience/particle_suck2.wav", 1, ATTN_NONE, 0, PITCH_NORM );
			CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin, 512, 10 );
		}

		pev->scale += 0.05;
		pev->renderamt = pev->scale * 255;

		if ( pev->scale >= 1 )
		{
			pev->scale = 1;
			pev->renderamt = 255;
			EMIT_SOUND_DYN( ENT( pev ), CHAN_STATIC, "ambience/alienwind1.wav", 1, ATTN_NONE, 0, PITCH_NORM );
			pev->dmgtime = 0;
			pev->pain_finished = 0;
		}
	}
	else if ( pev->pain_finished <= gpGlobals->time )
	{
		pev->frags += 1;
		if ( pev->frags > 24 )
		{
			// time is up: fade out
			STOP_SOUND( ENT( pev ), CHAN_STATIC, "ambience/alienwind1.wav" );
			Expand( 0, 200 );
			return;
		}

		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_IMPLOSION );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_BYTE( 255 );	// radius
			WRITE_BYTE( RANDOM_LONG( 160, 255 ) );	// count
			WRITE_BYTE( 10 );	// life
		MESSAGE_END();

		pev->pain_finished = gpGlobals->time + 1;
		UTIL_ScreenShake( pev->origin, 500, 0.2, 2, 768 );
	}
	else if ( pev->dmgtime <= gpGlobals->time )
	{
		// a random beam
		pev->dmgtime = gpGlobals->time + RANDOM_FLOAT( 0.2, 2 );

		Vector vecDir = Vector( RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ) ) * RANDOM_FLOAT( 512, 1024 );

		CBeam *pBeam = CBeam::BeamCreate( "sprites/obeam1.spr", RANDOM_LONG( 20, 40 ) );
		pBeam->PointEntInit( pev->origin + vecDir, ENTINDEX( edict() ) );
		pBeam->SetNoise( RANDOM_LONG( 10, 32 ) );
		pBeam->SetFlags( BEAM_FSOLID );
		pBeam->SetThink( SUB_Remove );
		pBeam->pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.5, 5 );
	}

	if ( pev->scale == 1 )
		Suck( );
}

void CBlackHole::Suck( void )
{
	CBaseEntity *pEntity = NULL;

	while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 512 ) ) != NULL )
	{
		Vector vecDelta = pev->origin - pEntity->pev->origin;

		if ( pEntity->CanBeRepulsed( true ) )
		{
			// a player holding the vortex launcher isn't pulled (nor hurt)
			if ( pEntity->IsPlayer() && ((CBasePlayer *)pEntity)->m_pActiveItem
				&& !strcmp( STRING( ((CBasePlayer *)pEntity)->m_pActiveItem->pev->classname ), "weapon_vortexlauncher" ) )
				continue;

			pEntity->pev->velocity = pEntity->pev->velocity + vecDelta.Normalize() * 140;
		}

		if ( pEntity->pev->takedamage && vecDelta.Length() < 256 )
		{
			entvars_t *pevOwner = pev;
			if ( pev->owner )
				pevOwner = VARS( pev->owner );
			pEntity->TakeDamage( pev, pevOwner, 30, DMG_FALL );
		}
	}
}

void CBlackHole::HurtTouch( CBaseEntity *pOther )
{
	if ( pOther->pev->takedamage )
	{
		entvars_t *pevOwner = pev;
		if ( pev->owner )
			pevOwner = VARS( pev->owner );
		pOther->TakeDamage( pev, pevOwner, 300, DMG_ALWAYSGIB | DMG_ENERGYBEAM );
		return;
	}

	if ( FClassnameIs( pOther->pev, "gib" ) )
	{
		if ( pOther->pev->iuser1 == 2 )
			PLAYBACK_EVENT_FULL( 0, pOther->edict(), g_iBloodSprayEvent, 0, (float *)&pev->origin, (float *)&pev->angles, 0, 0, 0, 0, 1, 0 );
		UTIL_Remove( pOther );
	}
}
