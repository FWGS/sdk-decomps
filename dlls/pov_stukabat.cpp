/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
// POV: the stukabat. The player's weapon (weapon_stukabat)
// throws a bat (stkb_grenade) that flies after the nearest
// enemy like the ichthyosaur swims and blows up when it hits
// something or its flying time is over. A stukabat lying
// in the world flies to the player when he comes near and
// gives itself to him. File name and the names retail
// doesn't give are ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"flyingmonster.h"
#include	"nodes.h"
#include	"soundent.h"
#include	"animation.h"
#include	"effects.h"
#include	"weapons.h"
#include	"player.h"
#include	"gamerules.h"
#include	"decals.h"

extern DLL_GLOBAL BOOL g_fLoading;			// POV
extern int gEvilImpulse101;
extern DLL_GLOBAL short g_sModelIndexWExplosion;

// view model animations (names ours)
enum stukabat_e {
	STKB_IDLE = 0,
	STKB_FIDGET,
	STKB_UNUSED,
	STKB_THROW,
	STKB_HOLSTER,
	STKB_DRAW,
};

#define STKB_DEFAULT_GIVE	1
#define STKB_MAX_CARRY		10
#define STKB_WEIGHT			20

class CStukabatGrenade;

class CStukabat : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 3; }
	int GetItemInfo(ItemInfo *p);

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void DeploySound( void );			// name ours
	void FireSound( void );				// name ours
	void IdleSound( float flVolume );	// name ours

	BOOL Deploy( void );
	void Holster( void );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );

	void EXPORT StukabatIdle( void );

	void StartVModelSound( int iSound );
	void EXPORT PlayVModelSound( void );
	void StopVModelSound( void );	// name ours

	static const char *pDeploySounds[];	// names ours
	static const char *pFireSounds[];
	static const char *pIdleSounds[];
	static const char *pVModelSounds[];

	int m_iSoundPlayed;
	int m_fJustThrown;		// names ours
	int m_fFlying;			// flying to the player
	float m_flNextIdleSound;
	float m_flNextTrace;
	Vector m_vecGoal;
	int m_cActiveRockets;
};

class CStukabatGrenade : public CFlyingMonster
{
public:
	void  Spawn( void );
	void  Precache( void );
	void  SetYawSpeed( void );
	int   Classify( void );
	void  HandleAnimEvent( MonsterEvent_t *pEvent );
	CUSTOM_SCHEDULES;

	int	Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	static CStukabatGrenade *CreateStkbGrenade( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, CStukabat *pLauncher );

	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType ( int Type );

	void Killed( entvars_t *pevAttacker, int iGib );
	void GibMonster( void );

	void  StartTask( Task_t *pTask );
	void  RunTask( Task_t *pTask );

	BOOL  CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL  CheckRangeAttack1 ( float flDot, float flDist );

	float ChangeYaw( int speed );
	Activity GetStoppedActivity( void );
	void SetActivity( Activity NewActivity );

	void  Move( float flInterval );
	void  MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval );
	void  MonsterThink( void );
	void  Stop( void );
	void  Swim( void );
	Vector DoProbe(const Vector &Probe);

	float VectorToPitch( const Vector &vec);
	float FlPitchDiff( void );
	float ChangePitch( int speed );

	void  Detonate( void );		// names ours
	void  StartFlying( void );
	void  Hunt( void );

	Vector m_SaveVelocity;
	float m_idealDist;			// name ours
	int m_iExplode;				// name ours

	int  m_iMyClass;
	float m_flIgniteTime;
	float m_flStukabatFlyTime;

	static const char *pWarnSounds[];	// names ours
	static const char *pFlySounds[];

private:
	void EXPORT StkbTouch( CBaseEntity *pOther );
};


const char *CStukabatGrenade::pWarnSounds[] =
{
	"stukabat/stkb_fire2.wav",
};

const char *CStukabatGrenade::pFlySounds[] =
{
	"stukabat/stkb_flying1.wav",
};

const char *CStukabat::pDeploySounds[] =
{
	"stukabat/stkb_deploy1.wav",
	"stukabat/stkb_deploy2.wav",
};

const char *CStukabat::pFireSounds[] =
{
	"stukabat/stkb_fire1.wav",
	"stukabat/stkb_fire2.wav",
};

const char *CStukabat::pIdleSounds[] =
{
	"stukabat/stkb_idletpick1.wav",
	"stukabat/stkb_idletpick2.wav",
};


LINK_ENTITY_TO_CLASS( weapon_stukabat, CStukabat );

