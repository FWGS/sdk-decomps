/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
#ifndef OEM_BUILD

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"

extern DLL_GLOBAL int		g_iSkillLevel;

// sic: the precompiled header stops before weapons.h (retail emits CGrenade's
// BloodColor right after hvr_rocket's factory, not at the end of the object)
#include "weapons.h"
#include "nodes.h"
#include "effects.h"
#include "soundent.h"
#include "decals.h"

// defined in util.cpp, which util.h doesn't declare; name ours
extern void UTIL_TraceToss( CBaseEntity *pEntity, edict_t *pentIgnore, TraceResult *ptr );

#define SF_WAITFORTRIGGER	(0x04 | 0x40) // UNDONE: Fix!
#define SF_NOWRECKAGE		0x08

// pev->weapons: the turret weapon ("weapons") and the missile ("weapons2"), -1 random; names ours
typedef struct
{
	unsigned int	iGun : 16;		// 0 12mm MG, 1 standard flak, 2 lightning gun, 3 pulse laser
	int				iMissile : 16;	// 0 heavy rocket, 1 napalm rocket, 2 heat-seeking missile, 3 photon torpedo
} apacheweapons_t;

#define APACHE_WEAPONS( pev )	( (apacheweapons_t *)&(pev)->weapons )

class CApache : public CBaseMonster
{
public:
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	int  Classify( void ) { return CLASS_HUMAN_MILITARY; };
	int  BloodColor( void ) { return DONT_BLEED; }
	void Killed( entvars_t *pevAttacker, int iGib );
	void GibMonster( void );

	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin + Vector( -300, -300, -172);
		pev->absmax = pev->origin + Vector(300, 300, 8);
	}

	int GetOnState( void ) { if ( pev->nextthink ) return IsAlive(); return 0; }
	int HeatSignature( void ) { return 6; }
	BOOL CanBeRepulsed( bool fForce ) { if ( fForce && pev->movetype ) return TRUE; return FALSE; }
	BOOL CanBeGrenaded( int fLauncher ) { return fLauncher; }
	BOOL CanBeTractored( void ) { return TRUE; }
	float GetHoldDistance( void ) { if ( CanBeTractored() ) return 64; return 0; }
	const char *GetHudName( void ) { return "Apache"; }
	const char *GetKillDesc( void ) { return pev->health <= 0 ? "" : "an assault helicopter"; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return pev->health <= 0 ? "was blown up by a crashing assault helicopter" : "shot"; }
	const char *GetHitGroupDesc( int iHitGroup ) { return HitGroupIn( iHitGroup ); }
	int GetBattlePower( void );

	int GunType( void ) { return APACHE_WEAPONS( pev )->iGun; }		// names ours
	int MissileType( void ) { return APACHE_WEAPONS( pev )->iMissile; }

	void EXPORT HuntThink( void );
	void EXPORT FlyTouch( CBaseEntity *pOther );
	void EXPORT CrashTouch( CBaseEntity *pOther );
	void EXPORT DyingThink( void );
	void EXPORT StartupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT NullThink( void );

	virtual void Hunt( void );		// name ours: HuntThink's body
	void ShowDamage( void );
	void Flight( void );
	virtual void FireRocket( void );
	virtual BOOL FireGun( void );
	virtual void FireFlak( Vector &vecSrc, Vector &vecEnd );	// name ours

	int  TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	int m_iRockets;
	float m_flForce;
	float m_flNextRocket;

	Vector m_vecTarget;
	Vector m_posTarget;

	Vector m_vecDesired;
	Vector m_posDesired;

	Vector m_vecGoal;

	Vector m_angGun;
	float m_flLastSeen;
	float m_flPrevSeen;

	int m_iSoundState; // don't save this

	int m_iSpriteTexture;
	int m_iExplode;
	int m_iBodyGibs;

	float m_flGoalSpeed;

	int m_iDoSmokePuff;
	CBeam *m_pBeam;
};
LINK_ENTITY_TO_CLASS( monster_apache, CApache );

TYPEDESCRIPTION	CApache::m_SaveData[] =
{
	DEFINE_FIELD( CApache, m_iRockets, FIELD_INTEGER ),
	DEFINE_FIELD( CApache, m_flForce, FIELD_FLOAT ),
	DEFINE_FIELD( CApache, m_flNextRocket, FIELD_TIME ),
	DEFINE_FIELD( CApache, m_vecTarget, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_posTarget, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CApache, m_vecDesired, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_posDesired, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CApache, m_vecGoal, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_angGun, FIELD_VECTOR ),
	DEFINE_FIELD( CApache, m_flLastSeen, FIELD_TIME ),
	DEFINE_FIELD( CApache, m_flPrevSeen, FIELD_TIME ),
//	DEFINE_FIELD( CApache, m_iSoundState, FIELD_INTEGER ),		// Don't save, precached
//	DEFINE_FIELD( CApache, m_iSpriteTexture, FIELD_INTEGER ),
//	DEFINE_FIELD( CApache, m_iExplode, FIELD_INTEGER ),
//	DEFINE_FIELD( CApache, m_iBodyGibs, FIELD_INTEGER ),
	DEFINE_FIELD( CApache, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CApache, m_flGoalSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CApache, m_iDoSmokePuff, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CApache, CBaseMonster );


void CApache :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/apache.mdl");
	UTIL_SetSize( pev, Vector( -32, -32, -64 ), Vector( 32, 32, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->flags |= FL_MONSTER;
	pev->takedamage		= DAMAGE_AIM;
	pev->health			= gSkillData.apacheHealth;
	pev->max_health		= gSkillData.apacheHealth;

	m_flFieldOfView = -0.707; // 270 degrees

	pev->sequence = 0;
	ResetSequenceInfo( );
	pev->frame = RANDOM_LONG(0, 0xFF);

	if (pev->weapons <= -1)
	{
		APACHE_WEAPONS( pev )->iGun = RANDOM_LONG( 0, 3 );
		APACHE_WEAPONS( pev )->iMissile = RANDOM_LONG( 0, 3 );
	}

	InitBoneControllers();

	if (pev->spawnflags & SF_WAITFORTRIGGER)
	{
		SetUse( StartupUse );
		pev->nextthink = 0;
	}
	else
	{
		SetThink( HuntThink );
		SetTouch( FlyTouch );
		pev->nextthink = gpGlobals->time + 1.0;
	}

	m_iRockets = 10;
}


void CApache :: HuntThink( void )
{
	Hunt( );
}


void CApache::Precache( void )
{
	PRECACHE_MODEL("models/apache.mdl");

	pev->noise = MAKE_STRING("apache/ap_rotor2.wav");
	PRECACHE_SOUND("apache/ap_rotor2.wav");

	PRECACHE_SOUND("weapons/mortarhit.wav");

	m_iSpriteTexture = PRECACHE_MODEL( "sprites/white.spr" );

	PRECACHE_SOUND("turret/tu_fire1.wav");

	PRECACHE_MODEL("sprites/lgtning.spr");

	UTIL_PrecacheOther( "standard_flak" );

	m_iExplode	= PRECACHE_MODEL( "sprites/fexplo.spr" );
	m_iBodyGibs = PRECACHE_MODEL( "models/metalplategibs_green.mdl" );

	PRECACHE_SOUND("weapons/pl_gun1.wav");
	PRECACHE_SOUND("weapons/pl_gun2.wav");
	UTIL_PrecacheOther( "laser_pulse" );
	CLaserPulse::PrecacheColor( 2 );

	UTIL_PrecacheOther( "hvr_rocket" );
	UTIL_PrecacheOther( "rocket" );
	PRECACHE_MODEL("sprites/photon_t.spr");
}


void CApache :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "weapons2"))
	{
		if (pev->weapons == -1)
			APACHE_WEAPONS( pev )->iGun = RANDOM_LONG( 0, 3 );

		int iMissile = atoi( pkvd->szValue );
		if (iMissile == -1)
			iMissile = RANDOM_LONG( 0, 3 );
		APACHE_WEAPONS( pev )->iMissile = iMissile;

		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}


