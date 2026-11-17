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
/*

===== turret.cpp ========================================================

*/

//
// TODO: 
//		Take advantage of new monster fields like m_hEnemy and get rid of that OFFSET() stuff
//		Revisit enemy validation stuff, maybe it's not necessary with the newest monster code
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "effects.h"
#include "soundent.h"

extern Vector VecBModelOrigin( entvars_t* pevBModel );

#define TURRET_SHOTS	2
#define TURRET_RANGE	(100 * 12)
#define TURRET_SPREAD	Vector( 0, 0, 0 )
#define TURRET_TURNRATE	30		//angles per 0.1 second
#define TURRET_MAXWAIT	15		// seconds turret will stay active w/o a target
#define TURRET_MAXSPIN	5		// seconds turret barrel will spin w/o a target
#define TURRET_MACHINE_VOLUME	0.5
#define TURRET_BULLET_RANGE		8192	// name ours: how far the turrets' bullets reach

#define SF_MONSTER_TURRET_INVULNERABLE	128	// fgd "Invulnerable"
#define SF_MONSTER_TURRET_ALLY			256	// name ours: not in the fgd; the turret counts as a player ally

typedef enum
{
	TURRET_ANIM_NONE = 0,
	TURRET_ANIM_FIRE,
	TURRET_ANIM_SPIN,
	TURRET_ANIM_DEPLOY,
	TURRET_ANIM_RETIRE,
	TURRET_ANIM_DIE,
} TURRET_ANIM;

class CBaseTurret : public CBaseMonster
{
public:
	void Spawn(void);
	virtual void Precache(void);
	void KeyValue( KeyValueData *pkvd );
	void EXPORT TurretUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	
	virtual void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	virtual int	 TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	virtual int	 Classify(void);

	int BloodColor( void ) { return DONT_BLEED; }
	void GibMonster( void ) {}	// UNDONE: Throw turret gibs?

	// rockets seek a firing or spinning turret less
	int		HeatSignature( void ) { return ( pev->sequence == TURRET_ANIM_FIRE || pev->sequence == TURRET_ANIM_SPIN ) ? 1 : 2; }
	BOOL	CanBeRepulsed( bool fForce ) { return FALSE; }
	BOOL	CanBeTeleported( void ) { return FALSE; }
	// hand grenades aren't thrown at ceiling turrets
	BOOL	CanBeGrenaded( int fLauncher ) { if ( !fLauncher && m_iOrientation ) return FALSE; return TRUE; }
	BOOL	IsGrenadeTarget( void ) { return !m_iOrientation; }
	int		GetOnState( void ) { if ( m_iOn ) return IsAlive(); return 0; }

	// Think functions

	virtual void ActiveThink(void);
	void EXPORT CallActiveThink(void);	// name ours: ActiveThink is virtual, SetThink takes this
	void MonsterThink(void);			// the SDK's SearchThink, now run through CallMonsterThink
	void EXPORT AutoSearchThink(void);
	void EXPORT TurretDeath(void);

	virtual void SpinDownCall(void) { m_iSpin = 0; }
	virtual void SpinUpCall(void) { m_iSpin = 1; }

	// void SpinDown(void);
	// float EXPORT SpinDownCall( void ) { return SpinDown(); }

	// virtual float SpinDown(void) { return 0;}
	// virtual float Retire(void) { return 0;}

	void EXPORT Deploy(void);
	void EXPORT Retire(void);
	
	void EXPORT Initialize(void);

	virtual void Ping(void);
	virtual void EyeOn(void);
	virtual void EyeOff(void);

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];

	// other functions
	void SetTurretAnim(TURRET_ANIM anim);
	virtual int MoveTurret(void);
	virtual void Shoot(Vector &vecSrc, Vector &vecDirToEnemy) { };

	float m_flMaxSpin;		// Max time to spin the barrel w/o a target
	int m_iSpin;

	CSprite *m_pEyeGlow;
	int		m_eyeBrightness;

	int	m_iDeployHeight;
	int	m_iRetractHeight;
	int m_iMinPitch;

	int m_iBaseTurnRate;	// angles per second
	float m_fTurnRate;		// actual turn rate
	int m_iOrientation;		// 0 = floor, 1 = Ceiling
	int	m_iOn;
	int m_fBeserk;			// Sometimes this bitch will just freak out
	int m_iAutoStart;		// true if the turret auto deploys when a target
							// enters its range

	Vector m_vecLastSight;
	float m_flLastSight;	// Last time we saw a target
	float m_flMaxWait;		// Max time to seach w/o a target
	float m_flSightRange;	// "range"; at least TURRET_RANGE

	// movement
	float	m_flStartYaw;
	Vector	m_vecCurAngles;
	Vector	m_vecGoalAngles;


	float	m_flPingTime;	// Time until the next ping, used when searching
	float	m_flSpinUpTime;	// Amount of time until the barrel should spin down when searching
};


TYPEDESCRIPTION	CBaseTurret::m_SaveData[] = 
{
	DEFINE_FIELD( CBaseTurret, m_flMaxSpin, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseTurret, m_iSpin, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseTurret, m_pEyeGlow, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBaseTurret, m_eyeBrightness, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iDeployHeight, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iRetractHeight, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iMinPitch, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseTurret, m_iBaseTurnRate, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_fTurnRate, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseTurret, m_iOrientation, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iOn, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_fBeserk, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iAutoStart, FIELD_INTEGER ),


	DEFINE_FIELD( CBaseTurret, m_vecLastSight, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseTurret, m_flLastSight, FIELD_TIME ),
	DEFINE_FIELD( CBaseTurret, m_flMaxWait, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseTurret, m_flSightRange, FIELD_FLOAT ),

	DEFINE_FIELD( CBaseTurret, m_flStartYaw, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseTurret, m_vecCurAngles, FIELD_VECTOR ),
	DEFINE_FIELD( CBaseTurret, m_vecGoalAngles, FIELD_VECTOR ),

	DEFINE_FIELD( CBaseTurret, m_flPingTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseTurret, m_flSpinUpTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CBaseTurret, CBaseMonster );

class CTurret : public CBaseTurret
{
public:
	void Spawn(void);
	void Precache(void);
	// Think functions
	void SpinUpCall(void);
	void SpinDownCall(void);

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];

	// other functions
	void Shoot(Vector &vecSrc, Vector &vecDirToEnemy);

	const char *GetKillDesc( void ) { return "a machine-gun turret"; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return "shot"; }
	const char *GetHitGroupDesc( int iHitGroup ) { return HitGroupIn( iHitGroup ); }
	int GetBattlePower( void );

private:
	int m_iStartSpin;

};
TYPEDESCRIPTION	CTurret::m_SaveData[] = 
{
	DEFINE_FIELD( CTurret, m_iStartSpin, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CTurret, CBaseTurret );

//=========================================================
// GetBattlePower - the target ID's "battle power"
//=========================================================
int CTurret::GetBattlePower( void )
{
	return ( pev->max_health * 0.1 ) * ( gSkillData.monDmg12MM * 12.5 );
}


class CMiniTurret : public CBaseTurret
{
public:
	void Spawn( );
	void Precache(void);
	// other functions
	void Shoot(Vector &vecSrc, Vector &vecDirToEnemy);

	const char *GetKillDesc( void ) { return "a small machine-gun turret"; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return "shot"; }
	const char *GetHitGroupDesc( int iHitGroup ) { return HitGroupIn( iHitGroup ); }
	int GetBattlePower( void );
};


LINK_ENTITY_TO_CLASS( monster_turret, CTurret );
LINK_ENTITY_TO_CLASS( monster_miniturret, CMiniTurret );

void CBaseTurret::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "maxsleep"))
	{
		m_flMaxWait = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "orientation"))
	{
		m_iOrientation = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;

	}
	else if (FStrEq(pkvd->szKeyName, "turnrate"))
	{
		m_iBaseTurnRate = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "range"))
	{
		m_flSightRange = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "style") ||
			 FStrEq(pkvd->szKeyName, "height") ||
			 FStrEq(pkvd->szKeyName, "value1") ||
			 FStrEq(pkvd->szKeyName, "value2") ||
			 FStrEq(pkvd->szKeyName, "value3"))
		pkvd->fHandled = TRUE;
	else
		CBaseMonster::KeyValue( pkvd );
}


void CBaseTurret::Spawn()
{ 
	Precache( );
	pev->nextthink		= gpGlobals->time + 1;
	pev->movetype		= MOVETYPE_FLY;
	pev->sequence		= 0;
	pev->frame			= 0;
	pev->solid			= SOLID_SLIDEBOX;
	pev->takedamage		= DAMAGE_AIM;
	if ( pev->spawnflags & SF_MONSTER_TURRET_INVULNERABLE )
		pev->takedamage	= DAMAGE_NO;
	pev->max_health		= pev->health;

	SetBits (pev->flags, FL_MONSTER);
	SetUse( TurretUse );

	if (( pev->spawnflags & SF_MONSTER_TURRET_AUTOACTIVATE ) 
		 && !( pev->spawnflags & SF_MONSTER_TURRET_STARTINACTIVE ))
	{
		m_iAutoStart = TRUE;
	}

	ResetSequenceInfo( );
	SetBoneController( 0, 0 );
	SetBoneController( 1, 0 );
	m_flFieldOfView = VIEW_FIELD_FULL;
	if ( m_flSightRange < TURRET_RANGE )
		m_flSightRange = TURRET_RANGE;
	m_flNextAttack = gpGlobals->time;
	m_flDelay = 0;
}


void CBaseTurret::Precache( )
{
	PRECACHE_SOUND ("turret/tu_fire1.wav");
	PRECACHE_SOUND ("turret/tu_ping.wav");
	PRECACHE_SOUND ("turret/tu_active2.wav");
	PRECACHE_SOUND ("turret/tu_die.wav");
	PRECACHE_SOUND ("turret/tu_die2.wav");
	PRECACHE_SOUND ("turret/tu_die3.wav");
	// PRECACHE_SOUND ("turret/tu_retract.wav"); // just use deploy sound to save memory
	PRECACHE_SOUND ("turret/tu_deploy.wav");
	PRECACHE_SOUND ("turret/tu_spinup.wav");
	PRECACHE_SOUND ("turret/tu_spindown.wav");
	PRECACHE_SOUND ("turret/tu_search.wav");
	PRECACHE_SOUND ("turret/tu_alert.wav");
}

