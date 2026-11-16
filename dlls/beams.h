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
#ifndef BEAMS_H
#define BEAMS_H

// Scientist Slaughterhouse's beam_cannon (defined in beams.cpp). Declared here because the crossbow's
// laser net and func_turret create beam cannons. The fields are the save table's (the unsaved one at
// 0x70 is unknown); the other names are ours.
class CBeamCannon : public CBeam
{
public:
	static CBeamCannon *CannonCreate( int iszModel, int iszStartSprite, int iszEndSprite );	// 0x10013870
	void	TurnOn( void );		// 0x10012780

	CSprite	*m_pStartSprite;
	CSprite	*m_pEndSprite;
	int		m_iszStartSpriteName;
	float	m_flStartSpriteScale;
	float	m_flEndSpriteScale;
	int		m_iszEndSpriteName;
	int		m_iUnknown;			// 0x70, not saved
	Vector	m_firePosition;
	float	m_scale;
	float	m_flicker;
	int		m_firstShot;
	int		m_damageType;
	float	m_radius;
	int		m_endType;
	float	m_endValue;
	Vector	m_startLight;
	Vector	m_endLight;
	float	m_expireTime;
	int		m_maxFrame;
	float	m_lastTime;
};

#endif // BEAMS_H