TYPEDESCRIPTION	CStukabat::m_SaveData[] =
{
	DEFINE_FIELD( CStukabat, m_cActiveRockets, FIELD_INTEGER ),
	DEFINE_FIELD( CStukabat, m_iSoundPlayed, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CStukabat, CBasePlayerWeapon );


LINK_ENTITY_TO_CLASS( stkb_grenade, CStukabatGrenade );

TYPEDESCRIPTION	CStukabatGrenade::m_SaveData[] =
{
	DEFINE_FIELD( CStukabatGrenade, m_SaveVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CStukabatGrenade, m_iMyClass, FIELD_INTEGER ),
	DEFINE_FIELD( CStukabatGrenade, m_flIgniteTime, FIELD_TIME ),
	DEFINE_FIELD( CStukabatGrenade, m_flStukabatFlyTime, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CStukabatGrenade, CFlyingMonster );


CStukabatGrenade *CStukabatGrenade::CreateStkbGrenade( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, CStukabat *pLauncher )
{
	CStukabatGrenade *pBat = GetClassPtr( (CStukabatGrenade *)NULL );

	UTIL_SetOrigin( pBat->pev, vecOrigin );
	pBat->pev->angles = vecAngles;
	pBat->Spawn();
	pBat->pev->owner = pOwner->edict();
	pBat->m_hEnemy = NULL;
	pBat->m_flStukabatFlyTime = gSkillData.plrStukabatFlyTime;

	return pBat;
}


//=========================================================
// monster-specific tasks and states
//=========================================================
enum
{
	TASK_STKB_HUNTFLY = LAST_COMMON_TASK + 1,	// name ours
};

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

static Task_t	tlStkbRocketHuntFly[] =
{
	{ TASK_SET_ACTIVITY,			(float)ACT_RUN },
	{ TASK_STKB_HUNTFLY,			0.0 },
};

static Schedule_t	slStkbRocketHuntFly[] =
{
	{
		tlStkbRocketHuntFly,
		ARRAYSIZE(tlStkbRocketHuntFly),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_SEE_ENEMY		|
		bits_COND_NEW_ENEMY		|
		bits_COND_HEAR_SOUND,
		bits_SOUND_PLAYER |
		bits_SOUND_COMBAT,
		"StkbRocketHuntFly"
	},
};

DEFINE_CUSTOM_SCHEDULES(CStukabatGrenade)
{
	slStkbRocketHuntFly,
};
IMPLEMENT_CUSTOM_SCHEDULES(CStukabatGrenade, CFlyingMonster);


const char *CStukabat::pVModelSounds[] =
{
	"stukabat/stkb_idle1.wav",
};


//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CStukabatGrenade :: Classify ( void )
{
	if (m_iMyClass != 0)
		return m_iMyClass; // protect against recursion

	if (m_hEnemy != NULL)
	{
		m_iMyClass = CLASS_INSECT; // no one cares about it
		switch( m_hEnemy->Classify( ) )
		{
			case CLASS_PLAYER:
			case CLASS_HUMAN_PASSIVE:
			case CLASS_HUMAN_MILITARY:
				m_iMyClass = 0;
				return CLASS_ALIEN_MILITARY; // barney's get mad, grunts get mad at it
		}
		m_iMyClass = 0;
	}

	return CLASS_ALIEN_BIOWEAPON;
}


//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CStukabatGrenade :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( flDot >= 0.7 )
	{
		return TRUE;
	}
	return FALSE;
}

void CStukabatGrenade::StkbTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	g_vecAttackDir = pev->velocity.Normalize( );
	pev->health = -1;
	Killed( pev, 4 );
}

//=========================================================
// CheckRangeAttack1  - fly in for a bite
//
//=========================================================
BOOL CStukabatGrenade :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDot > -0.7 && flDist <= 1536 && m_idealDist <= 1536 )
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CStukabatGrenade :: SetYawSpeed ( void )
{
	pev->yaw_speed = 100;
}



//=========================================================
// Killed - overrides CFlyingMonster.
//
void CStukabatGrenade :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->model = iStringNull;// make invisible
	SetTouch( NULL );
	pev->takedamage = DAMAGE_NO;

	EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, "stukabat/stkb_die1.wav", 1, ATTN_NORM, 0, PITCH_NORM);

	Detonate( );

	UTIL_BloodDrips( pev->origin, g_vecZero, BloodColor(), 200 );

	CBaseMonster::Killed( pevAttacker, GIB_ALWAYS );

	SetThink( NULL );
	UTIL_Remove( this );
}