#define TURRET_GLOW_SPRITE "sprites/flare3.spr"

void CTurret::Spawn()
{ 
	Precache( );
	SET_MODEL(ENT(pev), "models/turret.mdl");
	pev->health			= gSkillData.turretHealth;
	m_HackedGunPos		= Vector( 0, 0, 12.75 );
	m_flMaxSpin =		TURRET_MAXSPIN;
	pev->view_ofs.z = 12.75;

	CBaseTurret::Spawn( );

	m_iRetractHeight = 16;
	m_iDeployHeight = 32;
	m_iMinPitch	= -15;
	UTIL_SetSize(pev, Vector(-32, -32, -m_iRetractHeight), Vector(32, 32, m_iRetractHeight));
	
	SetThink(Initialize);	

	m_pEyeGlow = CSprite::SpriteCreate( TURRET_GLOW_SPRITE, pev->origin, FALSE );
	m_pEyeGlow->SetTransparency( kRenderGlow, 255, 0, 0, 0, kRenderFxNoDissipation );
	m_pEyeGlow->SetAttachment( edict(), 2 );
	m_eyeBrightness = 0;

	pev->nextthink = gpGlobals->time + 0.3; 
}

void CTurret::Precache()
{
	CBaseTurret::Precache( );
	PRECACHE_MODEL ("models/turret.mdl");	
	PRECACHE_MODEL (TURRET_GLOW_SPRITE);
}

void CMiniTurret::Spawn()
{ 
	Precache( );
	SET_MODEL(ENT(pev), "models/miniturret.mdl");
	pev->health			= gSkillData.miniturretHealth;
	m_HackedGunPos		= Vector( 0, 0, 12.75 );
	m_flMaxSpin = 0;
	pev->view_ofs.z = 12.75;

	CBaseTurret::Spawn( );
	m_iRetractHeight = 16;
	m_iDeployHeight = 32;
	m_iMinPitch	= -15;
	UTIL_SetSize(pev, Vector(-16, -16, -m_iRetractHeight), Vector(16, 16, m_iRetractHeight));

	SetThink(Initialize);	
	pev->nextthink = gpGlobals->time + 0.3; 
}


void CMiniTurret::Precache()
{
	CBaseTurret::Precache( );
	PRECACHE_MODEL ("models/miniturret.mdl");	
	PRECACHE_SOUND("weapons/hks1.wav");
	PRECACHE_SOUND("weapons/hks2.wav");
	PRECACHE_SOUND("weapons/hks3.wav");
}

int CMiniTurret::GetBattlePower( void )
{
	return ( pev->max_health * 0.1 ) * ( gSkillData.monDmg12MM * 12.5 );
}

void CBaseTurret::Initialize(void)
{
	m_iOn = 0;
	m_fBeserk = 0;
	m_iSpin = 0;

	SetBoneController( 0, 0 );
	SetBoneController( 1, 0 );

	if (m_iBaseTurnRate == 0) m_iBaseTurnRate = TURRET_TURNRATE;
	if (m_flMaxWait == 0) m_flMaxWait = TURRET_MAXWAIT;
	m_flStartYaw = pev->angles.y;
	if (m_iOrientation == 1)
	{
		pev->idealpitch = 180;
		pev->angles.x = 180;
		pev->view_ofs.z = -pev->view_ofs.z;
		pev->effects |= EF_INVLIGHT;
		pev->angles.y = pev->angles.y + 180;
		if (pev->angles.y > 360)
			pev->angles.y = pev->angles.y - 360;
	}

	m_vecGoalAngles.x = 0;

	if (m_iAutoStart)
	{
		m_flLastSight = gpGlobals->time + m_flMaxWait;
		SetThink(AutoSearchThink);		
		pev->nextthink = gpGlobals->time + .1;
	}
	else
		SetThink(SUB_DoNothing);
}

void CBaseTurret::TurretUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_iOn ) )
		return;

	if (m_iOn)
	{
		m_hEnemy = NULL;
		pev->nextthink = gpGlobals->time + 0.1;
		m_iAutoStart = FALSE;// switching off a turret disables autostart
		//!!!! this should spin down first!!BUGBUG
		SetThink(Retire);
	}
	else 
	{
		pev->nextthink = gpGlobals->time + 0.1; // turn on delay

		// if the turret is flagged as an autoactivate turret, re-enable it's ability open self.
		if ( pev->spawnflags & SF_MONSTER_TURRET_AUTOACTIVATE )
		{
			m_iAutoStart = TRUE;
		}
		
		SetThink(Deploy);
	}
}


void CBaseTurret::Ping( void )
{
	// make the pinging noise every second while searching
	if (m_flPingTime == 0)
		m_flPingTime = gpGlobals->time + 1;
	else if (m_flPingTime <= gpGlobals->time)
	{
		m_flPingTime = gpGlobals->time + 1;
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "turret/tu_ping.wav", 1, ATTN_NORM);
		EyeOn( );
	}
	else if (m_eyeBrightness > 0)
	{
		EyeOff( );
	}
}


void CBaseTurret::EyeOn( )
{
	if (m_pEyeGlow)
	{
		if (m_eyeBrightness != 255)
		{
			m_eyeBrightness = 255;
		}
		m_pEyeGlow->SetBrightness( m_eyeBrightness );
	}
}


void CBaseTurret::EyeOff( )
{
	if (m_pEyeGlow)
	{
		if (m_eyeBrightness > 0)
		{
			m_eyeBrightness = max( 0, m_eyeBrightness - 30 );
			m_pEyeGlow->SetBrightness( m_eyeBrightness );
		}
	}
}


void CBaseTurret::ActiveThink(void)
{
	int fAttack = 0;
	Vector vecDirToEnemy;

	pev->nextthink = gpGlobals->time + 0.1;
	StudioFrameAdvance( );

	if ((!m_iOn) || (m_hEnemy == NULL))
	{
		m_hEnemy = NULL;
		m_flLastSight = gpGlobals->time + m_flMaxWait;
		SetThink(CallMonsterThink);
		return;
	}
	
	// if it's dead, look for something new
	if ( !m_hEnemy->IsAlive() )
	{
		if (!m_flLastSight)
		{
			m_flLastSight = gpGlobals->time + 0.5; // continue-shooting timeout
		}
		else
		{
			if (gpGlobals->time > m_flLastSight)
			{	
				m_hEnemy = NULL;
				m_flLastSight = gpGlobals->time + m_flMaxWait;
				SetThink(CallMonsterThink);
				return;
			}
		}
	}

	Vector vecMid = pev->origin + pev->view_ofs;
	Vector vecMidEnemy = m_hEnemy->BodyTarget( vecMid );

	// Look for our current enemy
	int fEnemyVisible = FBoxVisible(pev, m_hEnemy->pev, vecMidEnemy );	

	vecDirToEnemy = vecMidEnemy - vecMid;	// calculate dir and dist to enemy
	float flDistToEnemy = vecDirToEnemy.Length();

	Vector vec = UTIL_VecToAngles(vecMidEnemy - vecMid);	

	// Current enmey is not visible.
	if (!fEnemyVisible || (flDistToEnemy > m_flSightRange))
	{
		if (!m_flLastSight)
			m_flLastSight = gpGlobals->time + 0.5;
		else
		{
			// Should we look for a new target?
			if (gpGlobals->time > m_flLastSight)
			{
				m_hEnemy = NULL;
				m_flLastSight = gpGlobals->time + m_flMaxWait;
				SetThink(CallMonsterThink);
				return;
			}
		}
		fEnemyVisible = 0;
	}
	else
	{
		m_vecLastSight = vecMidEnemy;
	}

	UTIL_MakeAimVectors(m_vecCurAngles);	

	/*
	ALERT( at_console, "%.0f %.0f : %.2f %.2f %.2f\n", 
		m_vecCurAngles.x, m_vecCurAngles.y,
		gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_forward.z );
	*/
	
	Vector vecLOS = vecDirToEnemy; //vecMid - m_vecLastSight;
	vecLOS = vecLOS.Normalize();

	// Is the Gun looking at the target
	if (DotProduct(vecLOS, gpGlobals->v_forward) <= 0.866) // 30 degree slop
		fAttack = FALSE;
	else
		fAttack = TRUE;

	// fire the gun
	if (m_iSpin && ((fAttack) || (m_fBeserk)))
	{
		Vector vecSrc, vecAng;
		GetAttachment( 0, vecSrc, vecAng );
		SetTurretAnim(TURRET_ANIM_FIRE);
		Shoot(vecSrc, gpGlobals->v_forward );
	} 
	else
	{
		SetTurretAnim(TURRET_ANIM_SPIN);
	}

	//move the gun
	if (m_fBeserk)
	{
		if (RANDOM_LONG(0,9) == 0)
		{
			m_vecGoalAngles.y = RANDOM_FLOAT(0,360);
			m_vecGoalAngles.x = RANDOM_FLOAT(0,90) - 90 * m_iOrientation;
			TakeDamage(pev,pev,1, DMG_GENERIC); // don't beserk forever
			return;
		}
	} 
	else if (fEnemyVisible)
	{
		if (vec.y > 360)
			vec.y -= 360;

		if (vec.y < 0)
			vec.y += 360;

		//ALERT(at_console, "[%.2f]", vec.x);
		
		if (vec.x < -180)
			vec.x += 360;

		if (vec.x > 180)
			vec.x -= 360;

		// now all numbers should be in [1...360]
		// pin to turret limitations to [-90...15]

		if (m_iOrientation == 0)
		{
			if (vec.x > 90)
				vec.x = 90;
			else if (vec.x < m_iMinPitch)
				vec.x = m_iMinPitch;
		}
		else
		{
			if (vec.x < -90)
				vec.x = -90;
			else if (vec.x > -m_iMinPitch)
				vec.x = -m_iMinPitch;
		}

		// ALERT(at_console, "->[%.2f]\n", vec.x);

		m_vecGoalAngles.y = vec.y;
		m_vecGoalAngles.x = vec.x;

	}

	SpinUpCall();
	MoveTurret();
}


