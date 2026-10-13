/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//=========================================================
// decorations: props that sit in the level, animate, gib
// and react to the player; the embryos in their tubes, the
// lab clutter, the asteroids and space debris, and the
// training bot
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "effects.h"
#include "soundent.h"
#include "decals.h"
#include "weapons.h"

//=========================================================
// CDecore - the base of the decorations: a monster that
// stands in place, animates and notices players touching it
//=========================================================
class CDecore : public CBaseMonster
{
public:
	void KeyValue( KeyValueData *pkvd );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	// a player touched the decoration (slot 139)
	virtual int PlayerTouch( CBaseEntity *pOther );

	void DecoreInit( void );
	void EXPORT DecoreTouch( CBaseEntity *pOther );
	void EXPORT Animate( void );

	int m_iAnimate;		// +0x270
	int m_maxFrame;		// +0x274
	int m_iUnused[3];	// +0x278
	int m_fAnimEnd;		// +0x284: CBigFootCrush waits for it, nothing sets it
	int m_iUnused2;		// +0x288
};

// ==== decore part A: CDecore .. CBigFootCrush (fork A) ====

void CDecore::DecoreInit( void )
{
	pev->takedamage = DAMAGE_YES;
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_BBOX;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
	SetTouch( DecoreTouch );
	pev->nextthink = gpGlobals->time + 0.1;
	SetThink( Animate );
	pev->health = 20;
	ResetSequenceInfo( );
}

void CDecore::DecoreTouch( CBaseEntity *pOther )
{
	if ( pOther && ( pOther->pev->flags & FL_CLIENT ) )
		PlayerTouch( pOther );
}

void CDecore::Animate( void )
{
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CDecore::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "frame" ) )
	{
		pev->frame = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pev->frame = 0;	// sic: every other key is dropped
}


//=========================================================
// decore_baboon - a dying baboon twitching on the ground
//=========================================================
class CDecoreBaboon : public CDecore
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify( void );
	void GibMonster( void );
	int PlayerTouch( CBaseEntity *pOther );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
};
LINK_ENTITY_TO_CLASS( decore_baboon, CDecoreBaboon );

TYPEDESCRIPTION	CDecoreBaboon::m_SaveData[] =
{
	DEFINE_FIELD( CDecoreBaboon, m_maxFrame, FIELD_INTEGER ),
	DEFINE_FIELD( CDecoreBaboon, m_iAnimate, FIELD_INTEGER ),
};

void CDecoreBaboon::Spawn( void )
{
	Precache( );
	SET_MODEL( ENT( pev ), "models/baboon.mdl" );
	DecoreInit( );
	pev->health = 30;
	SetSequenceByName( "deadtwitch" );
	ResetSequenceInfo( );
	m_bloodColor = BLOOD_COLOR_RED;
}

void CDecoreBaboon::GibMonster( void )
{
	SpawnGib( "models/hgibs.mdl", 1, 10, 0 );
	SpawnGib( "models/hgibs.mdl", 1, 10, 0 );
	SpawnGib( "models/hgibs.mdl", 1, 10, 0 );
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "ambience/monkydie.wav", 1, 0.8, 0, PITCH_NORM );
}

int CDecoreBaboon::Classify( void )
{
	return CLASS_NONE;
}

void CDecoreBaboon::Precache( void )
{
	PRECACHE_MODEL( "models/baboon.mdl" );
	PRECACHE_MODEL( "models/hgibs.mdl" );
	PRECACHE_SOUND( "ambience/monkydie.wav" );
}

int CDecoreBaboon::PlayerTouch( CBaseEntity *pOther )
{
	return 0;
}

IMPLEMENT_SAVERESTORE( CDecoreBaboon, CDecore );


//=========================================================
// decore_pipes - pipes that start on a random frame
//=========================================================
class CDecorePipes : public CDecore
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify( void );
	int PlayerTouch( CBaseEntity *pOther );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
};
LINK_ENTITY_TO_CLASS( decore_pipes, CDecorePipes );

TYPEDESCRIPTION	CDecorePipes::m_SaveData[] =
{
	DEFINE_FIELD( CDecorePipes, m_maxFrame, FIELD_INTEGER ),
	DEFINE_FIELD( CDecorePipes, m_iAnimate, FIELD_INTEGER ),
};

