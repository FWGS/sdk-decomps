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

===== bot.cpp ========================================================

  The Cow-Bots, part 1: the bot player (CBot) - its constructor and
  Spawn, the per-frame BotThink the deathmatch rules call, enemy and
  item choice, weapon choice and firing.

  The file name is ours (retail's object sits between bmodels.obj and
  the next bot object); so are all bot names.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"		// right after cbase.h, as in player.cpp: retail places CBasePlayer's
						// inline virtuals between FightThink and CanWalkTo (the .pch's lazy inlines)
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "items.h"
#include "gamerules.h"
#include "bot.h"


CBot::CBot() :
	m_fLookDirSet( FALSE ),
	m_vecMoveVelocity( Vector( 0, 0, 0 ) ),
	m_fDuckToggle( TRUE ),
	m_vecAimError( Vector( 0, 0, 0 ) ),
	m_vecLookDir( Vector( 0, 0, 0 ) ),
	m_flMoved( 0 ),
	m_flForwardMove( 0 ),
	m_flSideMove( 0 ),
	m_flUpMove( 0 ),
	m_flMsec( 0 ),
	m_cFrames( 0 ),
	m_iEnemy( 0 ),
	m_cRouteSteps( 0 ),
	m_pRoute( NULL ),
	m_iGoalItem( 0 ),
	m_flNextStrafe( gpGlobals->time ),
	m_flStrafeInterval( 1.0 ),
	m_flNextFrameCount( gpGlobals->time ),
	m_flNextEnemyCheck( gpGlobals->time ),
	m_flNextItemCheck( gpGlobals->time ),
	m_flItemRadius( 256 ),
	m_iDodgeDir( 0 ),
	m_fMelee( FALSE ),
	m_Fire( this ),
	m_Memory( this ),
	m_Profile( this )
{
}

CBot::~CBot()
{
}

//=========================================================
// Spawn - a bot is a player with FL_FAKECLIENT; the first
// spawn also makes its marker, an invisible dynamite model
// that BotThink keeps in front of its face
//=========================================================
void CBot::Spawn( void )
{
	CBasePlayer::Spawn();

	pev->flags |= FL_FAKECLIENT;

	if ( !m_pCamera )
	{
		entvars_t *pevMarker = VARS( CREATE_ENTITY() );
		CBaseEntity *pMarker = (CBaseEntity *)GET_PRIVATE( ENT( pevMarker ) );

		if ( pMarker == NULL )
		{
			pMarker = new( pevMarker ) CBaseEntity;
			pMarker->pev = pevMarker;
		}
		m_pCamera = pMarker;

		m_pCamera->pev->modelindex = MODEL_INDEX( "models/dynamite.mdl" );
		m_pCamera->pev->solid = SOLID_NOT;
		m_pCamera->pev->movetype = MOVETYPE_FLY;
		m_pCamera->pev->renderamt = 0;
		m_pCamera->pev->rendermode = kRenderTransTexture;
	}

	m_Memory.Forget();
	m_iEnemy = 0;
	ClearRoute();
	m_iState = 0;
	m_fHasGoalPos = FALSE;
}

