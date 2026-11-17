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
#ifndef EFFECTS_H
#define EFFECTS_H

#define SF_BEAM_STARTON			0x0001
#define SF_BEAM_TOGGLE			0x0002
#define SF_BEAM_RANDOM			0x0004
#define SF_BEAM_RING			0x0008
#define SF_BEAM_SPARKSTART		0x0010
#define SF_BEAM_SPARKEND		0x0020
#define SF_BEAM_DECALS			0x0040
#define SF_BEAM_SHADEIN			0x0080
#define SF_BEAM_SHADEOUT		0x0100
#define SF_BEAM_TEMPORARY		0x8000

#define SF_SPRITE_STARTON		0x0001
#define SF_SPRITE_ONCE			0x0002
#define SF_SPRITE_REVERSE		0x0004	// name ours: the animation runs backwards
#define SF_SPRITE_TEMPORARY		0x8000

class CSprite : public CPointEntity
{
public:
	void Spawn( void );
	void Precache( void );

	int		ObjectCaps( void )
	{ 
		int flags = 0;
		if ( pev->spawnflags & SF_SPRITE_TEMPORARY )
			flags = FCAP_DONT_SAVE;
		return (CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | flags; 
	}
	int		GetOnState( void ) { return !FBitSet( pev->effects, EF_NODRAW ); }
	void EXPORT AnimateThink( void );
	void EXPORT ExpandThink( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Animate( float frames );
	void Expand( float scaleSpeed, float fadeSpeed );
	void SpriteInit( const char *pSpriteName, const Vector &origin );

	inline void SetAttachment( edict_t *pEntity, int attachment )
	{
		if ( pEntity )
		{
			pev->skin = ENTINDEX(pEntity);
			pev->body = attachment;
			pev->aiment = pEntity;
			pev->movetype = MOVETYPE_FOLLOW;
		}
	}
	void TurnOff( void );
	void TurnOn( void );
	inline float Frames( void ) { return m_maxFrame; }
	inline void SetTransparency( int rendermode, int r, int g, int b, int a, int fx )
	{
		pev->rendermode = rendermode;
		pev->rendercolor.x = r;
		pev->rendercolor.y = g;
		pev->rendercolor.z = b;
		pev->renderamt = a;
		pev->renderfx = fx;
	}
	inline void SetTexture( int spriteIndex ) { pev->modelindex = spriteIndex; }
	inline void SetScale( float scale ) { pev->scale = scale; }
	inline void SetColor( int r, int g, int b ) { pev->rendercolor.x = r; pev->rendercolor.y = g; pev->rendercolor.z = b; }
	inline void SetBrightness( int brightness ) { pev->renderamt = brightness; }

	inline void AnimateAndDie( float framerate ) 
	{ 
		SetThink(AnimateUntilDead); 
		pev->framerate = framerate;
		pev->dmgtime = gpGlobals->time + (m_maxFrame / framerate); 
		pev->nextthink = gpGlobals->time; 
	}

	void EXPORT AnimateUntilDead( void );
	void EXPORT TurnOnThink( void );	// name ours

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
	static CSprite *SpriteCreate( const char *pSpriteName, const Vector &origin, BOOL animate );

protected:	// the mod's sprite projectiles set m_maxFrame

	float		m_lastTime;
	float		m_maxFrame;
};


// magic_missile (turret.cpp): a sprite that flies, hurts what it touches and fades out;
// the base of the mod's sprite projectiles (psi_blast ...)
class CMagicMissile : public CSprite
{
public:
	const char *GetKillDesc( void ) { return "a disruptor gun"; }
	void Spawn( void );
	void Precache( void );
	void EXPORT HurtTouch( CBaseEntity *pOther );
};


class CBeam : public CBaseEntity
{
public:
	void	Spawn( void );
	void	Precache( void );
	int		ObjectCaps( void )
	{ 
		int flags = 0;
		if ( pev->spawnflags & SF_BEAM_TEMPORARY )
			flags = FCAP_DONT_SAVE;
		return (CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | flags; 
	}
	const char *GetKillDesc( void ) { if ( !m_iszKillDesc ) return "an energy beam"; return STRING( m_iszKillDesc ); }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return DamageVerb( DMG_ENERGYBEAM, fGibbed ); }

	void EXPORT TriggerTouch( CBaseEntity *pOther );

	// These functions are here to show the way beams are encoded as entities.
	// Encoding beams as entities simplifies their management in the client/server architecture
	inline void	SetType( int type ) { pev->rendermode = (pev->rendermode & 0xF0) | (type&0x0F); }
	inline void	SetFlags( int flags ) { pev->rendermode = (pev->rendermode & 0x0F) | (flags&0xF0); }
	inline void SetStartPos( const Vector& pos ) { pev->origin = pos; }
	inline void SetEndPos( const Vector& pos ) { pev->angles = pos; }
	void SetStartEntity( int entityIndex );
	void SetEndEntity( int entityIndex );

	inline void SetStartAttachment( int attachment ) { pev->sequence = (pev->sequence & 0x0FFF) | ((attachment&0xF)<<12); }
	inline void SetEndAttachment( int attachment ) { pev->skin = (pev->skin & 0x0FFF) | ((attachment&0xF)<<12); }

	inline void SetTexture( int spriteIndex ) { pev->modelindex = spriteIndex; }
	inline void SetWidth( int width ) { pev->scale = width; }
	inline void SetNoise( int amplitude ) { pev->body = amplitude; }
	inline void SetColor( int r, int g, int b ) { pev->rendercolor.x = r; pev->rendercolor.y = g; pev->rendercolor.z = b; }
	inline void SetBrightness( int brightness ) { pev->renderamt = brightness; }
	inline void SetFrame( float frame ) { pev->frame = frame; }
	inline void SetScrollRate( int speed ) { pev->animtime = speed; }

	inline int	GetType( void ) { return pev->rendermode & 0x0F; }
	inline int	GetFlags( void ) { return pev->rendermode & 0xF0; }
	inline int	GetStartEntity( void ) { return pev->sequence & 0xFFF; }
	inline int	GetEndEntity( void ) { return pev->skin & 0xFFF; }

	const Vector &GetStartPos( void );
	const Vector &GetEndPos( void );

	Vector Center( void ) { return (GetStartPos() + GetEndPos()) * 0.5; }; // center point of beam

	inline int  GetTexture( void ) { return pev->modelindex; }
	inline int  GetWidth( void ) { return pev->scale; }
	inline int  GetNoise( void ) { return pev->body; }
	// inline void GetColor( int r, int g, int b ) { pev->rendercolor.x = r; pev->rendercolor.y = g; pev->rendercolor.z = b; }
	inline int  GetBrightness( void ) { return pev->renderamt; }
	inline int  GetFrame( void ) { return pev->frame; }
	inline int  GetScrollRate( void ) { return pev->animtime; }

	// Call after you change start/end positions
	void		RelinkBeam( void );
//	void		SetObjectCollisionBox( void );

	void		DoSparks( const Vector &start, const Vector &end );
	CBaseEntity *RandomTargetname( const char *szName );
	void		BeamDamage( TraceResult *ptr );
	// Init after BeamCreate()
	void		BeamInit( const char *pSpriteName, int width );
	void		PointsInit( const Vector &start, const Vector &end );
	void		PointEntInit( const Vector &start, int endIndex );
	void		EntsInit( int startIndex, int endIndex );
	void		HoseInit( const Vector &start, const Vector &direction );

	static CBeam *BeamCreate( const char *pSpriteName, int width );

	inline void LiveForTime( float time ) { SetThink(SUB_Remove); pev->nextthink = gpGlobals->time + time; }
	inline void	BeamDamageInstant( TraceResult *ptr, float damage ) 
	{ 
		pev->dmg = damage; 
		pev->dmgtime = gpGlobals->time - 1;
		BeamDamage(ptr); 
	}
	int		GetOnState( void ) { return !FBitSet( pev->effects, EF_NODRAW ); }
};


#define SF_MESSAGE_ONCE			0x0001		// Fade in, not out
#define SF_MESSAGE_ALL			0x0002		// Send to all clients


class CLaser : public CBeam
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	KeyValue( KeyValueData *pkvd );

	void	TurnOn( void );
	void	TurnOff( void );
	int		IsOn( void );

	void	FireAtPoint( TraceResult &point );

	void	EXPORT StrikeThink( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	CSprite	*m_pSprite;
	int		m_iszSpriteName;
	Vector  m_firePosition;
};


// The repulsor's tractor beam (CRepulsor::WeaponSpecial creates it and sets m_hTarget).
// pev->playerclass set: it rides on the target while its owner lives; otherwise a beam
// from its origin to the target. The client draws it (events/tractorbeam.sc).
class CTractorBeam : public CBaseEntity
{
public:
	void	Spawn( void );
	void	Precache( void );
	int		ObjectCaps( void )
	{ 
		int flags = 0;
		if ( pev->spawnflags & SF_BEAM_TEMPORARY )
			flags = FCAP_DONT_SAVE;
		return (CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | flags; 
	}
	void	Killed( entvars_t *pevAttacker, int iGib );
	void	EXPORT BeamThink( void );

	EHANDLE	m_hTarget;			// name ours
	int		m_usTractorBeam;	// name ours
};

extern void UpdateFogAndRain( CBaseEntity *pPlayer );	// name ours; env_fog, env_rain for a new player

#endif		//EFFECTS_H