void CDecorePipes::Spawn( void )
{
	Precache( );
	SET_MODEL( ENT( pev ), "models/pipes.mdl" );
	pev->takedamage = DAMAGE_NO;
	m_iAnimate = 1;
	m_maxFrame = 255;
	ResetSequenceInfo( );
	pev->frame = rand( ) % m_maxFrame;
	DecoreInit( );	// sic: makes the pipes take damage after all
}

int CDecorePipes::Classify( void )
{
	return CLASS_NONE;
}

void CDecorePipes::Precache( void )
{
	PRECACHE_MODEL( "models/pipes.mdl" );
}

int CDecorePipes::PlayerTouch( CBaseEntity *pOther )
{
	return 0;
}

IMPLEMENT_SAVERESTORE( CDecorePipes, CDecore );


//=========================================================
// decore_mushroom - a mushroom that puffs spores when a
// player steps on it and bursts when it's hurt
//=========================================================
int g_sModelIndexGibMushroom;	// sprites/gibmushroom.spr

class CDecoreMushroom : public CDecore
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify( void );
	int PlayerTouch( CBaseEntity *pOther );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
};
LINK_ENTITY_TO_CLASS( decore_mushroom, CDecoreMushroom );

void CDecoreMushroom::Spawn( void )
{
	Precache( );
	SET_MODEL( ENT( pev ), "models/mushroom.mdl" );
	DecoreInit( );
	m_bloodColor = DONT_BLEED;
}

int CDecoreMushroom::Classify( void )
{
	return CLASS_NONE;
}

void CDecoreMushroom::Precache( void )
{
	PRECACHE_MODEL( "models/mushroom.mdl" );
	PRECACHE_SOUND( "mushroom/mushroom_hit2.wav" );
	PRECACHE_SOUND( "mushroom/mushroom_hit3.wav" );
	PRECACHE_SOUND( "mushroom/mushroom_pop1.wav" );
	g_sModelIndexGibMushroom = PRECACHE_MODEL( "sprites/gibmushroom.spr" );
}

int CDecoreMushroom::PlayerTouch( CBaseEntity *pOther )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SPRITE_SPRAY );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );
		WRITE_COORD( 1 );
		WRITE_SHORT( g_sModelIndexGibMushroom );
		WRITE_BYTE( 16 );	// count
		WRITE_BYTE( 40 );	// speed
		WRITE_BYTE( 100 );	// noise
	MESSAGE_END( );

	if ( RANDOM_LONG( 0, 1 ) )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "mushroom/mushroom_hit2.wav", 1, 0.8, 0, PITCH_NORM );
	else
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "mushroom/mushroom_hit3.wav", 1, 0.8, 0, PITCH_NORM );

	SetTouch( NULL );
	return TRUE;
}

int CDecoreMushroom::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SPRITE_SPRAY );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );
		WRITE_COORD( 1 );
		WRITE_SHORT( g_sModelIndexGibMushroom );
		WRITE_BYTE( 16 );	// count
		WRITE_BYTE( 40 );	// speed
		WRITE_BYTE( 100 );	// noise
	MESSAGE_END( );

	EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "mushroom/mushroom_pop1.wav", 1, 0.8, 0, PITCH_NORM );

	SetThink( SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;
	return 0;
}


//=========================================================
// decore_mushroom2 - the other mushroom
//=========================================================
class CDecoreMushroom2 : public CDecore
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify( void );
	int PlayerTouch( CBaseEntity *pOther );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
};
LINK_ENTITY_TO_CLASS( decore_mushroom2, CDecoreMushroom2 );

void CDecoreMushroom2::Spawn( void )
{
	Precache( );
	SET_MODEL( ENT( pev ), "models/mushroom2.mdl" );
	DecoreInit( );
	m_bloodColor = DONT_BLEED;
}

int CDecoreMushroom2::Classify( void )
{
	return CLASS_NONE;
}