int CApache :: GetBattlePower( void )
{
	float flPower;

	switch (APACHE_WEAPONS( pev )->iGun)
	{
	case 1:
		flPower = 250;
		break;
	case 3:
		flPower = gSkillData.monDmg12MM * 14;
		break;
	default:
		flPower = gSkillData.monDmg12MM * 12.5;
		break;
	}

	switch (APACHE_WEAPONS( pev )->iMissile)
	{
	case 2:
		flPower += 187.5;
		break;
	case 3:
		flPower += 250;
		break;
	default:
		flPower += 150;
		break;
	}

	return flPower * (pev->max_health / 8.0);
}


void CApache::NullThink( void )
{
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.5;
}


void CApache::StartupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( HuntThink );
	SetTouch( FlyTouch );
	pev->nextthink = gpGlobals->time + 0.1;
	SetUse( NULL );
}

void CApache :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->gravity = 0.3;

	STOP_SOUND( ENT(pev), CHAN_STATIC, (char *)STRING(pev->noise) );

	UTIL_SetSize( pev, Vector( -32, -32, -64), Vector( 32, 32, 0) );
	SetThink( DyingThink );
	SetTouch( CrashTouch );
	pev->nextthink = gpGlobals->time + 0.1;
	pev->takedamage = DAMAGE_YES;

	if (m_pBeam)
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}

	if (pev->spawnflags & SF_NOWRECKAGE)
	{
		m_flNextRocket = gpGlobals->time + 4.0;
	}
	else
	{
		m_flNextRocket = gpGlobals->time + 15.0;
	}

	// tell the monsters where it is going to crash
	TraceResult tr;
	UTIL_TraceToss( this, NULL, &tr );
	CSoundEnt::InsertSound( bits_SOUND_DANGER, tr.vecEndPos, 400, m_flNextRocket - gpGlobals->time );
}

void CApache :: DyingThink( void )
{
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	pev->avelocity = pev->avelocity * 1.02;

	// still falling?
	if (m_flNextRocket > gpGlobals->time && pev->health > -250)
	{
		// random explosions
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION);		// This just makes a dynamic light now
			WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -150, 150 ));
			WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -150, 150 ));
			WRITE_COORD( pev->origin.z + RANDOM_FLOAT( -150, -50 ));
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( RANDOM_LONG(0,29) + 30  ); // scale * 10
			WRITE_BYTE( 12  ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();

		// lots of smoke
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -150, 150 ));
			WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -150, 150 ));
			WRITE_COORD( pev->origin.z + RANDOM_FLOAT( -150, -50 ));
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 100 ); // scale * 10
			WRITE_BYTE( 10  ); // framerate
		MESSAGE_END();

		Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_BREAKMODEL);

			// position
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z );

			// size
			WRITE_COORD( 400 );
			WRITE_COORD( 400 );
			WRITE_COORD( 132 );

			// velocity
			WRITE_COORD( pev->velocity.x );
			WRITE_COORD( pev->velocity.y );
			WRITE_COORD( pev->velocity.z );

			// randomization
			WRITE_BYTE( 50 );

			// Model
			WRITE_SHORT( m_iBodyGibs );	//model id#

			// # of shards
			WRITE_BYTE( 4 );	// let client decide

			// duration
			WRITE_BYTE( 30 );// 3.0 seconds

			// flags

			WRITE_BYTE( BREAK_METAL );
		MESSAGE_END();

		// don't stop it we touch a entity
		pev->flags &= ~FL_ONGROUND;
		pev->nextthink = gpGlobals->time + 0.2;
		return;
	}
	else
	{
		pev->takedamage = DAMAGE_NO;
		pev->solid = SOLID_NOT;
		pev->velocity = g_vecZero;
		pev->movetype = MOVETYPE_NONE;
		pev->effects = EF_NODRAW;

		Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;

		/*
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_EXPLOSION);		// This just makes a dynamic light now
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 300 );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( 250 ); // scale * 10
			WRITE_BYTE( 8  ); // framerate
		MESSAGE_END();
		*/

		// fireball
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_SPRITE );
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 256 );
			WRITE_SHORT( m_iExplode );
			WRITE_BYTE( 120 ); // scale * 10
			WRITE_BYTE( 255 ); // brightness
		MESSAGE_END();

		// big smoke
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 512 );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 250 ); // scale * 10
			WRITE_BYTE( 5  ); // framerate
		MESSAGE_END();

		// blast circle
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_BEAMCYLINDER );
			WRITE_COORD( pev->origin.x);
			WRITE_COORD( pev->origin.y);
			WRITE_COORD( pev->origin.z);
			WRITE_COORD( pev->origin.x);
			WRITE_COORD( pev->origin.y);
			WRITE_COORD( pev->origin.z + 2000 ); // reach damage radius over .2 seconds
			WRITE_SHORT( m_iSpriteTexture );
			WRITE_BYTE( 0 ); // startframe
			WRITE_BYTE( 0 ); // framerate
			WRITE_BYTE( 4 ); // life
			WRITE_BYTE( 32 );  // width
			WRITE_BYTE( 0 );   // noise
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 192 );   // r, g, b
			WRITE_BYTE( 128 ); // brightness
			WRITE_BYTE( 0 );		// speed
		MESSAGE_END();

		EMIT_SOUND(ENT(pev), CHAN_STATIC, "weapons/mortarhit.wav", 1.0, 0.3);

		RadiusDamage( pev->origin, pev, pev, 300, CLASS_NONE, DMG_BLAST );

		// gibs
		vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_BREAKMODEL);

			// position
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 64);

			// size
			WRITE_COORD( 400 );
			WRITE_COORD( 400 );
			WRITE_COORD( 128 );

			// velocity
			WRITE_COORD( 0 );
			WRITE_COORD( 0 );
			WRITE_COORD( 200 );

			// randomization
			WRITE_BYTE( 30 );

			// Model
			WRITE_SHORT( m_iBodyGibs );	//model id#

			// # of shards
			WRITE_BYTE( 200 );

			// duration
			WRITE_BYTE( 200 );// 10.0 seconds

			// flags

			WRITE_BYTE( BREAK_METAL );
		MESSAGE_END();

		SetThink( SUB_Remove );
		pev->nextthink = gpGlobals->time + 1;

		CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
		if ( pOwner )
			pOwner->DeathNotice( pev );
	}
}


void CApache::FlyTouch( CBaseEntity *pOther )
{
	// bounce if we hit something solid
	if ( pOther->pev->solid == SOLID_BSP)
	{
		TraceResult tr = UTIL_GetGlobalTrace( );

		// UNDONE, do a real bounce
		pev->velocity = pev->velocity + tr.vecPlaneNormal * (pev->velocity.Length() + 200);
	}
}


void CApache::CrashTouch( CBaseEntity *pOther )
{
	// only crash if we hit something solid
	if ( pOther->pev->solid == SOLID_BSP)
	{
		SetTouch( NULL );
		m_flNextRocket = gpGlobals->time;
		pev->nextthink = gpGlobals->time;
	}
}



void CApache :: GibMonster( void )
{
	// EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "common/bodysplat.wav", 0.75, ATTN_NORM, 0, 200);
}