#define STKB_AE_FLAP	9	// name ours

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CStukabatGrenade :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case STKB_AE_FLAP:
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pFlySounds[ RANDOM_LONG(0,ARRAYSIZE(pFlySounds)-1) ], 1.0, ATTN_IDLE, 0, RANDOM_LONG(98,102) );
		break;
	default:
		CFlyingMonster::HandleAnimEvent( pEvent );
		break;
	}
}

void CStukabatGrenade :: SetActivity( Activity NewActivity )
{
}

//=========================================================
// Spawn
//=========================================================
void CStukabatGrenade :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/stukabat.mdl");
	UTIL_SetSize( pev, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ) );
	UTIL_SetOrigin( pev, pev->origin );

	MonsterInit();

	pev->solid			= SOLID_BBOX;
	pev->movetype		= MOVETYPE_FLY;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->health			= (int)(gSkillData.plrDmgStukabat / 2);
	pev->view_ofs		= Vector ( 0, 0, 6 );
	m_flFieldOfView		= VIEW_FIELD_WIDE;
	m_MonsterState		= MONSTERSTATE_COMBAT;
	SetBits(pev->flags, FL_FLY);
	SetFlyingSpeed( 200 );
	SetFlyingMomentum( 2.5 );	// Set momentum constant

	m_afCapability		= bits_CAP_RANGE_ATTACK1 | bits_CAP_FLY;

	pev->classname = MAKE_STRING( "stkb_grenade" );
	pev->dmg = gSkillData.plrDmgStukabat;
	m_flStukabatFlyTime = gSkillData.plrStukabatFlyTime;

	SetTouch( StkbTouch );

	Vector Forward;
	UTIL_MakeVectorsPrivate(pev->angles, Forward, 0, 0);
	pev->velocity = m_flightSpeed * Forward.Normalize();
	m_SaveVelocity = pev->velocity;

	StartFlying( );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CStukabatGrenade :: Precache()
{
	PRECACHE_MODEL("models/stukabat.mdl");

	PRECACHE_SOUND_ARRAY( pFlySounds );
	PRECACHE_SOUND_ARRAY( pWarnSounds );
	PRECACHE_SOUND("common/bodysplat.wav");
	PRECACHE_SOUND("common/bodydrop2.wav");
	PRECACHE_SOUND("stukabat/stkb_die1.wav");

	m_iExplode = PRECACHE_MODEL( "sprites/bexplo.spr" );
}

//=========================================================
// GetSchedule
//=========================================================
Schedule_t* CStukabatGrenade::GetSchedule()
{
	// eat them
	if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
	{
		return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
	}
	// chase them down and eat them
	if ( m_hEnemy != NULL )
	{
		return GetScheduleOfType( SCHED_CHASE_ENEMY );
	}

	return CFlyingMonster :: GetSchedule();
}


//=========================================================
//=========================================================
Schedule_t* CStukabatGrenade :: GetScheduleOfType ( int Type )
{
	switch	( Type )
	{
	case SCHED_IDLE_WALK:
	case SCHED_STANDOFF:
	case SCHED_FAIL:
		return slStkbRocketHuntFly;
	case SCHED_CHASE_ENEMY:
		AlertSound( );
	}

	return CBaseMonster :: GetScheduleOfType( Type );
}



//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.
//=========================================================
void CStukabatGrenade::StartTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_STKB_HUNTFLY:
		break;

	default:
		CFlyingMonster::StartTask(pTask);
		break;
	}
}

void CStukabatGrenade :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_STKB_HUNTFLY:
		if (m_fSequenceFinished)
		{
			TaskComplete( );
		}
		break;

	default:
		CFlyingMonster :: RunTask ( pTask );
		break;
	}
}



