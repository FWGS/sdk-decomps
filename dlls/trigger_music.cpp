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
// trigger_music - T2DEF: plays music/<MusicFile> on the
// clients (PlayMusic message), looped if LoopMusic is set.
// Fires when used, or when a player touches it.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"

extern int gmsgPlayMusic;
extern int gmsgStopMusic;
extern int g_iT2DefMusicOn;

class CTriggerMusic : public CBaseEntity
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT PlayMusic( CBaseEntity *pOther );
	virtual int ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	string_t	m_iszMusicFile;
	char		m_fLoop;
};

LINK_ENTITY_TO_CLASS( trigger_music, CTriggerMusic );

TYPEDESCRIPTION	CTriggerMusic::m_SaveData[] = 
{
	DEFINE_FIELD( CTriggerMusic, m_iszMusicFile, FIELD_STRING ),
	DEFINE_FIELD( CTriggerMusic, m_fLoop, FIELD_CHARACTER ),
};

IMPLEMENT_SAVERESTORE(CTriggerMusic,CBaseEntity);

void CTriggerMusic :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "LoopMusic"))
	{
		m_fLoop = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "MusicFile"))
	{
		m_iszMusicFile = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CTriggerMusic :: Spawn( void )
{
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL( ENT(pev), STRING(pev->model) );
	pev->effects |= EF_NODRAW;
	SetTouch( PlayMusic );
}

void CTriggerMusic :: PlayMusic( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() )
		return;

	Use( pOther, pOther, USE_ON, 0 );
}

void CTriggerMusic :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	char szMusic[256];

	if ( g_iT2DefMusicOn == 1 )
		return;

	MESSAGE_BEGIN( MSG_ALL, gmsgStopMusic );
		WRITE_BYTE( 0 );
	MESSAGE_END();

	if ( !m_iszMusicFile )
		return;

	strcpy( szMusic, STRING(m_iszMusicFile) );
	SetTouch( NULL );

	MESSAGE_BEGIN( MSG_ALL, gmsgPlayMusic );
		WRITE_BYTE( m_fLoop );
		WRITE_STRING( szMusic );
	MESSAGE_END();
}