void CBaseTurret::CallActiveThink(void)
{
	ActiveThink();
}


//=========================================================
// The turrets that wait m_flDelay ("delay") between shots:
// the TFC sentry guns and the sphere gun.
//=========================================================
class CTimedTurret : public CBaseTurret	// name ours
{
public:
	void ActiveThink(void);
};


void CTimedTurret::ActiveThink(void)
{
	int fAttack = 0;
	Vector vecDirToEnemy;

	pev->nextthink = gpGlobals->time + 0.1;
	StudioFrameAdvance( );

	if ((!m_iOn) || (m_hEnemy == NULL))
	{
		m_hEnemy = NULL;
		m_flLastSight = gpGlobals->time + m_flMaxWait;
		SetThink(CallMonsterThink);
		return;
	}
	
	// if it's dead, look for something new
	if ( !m_hEnemy->IsAlive() )
	{
		if (!m_flLastSight)
		{
			m_flLastSight = gpGlobals->time + 0.5; // continue-shooting timeout
		}
		else
		{
			if (gpGlobals->time > m_flLastSight)
			{	
				m_hEnemy = NULL;
				m_flLastSight = gpGlobals->time + m_flMaxWait;
				SetThink(CallMonsterThink);
				return;
			}
		}
	}

	Vector vecMid = pev->origin + pev->view_ofs;
	Vector vecMidEnemy = m_hEnemy->BodyTarget( vecMid );

	// Look for our current enemy
	int fEnemyVisible = FBoxVisible(pev, m_hEnemy->pev, vecMidEnemy );	

	vecDirToEnemy = vecMidEnemy - vecMid;	// calculate dir and dist to enemy
	float flDistToEnemy = vecDirToEnemy.Length();

	Vector vec = UTIL_VecToAngles(vecMidEnemy - vecMid);	

	// Current enmey is not visible.
	if (!fEnemyVisible || (flDistToEnemy > m_flSightRange))
	{
		if (!m_flLastSight)
			m_flLastSight = gpGlobals->time + 0.5;
		else
		{
			// Should we look for a new target?
			if (gpGlobals->time > m_flLastSight)
			{
				m_hEnemy = NULL;
				m_flLastSight = gpGlobals->time + m_flMaxWait;
				SetThink(CallMonsterThink);
				return;
			}
		}
		fEnemyVisible = 0;
	}
	else
	{
		m_vecLastSight = vecMidEnemy;
	}

	UTIL_MakeAimVectors(m_vecCurAngles);	

	/*
	ALERT( at_console, "%.0f %.0f : %.2f %.2f %.2f\n", 
		m_vecCurAngles.x, m_vecCurAngles.y,
		gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_forward.z );
	*/
	
	Vector vecLOS = vecDirToEnemy; //vecMid - m_vecLastSight;
	vecLOS = vecLOS.Normalize();

	// Is the Gun looking at the target
	if (DotProduct(vecLOS, gpGlobals->v_forward) <= 0.866) // 30 degree slop
		fAttack = FALSE;
	else
		fAttack = TRUE;

	// fire the gun, at most once every m_flDelay seconds
	if (m_iSpin && ((fAttack) || (m_fBeserk)) && m_flNextAttack <= gpGlobals->time)
	{
		Vector vecSrc, vecAng;
		GetAttachment( 0, vecSrc, vecAng );
		SetTurretAnim(TURRET_ANIM_FIRE);
		Shoot(vecSrc, gpGlobals->v_forward );
		m_flNextAttack = gpGlobals->time + m_flDelay;
	} 
	else
	{
		SetTurretAnim(TURRET_ANIM_SPIN);
	}

	//move the gun
	if (m_fBeserk)
	{
		if (RANDOM_LONG(0,9) == 0)
		{
			m_vecGoalAngles.y = RANDOM_FLOAT(0,360);
			m_vecGoalAngles.x = RANDOM_FLOAT(0,90) - 90 * m_iOrientation;
			TakeDamage(pev,pev,1, DMG_GENERIC); // don't beserk forever
			return;
		}
	} 
	else if (fEnemyVisible)
	{
		if (vec.y > 360)
			vec.y -= 360;

		if (vec.y < 0)
			vec.y += 360;

		//ALERT(at_console, "[%.2f]", vec.x);
		
		if (vec.x < -180)
			vec.x += 360;

		if (vec.x > 180)
			vec.x -= 360;

		// now all numbers should be in [1...360]
		// pin to turret limitations to [-90...15]

		if (m_iOrientation == 0)
		{
			if (vec.x > 90)
				vec.x = 90;
			else if (vec.x < m_iMinPitch)
				vec.x = m_iMinPitch;
		}
		else
		{
			if (vec.x < -90)
				vec.x = -90;
			else if (vec.x > -m_iMinPitch)
				vec.x = -m_iMinPitch;
		}

		// ALERT(at_console, "->[%.2f]\n", vec.x);

		m_vecGoalAngles.y = vec.y;
		m_vecGoalAngles.x = vec.x;

	}

	SpinUpCall();
	MoveTurret();
}


void CTurret::Shoot(Vector &vecSrc, Vector &vecDirToEnemy)
{
	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 1000, 0.3 );
	FireBullets( 1, vecSrc, vecDirToEnemy, TURRET_SPREAD, TURRET_BULLET_RANGE, BULLET_MONSTER_12MM, 1 );
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "turret/tu_fire1.wav", 1, 0.6);
	pev->effects = pev->effects | EF_MUZZLEFLASH;
}


void CMiniTurret::Shoot(Vector &vecSrc, Vector &vecDirToEnemy)
{
	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 600, 0.3 );
	FireBullets( 1, vecSrc, vecDirToEnemy, TURRET_SPREAD, TURRET_BULLET_RANGE, BULLET_MONSTER_9MM, 1 );

	switch(RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/hks1.wav", 1, ATTN_NORM); break;
	case 1: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/hks2.wav", 1, ATTN_NORM); break;
	case 2: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/hks3.wav", 1, ATTN_NORM); break;
	}
	pev->effects = pev->effects | EF_MUZZLEFLASH;
}


void CBaseTurret::Deploy(void)
{
	pev->nextthink = gpGlobals->time + 0.1;
	StudioFrameAdvance( );

	if (pev->sequence != TURRET_ANIM_DEPLOY)
	{
		m_iOn = 1;
		SetTurretAnim(TURRET_ANIM_DEPLOY);
		EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_deploy.wav", TURRET_MACHINE_VOLUME, ATTN_NORM);
		SUB_UseTargets( this, USE_ON, 0 );
	}

	if (m_fSequenceFinished)
	{
		pev->maxs.z = m_iDeployHeight;
		pev->mins.z = -m_iDeployHeight;
		UTIL_SetSize(pev, pev->mins, pev->maxs);

		m_vecCurAngles.x = 0;

		if (m_iOrientation == 1)
		{
			m_vecCurAngles.y = UTIL_AngleMod( pev->angles.y + 180 );
		}
		else
		{
			m_vecCurAngles.y = UTIL_AngleMod( pev->angles.y );
		}

		SetTurretAnim(TURRET_ANIM_SPIN);
		pev->framerate = 0;
		SetThink(CallMonsterThink);
	}

	m_flLastSight = gpGlobals->time + m_flMaxWait;
}

void CBaseTurret::Retire(void)
{
	// make the turret level
	m_vecGoalAngles.x = 0;
	m_vecGoalAngles.y = m_flStartYaw;

	pev->nextthink = gpGlobals->time + 0.1;

	StudioFrameAdvance( );

	EyeOff( );

	if (!MoveTurret())
	{
		if (m_iSpin)
		{
			SpinDownCall();
		}
		else if (pev->sequence != TURRET_ANIM_RETIRE)
		{
			SetTurretAnim(TURRET_ANIM_RETIRE);
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "turret/tu_deploy.wav", TURRET_MACHINE_VOLUME, ATTN_NORM, 0, 120);
			SUB_UseTargets( this, USE_OFF, 0 );
		}
		else if (m_fSequenceFinished) 
		{	
			m_iOn = 0;
			m_flLastSight = 0;
			SetTurretAnim(TURRET_ANIM_NONE);
			pev->maxs.z = m_iRetractHeight;
			pev->mins.z = -m_iRetractHeight;
			UTIL_SetSize(pev, pev->mins, pev->maxs);
			if (m_iAutoStart)
			{
				SetThink(AutoSearchThink);		
				pev->nextthink = gpGlobals->time + .1;
			}
			else
				SetThink(SUB_DoNothing);
		}
	}
	else
	{
		SetTurretAnim(TURRET_ANIM_SPIN);
	}
}


void CTurret::SpinUpCall(void)
{
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	// Are we already spun up? If not start the two stage process.
	if (!m_iSpin)
	{
		SetTurretAnim( TURRET_ANIM_SPIN );
		// for the first pass, spin up the the barrel
		if (!m_iStartSpin)
		{
			pev->nextthink = gpGlobals->time + 1.0; // spinup delay
			EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_spinup.wav", TURRET_MACHINE_VOLUME, ATTN_NORM);
			m_iStartSpin = 1;
			pev->framerate = 0.1;
		}
		// after the barrel is spun up, turn on the hum
		else if (pev->framerate >= 1.0)
		{
			pev->nextthink = gpGlobals->time + 0.1; // retarget delay
			EMIT_SOUND(ENT(pev), CHAN_STATIC, "turret/tu_active2.wav", TURRET_MACHINE_VOLUME, ATTN_NORM);
			SetThink(CallActiveThink);
			m_iStartSpin = 0;
			m_iSpin = 1;
		} 
		else
		{
			pev->framerate += 0.075;
		}
	}

	if (m_iSpin)
	{
		SetThink(CallActiveThink);
	}
}


void CTurret::SpinDownCall(void)
{
	if (m_iSpin)
	{
		SetTurretAnim( TURRET_ANIM_SPIN );
		if (pev->framerate == 1.0)
		{
			EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "turret/tu_active2.wav", 0, 0, SND_STOP, 100);
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "turret/tu_spindown.wav", TURRET_MACHINE_VOLUME, ATTN_NORM);
		}
		pev->framerate -= 0.02;
		if (pev->framerate <= 0)
		{
			pev->framerate = 0;
			m_iSpin = 0;
		}
	}
}