float CStukabatGrenade::VectorToPitch( const Vector &vec )
{
	float pitch;
	if (vec.z == 0 && vec.x == 0)
		pitch = 0;
	else
	{
		pitch = (int) (atan2(vec.z, sqrt(vec.x*vec.x+vec.y*vec.y)) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}
	return pitch;
}

//=========================================================
void CStukabatGrenade::Move(float flInterval)
{
	CFlyingMonster::Move( flInterval );
}

float CStukabatGrenade::FlPitchDiff( void )
{
	float	flPitchDiff;
	float	flCurrentPitch;

	flCurrentPitch = UTIL_AngleMod( pev->angles.z );

	if ( flCurrentPitch == pev->idealpitch )
	{
		return 0;
	}

	flPitchDiff = pev->idealpitch - flCurrentPitch;

	if ( pev->idealpitch > flCurrentPitch )
	{
		if (flPitchDiff >= 180)
			flPitchDiff = flPitchDiff - 360;
	}
	else
	{
		if (flPitchDiff <= -180)
			flPitchDiff = flPitchDiff + 360;
	}
	return flPitchDiff;
}

float CStukabatGrenade :: ChangePitch( int speed )
{
	if ( pev->movetype == MOVETYPE_FLY )
	{
		float diff = FlPitchDiff();
		float target = 0;
		if ( m_IdealActivity != GetStoppedActivity() )
		{
			if (diff < -20)
				target = 45;
			else if (diff > 20)
				target = -45;
		}
		pev->angles.x = UTIL_Approach(target, pev->angles.x, 220.0 * 0.1 );
	}
	return 0;
}

float CStukabatGrenade::ChangeYaw( int speed )
{
	if ( pev->movetype == MOVETYPE_FLY )
	{
		float diff = FlYawDiff();
		float target = 0;

		if ( m_IdealActivity != GetStoppedActivity() )
		{
			if ( diff < -20 )
				target = 20;
			else if ( diff > 20 )
				target = -20;
		}
		pev->angles.z = UTIL_Approach( target, pev->angles.z, 220.0 * 0.1 );
	}
	return CFlyingMonster::ChangeYaw( speed );
}


Activity CStukabatGrenade:: GetStoppedActivity( void )
{
	if ( pev->movetype != MOVETYPE_FLY )		// UNDONE: Ground idle here, IDLE may be something else
		return ACT_IDLE;
	return ACT_WALK;
}

void CStukabatGrenade::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
	m_SaveVelocity = vecDir * m_flightSpeed;
}


void CStukabatGrenade::MonsterThink ( void )
{
	CFlyingMonster::MonsterThink( );

	if (pev->deadflag == DEAD_NO)
	{
		if (m_MonsterState != MONSTERSTATE_SCRIPT)
		{
			Hunt( );
			Swim( );
		}
	}
}

void CStukabatGrenade :: Stop( void )
{
	m_flightSpeed = 200.0;
}

void CStukabatGrenade::Swim( )
{
	Vector start = pev->origin;

	Vector Angles;
	Vector Forward, Right, Up;

	if (FBitSet( pev->flags, FL_ONGROUND))
	{
		pev->angles.x = 0;
		pev->angles.y += RANDOM_FLOAT( -45, 45 );
		ClearBits( pev->flags, FL_ONGROUND );

		Angles = Vector( -pev->angles.x, pev->angles.y, pev->angles.z );
		UTIL_MakeVectorsPrivate(Angles, Forward, Right, Up);

		pev->velocity = Forward * 200 + Up * 200;

		return;
	}

	if (m_IdealActivity == ACT_WALK)
		SetActivity( ACT_RUN );
	if (m_IdealActivity == ACT_RUN)
		pev->framerate = m_flightSpeed / 200;

#define PROBE_LENGTH 1536
	Angles = UTIL_VecToAngles( m_SaveVelocity );
	Angles.x = -Angles.x;
	UTIL_MakeVectorsPrivate(Angles, Forward, Right, Up);

	Vector f, u, l, r, d;
	f = DoProbe(start + PROBE_LENGTH   * Forward);
	r = DoProbe(start + PROBE_LENGTH/3 * Forward+Right);
	l = DoProbe(start + PROBE_LENGTH/3 * Forward-Right);
	u = DoProbe(start + PROBE_LENGTH/3 * Forward+Up);
	d = DoProbe(start + PROBE_LENGTH/3 * Forward-Up);

	Vector SteeringVector = f+r+l+u+d;
	m_SaveVelocity = (m_SaveVelocity + SteeringVector/2).Normalize();

	Angles = Vector( -pev->angles.x, pev->angles.y, pev->angles.z );
	UTIL_MakeVectorsPrivate(Angles, Forward, Right, Up);

	float flDot = DotProduct( Forward, m_SaveVelocity );
	if (flDot > 0.5)
		pev->velocity = m_SaveVelocity = m_SaveVelocity * m_flightSpeed;
	else if (flDot > 0)
		pev->velocity = m_SaveVelocity = m_SaveVelocity * m_flightSpeed * (flDot + 0.5);
	else
		pev->velocity = m_SaveVelocity = m_SaveVelocity * 200;

	Angles = UTIL_VecToAngles( m_SaveVelocity );

	// Smooth Pitch
	//
	if (Angles.x > 180)
		Angles.x = Angles.x - 360;
	pev->angles.x = UTIL_Approach(Angles.x, pev->angles.x, 50 * 0.1 );
	if (pev->angles.x < -180) pev->angles.x = -180;
	if (pev->angles.x >  180) pev->angles.x =  180;

	// Smooth Yaw and generate Roll
	//
	float turn = 360;

	if (fabs(Angles.y - pev->angles.y) < fabs(turn))
	{
		turn = Angles.y - pev->angles.y;
	}
	if (fabs(Angles.y - pev->angles.y + 360) < fabs(turn))
	{
		turn = Angles.y - pev->angles.y + 360;
	}
	if (fabs(Angles.y - pev->angles.y - 360) < fabs(turn))
	{
		turn = Angles.y - pev->angles.y - 360;
	}

	float speed = m_flightSpeed * 0.1;

	if (fabs(turn) > speed)
	{
		if (turn < 0.0)
		{
			turn = -speed;
		}
		else
		{
			turn = speed;
		}
	}
	pev->angles.y += turn;
	pev->angles.z -= turn;
	pev->angles.y = fmod((pev->angles.y + 360.0), 360.0);

	static float yaw_adj;

	yaw_adj = yaw_adj * 0.8 + turn;

	SetBoneController( 0, -yaw_adj / 4.0 );

	// Roll Smoothing
	//
	turn = 360;
	if (fabs(Angles.z - pev->angles.z) < fabs(turn))
	{
		turn = Angles.z - pev->angles.z;
	}
	if (fabs(Angles.z - pev->angles.z + 360) < fabs(turn))
	{
		turn = Angles.z - pev->angles.z + 360;
	}
	if (fabs(Angles.z - pev->angles.z - 360) < fabs(turn))
	{
		turn = Angles.z - pev->angles.z - 360;
	}
	speed = m_flightSpeed/2 * 0.1;
	if (fabs(turn) < speed)
	{
		pev->angles.z += turn;
	}
	else
	{
		if (turn < 0.0)
		{
			pev->angles.z -= speed;
		}
		else
		{
			pev->angles.z += speed;
		}
	}
	if (pev->angles.z < -20) pev->angles.z = -20;
	if (pev->angles.z >  20) pev->angles.z =  20;

	UTIL_MakeVectorsPrivate( Vector( -Angles.x, Angles.y, Angles.z ), Forward, Right, Up);
}


