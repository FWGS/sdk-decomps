//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: client particle systems: TriAPI sprite particles streaming from an
//          entity (the flamethrower's flames)
//
// $NoKeywords: $
//=============================================================================

#ifndef PARTICLESYS_H
#define PARTICLESYS_H

#define MAX_PARTICLES	256

struct flame_t	// name ours
{
	bool	active;
	Vector	origin;
	Vector	velocity;
	float	age;		// seconds; dies at 3
	float	frame;
	float	size;
};

class CParticleSys
{
public:
	CParticleSys();
	~CParticleSys();

	void Draw( void );
	void Think( float flFrametime, float flTime );
	flame_t *AddNewParticle( Vector &vecOrigin, Vector &vecVelocity );	// name ours
	Vector CalcMuzzlePos( const Vector &vecOffset );	// name ours
	Vector GetMuzzleVel( const Vector &vecAngles );	// name ours

	Vector		m_vecOffset;	// spawn point: forward, right, up of the owner's view
	Vector		m_vecAngles;	// spawn direction, added to the owner's view angles
	float		m_flSpeed;
	int			m_iType;		// 1: the particles fall
	int			m_iEntity;		// owner; -1 once it is gone
	char		*m_pszSprite;
	float		m_flDieTime;	// stop spawning (0: never); removed once its particles are gone
	float		m_flScale;		// particle size at birth
	Vector		m_vecColor;
	flame_t	m_Particles[ MAX_PARTICLES ];
	struct model_s *m_pSprite;
	int			m_iFrames;
	int			m_iLastParticle;
	float		m_flSpawn;		// particles to spawn, 50 a second
	CParticleSys	*m_pNext;
	CParticleSys	*m_pPrev;
};

class CParticleStruct
{
public:
	CParticleStruct();
	~CParticleStruct();

	void Delete( CParticleSys *pSystem );
	void Reset( void );
	CParticleSys *Create( int iEntity );
	void Think( float flFrametime, float flTime );
	void Draw( void );
	void Stop( int iEntity );

	CParticleSys	*m_pFirst;
	CParticleSys	*m_pLast;
};

extern CParticleStruct g_ParticleSystems;

#endif // PARTICLESYS_H