void CBaseTurret::SetTurretAnim(TURRET_ANIM anim)
{
	if (pev->sequence != anim)
	{
		switch(anim)
		{
		case TURRET_ANIM_FIRE:
		case TURRET_ANIM_SPIN:
			if (pev->sequence != TURRET_ANIM_FIRE && pev->sequence != TURRET_ANIM_SPIN)
			{
				pev->frame = 0;
			}
			break;
		default:
			pev->frame = 0;
			break;
		}

		pev->sequence = anim;
		ResetSequenceInfo( );

		switch(anim)
		{
		case TURRET_ANIM_RETIRE:
			pev->frame			= 255;
			pev->framerate		= -1.0;
			break;
		case TURRET_ANIM_DIE:
			pev->framerate		= 1.0;
			break;
		}
		//ALERT(at_console, "Turret anim #%d\n", anim);
	}
}


//
// This search function will sit with the turret deployed and look for a new target. 
// After a set amount of time, the barrel will spin down. After m_flMaxWait, the turret will
// retact.
//
void CBaseTurret::MonsterThink(void)
{
	// ensure rethink
	SetTurretAnim(TURRET_ANIM_SPIN);
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	if (m_flSpinUpTime == 0 && m_flMaxSpin)
		m_flSpinUpTime = gpGlobals->time + m_flMaxSpin;

	Ping( );

	// If we have a target and we're still healthy
	if (m_hEnemy != NULL)
	{
		if (!m_hEnemy->IsAlive() )
			m_hEnemy = NULL;// Dead enemy forces a search for new one
	}


	// Acquire Target
	if (m_hEnemy == NULL)
	{
		Look(m_flSightRange);
		m_hEnemy = BestVisibleEnemy();
	}

	// If we've found a target, spin up the barrel and start to attack
	if (m_hEnemy != NULL)
	{
		m_flLastSight = 0;
		m_flSpinUpTime = 0;
		SetThink(CallActiveThink);
	}
	else
	{
		// Are we out of time, do we need to retract?
 		if (gpGlobals->time > m_flLastSight)
		{
			//Before we retrace, make sure that we are spun down.
			m_flLastSight = 0;
			m_flSpinUpTime = 0;
			SetThink(Retire);
		}
		// should we stop the spin?
		else if ((m_flSpinUpTime) && (gpGlobals->time > m_flSpinUpTime))
		{
			SpinDownCall();
		}
		
		// generic hunt for new victims
		m_vecGoalAngles.y = (m_vecGoalAngles.y + 0.1 * m_fTurnRate);
		if (m_vecGoalAngles.y >= 360)
			m_vecGoalAngles.y -= 360;
		MoveTurret();
	}
}


// 
// This think function will deploy the turret when something comes into range. This is for
// automatically activated turrets.
//
void CBaseTurret::AutoSearchThink(void)
{
	// ensure rethink
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.3;

	// If we have a target and we're still healthy

	if (m_hEnemy != NULL)
	{
		if (!m_hEnemy->IsAlive() )
			m_hEnemy = NULL;// Dead enemy forces a search for new one
	}

	// Acquire Target

	if (m_hEnemy == NULL)
	{
		Look( m_flSightRange );
		m_hEnemy = BestVisibleEnemy();
	}

	if (m_hEnemy != NULL)
	{
		SetThink(Deploy);
		EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_alert.wav", TURRET_MACHINE_VOLUME, ATTN_NORM);
	}
}


void CBaseTurret ::	TurretDeath( void )
{
	BOOL iActive = FALSE;

	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->deadflag != DEAD_DEAD)
	{
		pev->deadflag = DEAD_DEAD;

		float flRndSound = RANDOM_FLOAT ( 0 , 1 );

		if ( flRndSound <= 0.33 )
			EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_die.wav", 1.0, ATTN_NORM);
		else if ( flRndSound <= 0.66 )
			EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_die2.wav", 1.0, ATTN_NORM);
		else 
			EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_die3.wav", 1.0, ATTN_NORM);

		EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "turret/tu_active2.wav", 0, 0, SND_STOP, 100);

		if (m_iOrientation == 0)
			m_vecGoalAngles.x = -15;
		else
			m_vecGoalAngles.x = -90;

		SetTurretAnim(TURRET_ANIM_DIE); 

		EyeOn( );	
	}

	EyeOff( );

	if (pev->dmgtime + RANDOM_FLOAT( 0, 2 ) > gpGlobals->time)
	{
		// lots of smoke
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( RANDOM_FLOAT( pev->absmin.x, pev->absmax.x ) );
			WRITE_COORD( RANDOM_FLOAT( pev->absmin.y, pev->absmax.y ) );
			WRITE_COORD( pev->origin.z - m_iOrientation * 64 );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 25 ); // scale * 10
			WRITE_BYTE( 10 - m_iOrientation * 5); // framerate
		MESSAGE_END();
	}
	
	if (pev->dmgtime + RANDOM_FLOAT( 0, 5 ) > gpGlobals->time)
	{
		Vector vecSrc = Vector( RANDOM_FLOAT( pev->absmin.x, pev->absmax.x ), RANDOM_FLOAT( pev->absmin.y, pev->absmax.y ), 0 );
		if (m_iOrientation == 0)
			vecSrc = vecSrc + Vector( 0, 0, RANDOM_FLOAT( pev->origin.z, pev->absmax.z ) );
		else
			vecSrc = vecSrc + Vector( 0, 0, RANDOM_FLOAT( pev->absmin.z, pev->origin.z ) );

		UTIL_Sparks( vecSrc );
	}

	if (m_fSequenceFinished && !MoveTurret( ) && pev->dmgtime + 5 < gpGlobals->time)
	{
		pev->framerate = 0;
		SetThink( NULL );

		// tell the owner (a monstermaker) that we're dead, and fade out if we have one
		CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
		if ( pOwner )
		{
			pOwner->DeathNotice( pev );
			if ( !FNullEnt( pev->owner ) )
				FadeMonster();
		}
	}
}



void CBaseTurret :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( ptr->iHitgroup == 10 )
	{
		// hit armor
		if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,10) < 1) )
		{
			UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 1, 2) );
			pev->dmgtime = gpGlobals->time;
		}

		flDamage = 0.1;// don't hurt the monster much, but allow bits_COND_LIGHT_DAMAGE to be generated
	}

	if ( !pev->takedamage )
		return;

	AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
}

// take damage. bitsDamageType indicates type of damage sustained, ie: DMG_BULLET

int CBaseTurret::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	if ( !pev->takedamage )
		return 0;

	if (!m_iOn)
		flDamage /= 10.0;

	pev->health -= flDamage;
	if (pev->health <= 0)
	{
		pev->health = 0;
		pev->takedamage = DAMAGE_NO;
		pev->dmgtime = gpGlobals->time;

		ClearBits (pev->flags, FL_MONSTER); // why are they set in the first place???

		SetUse(NULL);
		SetThink(TurretDeath);
		SUB_UseTargets( this, USE_ON, 0 ); // wake up others
		pev->nextthink = gpGlobals->time + 0.1;

		return 0;
	}

	if (pev->health <= 10)
	{
		if (m_iOn && (1 || RANDOM_LONG(0, 0x7FFF) > 800))
		{
			m_fBeserk = 1;
			SetThink(CallMonsterThink);
		}
	}

	return 1;
}

int CBaseTurret::MoveTurret(void)
{
	int state = 0;
	// any x movement?
	
	if (m_vecCurAngles.x != m_vecGoalAngles.x)
	{
		float flDir = m_vecGoalAngles.x > m_vecCurAngles.x ? 1 : -1 ;

		m_vecCurAngles.x += 0.1 * m_fTurnRate * flDir;

		// if we started below the goal, and now we're past, peg to goal
		if (flDir == 1)
		{
			if (m_vecCurAngles.x > m_vecGoalAngles.x)
				m_vecCurAngles.x = m_vecGoalAngles.x;
		} 
		else
		{
			if (m_vecCurAngles.x < m_vecGoalAngles.x)
				m_vecCurAngles.x = m_vecGoalAngles.x;
		}

		if (m_iOrientation == 0)
			SetBoneController(1, -m_vecCurAngles.x);
		else
			SetBoneController(1, m_vecCurAngles.x);
		state = 1;
	}

	if (m_vecCurAngles.y != m_vecGoalAngles.y)
	{
		float flDir = m_vecGoalAngles.y > m_vecCurAngles.y ? 1 : -1 ;
		float flDist = fabs(m_vecGoalAngles.y - m_vecCurAngles.y);
		
		if (flDist > 180)
		{
			flDist = 360 - flDist;
			flDir = -flDir;
		}
		if (flDist > 30)
		{
			if (m_fTurnRate < m_iBaseTurnRate * 10)
			{
				m_fTurnRate += m_iBaseTurnRate;
			}
		}
		else if (m_fTurnRate > 45)
		{
			m_fTurnRate -= m_iBaseTurnRate;
		}
		else
		{
			m_fTurnRate += m_iBaseTurnRate;
		}

		m_vecCurAngles.y += 0.1 * m_fTurnRate * flDir;

		if (m_vecCurAngles.y < 0)
			m_vecCurAngles.y += 360;
		else if (m_vecCurAngles.y >= 360)
			m_vecCurAngles.y -= 360;

		if (flDist < (0.05 * m_iBaseTurnRate))
			m_vecCurAngles.y = m_vecGoalAngles.y;

		//ALERT(at_console, "%.2f -> %.2f\n", m_vecCurAngles.y, y);
		if (m_iOrientation == 0)
			SetBoneController(0, m_vecCurAngles.y - pev->angles.y );
		else 
			SetBoneController(0, pev->angles.y - 180 - m_vecCurAngles.y );
		state = 1;
	}

	if (!state)
		m_fTurnRate = m_iBaseTurnRate;

	//ALERT(at_console, "(%.2f, %.2f)->(%.2f, %.2f)\n", m_vecCurAngles.x, 
	//	m_vecCurAngles.y, m_vecGoalAngles.x, m_vecGoalAngles.y);
	return state;
}