void CDecoreMushroom2::Precache( void )
{
	PRECACHE_MODEL( "models/mushroom2.mdl" );
	PRECACHE_SOUND( "mushroom/mushroom_hit1.wav" );
	PRECACHE_SOUND( "mushroom/mushroom_hit2.wav" );
	PRECACHE_SOUND( "mushroom/mushroom_pop2.wav" );
	g_sModelIndexGibMushroom = PRECACHE_MODEL( "sprites/gibmushroom.spr" );
}

int CDecoreMushroom2::PlayerTouch( CBaseEntity *pOther )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SPRITE_SPRAY );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );
		WRITE_COORD( 1 );
		WRITE_SHORT( g_sModelIndexGibMushroom );
		WRITE_BYTE( 16 );	// count
		WRITE_BYTE( 40 );	// speed
		WRITE_BYTE( 100 );	// noise
	MESSAGE_END( );

	// sic: both picks play hit1; hit2 is precached and never played
	if ( RANDOM_LONG( 0, 1 ) )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "mushroom/mushroom_hit1.wav", 1, 0.8, 0, PITCH_NORM );
	else
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "mushroom/mushroom_hit1.wav", 1, 0.8, 0, PITCH_NORM );

	SetTouch( NULL );
	return TRUE;
}

int CDecoreMushroom2::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SPRITE_SPRAY );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );
		WRITE_COORD( 1 );
		WRITE_SHORT( g_sModelIndexGibMushroom );
		WRITE_BYTE( 16 );	// count
		WRITE_BYTE( 40 );	// speed
		WRITE_BYTE( 100 );	// noise
	MESSAGE_END( );

	EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "mushroom/mushroom_pop2.wav", 1, 0.8, 0, PITCH_NORM );

	SetThink( SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;
	return 0;
}


//=========================================================
// decore_cactus - pricks players who walk into it, at most
// once a second, and breaks into pieces
//=========================================================
class CDecoreCactus : public CDecore
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify( void );
	void GibMonster( void );
	int PlayerTouch( CBaseEntity *pOther );

	float m_flNextHurt;
};
LINK_ENTITY_TO_CLASS( decore_cactus, CDecoreCactus );

void CDecoreCactus::Spawn( void )
{
	Precache( );
	SET_MODEL( ENT( pev ), "models/cactus.mdl" );
	DecoreInit( );
	pev->max_health = pev->health;
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 64 ) );
	m_flNextHurt = gpGlobals->time;
	m_bloodColor = BLOOD_COLOR_YELLOW;

	if ( DROP_TO_FLOOR( ENT( pev ) ) == 0 )
	{
		ALERT( at_error, "Item %s fell out of level at %f,%f,%f", STRING( pev->classname ), pev->origin.x, pev->origin.y, pev->origin.z );
		UTIL_Remove( this );
	}
}

int CDecoreCactus::Classify( void )
{
	return CLASS_NONE;
}

void CDecoreCactus::Precache( void )
{
	PRECACHE_MODEL( "models/cactus.mdl" );
	PRECACHE_MODEL( "models/cactusgib.mdl" );
}

int CDecoreCactus::PlayerTouch( CBaseEntity *pOther )
{
	if ( m_flNextHurt < gpGlobals->time )
	{
		m_flNextHurt = gpGlobals->time + 1;
		pOther->TakeDamage( pev, pev, 1, DMG_SLASH );
	}
	return TRUE;
}

void CDecoreCactus::GibMonster( void )
{
	for ( int i = 1; i < 16; i += 2 )
	{
		CBaseMonster *pGib = SpawnGib( "models/cactusgib.mdl", RANDOM_FLOAT( 0.5, 1.5 ), i % 3, 0 );
		if ( pGib )
			UTIL_SetOrigin( pGib->pev, pev->origin + Vector( 0, 0, i ) );
	}
}


//=========================================================
// decore_prickle - a small prickly plant
//=========================================================
class CDecorePrickle : public CDecore
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify( void );

	float m_flNextHurt;	// set, never read
};
LINK_ENTITY_TO_CLASS( decore_prickle, CDecorePrickle );

