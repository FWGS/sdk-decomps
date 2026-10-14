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
// Gunman's drivable tank: a studio model entity that drives
// and three brush parts (body, turret, barrel) that follow it
//=========================================================
#ifndef VEHICLETANK_H
#define VEHICLETANK_H

class CBasePlayer;
class CVehicleTankBSP;

// tank.mdl's sequences
enum tank_e {
	TANK_OFFLINE = 0,
	TANK_ENGINE,
	TANK_BUMPFRONT,
	TANK_BUMPBACK,
	TANK_BUMPLEFT,
	TANK_BUMPRIGHT,
	TANK_BUMPFRONTLEFT,
	TANK_BUMPFRONTRIGHT,
	TANK_BUMPBACKLEFT,
	TANK_BUMPBACKRIGHT,
	TANK_MINGUNS,
	TANK_TURRET,
};

// m_iGasState
#define TANK_GAS_EMPTY		0	// out of gas
#define TANK_GAS_NONE		1	// treated as empty; nothing sets it
#define TANK_GAS_FULL		2
#define TANK_GAS_REFUEL		3	// refuelled, starts up on the next think

#define SF_TANK_NOCANNONS	1	// the shell and rocket cannons don't fire
#define SF_TANK_NOMGUNS		2	// the machine guns don't fire

//=========================================================
// What the tank and its brush parts share: the driver, the
// vehicle_id that links them, and where they last fitted.
//=========================================================
class CVehicle : public CBaseMonster
{
public:
	void KeyValue( KeyValueData *pkvd );
	int Classify( void ) { return CLASS_PLAYER_ALLY; }

	CBasePlayer *m_pPlayer;		// the driver
	int		m_iVehicleID;
	Vector	m_vecLastValidPos;
	Vector	m_vecLastValidAng;
	float	m_fTankSoundAdj;	// volume of the tank's sounds
	int		m_iTankHealth;		// health in multiplayer
};

//=========================================================
// vehicle_tank
//=========================================================
class CVehicleTank : public CVehicle
{
public:
	CVehicleTank( void );

	void Spawn( void );
	void Precache( void );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int ObjectCaps( void ) { return FCAP_ACROSS_TRANSITION | FCAP_ONOFF_USE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	virtual int VehicleLength( void ) { return m_iVehicleLength; }
	virtual int VehicleWidth( void ) { return m_iVehicleWidth; }
	virtual void HullCheck( void );

	void EXPORT BuildVehicleThink( void );
	void EXPORT WithoutPlayerThink( void );
	void EXPORT WithPlayerThink( void );
	void EXPORT UseVehicle( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int TankControls( void );
	void SetTankAnim( int iAnim );
	void MoveTank( float flInterval );
	void Drive( float flInterval );
	int UpdatePlayer( float flInterval );
	void UpdateControllers( void );
	void SetAvelocity( Vector vecAvelocity );
	void SetVelocity( Vector vecVelocity );
	void SetVelocityX( float x );
	void SetVelocityY( float y );
	void SetVelocityZ( float z );
	int SideBlocked( int iStartForward, int iEndForward, int iStartRight, int iEndRight );
	int SidesBlocked( void );
	int CornerBlocked( int iStartForward, int iEndForward, int iStartRight, int iEndRight, Vector vecVelocity, float flInterval, float flDist );
	BOOL CornersBlocked( Vector vecVelocity, float flInterval, float flDist );
	Vector PushAway( void );
	Vector Slide( Vector vecVelocity, Vector vecNormal );
	Vector GroundAngles( void );
	Vector BodyAvelocity( float flYawDiff, Vector vecGround );
	Vector TurretAvelocity( float flYawDiff, Vector vecGround );
	void RestoreLastValid( void );
	void SaveLastValid( void );
	int CanExit( void );
	void EjectPlayer( void );
	void Debris( Vector vecPos );
	void FireShell( void );
	void FireRocket( void );
	void FireMachineGuns( void );

	CVehicleTankBSP *m_pBarrelBSP;
	CVehicleTankBSP *m_pTurretBSP;
	CVehicleTankBSP *m_pBodyBSP;
	float	m_flSpeed;
	float	m_fYawAccel;
	float	m_flTimeHolder;
	BOOL	m_bCollisionLastFrame;
	Vector	m_vecNormalLastFrame;
	float	m_flAccelTime;
	float	m_fNextShellAttack;
	float	m_fNextRocketAttack;
	float	m_fNextMGunAttack;
	Vector	m_vecCollision;		// where a corner of the tank hit something
	float	m_flNextRamSound;
	float	m_flLastSpeed;
	int		m_iVehicleLength;
	int		m_iVehicleWidth;
	int		m_iGasState;
	float	m_sTurretHUDPos;
	BOOL	m_bCanBounce;
	float	m_flNextBounceMove;
	int		m_iTankMsgs;		// Tank messages sent since the driver got in
	float	m_flNextOjectionDeniedSound;
	float	m_flNextDebris;
	int		m_iRockGibs;
	int		m_iMetalGibs;
	int		m_iWoodGibs;
	BOOL	m_bFirstTimeInTank;
	float	m_flNextHullCheck;
	int		m_iMGunSide;		// which machine gun fires next
	BOOL	m_bStopOnThink;		// stop the tank on the next think
	unsigned short m_usTankMGuns;
};

//=========================================================
// vehicle_tank_barrel, vehicle_tank_turret, vehicle_tank_body
//=========================================================
class CVehicleTankBSP : public CVehicle
{
public:
	void Spawn( void );
	void Precache( void );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int ObjectCaps( void ) { return FCAP_ONOFF_USE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void Blocked( CBaseEntity *pOther );

	void EXPORT TankBSPUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT WaitThink( void );

	BOOL	m_bBlocked;		// something blocked this part this frame
};

// brings an angle into (-180, 180]
extern float UTIL_AngleNormalize( float flAngle );

#endif // VEHICLETANK_H
