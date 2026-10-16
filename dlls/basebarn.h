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
#ifndef BASEBARN_H
#define BASEBARN_H

//=========================================================
// POV: base class of the armed humans (barney, barniel,
// kate, gordon, the HEV family). Built from the SDK's
// barney and the human grunt's weapons. File name and
// the names retail doesn't give are ours.
//=========================================================

#ifndef TALKMONSTER_H
#include "talkmonster.h"
#endif

// m_iWeaponUsed ("weaponused" key)
#define BARN_WEAPON_GLOCK		0
#define BARN_WEAPON_PYTHON		1
#define BARN_WEAPON_MP5			2
#define BARN_WEAPON_SHOTGUN		3

#define	BARN_BODY_GUNHOLSTERED	0
#define	BARN_BODY_GLOCK			1
#define	BARN_BODY_PYTHON		2
#define	BARN_BODY_MP5			3
#define	BARN_BODY_SHOTGUN		4
#define BARN_BODY_GUNGONE		5

enum
{
	SCHED_BASEBARN_HIDE_RELOAD = LAST_COMMON_SCHEDULE + 1,
};

class CBaseBarn : public CTalkMonster
{
public:
	virtual void FireWeapon( BOOL fSound );			// slot 127
	virtual int GetHead( void ) { return 0; }		// slot 128
	virtual void SetGun( int iGun );				// slot 129
	virtual BOOL HasGun( void );					// slot 130
	virtual void SetupWeapon( void );				// slot 131
	virtual void WeaponSound( void );				// slot 132
	virtual void ReloadSound( void );				// slot 133

	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void SetYawSpeed( void );
	int  ISoundMask( void );
	void AlertSound( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int IRelationship( CBaseEntity *pTarget ) { return CTalkMonster::IRelationship( pTarget ); }

	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	void CheckAmmo ( void );
	Vector GetGunPosition( void );
	void SetActivity ( Activity NewActivity );
	CBaseEntity *Kick( void );

	void FireGlock( BOOL fSound );
	void FirePython( BOOL fSound );
	void FireMP5( BOOL fSound );
	void FireShotgun( BOOL fSound );

	// Override these to set behavior
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );
	MONSTERSTATE GetIdealState ( void );

	void DeathSound( void );
	void PainSound( void );

	void TalkInit( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Killed( entvars_t *pevAttacker, int iGib );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL	m_fGunDrawn;
	float	m_painTime;
	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;
	BOOL	m_fUsesMeleeAttack;
	int		m_KarateMoveNum;
	int		m_iWeaponUsed;
	int		m_cClipSize;
	BOOL	m_fStanding;
	int		m_iBrassShell;
	int		m_iShotgunShell;

	// UNDONE: What is this for?  It isn't used?
	float	m_flPlayerDamage;// how much pain has the player inflicted on me?

	CUSTOM_SCHEDULES;
};

#endif // BASEBARN_H