//=========================================================
// PressFire - picks a better weapon when the knife or the
// pick is out (or under water), reloads an empty clip, and
// otherwise fires one time in five (every frame while
// m_Fire says so): secondary fire when m_Fire chose it or
// the primary ammo is gone
//=========================================================
void CBot::PressFire( void )
{
	if ( !m_pActiveItem )
		return;

	if ( !m_pActiveItem->CanDeploy() || FClassnameIs( m_pActiveItem->pev, "weapon_knife" ) || FClassnameIs( m_pActiveItem->pev, "weapon_pick" ) || pev->waterlevel > 1 )
		ChooseWeapon();

	CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)m_pActiveItem;

	if ( pWeapon->pszAmmo1() && pWeapon->m_iClip <= 0 && pWeapon->iMaxClip() != WEAPON_NOCLIP )
	{
		CheckReload();
		return;
	}

	if ( RANDOM_LONG( 1, 100 ) > 20 && !m_Fire.m_fKeepFiring )
		return;

	if ( pWeapon->m_fInReload )
		return;

	if ( m_Fire.m_fSecondary )
	{
		if ( !pWeapon->pszAmmo1() || pWeapon->iMaxClip() == WEAPON_NOCLIP || ( pWeapon->m_iSecondaryAmmoType > -1 && m_rgAmmo[ pWeapon->m_iSecondaryAmmoType ] ) || FClassnameIs( pWeapon->pev, "weapon_gattlinggun" ) )
		{
			pev->button |= IN_ATTACK2;
			return;
		}
	}
	else if ( pWeapon->pszAmmo2() && pWeapon->iMaxClip() != WEAPON_NOCLIP && !( pWeapon->m_iPrimaryAmmoType > -1 && m_rgAmmo[ pWeapon->m_iPrimaryAmmoType ] ) )
	{
		pev->button |= IN_ATTACK2;
		return;
	}

	pev->button |= IN_ATTACK;
}

//=========================================================
// ChooseWeapon - switches to the weapon the profile rates
// best at the enemy's distance (8192 without an enemy)
//=========================================================
void CBot::ChooseWeapon( void )
{
	CBasePlayerItem *pBest = NULL;
	int iBest = 0;

	if ( m_pActiveItem && !m_pActiveItem->CanHolster() )
		return;

	for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		CBasePlayerItem *pItem = m_rgpPlayerItems[i];

		while ( pItem )
		{
			float flDist = 8192;

			if ( HasEnemy() )
				flDist = ( GetEnemy()->pev->origin - pev->origin ).Length();

			float flRating = m_Profile.RateWeapon( this, pItem, flDist );

			if ( flRating > iBest && pItem->CanDeploy() )
			{
				iBest = flRating;
				pBest = pItem;
			}

			pItem = pItem->m_pNext;
		}
	}

	if ( pBest && pBest != m_pActiveItem && pev )
		SwitchWeapon( pBest );

	m_Fire.Update( m_pActiveItem );
}