//
// ID as a machine
//
int	CBaseTurret::Classify ( void )
{
	if (m_iOn || m_iAutoStart)
	{
		if ( pev->spawnflags & SF_MONSTER_TURRET_ALLY )
			return CLASS_PLAYER_ALLY;
		return	CLASS_MACHINE;
	}
	return CLASS_NONE;
}




//=========================================================
// Sentry gun - smallest turret, placed near grunt entrenchments
//=========================================================
class CSentry : public CBaseTurret
{
public:
	void Spawn( );
	void Precache(void);
	// other functions
	void Shoot(Vector &vecSrc, Vector &vecDirToEnemy);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	void EXPORT SentryTouch( CBaseEntity *pOther );
	void EXPORT SentryDeath( void );

	const char *GetKillDesc( void ) { return "a tripod-mounted machine gun"; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return "shot"; }
	const char *GetHitGroupDesc( int iHitGroup ) { return HitGroupIn( iHitGroup ); }
	int GetBattlePower( void );
};

LINK_ENTITY_TO_CLASS( monster_sentry, CSentry );

void CSentry::Precache()
{
	CBaseTurret::Precache( );
	PRECACHE_MODEL ("models/sentry.mdl");	
}

void CSentry::Spawn()
{ 
	Precache( );
	SET_MODEL(ENT(pev), "models/sentry.mdl");
	pev->health			= gSkillData.sentryHealth;
	m_HackedGunPos		= Vector( 0, 0, 48 );
	pev->view_ofs.z		= 48;
	m_flMaxWait = 1E6;
	m_flMaxSpin	= 1E6;

	CBaseTurret::Spawn();
	m_iRetractHeight = 64;
	m_iDeployHeight = 64;
	m_iMinPitch	= -60;
	UTIL_SetSize(pev, Vector(-16, -16, -m_iRetractHeight), Vector(16, 16, m_iRetractHeight));

	SetTouch(SentryTouch);
	SetThink(Initialize);	
	pev->nextthink = gpGlobals->time + 0.3; 
}

void CSentry::Shoot(Vector &vecSrc, Vector &vecDirToEnemy)
{
	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 600, 0.3 );
	FireBullets( 1, vecSrc, vecDirToEnemy, TURRET_SPREAD, TURRET_BULLET_RANGE, BULLET_MONSTER_MP5, 1 );
	
	switch(RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/hks1.wav", 1, ATTN_NORM); break;
	case 1: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/hks2.wav", 1, ATTN_NORM); break;
	case 2: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/hks3.wav", 1, ATTN_NORM); break;
	}
	pev->effects = pev->effects | EF_MUZZLEFLASH;
}

int CSentry::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	if ( !pev->takedamage )
		return 0;

	if (!m_iOn)
	{
		SetThink( Deploy );
		SetUse( NULL );
		pev->nextthink = gpGlobals->time + 0.1;
	}

	pev->health -= flDamage;
	if (pev->health <= 0)
	{
		pev->health = 0;
		pev->takedamage = DAMAGE_NO;
		pev->dmgtime = gpGlobals->time;

		ClearBits (pev->flags, FL_MONSTER); // why are they set in the first place???

		SetUse(NULL);
		SetThink(SentryDeath);
		SUB_UseTargets( this, USE_ON, 0 ); // wake up others
		pev->nextthink = gpGlobals->time + 0.1;

		return 0;
	}

	return 1;
}

int CSentry::GetBattlePower( void )
{
	return ( pev->max_health * 0.1 ) * ( gSkillData.monDmg12MM * 12.5 );
}


void CSentry::SentryTouch( CBaseEntity *pOther )
{
	if ( pOther && (pOther->IsPlayer() || (pOther->pev->flags & FL_MONSTER)) )
	{
		TakeDamage(pOther->pev, pOther->pev, 0, 0 );
	}
}


void CSentry ::	SentryDeath( void )
{
	BOOL iActive = FALSE;

	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->deadflag != DEAD_DEAD)
	{
		pev->deadflag = DEAD_DEAD;

		float flRndSound = RANDOM_FLOAT ( 0 , 1 );

		if ( flRndSound <= 0.33 )
			EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_die.wav", 1.0, ATTN_NORM);
		else if ( flRndSound <= 0.66 )
			EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_die2.wav", 1.0, ATTN_NORM);
		else 
			EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_die3.wav", 1.0, ATTN_NORM);

		EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "turret/tu_active2.wav", 0, 0, SND_STOP, 100);

		SetBoneController( 0, 0 );
		SetBoneController( 1, 0 );

		SetTurretAnim(TURRET_ANIM_DIE); 

		pev->solid = SOLID_NOT;
		pev->angles.y = UTIL_AngleMod( pev->angles.y + RANDOM_LONG( 0, 2 ) * 120 );

		EyeOn( );
	}

	EyeOff( );

	Vector vecSrc, vecAng;
	GetAttachment( 1, vecSrc, vecAng );

	if (pev->dmgtime + RANDOM_FLOAT( 0, 2 ) > gpGlobals->time)
	{
		// lots of smoke
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( vecSrc.x + RANDOM_FLOAT( -16, 16 ) );
			WRITE_COORD( vecSrc.y + RANDOM_FLOAT( -16, 16 ) );
			WRITE_COORD( vecSrc.z - 32 );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 15 ); // scale * 10
			WRITE_BYTE( 8 ); // framerate
		MESSAGE_END();
	}
	
	if (pev->dmgtime + RANDOM_FLOAT( 0, 8 ) > gpGlobals->time)
	{
		UTIL_Sparks( vecSrc );
	}

	if (m_fSequenceFinished && pev->dmgtime + 5 < gpGlobals->time)
	{
		pev->framerate = 0;
		SetThink( NULL );

		// tell the owner (a monstermaker) that we're dead, and fade out if we have one
		CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
		if ( pOwner )
		{
			pOwner->DeathNotice( pev );
			if ( !FNullEnt( pev->owner ) )
				FadeMonster();
		}
	}
}



#include "explode.h"

//=========================================================
// Flak shells: fly to where the turret aimed, then burst
//=========================================================
class CFlakBullet : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify( void );

	const char *GetKillDesc( void ) { return "a flak cannon"; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return "ripped"; }
	const char *GetHitGroupDesc( int iHitGroup ) { return " apart"; }

	static CFlakBullet *FlakCreate( edict_t *pOwner );	// name ours

	void EXPORT ExplodeTouch( CBaseEntity *pOther );
	void EXPORT ExplodeThink( void );
	virtual void Explode( void );	// name ours
};

LINK_ENTITY_TO_CLASS( standard_flak, CFlakBullet );


//=========================================================
// Flak turret - a turret that shoots flak shells
//=========================================================
class CFlakTurret : public CTurret
{
public:
	void Spawn(void);
	void Precache(void);
	void ActiveThink(void);

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	void Shoot(Vector &vecSrc, Vector &vecDirToEnemy);

	const char *GetKillDesc( void ) { return "a turret"; }
	const char *GetHudName( void ) { return "Flak Cannon"; }
	int GetBattlePower( void );

	float m_flNextShot;
};

LINK_ENTITY_TO_CLASS( monster_flakturret, CFlakTurret );

TYPEDESCRIPTION	CFlakTurret::m_SaveData[] =
{
	DEFINE_FIELD( CFlakTurret, m_flNextShot, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CFlakTurret, CTurret );

void CFlakTurret::Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/turret.mdl");
	pev->health			= 100;
	m_HackedGunPos		= Vector( 0, 0, 12.75 );
	m_flMaxSpin =		TURRET_MAXSPIN;
	pev->view_ofs.z = 12.75;

	CBaseTurret::Spawn( );

	m_flNextShot = gpGlobals->time;
	m_iRetractHeight = 16;
	m_iDeployHeight = 32;
	m_iMinPitch	= -15;
	UTIL_SetSize(pev, Vector(-32, -32, -m_iRetractHeight), Vector(32, 32, m_iRetractHeight));

	SetThink(Initialize);

	m_pEyeGlow = CSprite::SpriteCreate( TURRET_GLOW_SPRITE, pev->origin, FALSE );
	m_pEyeGlow->SetTransparency( kRenderGlow, 255, 0, 0, 0, kRenderFxNoDissipation );
	m_pEyeGlow->SetAttachment( edict(), 2 );
	m_eyeBrightness = 0;

	pev->nextthink = gpGlobals->time + 0.3;
}

void CFlakTurret::Precache()
{
	CBaseTurret::Precache( );
	PRECACHE_MODEL ("models/turret.mdl");
	PRECACHE_MODEL (TURRET_GLOW_SPRITE);
	UTIL_PrecacheOther( "standard_flak" );
}

int CFlakTurret::GetBattlePower( void )
{
	return ( pev->max_health * 0.1 ) * 250;
}

void CFlakTurret::Shoot(Vector &vecSrc, Vector &vecDirToEnemy)
{
	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 600, 0.3 );

	// the shell bursts somewhere within 96 units of the aim point
	UTIL_MakeVectors( Vector( RANDOM_FLOAT( -180, 180 ), RANDOM_FLOAT( -180, 180 ), RANDOM_FLOAT( -180, 180 ) ) );
	Vector vecEnd = gpGlobals->v_forward * RANDOM_FLOAT( 0, 96 ) + vecDirToEnemy;

	// warn whoever is near the aim point, at most every 10 seconds
	if ( pev->pain_finished < gpGlobals->time )
	{
		CSoundEnt::InsertSound( bits_SOUND_DANGER, vecDirToEnemy, 192, 3.0 );
		pev->pain_finished = gpGlobals->time + 10;
	}

	CFlakBullet *pFlak = CFlakBullet::FlakCreate( ENT(pev) );
	pFlak->pev->origin = vecSrc;
	pFlak->pev->velocity = vecEnd - vecSrc;
	pFlak->pev->nextthink = gpGlobals->time + pFlak->pev->velocity.Length() / 2000;
	pFlak->pev->velocity = pFlak->pev->velocity.Normalize() * 2000;

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "turret/tu_fire1.wav", 1, 0.6);
	pev->effects = pev->effects | EF_MUZZLEFLASH;
}