void CApache :: Hunt( void )
{
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	ShowDamage( );

	if ( m_pGoalEnt == NULL && !FStringNull(pev->target) )// this monster has a target
	{
		m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
		if (m_pGoalEnt)
		{
			m_posDesired = m_pGoalEnt->pev->origin;
			UTIL_MakeAimVectors( m_pGoalEnt->pev->angles );
			m_vecGoal = gpGlobals->v_forward;
		}
	}

	// if (m_hEnemy == NULL)
	{
		Look( 4092 );
		m_hEnemy = BestVisibleEnemy( );
	}

	// generic speed up
	if (m_flGoalSpeed < 800)
		m_flGoalSpeed += 5;

	if (m_hEnemy != NULL)
	{
		// ALERT( at_console, "%s\n", STRING( m_hEnemy->pev->classname ) );
		if (FVisible( m_hEnemy ))
		{
			if (m_flLastSeen < gpGlobals->time - 5)
				m_flPrevSeen = gpGlobals->time;
			m_flLastSeen = gpGlobals->time;
			m_posTarget = m_hEnemy->Center( );
		}
		else
		{
			m_hEnemy = NULL;
		}
	}

	m_vecTarget = (m_posTarget - pev->origin).Normalize();

	float flLength = (pev->origin - m_posDesired).Length();

	if (m_pGoalEnt)
	{
		// ALERT( at_console, "%.0f\n", flLength );

		if (flLength < 128)
		{
			m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( m_pGoalEnt->pev->target ) );
			if (m_pGoalEnt)
			{
				m_posDesired = m_pGoalEnt->pev->origin;
				UTIL_MakeAimVectors( m_pGoalEnt->pev->angles );
				m_vecGoal = gpGlobals->v_forward;
				flLength = (pev->origin - m_posDesired).Length();
			}
		}
	}
	else
	{
		m_posDesired = pev->origin;
	}

	if (flLength > 250) // 500
	{
		// float flLength2 = (m_posTarget - pev->origin).Length() * (1.5 - DotProduct((m_posTarget - pev->origin).Normalize(), pev->velocity.Normalize() ));
		// if (flLength2 < flLength)
		if (m_flLastSeen + 90 > gpGlobals->time && DotProduct( (m_posTarget - pev->origin).Normalize(), (m_posDesired - pev->origin).Normalize( )) > 0.25)
		{
			m_vecDesired = (m_posTarget - pev->origin).Normalize( );
		}
		else
		{
			m_vecDesired = (m_posDesired - pev->origin).Normalize( );
		}
	}
	else
	{
		m_vecDesired = m_vecGoal;
	}

	Flight( );

	// ALERT( at_console, "%.0f %.0f %.0f\n", gpGlobals->time, m_flLastSeen, m_flPrevSeen );
	if ((m_flLastSeen + 1 > gpGlobals->time) && (m_flPrevSeen + 2 < gpGlobals->time))
	{
		if (FireGun( ))
		{
			// slow down if we're fireing
			if (m_flGoalSpeed > 400)
				m_flGoalSpeed = 400;
		}
	}
	else
	{
		if (m_pBeam)
		{
			UTIL_Remove( m_pBeam );
			m_pBeam = NULL;
		}
	}

	UTIL_MakeAimVectors( pev->angles );
	Vector vecEst = (gpGlobals->v_forward * 800 + pev->velocity).Normalize( );
	// ALERT( at_console, "%d %d %d %4.2f\n", pev->angles.x < 0, DotProduct( pev->velocity, gpGlobals->v_forward ) > -100, m_flNextRocket < gpGlobals->time, DotProduct( m_vecTarget, vecEst ) );

	if ((m_iRockets % 2) == 1)
	{
		FireRocket( );
		m_flNextRocket = gpGlobals->time + 1;
		if (m_iRockets <= 0)
		{
			m_flNextRocket = gpGlobals->time + 10;
			m_iRockets = 10;
		}
	}
	else if (DotProduct( pev->velocity, gpGlobals->v_forward ) > -100 && m_flNextRocket < gpGlobals->time)
	{
		if (m_flLastSeen + 60 > gpGlobals->time)
		{
			if (m_hEnemy != NULL)
			{
				float flDot = 0.965;
				int iMissile = MissileType();
				if (iMissile == 2)
					flDot = 0.7;
				else if (iMissile == 3)
					flDot = 0.9;

				// make sure it's a good shot
				if (DotProduct( m_vecTarget, vecEst) > flDot)
				{
					TraceResult tr;

					UTIL_TraceLine( pev->origin, pev->origin + vecEst * 4096, ignore_monsters, edict(), &tr );
					if ((tr.vecEndPos - m_posTarget).Length() < 1024)
						FireRocket( );
				}
			}
			else
			{
				TraceResult tr;

				UTIL_TraceLine( pev->origin, pev->origin + vecEst * 4096, dont_ignore_monsters, edict(), &tr );
				// just fire when close
				if ((tr.vecEndPos - m_posTarget).Length() < 512)
					FireRocket( );
			}
		}
	}
}


void CApache :: Flight( void )
{
	// tilt model 5 degrees
	Vector vecAdj = Vector( 5.0, 0, 0 );

	// estimate where I'll be facing in one seconds
	UTIL_MakeAimVectors( pev->angles + pev->avelocity * 2 + vecAdj);
	// Vector vecEst1 = pev->origin + pev->velocity + gpGlobals->v_up * m_flForce - Vector( 0, 0, 384 );
	// float flSide = DotProduct( m_posDesired - vecEst1, gpGlobals->v_right );

	float flSide = DotProduct( m_vecDesired, gpGlobals->v_right );

	if (flSide < 0)
	{
		if (pev->avelocity.y < 60)
		{
			pev->avelocity.y += 8; // 9 * (3.0/2.0);
		}
	}
	else
	{
		if (pev->avelocity.y > -60)
		{
			pev->avelocity.y -= 8; // 9 * (3.0/2.0);
		}
	}
	pev->avelocity.y *= 0.98;

	// estimate where I'll be in two seconds
	UTIL_MakeAimVectors( pev->angles + pev->avelocity * 1 + vecAdj);
	Vector vecEst = pev->origin + pev->velocity * 2.0 + gpGlobals->v_up * m_flForce * 20 - Vector( 0, 0, 384 * 2 );

	// add immediate force
	UTIL_MakeAimVectors( pev->angles + vecAdj);
	pev->velocity.x += gpGlobals->v_up.x * m_flForce;
	pev->velocity.y += gpGlobals->v_up.y * m_flForce;
	pev->velocity.z += gpGlobals->v_up.z * m_flForce;
	// add gravity
	pev->velocity.z -= 38.4; // 32ft/sec


	float flSpeed = pev->velocity.Length();
	float flDir = DotProduct( Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, 0 ), Vector( pev->velocity.x, pev->velocity.y, 0 ) );
	if (flDir < 0)
		flSpeed = -flSpeed;

	float flDist = DotProduct( m_posDesired - vecEst, gpGlobals->v_forward );

	// float flSlip = DotProduct( pev->velocity, gpGlobals->v_right );
	float flSlip = -DotProduct( m_posDesired - vecEst, gpGlobals->v_right );

	// fly sideways
	if (flSlip > 0)
	{
		if (pev->angles.z > -30 && pev->avelocity.z > -15)
			pev->avelocity.z -= 4;
		else
			pev->avelocity.z += 2;
	}
	else
	{

		if (pev->angles.z < 30 && pev->avelocity.z < 15)
			pev->avelocity.z += 4;
		else
			pev->avelocity.z -= 2;
	}

	// sideways drag
	pev->velocity.x = pev->velocity.x * (1.0 - fabs( gpGlobals->v_right.x ) * 0.05);
	pev->velocity.y = pev->velocity.y * (1.0 - fabs( gpGlobals->v_right.y ) * 0.05);
	pev->velocity.z = pev->velocity.z * (1.0 - fabs( gpGlobals->v_right.z ) * 0.05);

	// general drag
	pev->velocity = pev->velocity * 0.995;

	// apply power to stay correct height
	if (m_flForce < 80 && vecEst.z < m_posDesired.z)
	{
		m_flForce += 12;
	}
	else if (m_flForce > 30)
	{
		if (vecEst.z > m_posDesired.z)
			m_flForce -= 8;
	}

	// pitch forward or back to get to target
	if (flDist > 0 && flSpeed < m_flGoalSpeed /* && flSpeed < flDist */ && pev->angles.x + pev->avelocity.x > -40)
	{
		// ALERT( at_console, "F " );
		// lean forward
		pev->avelocity.x -= 12.0;
	}
	else if (flDist < 0 && flSpeed > -50 && pev->angles.x + pev->avelocity.x  < 20)
	{
		// ALERT( at_console, "B " );
		// lean backward
		pev->avelocity.x += 12.0;
	}
	else if (pev->angles.x + pev->avelocity.x > 0)
	{
		// ALERT( at_console, "f " );
		pev->avelocity.x -= 4.0;
	}
	else if (pev->angles.x + pev->avelocity.x < 0)
	{
		// ALERT( at_console, "b " );
		pev->avelocity.x += 4.0;
	}

	// ALERT( at_console, "%.0f %.0f : %.0f %.0f : %.0f %.0f : %.0f\n", pev->origin.x, pev->velocity.x, flDist, flSpeed, pev->angles.x, pev->avelocity.x, m_flForce );
	// ALERT( at_console, "%.0f %.0f : %.0f %0.f : %.0f\n", pev->origin.z, pev->velocity.z, vecEst.z, m_posDesired.z, m_flForce );

	// make rotor, engine sounds
	if (m_iSoundState == 0)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, (char *)STRING(pev->noise), 1.0, 0.3, 0, 110 );
		// EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_whine1.wav", 0.5, 0.2, 0, 110 );

		m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions
	}
	else
	{
		CBaseEntity *pPlayer = NULL;

		pPlayer = UTIL_FindEntityByClassname( NULL, "player" );
		// UNDONE: this needs to send different sounds to every player for multiplayer.
		if (pPlayer)
		{

			float pitch = DotProduct( pev->velocity - pPlayer->pev->velocity, (pPlayer->pev->origin - pev->origin).Normalize() );

			pitch = (int)(100 + pitch / 50.0);

			if (pitch > 250)
				pitch = 250;
			if (pitch < 50)
				pitch = 50;
			if (pitch == 100)
				pitch = 101;

			float flVol = (m_flForce / 100.0) + .1;
			if (flVol > 1.0)
				flVol = 1.0;

			EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, (char *)STRING(pev->noise), 1.0, 0.3, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch);
		}
		// EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_whine1.wav", flVol, 0.2, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch);

		// ALERT( at_console, "%.0f %.2f\n", pitch, flVol );
	}
}


