//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: client particle systems: TriAPI sprite particles streaming from an
//          entity (the flamethrower's flames); names ours
//
// $NoKeywords: $
//=============================================================================

#ifndef PARTICLESYS_H
#define PARTICLESYS_H

#define MAX_PARTICLES	256

struct particle_t
{
	bool	active;
	Vector	origin;
	Vector	velocity;
	float	age;		// seconds; dies at 3
	float	frame;
	float	size;
};

class CParticleSystem
{
public:
	CParticleSystem();
	~CParticleSystem();

	void Draw( void );
	void Update( float flFrametime, float flTime );
	particle_t *AddParticle( const Vector &vecOrigin, const Vector &vecVelocity );
	Vector GetOrigin( const Vector &vecOffset );
	Vector GetVelocity( const Vector &vecAngles );

	Vector		m_vecOffset;	// spawn point: forward, right, up of the owner's view
	Vector		m_vecAngles;	// spawn direction, added to the owner's view angles
	float		m_flSpeed;
	int			m_iType;		// 1: the particles fall
	int			m_iEntity;		// owner; -1 once it is gone
	char		*m_pszSprite;
	float		m_flDieTime;	// stop spawning (0: never); removed once its particles are gone
	float		m_flScale;		// particle size at birth
	Vector		m_vecColor;
	particle_t	m_Particles[ MAX_PARTICLES ];
	struct model_s *m_pSprite;
	int			m_iFrames;
	int			m_iLastParticle;
	float		m_flSpawn;		// particles to spawn, 50 a second
	CParticleSystem	*m_pNext;
	CParticleSystem	*m_pPrev;
};

class CParticleSystemManager
{
public:
	CParticleSystemManager();
	~CParticleSystemManager();

	void Remove( CParticleSystem *pSystem );
	void Clear( void );
	CParticleSystem *Create( int iEntity );
	void Update( float flFrametime, float flTime );
	void Draw( void );
	void Stop( int iEntity );

	CParticleSystem	*m_pFirst;
	CParticleSystem	*m_pLast;
};

extern CParticleSystemManager g_ParticleSystems;

#endif // PARTICLESYS_H
