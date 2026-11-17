//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: client particle systems: TriAPI sprite particles streaming from an
//          entity (the flamethrower's flames)
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "event_api.h"
#include "pm_defs.h"
#include "pmtrace.h"
#include "eventscripts.h"
#include "particlesys.h"

bool EV_IsEntityVisible( cl_entity_t *pEntity );

float g_flGravity;	// sv_gravity, from HUD_TempEntUpdate

Vector g_vecParticleZero( 0, 0, 0 );

CParticleSysManager g_ParticleSystems;

CParticleSys::CParticleSys()
{
	memset( this, 0, sizeof( *this ) );
}

CParticleSys::~CParticleSys()
{
	delete m_pNext;
	m_pNext = NULL;
}

Vector g_vecParticleForward, g_vecParticleRight, g_vecParticleUp;

//=========================================================
// Draw the particles as camera-facing quads, then the
// next system's
//=========================================================
void CParticleSys::Draw( void )
{
	Vector vecCorner;

	if ( !m_pSprite )
	{
		HSPRITE hSprite = SPR_Load( m_pszSprite );
		m_pSprite = (struct model_s *)gEngfuncs.GetSpritePointer( hSprite );
		m_iFrames = SPR_Frames( hSprite ) - 1;
	}

	gEngfuncs.pTriAPI->Brightness( 1 );

	for ( int i = 0; i < MAX_PARTICLES; i++ )
	{
		if ( !m_Particles[i].active )
			continue;

		float flAlpha = 1;
		if ( m_Particles[i].age > 1 )
			flAlpha = 1.0 - ( m_Particles[i].age - 1 ) / 2 * 0.8;

		gEngfuncs.pTriAPI->Color4f( m_vecColor.x, m_vecColor.y, m_vecColor.z, flAlpha );
		gEngfuncs.pTriAPI->SpriteTexture( m_pSprite, (int)m_Particles[i].frame );
		gEngfuncs.pTriAPI->Begin( TRI_QUADS );

		gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
		VectorMA( m_Particles[i].origin, m_Particles[i].size, g_vecParticleUp, vecCorner );
		VectorMA( vecCorner, -m_Particles[i].size, g_vecParticleRight, vecCorner );
		gEngfuncs.pTriAPI->Vertex3fv( vecCorner );

		gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
		VectorMA( m_Particles[i].origin, m_Particles[i].size, g_vecParticleUp, vecCorner );
		VectorMA( vecCorner, m_Particles[i].size, g_vecParticleRight, vecCorner );
		gEngfuncs.pTriAPI->Vertex3fv( vecCorner );

		gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
		VectorMA( m_Particles[i].origin, -m_Particles[i].size, g_vecParticleUp, vecCorner );
		VectorMA( vecCorner, m_Particles[i].size, g_vecParticleRight, vecCorner );
		gEngfuncs.pTriAPI->Vertex3fv( vecCorner );

		gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
		VectorMA( m_Particles[i].origin, -m_Particles[i].size, g_vecParticleUp, vecCorner );
		VectorMA( vecCorner, -m_Particles[i].size, g_vecParticleRight, vecCorner );
		gEngfuncs.pTriAPI->Vertex3fv( vecCorner );

		gEngfuncs.pTriAPI->End();
	}

	if ( m_pNext )
		m_pNext->Draw();
}