void CFlakTurret::ActiveThink(void)
{
	int fAttack = 0;
	Vector vecDirToEnemy;

	pev->nextthink = gpGlobals->time + 0.1;
	StudioFrameAdvance( );

	if ((!m_iOn) || (m_hEnemy == NULL))
	{
		m_hEnemy = NULL;
		m_flLastSight = gpGlobals->time + m_flMaxWait;
		SetThink(CallMonsterThink);
		return;
	}

	// if it's dead, look for something new
	if ( !m_hEnemy->IsAlive() )
	{
		if (!m_flLastSight)
		{
			m_flLastSight = gpGlobals->time + 0.5; // continue-shooting timeout
		}
		else
		{
			if (gpGlobals->time > m_flLastSight)
			{
				m_hEnemy = NULL;
				m_flLastSight = gpGlobals->time + m_flMaxWait;
				SetThink(CallMonsterThink);
				return;
			}
		}
	}

	// aim from the barrel
	Vector vecSrc, vecAng;
	GetAttachment( 0, vecSrc, vecAng );
	Vector vecMidEnemy = m_hEnemy->BodyTarget( vecSrc );

	// Look for our current enemy
	int fEnemyVisible = FBoxVisible(pev, m_hEnemy->pev, vecMidEnemy );

	vecDirToEnemy = vecMidEnemy - vecSrc;	// calculate dir and dist to enemy
	float flDistToEnemy = vecDirToEnemy.Length();

	Vector vec = UTIL_VecToAngles(vecMidEnemy - vecSrc);

	// Current enmey is not visible.
	if (!fEnemyVisible || (flDistToEnemy > m_flSightRange))
	{
		if (!m_flLastSight)
			m_flLastSight = gpGlobals->time + 0.5;
		else
		{
			// Should we look for a new target?
			if (gpGlobals->time > m_flLastSight)
			{
				m_hEnemy = NULL;
				m_flLastSight = gpGlobals->time + m_flMaxWait;
				SetThink(CallMonsterThink);
				return;
			}
		}
		fEnemyVisible = 0;
	}
	else
	{
		m_vecLastSight = vecMidEnemy;
	}

	UTIL_MakeAimVectors(m_vecCurAngles);

	Vector vecLOS = vecDirToEnemy; //vecMid - m_vecLastSight;
	vecLOS = vecLOS.Normalize();

	// Is the Gun looking at the target
	if (DotProduct(vecLOS, gpGlobals->v_forward) <= 0.866) // 30 degree slop
		fAttack = FALSE;
	else
		fAttack = TRUE;

	// fire the gun, at most five times a second; the shell bursts at the enemy's distance
	if (m_iSpin && ((fAttack) || (m_fBeserk)) && m_flNextShot <= gpGlobals->time)
	{
		SetTurretAnim(TURRET_ANIM_FIRE);
		float flDist;
		if (m_fBeserk)
			flDist = RANDOM_FLOAT( 40, 1200 );
		else
			flDist = flDistToEnemy;
		Vector vecEnd = vecSrc + gpGlobals->v_forward * flDist;
		Shoot(vecSrc, vecEnd );
		m_flNextShot = gpGlobals->time + 0.2;
	}
	else
	{
		SetTurretAnim(TURRET_ANIM_SPIN);
	}

	//move the gun
	if (m_fBeserk)
	{
		if (RANDOM_LONG(0,9) == 0)
		{
			m_vecGoalAngles.y = RANDOM_FLOAT(0,360);
			m_vecGoalAngles.x = RANDOM_FLOAT(0,90) - 90 * m_iOrientation;
			TakeDamage(pev,pev,1, DMG_GENERIC); // don't beserk forever
			return;
		}
	}
	else if (fEnemyVisible)
	{
		if (vec.y > 360)
			vec.y -= 360;

		if (vec.y < 0)
			vec.y += 360;

		if (vec.x < -180)
			vec.x += 360;

		if (vec.x > 180)
			vec.x -= 360;

		// now all numbers should be in [1...360]
		// pin to turret limitations to [-90...15]

		if (m_iOrientation == 0)
		{
			if (vec.x > 90)
				vec.x = 90;
			else if (vec.x < m_iMinPitch)
				vec.x = m_iMinPitch;
		}
		else
		{
			if (vec.x < -90)
				vec.x = -90;
			else if (vec.x > -m_iMinPitch)
				vec.x = -m_iMinPitch;
		}

		m_vecGoalAngles.y = vec.y;
		m_vecGoalAngles.x = vec.x;

	}

	SpinUpCall();
	MoveTurret();
}


CFlakBullet *CFlakBullet::FlakCreate( edict_t *pOwner )
{
	CFlakBullet *pFlak = GetClassPtr( (CFlakBullet *)NULL );
	pFlak->pev->owner = pOwner;
	pFlak->pev->classname = MAKE_STRING( "standard_flak" );
	pFlak->Spawn();

	return pFlak;
}

void CFlakBullet::Spawn( void )
{
	Precache( );
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->renderamt = 255;
	pev->rendermode = kRenderTransAdd;
	SET_MODEL( ENT(pev), "sprites/yelflare2.spr" );
	pev->scale = 0.5;

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	SetTouch( ExplodeTouch );
	SetThink( ExplodeThink );
	pev->nextthink = 0;	// the turret sets the burst time
}

void CFlakBullet::Precache( void )
{
	PRECACHE_MODEL( "sprites/yelflare2.spr" );
}

int CFlakBullet::Classify( void )
{
	return CLASS_NONE;
}

void CFlakBullet::ExplodeTouch( CBaseEntity *pOther )
{
	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 1024, 3.0 );
	Explode( );
}

void CFlakBullet::ExplodeThink( void )
{
	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 1024, 3.0 );
	Explode( );
}

void CFlakBullet::Explode( void )
{
	int iContents = UTIL_PointContents( pev->origin );

	pev->dmg = 25;

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		if (iContents != CONTENTS_WATER)
		{
			WRITE_SHORT( g_sModelIndexFireball );
		}
		else
		{
			WRITE_SHORT( g_sModelIndexWExplosion );
		}
		WRITE_BYTE( 10 ); // scale * 10
		WRITE_BYTE( 15 ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	RadiusDamage( pev->origin, pev, VARS( pev->owner ), pev->dmg, 64, CLASS_NONE, DMG_BLAST );

	UTIL_Remove( this );
}


//=========================================================
// The TFC sentry guns' tripod (a plain model)
//=========================================================
class CTFCBase : public CBaseEntity	// name ours
{
public:
	void Spawn( void );
	void Precache( void );
};

LINK_ENTITY_TO_CLASS( tfc_base, CTFCBase );

void CTFCBase::Precache( void )
{
	PRECACHE_MODEL( "models/tfc_base.mdl" );
}

void CTFCBase::Spawn( void )
{
	Precache( );
	SET_MODEL( ENT(pev), "models/tfc_base.mdl" );
	pev->takedamage = DAMAGE_NO;
	UTIL_SetSize( pev, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ) );
}


//=========================================================
// TFC sentry gun, level 1: a 9mm gun on a tripod
//=========================================================
class CTFCSentry1 : public CTimedTurret
{
public:
	void Spawn( );
	void Precache(void);
	// other functions
	void Shoot(Vector &vecSrc, Vector &vecDirToEnemy);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	int MoveTurret(void);
	void EXPORT SentryDeath( void );
	void EXPORT SearchThink( void );
	void EXPORT SentryUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	const char *GetKillDesc( void ) { return "a sentry gun"; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return "shot"; }
	const char *GetHitGroupDesc( int iHitGroup ) { return HitGroupIn( iHitGroup ); }
	int GetBattlePower( void );

	EHANDLE m_base;	// the tripod
};

LINK_ENTITY_TO_CLASS( monster_tfc_sentry1, CTFCSentry1 );

TYPEDESCRIPTION	CTFCSentry1::m_SaveData[] =
{
	DEFINE_FIELD( CTFCSentry1, m_base, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CTFCSentry1, CTimedTurret );

void CTFCSentry1::Precache()
{
	CBaseTurret::Precache( );
	PRECACHE_MODEL ("models/sentry1.mdl");
	UTIL_PrecacheOther( "tfc_base" );
}

void CTFCSentry1::Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/sentry1.mdl");
	if ( m_base == NULL )
		m_base = CBaseEntity::Create( "tfc_base", Vector( pev->origin.x, pev->origin.y, pev->origin.z - 20 ), pev->angles, edict() );
	pev->health			= 50;
	m_HackedGunPos		= Vector( 0, 0, 30 );
	pev->view_ofs.z		= 0;
	m_flMaxWait = 1E6;
	m_flMaxSpin	= 1E6;

	CBaseTurret::Spawn();
	m_iRetractHeight = 16;
	m_iDeployHeight = 16;
	m_iMinPitch	= -30;
	UTIL_SetSize(pev, Vector(-16, -16, -m_iRetractHeight), Vector(16, 16, m_iRetractHeight));

	Initialize();
	m_iOrientation = 1;	// sic: the model's controllers run the other way
	m_flStartYaw = pev->angles.y - 90;
	if ( !HasSpawnFlag( SF_MONSTER_TURRET_STARTINACTIVE ) )
	{
		SetThink(SearchThink);
		m_iOn = 1;
		pev->nextthink = gpGlobals->time + 0.3;
	}
	m_flFieldOfView = 0.7;
	m_flDelay = 0.2;
	if ( pev->targetname )
		SetUse( SentryUse );
}

int CTFCSentry1::GetBattlePower( void )
{
	return ( pev->max_health * 0.1 ) * ( gSkillData.monDmg9MM * 6.25 );
}

void CTFCSentry1::Shoot(Vector &vecSrc, Vector &vecDirToEnemy)
{
	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 600, 0.3 );
	FireBullets( 1, vecSrc, vecDirToEnemy, TURRET_SPREAD, TURRET_BULLET_RANGE, BULLET_MONSTER_9MM, 1 );
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/pl_gun3.wav", 1, 0.8);
	pev->effects = pev->effects | EF_MUZZLEFLASH;
}