void CApache :: FireRocket( void )
{
	static float side = 1.0;
	static int count;

	if (m_iRockets <= 0)
		return;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + 1.5 * (gpGlobals->v_forward * 21 + gpGlobals->v_right * 70 * side + gpGlobals->v_up * -79);

	switch( m_iRockets % 5)
	{
	case 0:	vecSrc = vecSrc + gpGlobals->v_right * 10; break;
	case 1: vecSrc = vecSrc - gpGlobals->v_right * 10; break;
	case 2: vecSrc = vecSrc + gpGlobals->v_up * 10; break;
	case 3: vecSrc = vecSrc - gpGlobals->v_up * 10; break;
	case 4: break;
	}

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
		WRITE_BYTE( TE_SMOKE );
		WRITE_COORD( vecSrc.x );
		WRITE_COORD( vecSrc.y );
		WRITE_COORD( vecSrc.z );
		WRITE_SHORT( g_sModelIndexSmoke );
		WRITE_BYTE( 20 ); // scale * 10
		WRITE_BYTE( 12 ); // framerate
	MESSAGE_END();

	int iMissile = MissileType();
	if (iMissile == 2)
	{
		// heat-seeking missile
		CRocket *pRocket = CRocket::CreateRocket( vecSrc, pev->angles, edict(), Vector( 255, 224, 64 ), 0 );	// sic: used unchecked
		pRocket->m_flSeekTime = gpGlobals->time + 0.2;
		pRocket->pev->dmg = 150;
		pRocket->pev->velocity = gpGlobals->v_forward * 100;
		pRocket->pev->speed = 1200;
		pRocket->m_flFieldOfView = 0.7;
		pRocket->SetThink( CRocket::SeekThink );
		pRocket->m_iszKillDesc = MAKE_STRING( "a heat-seeking missile" );
		SET_MODEL( ENT( pRocket->pev ), "models/HVR.mdl" );
	}
	else if (iMissile == 3)
	{
		// photon torpedo
		CRocket *pRocket = CRocket::CreateRocket( vecSrc, pev->angles, edict(), Vector( 255, 192, 12 ), MAKE_STRING( "sprites/photon_t.spr" ) );
		if (pRocket)
		{
			pRocket->SetThink( CRocket::TargetThink );
			pRocket->pev->speed = 1500;
			pRocket->pev->renderamt = 255;
			pRocket->pev->rendermode = kRenderTransAdd;
			pRocket->m_flSeekTime = gpGlobals->time + 0.2;
			pRocket->pev->velocity = gpGlobals->v_forward * 256;
			pRocket->m_hEnemy = m_hEnemy;
			pRocket->pev->dmg = 200;
			pRocket->pev->spawnflags |= 8;
			pRocket->m_iszKillDesc = MAKE_STRING( "a photon torpedo" );
		}
	}
	else
	{
		CBaseEntity *pRocket = CBaseEntity::Create( "hvr_rocket", vecSrc, pev->angles, edict() );
		if (pRocket)
		{
			// napalm rocket
			if (MissileType() == 1)
				pRocket->pev->spawnflags |= 3;
			pRocket->pev->velocity = pev->velocity + gpGlobals->v_forward * 100;
		}
	}

	m_iRockets--;

	side = - side;
}


void CApache :: FireFlak( Vector &vecSrc, Vector &vecEnd )
{
	Vector vecAngles = Vector( RANDOM_FLOAT( -180, 180 ), RANDOM_FLOAT( -180, 180 ), RANDOM_FLOAT( -180, 180 ) );
	UTIL_MakeVectors( vecAngles );
	Vector vecDest = vecEnd + gpGlobals->v_forward * RANDOM_FLOAT( 0, 96 );

	CBaseEntity *pFlak = CBaseEntity::Create( "standard_flak", vecSrc, g_vecZero, edict() );
	pFlak->pev->velocity = vecDest - vecSrc;

	float flTime = pFlak->pev->velocity.Length() / 2000;
	pFlak->pev->nextthink = gpGlobals->time + max( 0.1, flTime ) + 0.1;
	pFlak->pev->velocity = pFlak->pev->velocity.Normalize() * 2000;

	EMIT_SOUND( ENT(pev), CHAN_WEAPON, "turret/tu_fire1.wav", 1.0, 0.6 );
	pev->effects |= EF_MUZZLEFLASH;
}