//=========================================================
// Move the particles, then the next system's; spawn new
// ones while the owner is in view
//=========================================================
void CParticleSys::Think( float flFrametime, float flTime )
{
	bool bAlive = false;

	if ( !m_pSprite )
	{
		HSPRITE hSprite = SPR_Load( m_pszSprite );
		m_pSprite = (struct model_s *)gEngfuncs.GetSpritePointer( hSprite );
		m_iFrames = SPR_Frames( hSprite ) - 1;
	}

	gEngfuncs.pEventAPI->EV_SetSolidPlayers( m_iEntity - 1 );
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );

	for ( int i = 0; i < MAX_PARTICLES; i++ )
	{
		if ( !m_Particles[i].active )
			continue;

		m_Particles[i].age += flFrametime;

		if ( m_Particles[i].age > 3 )
		{
			m_Particles[i].active = false;
			continue;
		}

		if ( m_Particles[i].age > 1 )
		{
			m_Particles[i].size = ( ( m_Particles[i].age - 1 ) * 2 + 1 ) * m_flScale;
			m_Particles[i].velocity = m_Particles[i].velocity * ( 1 - flFrametime * 2 );
		}

		bAlive = true;
		m_Particles[i].frame = m_iFrames * m_Particles[i].age / 3;

		if ( m_Particles[i].velocity == g_vecParticleZero )
			continue;

		if ( m_iType == 1 )
			m_Particles[i].velocity.z -= g_flGravity * flFrametime * 0.1;

		pmtrace_t tr;
		Vector vecEnd = m_Particles[i].origin + m_Particles[i].velocity * flFrametime;

		gEngfuncs.pEventAPI->EV_PlayerTrace( m_Particles[i].origin, vecEnd, PM_STUDIO_BOX, -1, &tr );

		if ( tr.fraction < 1 )
		{
			m_Particles[i].origin = tr.endpos + tr.plane.normal * 8;
			m_Particles[i].velocity = g_vecParticleZero;
			m_Particles[i].age = max( m_Particles[i].age, 1 );
		}
		else
		{
			m_Particles[i].origin = vecEnd;
		}
	}

	if ( m_pNext )
		m_pNext->Think( flFrametime, flTime );

	if ( m_flDieTime != 0 && m_flDieTime <= flTime )
	{
		if ( !bAlive )
			g_ParticleSystems.Remove( this );
		return;
	}

	if ( m_iEntity && !EV_IsEntityVisible( gEngfuncs.GetEntityByIndex( m_iEntity ) ) )
		return;

	m_flSpawn += flFrametime * 50;
	if ( m_flSpawn >= 1 )
	{
		AddParticle( GetOrigin( m_vecOffset ), GetVelocity( m_vecAngles ) );
		m_flSpawn -= 1;
	}
}

particle_t *CParticleSys::AddParticle( const Vector &vecOrigin, const Vector &vecVelocity )
{
	int i = m_iLastParticle + 1;	// sic: 256 when the last one was 255

	while ( i != m_iLastParticle )
	{
		if ( !m_Particles[i].active )
		{
			m_iLastParticle = i;
			m_Particles[i].active = true;
			m_Particles[i].origin = vecOrigin;
			m_Particles[i].velocity = vecVelocity;
			m_Particles[i].age = 0;
			m_Particles[i].size = m_flScale;
			return &m_Particles[i];
		}

		i++;
		if ( i >= MAX_PARTICLES )
			i = 0;
	}

	return NULL;
}

//=========================================================
// The spawn point: m_vecOffset along the owner's view
// (a player's eyes), or the offset itself without an owner
//=========================================================
Vector CParticleSys::GetOrigin( const Vector &vecOffset )
{
	if ( m_iEntity < 0 )
		return vecOffset;

	cl_entity_t *pEnt = gEngfuncs.GetEntityByIndex( m_iEntity );

	if ( !pEnt || !pEnt->curstate.modelindex )
	{
		m_iEntity = -1;
		return vecOffset;
	}

	Vector vecAngles;

	if ( EV_IsLocal( m_iEntity ) )
		gEngfuncs.GetViewAngles( vecAngles );
	else if ( pEnt->player )
		vecAngles = pEnt->curstate.vuser1;
	else if ( pEnt->curstate.modelindex )
		vecAngles = pEnt->angles;

	vecAngles.x = -vecAngles.x;

	Vector vecForward, vecRight, vecUp;
	gEngfuncs.pfnAngleVectors( vecAngles, vecForward, vecRight, vecUp );

	Vector vecOrigin = pEnt->origin;

	if ( pEnt->player )
	{
		if ( pEnt->curstate.usehull == 1 )
			vecOrigin.z += 12;
		else
			vecOrigin.z += 28;
	}

	return vecOrigin + vecForward * vecOffset.x + vecRight * vecOffset.y + vecUp * vecOffset.z;
}

