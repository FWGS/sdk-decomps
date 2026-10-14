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
// CMaggot - the maggot. The critter spits maggots at its
// enemies, so it needs the class too. Include
// squadmonster.h first.
//=========================================================

#ifndef MAGGOT_H
#define MAGGOT_H

class CMaggot : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void PrecacheSounds( void );
	int  Classify( void );
	int  MonsterType( void );
	void SetYawSpeed( void );
	Vector Center( void );
	int  TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void GibMonster( void );
	void RunAI( void );
	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );
	void PrescheduleThink( void );
	BOOL CheckMeleeAttack1( float flDot, float flDist );
	Schedule_t *GetScheduleOfType( int Type );
	void IdleSound( void );
	void AlertSound( void );
	void PainSound( void );
	void DeathSound( void );
	void GibSound( void );
	void AttackHitSound( CBaseMonster *pVictim );

	// the critter spits the maggot: it flies as a larva and
	// dies by itself after a while
	void Launch( void );

	void EXPORT LaunchedTouch( CBaseEntity *pOther );
	void EXPORT SquashTouch( CBaseEntity *pOther );
	void EXPORT LeapTouch( CBaseEntity *pOther );

	int	Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES;

	static const char *pMaggotSounds[];

	float	m_flNextDodgeTime;		// when the maggot may jump aside again
	BOOL	m_bCanDodge;			// half of the maggots jump aside when their enemy looks at them
	int		m_iCritterKillTime;		// thinks until a launched maggot dies; -1 when not launched
	BOOL	m_fLaunched;			// in flight from the critter, trailing slime
	int		m_iSplashSprite;
};

#endif // MAGGOT_H