Vector CStukabatGrenade::DoProbe(const Vector &Probe)
{
	Vector WallNormal = Vector(0,0,0);
	float frac;
	BOOL bBumpedSomething = ProbeZ(pev->origin, Probe, &frac);

	TraceResult tr;
	TRACE_MONSTER_HULL(edict(), pev->origin, Probe, dont_ignore_monsters, edict(), &tr);
	if ( tr.fAllSolid || tr.flFraction < 0.99 )
	{
		if (tr.flFraction < 0.0) tr.flFraction = 0.0;
		if (tr.flFraction > 1.0) tr.flFraction = 1.0;
		if (tr.flFraction < frac)
		{
			frac = tr.flFraction;
			bBumpedSomething = TRUE;
			WallNormal = tr.vecPlaneNormal;
		}
	}

	if (bBumpedSomething && (m_hEnemy == NULL || tr.pHit != m_hEnemy->edict()))
	{
		Vector ProbeDir = Probe - pev->origin;

		Vector NormalToProbeAndWallNormal = CrossProduct(ProbeDir, WallNormal);
		Vector SteeringVector = CrossProduct( NormalToProbeAndWallNormal, ProbeDir);

		float SteeringForce = m_flightSpeed * (1-frac) * (DotProduct(WallNormal.Normalize(), m_SaveVelocity.Normalize()));
		if (SteeringForce < 0.0)
		{
			SteeringForce = -SteeringForce;
		}
		SteeringVector = SteeringForce * SteeringVector.Normalize();

		return SteeringVector;
	}
	return Vector(0, 0, 0);
}


// the bat blows up like a grenade (CGrenade::Detonate and Explode in one)
void CStukabatGrenade::Detonate( void )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	vecSpot = pev->origin + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -40 ),  ignore_monsters, ENT(pev), & tr);

	float		flRndSound;// sound randomizer

	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	pev->takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( tr.flFraction != 1.0 )
	{
		pev->origin = tr.vecEndPos + (tr.vecPlaneNormal * (pev->dmg - 24) * 0.6);
	}

	int iContents = UTIL_PointContents ( pev->origin );

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		if (iContents == CONTENTS_WATER)
		{
			WRITE_SHORT( g_sModelIndexWExplosion );
		}
		else
		{
			WRITE_SHORT( m_iExplode );
		}
		WRITE_BYTE( (pev->dmg - 50) * .60  ); // scale * 10
		WRITE_BYTE( 15  ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );

	RadiusDamage ( pev, pev, pev->dmg, CLASS_NONE, DMG_BLAST );

	if ( RANDOM_FLOAT( 0 , 1 ) < 0.5 )
	{
		UTIL_DecalTrace( &tr, DECAL_SCORCH1 );
	}
	else
	{
		UTIL_DecalTrace( &tr, DECAL_SCORCH2 );
	}

	flRndSound = RANDOM_FLOAT( 0 , 1 );

	switch ( RANDOM_LONG( 0, 2 ) )
	{
		case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM);	break;
		case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM);	break;
		case 2:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM);	break;
	}

	pev->effects |= EF_NODRAW;
	pev->velocity = g_vecZero;

	if (iContents != CONTENTS_WATER)
	{
		int sparkCount = RANDOM_LONG(0,3);
		for ( int i = 0; i < sparkCount; i++ )
			Create( "spark_shower", pev->origin, tr.vecPlaneNormal, NULL );
	}
}

