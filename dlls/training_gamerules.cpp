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
//
// training_gamerules.cpp
//
// Counter-Strike 1.0: the rules of the training course (CHalfLifeTraining) and the course's
// two brush entities, func_grencatch and func_weaponcheck.  Reconstructed from retail 1.0's
// mp.dll; the function order, the class members' names and the locals' names are
// Counter-Strike 1.6's (debug information), the entities' member names retail's own (their
// save tables).
//
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"

extern DLL_GLOBAL BOOL	g_fGameOver;
extern int gmsgStatusIcon;
extern int gmsgBlinkAcct;
extern int gmsgBarTime;

inline int FNullEnt( CBaseEntity *ent ) { return (ent == NULL) || FNullEnt( ent->edict() ); }

//=========================================================
//=========================================================
CHalfLifeTraining::CHalfLifeTraining( void )
{
	PRECACHE_MODEL( "models/w_weaponbox.mdl" );
}

BOOL CHalfLifeTraining::IsDeathmatch( void ) { return FALSE; }
void CHalfLifeTraining::InitHUD( CBasePlayer *pl ) {}

//=========================================================
// CHostage::TakeDamage calls it when the player has killed a hostage: PlayerThink reloads
// the map three seconds later.  The time is kept in the player's pev->radsuit_finished.
//=========================================================
void CHalfLifeTraining::HostageDied( void )
{
	CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( 1 );

	if ( pPlayer )
		pPlayer->pev->radsuit_finished = gpGlobals->time + 3.0;
}

//=========================================================
// CGameRules::GetPlayerSpawnSpot's text, on the first info_player_start
//=========================================================
edict_t *CHalfLifeTraining::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "info_player_start" );

	if ( FNullEnt( pSpot ) )
	{
		ALERT( at_error, "PutClientInServer: no info_player_start on level" );
		return INDEXENT( 0 );
	}

	pPlayer->pev->origin = pSpot->pev->origin + Vector( 0, 0, 1 );
	pPlayer->pev->v_angle = g_vecZero;
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->angles = pSpot->pev->angles;
	pPlayer->pev->punchangle = g_vecZero;
	pPlayer->pev->fixangle = TRUE;

	return pSpot->edict();
}

