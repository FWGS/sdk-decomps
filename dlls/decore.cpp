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

// ==== end of part A ====

// ==== decore part B: CXenomeEmbryo .. CSpaceDebris (fork B) ====

//=========================================================
// xenome embryo: a small creature that idles, chitters now
// and then, and bursts when used or hurt
//=========================================================
class CXenomeEmbryo : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify( void );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void GibMonster( void );

private:
	void EXPORT XenomeEmbryoGibUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT IdleSoundThink( void );
};

void CXenomeEmbryo::XenomeEmbryoGibUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	ALERT( at_console, "Using Embryo\n" );
	Killed( pev, GIB_ALWAYS );
}

void CXenomeEmbryo::IdleSoundThink( void )
{
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "xenome/xmbryo_idle1.wav", 1, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "xenome/xmbryo_idle2.wav", 1, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "xenome/xmbryo_idle3.wav", 1, ATTN_NORM );
		break;
	}

	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 1, 25 );
}

LINK_ENTITY_TO_CLASS( monster_xenome_embryo, CXenomeEmbryo );

void CXenomeEmbryo::Spawn( void )
{
	Precache( );

	pev->health = 20;
	pev->takedamage = DAMAGE_YES;
	m_bloodColor = 100;

	SET_MODEL( ENT(pev), "models/xmbryo.mdl" );
	UTIL_SetSize( pev, Vector( -6, -6, -6 ), Vector( 6, 6, 12 ) );

	pev->solid = SOLID_SLIDEBOX;
	m_flFieldOfView = -0.7;
	m_MonsterState = MONSTERSTATE_IDLE;
	pev->spawnflags |= SF_MONSTER_FALL_TO_GROUND;

	SetUse( XenomeEmbryoGibUse );
	SetThink( IdleSoundThink );
	pev->nextthink = gpGlobals->time + RANDOM_LONG( 0, 6 );
}

int CXenomeEmbryo::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, DMG_ALWAYSGIB );
}

void CXenomeEmbryo::GibMonster( void )
{
	SpawnGib( "models/xenomegibs.mdl", 0.3, 0, 0 );
	SpawnGib( "models/xenomegibs.mdl", 0.3, 1, 0 );
	SpawnGib( "models/xenomegibs.mdl", 0.3, 2, 0 );
	CBaseMonster::GibMonster( );
}

int CXenomeEmbryo::Classify( void )
{
	return CLASS_ALIEN_CRITTER;
}

void CXenomeEmbryo::Precache( void )
{
	PRECACHE_MODEL( "models/xmbryo.mdl" );
	PRECACHE_MODEL( "models/xenomegibs.mdl" );
	PRECACHE_SOUND( "xenome/xmbryo_idle1.wav" );
	PRECACHE_SOUND( "xenome/xmbryo_idle2.wav" );
	PRECACHE_SOUND( "xenome/xmbryo_idle3.wav" );
}


//=========================================================
// sitting tube mortar: a tube that sits on the floor and,
// touched or hurt, bursts into a spray of tube rockets
//=========================================================
class CSittingTubeMortar : public CDecore
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify( void );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	int PlayerTouch( CBaseEntity *pOther );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int m_iModelIndexTubeSuicide;
};
LINK_ENTITY_TO_CLASS( decore_sittingtubemortar, CSittingTubeMortar );

void CSittingTubeMortar::Spawn( void )
{
	Precache( );
	SET_MODEL( ENT(pev), "models/tubemortar.mdl" );
	DecoreInit( );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 48 ) );
	SetSequenceByName( "standing" );
	ResetSequenceInfo( );

	if ( DROP_TO_FLOOR( ENT(pev) ) == 0 )
	{
		ALERT( at_error, "Item %s fell out of level at %f,%f,%f", STRING( pev->classname ), pev->origin.x, pev->origin.y, pev->origin.z );
		UTIL_Remove( this );
	}
}