void CStukabatGrenade :: GibMonster( void )
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "common/bodysplat.wav", 0.7, ATTN_NORM, 0, PITCH_NORM);
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "common/bodydrop2.wav", 0.7, ATTN_NORM, 0, PITCH_NORM);

	if ( CVAR_GET_FLOAT( "violence_agibs" ) != 0 )
		CGib::SpawnRandomGibs( pev, 4, 0 );
}

void CStukabatGrenade::StartFlying( void )
{
	pev->sequence = 10;
	ResetSequenceInfo( );

	CBaseEntity *pPlayer = UTIL_PlayerByIndex( 1 );
	pev->angles = pPlayer->pev->angles;

	m_flIgniteTime = gpGlobals->time;
}

void CStukabatGrenade::Hunt( void )
{
	if ( m_hEnemy == NULL || !m_hEnemy->IsAlive() )
	{
		Look( 1536 );
		m_hEnemy = BestVisibleEnemy( );

		if ( m_hEnemy == NULL )
			m_hEnemy = UTIL_PlayerByIndex( 1 );
	}

	if ( m_flIgniteTime + m_flStukabatFlyTime < gpGlobals->time )
	{
		SetTouch( NULL );
		StkbTouch( NULL );
		return;
	}

	if ( m_flStukabatFlyTime * 0.8 + m_flIgniteTime < gpGlobals->time )
	{
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, pWarnSounds[ RANDOM_LONG(0,ARRAYSIZE(pWarnSounds)-1) ], 1.0, ATTN_IDLE, 0, RANDOM_LONG(98,100) );
	}
}



void CStukabat::Spawn( )
{
	Precache( );
	m_iId = WEAPON_STUKABAT;
	SET_MODEL(ENT(pev), "models/stukabat.mdl");

	FallInit();//get ready to fall down.

	m_iDefaultAmmo = STKB_DEFAULT_GIVE;

	pev->sequence = 18;
	pev->animtime = gpGlobals->time;
	pev->framerate = 0.6;
	pev->view_ofs = Vector( 0, 0, 6 );

	SetThink( StukabatIdle );
	pev->nextthink = gpGlobals->time + 0.2;

	m_fFlying = 0;
	m_flNextIdleSound = 0;
	m_flNextTrace = 0;
	m_fJustThrown = 0;
}


void CStukabat::Precache( void )
{
	PRECACHE_MODEL("models/stukabat.mdl");
	PRECACHE_MODEL("models/w_stukabat.mdl");
	PRECACHE_MODEL("models/v_stukabat.mdl");
	PRECACHE_MODEL("models/p_stukabat.mdl");
	UTIL_PrecacheOther("stkb_grenade");

	PRECACHE_SOUND_ARRAY( pFireSounds );
	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pDeploySounds );
	PRECACHE_SOUND_ARRAY( pVModelSounds );
}


int CStukabat::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Stukabats";
	p->iMaxAmmo1 = STKB_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_STUKABAT;
	p->iWeight = STKB_WEIGHT;
	p->iFlags = ITEM_FLAG_NOAUTOSWITCHEMPTY | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}


void CStukabat::DeploySound( void )
{
	ALERT( at_console, "DEPLOY SOUND!\n" );
	EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, pDeploySounds[ RANDOM_LONG(0,ARRAYSIZE(pDeploySounds)-1) ], 0.7, ATTN_IDLE, 0, RANDOM_LONG(98,100) );
}

void CStukabat::FireSound( void )
{
	EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, pFireSounds[ RANDOM_LONG(0,ARRAYSIZE(pFireSounds)-1) ], 1.0, ATTN_IDLE, 0, RANDOM_LONG(98,100) );
	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 600, 3.0 );
}