void CDecorePrickle::Spawn( void )
{
	Precache( );
	SET_MODEL( ENT( pev ), "models/prickle.mdl" );
	DecoreInit( );
	UTIL_SetSize( pev, Vector( -8, -8, 0 ), Vector( 8, 8, 8 ) );
	m_flNextHurt = gpGlobals->time;
	pev->skin = RANDOM_LONG( 0, 1 );
	m_bloodColor = BLOOD_COLOR_YELLOW;
	pev->takedamage = DAMAGE_YES;

	if ( DROP_TO_FLOOR( ENT( pev ) ) == 0 )
	{
		ALERT( at_error, "Item %s fell out of level at %f,%f,%f", STRING( pev->classname ), pev->origin.x, pev->origin.y, pev->origin.z );
		UTIL_Remove( this );
	}
}

int CDecorePrickle::Classify( void )
{
	return CLASS_NONE;
}

void CDecorePrickle::Precache( void )
{
	PRECACHE_MODEL( "models/prickle.mdl" );
}


//=========================================================
// decore_gutspile, decore_bodygib, decore_hatgib - human
// remains that bleed when shot and, once, smell for the
// monsters ten seconds after they spawn
//=========================================================
class CDecoreHumanGibsParts : public CDecore
{
public:
	CDecoreHumanGibsParts( ) { }

	void Spawn( void );
	int Classify( void );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	// the model of this kind of remains (slot 140)
	virtual void SetGibModel( void ) = 0;

private:
	void EXPORT GutSmell( void );
};

void CDecoreHumanGibsParts::GutSmell( void )
{
	CSoundEnt::InsertSound( bits_SOUND_MEAT, pev->origin, 400, 10 );
}

class CDecoreGutspile : public CDecoreHumanGibsParts
{
public:
	void Precache( void );
	void SetGibModel( void );
};
LINK_ENTITY_TO_CLASS( decore_gutspile, CDecoreGutspile );

void CDecoreHumanGibsParts::Spawn( void )
{
	Precache( );
	SetGibModel( );
	DecoreInit( );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 40 ) );
	pev->solid = SOLID_NOT;
	m_bloodColor = BLOOD_COLOR_RED;
	m_iAnimate = 0;
	m_maxFrame = 0;
	pev->health = 400;

	if ( DROP_TO_FLOOR( ENT( pev ) ) == 0 )
	{
		ALERT( at_error, "Item %s fell out of level at %f,%f,%f", STRING( pev->classname ), pev->origin.x, pev->origin.y, pev->origin.z );
		UTIL_Remove( this );
		return;
	}

	SetThink( GutSmell );
	pev->nextthink = gpGlobals->time + 10;
}

int CDecoreHumanGibsParts::Classify( void )
{
	return CLASS_NONE;
}

int CDecoreHumanGibsParts::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( RANDOM_FLOAT( 0, 1 ) < 0.5 )
		UTIL_BloodDrips( pev->origin, UTIL_RandomBloodVector( ), BLOOD_COLOR_RED, 10 );

	CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
	return 1;
}

void CDecoreGutspile::SetGibModel( void )
{
	SET_MODEL( ENT( pev ), "models/gutspile.mdl" );
}

void CDecoreGutspile::Precache( void )
{
	PRECACHE_MODEL( "models/gutspile.mdl" );
}


class CDecoreBodygib : public CDecoreHumanGibsParts
{
public:
	void Precache( void );
	void SetGibModel( void );
};
LINK_ENTITY_TO_CLASS( decore_bodygib, CDecoreBodygib );

void CDecoreBodygib::SetGibModel( void )
{
	SET_MODEL( ENT( pev ), "models/bodygib.mdl" );
}

void CDecoreBodygib::Precache( void )
{
	PRECACHE_MODEL( "models/bodygib.mdl" );
}


class CDecoreHatgib : public CDecoreHumanGibsParts
{
public:
	void Precache( void );
	void SetGibModel( void );
};
LINK_ENTITY_TO_CLASS( decore_hatgib, CDecoreHatgib );

void CDecoreHatgib::SetGibModel( void )
{
	SET_MODEL( ENT( pev ), "models/hatgib.mdl" );
}

void CDecoreHatgib::Precache( void )
{
	PRECACHE_MODEL( "models/hatgib.mdl" );
}