int CTFCSentry1::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	if ( !pev->takedamage )
		return 0;

	if (!m_iOn)
	{
		SetThink( SearchThink );
		SetUse( NULL );
		pev->nextthink = gpGlobals->time + 0.1;
	}

	pev->health -= flDamage;

	// glow: blue for an ally, red for an enemy (never switched off)
	pev->renderfx = kRenderFxGlowShell;
	if ( pev->spawnflags & SF_MONSTER_TURRET_ALLY )
		pev->rendercolor = Vector( 0, 0, 255 );
	else
		pev->rendercolor = Vector( 255, 0, 0 );

	if (pev->health <= 0)
	{
		pev->health = 0;
		pev->takedamage = DAMAGE_NO;
		pev->dmgtime = gpGlobals->time;

		ClearBits (pev->flags, FL_MONSTER); // why are they set in the first place???

		SetUse(NULL);
		SetThink(SentryDeath);
		SUB_UseTargets( this, USE_ON, 0 ); // wake up others
		pev->nextthink = gpGlobals->time + 0.1;

		return 0;
	}

	return 1;
}

void CTFCSentry1::SentryDeath( void )
{
	pev->deadflag = DEAD_DEAD;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;

	// tell the owner (a monstermaker) that we're dead
	CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
	if ( pOwner )
		pOwner->DeathNotice( pev );

	ExplosionCreate( pev->origin, pev->angles, edict(), 65, TRUE );

	if ( m_base != NULL )
		UTIL_Remove( m_base );
	m_base = NULL;

	UTIL_Remove( this );
}

//
// Like CBaseTurret::MonsterThink, but a sentry never retracts
//
void CTFCSentry1::SearchThink(void)
{
	// ensure rethink
	SetTurretAnim(TURRET_ANIM_SPIN);
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	if (m_flSpinUpTime == 0 && m_flMaxSpin)
		m_flSpinUpTime = gpGlobals->time + m_flMaxSpin;

	Ping( );

	// If we have a target and we're still healthy
	if (m_hEnemy != NULL)
	{
		if (!m_hEnemy->IsAlive() )
			m_hEnemy = NULL;// Dead enemy forces a search for new one
	}


	// Acquire Target
	if (m_hEnemy == NULL)
	{
		Look(m_flSightRange);
		m_hEnemy = BestVisibleEnemy();
	}

	// If we've found a target, spin up the barrel and start to attack
	if (m_hEnemy != NULL)
	{
		m_flLastSight = 0;
		m_flSpinUpTime = 0;
		SetThink(CallActiveThink);
	}
	else
	{
		// should we stop the spin?
		if ((m_flSpinUpTime) && (gpGlobals->time > m_flSpinUpTime))
		{
			SpinDownCall();
		}

		// generic hunt for new victims
		m_vecGoalAngles.y = (m_vecGoalAngles.y + 0.1 * m_fTurnRate);
		if (m_vecGoalAngles.y >= 360)
			m_vecGoalAngles.y -= 360;
		MoveTurret();
	}
}

void CTFCSentry1::SentryUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_iOn ) )
		return;

	if (m_iOn)
	{
		m_hEnemy = NULL;
		m_iOn = 0;
		pev->nextthink = 0;
	}
	else
	{
		SetThink(SearchThink);
		m_iOn = 1;
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

int CTFCSentry1::MoveTurret(void)
{
	int state = 0;
	// any x movement?

	if (m_vecCurAngles.x != m_vecGoalAngles.x)
	{
		float flDir = m_vecGoalAngles.x > m_vecCurAngles.x ? 1 : -1 ;

		m_vecCurAngles.x += 0.1 * m_fTurnRate * flDir;

		// if we started below the goal, and now we're past, peg to goal
		if (flDir == 1)
		{
			if (m_vecCurAngles.x > m_vecGoalAngles.x)
				m_vecCurAngles.x = m_vecGoalAngles.x;
		}
		else
		{
			if (m_vecCurAngles.x < m_vecGoalAngles.x)
				m_vecCurAngles.x = m_vecGoalAngles.x;
		}

		SetBoneController(1, m_vecCurAngles.x);
		state = 1;
	}

	if (m_vecCurAngles.y != m_vecGoalAngles.y)
	{
		float flDir = m_vecGoalAngles.y > m_vecCurAngles.y ? 1 : -1 ;
		float flDist = fabs(m_vecGoalAngles.y - m_vecCurAngles.y);

		if (flDist > 180)
		{
			flDist = 360 - flDist;
			flDir = -flDir;
		}
		if (flDist > 30)
		{
			if (m_fTurnRate < m_iBaseTurnRate * 10)
			{
				m_fTurnRate += m_iBaseTurnRate;
			}
		}
		else if (m_fTurnRate > 45)
		{
			m_fTurnRate -= m_iBaseTurnRate;
		}
		else
		{
			m_fTurnRate += m_iBaseTurnRate;
		}

		m_vecCurAngles.y += 0.1 * m_fTurnRate * flDir;

		if (m_vecCurAngles.y < 0)
			m_vecCurAngles.y += 360;
		else if (m_vecCurAngles.y >= 360)
			m_vecCurAngles.y -= 360;

		if (flDist < (0.05 * m_iBaseTurnRate))
			m_vecCurAngles.y = m_vecGoalAngles.y;

		SetBoneController(0, m_vecCurAngles.y - pev->angles.y );
		state = 1;
	}

	if (!state)
		m_fTurnRate = m_iBaseTurnRate;

	return state;
}


//=========================================================
// TFC sentry gun, level 2: a 12mm gun
//=========================================================
class CTFCSentry2 : public CTFCSentry1	// name ours
{
public:
	void Spawn( );
	void Precache(void);
	void Shoot(Vector &vecSrc, Vector &vecDirToEnemy);
	int GetBattlePower( void );
};

LINK_ENTITY_TO_CLASS( monster_tfc_sentry2, CTFCSentry2 );

void CTFCSentry2::Precache()
{
	CBaseTurret::Precache( );
	PRECACHE_MODEL ("models/sentry2.mdl");
	UTIL_PrecacheOther( "tfc_base" );
}

void CTFCSentry2::Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/sentry2.mdl");
	if ( m_base == NULL )
		m_base = CBaseEntity::Create( "tfc_base", Vector( pev->origin.x, pev->origin.y, pev->origin.z - 20 ), pev->angles, edict() );
	pev->health			= 75;
	Vector vecAng;
	GetAttachment( 0, m_HackedGunPos, vecAng );
	pev->view_ofs.z		= 0;
	m_flMaxWait = 1E6;
	m_flMaxSpin	= 1E6;

	CBaseTurret::Spawn();
	m_iRetractHeight = 16;
	m_iDeployHeight = 16;
	m_iMinPitch	= -30;
	UTIL_SetSize(pev, Vector(-16, -16, -m_iRetractHeight), Vector(16, 16, m_iRetractHeight));

	Initialize();
	if ( !HasSpawnFlag( SF_MONSTER_TURRET_STARTINACTIVE ) )
	{
		SetThink(SearchThink);
		m_iOn = 1;
		pev->nextthink = gpGlobals->time + 0.3;
	}
	m_flFieldOfView = 0.7;
	m_flDelay = 0;
	if ( pev->targetname )
		SetUse( SentryUse );
}

int CTFCSentry2::GetBattlePower( void )
{
	return ( pev->max_health * 0.1 ) * ( gSkillData.monDmg12MM * 12.5 );
}

void CTFCSentry2::Shoot(Vector &vecSrc, Vector &vecDirToEnemy)
{
	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 1000, 0.3 );
	FireBullets( 1, vecSrc, vecDirToEnemy, TURRET_SPREAD, TURRET_BULLET_RANGE, BULLET_MONSTER_12MM, 1 );
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "turret/tu_fire1.wav", 1, 0.8);
	pev->effects = pev->effects | EF_MUZZLEFLASH;
}


//=========================================================
// TFC sentry gun, level 3: two 12mm guns and a rocket
// launcher ("weapons" 1: heat-seeking, -1: random)
//=========================================================
class CTFCSentry3 : public CTFCSentry1	// name ours
{
public:
	void Spawn( );
	void Precache(void);
	void Shoot(Vector &vecSrc, Vector &vecDirToEnemy);
	int GetBattlePower( void );
	void FireRocket( void );	// name ours
	void ActiveThink( void );
	int MoveTurret(void);
};

LINK_ENTITY_TO_CLASS( monster_tfc_sentry3, CTFCSentry3 );

void CTFCSentry3::Precache()
{
	CBaseTurret::Precache( );
	PRECACHE_MODEL ("models/sentry3.mdl");
	UTIL_PrecacheOther( "tfc_base" );
	UTIL_PrecacheOther( "rocket" );
}

void CTFCSentry3::Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/sentry3.mdl");
	if ( m_base == NULL )
		m_base = CBaseEntity::Create( "tfc_base", Vector( pev->origin.x, pev->origin.y, pev->origin.z - 20 ), pev->angles, edict() );
	pev->health			= 100;
	Vector vecAng, vecGun2;
	GetAttachment( 0, m_HackedGunPos, vecAng );
	GetAttachment( 1, vecGun2, vecAng );
	m_HackedGunPos = m_HackedGunPos + vecGun2 * 0.5;	// sic: not the midpoint
	pev->view_ofs.z		= 0;
	m_flMaxWait = 1E6;
	m_flMaxSpin	= 1E6;

	if ( pev->weapons < 0 )
		pev->weapons = RANDOM_LONG( 0, 1 );

	CBaseTurret::Spawn();
	m_iRetractHeight = 16;
	m_iDeployHeight = 16;
	m_iMinPitch	= -30;
	UTIL_SetSize(pev, Vector(-16, -16, -m_iRetractHeight), Vector(16, 16, m_iRetractHeight));

	Initialize();
	if ( !HasSpawnFlag( SF_MONSTER_TURRET_STARTINACTIVE ) )
	{
		SetThink(SearchThink);
		m_iOn = 1;
		pev->nextthink = gpGlobals->time + 0.3;
	}
	m_flFieldOfView = 0.7;
	m_flDelay = 2;
	m_flNextAttack = gpGlobals->time + 2;
	if ( pev->targetname )
		SetUse( SentryUse );
}

int CTFCSentry3::GetBattlePower( void )
{
	return ( pev->max_health * 0.1 ) * ( gSkillData.monDmg12MM * 25 + 40 );
}

