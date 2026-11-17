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
// CBaseFlyingObject (flyingobject.cpp): the base of the flying spheres and cubes
// (monster_ddisphere, monster_ddicube). Its flight AI follows CFlyingMonster's, its
// death and crash CApache's. No save table of its own; derived classes must provide
// RunAttack. Names ours but the class name, DyingThink and CrashTouch.

#ifndef FLYINGOBJECT_H
#define FLYINGOBJECT_H

class CBaseFlyingObject : public CBaseMonster
{
public:
	void Spawn( void );			// derived classes' Spawn calls it first, then sets its model
	void Killed( entvars_t *pevAttacker, int iGib );
	void EXPORT DyingThink( void );
	void EXPORT CrashTouch( CBaseEntity *pOther );

	void RunAI( void );
	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );
	void SetYawSpeed( void );
	void ChangePitch( float speed );
	Schedule_t *GetSchedule( void );
	BOOL ShouldAdvanceRoute( float flWaypointDist );
	void Move( float flInterval = 0.1 );
	BOOL FRefreshRoute( void );
	float CeilingZ( const Vector &position );
	float FloorZ( const Vector &position );

	virtual void RunAttack( void ) = 0;	// called by RunAI after GetEnemy

	int m_iSpriteTexture;	// the death blast's ring sprite (TE_BEAMCYLINDER), none if 0
	int m_iBodyGibs;		// the wreck's gib model (TE_BREAKMODEL), none if 0
};

#endif // FLYINGOBJECT_H