//=========================================================
// Every frame of the player.  The course has no rounds: the round's clock is kept one
// second ahead, the player stays a counter-terrorist who may shoot.  The money and the
// defuse kit are carried through a saved game in pev->scale and pev->ideal_yaw.
//=========================================================
void CHalfLifeTraining::PlayerThink( CBasePlayer *pPlayer )
{
	// a hostage died (HostageDied): start the map again
	if ( pPlayer->pev->radsuit_finished && pPlayer->pev->radsuit_finished < gpGlobals->time )
		SERVER_COMMAND( "reload\n" );

	if ( !pPlayer->m_iAccount && pPlayer->pev->scale )
		pPlayer->m_iAccount = pPlayer->pev->scale;

	if ( pPlayer->m_iTeam == TEAM_UNASSIGNED )
	{
		// a saved game was loaded: no progress bar, the defuse kit back
		// (Counter-Strike 1.4: through the player's SetProgressBarTime; to 1.3 a BarTime message here)
		pPlayer->SetProgressBarTime( 0 );

		pPlayer->m_fHasDefuser = ( pPlayer->pev->ideal_yaw != 0 );
	}

	m_iHostagesRescued = 0;
	m_iRoundTimeSecs = gpGlobals->time + 1.0;
	m_bFreezePeriod = FALSE;
	g_fGameOver = FALSE;

	pPlayer->m_iTeam = TEAM_CT;
	pPlayer->m_bCanShoot = true;
	pPlayer->m_flLastActivity = gpGlobals->time;

	if ( pPlayer->m_pActiveItem )
		pPlayer->m_iHideHUD &= ~HIDEHUD_WEAPONS;
	else
		pPlayer->m_iHideHUD |= HIDEHUD_WEAPONS;

	if ( pPlayer->HasNamedPlayerItem( "weapon_c4" ) )
	{
		if ( pPlayer->m_rgAmmo[ pPlayer->GetAmmoIndex( "C4" ) ] > 0 )
		{
			pPlayer->m_bHasC4 = true;
		}
		else
		{
			// the bomb is planted: the empty weapon goes
			pPlayer->m_bHasC4 = false;

			CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)pPlayer->m_pActiveItem;

			if ( FClassnameIs( pWeapon->pev, "weapon_c4" ) )
			{
				pPlayer->pev->weapons &= ~( 1 << pWeapon->m_iId );
				pPlayer->RemovePlayerItem( pWeapon );
				pWeapon->Drop();
			}
		}
	}

	if ( !pPlayer->m_bVGUIMenus )
	{
		if ( fVGUIMenus )
			pPlayer->m_bVGUIMenus = fVGUIMenus;
	}

	// a bomb planted at a bomb target (the player's own) can not be defused: with
	// m_bStartDefuse set CGrenade::Use starts nothing
	CGrenade *pBomb = NULL;

	while ( ( pBomb = (CGrenade *)UTIL_FindEntityByClassname( pBomb, "grenade" ) ) != NULL )
	{
		if ( pBomb->m_pentCurBombTarget )
			break;
	}

	if ( pBomb )
		pBomb->m_bStartDefuse = true;

	if ( pPlayer->m_signals.GetState() & 1 )	// in a buy zone
	{
		if ( !fInBuyArea )
		{
			FillAccountTime = 1;

			if ( !fVisitedBuyArea )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pPlayer->pev );
					WRITE_BYTE( 2 );	// flashing
					WRITE_STRING( "buyzone" );
					WRITE_BYTE( 0 );
					WRITE_BYTE( 160 );
					WRITE_BYTE( 0 );
				MESSAGE_END();
			}
		}

		fInBuyArea = TRUE;

		if ( pPlayer->m_iAccount < 16000 && FillAccountTime == 0 )
			FillAccountTime = gpGlobals->time + 5.0;

		if ( FillAccountTime != 0 && gpGlobals->time > FillAccountTime )
		{
			if ( !fVisitedBuyArea )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgBlinkAcct, NULL, pPlayer->pev );
					WRITE_BYTE( 3 );
				MESSAGE_END();

				MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pPlayer->pev );
					WRITE_BYTE( 1 );
					WRITE_STRING( "buyzone" );
					WRITE_BYTE( 0 );
					WRITE_BYTE( 160 );
					WRITE_BYTE( 0 );
				MESSAGE_END();

				fVisitedBuyArea = TRUE;
			}

			pPlayer->AddAccount( 16000 - pPlayer->m_iAccount, true );
			FillAccountTime = 0;
		}
	}
	else if ( fInBuyArea && fVisitedBuyArea )
	{
		fInBuyArea = FALSE;
	}

	pPlayer->pev->scale = pPlayer->m_iAccount;
	pPlayer->pev->ideal_yaw = pPlayer->m_fHasDefuser;
}

//=========================================================
// The player's first spawn of the map (m_fNotKilled says he has spawned): a
// counter-terrorist with what the map's game_player_equip entities give
//=========================================================
void CHalfLifeTraining::PlayerSpawn( CBasePlayer *pPlayer )
{
	if ( pPlayer->m_fNotKilled )
		return;

	fInBuyArea = TRUE;
	fVisitedBuyArea = FALSE;
	FillAccountTime = 0;

	pPlayer->m_iTeam = TEAM_CT;
	pPlayer->m_iJoiningState = JOIN_DONE;
	pPlayer->m_fNotKilled = true;
	pPlayer->pev->body = 0;
	pPlayer->m_iAppearance = 1;
	fVGUIMenus = pPlayer->m_bVGUIMenus;

	SET_MODEL( ENT( pPlayer->pev ), "models/player.mdl" );

	CBaseEntity *pWeaponEntity = NULL;

	while ( ( pWeaponEntity = UTIL_FindEntityByClassname( pWeaponEntity, "game_player_equip" ) ) != NULL )
		pWeaponEntity->Touch( pPlayer );

	pPlayer->SetPlayerModel( FALSE );
	pPlayer->Spawn();
	pPlayer->m_iHideHUD |= ( HIDEHUD_WEAPONS | HIDEHUD_HEALTH | HIDEHUD_TIMER | HIDEHUD_MONEY );
}

