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
// the shells and rockets the tank fires (vehicletank.cpp fires
// them, vehicletankbsp.cpp defines them). The file name is ours.
//=========================================================

#ifndef TANKSHOTS_H
#define TANKSHOTS_H

//=========================================================
// vehicle_tank_shell - the tank's cannon shell
//=========================================================
class CVehicleTankShell : public CGrenade
{
public:
	void Spawn( void );
	void Precache( void );

	static CVehicleTankShell *Create( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner );

	void EXPORT ShellTouch( CBaseEntity *pOther );
	void EXPORT OrientThink( void );
	void EXPORT ShellSmokeThink( void );
	void Debris( TraceResult *pTrace, Vector vecSrc, Vector vecEnd );

	int m_iSmokeTrail;
	int m_iRockGibs;
	int m_iMetalGibs;
	int m_iWoodGibs;
};

//=========================================================
// vehicle_tank_rocket - the tank's homing rocket
//=========================================================
class CVehicleTankRocket : public CGrenade
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify( void ) { return CLASS_PLAYER_ALLY; }

	static CVehicleTankRocket *Create( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, CBaseEntity *pEnemy );

	void EXPORT TankIgniteThink( void );
	void EXPORT RocketTouch( CBaseEntity *pOther );
	void EXPORT RocketSmokeThink( void );
	void EXPORT RocketThink( void );

	int m_iTrail;
	int m_iTrail2;
	CSprite *m_pFlame;
	CSprite *m_pSmoke;
};

#endif // TANKSHOTS_H
