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
// the bandit base: Half-Life's human grunt reworked into
// the base of Gunman's human enemies, the bandits and
// grunts, the demoman and the hostile scientist
//=========================================================
#ifndef BANDIT_H
#define BANDIT_H

#ifndef SQUADMONSTER_H
#include "squadmonster.h"
#endif

//=========================================================
// what a bandit carries, in pev->weapons
//=========================================================
#define BANDIT_MACHINEGUN			( 1 << 0 )
#define BANDIT_ROCKETLAUNCHER		( 1 << 1 )
#define BANDIT_SHOTGUN				( 1 << 2 )
#define BANDIT_HANDGRENADE			( 1 << 3 )
#define BANDIT_ROCKETLAUNCHER2		( 1 << 4 )	// fires like the rocket launcher
#define BANDIT_PISTOLS_CLOSE		( 1 << 6 )	// the grunt's two pistols, close up
#define BANDIT_MACHINEGUN2			( 1 << 7 )	// the mechanical gun bandit's machine gun

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_BANDIT_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_BANDIT_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_BANDIT_COVER_AND_RELOAD,
	SCHED_BANDIT_SWEEP,
	SCHED_BANDIT_FOUND_ENEMY,
	SCHED_BANDIT_REPEL,
	SCHED_BANDIT_REPEL_ATTACK,
	SCHED_BANDIT_REPEL_LAND,
	SCHED_BANDIT_WAIT_FACE_ENEMY,
	SCHED_BANDIT_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_BANDIT_ELOF_FAIL,
	SCHED_BANDIT_RANGE_MACHINEGUN,
	SCHED_BANDIT_RANGE_ROCKET,
	SCHED_BANDIT_RANGE_SHOTGUN,
	SCHED_BANDIT_DRAW,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_BANDIT_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_BANDIT_SPEAK_SENTENCE,
	TASK_BANDIT_CHECK_FIRE,
	TASK_BANDIT_ROLL_RIGHT,
	TASK_BANDIT_ROLL_LEFT,
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_BANDIT_NOFIRE	( bits_COND_SPECIAL1 )

class CBaseBandit : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed ( void );
	int  Classify ( void );
	int ISoundMask ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL FCanCheckAttacks ( void );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	void CheckAmmo ( void );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	void IdleSound( void );
	Vector GetGunPosition( void );
	void Shoot ( void );
	void ShootGauss ( Vector vecShootOrigin, int iGun );
	void FireGauss ( Vector vecOrigSrc, Vector vecDir, float flDamage, CBaseEntity *pShooter, int iGun );
	void ShootFastShot ( Vector vecShootOrigin );
	void ShootBoltShot ( Vector vecShootOrigin );
	void Shotgun ( void );
	void PrescheduleThink ( void );
	Activity GetDeathActivity( void );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	int	Save( CSave &save );
	int Restore( CRestore &restore );

	CBaseEntity	*Kick( void );
	Schedule_t	*GetSchedule( void );
	Schedule_t  *GetScheduleOfType ( int Type );

	BOOL FOkToSpeak( void );
	void JustSpoke( void );

	// Gunman's additions, each bandit says how it fights
	virtual void Crouch( void ) {}				// get down to shoot
	virtual void Stand( void ) {}				// get up again
	virtual BOOL HasUnlimitedAmmo( void ) = 0;	// never reloads
	virtual BOOL HasGrenades( void ) { return FALSE; }
	virtual BOOL CanCrouch( void ) { return FALSE; }
	virtual int GetGunsFiring( void ) = 0;		// which guns can hit the enemy: 1 one, 2 both
	virtual void GetHeadGib( BOOL *pfHeadGib, int *piGroup, int *piHeadless ) = 0;	// a head shot takes off this body group

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	void SpeakSentence( void );

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextGrenadeCheck;
	float m_flNextRollCheck;
	float m_flLastEnemySightTime;	// not used
	float m_flNextPainTime;
	float m_flLastShot;				// not used

	Vector	m_vecTossVelocity;

	BOOL	m_bThrowGrenade;
	BOOL	m_bStanding;
	BOOL	m_bFirstEncounter;// only put on the handsign show in the squad's first encounter.
	int		m_cClipSize;

	int		m_iVoicePitch;

	int		m_iBrassShell;
	int		m_iShotgunShell;

	int		m_iSentence;
	int		m_iGunsDrawn;
	int		m_iBeam;
	unsigned short m_usShotgun;

	static const char *pBanditSentences[];
};

#endif // BANDIT_H