//=========================================================
// CheckReload - presses reload when the clip isn't full
//=========================================================
BOOL CBot::CheckReload( void )
{
	CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)m_pActiveItem;

	if ( pWeapon && pWeapon->pszAmmo1() && pWeapon->iMaxClip() != WEAPON_NOCLIP && pWeapon->m_iClip != pWeapon->iMaxClip() )
	{
		pev->button |= IN_RELOAD;
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// Say - chat line from the bot (unused)
//=========================================================
void CBot::Say( char *pszText )
{
	char szText[256];

	sprintf( szText, "%s: %s\n", STRING( pev->netname ), pszText );
	UTIL_SayTextAll( szText, this );
}

//=========================================================
// BotThink - the bot's frame, run by the deathmatch rules
// for every fake client
//=========================================================
void CBot::BotThink( void )
{
	BeginFrame();

	if ( IsAlive() )
	{
		if ( GetEnemy() )
			CheckEnemy();
		if ( GetGoalItem() )
			CheckGoalItem();

		LookAround( m_flItemRadius );
		CheckMelee();
		FightThink();

		if ( !m_fLookDirSet )
			SetLookDir();
		MoveThink();
	}
	else
		DeadThink();

	m_Memory.Update( this );

	if ( m_pCamera )
	{
		MAKE_VECTORS( pev->v_angle );
		m_pCamera->pev->origin = pev->origin + pev->view_ofs - gpGlobals->v_forward * 12 + Vector( 0, 0, 16 );
		m_pCamera->pev->angles = UTIL_VecToAngles( pev->origin + gpGlobals->v_forward * 64 + Vector( 0, 0, 40 ) - m_pCamera->pev->origin );
		UTIL_SetOrigin( m_pCamera->pev, m_pCamera->pev->origin );
	}

	(*g_engfuncs.pfnRunPlayerMove)( edict(), pev->v_angle, GetForwardMove(), GetSideMove(), GetUpMove(), pev->button, 0, m_flMsec );
}

//=========================================================
// LookAround - every 0.3 s picks the nearest player it can
// see and notice as the enemy (the chicken's carrier
// first), unless the current one was seen in the last 3 s;
// every 0.3 s also looks at the entities within iRange
// units: runs from bear traps, scorpions, cannon balls and
// grenades, uses buttons within 64 units, and lists the
// items around its feet for PickGoalItem
//=========================================================
void CBot::LookAround( int iRange )
{
	int i;
	float flBest = 8192;
	BOOL fFlag;
	Vector vecDelta;

	if ( gpGlobals->time >= NextEnemyCheck() )
	{
		m_flNextEnemyCheck = gpGlobals->time + 0.3;

		if ( !HasEnemy() || gpGlobals->time - m_Memory.SeenTime() > 3 )
		{
			fFlag = FALSE;

			for ( i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );

				if ( IsValidEnemy( pPlayer ) )
				{
					if ( pPlayer != GetEnemy() )
					{
						float flDist = ( pPlayer->pev->origin - pev->origin ).Length();

						// the chicken's carrier always wins
						if ( pPlayer->m_pActiveItem && FClassnameIs( pPlayer->m_pActiveItem->pev, "ctc_chicken" ) )
						{
							flDist = 0;
							fFlag = TRUE;
						}

						if ( flDist < flBest )
						{
							flBest = flDist;
							m_iEnemy = ENTINDEX( pPlayer->edict() );
							m_iEnemySerial = pPlayer->edict()->serialnumber;
						}
					}
				}
			}

			if ( HasEnemy() && CanSee( GetEnemy() ) )
			{
				ChooseWeapon();
				m_vecAimError = Vector( RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ) );
				m_Memory.SetEnemy( GetEnemy(), fFlag );
			}
		}
	}

	m_pLink = NULL;

	if ( m_flNextItemCheck <= gpGlobals->time )
	{
		CBaseEntity *pList[100];

		m_flNextItemCheck = gpGlobals->time + 0.3;

		if ( m_flItemRadius < 256 )
			m_flItemRadius += 3;

		Vector vecRange( iRange, iRange, iRange );

		fFlag = FALSE;
		if ( m_flNextStrafe < gpGlobals->time && !HasNamedPlayerItem( "ctc_chicken" ) )
		{
			fFlag = TRUE;
			m_flNextStrafe = StrafeInterval() + gpGlobals->time;
		}

		int count = UTIL_EntitiesInBox( pList, 100, pev->origin - vecRange, pev->origin + vecRange, 0 );

		for ( i = 0; i < count; i++ )
		{
			CBaseEntity *pEntity = pList[i];

			if ( !pEntity || pEntity == this || !CanSee( pEntity ) || !CanNotice( pEntity ) )
				continue;

			vecDelta = pEntity->pev->origin - pev->origin;

			if ( FClassnameIs( pEntity->pev, "monster_beartrap" ) || FClassnameIs( pEntity->pev, "monster_scorpion" ) || FClassnameIs( pEntity->pev, "cannon_ball" ) || FClassnameIs( pEntity->pev, "grenade" ) )
			{
				MoveToEntity( pEntity, 6 );
			}
			else if ( FClassnameIs( pEntity->pev, "func_button" ) )
			{
				if ( vecDelta.Length() < 64 )
					pev->button |= IN_USE;
			}
			else if ( !HasGoalItem() && fFlag )
			{
				float flHeight = pev->origin.z - pEntity->pev->origin.z;

				if ( flHeight < 40 && flHeight > 0 )
				{
					pEntity->m_pLink = m_pLink;
					m_pLink = pEntity;
				}
			}
		}

		if ( !HasGoalItem() )
			PickGoalItem();
	}
}

