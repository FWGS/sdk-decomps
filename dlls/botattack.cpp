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

===== botattack.cpp ========================================================

  The Cow-Bots, part 2: the bot's weapon handling (CBotFire, how each
  weapon is fired) and its memory of the enemy (CBotMemory, positions
  and velocities over the last frames, for leading shots).

  The file and class names are ours.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "items.h"
#include "gamerules.h"
#include "bot.h"

//=========================================================
// CBotFire - how the bot uses the weapon in its hands
//=========================================================

CBotFire::CBotFire( CBot *pBot )
{
	m_iAimMode = 1;
	m_fKeepFiring = FALSE;
	m_fSecondary = FALSE;
	m_pBot = pBot;
}

CBotFire::~CBotFire()
{
}

inline void CBotFire::SetAimMode( int iMode )
{
	m_iAimMode = iMode;
}

//=========================================================
// AimBow - Update's bow branch (name ours): the secondary
// attack by the enemy's distance, then aim mode 2 unless
// the secondary was chosen
//=========================================================
// An inline step: retail reads m_pBot afresh here and keeps
// it across GetEnemy (Update's own pBot is another register),
// and the step's share of Update's budget is thin enough
// that GetEnemy (78) is expanded but the distance's
// operator- and Length are not (share 78-136; 101 with the
// SetAimMode sites of the three later branches, 158 without).
inline void CBotFire::AimBow( void )
{
	CBot *pBot = m_pBot;

	float flDist = ( pBot->GetEnemy()->pev->origin - pBot->pev->origin ).Length();
	if ( flDist > 700 )
		RollSecondary( 90 );
	else if ( flDist > 200 )
		RollSecondary( 60 );
	else
		RollSecondary( 20 );

	if ( m_fSecondary )
		RollAimMode( 60 );
	else
		SetAimMode( 2 );
}

//=========================================================
// Update - pick how to aim and fire the weapon the bot
// holds: melee weapons and guns roll the aim mode, lobbed
// weapons (cannon, dynamite, scorpions, bear trap, the bow
// without its secondary) aim with mode 2.
//=========================================================
void CBotFire::Update( CBasePlayerItem *pItem )
{
	CBot *pBot = m_pBot;

	m_fKeepFiring = FALSE;
	m_fSecondary = FALSE;

	if ( !pItem )
		return;

	strncmp( STRING( pItem->pev->classname ), "weapon_", 7 );	// sic: the result is not used

	if ( FClassnameIs( pItem->pev, "weapon_knife" ) || FClassnameIs( pItem->pev, "weapon_pick" ) )
	{
		RollAimMode( 80 );
		if ( ( pBot->GetEnemy()->pev->origin - pBot->pev->origin ).Length() < 90 )
			m_fKeepFiring = TRUE;
	}
	else if ( FClassnameIs( pItem->pev, "weapon_pistol" ) )
	{
		RollAimMode( pBot->m_Profile.AimSkill() );
	}
	else if ( FClassnameIs( pItem->pev, "weapon_shotgun" ) )
	{
		if ( ( pBot->GetEnemy()->pev->origin - pBot->pev->origin ).Length() > 700 )
			RollSecondary( 20 );
		else
			RollSecondary( 90 );
		RollAimMode( 40 );
	}
	else if ( FClassnameIs( pItem->pev, "weapon_gattlinggun" ) )
	{
		RollAimMode( pBot->m_Profile.AimSkill() );
		m_fKeepFiring = TRUE;
		if ( gpGlobals->time > ((CBasePlayerWeapon *)pItem)->m_flNextPrimaryAttack + 0.5 )
			RollSecondary( 50 );
	}
	else if ( FClassnameIs( pItem->pev, "weapon_winchester" ) || FClassnameIs( pItem->pev, "weapon_baffalogun" ) || FClassnameIs( pItem->pev, "weapon_colts" ) )
	{
		RollAimMode( pBot->m_Profile.AimSkill() );
	}
	else if ( FClassnameIs( pItem->pev, "weapon_cannon" ) )
	{
		SetAimMode( 2 );
	}
	else if ( FClassnameIs( pItem->pev, "weapon_bow" ) )
	{
		AimBow();
	}
	else if ( FClassnameIs( pItem->pev, "weapon_scorpion" ) )
	{
		SetAimMode( 2 );
		m_fKeepFiring = TRUE;
	}
	else if ( FClassnameIs( pItem->pev, "weapon_dynamite" ) )
	{
		SetAimMode( 2 );
	}
	else if ( FClassnameIs( pItem->pev, "weapon_beartrap" ) )
	{
		SetAimMode( 2 );
		m_fKeepFiring = TRUE;
	}
	else if ( !FClassnameIs( pItem->pev, "ctc_chicken" ) )
	{
		ALERT( at_aiconsole, "Unidentified Weapon!\n" );
	}

	if ( strncmp( STRING( pItem->pev->classname ), "weapon_", 7 ) )
		m_iClip = ((CBasePlayerWeapon *)pItem)->m_iClip;
}

//=========================================================
// RollAimMode - mode 1 unless a roll of 0-100 under
// iChance, or a skill-adjusted aim of 99 or more, gives 0
//=========================================================
void CBotFire::RollAimMode( int iChance )
{
	CBot *pBot = m_pBot;

	if ( RANDOM_LONG( 0, 100 ) >= iChance && pBot->m_Profile.AimSkill() < 99 )
		m_iAimMode = 1;
	else
		m_iAimMode = 0;
}

