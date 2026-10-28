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
// func_console - T2DEF: the security camera console.
// Using it shows the view of the first info_security_cam;
// the client's camera panel sends t2defprevcam, t2defnextcam
// and t2defexitcam (ClientCommand -> SecurityCamCommand).
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"

#define MAX_SECURITY_CAMS	6	// name ours

extern int gmsgSCShow;
extern int gmsgSetFOV;
extern int g_iT2DefCameraActive;

class CSecurityCamConsole : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	virtual int	ObjectCaps( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Think( void );
	void SecurityCamCommand( int iCommand );
	void ExitCamera( void );	// name ours

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int		m_iTotalCams;
	int		m_state;
	int		m_iCurrentCam;
	EHANDLE	m_hCamera[MAX_SECURITY_CAMS];
	EHANDLE	m_hPlayer;
	Vector	m_vecOrigin[MAX_SECURITY_CAMS];
};

TYPEDESCRIPTION	CSecurityCamConsole::m_SaveData[] = 
{
	DEFINE_FIELD( CSecurityCamConsole, m_hPlayer, FIELD_EHANDLE ),
	DEFINE_FIELD( CSecurityCamConsole, m_iTotalCams, FIELD_INTEGER ),
	DEFINE_ARRAY( CSecurityCamConsole, m_hCamera, FIELD_EHANDLE, MAX_SECURITY_CAMS ),
	DEFINE_ARRAY( CSecurityCamConsole, m_vecOrigin, FIELD_POSITION_VECTOR, MAX_SECURITY_CAMS ),
	DEFINE_FIELD( CSecurityCamConsole, m_state, FIELD_INTEGER ),
	DEFINE_FIELD( CSecurityCamConsole, m_iCurrentCam, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE(CSecurityCamConsole,CBaseEntity);

void CSecurityCamConsole :: Precache( void )
{
	PRECACHE_SOUND( "buttons/button3.wav" );
}

LINK_ENTITY_TO_CLASS( func_console, CSecurityCamConsole );

int	CSecurityCamConsole :: ObjectCaps( void )
{
	return (CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_IMPULSE_USE;
}

void CSecurityCamConsole :: Spawn( void )
{
	Precache();
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;
	SET_MODEL( ENT(pev), STRING(pev->model) );

	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( this, pev->origin );

	SetNextThink( 1 );
}

void CSecurityCamConsole :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	int iConsole;
	CBasePlayer *pPlayer;

	if ( !ShouldToggle( useType, m_state ) )
		return;

	m_state = !m_state;

	if ( !m_state )
	{
		ExitCamera();
		return;
	}

	if ( !pActivator || !pActivator->IsPlayer() )
		pActivator = CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );

	m_hPlayer = pActivator;
	pPlayer = (CBasePlayer *)pActivator;

	pPlayer->EnableControl( FALSE );
	pPlayer->pev->velocity.x = 0;
	pPlayer->pev->velocity.y = 0;
	pPlayer->pev->fov = 0;

	MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pPlayer->pev );
		WRITE_BYTE( 0 );
	MESSAGE_END();

	iConsole = (short)entindex();

	MESSAGE_BEGIN( MSG_ALL, gmsgSCShow );
		WRITE_SHORT( iConsole );
		WRITE_BYTE( 1 );
	MESSAGE_END();

	SET_VIEW( m_hPlayer->edict(), m_hCamera[0]->edict() );
	EMIT_SOUND( ENT(m_hCamera[0]->pev), CHAN_STATIC, "buttons/button3.wav", 1, 0.8 );

	g_iT2DefCameraActive = 1;
}

void CSecurityCamConsole :: SecurityCamCommand( int iCommand )
{
	m_hPlayer = CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );

	if ( iCommand == 0 )
	{
		ExitCamera();
	}
	else if ( iCommand == 1 )
	{
		m_iCurrentCam--;
		if ( m_iCurrentCam <= -1 )
			m_iCurrentCam = m_iTotalCams - 1;

		SET_VIEW( m_hPlayer->edict(), m_hCamera[m_iCurrentCam]->edict() );
		EMIT_SOUND( ENT(m_hCamera[m_iCurrentCam]->pev), CHAN_STATIC, "buttons/button3.wav", 1, 0.8 );
	}
	else if ( iCommand == 2 )
	{
		m_iCurrentCam++;
		if ( m_iCurrentCam >= m_iTotalCams )
			m_iCurrentCam = 0;

		SET_VIEW( m_hPlayer->edict(), m_hCamera[m_iCurrentCam]->edict() );
		EMIT_SOUND( ENT(m_hCamera[m_iCurrentCam]->pev), CHAN_STATIC, "buttons/button3.wav", 1, 0.8 );
	}
}

void CSecurityCamConsole :: Think( void )
{
	CBaseEntity *pCamera = NULL;

	m_iTotalCams = 0;
	while ( m_iTotalCams < MAX_SECURITY_CAMS && (pCamera = UTIL_FindEntityByClassname( pCamera, "info_security_cam" )) != NULL )
	{
		m_hCamera[m_iTotalCams] = pCamera;
		m_vecOrigin[m_iTotalCams] = pCamera->pev->origin;
		m_iTotalCams++;
	}

	if ( m_iTotalCams == 0 )
	{
		ALERT( at_console, "func_console error: no info_security_cam entities\n" );
		UTIL_Remove( this );
	}
	else
	{
		m_iCurrentCam = 0;
		ALERT( at_console, "found %d info_security_cam entities\n", m_iTotalCams );
		SetThink( NULL );
	}
}

void CSecurityCamConsole :: ExitCamera( void )
{
	SET_VIEW( m_hPlayer->edict(), m_hPlayer->edict() );
	((CBasePlayer *)(CBaseEntity *)m_hPlayer)->EnableControl( TRUE );
	m_state = 0;
	m_iCurrentCam = 0;
	g_iT2DefCameraActive = 0;
}