//=========================================================
// PickGoalItem - rates the items LookAround listed and
// makes the best reachable one the goal item: any weapon
// (100) while it has no gun; otherwise ammo it can carry
// more of (1), weapons as the profile rates them, health
// (100 - health) and armour (100 - armour), weapon boxes
// (80), the other team's loose chicken (90) and its own
// capture point while carrying the chicken (100); the
// rating is scaled by the square of 256 - distance
//=========================================================
void CBot::PickGoalItem( void )
{
	float flBest = 0;
	CBaseEntity *pEntity = m_pLink;

	while ( pEntity )
	{
		float flRating = 0;

		if ( !HasGun() )
		{
			if ( ( pEntity->pev->effects & EF_NODRAW ) || strncmp( "weapon", STRING( pEntity->pev->classname ), 6 ) )
				goto next;

			if ( FStrEq( "weaponbox", STRING( pEntity->pev->classname ) ) )
			{
				pEntity->pev->owner = NULL;
				flRating = 100;
			}
			else
			{
				if ( ((CBasePlayerItem *)pEntity)->m_pPlayer )
					continue;	// sic: never steps to the next entity
				if ( !g_pGameRules->CanHavePlayerItem( this, (CBasePlayerItem *)pEntity ) )
					goto next;
				flRating = 100;
			}
		}
		else
		{
			if ( pEntity->pev->effects & EF_NODRAW )
				goto next;

			if ( !strncmp( "ammo_", STRING( pEntity->pev->classname ), 5 ) )
			{
				char szAmmo[32];

				strcpy( szAmmo, STRING( pEntity->pev->classname ) );
				int iAmmo = GetAmmoIndex( szAmmo + 5 );
				if ( iAmmo <= -1 )
					goto next;

				for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
				{
					for ( CBasePlayerItem *pItem = m_rgpPlayerItems[i]; pItem; pItem = pItem->m_pNext )
					{
						if ( pItem->pszAmmo1() && strstr( STRING( pEntity->pev->classname ), pItem->pszAmmo1() ) && AmmoInventory( iAmmo ) < pItem->iMaxAmmo1() )
							flRating = 1;
					}
				}
			}
			else if ( !strncmp( "weapon_", STRING( pEntity->pev->classname ), 7 ) )
			{
				if ( ((CBasePlayerItem *)pEntity)->m_pPlayer )
					continue;	// sic
				if ( !g_pGameRules->CanHavePlayerItem( this, (CBasePlayerItem *)pEntity ) )
					goto next;
				flRating = m_Profile.RateWeapon( this, (CBasePlayerItem *)pEntity, 8192 );
			}
			else if ( FClassnameIs( pEntity->pev, "item_healthkit" ) )
				flRating = 100.0 - pev->health;
			else if ( FClassnameIs( pEntity->pev, "weaponbox" ) )
			{
				pEntity->pev->owner = NULL;
				flRating = 80;
			}
			else if ( FClassnameIs( pEntity->pev, "item_battery" ) )
				flRating = 100 - pev->armorvalue;
			else
			{
				if ( FClassnameIs( pEntity->pev, "ctc_chicken" ) && strcmp( pEntity->TeamID(), TeamID() ) && !pEntity->pev->owner )
					flRating = 90;
				else if ( FClassnameIs( pEntity->pev, "ctc_capturepoint" ) && !strcmp( pEntity->TeamID(), TeamID() ) && HasNamedPlayerItem( "ctc_chicken" ) )
					flRating = 100;
				else
					goto next;
			}
		}

		if ( flRating > 0 && CanWalkTo( pEntity->Center() ) )
		{
			float flDist = 256 - ( pEntity->pev->origin - pev->origin ).Length();

			flRating = flDist * flDist * flRating;
			if ( flRating > flBest )
			{
				flBest = flRating;
				SetGoalItem( pEntity );
				m_flGiveUpItemTime = gpGlobals->time + 5.0;
			}
		}
next:
		pEntity = pEntity->m_pLink;
	}

	m_pLink = NULL;
}

