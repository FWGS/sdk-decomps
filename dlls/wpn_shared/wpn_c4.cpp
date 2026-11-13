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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

enum c4_e		// names ours
{
	C4_IDLE1 = 0,
	C4_DRAW,
	C4_DROP,
	C4_ARM,
};

extern int gmsgBarTime;

#ifdef CLIENT_DLL
extern bool g_bInBombZone;
#endif

LINK_ENTITY_TO_CLASS( weapon_c4, CC4 );


void CC4::Spawn( )
{
	SET_MODEL(ENT(pev), "models/w_backpack.mdl");

	pev->frame = 0;
	pev->body = 3;
	pev->sequence = 0;
	pev->framerate = 0;

	m_iId = WEAPON_C4;
	m_iDefaultAmmo = C4_DEFAULT_GIVE;
	m_bStartedArming = false;
	m_fArmedTime = 0;

	if ( pev->targetname )
	{
		pev->effects |= EF_NODRAW;
		DROP_TO_FLOOR( ENT(pev) );
		return;
	}

	FallInit();// get ready to fall down.

	SetThink( &CBasePlayerItem::FallThink );
	pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
}


void CC4::Precache( void )
{
	PRECACHE_MODEL("models/v_c4.mdl");
	PRECACHE_MODEL("models/w_backpack.mdl");

	PRECACHE_SOUND("weapons/c4_click.wav");
}

int CC4::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "C4";
	p->iMaxAmmo1 = C4_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_C4;
	p->iWeight = C4_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}

BOOL CC4::Deploy( )
{
	pev->body = 0;

	m_bStartedArming = false;
	m_fArmedTime = 0;

	return DefaultDeploy( "models/v_c4.mdl", "models/p_c4.mdl", C4_DRAW, "c4", UseDecrement() ? 1 : 0 );
}

void CC4::Holster( int skiplocal )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	m_bStartedArming = false;

	if ( !m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
	{
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_C4);
		DestroyItem();
	}

	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM );
}

void CC4::PrimaryAttack( void )
{
	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] <= 0 )
		return;

	// Counter-Strike 1.4: the planter stands on the ground, in the zone, and does not move
	// (1.6's two names)
	int onGround = ( m_pPlayer->pev->flags & FL_ONGROUND );
#ifdef CLIENT_DLL
	int inBombZone = g_bInBombZone;
#else
	int inBombZone = ( m_pPlayer->m_signals.m_iState & 2 );	// the bomb target zone
#endif

	if ( m_bStartedArming == false )
	{
		if ( inBombZone && onGround )
		{
			m_bStartedArming = true;
			m_fArmedTime = gpGlobals->time + 3.0;
			m_bBombPlacedAnimation = false;

			SendWeaponAnim( C4_ARM, UseDecrement() ? 1 : 0 );

#ifndef CLIENT_DLL
			g_engfuncs.pfnSetClientMaxspeed( ENT( m_pPlayer->pev ), 1 );

			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
			m_pPlayer->SetProgressBarTime( 3 );
#endif
		}
		else
		{
			if ( !inBombZone )
				ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "#C4_Plant_At_Bomb_Spot" );
			else
				ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "#C4_Plant_Must_Be_On_Ground" );

			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0;
			return;
		}
	}
	else
	{
		if ( onGround && inBombZone )
		{
			if ( !( gpGlobals->time >= m_fArmedTime ) )
			{
				if ( gpGlobals->time >= m_fArmedTime - 0.75 && m_bBombPlacedAnimation == false )
				{
					m_bBombPlacedAnimation = true;
					SendWeaponAnim( C4_DROP, UseDecrement() ? 1 : 0 );
#ifndef CLIENT_DLL
					m_pPlayer->SetAnimation( PLAYER_HOLDBOMB );
#endif
				}
			}
			else if ( m_bStartedArming == true )
			{
				m_bStartedArming = false;
				m_fArmedTime = 0;

#ifndef CLIENT_DLL
				Broadcast( "BOMBPL" );
				m_pPlayer->m_bHasC4 = false;

				if ( pev->speed != 0 && g_pGameRules )
					((CHalfLifeMultiplay *)g_pGameRules)->m_iC4Timer = (int)pev->speed;

				CGrenade::ShootSatchelCharge( m_pPlayer->pev, m_pPlayer->pev->origin, Vector( 0, 0, 0 ) );

				// tell director about it
				MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
					WRITE_BYTE ( 9 );	// command length in bytes
					WRITE_BYTE ( DRC_CMD_EVENT );
					WRITE_SHORT( ENTINDEX( m_pPlayer->edict() ) );
					WRITE_SHORT( 0 );
					WRITE_LONG( 11 | DRC_FLAG_FACEPLAYER );
				MESSAGE_END();

				UTIL_ClientPrintAll( HUD_PRINTCENTER, "#Bomb_Planted" );
				UTIL_LogPrintf( "\"%s<%i><%s><TERRORIST>\" triggered \"Planted_The_Bomb\"\n", STRING( m_pPlayer->pev->netname ),
					GETPLAYERUSERID( m_pPlayer->edict() ), GETPLAYERAUTHID( m_pPlayer->edict() ) );
				EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/c4_plant.wav", VOL_NORM, ATTN_NORM );

				m_pPlayer->pev->body = 0;
				m_pPlayer->ResetMaxSpeed();
				m_pPlayer->SetBombIcon( FALSE );
#endif

				m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ]--;

				if ( !m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
				{
					RetireWeapon();
					return;
				}
			}
		}
		else
		{
			if ( !inBombZone )
				ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "#C4_Arming_Cancelled" );
			else
				ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "#C4_Plant_Must_Be_On_Ground" );

			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.5;
			m_bStartedArming = false;