//=========================================================
// decore_icebeak - the beak frozen in a block of ice
//=========================================================
class CDecoreIcebeak : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
};
LINK_ENTITY_TO_CLASS( decore_icebeak, CDecoreIcebeak );

void CDecoreIcebeak::Precache( void )
{
	PRECACHE_MODEL( "models/icebeak.mdl" );
}

void CDecoreIcebeak::Spawn( void )
{
	pev->model = MAKE_STRING( "models/icebeak.mdl" );
	pev->classname = MAKE_STRING( "decore_icebeak" );
	SET_MODEL( ENT( pev ), "models/icebeak.mdl" );
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NOCLIP;
	pev->effects = 0;
	pev->takedamage = DAMAGE_NO;
}


//=========================================================
// decore_ice - a block of ice, with a beak inside when
// "beakinside" is set; it shatters when used
//=========================================================
class CIce : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	int Classify( void );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int m_iBeakInside;
	CBaseEntity *pBeak;

private:
	void EXPORT SpawnIceBeak( void );
	void EXPORT Melt( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

TYPEDESCRIPTION	CIce::m_SaveData[] =
{
	DEFINE_FIELD( CIce, m_iBeakInside, FIELD_INTEGER ),
	DEFINE_FIELD( CIce, pBeak, FIELD_CLASSPTR ),
};

void CIce::SpawnIceBeak( void )
{
	if ( m_iBeakInside == 1 )
	{
		pev->renderamt = 180;
		pBeak = GetClassPtr( (CDecoreIcebeak *)NULL );
		pBeak->Spawn( );
		pBeak->pev->origin = pev->origin;
		pBeak->pev->angles = pev->angles;
	}
}

void CIce::Melt( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( m_iBeakInside && pBeak )
		UTIL_Remove( pBeak );

	SpawnGib( "models/ceilinggibs.mdl", 1, 0, 0 );
	SpawnGib( "models/ceilinggibs.mdl", 1, 0, 0 );
	SpawnGib( "models/ceilinggibs.mdl", 1, 0, 0 );
	SpawnGib( "models/ceilinggibs.mdl", 1, 0, 0 );
	UTIL_Remove( this );
}

LINK_ENTITY_TO_CLASS( decore_ice, CIce );

void CIce::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "beakinside" ) )
	{
		m_iBeakInside = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

void CIce::Spawn( void )
{
	Precache( );
	SET_MODEL( ENT( pev ), "models/ice.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 40 ) );
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_NO;
	pev->health = 1;
	pev->takedamage = DAMAGE_NO;
	pBeak = NULL;
	pev->renderamt = 200;
	pev->rendermode = kRenderTransTexture;

	if ( DROP_TO_FLOOR( ENT( pev ) ) == 0 )
	{
		ALERT( at_error, "Item %s fell out of level at %f,%f,%f", STRING( pev->classname ), pev->origin.x, pev->origin.y, pev->origin.z );
		UTIL_Remove( this );
		return;
	}

	SetTouch( NULL );
	SetUse( Melt );
	SetThink( SpawnIceBeak );
	pev->nextthink = gpGlobals->time + 0.1;
}

int CIce::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return 0;
}

int CIce::Classify( void )
{
	return CLASS_NONE;
}

void CIce::Precache( void )
{
	PRECACHE_MODEL( "models/ceilinggibs.mdl" );
	PRECACHE_MODEL( "models/ice.mdl" );
	if ( m_iBeakInside )
		UTIL_PrecacheOther( "decore_icebeak" );
}

IMPLEMENT_SAVERESTORE( CIce, CBaseMonster );


//=========================================================
// decore_swampplants - swamp plants
//=========================================================
class CDecoreSwampPlants : public CDecore
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify( void );
	int PlayerTouch( CBaseEntity *pOther );
};
LINK_ENTITY_TO_CLASS( decore_swampplants, CDecoreSwampPlants );

void CDecoreSwampPlants::Spawn( void )
{
	Precache( );
	SET_MODEL( ENT( pev ), "models/swampstuff.mdl" );
	DecoreInit( );
	pev->solid = SOLID_NOT;
}

int CDecoreSwampPlants::Classify( void )
{
	return CLASS_NONE;
}