// IsValidEnemy - whether a player may become the enemy: alive, seen,
// hurtable, noticed, not a teammate, and only while the bot doesn't carry
// the chicken (name ours); an inline step of LookAround, which takes its
// share of LookAround's inline budget
inline BOOL CBot::IsValidEnemy( CBasePlayer *pPlayer )
{
	return pPlayer && pPlayer != this && pPlayer->IsAlive() && CanSee( pPlayer ) && pPlayer->pev->takedamage != DAMAGE_NO
		&& !HasNamedPlayerItem( "ctc_chicken" ) && g_pGameRules->PlayerRelationship( this, pPlayer ) == GR_NOTTEAMMATE && CanNotice( pPlayer );
}

//=========================================================
// HasGun - anything in weapon slots 2 or 3
//=========================================================
BOOL CBot::HasGun( void )
{
	for ( int i = 2; i < 4; i++ )
	{
		if ( m_rgpPlayerItems[i] )
			return TRUE;
	}
	return FALSE;
}

//=========================================================
// CanNotice - in view, or a player making more noise than
// the bot's notice skill ignores, or by chance: likelier
// nearer and with a better notice skill
//=========================================================
BOOL CBot::CanNotice( CBaseEntity *pEntity )
{
	if ( FInViewCone( pEntity ) )
		return TRUE;

	if ( pEntity->IsPlayer() && ((CBasePlayer *)pEntity)->m_iTargetVolume >= 1000 - m_Profile.NoticeSkill() * 10 )
		return TRUE;

	float flDist = Vector( pev->origin - pEntity->pev->origin ).Length();

	if ( flDist > 8192 )
		flDist = 8192;
	else if ( flDist < 32 )
		flDist = 32;

	float flRatio = 8192 / flDist;
	float flSkill = m_Profile.NoticeSkill();
	float flRandom = RANDOM_FLOAT( 1, 65536 );
	flSkill = flSkill / 100 - 0.5;

	if ( flRandom < flRatio * flRatio + ( flSkill + flSkill ) * 65536 )
		return TRUE;

	return FALSE;
}

//=========================================================
// CanSee
//=========================================================
BOOL CBot::CanSee( CBaseEntity *pEntity )
{
	if ( pEntity && pEntity != this && pEntity->pev )
		return FVisible( pEntity );

	return FALSE;
}