int CSittingTubeMortar::Classify( void )
{
	return CLASS_NONE;
}

void CSittingTubeMortar::Precache( void )
{
	m_iModelIndexTubeSuicide = PRECACHE_MODEL( "sprites/tubeguts.spr" );
	PRECACHE_MODEL( "models/tubemortar.mdl" );
	PRECACHE_SOUND( "tubequeen/mortarexplode1.wav" );
	PRECACHE_SOUND( "tubequeen/mortarexplode2.wav" );
	PRECACHE_SOUND( "tubequeen/mortarexplode3.wav" );
	PRECACHE_SOUND( "tube/tube_explodingdeath.wav" );
	UTIL_PrecacheOther( "tuberocket" );
}

int CSittingTubeMortar::PlayerTouch( CBaseEntity *pOther )
{
	Vector vecDir, vecSrc, vecAngles;

	EMIT_SOUND_DYN( ENT(pev), CHAN_BODY, "tube/tube_explodingdeath.wav", 1, ATTN_NORM, 0, PITCH_NORM );

	int iRockets = RANDOM_LONG( 6, 12 );
	for ( int i = 0; i < iRockets; i++ )
	{
		vecDir = m_vecEnemyLKP - pev->origin;
		vecDir = vecDir.Normalize( );
		vecAngles = UTIL_VecToAngles( vecDir );	// sic: never used
		vecSrc = pev->origin + vecDir * 25;
		vecSrc.z += 16;
		vecDir.z = vecDir.z * -1;

		CBaseEntity *pRocket = CBaseEntity::Create( "tuberocket", vecSrc, UTIL_VecToAngles( vecDir ), edict( ) );
		UTIL_MakeVectors( pRocket->pev->angles );
		pRocket->pev->velocity = gpGlobals->v_forward * 300;

		CBaseMonster *pMonster = pRocket->MyMonsterPointer( );
		if ( pMonster )
			pMonster->m_hEnemy = m_hEnemy;

		// sic: the rest goes through pMonster without testing it
		vecDir.x = RANDOM_FLOAT( -0.5, 0.5 );
		vecDir.y = RANDOM_FLOAT( -0.5, 0.5 );
		vecDir.z = RANDOM_FLOAT( 0.1, 0.5 );
		vecDir = vecDir.Normalize( );
		pMonster->pev->angles = UTIL_VecToAngles( vecDir );
		UTIL_MakeAimVectors( pMonster->pev->angles );
		pMonster->pev->velocity = gpGlobals->v_forward * 300;
		pMonster->pev->origin.x += RANDOM_FLOAT( -20, 20 );
		pMonster->pev->origin.y += RANDOM_FLOAT( -20, 20 );
		pMonster->pev->origin.z += RANDOM_FLOAT( 32, 64 );
		pMonster->pev->takedamage = DAMAGE_NO;
		pMonster->pev->movetype = MOVETYPE_TOSS;
		pMonster->pev->nextthink = gpGlobals->time + 0.3;

		if ( m_MonsterState == MONSTERSTATE_COMBAT )
		{
			pMonster->m_vecEnemyLKP = m_vecEnemyLKP;
			pMonster->m_hEnemy = m_hEnemy;
		}

		pMonster->m_flNextAttack = gpGlobals->time + RANDOM_FLOAT( 0.3, 0.6 );
	}

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( m_iModelIndexTubeSuicide );
		WRITE_BYTE( 10 );	// scale * 10
		WRITE_BYTE( 15 );	// framerate
		WRITE_BYTE( TE_EXPLFLAG_NODLIGHTS | TE_EXPLFLAG_NOSOUND );
	MESSAGE_END( );

	UTIL_Remove( this );
	return 1;
}

int CSittingTubeMortar::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	PlayerTouch( NULL );
	return 0;
}

