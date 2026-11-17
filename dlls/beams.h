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

// Scientist Slaughterhouse's beam_cannon and beam_charge (defined in beams.cpp). Declared here because the
// crossbow's laser net and func_turret create and drive them. Field names are the save tables'; the other
// names are ours.

// beam_cannon spawnflags
#define SF_CANNON_STARTON		0x0001
#define SF_CANNON_POINT2POINT	0x0002	// render up to the target even if something is in between
#define SF_CANNON_DIRECTIONAL	0x0004	// go on past the target until something is hit
#define SF_CANNON_SPIRAL		0x0008
#define SF_CANNON_DECALS		0x0040
#define SF_CANNON_GLOWSPRITES	0x0080
#define SF_CANNON_ADDSPRITES	0x0100
#define SF_CANNON_SOLID			0x0200
#define SF_CANNON_IGNOREGLASS	0x0400
#define SF_CANNON_MIMIC			0x0800	// copy another beam_cannon's beam and on/off state

// beam_cannon end effects (m_endType)
#define CANNON_END_NONE			0
#define CANNON_END_EXPLOSION	1	// one explosion when the beam comes on
#define CANNON_END_EXPLOSIONS	2	// an explosion every m_endValue seconds
#define CANNON_END_SPRITES		3	// a trail of end sprites
#define CANNON_END_TORUS		4
#define CANNON_END_DISK			5
#define CANNON_END_CYLINDER		6
#define CANNON_END_PARTICLES	7
#define CANNON_END_BLOOD		8
#define CANNON_END_ALIENBLOOD	9
#define CANNON_END_RESERVED		10

class CBeamCannon : public CBeam
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	KeyValue( KeyValueData *pkvd );
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return DamageVerb( m_damageType, fGibbed ); }
	void	Killed( entvars_t *pevAttacker, int iGib )
	{
		if ( m_pStartSprite )
			UTIL_Remove( m_pStartSprite );
		if ( m_pEndSprite )
			UTIL_Remove( m_pEndSprite );
		m_pEndSprite = NULL;
		m_pStartSprite = NULL;
		CBeam::Killed( pevAttacker, iGib );
	}

	int		IsOn( void );
	void	TurnOffBeam( void );	// the beam and end sprite off, the start effects stay
	void	TurnOnBeam( void );
	void	TurnOff( void );
	void	TurnOn( void );
	void	Explode( const Vector &vecPos );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	EXPORT StartFxThink( void );
	void	FireAtPoint( TraceResult &tr );
	void	BeamDamage( TraceResult *ptr );
	void	EXPORT StrikeThink( void );
	void	EXPORT MatchThink( void );

	static CBeamCannon *CannonCreate( int iszModel, int iszStartSprite, int iszEndSprite );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	CSprite	*m_pStartSprite;
	CSprite	*m_pEndSprite;
	int		m_iszStartSpriteName;
	float	m_flStartSpriteScale;
	float	m_flEndSpriteScale;
	int		m_iszEndSpriteName;
	int		m_iEndSpriteIndex;	// not saved
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

// beam_charge spawnflags
#define SF_BEAMCHARGE_STARTON		0x0001
#define SF_BEAMCHARGE_EXPANDFADE	0x0004	// grow while fading out
#define SF_BEAMCHARGE_PARTICLES		0x0008

class CBeamCharge : public CSprite
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	KeyValue( KeyValueData *pkvd );
	void	Expand( float scaleSpeed, float fadeSpeed );
	void	Charge( void );
	void	EXPORT ExpandThink( void );
	void	EXPORT ChargeThink( void );
	void	RandomBeam( void );
	void	CoolDown( void );
	void	WarmUp( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int		m_on;
	float	m_beamSlack;
	float	m_scale;
	float	m_fxScale;
	float	m_alpha;
	int		m_iBeamSprite;		// not saved
	int		m_iszBeamSpriteName;
	float	m_flWarmTime;
	float	m_flCoolTime;
	float	m_volume;
	Vector	m_light;			// not saved
};

#endif // BEAMS_H