//=========================================================
// BeginFrame - BotThink's first half (name ours): counts
// the frame, clears the frame's move goals and buttons,
// measures how far the bot moved, and when it is stuck
// drops its goal and turns towards an open side; when it
// makes no progress towards its goal position for 8 s it
// gives the position up
//=========================================================
// An inline step, as the move goals' are: it takes almost all
// of BotThink's inline budget (BotThink's own size 471, so
// 1000; BeginFrame 959), so neither its calls nor BotThink's
// later ones are expanded, as in retail (only the free ones
// of at most 40, edict() and Vector's float * conversion).
// Defined after its caller (VC6 still expands it there), as are
// IsValidEnemy and CountFrames: retail's bot.obj has the
// out-of-line Length and Length2D after PickGoalItem, which
// leaves no room for these bodies before LookAround in the
// file (the .pch's inlines are compiled as the file's own
// code advances; PLAN.md, "Image identity").
inline void CBot::BeginFrame( void )
{
	int fRightBlocked, fLeftBlocked;

	CountFrames();

	m_fLookDirSet = FALSE;
	m_cMoveGoals = 0;
	m_iMovePriority = 0;
	pev->button = 0;

	float flMoved = ( m_Memory.GetOrigin() - pev->origin ).Length();
	m_flMoved = flMoved;

	MAKE_VECTORS( pev->angles );

	if ( gpGlobals->time > 3.0 )
		m_fBlocked = CanWalkTo( pev->origin + gpGlobals->v_forward * 32 );

	if ( gpGlobals->time > m_flPauseTime + 1.0 )
	{
		if ( IsOnLadder() && !m_iLadderDir )
			pev->button |= IN_JUMP;

		if ( m_flMoved <= 1.0 && ( pev->flags & FL_ONGROUND ) )
		{
			// stuck
			g_BotNavigator.AddVisit2( pev->origin );

			if ( !m_fBlocked && !m_fJump && !m_iLadderDir && m_flNextStuckCheck < gpGlobals->time )
			{
				m_flNextStuckCheck = gpGlobals->time + 1.5;

				if ( GetGoalItem() )
					SetGoalItem( NULL );
				else
					m_fHasGoalPos = FALSE;

				if ( gpGlobals->time - m_flLastMoveTime > 3.0 )
				{
					// turn towards the open side
					Vector vecOrigin( pev->origin );
					Vector vecSpot;

					vecSpot = pev->origin + gpGlobals->v_right * 16 + gpGlobals->v_forward * 64;
					pev->origin = pev->origin + gpGlobals->v_right * 16;
					// sic: retail's source leaves each flag unset when the side is
					// visible, and VC6 then reads the stack slot it shares with
					// the spot just tested (vecSpot.x here, vecSpot.y below): a
					// visible side counts as blocked unless that coordinate is
					// +0.0, so the bot nearly always turns 180 degrees. Written
					// out so that this build does the same.
					if ( !FVisible( vecSpot ) )
						fRightBlocked = TRUE;
					else
						fRightBlocked = *(int *)&vecSpot.x;
					pev->origin = vecOrigin;

					vecSpot = pev->origin - gpGlobals->v_right * 16 + gpGlobals->v_forward * 64;
					pev->origin = pev->origin - gpGlobals->v_right * 16;
					if ( !FVisible( vecSpot ) )
						fLeftBlocked = TRUE;
					else
						fLeftBlocked = *(int *)&vecSpot.y;	// sic, as above
					pev->origin = vecOrigin;

					if ( !fRightBlocked )
						m_vecMoveAngles.y += 15;
					else if ( !fLeftBlocked )
						m_vecMoveAngles.y -= 15;
					else
						m_vecMoveAngles.y += 180;

					m_flLastMoveTime = gpGlobals->time;
				}
			}
		}
		else if ( !m_iState )
		{
			if ( m_fHasGoalPos && !GetGoalItem() && !GetEnemy() )
			{
				if ( ( GetGoalPos() - pev->origin ).Length2D() < m_flGoalDist )
					m_flGoalDist = ( GetGoalPos() - pev->origin ).Length2D();
				else if ( gpGlobals->time - m_flGoalTime > 8.0 )
				{
					pev->v_angle.y = -pev->v_angle.y;
					m_fHasGoalPos = FALSE;
				}
			}
			m_flLastMoveTime = gpGlobals->time;
		}
	}
}

// CountFrames - counts the frames of each second into the
// command's msec (1-100); called once per frame by BotThink
// (out of line there). Defined here, not in bot.h: retail's
// out-of-line copy follows CanSee, before every player.h
// inline the file emits, which a bot.h body (after player.h
// in the .pch) can't do
inline void CBot::CountFrames( void )
{
	if ( NextFrameCount() <= gpGlobals->time )
	{
		m_flNextFrameCount = gpGlobals->time + 1;
		if ( m_cFrames > 0 )
			m_flMsec = 1000 / m_cFrames;
		m_cFrames = 0;
	}
	else
		m_cFrames++;

	if ( m_flMsec < 1 )
		m_flMsec = 1;
	if ( m_flMsec > 100 )
		m_flMsec = 100;
}