TYPEDESCRIPTION	CSittingTubeMortar::m_SaveData[] =
{
	DEFINE_FIELD( CSittingTubeMortar, m_iModelIndexTubeSuicide, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CSittingTubeMortar, CDecore );


//=========================================================
// the sack around a tube embryo
//=========================================================
class CDecoreSack : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify( void );
	Schedule_t *GetSchedule( void );
};
LINK_ENTITY_TO_CLASS( decore_sack, CDecoreSack );

void CDecoreSack::Precache( void )
{
	PRECACHE_MODEL( "models/sack.mdl" );
}

int CDecoreSack::Classify( void )
{
	return CLASS_ALIEN_CRITTER;
}

void CDecoreSack::Spawn( void )
{
	pev->model = MAKE_STRING( "models/sack.mdl" );
	pev->classname = MAKE_STRING( "decore_sack" );
	SET_MODEL( ENT(pev), "models/sack.mdl" );

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NOCLIP;
	pev->effects = 0;
	pev->takedamage = DAMAGE_NO;
	pev->spawnflags |= SF_MONSTER_FALL_TO_GROUND;

	MonsterInit( );
}

Schedule_t *CDecoreSack::GetSchedule( void )
{
	return GetScheduleOfType( SCHED_LOCKED_IDLE_STAND );
}


//=========================================================
// tube embryo: an embryo floating in its sack, which
// twitches when shocked
//=========================================================
class CTubeEmbryo : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify( void );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void Killed( entvars_t *pevAttacker, int iGib );
	Schedule_t *GetSchedule( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	CBaseMonster *pSack;

private:
	void EXPORT SpawnSack( void );
	void EXPORT ShockAnim( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

void CTubeEmbryo::SpawnSack( void )
{
	pSack = GetClassPtr( (CDecoreSack *)NULL );
	pSack->Spawn( );
	pSack->pev->origin = pev->origin;
	pSack->pev->angles = pev->angles;
	pSack->pev->renderamt = 180;
	pSack->pev->rendermode = kRenderTransTexture;

	UTIL_SetSize( pev, Vector( -12, -12, -16 ), Vector( 12, 12, 24 ) );
	MonsterInit( );
	SetUse( ShockAnim );
}

void CTubeEmbryo::ShockAnim( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetSequenceByName( "shock" );
	if ( pSack )
		pSack->SetSequenceByName( "shock" );
	else
		ALERT( at_console, "no sack for shocking!\n" );
}

LINK_ENTITY_TO_CLASS( monster_tube_embryo, CTubeEmbryo );

void CTubeEmbryo::Spawn( void )
{
	Precache( );

	pev->health = 30;
	m_bloodColor = 50;

	SET_MODEL( ENT(pev), "models/tubryo.mdl" );
	UTIL_SetSize( pev, Vector( -6, -6, -6 ), Vector( 6, 6, 12 ) );

	pSack = NULL;
	pev->solid = SOLID_SLIDEBOX;
	m_flFieldOfView = -0.7;
	m_MonsterState = MONSTERSTATE_IDLE;
	pev->nextthink = gpGlobals->time + 0.1;
	pev->spawnflags |= SF_MONSTER_FALL_TO_GROUND;

	SetThink( SpawnSack );
}

void CTubeEmbryo::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( pSack )
		UTIL_Remove( pSack );

	CBaseEntity::Killed( pevAttacker, iGib );
}

Schedule_t *CTubeEmbryo::GetSchedule( void )
{
	return GetScheduleOfType( SCHED_LOCKED_IDLE_STAND );
}

int CTubeEmbryo::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, DMG_ALWAYSGIB );
}

int CTubeEmbryo::Classify( void )
{
	return CLASS_ALIEN_CRITTER;
}

void CTubeEmbryo::Precache( void )
{
	PRECACHE_MODEL( "models/tubryo.mdl" );
	UTIL_PrecacheOther( "decore_sack" );
}

