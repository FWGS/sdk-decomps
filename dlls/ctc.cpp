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
/*

===== ctc.cpp ========================================================

  Capture the Chicken's entities. The rules are in ctc_gamerules.cpp.

  ctc_chicken: a team's chicken (models/chicken.mdl). Its "master" names
  the game_team_master of the team it belongs to; it spawns only in CTC
  (in other modes, or before info_wanteddetect has installed the rules,
  it waits, see WaitForGameRules). On the ground it clucks every 2-4
  seconds and plays one of its three idles. An enemy who touches it
  carries it as his only weapon (v_chicken/p_chicken, the "chicken"
  animation extension; his other weapons come back when he loses it):
  the chicken screams every second. Dropped (the carrier died), it falls
  in front of him; it finds its way home 60 seconds later, or at once
  when a team mate touches it or it leaves the world. A carrier who
  reaches his team's ctc_capturepoint while his own chicken is home
  scores (ctc_gamerules.cpp) and the chicken goes home.

  ctc_capturepoint: a trigger brush; "master" names the team's
  game_team_master. Bots aim for its centre.

  The file and class names are ours.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"

float g_flChickenAway[2];	// per team: when the dropped chicken goes home; -1 carried, 0 home

//=========================================================
// ctc_chicken
//=========================================================
class CCTCChicken : public CBasePlayerItem
{
public:
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void Activate( void );
	int IsTriggered( CBaseEntity *pActivator ) { return UTIL_IsMasterTriggered( pev->netname, pActivator ); }
	const char *TeamID( void );

	int AddToPlayer( CBasePlayer *pPlayer );
	int AddDuplicate( CBasePlayerItem *pOriginal );
	BOOL Deploy( void );
	void Drop( void );
	void Materialize( void );
	void AttachToPlayer( CBasePlayer *pPlayer );

	void SendWeaponAnim( int iAnim, int body );
	void Detach( void );
	void ChickenThink( void );	// sic: not EXPORT, as in retail
	void ReturnHome( void );
	void GoHome( BOOL fFromCarrier );

	Vector	m_vecHomeOrigin;
	Vector	m_vecHomeAngles;
	int		m_iCarrierWeapons;	// the carrier's pev->weapons while he holds the chicken
	float	m_flNextCluck;
	float	m_flNextScream;
	float	m_flNextIdle;
	int		m_iLastSlot;		// the carrier's weapon before the chicken
	int		m_iLastPosition;
	CBasePlayer	*m_pDropper;	// can't take it back until he stops touching it
	BOOL	m_fSilent;			// hushed while the score sound plays
};

LINK_ENTITY_TO_CLASS( ctc_chicken, CCTCChicken );

void CCTCChicken::Activate( void )
{
	if ( g_pGameRules->GetGameType() != GAMETYPE_CTC )
	{
		WaitForGameRules( this, g_pWaitingChickens );
		return;
	}

	if ( pev->netname && strlen( STRING( pev->netname ) ) && g_pGameRules->GetTeamIndex( TeamID() ) > -1 )
	{
		m_vecHomeOrigin = pev->origin;
		m_vecHomeAngles = pev->angles;
		m_iLastSlot = -1;
		m_flNextCluck = 0;
		pev->spawnflags |= SF_NORESPAWN;
		pev->classname = MAKE_STRING( "ctc_chicken" );
		pev->model = MAKE_STRING( "models/chicken.mdl" );
		SET_MODEL( ENT( pev ), STRING( pev->model ) );
		pev->body = 1;
		pev->sequence = LookupSequence( "chicken_idle2" );
		pev->framerate = 1;
		g_flChickenAway[ g_pGameRules->GetTeamIndex( TeamID() ) ] = 0;
		FallInit();
	}
	else
	{
		ALERT( at_console, "TeamIndex!!!!!!!!!! %i\n", g_pGameRules->GetTeamIndex( TeamID() ) );
		ALERT( at_console, "Error: Chicken not assigned a team!\n" );
	}
}

void CCTCChicken::Precache( void )
{
	PRECACHE_MODEL( "models/chicken.mdl" );
	PRECACHE_MODEL( "models/p_chicken.mdl" );
	PRECACHE_MODEL( "models/v_chicken.mdl" );
	PRECACHE_SOUND( "chicken/chick_cluck.wav" );

	char szSound[128];
	for ( int i = 1; i <= 5; i++ )
	{
		sprintf( szSound, "chicken/chick_scream%i.wav", i );
		PRECACHE_SOUND( (char *)STRING( ALLOC_STRING( szSound ) ) );
	}

	PRECACHE_SOUND( "chicken/chick_spawn.wav" );
	PRECACHE_SOUND( "common/null.wav" );
	PRECACHE_SOUND( "items/gunpickup2.wav" );
}

//=========================================================
// TeamID - the team of the chicken's game_team_master
//=========================================================
const char *CCTCChicken::TeamID( void )
{
	CBaseEntity *pMaster = UTIL_FindEntityByTargetname( NULL, STRING( pev->netname ) );

	while ( pMaster )
	{
		if ( FClassnameIs( pMaster->pev, "game_team_master" ) )
			break;
		pMaster = UTIL_FindEntityByTargetname( pMaster, STRING( pev->netname ) );
	}

	if ( !pMaster )
		return "";

	return pMaster->TeamID();
}

//=========================================================
// AddToPlayer - an enemy takes the chicken, a team mate
// brings it home
//=========================================================
int CCTCChicken::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( pPlayer->IsAlive() && ( !m_pDropper || m_pDropper != pPlayer ) )
	{
		if ( IsTriggered( pPlayer ) )
		{
			// a team mate brings it home
			if ( g_flChickenAway[ g_pGameRules->GetTeamIndex( TeamID() ) ] != 0 )
			{
				UTIL_CenterPrintAll( UTIL_VarArgs( "%s returned the %s chicken!\n", STRING( pPlayer->pev->netname ), TeamID() ) );
				GoHome( FALSE );
				EMIT_SOUND( ENT( pev ), CHAN_ITEM, "chicken/chick_spawn.wav", 1, ATTN_NORM );
			}
		}
		else
		{
			// remember the weapon he held, he gets it back
			if ( pPlayer->m_pActiveItem && pPlayer->m_pActiveItem != this )
			{
				m_iLastSlot = pPlayer->m_pActiveItem->iItemSlot();
				m_iLastPosition = pPlayer->m_pActiveItem->iItemPosition();
			}

			pPlayer->SwitchWeapon( this );
			m_iCarrierWeapons |= pPlayer->pev->weapons;
			pPlayer->pev->weapons &= (1<<WEAPON_SUIT);
			return CBasePlayerItem::AddToPlayer( pPlayer );
		}
	}

	return FALSE;
}

void CCTCChicken::AttachToPlayer( CBasePlayer *pPlayer )
{
	pev->movetype = MOVETYPE_FOLLOW;
	pev->aiment = pPlayer->edict();
	pev->owner = pPlayer->edict();
	g_flChickenAway[ g_pGameRules->GetTeamIndex( TeamID() ) ] = -1;
	m_flNextScream = gpGlobals->time + 1.8;
	m_flNextCluck = 0;
	pev->modelindex = 0;
	SetTouch( NULL );
}

int CCTCChicken::AddDuplicate( CBasePlayerItem *pOriginal )
{
	if ( pOriginal->m_pPlayer )
		AddToPlayer( pOriginal->m_pPlayer );
	return FALSE;
}

BOOL CCTCChicken::Deploy( void )
{
	if ( !m_pPlayer )
		return FALSE;

	m_pPlayer->pev->viewmodel = MAKE_STRING( "models/v_chicken.mdl" );
	SendWeaponAnim( 7, 0 );
	m_pPlayer->pev->weaponmodel = MAKE_STRING( "models/p_chicken.mdl" );
	strcpy( m_pPlayer->m_szAnimExtention, "chicken" );
	return TRUE;
}

void CCTCChicken::SendWeaponAnim( int iAnim, int body )
{
	MESSAGE_BEGIN( MSG_ONE, SVC_WEAPONANIM, NULL, m_pPlayer->edict() );
		WRITE_BYTE( iAnim );
		WRITE_BYTE( body );
	MESSAGE_END();
}

//=========================================================
// Detach - the carrier gets his weapons back
//=========================================================
void CCTCChicken::Detach( void )
{
	m_pPlayer->pev->weapons = m_iCarrierWeapons;
	m_iCarrierWeapons = 0;
	strcpy( m_pPlayer->m_szAnimExtention, "" );

	BOOL fSwitched = FALSE;
	if ( m_iLastSlot != -1 )
	{
		for ( CBasePlayerItem *pItem = m_pPlayer->m_rgpPlayerItems[ m_iLastSlot ]; pItem; pItem = pItem->m_pNext )
		{
			if ( pItem->iItemPosition() == m_iLastPosition )
			{
				m_pPlayer->SwitchWeapon( pItem );
				fSwitched = TRUE;
				break;
			}
		}
	}

	if ( !fSwitched && !g_pGameRules->GetNextBestWeapon( m_pPlayer, this ) )
	{
		Holster( );
		m_pPlayer->m_pActiveItem = NULL;
	}

	m_iLastSlot = -1;
	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_VOICE, "common/null.wav", 1, ATTN_NORM );
	pev->owner = NULL;
	m_pPlayer = NULL;
}

void CCTCChicken::Drop( void )
{
	BOOL fSolid = FALSE;

	if ( m_pPlayer )
	{
		m_pDropper = m_pPlayer;
		pev->origin = m_pPlayer->pev->origin;
		UTIL_CenterPrintAll( UTIL_VarArgs( "%s dropped the %s chicken!\n", STRING( m_pPlayer->pev->netname ), TeamID() ) );

		if ( m_pPlayer->pev->solid )
			fSolid = TRUE;

		UTIL_MakeVectors( m_pPlayer->pev->angles );
		pev->velocity = gpGlobals->v_forward * 100;
		Detach();
	}

	pev->aiment = NULL;
	pev->velocity.z += 100;
	pev->flags &= ~FL_ONGROUND;
	FallInit();

	if ( fSolid )
		pev->solid = SOLID_NOT;

	int iTeam = g_pGameRules->GetTeamIndex( TeamID() );
	g_flChickenAway[ iTeam ] = gpGlobals->time + 60;
}

void CCTCChicken::Materialize( void )
{
	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, pev->origin );
	SetTouch( DefaultTouch );
	m_flNextCluck = 1;
	m_flNextScream = 0;
	SetThink( ChickenThink );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CCTCChicken::ChickenThink( void )
{
	if ( m_pDropper && !m_pDropper->Intersects( this ) )
		m_pDropper = NULL;

	if ( m_flNextCluck != 0 && gpGlobals->time >= m_flNextCluck && !m_fSilent )
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "chicken/chick_cluck.wav", 1, ATTN_NORM, 0, RANDOM_FLOAT( 96, 104 ) );
		m_flNextCluck = RANDOM_FLOAT( 2, 4 ) + gpGlobals->time;
	}

	if ( m_flNextScream != 0 && gpGlobals->time >= m_flNextScream && !m_fSilent )
	{
		char szSound[128];
		sprintf( szSound, "chicken/chick_scream%i.wav", RANDOM_LONG( 1, 5 ) );
		EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_VOICE, szSound, 1, ATTN_NORM, 0, RANDOM_FLOAT( 96, 104 ) );
		m_flNextScream = RANDOM_FLOAT( 1, 1 ) + gpGlobals->time;
	}

	if ( gpGlobals->time >= m_flNextIdle && m_pPlayer )
	{
		SendWeaponAnim( 0, 0 );
		m_flNextIdle = RANDOM_FLOAT( 0, 3 ) + gpGlobals->time;
	}
	else if ( gpGlobals->time >= m_flNextIdle && !m_pPlayer )
	{
		int iIdle = RANDOM_LONG( 1, 3 );
		char szSequence[32];

		strcpy( szSequence, "chicken_idle" );
		strcat( szSequence, UTIL_VarArgs( "%i", iIdle ) );
		pev->sequence = LookupSequence( szSequence );
		ResetSequenceInfo();

		switch ( iIdle )
		{
		case 1:
			m_flNextIdle = gpGlobals->time + 1.0;
			break;
		case 2:
			m_flNextIdle = gpGlobals->time + 0.7;
			break;
		case 3:
			m_flNextIdle = gpGlobals->time + 1.0;
			break;
		}
	}

	// left on the ground too long
	if ( g_flChickenAway[ g_pGameRules->GetTeamIndex( TeamID() ) ] > 0 && gpGlobals->time >= g_flChickenAway[ g_pGameRules->GetTeamIndex( TeamID() ) ] )
		ReturnHome();

	if ( !IsInWorld() )
		ReturnHome();

	if ( g_flChickenHushTime > gpGlobals->time && !m_fSilent )
	{
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "common/null.wav", 1, ATTN_NORM );
		if ( m_pPlayer )
			EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_VOICE, "common/null.wav", 1, ATTN_NORM );
		m_fSilent = TRUE;
	}
	else
		m_fSilent = FALSE;

	pev->nextthink = gpGlobals->time + 0.1;
}

void CCTCChicken::ReturnHome( void )
{
	UTIL_CenterPrintAll( UTIL_VarArgs( "The %s chicken found its way home!\n", TeamID() ) );
	GoHome( FALSE );
	EMIT_SOUND( ENT( pev ), CHAN_ITEM, "chicken/chick_spawn.wav", 1, ATTN_NORM );
}

void CCTCChicken::GoHome( BOOL fFromCarrier )
{
	if ( fFromCarrier )
		Detach();

	pev->origin = m_vecHomeOrigin;
	pev->angles = m_vecHomeAngles;
	pev->aiment = NULL;
	pev->flags &= ~FL_ONGROUND;
	g_flChickenAway[ g_pGameRules->GetTeamIndex( TeamID() ) ] = 0;
	FallInit();
}

void CCTCChicken::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		pev->netname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBasePlayerItem::KeyValue( pkvd );
}

//=========================================================
// ctc_capturepoint
//=========================================================
class CCTCCapturePoint : public CBaseEntity
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	Vector EyePosition( void ) { return Center(); }
	int IsTriggered( CBaseEntity *pActivator ) { return UTIL_IsMasterTriggered( pev->netname, pActivator ); }

	// a brush entity: seen from its centre
	BOOL FVisible( CBaseEntity *pEntity )
	{
		pev->origin = Center();
		BOOL fVisible = CBaseEntity::FVisible( pEntity );
		pev->origin = g_vecZero;
		return fVisible;
	}
	BOOL FVisible( const Vector &vecOrigin )
	{
		pev->origin = Center();
		BOOL fVisible = CBaseEntity::FVisible( vecOrigin );
		pev->origin = g_vecZero;
		return fVisible;
	}

	const char *TeamID( void );
	void CaptureTouch( CBaseEntity *pOther );	// sic: not EXPORT, as in retail
};

LINK_ENTITY_TO_CLASS( ctc_capturepoint, CCTCCapturePoint );

void CCTCCapturePoint::Spawn( void )
{
	if ( g_pGameRules->GetGameType() != GAMETYPE_CTC )
	{
		WaitForGameRules( this, g_pWaitingChickens );
		return;
	}

	pev->solid = SOLID_TRIGGER;
	pev->flags |= FL_WORLDBRUSH;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	pev->modelindex = 0;
	SetTouch( CaptureTouch );
}

const char *CCTCCapturePoint::TeamID( void )
{
	edict_t *pentMaster = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->netname ) );

	if ( !pentMaster )
		return "";

	return CBaseEntity::Instance( pentMaster )->TeamID();
}

//=========================================================
// CaptureTouch - a carrier brings an enemy chicken home
// while his own chicken is home
//=========================================================
void CCTCCapturePoint::CaptureTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() )
		return;

	if ( !IsTriggered( pOther ) )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	if ( !pPlayer->HasNamedPlayerItem( "ctc_chicken" ) )
		return;

	if ( g_flChickenAway[ g_pGameRules->GetTeamIndex( pPlayer->TeamID() ) ] != 0 )
		return;

	CBasePlayerItem *pChicken;
	for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		pChicken = pPlayer->m_rgpPlayerItems[ i ];
		while ( pChicken )
		{
			if ( !strcmp( "ctc_chicken", STRING( pChicken->pev->classname ) ) )
			{
				i = MAX_ITEM_TYPES;
				break;
			}
			pChicken = pChicken->m_pNext;
		}
	}

	if ( !pChicken )
	{
		ALERT( at_console, "Error: Couldn't find chicken on player!\n" );
		return;
	}

	pPlayer->RemovePlayerItem( pChicken );

	if ( g_pGameRules->GetGameType() == GAMETYPE_CTC )
		((CCaptureTheChicken *)g_pGameRules)->UpdateTeamScore( pPlayer, pChicken->TeamID() );

	((CCTCChicken *)pChicken)->GoHome( TRUE );
}

void CCTCCapturePoint::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		pev->netname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}