//=========================================================
//=========================================================
int CHalfLifeTraining::ItemShouldRespawn( CItem *pItem )
{
	return GR_ITEM_RESPAWN_NO;
}

BOOL CHalfLifeTraining::FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	return TRUE;
}

//=========================================================
// CBasePlayer::CanPlayerBuy asks: in the course a buy zone is all it takes
//=========================================================
bool CHalfLifeTraining::PlayerCanBuy( CBasePlayer *pPlayer )
{
	return ( pPlayer->m_signals.GetState() & 1 ) ? true : false;	// (a plain return of the masked value, or != 0, gives and eax, 1; retail has and al, 1)
}

//=========================================================
//=========================================================
void CHalfLifeTraining::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	SET_VIEW( ENT( pVictim->pev ), ENT( pVictim->pev ) );
	FireTargets( "game_playerdie", pVictim, pVictim, USE_TOGGLE, 0 );
}

//=========================================================
// The course's tasks, told to the map by targets: the bomb defused (the bomb's target),
// the bomb gone off (its noise1), a hostage rescued (the rescue zone's target)
//=========================================================
void CHalfLifeTraining::CheckWinConditions( void )
{
	if ( m_fBombDefused )
	{
		CGrenade *pBomb = NULL;

		while ( ( pBomb = (CGrenade *)UTIL_FindEntityByClassname( pBomb, "grenade" ) ) != NULL )
		{
			if ( pBomb->m_bIsC4 && pBomb->m_bJustBlew )
				break;
		}

		if ( pBomb )
		{
			pBomb->m_bJustBlew = false;
			m_fBombDefused = false;
			FireTargets( STRING( pBomb->pev->target ), CBaseEntity::Instance( pBomb->pev->owner ), CBaseEntity::Instance( pBomb->pev->owner ), USE_TOGGLE, 0 );
		}
	}
	else if ( m_fTargetBombed )
	{
		CGrenade *pBomb = NULL;

		while ( ( pBomb = (CGrenade *)UTIL_FindEntityByClassname( pBomb, "grenade" ) ) != NULL )
		{
			if ( pBomb->m_bIsC4 && pBomb->m_bJustBlew )
				break;
		}

		if ( pBomb )
		{
			if ( pBomb->pev->noise1 )
			{
				pBomb->m_bJustBlew = false;
				m_fTargetBombed = false;
				FireTargets( STRING( pBomb->pev->noise1 ), CBaseEntity::Instance( pBomb->pev->owner ), CBaseEntity::Instance( pBomb->pev->owner ), USE_TOGGLE, 0 );
			}
		}
	}

	// a rescued hostage (CHostage's DEAD_RESPAWNABLE) not told to the map yet (noise1)
	CBaseEntity *pHostage = CBaseEntity::Instance( FIND_ENTITY_BY_CLASSNAME( NULL, "hostage_entity" ) );

	while ( pHostage )
	{
		if ( pHostage->pev->deadflag == DEAD_RESPAWNABLE && !pHostage->pev->noise1 )
			break;

		pHostage = UTIL_FindEntityByClassname( pHostage, "hostage_entity" );
	}

	if ( pHostage )
	{
		UTIL_SetSize( pHostage->pev, Vector( -16, -16, 0 ), Vector( 16, 16, 72 ) );

		CBaseEntity *pRescueArea;
		CBaseEntity *pFirstRescueArea;

		pFirstRescueArea = CBaseEntity::Instance( FIND_ENTITY_BY_CLASSNAME( NULL, "func_hostage_rescue" ) );
		pRescueArea = pFirstRescueArea;

		// retail: the first zone always passes for the one the hostage is in
		while ( pRescueArea )
		{
			if ( pRescueArea == pFirstRescueArea || pRescueArea->Intersects( pHostage ) )
				break;

			pRescueArea = UTIL_FindEntityByClassname( pRescueArea, "func_hostage_rescue" );
		}

		if ( pRescueArea )
		{
			pHostage->pev->noise1 = 1;
			FireTargets( STRING( pRescueArea->pev->target ), NULL, NULL, USE_TOGGLE, 0 );
		}
	}
}