BOOL CApache :: FireGun( )
{
	UTIL_MakeAimVectors( pev->angles );

	Vector posGun, angGun;
	GetAttachment( 1, posGun, angGun );

	Vector vecTarget = m_posTarget - posGun;
	float flDist = vecTarget.Length();
	vecTarget = vecTarget.Normalize( );

	Vector vecOut;

	vecOut.x = DotProduct( gpGlobals->v_forward, vecTarget );
	vecOut.y = -DotProduct( gpGlobals->v_right, vecTarget );
	vecOut.z = DotProduct( gpGlobals->v_up, vecTarget );

	Vector angles = UTIL_VecToAngles (vecOut);

	angles.x = -angles.x;
	if (angles.y > 180)
		angles.y = angles.y - 360;
	if (angles.y < -180)
		angles.y = angles.y + 360;
	if (angles.x > 180)
		angles.x = angles.x - 360;
	if (angles.x < -180)
		angles.x = angles.x + 360;

	if (angles.x > m_angGun.x)
		m_angGun.x = min( angles.x, m_angGun.x + 12 );
	if (angles.x < m_angGun.x)
		m_angGun.x = max( angles.x, m_angGun.x - 12 );
	if (angles.y > m_angGun.y)
		m_angGun.y = min( angles.y, m_angGun.y + 12 );
	if (angles.y < m_angGun.y)
		m_angGun.y = max( angles.y, m_angGun.y - 12 );

	m_angGun.y = SetBoneController( 0, m_angGun.y );
	m_angGun.x = SetBoneController( 1, m_angGun.x );

	Vector posBarrel, angBarrel;
	GetAttachment( 0, posBarrel, angBarrel );
	Vector vecGun = (posBarrel - posGun).Normalize( );

	if (DotProduct( vecGun, vecTarget ) > 0.98)
	{
		if (GunType() == 1)
		{
			// standard flak
			FireFlak( posGun, posGun + vecGun * flDist );
			return TRUE;
		}
		else if (GunType() == 3)
		{
			// pulse laser
			CLaserPulse *pPulse = CLaserPulse::Shoot( posGun, edict(), Vector( 255, 255, 255 ), 2, 0.25, 2 );
			pPulse->pev->velocity = vecGun * 2000;
			pPulse->pev->dmg = gSkillData.monDmg12MM * 1.4;
			pPulse->pev->iuser2 = DMG_ENERGYBEAM | DMG_NOGIB;	// damage type
			if (RANDOM_LONG( 0, 1 ))
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/pl_gun1.wav", 1, 0.3 );
			else
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/pl_gun2.wav", 1, 0.3 );
			return TRUE;
		}
		else if (GunType() != 2)
		{
			FireBullets( 1, posGun, vecGun, VECTOR_CONE_4DEGREES, 8192, BULLET_MONSTER_12MM, 1 );
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "turret/tu_fire1.wav", 1, 0.3);
			return TRUE;
		}
		else
		{
			// lightning gun
			TraceResult tr;
			UTIL_TraceLine( posGun, posGun + vecGun * 8192, dont_ignore_monsters, ENT( pev ), &tr );

			if (!m_pBeam)
			{
				m_pBeam = CBeam::BeamCreate( "sprites/lgtning.spr", 80 );
				m_pBeam->PointEntInit( pev->origin, entindex( ) );
				m_pBeam->SetEndAttachment( 1 );
				m_pBeam->SetColor( 255, 180, 96 );
				m_pBeam->SetBrightness( 192 );
				m_pBeam->SetScrollRate( 35 );
				m_pBeam->m_iszKillDesc = MAKE_STRING( "a lightning gun" );
				m_pBeam->pev->owner = edict();
				m_pBeam->pev->spawnflags |= SF_BEAM_SPARKSTART | SF_BEAM_DECALS;
			}

			m_pBeam->BeamDamageInstant( &tr, gSkillData.monDmg12MM );
			m_pBeam->SetStartPos( tr.vecEndPos );
			m_pBeam->DoSparks( tr.vecEndPos, g_vecZero );
			return TRUE;
		}
	}
	else
	{
		if (m_pBeam)
		{
			UTIL_Remove( m_pBeam );
			m_pBeam = NULL;
		}
	}
	return FALSE;
}



void CApache :: ShowDamage( void )
{
	if (m_iDoSmokePuff > 0 || RANDOM_LONG(0,99) > pev->health)
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z - 32 );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( RANDOM_LONG(0,9) + 20 ); // scale * 10
			WRITE_BYTE( 12 ); // framerate
		MESSAGE_END();
	}
	if (m_iDoSmokePuff > 0)
		m_iDoSmokePuff--;
}


int CApache :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if (pevAttacker == pev)
		return 0;

	if (bitsDamageType & DMG_BLAST)
	{
		flDamage *= 2;
	}

	/*
	if ( (bitsDamageType & DMG_BULLET) && flDamage > 50)
	{
		// clip bullet damage at 50
		flDamage = 50;
	}
	*/

	// ALERT( at_console, "%.0f\n", flDamage );
	return CBaseEntity::TakeDamage(  pevInflictor, pevAttacker, flDamage, bitsDamageType );
}



void CApache::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	// ALERT( at_console, "%d %.0f\n", ptr->iHitgroup, flDamage );

	// ignore blades
	if (ptr->iHitgroup == 6 && (bitsDamageType & (DMG_ENERGYBEAM|DMG_BULLET|DMG_CLUB)))
		return;

	// hit hard, hits cockpit, hits engines
	if (flDamage > 50 || ptr->iHitgroup == 1 || ptr->iHitgroup == 2)
	{
		// ALERT( at_console, "%.0f\n", flDamage );
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
		m_iDoSmokePuff = 3 + (flDamage / 5.0);
	}
	else if (bitsDamageType & (DMG_BLAST|DMG_SHOCK|DMG_SONIC|DMG_ENERGYBEAM))
	{
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
	}
	else
	{
		// do half damage in the body
		// AddMultiDamage( pevAttacker, this, flDamage / 2.0, bitsDamageType );
		UTIL_Ricochet( ptr->vecEndPos, 2.0 );
	}
}





class CApacheHVR : public CGrenade
{
public:
	void Spawn( void );
	void Precache( void );
	const char *GetKillDesc( void ) { return "a heavy rocket"; }
	void Killed( entvars_t *pevAttacker, int iGib );
	virtual void EXPORT IgniteThink( void );
	void EXPORT AccelerateThink( void );

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	Vector m_vecForward;
};
LINK_ENTITY_TO_CLASS( hvr_rocket, CApacheHVR );

TYPEDESCRIPTION	CApacheHVR::m_SaveData[] =
{
//	DEFINE_FIELD( CApacheHVR, m_iTrail, FIELD_INTEGER ),	// Dont' save, precache
	DEFINE_FIELD( CApacheHVR, m_vecForward, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CApacheHVR, CGrenade );

void CApacheHVR :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/HVR.mdl");
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin( pev, pev->origin );

	SetThink( IgniteThink );
	SetTouch( ExplodeTouch );

	UTIL_MakeAimVectors( pev->angles );
	m_vecForward = gpGlobals->v_forward;
	pev->gravity = 0.5;

	pev->nextthink = gpGlobals->time + 0.1;

	pev->dmg = 150;
}


void CApacheHVR :: Precache( void )
{
	PRECACHE_MODEL("models/HVR.mdl");
	PRECACHE_SOUND ("weapons/rocket1.wav");
}


void CApacheHVR :: IgniteThink( void  )
{
	// pev->movetype = MOVETYPE_TOSS;

	// pev->movetype = MOVETYPE_FLY;
	pev->effects |= EF_LIGHT;

	// make rocket sound
	EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5 );

	// rocket trail
	BeamFollow( g_sModelIndexSmokePuff, 15, 5, Vector( 224, 224, 255 ), 255 );

	// set to accelerate
	SetThink( AccelerateThink );
	pev->nextthink = gpGlobals->time + 0.1;
}


void CApacheHVR :: Killed( entvars_t *pevAttacker, int iGib )
{
	STOP_SOUND( ENT(pev), CHAN_VOICE, "weapons/rocket1.wav" );
	CGrenade::Killed( pevAttacker, iGib );
}


void CApacheHVR :: AccelerateThink( void  )
{
	// check world boundaries
	if (pev->origin.x < -4096 || pev->origin.x > 4096 || pev->origin.y < -4096 || pev->origin.y > 4096 || pev->origin.z < -4096 || pev->origin.z > 4096)
	{
		UTIL_Remove( this );
		return;
	}

	// accelerate
	float flSpeed = pev->velocity.Length();
	if (flSpeed < 1800)
	{
		pev->velocity = pev->velocity + m_vecForward * 200;
	}

	// re-aim
	pev->angles = UTIL_VecToAngles( pev->velocity );

	pev->nextthink = gpGlobals->time + 0.1;
}




class CNAApache : public CApache
{
public:
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	int  Classify( void ) { return 14; };	// the Nephilim Alliance (no CLASS_ define)
	int  Shielded( void ) { if ( pev->armorvalue > 0 ) return 2; return 0; }
	BOOL CanBeTractored( void ) { return !Shielded(); }
	BOOL CanBeRepulsed( bool fForce ) { if ( !Shielded() || fForce ) return TRUE; return FALSE; }
	int HeatSignature( void ) { return 2; }
	const char *GetHudName( void ) { return "UFO"; }
	const char *GetKillDesc( void ) { return pev->health <= 0 ? "" : "an unidentified flying object"; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return pev->health <= 0 ? "was blown up by a crashing unidentified flying object" : "shot"; }
	const char *GetHitGroupDesc( int iHitGroup ) { return HitGroupIn( iHitGroup ); }
	int GetBattlePower( void );
	void Killed( entvars_t *pevAttacker, int iGib );