//=========================================================
// DeadThink - presses fire (to respawn) one frame in five
// and forgets everything
//=========================================================
void CBot::DeadThink( void )
{
	if ( RANDOM_LONG( 0, 4 ) == 0 )
		pev->button |= IN_ATTACK;

	ClearRoute();
	m_vecMoveVelocity = Vector( 0, 0, 0 );
	m_fDuckToggle = TRUE;
	m_vecLookDir = Vector( 0, 0, 0 );
	m_flMoved = 0;
	m_flForwardMove = 0;
	m_flSideMove = 0;
	m_flUpMove = 0;
	m_iEnemy = 0;
	m_iGoalItem = 0;
	m_fHasGoalPos = FALSE;
	float flTime = gpGlobals->time;
	m_flNextStrafe = flTime;
	m_iDodgeDir = 0;
	m_fMelee = FALSE;
}

//=========================================================
// CheckMelee - decides whether to close in: always with the
// knife or the pick; otherwise when health and armour beat
// the enemy's (the aggression skill counting from 50), or,
// without an enemy, when they exceed 140 - aggression and
// it has a gun
//=========================================================
void CBot::CheckMelee( void )
{
	CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)m_pActiveItem;

	if ( !pWeapon )
		return;

	if ( pWeapon->m_fInReload )
	{
		m_fMelee = FALSE;
		return;
	}

	if ( !FClassnameIs( pWeapon->pev, "weapon_knife" ) && !FClassnameIs( pWeapon->pev, "weapon_pick" ) )
	{
		if ( HasEnemy() )
		{
			CBaseEntity *pEnemy = GetEnemy();
			if ( ( m_Profile.m_flAggression - 50 ) + ( pev->armorvalue + pev->health ) <= pEnemy->pev->armorvalue + pEnemy->pev->health )
			{
				m_fMelee = FALSE;
				return;
			}
		}
		else if ( pev->armorvalue + pev->health <= 140 - m_Profile.Aggression() || !HasGun() )
		{
			m_fMelee = FALSE;
			return;
		}
	}

	m_fMelee = TRUE;
}

// JumpSkill - the jump skill scaled by the bot list's skill (name ours);
// an inline CBotProfile member, as AimSkill and NoticeSkill are: retail
// computes &m_Profile first and reads m_flJump through it
inline float CBotProfile::JumpSkill( void )
{
	return SkillAdjust( ( g_BotManager.m_flSkillScale / 100 ) * m_flJump );
}

//=========================================================
// FightThink - with a live enemy: aims and fires when it
// can see it (and sometimes pauses 1-3 s), and picks the
// move goal; without one, reloads and heads for the goal
// item or roams. Jumps now and then.
//=========================================================
void CBot::FightThink( void )
{
	if ( HasEnemy() && GetEnemy()->pev->deadflag == DEAD_NO )
	{
		if ( CanSee( GetEnemy() ) )
		{
			AimAtEnemy();
			PressFire();
			m_iState = 0;
			m_Memory.SawEnemy( GetEnemy() );

			if ( !RANDOM_LONG( 0, m_Profile.SkillAdjust( m_Profile.m_flPause ) * 10 ) )
				m_flPauseTime = RANDOM_LONG( 1, 3 ) + gpGlobals->time;
		}
		else
		{
			if ( m_Memory.m_fSeen && !CheckReload() )
				ChooseWeapon();
			m_Memory.LostEnemy();
		}

		float flJumpChance = 4000;

		if ( !m_fMelee && m_Memory.m_fSeen && !m_Memory.m_fChickenCarrier )
		{
			flJumpChance = 2000;
			MoveToEntity( GetEnemy(), 6 );
		}
		else if ( !HasGun() && HasGoalItem() && !m_Memory.m_fChickenCarrier )
		{
			flJumpChance = 1000;
			MoveToItem( GetGoalItem(), 1 );
		}
		else
		{
			if ( !m_Memory.m_fSeen || m_Memory.m_fChickenCarrier )
			{
				if ( m_iState != 3 )
				{
					ClearRoute();
					m_iState = 3;
					m_fHasGoalPos = FALSE;
				}
			}

			if ( m_iState != 3 && m_pActiveItem && ( FClassnameIs( m_pActiveItem->pev, "weapon_knife" ) || FClassnameIs( m_pActiveItem->pev, "weapon_knife" ) ) )	// sic: the pick was meant
				MoveToEntityOffset( GetEnemy(), 3, Vector( 0, 0, 0 ) );
			else
				Roam( 0 );
		}

		float flJump = m_Profile.JumpSkill();
		if ( RANDOM_FLOAT( 1, flJumpChance ) < flJump * 1.5 )
			pev->button |= IN_JUMP;
	}
	else
	{
		CheckReload();

		if ( HasGoalItem() )
			MoveToItem( GetGoalItem(), 1 );
		else
			Roam( 0 );
	}
}