//=========================================================
// RollSecondary - use the secondary attack iChance times in
// 100
//=========================================================
void CBotFire::RollSecondary( int iChance )
{
	if ( RANDOM_LONG( 0, 100 ) < iChance )
		m_fSecondary = TRUE;
	else
		m_fSecondary = FALSE;
}

//=========================================================
// CBotMemory - the enemy last seen, and the last four
// frames of the enemy's velocity and the bot's move
// direction and velocity (for leading shots)
//=========================================================

CBotMemory::CBotMemory( CBot *pBot ) :
	m_flTime( gpGlobals->time ),
	m_fSeen( FALSE ),
	m_vecEnemyOrigin( Vector( 0, 0, 0 ) ),
	m_flSeenTime( gpGlobals->time ),
	m_vecEnemyVelocity( Vector( 0, 0, 0 ) ),
	m_vecLookDir( Vector( 0, 0, 0 ) ),
	m_vecVelocity( Vector( 0, 0, 0 ) ),
	m_vecEnemyVelocity1( Vector( 0, 0, 0 ) ),
	m_vecLookDir1( Vector( 0, 0, 0 ) ),
	m_vecVelocity1( Vector( 0, 0, 0 ) ),
	m_vecEnemyVelocity2( Vector( 0, 0, 0 ) ),
	m_vecLookDir2( Vector( 0, 0, 0 ) ),
	m_vecVelocity2( Vector( 0, 0, 0 ) ),
	m_vecEnemyVelocity3( Vector( 0, 0, 0 ) ),
	m_vecLookDir3( Vector( 0, 0, 0 ) ),
	m_vecVelocity3( Vector( 0, 0, 0 ) ),
	m_vecOrigin( Vector( 0, 0, 0 ) ),
	m_pBot( pBot )
{
}

CBotMemory::~CBotMemory()
{
}

void CBotMemory::Forget( void )
{
	CBot *pBot = m_pBot;

	m_fSeen = FALSE;
	m_flSeenTime = 0;
	if ( pBot->m_iState == 3 )
		pBot->m_iState = 0;
}

void CBotMemory::SawEnemy( CBaseEntity *pEnemy )
{
	m_fSeen = TRUE;
	float flTime = gpGlobals->time;
	m_flSeenTime = flTime;
	m_vecEnemyOrigin = pEnemy->pev->origin;
}

void CBotMemory::LostEnemy( void )
{
	m_fSeen = FALSE;
}

//=========================================================
// SetEnemy - a new enemy: its velocity fills the history
//=========================================================
void CBotMemory::SetEnemy( CBaseEntity *pEnemy, int fChickenCarrier )
{
	SawEnemy( pEnemy );
	m_fChickenCarrier = fChickenCarrier;
	float flTime = gpGlobals->time;
	m_flTime = flTime;
	m_vecEnemyVelocity = pEnemy->pev->velocity;
	m_vecEnemyVelocity1 = Vector( m_vecEnemyVelocity );
	m_vecEnemyVelocity2 = Vector( m_vecEnemyVelocity1 );
	m_vecEnemyVelocity3 = Vector( m_vecEnemyVelocity2 );
}

//=========================================================
// Update's look-direction accessors (names ours): retail
// copies each look direction through the out-of-line copy
// constructor into a temporary (an accessor's return copy,
// read back from the temporary), the velocities inline.
// Update's budget (1000) would expand them all; a depth
// limit where the accessors are defined does it (VC6 takes
// a nested call's limit from the callee's definition).
//=========================================================
#pragma inline_depth( 1 )	// sic: the accessors' return copies stay out of line
inline Vector CBotMemory::GetLookDir( void ) { return m_vecLookDir; }
inline Vector CBotMemory::GetLookDir1( void ) { return m_vecLookDir1; }
inline Vector CBotMemory::GetLookDir2( void ) { return m_vecLookDir2; }
inline Vector CBot::GetLookDir( void ) { return m_vecLookDir; }
#pragma inline_depth()	// sic: back to the default (8)

//=========================================================
// Update - shift the history by a frame (BotThink, every
// frame)
//=========================================================
void CBotMemory::Update( CBot *pBot )
{
	m_vecOrigin = pBot->pev->origin;

	m_vecLookDir3 = GetLookDir2();
	m_vecLookDir2 = GetLookDir1();
	m_vecLookDir1 = GetLookDir();
	m_vecLookDir = pBot->GetLookDir();

	m_vecVelocity3 = Vector( m_vecVelocity2 );
	m_vecVelocity2 = Vector( m_vecVelocity1 );
	m_vecVelocity1 = Vector( m_vecVelocity );
	m_vecVelocity = pBot->pev->velocity;

	if ( pBot->HasEnemy() )
	{
		m_vecEnemyVelocity3 = Vector( m_vecEnemyVelocity2 );
		m_vecEnemyVelocity2 = Vector( m_vecEnemyVelocity1 );
		m_vecEnemyVelocity1 = Vector( m_vecEnemyVelocity );
		m_vecEnemyVelocity = pBot->GetEnemy()->pev->velocity;
	}
}