void CStukabat::IdleSound( float flVolume )
{
	if ( gpGlobals->time > m_flNextIdleSound )
	{
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], flVolume, 1.25, 0, RANDOM_LONG(98,100) );
		m_flNextIdleSound = gpGlobals->time + RANDOM_FLOAT( 2, 3 );
	}
}


BOOL CStukabat::Deploy( )
{
	if ( !gEvilImpulse101 )
		DeploySound( );

	m_flNextPrimaryAttack = gpGlobals->time + 1.5;
	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_LONG( 5, 8 );

	return DefaultDeploy( "models/v_stukabat.mdl", "models/p_stukabat.mdl", STKB_DRAW, "stukabat" );
}


void CStukabat::Holster( void )
{
	if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_STUKABAT);
		SetThink( DestroyItem );
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	StopVModelSound( );
	DefaultHolster( STKB_HOLSTER, 1.5, 1.5 );
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}


void CStukabat::PrimaryAttack()
{
	if ( m_pPlayer->m_fDidHolster )
	{
		// POV: still holstering
		WeaponIdle( );
		return;
	}

	if ( m_flNextPrimaryAttack > gpGlobals->time )
		return;

	if ( pev->waterlevel == 3 )
	{
		ResetEmptySound( );
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->time + 0.5;
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_LONG( 3, 4 );
		return;
	}

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

		Vector vecSrc = m_pPlayer->GetGunPosition( );
		Vector vecDir = gpGlobals->v_forward;
		TraceResult tr;
		UTIL_TraceLine( vecSrc, vecSrc + vecDir * 64, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr );

		if ( tr.flFraction < 1.0 )
		{
			// no room to throw
			ResetEmptySound( );
			PlayEmptySound( );
			m_flNextPrimaryAttack = gpGlobals->time + 0.5;
			m_flTimeWeaponIdle = gpGlobals->time + RANDOM_LONG( 3, 4 );
			return;
		}

		StopVModelSound( );
		SendWeaponAnim( STKB_THROW );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		CStukabatGrenade *pBat = CStukabatGrenade::CreateStkbGrenade( vecSrc, m_pPlayer->pev->v_angle, m_pPlayer, this );

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );// Create stomps on globals, so remake.
		pBat->pev->velocity = pBat->pev->velocity + gpGlobals->v_forward * DotProduct( m_pPlayer->pev->velocity, gpGlobals->v_forward );

		EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, pFireSounds[ RANDOM_LONG(0,ARRAYSIZE(pFireSounds)-1) ], 0.9, ATTN_NORM, 0, PITCH_NORM );

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
		m_pPlayer->pev->punchangle.x -= 5;

		m_fJustThrown = 1;

		m_flNextPrimaryAttack = gpGlobals->time + 2.2;
	}
	else
	{
		ResetEmptySound( );
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->time + 0.5;
	}

	m_flTimeWeaponIdle = gpGlobals->time + 0.1;
}


void CStukabat::SecondaryAttack( void )
{
	if ( m_pPlayer->m_fDidHolster )
		WeaponIdle( );
}


void CStukabat::WeaponIdle( void )
{
	if ( g_fLoading == TRUE )
	{
		StopVModelSound( );
		return;
	}

	if ( m_pPlayer->m_fDidHolster )
	{
		// POV: the holster animation is over, draw the weapon again
		if ( m_flTimeWeaponIdle <= gpGlobals->time )
		{
			m_pPlayer->m_fDidHolster = FALSE;
			Deploy( );
			UpdateItemInfo( );
		}
		return;
	}

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if (m_fJustThrown)
	{
		m_fJustThrown = 0;

		if ( !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] )
		{
			RetireWeapon();
			return;
		}

		SendWeaponAnim( STKB_DRAW );
		m_flNextPrimaryAttack = gpGlobals->time + 1.5;
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_LONG( 5, 8 );
		return;
	}

	if ( m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] )
	{
		if ( !m_pPlayer->pev->viewmodel )
		{
			Deploy( );
			m_flTimeWeaponIdle = gpGlobals->time + 0.1;
			return;
		}

		m_iSoundPlayed = -1;

		switch ( RANDOM_LONG( 0, 1 ) )
		{
		case 0:
			SendWeaponAnim( STKB_IDLE );
			m_flTimeWeaponIdle = gpGlobals->time + RANDOM_LONG( 2, 6 );
			break;
		case 1:
			StartVModelSound( 0 );
			SendWeaponAnim( STKB_FIDGET );
			m_flTimeWeaponIdle = gpGlobals->time + RANDOM_LONG( 6, 8 );
			break;
		}
	}
}


