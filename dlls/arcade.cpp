/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
// trigger_arcade - T2DEF: the Hazard Course shooting gallery.
// Use starts a round (score and time remaining on the HUD),
// the next Use ends it and shows the final score.
// Killed monsters add to the score (monstermaker.cpp).
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"

class CArcade : public CBaseDelay
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int ObjectCaps( void ) { return CBaseDelay :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	void AddScore( int iScore );		// name ours
	void EXPORT ArcadeThink( void );
	void ShowFinalScore( void );		// name ours

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	float	m_flStartTime;
	float	m_flDuration;
	int		m_iTotalScore;
	int		m_iActive;
	hudtextparms_t	m_textParms;	// name ours
};

LINK_ENTITY_TO_CLASS( trigger_arcade, CArcade );

TYPEDESCRIPTION	CArcade::m_SaveData[] =
{
	DEFINE_FIELD( CArcade, m_flStartTime, FIELD_FLOAT ),
	DEFINE_FIELD( CArcade, m_flDuration, FIELD_FLOAT ),
	DEFINE_FIELD( CArcade, m_iTotalScore, FIELD_INTEGER ),
	DEFINE_FIELD( CArcade, m_iActive, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE(CArcade,CBaseDelay);

void CArcade :: Spawn( void )
{
	m_iTotalScore = 0;
	m_flStartTime = 0;
	m_iActive = 0;
}

void CArcade :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "duration"))
	{
		m_flDuration = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CArcade :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!m_iActive)
	{
		m_iTotalScore = 0;
		m_flStartTime = gpGlobals->time;
		m_iActive = 1;
		SetThink( ArcadeThink );
		SetNextThink( 1 );
	}
	else
	{
		m_iActive = 0;
		SetThink( NULL );
		ShowFinalScore();
	}
}

void CArcade :: AddScore( int iScore )
{
	m_iTotalScore += iScore;
	SetNextThink( 0.1 );
}

void CArcade :: ArcadeThink( void )
{
	char szText[64];
	int iLen = 0;	// sic: unused
	char szScore[6];
	char szTime[3];
	float flTime;

	memset( szText, 0, 64 );
	memset( szScore, 0, 6 );
	memset( szTime, 0, 3 );

	flTime = (m_flStartTime + m_flDuration) - gpGlobals->time;

	strcpy( szText, "Score: " );
	sprintf( szScore, "%06d", m_iTotalScore ); // sic: 7 bytes into 6
	strcat( szText, szScore );
	strcat( szText, " Time Remaining: " );
	sprintf( szTime, "%3.0f", flTime ); // sic: 4 bytes into 3
	strcat( szText, szTime );

	m_textParms.channel = 1;
	m_textParms.x = -1;
	m_textParms.y = 0.1;
	m_textParms.effect = 0;
	m_textParms.r1 = 255;
	m_textParms.g1 = 0;
	m_textParms.b1 = 0;
	m_textParms.a1 = 0;
	m_textParms.r2 = 255;
	m_textParms.g2 = 0;
	m_textParms.b2 = 0;
	m_textParms.a2 = 0;
	m_textParms.fadeinTime = 0;
	m_textParms.fadeoutTime = 3;
	m_textParms.holdTime = 2;
	m_textParms.fxTime = 0.15;
	UTIL_HudMessageAll( m_textParms, szText );

	SetNextThink( 1 );
}

void CArcade :: ShowFinalScore( void )
{
	char szText[64];
	int iLen = 0;	// sic: unused
	char szScore[6];

	memset( szText, 0, 64 );
	memset( szScore, 0, 6 );

	strcpy( szText, "Final Score: " );
	sprintf( szScore, "%06d", m_iTotalScore ); // sic: 7 bytes into 6
	strcat( szText, szScore );

	m_textParms.channel = 1;
	m_textParms.x = -1;
	m_textParms.y = 0.4;
	m_textParms.effect = 0;
	m_textParms.r1 = 255;
	m_textParms.g1 = 0;
	m_textParms.b1 = 0;
	m_textParms.a1 = 0;
	m_textParms.r2 = 255;
	m_textParms.g2 = 0;
	m_textParms.b2 = 0;
	m_textParms.a2 = 0;
	m_textParms.fadeinTime = 0;
	m_textParms.fadeoutTime = 3;
	m_textParms.holdTime = 20;
	m_textParms.fxTime = 0.15;
	UTIL_HudMessageAll( m_textParms, szText );
}
