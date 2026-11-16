/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
// Hornets
//=========================================================

//=========================================================
// Hornet Defines
//=========================================================
// The hornet gun's six hornets, chosen by its secondary attack (the gun's and the hornet's
// pev->weapons, read by StartHornet; Spawn sets 0). Names ours.
#define HORNET_TYPE_NORMAL		0	// red trail, tracks
#define HORNET_TYPE_DART		1	// green, straight through what it hits
#define HORNET_TYPE_BOUNCE		2	// blue, bounces off the world
#define HORNET_TYPE_HEADSHOT	3	// yellow, goes for scientists' heads
#define HORNET_TYPE_FIRE		4	// orange, burns
#define HORNET_TYPE_ICE			5	// light blue, freezes
#define	HORNET_BUZZ_VOLUME		(float)0.8

extern int iHornetPuff;

//=========================================================
// Hornet - this is the projectile that the Alien Grunt fires.
//=========================================================
class CHornet : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int	 Classify ( void );
	int  IRelationship ( CBaseEntity *pTarget );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int HeatSignature( void ) { return 0; }
	const char *GetKillDesc( void ) { return "an unknown alien bioweapon"; }

	void IgniteTrail( void );
	void EXPORT StartHornet ( void );
	void EXPORT TrackTarget ( void );
	void EXPORT TrackTouch ( CBaseEntity *pOther );
	void EXPORT HeadshotTouch( CBaseEntity *pOther );
	void EXPORT DartTouch( CBaseEntity *pOther );
	void EXPORT BounceTouch( CBaseEntity *pOther );
	void EXPORT DieTouch ( CBaseEntity *pOther );
	void EXPORT HeadBlastThink( void );
	void EXPORT FlyThink( void );
	void Expire( void );	// name ours
	void EXPORT DeadTouch( CBaseEntity *pOther );
	
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	float			m_flStopAttack;
	int				m_iHornetType;
	float			m_flFlySpeed;
};

