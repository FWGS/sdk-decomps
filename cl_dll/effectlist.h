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
//
// effectlist.h
//
// client-side effects that the events start and tri.cpp draws
//

#ifndef EFFECTLIST_H
#define EFFECTLIST_H

// what tri.cpp draws for an effect
#define EFFECT_SHOCK		0	// gauss shock sprite
#define EFFECT_SPHERE		1	// growing sphere
#define EFFECT_FLAME		2	// flames, beams and smoke
#define EFFECT_FLAME2		3
#define EFFECT_FLAME3		4
#define EFFECT_RING			5	// growing ring
#define EFFECT_SPRITE		6	// animated sprite
#define EFFECT_BEAM			7
#define EFFECT_8			8

class CEffect
{
public:
	CEffect( void )
	{
		m_flScale = 1.0;
		m_flMaxScale = 0;
		m_flScaleSpeed = 0;
		m_iId = 0;
		m_iType = 0;
	}

	CEffect( float flMaxScale, Vector vecOrigin, Vector vecAngles, float flScaleSpeed, int iType, float flBrightness, float flFadeSpeed, float flStartTime, struct model_s *pSprite )
	{
		m_flScale = 1.0;
		m_flMaxScale = flMaxScale;
		m_flScaleSpeed = flScaleSpeed;
		m_iId = 0;
		m_iType = iType;
		m_flBrightness = flBrightness;
		m_flFadeSpeed = flFadeSpeed;
		m_flStartTime = flStartTime;
		m_pSprite = pSprite;
		for ( int i = 0; i < 3; i++ )
		{
			m_vecOrigin[i] = vecOrigin[i];
			m_vecAngles[i] = vecAngles[i];
		}
	}

	CEffect( float flMaxScale, Vector vecOrigin, Vector vecAngles, float flScaleSpeed, int iType, float flBrightness, float flFadeSpeed, int r, int g, int b, float flStartTime, struct model_s *pSprite )
	{
		m_flScale = 1.0;
		m_flMaxScale = flMaxScale;
		m_flScaleSpeed = flScaleSpeed;
		m_iId = 0;
		m_iType = iType;
		m_flBrightness = flBrightness;
		m_flFadeSpeed = flFadeSpeed;
		m_flStartTime = flStartTime;
		m_pSprite = pSprite;
		m_iRed = r;
		m_iGreen = g;
		m_iBlue = b;
		for ( int i = 0; i < 3; i++ )
		{
			m_vecOrigin[i] = vecOrigin[i];
			m_vecAngles[i] = vecAngles[i];
		}
	}

	float	m_flScale;
	Vector	m_vecOrigin;
	Vector	m_vecAngles;
	float	m_flMaxScale;
	float	m_flScaleSpeed;
	int		m_iId;
	int		m_iType;
	float	m_flBrightness;
	float	m_flFadeSpeed;
	float	m_flStartTime;
	struct model_s *m_pSprite;
	int		m_iRed;
	int		m_iGreen;
	int		m_iBlue;
};

class CEffectNode
{
public:
	CEffectNode *Previous( int iId );	// in tri.cpp

	CEffect		m_Effect;
	CEffectNode	*m_pNext;
};

class CEffectList
{
public:
	CEffectList( void )
	{
		m_iCount = 0;
		m_iLastId = 0;
		m_pHead = NULL;
	}

	~CEffectList( void )
	{
		while ( m_iCount )
		{
			if ( m_pHead )
			{
				CEffectNode *pNode = m_pHead;
				m_iCount--;
				m_pHead = pNode->m_pNext;
				delete pNode;
			}
		}
	}

	int Add( CEffect effect )
	{
		if ( !m_iCount )
		{
			m_pHead = new CEffectNode;
			m_pHead->m_Effect = effect;
			m_iCount++;
			m_iLastId++;
			m_pHead->m_Effect.m_iId = m_iLastId;
			if ( m_iLastId > 64000 )
				m_iLastId = 0;
			return m_pHead->m_Effect.m_iId;
		}

		CEffectNode *pNode = m_pHead;
		for ( int i = 1; i < m_iCount; i++ )
			pNode = pNode->m_pNext;

		pNode->m_pNext = new CEffectNode;
		pNode->m_pNext->m_Effect = effect;
		m_iCount++;
		m_iLastId++;
		pNode->m_pNext->m_Effect.m_iId = m_iLastId;
		if ( m_iLastId > 64000 )
			m_iLastId = 0;
		return m_pHead->m_Effect.m_iId;
	}

	bool Remove( int iId );				// in tri.cpp

	CEffect &operator[]( int i )
	{
		CEffectNode *pNode = m_pHead;
		for ( ; i > 0; i-- )
			pNode = pNode->m_pNext;
		return pNode->m_Effect;
	}

	int			m_iCount;
	int			m_iLastId;
	CEffectNode	*m_pHead;
};

extern CEffectList g_ClientEffects;

#endif // EFFECTLIST_H