void CTFCSentry3::Shoot(Vector &vecSrc, Vector &vecDirToEnemy)
{
	Vector vecAng;

	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 1000, 0.3 );
	GetAttachment( 0, vecSrc, vecAng );
	FireBullets( 1, vecSrc, vecDirToEnemy, TURRET_SPREAD, TURRET_BULLET_RANGE, BULLET_MONSTER_12MM, 1 );
	GetAttachment( 1, vecSrc, vecAng );
	FireBullets( 1, vecSrc, vecDirToEnemy, TURRET_SPREAD, TURRET_BULLET_RANGE, BULLET_MONSTER_12MM, 1 );
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "turret/tu_fire1.wav", 1, 0.8);
	pev->effects = pev->effects | EF_MUZZLEFLASH;
}

void CTFCSentry3::FireRocket( void )
{
	// the launchers take turns
	pev->iuser1 = !pev->iuser1;

	Vector vecSrc, vecAng;
	GetAttachment( pev->iuser1, vecSrc, vecAng );

	Vector vecColor( 224, 224, 255 );
	if ( pev->weapons )
		vecColor = Vector( 255, 224, 64 );

	CRocket *pRocket = CRocket::CreateRocket( vecSrc, m_vecCurAngles, edict(), vecColor, 0 );
	pRocket->pev->speed = 1000;
	pRocket->pev->dmg = 80;
	if ( pev->weapons )
	{
		pRocket->SetThink( CRocket::SeekThink );
		pRocket->pev->speed = 600;
		pRocket->m_iszKillDesc = MAKE_STRING( "a heat-seeking missile" );
	}
}

void CTFCSentry3::ActiveThink(void)
{
	int fAttack = 0;
	int fRocket = 0;
	Vector vecDirToEnemy;

	pev->nextthink = gpGlobals->time + 0.1;
	StudioFrameAdvance( );

	if ((!m_iOn) || (m_hEnemy == NULL))
	{
		m_hEnemy = NULL;
		m_flLastSight = gpGlobals->time + m_flMaxWait;
		SetThink(SearchThink);
		return;
	}

	// if it's dead, look for something new
	if ( !m_hEnemy->IsAlive() )
	{
		if (!m_flLastSight)
		{
			m_flLastSight = gpGlobals->time + 0.5; // continue-shooting timeout
		}
		else
		{
			if (gpGlobals->time > m_flLastSight)
			{
				m_hEnemy = NULL;
				m_flLastSight = gpGlobals->time + m_flMaxWait;
				SetThink(SearchThink);
				return;
			}
		}
	}

	Vector vecMid = pev->origin + pev->view_ofs;
	Vector vecMidEnemy = m_hEnemy->BodyTarget( vecMid );

	// Look for our current enemy
	int fEnemyVisible = FBoxVisible(pev, m_hEnemy->pev, vecMidEnemy );

	vecDirToEnemy = vecMidEnemy - vecMid;	// calculate dir and dist to enemy
	float flDistToEnemy = vecDirToEnemy.Length();

	Vector vec = UTIL_VecToAngles(vecMidEnemy - vecMid);

	// Current enmey is not visible.
	if (!fEnemyVisible || (flDistToEnemy > m_flSightRange))
	{
		if (!m_flLastSight)
			m_flLastSight = gpGlobals->time + 0.5;
		else
		{
			// Should we look for a new target?
			if (gpGlobals->time > m_flLastSight)
			{
				m_hEnemy = NULL;
				m_flLastSight = gpGlobals->time + m_flMaxWait;
				SetThink(SearchThink);
				return;
			}
		}
		fEnemyVisible = 0;
	}
	else
	{
		m_vecLastSight = vecMidEnemy;
	}

	UTIL_MakeAimVectors(m_vecCurAngles);

	Vector vecLOS = vecDirToEnemy; //vecMid - m_vecLastSight;
	vecLOS = vecLOS.Normalize();

	// Is the Gun looking at the target
	if (DotProduct(vecLOS, gpGlobals->v_forward) <= 0.866) // 30 degree slop
		fAttack = FALSE;
	else
		fAttack = TRUE;

	// rockets need a better aim
	if (DotProduct(vecLOS, gpGlobals->v_forward) <= 0.955)
		fRocket = FALSE;
	else
		fRocket = TRUE;

	// fire the guns
	if (m_iSpin && ((fAttack) || (m_fBeserk)))
	{
		Vector vecSrc;	// Shoot takes the barrels' positions
		SetTurretAnim(TURRET_ANIM_FIRE);
		Shoot(vecSrc, gpGlobals->v_forward );
	}
	else
	{
		SetTurretAnim(TURRET_ANIM_SPIN);
	}

	// and a rocket every m_flDelay seconds
	if (fRocket && m_flNextAttack <= gpGlobals->time)
	{
		FireRocket();
		m_flNextAttack = gpGlobals->time + m_flDelay;
	}

	//move the gun
	if (m_fBeserk)
	{
		if (RANDOM_LONG(0,9) == 0)
		{
			m_vecGoalAngles.y = RANDOM_FLOAT(0,360);
			m_vecGoalAngles.x = RANDOM_FLOAT(0,90) - 90 * m_iOrientation;
			TakeDamage(pev,pev,1, DMG_GENERIC); // don't beserk forever
			return;
		}
	}
	else if (fEnemyVisible)
	{
		if (vec.y > 360)
			vec.y -= 360;

		if (vec.y < 0)
			vec.y += 360;

		if (vec.x < -180)
			vec.x += 360;

		if (vec.x > 180)
			vec.x -= 360;

		// pin to [m_iMinPitch...60]
		if (m_iOrientation == 0)
		{
			if (vec.x > 60)
				vec.x = 60;
			else if (vec.x < m_iMinPitch)
				vec.x = m_iMinPitch;
		}

		m_vecGoalAngles.y = vec.y;
		m_vecGoalAngles.x = vec.x;

	}

	SpinUpCall();
	MoveTurret();
}

int CTFCSentry3::MoveTurret(void)
{
	int state = 0;
	// any x movement?

	if (m_vecCurAngles.x != m_vecGoalAngles.x)
	{
		float flDir = m_vecGoalAngles.x > m_vecCurAngles.x ? 1 : -1 ;

		m_vecCurAngles.x += 0.1 * m_fTurnRate * flDir;

		// if we started below the goal, and now we're past, peg to goal
		if (flDir == 1)
		{
			if (m_vecCurAngles.x > m_vecGoalAngles.x)
				m_vecCurAngles.x = m_vecGoalAngles.x;
		}
		else
		{
			if (m_vecCurAngles.x < m_vecGoalAngles.x)
				m_vecCurAngles.x = m_vecGoalAngles.x;
		}

		// the guns and the rocket launchers
		SetBoneController(1, -m_vecCurAngles.x);
		SetBoneController(2, -m_vecCurAngles.x);
		state = 1;
	}

	if (m_vecCurAngles.y != m_vecGoalAngles.y)
	{
		float flDir = m_vecGoalAngles.y > m_vecCurAngles.y ? 1 : -1 ;
		float flDist = fabs(m_vecGoalAngles.y - m_vecCurAngles.y);

		if (flDist > 180)
		{
			flDist = 360 - flDist;
			flDir = -flDir;
		}
		if (flDist > 30)
		{
			if (m_fTurnRate < m_iBaseTurnRate * 10)
			{
				m_fTurnRate += m_iBaseTurnRate;
			}
		}
		else if (m_fTurnRate > 45)
		{
			m_fTurnRate -= m_iBaseTurnRate;
		}
		else
		{
			m_fTurnRate += m_iBaseTurnRate;
		}

		m_vecCurAngles.y += 0.1 * m_fTurnRate * flDir;

		if (m_vecCurAngles.y < 0)
			m_vecCurAngles.y += 360;
		else if (m_vecCurAngles.y >= 360)
			m_vecCurAngles.y -= 360;

		if (flDist < (0.05 * m_iBaseTurnRate))
			m_vecCurAngles.y = m_vecGoalAngles.y;

		SetBoneController(0, m_vecCurAngles.y - pev->angles.y );
		state = 1;
	}

	if (!state)
		m_fTurnRate = m_iBaseTurnRate;

	return state;
}


//=========================================================
// Repulsor turret - a mini turret that fires repulsor beams
//=========================================================
class CRepulsorTurret : public CBaseTurret	// name ours
{
public:
	void Spawn( );
	void Precache(void);
	// other functions
	void Shoot(Vector &vecSrc, Vector &vecDirToEnemy);

	const char *GetKillDesc( void ) { return "a turret"; }
	const char *GetHudName( void ) { return "Repulsor Gun"; }
	int GetBattlePower( void );
};

LINK_ENTITY_TO_CLASS( monster_repturret, CRepulsorTurret );

void CRepulsorTurret::Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/miniturret.mdl");
	pev->health			= gSkillData.miniturretHealth;
	m_HackedGunPos		= Vector( 0, 0, 12.75 );
	m_flMaxSpin = 0;
	pev->view_ofs.z = 12.75;

	CBaseTurret::Spawn( );
	m_iRetractHeight = 16;
	m_iDeployHeight = 32;
	m_iMinPitch	= -15;
	UTIL_SetSize(pev, Vector(-16, -16, -m_iRetractHeight), Vector(16, 16, m_iRetractHeight));

	SetThink(Initialize);
	pev->nextthink = gpGlobals->time + 0.3;
}


void CRepulsorTurret::Precache()
{
	CBaseTurret::Precache( );
	PRECACHE_MODEL ("models/miniturret.mdl");
	UTIL_PrecacheOther( "repulsor_beam" );
}

int CRepulsorTurret::GetBattlePower( void )
{
	return ( pev->max_health * 0.1 ) * 50;
}

void CRepulsorTurret::Shoot(Vector &vecSrc, Vector &vecDirToEnemy)
{
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/ric1.wav", 1, 0.8);
	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 200, 3.0 );

	CBaseEntity *pBeam = CBaseEntity::Create( "repulsor_beam", vecSrc, g_vecZero, edict() );
	pBeam->pev->velocity = vecDirToEnemy * 1000;
	pBeam->pev->speed = 1000;
	pBeam->pev->dmg = 5;
	pBeam->pev->health = RANDOM_LONG( 2, 3 );

	pev->effects = pev->effects | EF_MUZZLEFLASH;
}