//=========================================================
// The spawn velocity: m_flSpeed along the owner's view
// turned by vecAngles
//=========================================================
Vector CParticleSys::GetVelocity( const Vector &vecDir )
{
	Vector vecAngles = vecDir;

	if ( EV_IsLocal( m_iEntity ) )
	{
		gEngfuncs.GetViewAngles( vecAngles );
		vecAngles = vecDir + vecAngles;
	}
	else if ( m_iEntity > 0 )
	{
		cl_entity_t *pEnt = gEngfuncs.GetEntityByIndex( m_iEntity );

		if ( pEnt )
		{
			if ( pEnt->player )
				vecAngles = pEnt->curstate.vuser1 + vecAngles;
			else if ( pEnt->curstate.modelindex )
				vecAngles = pEnt->angles + vecAngles;
		}
	}

	for ( int i = 0; i < 3; i++ )
	{
		if ( vecAngles[i] > 180.0 )
			vecAngles[i] -= 360.0;
		else if ( vecAngles[i] < -180.0 )
			vecAngles[i] += 360.0;
	}

	Vector vecForward, vecRight, vecUp;
	gEngfuncs.pfnAngleVectors( vecAngles, vecForward, vecRight, vecUp );

	return vecForward * m_flSpeed;
}

void CParticleSysManager::Remove( CParticleSys *pSystem )
{
	if ( !pSystem )
		return;

	if ( pSystem == m_pFirst )
		m_pFirst = pSystem->m_pNext;
	else
		pSystem->m_pPrev->m_pNext = pSystem->m_pNext;

	if ( pSystem == m_pLast )
		m_pLast = pSystem->m_pPrev;
	else
		pSystem->m_pNext->m_pPrev = pSystem->m_pPrev;

	pSystem->m_pPrev = NULL;
	pSystem->m_pNext = NULL;
	delete pSystem;
}

CParticleSysManager::CParticleSysManager()
{
	memset( this, 0, sizeof( *this ) );
}

CParticleSysManager::~CParticleSysManager()
{
	Clear();
}

void CParticleSysManager::Clear( void )
{
	delete m_pFirst;
	m_pFirst = m_pLast = NULL;
}

CParticleSys *CParticleSysManager::Create( int iEntity )
{
	CParticleSys *pSystem = new CParticleSys;

	if ( !pSystem )
	{
		gEngfuncs.Con_Printf( "ERROR: Out of memory creating particle system\n" );
		return NULL;
	}

	pSystem->m_iEntity = iEntity;

	if ( !m_pFirst )
	{
		m_pFirst = m_pLast = pSystem;
		return pSystem;
	}

	m_pLast->m_pNext = pSystem;
	pSystem->m_pPrev = m_pLast;
	m_pLast = pSystem;
	return pSystem;
}

void CParticleSysManager::Update( float flFrametime, float flTime )
{
	if ( m_pFirst )
		m_pFirst->Think( flFrametime, flTime );
}

void CParticleSysManager::Draw( void )
{
	if ( !m_pFirst )
		return;

	Vector vecAngles;
	gEngfuncs.GetViewAngles( vecAngles );
	gEngfuncs.pfnAngleVectors( vecAngles, g_vecParticleForward, g_vecParticleRight, g_vecParticleUp );

	gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
	m_pFirst->Draw();
}

//=========================================================
// The owner stopped firing: its systems spawn no more
//=========================================================
void CParticleSysManager::Stop( int iEntity )
{
	if ( !m_pFirst || iEntity < 0 )
		return;

	for ( CParticleSys *pSystem = m_pFirst; pSystem; pSystem = pSystem->m_pNext )
	{
		if ( pSystem->m_iEntity == iEntity )
			pSystem->m_flDieTime = -1;
	}
}

void RenderParticles( void )
{
	g_ParticleSystems.Draw();
}

void ThinkParticles( float flFrametime, float flTime )
{
	g_ParticleSystems.Update( flFrametime, flTime );
}

void ParticleVidInit( void )
{
	g_ParticleSystems.Clear();
}