TYPEDESCRIPTION	CTubeEmbryo::m_SaveData[] =
{
	DEFINE_FIELD( CTubeEmbryo, pSack, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CTubeEmbryo, CBaseMonster );


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


//=========================================================
// lab stuff: clutter on the lab tables that breaks into
// pieces at the first hit
//=========================================================
class CDecoreLabstuff : public CDecore
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify( void );
	int ObjectCaps( void );
	int PlayerTouch( CBaseEntity *pOther );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	int m_iGibModel;
};
LINK_ENTITY_TO_CLASS( decore_labstuff, CDecoreLabstuff );

void CDecoreLabstuff::Spawn( void )
{
	Precache( );
	SET_MODEL( ENT(pev), "models/labstuff.mdl" );
	DecoreInit( );
}

int CDecoreLabstuff::Classify( void )
{
	return CLASS_NONE;
}

int CDecoreLabstuff::ObjectCaps( void )
{
	return 0;
}

void CDecoreLabstuff::Precache( void )
{
	PRECACHE_MODEL( "models/labstuff.mdl" );
	m_iGibModel = PRECACHE_MODEL( "models/decoregibs.mdl" );
}

int CDecoreLabstuff::PlayerTouch( CBaseEntity *pOther )
{
	return 0;
}

int CDecoreLabstuff::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BREAKMODEL );

		// position
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );

		// size
		WRITE_COORD( 10 );
		WRITE_COORD( 10 );
		WRITE_COORD( 10 );

		// velocity
		WRITE_COORD( 10 );
		WRITE_COORD( 10 );
		WRITE_COORD( 32 );

		// randomization
		WRITE_BYTE( 5 );

		// model id#
		WRITE_SHORT( m_iGibModel );

		// # of shards
		WRITE_BYTE( 5 );

		// duration
		WRITE_BYTE( 25 );

		// flags
		WRITE_BYTE( BREAK_GLASS );
	MESSAGE_END( );

	UTIL_Remove( this );
	return 0;
}


//=========================================================
// asteroid: a rock that spins in place in the sky; the
// bigger it is, the faster it may spin
//=========================================================
class CAsteroid : public CBaseMonster
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	BOOL IsAlive( void ) { return FALSE; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int m_iBody;
};
LINK_ENTITY_TO_CLASS( decore_asteroid, CAsteroid );