	void Hunt( void );
	void FireRocket( void );
	BOOL FireGun( void );
	void FireFlak( Vector &vecSrc, Vector &vecEnd );

	int  TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	CSprite *m_pSprite;	// the beam cannon's muzzle flare
	int m_iSpark;		// name ours: sprite of the beam's impact
};
LINK_ENTITY_TO_CLASS( monster_na_apache, CNAApache );

TYPEDESCRIPTION	CNAApache::m_SaveData[] = 
{
	DEFINE_FIELD( CNAApache, m_pSprite, FIELD_CLASSPTR ),
};
IMPLEMENT_SAVERESTORE( CNAApache, CApache );


void CNAApache :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/na_apache.mdl");
	UTIL_SetSize( pev, Vector( -32, -32, -64 ), Vector( 32, 32, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->flags |= FL_MONSTER;
	pev->takedamage		= DAMAGE_AIM;
	pev->health			= 2000;
	pev->armorvalue		= 5000;		// the shield

	if (pev->weapons <= -1)
	{
		APACHE_WEAPONS( pev )->iGun = RANDOM_LONG( 0, 4 );
		APACHE_WEAPONS( pev )->iMissile = RANDOM_LONG( 0, 4 );
	}

	m_flFieldOfView = -0.707; // 270 degrees

	pev->sequence = 0;
	ResetSequenceInfo( );
	pev->frame = RANDOM_LONG(0, 0xFF);

	InitBoneControllers();

	if (pev->spawnflags & SF_WAITFORTRIGGER)
	{
		SetUse( StartupUse );
	}
	else
	{
		SetThink( HuntThink );
		SetTouch( FlyTouch );
		pev->nextthink = gpGlobals->time + 1.0;
	}

	m_iRockets = 10;
}


void CNAApache::Precache( void )
{
	PRECACHE_MODEL("models/na_apache.mdl");

	pev->noise = MAKE_STRING("apache/ap_rotor1.wav");
	PRECACHE_SOUND("apache/ap_rotor1.wav");

	PRECACHE_SOUND("weapons/mortarhit.wav");

	m_iSpriteTexture = PRECACHE_MODEL( "sprites/white.spr" );

	PRECACHE_SOUND("turret/tu_fire1.wav");

	PRECACHE_MODEL("sprites/LRed.spr");
	PRECACHE_MODEL("sprites/BFlare1.spr");
	m_iSpark = PRECACHE_MODEL("sprites/xspark4.spr");
	PRECACHE_SOUND("weapons/debris3.wav");

	UTIL_PrecacheOther( "bio_flak" );
	UTIL_PrecacheOther( "lght_flak" );
	UTIL_PrecacheOther( "laser_pulse" );
	CLaserPulse::PrecacheColor( 4 );

	m_iExplode	= PRECACHE_MODEL( "sprites/fexplo.spr" );
	m_iBodyGibs = PRECACHE_MODEL( "models/metalplategibs_dark.mdl" );

	UTIL_PrecacheOther( "vortex_missile" );
	UTIL_PrecacheOther( "na_rocket" );
	UTIL_PrecacheOther( "rocket" );
	PRECACHE_MODEL("sprites/flux_t.spr");
}


void CNAApache :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "weapons2"))
	{
		if (pev->weapons == -1)
			APACHE_WEAPONS( pev )->iGun = RANDOM_LONG( 0, 4 );

		int iMissile = atoi( pkvd->szValue );
		if (iMissile == -1)
			iMissile = RANDOM_LONG( 0, 4 );
		APACHE_WEAPONS( pev )->iMissile = iMissile;

		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}


int CNAApache :: GetBattlePower( void )
{
	float flGunPower[] = { 30, 62.5, 50, 35, 50 };
	float flPower = flGunPower[APACHE_WEAPONS( pev )->iGun];

	switch (APACHE_WEAPONS( pev )->iMissile)
	{
	case 2:
		flPower += 250;
		break;
	case 3:
		flPower += 500;
		break;
	case 4:
		flPower += 437.5f;
		break;
	default:
		flPower += 200;
		break;
	}

	flPower *= 937.5f;
	if (Shielded())
		flPower += 1000;

	return flPower;
}


int CNAApache :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if (pevAttacker == pev)
		return 0;

	if (pev->takedamage == DAMAGE_NO)
		return 0;

	if (pev->armorvalue > 0)
	{
		// the shield takes it and flashes
		pev->armorvalue -= flDamage;
		pev->renderfx = kRenderFxGlowShell;
		pev->renderamt = 255;
		pev->rendercolor = Vector( 255, 0, 128 );
	}
	else
	{
		pev->health -= flDamage;
	}

	if (pev->armorvalue <= 0)
		pev->armortype = 5;	// the shield stays down this long (tenths of HuntThink's 0.1 s)

	if (pev->health <= 0)
	{
		Killed( pevAttacker, GIB_NORMAL );
		return 0;
	}

	return 1;
}


void CNAApache::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	// the shield takes everything
	if (pev->armorvalue > 0)
	{
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
		return;
	}

	// ignore blades
	if (ptr->iHitgroup == 6 && (bitsDamageType & (DMG_ENERGYBEAM|DMG_BULLET|DMG_CLUB)))
		return;

	// hit hard, hits cockpit, hits engines
	if (flDamage > 50 || ptr->iHitgroup == 1 || ptr->iHitgroup == 2)
	{
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
		m_iDoSmokePuff = 3 + (flDamage / 5.0);
	}
	else if (bitsDamageType & (DMG_BLAST|DMG_SHOCK|DMG_SONIC|DMG_ENERGYBEAM))
	{
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
	}
	else
	{
		UTIL_Ricochet( ptr->vecEndPos, 2.0 );
	}
}