//=========================================================
// CheckEnemy - forgets a dead or invulnerable enemy, one
// not seen for (aggression - 50) / 10 + 10 seconds, or any
// enemy while it carries the chicken
//=========================================================
void CBot::CheckEnemy( void )
{
	if ( !HasEnemy() )
		return;

	if ( GetEnemy()->pev->deadflag == DEAD_NO )
	{
		if ( GetEnemy()->pev->takedamage != DAMAGE_NO && gpGlobals->time - m_Memory.SeenTime() <= ( m_Profile.m_flAggression - 50 ) / 10 + 10 && !HasNamedPlayerItem( "ctc_chicken" ) )
			return;
	}

	if ( m_iState == 3 )
		m_iState = 0;
	m_iEnemy = 0;
}

//=========================================================
// CheckGoalItem - drops a goal item it can't see, that was
// picked up or hidden, or (when it can't walk on) that it
// didn't reach in time, shrinking its search radius to
// the item's distance; a capture point stays the goal only
// while it carries the chicken
//=========================================================
// GiveUpItemTime - when CheckGoalItem gives up on an item it can see
// (name ours): an inline accessor, as NextEnemyCheck is
inline float CBot::GiveUpItemTime( void )
{
	return m_flGiveUpItemTime;
}

void CBot::CheckGoalItem( void )
{
	if ( !HasGoalItem() )
		return;

	if ( FVisible( GetGoalItem() ) && !( GetGoalItem()->pev->effects & EF_NODRAW ) && !GetGoalItem()->pev->owner )
	{
		if ( m_flGiveUpItemTime < gpGlobals->time )
		{
			if ( !m_fBlocked )
			{
				float flDist = ( GetGoalItem()->pev->origin - pev->origin ).Length() - 16;
				if ( flDist <= 32 )
					flDist = 32;
				m_flItemRadius = flDist;
				pev->v_angle.y = -pev->v_angle.y;
			}
			else
			{
				m_flGiveUpItemTime = GiveUpItemTime() + 5.0;
				return;
			}
		}
		else if ( !FClassnameIs( GetGoalItem()->pev, "ctc_capturepoint" ) || HasNamedPlayerItem( "ctc_chicken" ) )
			return;
	}

	SetGoalItem( NULL );
}

//=========================================================
// CanWalkTo - whether WALK_MOVE can step there in 16-unit
// steps (the bot is put back afterwards)
//=========================================================
BOOL CBot::CanWalkTo( const Vector &vecDest )
{
	BOOL fResult = TRUE;
	Vector vecOrigin = pev->origin;
	int iFlags = pev->flags;

	UTIL_SetOrigin( pev, vecOrigin );

	if ( pev->waterlevel > 1 )
		pev->flags |= FL_SWIM;

	float flDist = ( vecDest - vecOrigin ).Length2D();
	float flYaw = UTIL_VecToYaw( vecDest - vecOrigin );

	for ( float fl = 0; fl < flDist; fl += 16 )
	{
		float flStep = 16;

		if ( fl + 16 >= flDist - 1 )
			flStep = flDist - fl - 1;

		if ( !WALK_MOVE( ENT( pev ), flYaw, flStep, WALKMOVE_CHECKONLY ) )
		{
			fResult = FALSE;
			break;
		}
	}

	UTIL_SetOrigin( pev, vecOrigin );
	pev->flags = iFlags;

	return fResult;
}