#ifndef CLIENT_DLL
			m_pPlayer->ResetMaxSpeed();
			m_pPlayer->SetProgressBarTime( 0 );
			m_pPlayer->SetAnimation( PLAYER_HOLDBOMB );
#endif

			if ( m_bBombPlacedAnimation == true )
				SendWeaponAnim( C4_DRAW, UseDecrement() ? 1 : 0 );
			else
				SendWeaponAnim( C4_IDLE1, UseDecrement() ? 1 : 0 );

			return;
		}
	}

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
}

void CC4::WeaponIdle( void )
{
	if ( m_bStartedArming == true )
	{
		m_bStartedArming = false;
#ifndef CLIENT_DLL
		m_pPlayer->ResetMaxSpeed();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0;

		m_pPlayer->SetProgressBarTime( 0 );
#endif

		if ( m_bBombPlacedAnimation == true )
			SendWeaponAnim( C4_DRAW, UseDecrement() ? 1 : 0 );
		else
			SendWeaponAnim( C4_IDLE1, UseDecrement() ? 1 : 0 );
	}

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] > 0 )
	{
		SendWeaponAnim( C4_DRAW, UseDecrement() ? 1 : 0 );
		SendWeaponAnim( C4_IDLE1, UseDecrement() ? 1 : 0 );
	}
	else
	{
		RetireWeapon();
	}
}

void CC4::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "detonatedelay" ) )
	{
		pev->speed = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detonatetarget" ) )
	{
		pev->noise1 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "defusetarget" ) )
	{
		pev->target = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

void CC4::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( m_pPlayer )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( 1 );

	if ( pPlayer )
	{
		edict_t *m_pentOldCurBombTarget = pPlayer->m_pentCurBombTarget;
		pPlayer->m_pentCurBombTarget = NULL;

		if ( pev->speed != 0 && g_pGameRules )
			((CHalfLifeMultiplay *)g_pGameRules)->m_iC4Timer = (int)pev->speed;

		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/c4_plant.wav", VOL_NORM, ATTN_NORM );

		CGrenade::ShootSatchelCharge( pPlayer->pev, pev->origin, Vector( 0, 0, 0 ) );

		CGrenade *pC4 = NULL;

		while ( ( pC4 = (CGrenade *)UTIL_FindEntityByClassname( pC4, "grenade" ) ) != NULL )
		{
			if ( pC4->m_bIsC4 && pC4->m_flNextFreq == gpGlobals->time )
				break;
		}

		if ( pC4 )
		{
			pC4->pev->target = pev->target;
			pC4->pev->noise1 = pev->noise1;
		}

		pPlayer->m_pentCurBombTarget = m_pentOldCurBombTarget;
		SUB_Remove();
	}
}
