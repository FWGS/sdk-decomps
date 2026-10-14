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
// func_wind: a brush volume that blows in gusts; leaf_god's
// leaves read its wind vector
//=========================================================
#ifndef FUNCWIND_H
#define FUNCWIND_H

class CFuncWind : public CBaseEntity
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Think( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	Vector	m_Wind;			// the wind blowing right now
	Vector	m_Direction;	// dir_x/y/z are angles; Spawn turns them into a direction
	float	m_fMaxAmp;
	float	m_fMinAmp;
	float	m_fMaxFreq;
	float	m_fMinFreq;
	float	m_fAmp;			// this gust's strength
	float	m_fFreq;		// this gust's frequency
	float	m_fSqrTime;		// saved, never used
	float	m_fSqrScaler;	// saved, never used
	float	theta;			// phase in the gust, 0 to pi
};

#endif // FUNCWIND_H