// A stukabat lying in the world flies to the player once he is near
// and gives itself to him.
void CStukabat::StukabatIdle( void )
{
	CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( 1 );

	if ( !pPlayer )
	{
		pev->nextthink = gpGlobals->time + 1.5;
		return;
	}

	if ( !UTIL_StrIEq( STRING(pev->targetname), "STKB122363" ) )
	{
		ALERT( at_console, "CStukabat: found invalid me (np)\n" );
		SetThink( NULL );
	}

	if ( !pPlayer->FVisible( this ) )
	{
		if ( pev->sequence != 18 )
		{
			pev->sequence = 18;
			pev->framerate = 0.6;
			ResetSequenceInfo( );
		}

		if ( RANDOM_LONG( 0, 10 ) == 0 )
			IdleSound( 0.3 );

		pev->nextthink = gpGlobals->time + 0.2;
		return;
	}

	if ( (pPlayer->Center() - Center()).Length() < 50 )
	{
		if ( !m_fFlying )
		{
			pev->nextthink = gpGlobals->time + 0.1;
			return;
		}

		for ( int i = 0; i < m_iDefaultAmmo; i++ )
			pPlayer->GiveNamedItem( "weapon_stukabat" );

		SetThink( NULL );
		UTIL_Remove( this );
		return;
	}

	if ( (pPlayer->Center() - Center()).Length() < 256 )
	{
		m_fFlying = 1;
		pev->movetype = MOVETYPE_STEP;
		StudioFrameAdvance( );

		if ( pev->sequence == 18 )
		{
			pev->sequence = 19;
			pev->frame = 0;
			pev->framerate = 3.2;
			ResetSequenceInfo( );
		}
		else
		{
			Vector vecFlat;

			if ( gpGlobals->time > m_flNextTrace )
			{
				vecFlat = (Center() - pPlayer->Center()).Normalize();
				vecFlat.z = 0;
				m_vecGoal = pev->origin + vecFlat * 128;

				TraceResult tr;
				tr.flFraction = 1.0;
				UTIL_TraceLine( EyePosition(), EyePosition() + vecFlat * 68, ignore_monsters, ENT(pev), &tr );

				if ( tr.flFraction < 1.0 )
				{
					if ( CBaseEntity::Instance( tr.pHit )->Classify() )
						tr.vecPlaneNormal = (EyePosition() - tr.vecEndPos).Normalize();

					UTIL_MakeVectorsPrivate( Vector( 0, UTIL_AngleMod( UTIL_VecToAngles( tr.vecPlaneNormal ).y + 195 ), 0 ), vecFlat, NULL, NULL );
					m_vecGoal = pev->origin - vecFlat * 128;
				}

				m_flNextTrace = gpGlobals->time + 0.5;
			}

			vecFlat = m_vecGoal - Center();
			vecFlat.z = 0;
			pev->angles = UTIL_VecToAngles( vecFlat );
			UTIL_MoveToOrigin( ENT(pev), vecFlat * 256, 200 / m_flGroundSpeed, MOVE_STRAFE );
		}

		IdleSound( 0.7 );
		pev->nextthink = gpGlobals->time + 0.2;
		return;
	}
	else
	{
		if ( pev->sequence != 18 )
		{
			pev->sequence = 18;
			pev->framerate = 0.6;
			ResetSequenceInfo( );
		}

		if ( RANDOM_LONG( 0, 10 ) == 0 )
			IdleSound( 0.3 );
	}

	pev->nextthink = gpGlobals->time + 0.2;
}


// POV: the fidget animation's sound, played by think
void CStukabat::StartVModelSound( int iSound )
{
	m_iSoundPlayed = iSound;

	switch ( iSound )
	{
	case 0:
		pev->nextthink = gpGlobals->time;
		break;
	default:
		ALERT( at_console, "StartVModelSound: no %d sound to play!!\n", iSound );
		m_iSoundPlayed = -1;
		break;
	}

	if ( m_iSoundPlayed != -1 )
		SetThink( PlayVModelSound );
}

void CStukabat::PlayVModelSound( void )
{
	EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_VOICE, pVModelSounds[m_iSoundPlayed], 0.3, ATTN_IDLE, 0, PITCH_NORM );
	SetThink( NULL );
	pev->nextthink = gpGlobals->time;
}

void CStukabat::StopVModelSound( void )
{
	SetThink( NULL );
	pev->nextthink = gpGlobals->time;

	if ( m_iSoundPlayed != -1 )
		EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_VOICE, pVModelSounds[m_iSoundPlayed], 0.3, ATTN_IDLE, SND_STOP, PITCH_NORM );
}
