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
// monster_tube: the tube queen pulls tubes out of her belly
//=========================================================
#ifndef TUBE_H
#define TUBE_H

class CTube : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void PrecacheSounds( void );
	int  Classify( void );
	int  MonsterType( void );
	int  ISoundMask( void );
	void SetYawSpeed( void );
	void SetObjectCollisionBox( void );
	void GibMonster( void );
	void PrescheduleThink( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void RunAI( void );
	int  TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	BOOL FCanCheckAttacks( void );
	BOOL CheckRangeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack2( float flDot, float flDist );
	void StartTask( Task_t *pTask );
	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType( int Type );
	void IdleSound( void );
	void AlertSound( void );

	void StopTalking( void );
	BOOL ShouldSpeak( void );
	CBaseMonster *ShootRocket( void );
	int  CheckMentalHealth( void );
	void ChangeMentalHealth( int iChange );

	int	Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES;

	static const char *pTubeSounds[];

	int		m_iMentalHealth;		// below zero, the tube blows itself up
	int		m_iResidual;			// mental health still to gain (or lose) a point per think
	float	m_fInstability;			// how hard each hit takes the mental health down
	BOOL	m_fCanRocketAttack;
	float	m_tNextRocketAttackCheck;
	int		m_iModelIndexTubeSuicide;
	float	m_tNextAttack2Time;
	int		m_iVoicePitch;
	float	m_tNextSpeakTime;
	float	m_tNextWordTime;
};

#endif // TUBE_H