TYPEDESCRIPTION	CAsteroid::m_SaveData[] =
{
	DEFINE_FIELD( CAsteroid, m_iBody, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CAsteroid, CBaseMonster );

void CAsteroid::KeyValue( KeyValueData *pkvd )
{
	m_iBody = 1;	// sic: every key resets the size the "asteroidsize" key set

	if ( FStrEq( pkvd->szKeyName, "asteroidsize" ) )
	{
		m_iBody = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

void CAsteroid::Spawn( void )
{
	pev->classname = MAKE_STRING( "decore_asteroid" );
	PRECACHE_MODEL( "models/rockspin.mdl" );
	SET_MODEL( ENT(pev), "models/rockspin.mdl" );

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_FLY;
	pev->takedamage = DAMAGE_NO;
	pev->effects = 0;
	pev->health = 60;
	pev->body = m_iBody;
	pev->velocity = g_vecZero;

	float flSpin = ( m_iBody + 1 ) * 10;
	pev->avelocity.x = RANDOM_FLOAT( -flSpin, flSpin );
	pev->avelocity.y = RANDOM_FLOAT( -flSpin, flSpin );
	pev->avelocity.z = RANDOM_FLOAT( -flSpin, flSpin );

	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	pev->nextthink = gpGlobals->time + 0.1;
	SetTouch( NULL );
}


//=========================================================
// space debris: hidden until used, then flies at its target
// tumbling, and goes away when it stops or its life runs out
//=========================================================
class CSpaceDebris : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	BOOL IsAlive( void ) { return FALSE; }

	void EXPORT UseToggle( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT DebrisRemoveThink( void );
	void EXPORT DebrisTouch( CBaseEntity *pOther );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	float m_flSpeed;
	float m_flAngleSpeed;
	float m_flLife;
	string_t m_iszModel;
	Vector m_vecDir;		// sic: the target's position, not a direction
	Vector m_vecLastOrigin;
};
LINK_ENTITY_TO_CLASS( decore_spacedebris, CSpaceDebris );

TYPEDESCRIPTION	CSpaceDebris::m_SaveData[] =
{
	DEFINE_FIELD( CSpaceDebris, m_flSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CSpaceDebris, m_flAngleSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CSpaceDebris, m_vecDir, FIELD_VECTOR ),
	DEFINE_FIELD( CSpaceDebris, m_flLife, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CSpaceDebris, CBaseMonster );

void CSpaceDebris::Spawn( void )
{
	pev->classname = MAKE_STRING( "decore_spacedebris" );
	Precache( );
	SET_MODEL( ENT(pev), STRING( m_iszModel ) );

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_NO;
	pev->effects = EF_NODRAW;
	pev->health = 60;
	pev->gravity = 0.001;
	pev->velocity = g_vecZero;

	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	SetThink( NULL );
	SetTouch( NULL );
	SetUse( UseToggle );
}

void CSpaceDebris::Precache( void )
{
	if ( m_iszModel )
		PRECACHE_MODEL( (char *)STRING( m_iszModel ) );
}

void CSpaceDebris::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "modelname" ) )
	{
		m_iszModel = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	if ( FStrEq( pkvd->szKeyName, "forwardspeed" ) )
	{
		m_flSpeed = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	if ( FStrEq( pkvd->szKeyName, "anglespeed" ) )
	{
		m_flAngleSpeed = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	if ( FStrEq( pkvd->szKeyName, "debrislife" ) )
	{
		m_flLife = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;	// sic: drops the keys above as unhandled
}

void CSpaceDebris::UseToggle( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	edict_t *pentTarget = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->target ) );
	if ( !FNullEnt( pentTarget ) )
	{
		CBaseEntity *pTarget = CBaseEntity::Instance( pentTarget );
		if ( pTarget )
			m_vecDir = pTarget->pev->origin;
	}

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_BOUNCE;
	pev->effects = 0;
	pev->gravity = 0.001;
	Vector vecToTarget = m_vecDir - pev->origin;
	pev->velocity = vecToTarget.Normalize( ) * m_flSpeed;
	pev->avelocity.x = RANDOM_FLOAT( -m_flAngleSpeed, m_flAngleSpeed );
	pev->avelocity.y = RANDOM_FLOAT( -m_flAngleSpeed, m_flAngleSpeed );
	pev->avelocity.z = RANDOM_FLOAT( -m_flAngleSpeed, m_flAngleSpeed );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	SetThink( DebrisRemoveThink );
	pev->nextthink = gpGlobals->time + 0.2;
	SetTouch( DebrisTouch );

	UTIL_SetOrigin( pev, pev->origin );
	m_vecLastOrigin = pev->origin;
}

void CSpaceDebris::DebrisRemoveThink( void )
{
	pev->solid = SOLID_NOT;
	m_flLife -= 0.1;

	// stuck: it hasn't moved since the last think
	if ( m_vecLastOrigin == pev->origin )
	{
		UTIL_Remove( this );
		return;
	}

	pev->nextthink = gpGlobals->time + 0.1;
	m_vecLastOrigin = pev->origin;

	if ( m_flLife <= 0 )
		UTIL_Remove( this );
}

void CSpaceDebris::DebrisTouch( CBaseEntity *pOther )
{
	// pieces of debris pass through each other
	if ( FStrEq( STRING( pev->classname ), STRING( pOther->pev->classname ) ) )
		pev->solid = SOLID_NOT;
}

// ==== end of part B ====

// ==== decore part C: CDummyBot (fork C) ====

// ==== end of part C ====
