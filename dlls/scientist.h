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
#ifndef SCIENTIST_H
#define SCIENTIST_H

//=========================================================
// POV: the scientist, out of scientist.cpp so that the
// brave scientist, the SPF scientist and the workers can
// derive from it. File name ours.
//=========================================================

#ifndef TALKMONSTER_H
#include "talkmonster.h"
#endif

#define		NUM_SCIENTIST_HEADS		4 // four heads available for scientist model
enum { HEAD_GLASSES = 0, HEAD_EINSTEIN = 1, HEAD_LUTHER = 2, HEAD_SLICK = 3 };

enum
{
	SCHED_HIDE = LAST_TALKMONSTER_SCHEDULE + 1,
	SCHED_FEAR,
	SCHED_PANIC,
	SCHED_STARTLE,
	SCHED_TARGET_CHASE_SCARED,
	SCHED_TARGET_FACE_SCARED,
};

enum
{
	TASK_SAY_FEAR = LAST_TALKMONSTER_TASK + 1,
	TASK_RUN_PATH_SCARED,
	TASK_SCREAM,
	TASK_RANDOM_SCREAM,
	TASK_MOVE_TO_TARGET_RANGE_SCARED,
};

//=======================================================
// Scientist
//=======================================================

class CScientist : public CTalkMonster
{
public:
	void Spawn( void );
	void Precache( void );

	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	int	ObjectCaps( void ) { return CTalkMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	virtual int FriendNumber( int arrayNumber );
	void SetActivity ( Activity newActivity );
	Activity GetStoppedActivity( void );
	int ISoundMask( void );
	void MonsterThink( void );

	float	CoverRadius( void );		// Need more room for cover because scientists want to get far away!
	BOOL	DisregardEnemy( CBaseEntity *pEnemy );

	void	Scream( void );

	// Override these to set behavior
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );
	MONSTERSTATE GetIdealState ( void );

	void DeathSound( void );
	void PainSound( void );

	void TalkInit( void );

	void			Killed( entvars_t *pevAttacker, int iGib );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES;

	float m_painTime;
	float m_healTime;
	float m_fearTime;
};

//=======================================================
// Brave scientist (monster_scientist_brave; class name
// ours, after POV's CBrvWorker)
//=======================================================

class CBrvScientist : public CScientist
{
public:
	void Spawn( void );
	void Precache( void );
	void TalkInit( void );

	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	void SetActivity ( Activity NewActivity );
	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	void AlertSound( void );
	void SetAnswerQuestion( CTalkMonster *pSpeaker );

	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );
	MONSTERSTATE GetIdealState ( void );

	CBaseEntity *Kick( void );
	void AttackMissSound( void );
	void AttackHitSound( void );

	CUSTOM_SCHEDULES;
};

#endif		//SCIENTIST_H