//=========================================================
// func_grencatch: a brush that waits for a grenade.  "grenadetype" smoke: a smoke grenade
// come to rest inside fires "triggerongrenade" once and makes the entities named
// "disableongrenade" non-solid for as long as it lies there; flash: a flashbang's touch
// fires the target and removes the brush.
//=========================================================
class CBaseGrenCatch : public CBaseEntity
{
public:
	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	void Think( void );
	void KeyValue( KeyValueData *pkvd );
	int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	int m_NeedGrenadeType;		// 1 smoke, 2 flash

	string_t sTriggerOnGrenade, sDisableOnGrenade;

	bool m_fSmokeTouching, m_fFlashTouched;

	int Save( CSave &save );
	int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];
};

TYPEDESCRIPTION CBaseGrenCatch::m_SaveData[] =
{
	DEFINE_FIELD( CBaseGrenCatch, m_NeedGrenadeType, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseGrenCatch, m_fSmokeTouching, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseGrenCatch, m_fFlashTouched, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseGrenCatch, sTriggerOnGrenade, FIELD_STRING ),
	DEFINE_FIELD( CBaseGrenCatch, sDisableOnGrenade, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CBaseGrenCatch, CBaseEntity );

LINK_ENTITY_TO_CLASS( func_grencatch, CBaseGrenCatch );

void CBaseGrenCatch::Spawn( void )
{
	pev->solid = SOLID_TRIGGER;
	pev->flags |= FL_WORLDBRUSH;
	pev->effects |= EF_NODRAW;

	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBaseGrenCatch::Touch( CBaseEntity *pOther )
{
	if ( !pOther )
		return;

	if ( strstr( STRING( pOther->pev->model ), "flash" ) )
		m_fFlashTouched = true;
}

void CBaseGrenCatch::Think( void )
{
	CGrenade *pGrenade = NULL;

	BOOL m_fSmokeTouchingLastFrame = m_fSmokeTouching;
	m_fSmokeTouching = false;

	// (the three loops of this function test at the top and call once in retail: written
	// with the search in the condition they come out with the call doubled)
	while ( 1 )
	{
		pGrenade = (CGrenade *)UTIL_FindEntityByClassname( pGrenade, "grenade" );

		if ( !pGrenade )
			break;

		Vector vMax( pGrenade->pev->maxs ), vMin( pGrenade->pev->mins );

		UTIL_SetSize( pGrenade->pev, Vector( -8, -8, 0 ), Vector( 8, 8, 0 ) );

		if ( pGrenade->Intersects( this ) && strstr( STRING( pGrenade->pev->model ), "smoke" ) )
		{
			if ( pGrenade->pev->velocity.Length() == 0 )
				m_fSmokeTouching = true;
		}

		pGrenade->pev->maxs = vMax;
		pGrenade->pev->mins = vMin;
	}

	CBaseEntity *pTrigger = NULL;

	if ( ( m_NeedGrenadeType == 1 && m_fSmokeTouching && !m_fSmokeTouchingLastFrame )
		|| ( m_NeedGrenadeType == 2 && m_fFlashTouched ) )
	{
		FireTargets( STRING( sTriggerOnGrenade ), this, this, USE_TOGGLE, 0 );

		if ( m_NeedGrenadeType == 1 )
		{
			// the solid type waits in pev->team
			while ( 1 )
			{
				pTrigger = UTIL_FindEntityByTargetname( pTrigger, STRING( sDisableOnGrenade ) );

				if ( !pTrigger )
					break;

				pTrigger->pev->team = pTrigger->pev->solid;
				pTrigger->pev->solid = SOLID_NOT;
			}
		}
		else if ( m_NeedGrenadeType == 2 )
		{
			pev->flags |= FL_KILLME;
		}
	}

	if ( m_fSmokeTouchingLastFrame && !m_fSmokeTouching )
	{
		pTrigger = NULL;

		while ( 1 )
		{
			pTrigger = UTIL_FindEntityByTargetname( pTrigger, STRING( sDisableOnGrenade ) );

			if ( !pTrigger )
				break;

			pTrigger->pev->solid = pTrigger->pev->team;
			pTrigger->pev->team = 0;
			UTIL_SetOrigin( pTrigger->pev, pTrigger->pev->origin );
		}
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

void CBaseGrenCatch::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "triggerongrenade" ) )
	{
		sTriggerOnGrenade = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "disableongrenade" ) )
	{
		sDisableOnGrenade = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}

	if ( FStrEq( pkvd->szKeyName, "grenadetype" ) )
	{
		if ( FStrEq( pkvd->szValue, "smoke" ) )
			m_NeedGrenadeType = 1;
		else if ( FStrEq( pkvd->szValue, "flash" ) )
			m_NeedGrenadeType = 2;

		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

//=========================================================
// func_weaponcheck: a brush that looks at what the touching player carries.  With every
// "item..." of its list (or, "any_weapon" set, with one of them) it fires "trigger_items"
// and is gone; without, it fires "trigger_noitems", again every "trigger_noitems_delay"
// seconds (once if that is 0).
//=========================================================
class CFuncWeaponCheck : public CBaseEntity
{
public:
	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	void KeyValue( KeyValueData *pkvd );

	string_t sTriggerWithItems, sTriggerNoItems, sMaster;
	string_t sItemName[32];
	int iItemCount;
	int iAnyWeapon;

	int Save( CSave &save );
	int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];
};

void CFuncWeaponCheck::Spawn( void )
{
	pev->dmgtime = 0;
	pev->solid = SOLID_TRIGGER;
	pev->flags |= FL_WORLDBRUSH;
	pev->effects |= EF_NODRAW;

	SET_MODEL( ENT( pev ), STRING( pev->model ) );
}

TYPEDESCRIPTION CFuncWeaponCheck::m_SaveData[] =
{
	DEFINE_FIELD( CFuncWeaponCheck, sTriggerWithItems, FIELD_STRING ),
	DEFINE_FIELD( CFuncWeaponCheck, sTriggerNoItems, FIELD_STRING ),
	DEFINE_FIELD( CFuncWeaponCheck, iItemCount, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncWeaponCheck, sMaster, FIELD_STRING ),
	DEFINE_ARRAY( CFuncWeaponCheck, sItemName, FIELD_STRING, 32 ),
	DEFINE_FIELD( CFuncWeaponCheck, iAnyWeapon, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CFuncWeaponCheck, CBaseEntity );

LINK_ENTITY_TO_CLASS( func_weaponcheck, CFuncWeaponCheck );

void CFuncWeaponCheck::Touch( CBaseEntity *pOther )
{
	if ( !UTIL_IsMasterTriggered( sMaster, pOther ) )
		return;

	if ( !pOther )
		return;

	if ( !pOther->IsPlayer() )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	for ( int i = 0; i < iItemCount; i++ )
	{
		if ( !iAnyWeapon )
		{
			if ( !pPlayer->HasNamedPlayerItem( STRING( sItemName[i] ) ) )
			{
				// pev->dmgtime: no "trigger_noitems" before this time; pev->speed: the delay, -1 when it was fired for good
				if ( pev->dmgtime < gpGlobals->time )
				{
					if ( pev->speed > -1 )
					{
						FireTargets( STRING( sTriggerNoItems ), pOther, pOther, USE_TOGGLE, 0 );
						pev->dmgtime = pev->speed + gpGlobals->time;

						if ( pev->speed == 0 )
							pev->speed = -1;
					}
				}

				return;
			}
		}
		else
		{
			// retail: without any of them the loop ends and the brush fires all the same
			if ( pPlayer->HasNamedPlayerItem( STRING( sItemName[i] ) ) )
				break;
		}
	}

	FireTargets( STRING( sTriggerWithItems ), pOther, pOther, USE_TOGGLE, 0 );
	SUB_Remove();
}

void CFuncWeaponCheck::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "trigger_items" ) )
	{
		sTriggerWithItems = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "trigger_noitems" ) )
	{
		sTriggerNoItems = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "trigger_noitems_delay" ) )
	{
		pev->speed = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( strstr( pkvd->szKeyName, "item" ) )
	{
		if ( iItemCount < 32 )
		{
			sItemName[ iItemCount ] = ALLOC_STRING( pkvd->szValue );
			iItemCount++;
		}

		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "any_weapon" ) )
	{
		iAnyWeapon = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}