void CDecoreSwampPlants::Precache( void )
{
	PRECACHE_MODEL( "models/swampstuff.mdl" );
}

int CDecoreSwampPlants::PlayerTouch( CBaseEntity *pOther )
{
	return 0;
}


//=========================================================
// decore_goldskull - the golden monkey skull; it opens
// when used
//=========================================================
class CDecoreMonkeySkull : public CDecore
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify( void );
	int PlayerTouch( CBaseEntity *pOther );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

private:
	void EXPORT MonkeyUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

TYPEDESCRIPTION	CDecoreMonkeySkull::m_SaveData[] =
{
	DEFINE_FIELD( CDecoreMonkeySkull, m_maxFrame, FIELD_INTEGER ),
	DEFINE_FIELD( CDecoreMonkeySkull, m_iAnimate, FIELD_INTEGER ),
};

void CDecoreMonkeySkull::MonkeyUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetSequenceByName( "opening" );
	SetUse( NULL );
}

LINK_ENTITY_TO_CLASS( decore_goldskull, CDecoreMonkeySkull );

void CDecoreMonkeySkull::Spawn( void )
{
	Precache( );
	SET_MODEL( ENT( pev ), "models/goldskull.mdl" );
	DecoreInit( );
	pev->solid = SOLID_NOT;
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	SetUse( MonkeyUse );
	SetThink( NULL );
	pev->takedamage = DAMAGE_NO;
}

int CDecoreMonkeySkull::Classify( void )
{
	return CLASS_NONE;
}

void CDecoreMonkeySkull::Precache( void )
{
	PRECACHE_MODEL( "models/goldskull.mdl" );
}

int CDecoreMonkeySkull::PlayerTouch( CBaseEntity *pOther )
{
	return 0;
}

IMPLEMENT_SAVERESTORE( CDecoreMonkeySkull, CDecore );


//=========================================================
// decore_foot - the renesaur's foot, hidden until used;
// then it stamps down
//=========================================================
class CBigFootCrush : public CDecore
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify( void );
	int PlayerTouch( CBaseEntity *pOther );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

private:
	void EXPORT BigFootUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT CheckAnimEnd( void );
};

TYPEDESCRIPTION	CBigFootCrush::m_SaveData[] =
{
	DEFINE_FIELD( CBigFootCrush, m_maxFrame, FIELD_INTEGER ),
	DEFINE_FIELD( CBigFootCrush, m_iAnimate, FIELD_INTEGER ),
};

void CBigFootCrush::BigFootUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetSequenceByName( "footcrush" );
	SetUse( NULL );
	pev->effects &= ~EF_NODRAW;
	SetThink( CheckAnimEnd );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBigFootCrush::CheckAnimEnd( void )
{
	// sic: nothing sets m_fAnimEnd, so the foot stays
	if ( m_fAnimEnd )
		SetThink( SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;
}

LINK_ENTITY_TO_CLASS( decore_foot, CBigFootCrush );

void CBigFootCrush::Spawn( void )
{
	Precache( );
	SET_MODEL( ENT( pev ), "models/renesaurfoot.mdl" );
	UTIL_SetSize( pev, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ) );
	UTIL_SetOrigin( pev, pev->origin );
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	SetUse( BigFootUse );
	SetThink( NULL );
	SetTouch( NULL );
	pev->effects = EF_NODRAW;
	pev->takedamage = DAMAGE_NO;
}

int CBigFootCrush::Classify( void )
{
	return CLASS_NONE;
}

void CBigFootCrush::Precache( void )
{
	PRECACHE_MODEL( "models/renesaurfoot.mdl" );
}

int CBigFootCrush::PlayerTouch( CBaseEntity *pOther )
{
	return 0;
}

IMPLEMENT_SAVERESTORE( CBigFootCrush, CDecore );

// ==== end of part A ====

// ==== decore part B: CXenomeEmbryo .. CSpaceDebris (fork B) ====

// CDecore's own defaults sit here in retail, between the tube embryo and
// the lab stuff.
int CDecore::PlayerTouch( CBaseEntity *pOther )
{
	return 0;
}

int CDecore::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

// ==== end of part B ====

// ==== decore part C: CDummyBot (fork C) ====

// ==== end of part C ====