BOOL CNAApache :: FireGun( )
{
	UTIL_MakeAimVectors( pev->angles );

	Vector posGun, angGun;
	GetAttachment( 1, posGun, angGun );

	Vector vecTarget = m_posTarget - posGun;
	float flDist = vecTarget.Length();
	vecTarget = vecTarget.Normalize( );

	Vector vecOut;

	vecOut.x = DotProduct( gpGlobals->v_forward, vecTarget );
	vecOut.y = -DotProduct( gpGlobals->v_right, vecTarget );
	vecOut.z = DotProduct( gpGlobals->v_up, vecTarget );

	Vector angles = UTIL_VecToAngles (vecOut);

	angles.x = -angles.x;
	if (angles.y > 180)
		angles.y = angles.y - 360;
	if (angles.y < -180)
		angles.y = angles.y + 360;
	if (angles.x > 180)
		angles.x = angles.x - 360;
	if (angles.x < -180)
		angles.x = angles.x + 360;

	if (angles.x > m_angGun.x)
		m_angGun.x = min( angles.x, m_angGun.x + 12 );
	if (angles.x < m_angGun.x)
		m_angGun.x = max( angles.x, m_angGun.x - 12 );
	if (angles.y > m_angGun.y)
		m_angGun.y = min( angles.y, m_angGun.y + 12 );
	if (angles.y < m_angGun.y)
		m_angGun.y = max( angles.y, m_angGun.y - 12 );

	m_angGun.y = SetBoneController( 0, m_angGun.y );
	m_angGun.x = SetBoneController( 1, m_angGun.x );

	Vector posBarrel, angBarrel;
	GetAttachment( 0, posBarrel, angBarrel );
	Vector vecGun = (posBarrel - posGun).Normalize( );

	if (DotProduct( vecGun, vecTarget ) > 0.98)
	{
		if (GunType() >= 3)
		{
			// bionanite or energy flak
			if (m_flNextAttack <= gpGlobals->time)
			{
				m_flNextAttack = gpGlobals->time;
				FireFlak( posGun, posGun + vecGun * flDist );
				return TRUE;
			}
		}
		else if (GunType() == 0)
		{
			FireBullets( 1, posGun, vecGun, VECTOR_CONE_4DEGREES, 8192, BULLET_MONSTER_TRACER2, 1, 24 );
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "turret/tu_fire1.wav", 1, 0.3);
			return TRUE;
		}
		else if (GunType() == 2)
		{
			// pulse cannon
			CLaserPulse *pPulse = CLaserPulse::Shoot( posGun, edict(), Vector( 255, 255, 255 ), 2, 0.25, 4 );
			pPulse->pev->velocity = vecGun * 2000;
			pPulse->m_iszKillDesc = MAKE_STRING( "a hyperspace flux cannon" );
			pPulse->pev->dmg = 50;
			pPulse->pev->iuser2 = DMG_ENERGYBEAM | DMG_ALWAYSGIB;	// damage type
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/debris3.wav", 1, 0.3 );
			return TRUE;
		}
		else if (GunType() == 1)
		{
			// beam cannon
			TraceResult tr;
			UTIL_TraceLine( posGun, posGun + vecGun * 8192, dont_ignore_monsters, ENT( pev ), &tr );

			if (!m_pBeam)
			{
				m_pBeam = CBeam::BeamCreate( "sprites/LRed.spr", 80 );
				m_pBeam->PointEntInit( pev->origin, entindex( ) );
				m_pBeam->SetEndAttachment( 1 );
				m_pBeam->SetColor( 255, 255, 255 );
				m_pBeam->SetBrightness( 192 );
				m_pBeam->m_iszKillDesc = MAKE_STRING( "a hyperspace flux beam" );
				m_pBeam->SetScrollRate( 35 );
				m_pBeam->pev->owner = edict();
			}

			if (!m_pSprite)
			{
				m_pSprite = CSprite::SpriteCreate( "sprites/bflare1.spr", pev->origin, FALSE );
				m_pSprite->Spawn( );
				m_pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxDistort );
				m_pSprite->SetAttachment( edict(), 1 );
				m_pSprite->SetScale( 0.5 );
			}

			// scientists burn up
			float flDamage = 50;
			CBaseEntity *pHit = (CBaseEntity *)GET_PRIVATE( tr.pHit );
			if (pHit && pHit->Classify() == CLASS_HUMAN_PASSIVE)
				flDamage = pHit->pev->health + 50;

			m_pBeam->BeamDamageInstant( &tr, flDamage );
			m_pBeam->SetStartPos( tr.vecEndPos );

			if (tr.flFraction != 1.0)
			{
				UTIL_DecalTrace( &tr, DECAL_SMALLSCORCH1 + RANDOM_LONG( 0, 2 ) );

				MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
					WRITE_BYTE( TE_SPRITE );
					WRITE_COORD( tr.vecEndPos.x );
					WRITE_COORD( tr.vecEndPos.y );
					WRITE_COORD( tr.vecEndPos.z );
					WRITE_SHORT( m_iSpark );
					WRITE_BYTE( 10 ); // scale * 10
					WRITE_BYTE( 255 ); // brightness
				MESSAGE_END();
			}
		}
		return TRUE;
	}
	else
	{
		if (m_pBeam)
		{
			UTIL_Remove( m_pBeam );
			m_pBeam = NULL;
		}
		if (m_pSprite)
		{
			UTIL_Remove( m_pSprite );
			m_pSprite = NULL;
		}
	}
	return FALSE;
}


void CNAApache :: FireFlak( Vector &vecSrc, Vector &vecEnd )
{
	Vector vecAngles = Vector( RANDOM_FLOAT( -180, 180 ), RANDOM_FLOAT( -180, 180 ), RANDOM_FLOAT( -180, 180 ) );
	UTIL_MakeVectors( vecAngles );
	Vector vecDest = vecEnd + gpGlobals->v_forward * RANDOM_FLOAT( 0, 96 );

	CBaseEntity *pFlak;
	if (APACHE_WEAPONS( pev )->iGun == 3)
	{
		// bionanite flak
		pFlak = CBaseEntity::Create( "bio_flak", vecSrc, g_vecZero, edict() );
	}
	else
	{
		// energy flak
		pFlak = CBaseEntity::Create( "lght_flak", vecSrc, g_vecZero, edict() );
		m_flNextAttack = gpGlobals->time + 0.2;
	}
	pFlak->pev->velocity = vecDest - vecSrc;

	float flTime = pFlak->pev->velocity.Length() / 2000;
	pFlak->pev->nextthink = gpGlobals->time + max( 0.1, flTime ) + 0.1;
	pFlak->pev->velocity = pFlak->pev->velocity.Normalize() * 2000;

	EMIT_SOUND( ENT(pev), CHAN_WEAPON, "turret/tu_fire1.wav", 1.0, 0.6 );
	pev->effects |= EF_MUZZLEFLASH;
}



void CNAApache :: FireRocket( void )
{
	static float side = 1.0;
	static int count;

	if (m_iRockets <= 0)
		return;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + 1.5 * (gpGlobals->v_forward * 21 + gpGlobals->v_right * 70 * side + gpGlobals->v_up * -79);

	switch( m_iRockets % 5)
	{
	case 0:	vecSrc = vecSrc + gpGlobals->v_right * 10; break;
	case 1: vecSrc = vecSrc - gpGlobals->v_right * 10; break;
	case 2: vecSrc = vecSrc + gpGlobals->v_up * 10; break;
	case 3: vecSrc = vecSrc - gpGlobals->v_up * 10; break;
	case 4: break;
	}

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
		WRITE_BYTE( TE_SMOKE );
		WRITE_COORD( vecSrc.x );
		WRITE_COORD( vecSrc.y );
		WRITE_COORD( vecSrc.z );
		WRITE_SHORT( g_sModelIndexSmoke );
		WRITE_BYTE( 20 ); // scale * 10
		WRITE_BYTE( 12 ); // framerate
	MESSAGE_END();

	int iMissile = MissileType();
	if (iMissile == 2)
	{
		// bionic missile
		CRocket *pRocket = CRocket::CreateRocket( vecSrc, pev->angles, edict(), Vector( 0, 255, 170 ), 0 );	// sic: used unchecked
		pRocket->pev->dmg = 200;
		pRocket->pev->velocity = gpGlobals->v_forward * 100;
		pRocket->pev->speed = 1200;
		pRocket->m_flFieldOfView = 0.7;
		pRocket->SetThink( CRocket::SeekThink );
		pRocket->m_iszKillDesc = MAKE_STRING( "a bionic missile" );
		SET_MODEL( ENT( pRocket->pev ), "models/HVR.mdl" );
	}
	else if (iMissile == 3)
	{
		// vortex missile
		CBaseMonster *pRocket = (CBaseMonster *)CBaseEntity::Create( "vortex_missile", vecSrc, Vector( -pev->angles.x, pev->angles.y, pev->angles.z ), edict() );
		if (pRocket)
		{
			pRocket->pev->dmg = 500;
			pRocket->pev->velocity = gpGlobals->v_forward * 100;
			SET_MODEL( ENT( pRocket->pev ), "models/HVR.mdl" );
			pRocket->m_hEnemy = m_hEnemy;
		}
	}
	else if (iMissile == 4)
	{
		// flux torpedo
		CRocket *pRocket = CRocket::CreateRocket( vecSrc, pev->angles, edict(), Vector( 255, 0, 128 ), MAKE_STRING( "sprites/flux_t.spr" ) );
		if (pRocket)
		{
			pRocket->SetThink( CRocket::TargetThink );
			pRocket->pev->speed = 1500;
			pRocket->pev->renderamt = 255;
			pRocket->pev->rendermode = kRenderTransAdd;
			pRocket->m_flSeekTime = gpGlobals->time + 0.2;
			pRocket->pev->velocity = gpGlobals->v_forward * 256;
			pRocket->m_hEnemy = m_hEnemy;
			pRocket->pev->dmg = 300;
			pRocket->pev->spawnflags |= 8;
			pRocket->m_iszKillDesc = MAKE_STRING( "a hyperspace flux torpedo" );
		}
	}
	else
	{
		CBaseEntity *pRocket = CBaseEntity::Create( "na_rocket", vecSrc, pev->angles, edict() );
		if (pRocket)
		{
			// napalm rocket
			if (iMissile == 1)
				pRocket->pev->spawnflags |= 3;
			pRocket->pev->velocity = pev->velocity + gpGlobals->v_forward * 100;
		}
	}

	m_iRockets--;

	side = - side;
}



void CNAApache :: Hunt( void )
{
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	ShowDamage( );

	if ( m_pGoalEnt == NULL && !FStringNull(pev->target) )// this monster has a target
	{
		m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
		if (m_pGoalEnt)
		{
			m_posDesired = m_pGoalEnt->pev->origin;
			UTIL_MakeAimVectors( m_pGoalEnt->pev->angles );
			m_vecGoal = gpGlobals->v_forward;
		}
	}

	// if (m_hEnemy == NULL)
	{
		Look( 4092 );
		m_hEnemy = BestVisibleEnemy( );
	}

	// generic speed up
	if (m_flGoalSpeed < 800)
		m_flGoalSpeed += 5;

	if (m_hEnemy != NULL)
	{
		// ALERT( at_console, "%s\n", STRING( m_hEnemy->pev->classname ) );
		if (FVisible( m_hEnemy ))
		{
			if (m_flLastSeen < gpGlobals->time - 5)
				m_flPrevSeen = gpGlobals->time;
			m_flLastSeen = gpGlobals->time;
			m_posTarget = m_hEnemy->Center( );
		}
		else
		{
			m_hEnemy = NULL;
		}
	}

	m_vecTarget = (m_posTarget - pev->origin).Normalize();

	float flLength = (pev->origin - m_posDesired).Length();

	if (m_pGoalEnt)
	{
		// ALERT( at_console, "%.0f\n", flLength );

		if (flLength < 128)
		{
			m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( m_pGoalEnt->pev->target ) );
			if (m_pGoalEnt)
			{
				m_posDesired = m_pGoalEnt->pev->origin;
				UTIL_MakeAimVectors( m_pGoalEnt->pev->angles );
				m_vecGoal = gpGlobals->v_forward;
				flLength = (pev->origin - m_posDesired).Length();
			}
		}
	}
	else
	{
		m_posDesired = pev->origin;
	}

	if (flLength > 250) // 500
	{
		// float flLength2 = (m_posTarget - pev->origin).Length() * (1.5 - DotProduct((m_posTarget - pev->origin).Normalize(), pev->velocity.Normalize() ));
		// if (flLength2 < flLength)
		if (m_flLastSeen + 90 > gpGlobals->time && DotProduct( (m_posTarget - pev->origin).Normalize(), (m_posDesired - pev->origin).Normalize( )) > 0.25)
		{
			m_vecDesired = (m_posTarget - pev->origin).Normalize( );
		}
		else
		{
			m_vecDesired = (m_posDesired - pev->origin).Normalize( );
		}
	}
	else
	{
		m_vecDesired = m_vecGoal;
	}

	Flight( );

	// ALERT( at_console, "%.0f %.0f %.0f\n", gpGlobals->time, m_flLastSeen, m_flPrevSeen );
	if ((m_flLastSeen + 1 > gpGlobals->time) && (m_flPrevSeen + 2 < gpGlobals->time))
	{
		if (FireGun( ))
		{
			// slow down if we're fireing
			if (m_flGoalSpeed > 400)
				m_flGoalSpeed = 400;
		}
	}
	else
	{
		if (m_pBeam)
		{
			UTIL_Remove( m_pBeam );
			m_pBeam = NULL;
		}
	}

	UTIL_MakeAimVectors( pev->angles );
	Vector vecEst = (gpGlobals->v_forward * 800 + pev->velocity).Normalize( );
	// ALERT( at_console, "%d %d %d %4.2f\n", pev->angles.x < 0, DotProduct( pev->velocity, gpGlobals->v_forward ) > -100, m_flNextRocket < gpGlobals->time, DotProduct( m_vecTarget, vecEst ) );

	if ((m_iRockets % 2) == 1)
	{
		FireRocket( );
		m_flNextRocket = gpGlobals->time + 0.5;
		if (APACHE_WEAPONS( pev )->iMissile == 3)
			m_flNextRocket = gpGlobals->time + 3;
		if (m_iRockets <= 0)
		{
			m_flNextRocket = gpGlobals->time + 10;
			m_iRockets = 10;
		}
	}
	else if (DotProduct( pev->velocity, gpGlobals->v_forward ) > -100 && m_flNextRocket < gpGlobals->time)
	{
		if (m_flLastSeen + 60 > gpGlobals->time)
		{
			if (m_hEnemy != NULL)
			{
				float flDot = 0.965;
				int iMissile = APACHE_WEAPONS( pev )->iMissile;
				if (iMissile == 2)
					flDot = 0.7;
				else if (iMissile == 3 || iMissile == 4)
					flDot = 0.9;

				// make sure it's a good shot
				if (DotProduct( m_vecTarget, vecEst) > flDot)
				{
					TraceResult tr;

					UTIL_TraceLine( pev->origin, pev->origin + vecEst * 4096, ignore_monsters, edict(), &tr );
					if ((tr.vecEndPos - m_posTarget).Length() < 1024)
						FireRocket( );
				}
			}
			else
			{
				TraceResult tr;

				UTIL_TraceLine( pev->origin, pev->origin + vecEst * 4096, dont_ignore_monsters, edict(), &tr );
				// just fire when close
				if ((tr.vecEndPos - m_posTarget).Length() < 512)
					FireRocket( );
			}
		}
	}

	if (m_pSprite && !m_pBeam)
	{
		UTIL_Remove( m_pSprite );
		m_pSprite = NULL;
	}

	// the shield: down for 5 seconds once broken, then recharging
	if (pev->armortype > 0)
	{
		pev->renderfx = kRenderFxNone;
		pev->armortype -= 0.1;
		if (pev->armortype <= 0)
		{
			pev->armorvalue = 0;
			pev->renderfx = kRenderFxGlowShell;
			pev->renderamt = 255;
			pev->rendercolor = Vector( 255, 0, 128 );
		}
	}
	else if (pev->armorvalue < 5000)
	{
		pev->armorvalue += 0.25;
	}

	if (pev->health < 2000)
		pev->health += 0.05;

	// fade the shield's glow
	if (pev->renderfx == kRenderFxGlowShell)
	{
		pev->renderamt -= 10;
		if (pev->renderamt < 0)
		{
			pev->renderfx = kRenderFxNone;
			pev->renderamt = 0;
			pev->rendercolor = g_vecZero;
		}
		else
		{
			pev->rendercolor = Vector( pev->renderamt, 0, pev->renderamt * 0.5 );
		}
	}
}


void CNAApache :: Killed( entvars_t *pevAttacker, int iGib )
{
	if (m_pSprite)
	{
		UTIL_Remove( m_pSprite );
		m_pSprite = NULL;
	}
	CApache::Killed( pevAttacker, iGib );
}




class CApacheNAR : public CApacheHVR
{
	const char *GetKillDesc( void ) { return "a bionic missile"; }
	void EXPORT IgniteThink( void );
};
LINK_ENTITY_TO_CLASS( na_rocket, CApacheNAR );


void CApacheNAR :: IgniteThink( void  )
{
	pev->effects |= EF_LIGHT;

	// make rocket sound
	EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5 );

	// rocket trail
	BeamFollow( g_sModelIndexSmokePuff, 15, 5, Vector( 0, 170, 255 ), 255 );

	pev->dmg = 200;

	// set to accelerate
	SetThink( AccelerateThink );
	pev->nextthink = gpGlobals->time + 0.1;
}


#endif
