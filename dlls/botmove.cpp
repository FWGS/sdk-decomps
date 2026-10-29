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

===== botmove.cpp ========================================================

  The Cow-Bots, part 3: the bot list (g_BotManager: skill adjustment,
  names, profiles; adding and removing bots as fake clients), aiming,
  and movement (obstacle avoidance, strafing, ladders, jumping and
  ducking, the move goals that BotThink combines).

  The file and all names are ours.

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

// the bots' names and models when no profile file is free; the last
// entry's name is the buffer BotPickName prints "Cowbot<n>" into
botname_t g_rgBotNames[] =
{
	{ "H0GTIED",			"rogan" },
	{ "Crispen",			"crispen" },
	{ "Maverick",			"rogan" },
	{ "Whiplash",			"cowboy" },
	{ "LawBreaker",			"ramone" },
	{ "Rogan",				"rogan" },
	{ "Annie",				"annie" },
	{ "Billy The Kid",		"ramone" },
	{ "Bigbear",			"bear" },
	{ "BadBoY",				"rogan" },
	{ "Bandit",				"ramone" },
	{ "Neal",				"dyndave" },
	{ "sixShooter",			"twnwest" },
	{ "Gunslinger",			"twnwest" },
	{ "DangerousDeputy",	"hoss" },
	{ "KrazyCowboy",		"cowboy" },
	{ "El Bandito",			"bandit_mex" },
	{ "Montanna",			"hoss" },
	{ "Colonel Crispy",		"colonel" },
	{ "RavenShadow",		"nagatow" },
	{ "Longhorn",			"kaiewi" },
	{ "OldMan",				"bandit_mex" },
	{ "FrIskYCoW",			"kaiewi" },
	{ "Ben",				"townie" },
	{ "Sue",				"annie" },
	{ "Lampshade",			"townie" },
	{ "Slippery_Monkey",	"kaiewi" },
	{ "MorningStar",		"masala" },
	{ "GoldDigger!",		"miner" },
	{ "Dynamite Dave",		"dyndave" },
	{ "Big Jim",			"crispen" },
	{ "Sally",				"annie" },
	{ "Cowbot99999",		"rogan" },
};

#define NUM_BOT_NAMES	32

CBotManager g_BotManager;
float g_flBotQueueTime;		// the deathmatch rules' next bot add/remove

CBotManager::CBotManager()
{
	m_flNextAdjust = 12;
	m_flSkillScale = 50;
	strcpy( m_szLastProfile, "NoBotLastUsed" );
}

CBotManager::~CBotManager()
{
}

//=========================================================
// CanSeeSpot - nothing between the bot's eyes and the spot
// (monsters and glass block)
//=========================================================
BOOL CBot::CanSeeSpot( const Vector &vecSpot )
{
	TraceResult tr;

	gpGlobals->trace_flags = 1;	// FTRACE_SIMPLEBOX
	UTIL_TraceLine( EyePosition(), vecSpot, dont_ignore_monsters, dont_ignore_glass, ENT( pev ), &tr );

	if ( tr.flFraction != 1.0 )
		return FALSE;
	return TRUE;
}

//=========================================================
// FVisible - from the bot's eyes, through monsters and glass
//=========================================================
BOOL CBot::FVisible( const Vector &vecOrigin )
{
	TraceResult tr;

	UTIL_TraceLine( EyePosition(), vecOrigin, ignore_monsters, ignore_glass, ENT( pev ), &tr );

	if ( tr.flFraction != 1.0 )
		return FALSE;
	return TRUE;
}

BOOL CBot::FVisible( CBaseEntity *pEntity )
{
	TraceResult tr;

	if ( FBitSet( pEntity->pev->flags, FL_NOTARGET ) )
		return FALSE;

	UTIL_TraceLine( EyePosition(), pEntity->EyePosition(), ignore_monsters, ignore_glass, ENT( pev ), &tr );

	if ( tr.flFraction != 1.0 )
		return FALSE;
	return TRUE;
}

//=========================================================
// AdjustSkill - every 12 seconds, with sv_adjbotskill on and
// people playing: sv_botskill goes up a point when the bots'
// frags per death are below the people's, down a point when
// above (0-100)
//=========================================================
void CBotManager::AdjustSkill( void )
{
	if ( sv_adjbotskill.value == 0 )
		return;

	if ( m_flNextAdjust > gpGlobals->time )
		return;

	BOOL fHumans = FALSE;
	float flBotFrags = 1;
	float flBotDeaths = 1;
	float flHumanFrags = 1;
	float flHumanDeaths = 1;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

		if ( FBitSet( pPlayer->pev->flags, FL_FAKECLIENT ) )
		{
			flBotFrags = flBotFrags + pPlayer->pev->frags - pPlayer->pev->noise;	// sic: noise
			flBotDeaths = pPlayer->m_iDeaths + flBotDeaths;
		}
		else if ( pPlayer->IsNetClient() )
		{
			fHumans = TRUE;
			flHumanFrags = flHumanFrags + pPlayer->pev->frags;
			flHumanDeaths = pPlayer->m_iDeaths + flHumanDeaths;
		}
	}

	if ( !fHumans )
		return;

	if ( flBotFrags / flBotDeaths < flHumanFrags / flHumanDeaths )
	{
		if ( sv_botskill.value < 100 )
			CVAR_SET_FLOAT( "sv_botskill", sv_botskill.value + 1 );
	}
	else if ( flBotFrags / flBotDeaths > flHumanFrags / flHumanDeaths )
	{
		if ( sv_botskill.value > 0 )
			CVAR_SET_FLOAT( "sv_botskill", sv_botskill.value - 1 );
	}

	m_flNextAdjust += 12;
}

//=========================================================
// BotPickName - the first name in the table no player
// has, or else Cowbot1, Cowbot2... (the random number is
// thrown away)
//=========================================================
botname_t *BotPickName( void )
{
	int i, j;
	BOOL fUsed;
	BOOL fFound = FALSE;

	RANDOM_LONG( 0, 31 );	// sic: unused

	for ( i = 0; i < NUM_BOT_NAMES; i++ )
	{
		fUsed = FALSE;
		for ( j = 1; j < gpGlobals->maxClients; j++ )	// sic: skips the last slot
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( j );

			if ( pPlayer && !stricmp( STRING( pPlayer->pev->netname ), g_rgBotNames[i].pszName ) )
			{
				fUsed = TRUE;
				break;
			}
		}

		if ( !fUsed )
		{
			fFound = TRUE;
			break;
		}
	}

	if ( !fFound )
	{
		i = 0;
		do
		{
			i++;
			sprintf( g_rgBotNames[NUM_BOT_NAMES].pszName, "Cowbot%i", i );

			fUsed = FALSE;
			for ( j = 1; j < gpGlobals->maxClients; j++ )	// sic
			{
				CBaseEntity *pPlayer = UTIL_PlayerByIndex( j );

				if ( pPlayer && !stricmp( STRING( pPlayer->pev->netname ), g_rgBotNames[NUM_BOT_NAMES].pszName ) )
				{
					fUsed = TRUE;
					break;
				}
			}
		} while ( fUsed );

		return &g_rgBotNames[NUM_BOT_NAMES];
	}

	return &g_rgBotNames[i];
}

//=========================================================
// BotFindProfile - the first wantedhl/bots/*.bot whose name
// no bot in the game has goes into g_BotManager's
// m_szLastProfile; FALSE when all are taken
//=========================================================
BOOL BotFindProfile( void )
{
	BOOL fFound;
	char szName[100];
	WIN32_FIND_DATA fd;
	HANDLE hFind;
	int i;

	hFind = FindFirstFile( "wantedhl/bots/*.bot", &fd );
	if ( !hFind )	// sic: it fails with INVALID_HANDLE_VALUE
		return FALSE;

	strncpy( szName, fd.cFileName, strlen( fd.cFileName ) - 4 );
	fFound = FALSE;
	szName[strlen( fd.cFileName ) - 4] = 0;

	while ( 1 )
	{
		for ( i = 1; i < gpGlobals->maxClients; i++ )	// sic: skips the last slot
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

			if ( pPlayer && FBitSet( pPlayer->pev->flags, FL_FAKECLIENT ) && !stricmp( STRING( pPlayer->pev->netname ), szName ) )
				goto used;
		}

		fFound = TRUE;
		strcpy( g_BotManager.m_szLastProfile, szName );
		break;

used:
		if ( !FindNextFile( hFind, &fd ) )
			break;

		strncpy( szName, fd.cFileName, strlen( fd.cFileName ) - 4 );
		szName[strlen( fd.cFileName ) - 4] = 0;
	}

	FindClose( hFind );
	return fFound;
}

//=========================================================
// BotInit - the fake client's player entity becomes a
// CBot; its model and colours come from the profile file
// BotFindProfile chose, or else from pName (a free name
// from the table when pName is NULL) with random colours.
//=========================================================
void BotInit( edict_t *pEdict, botname_t *pName )
{
	char szModel[32] = "gordon";
	char *pszTopColor = "255";		// sic: both colours write one pooled "255", so
	char *pszBottomColor = "255";	// the last one written is both colours

	CBot *pBot = GetClassPtr( (CBot *)&pEdict->v );
	pBot->Spawn();

	if ( !pName )
	{
		char szFile[132] = "";

		sprintf( szFile, "wantedhl/bots/%s.bot", g_BotManager.m_szLastProfile );
		if ( pBot->m_Profile.Load( szFile ) )
		{
			strcpy( szModel, pBot->m_Profile.m_szModel );
			strcpy( pszBottomColor, pBot->m_Profile.m_szBottomColor );
			strcpy( pszTopColor, pBot->m_Profile.m_szTopColor );
			goto setkeys;
		}

		pName = BotPickName();
		if ( !pName )
			goto setkeys;
	}

	strcpy( szModel, pName->pszModel );
	sprintf( pszTopColor, "%d", RANDOM_LONG( 1, 254 ) );
	sprintf( pszBottomColor, "%d", RANDOM_LONG( 1, 254 ) );

setkeys:
	(*g_engfuncs.pfnSetClientKeyValue)( ENTINDEX( pBot->edict() ), (*g_engfuncs.pfnGetInfoKeyBuffer)( pEdict ), "model", szModel );
	(*g_engfuncs.pfnSetClientKeyValue)( ENTINDEX( pBot->edict() ), (*g_engfuncs.pfnGetInfoKeyBuffer)( pEdict ), "topcolor", pszTopColor );
	(*g_engfuncs.pfnSetClientKeyValue)( ENTINDEX( pBot->edict() ), (*g_engfuncs.pfnGetInfoKeyBuffer)( pEdict ), "bottomcolor", pszBottomColor );
}

//=========================================================
// BotCreate - connect a fake client named after a free profile
// or a free table name; pCaller (NULL from the server) is
// told when the server is full
//=========================================================
BOOL BotCreate( edict_t *pCaller )
{
	botname_t *pName = NULL;
	char *pszName = "123456789012345678901234567890123456789012345678901234567890";	// a buffer
	char szRejectReason[128];

	if ( BotFindProfile() )
		strcpy( pszName, g_BotManager.m_szLastProfile );
	else
	{
		pName = BotPickName();
		strcpy( pszName, pName->pszName );
	}

	edict_t *pEdict = (*g_engfuncs.pfnCreateFakeClient)( pszName );
	if ( !pEdict )
	{
		if ( pCaller )
			ClientPrint( &pCaller->v, HUD_PRINTNOTIFY, "There are no free client slots. Set maxplayers to a higher value.\n" );
		else
			ALERT( at_console, "There are no free client slots. Set maxplayers to a higher value\n" );
		return FALSE;
	}

	pEdict->v.netname = ALLOC_STRING( pszName );
	memset( szRejectReason, 0, sizeof( szRejectReason ) );
	g_pGameRules->ClientConnected( pEdict, STRING( pEdict->v.netname ), "127.0.0.1", szRejectReason );
	BotInit( pEdict, pName );
	return TRUE;
}

//=========================================================
// BotRemove - kick the bot in client slot iIndex
//=========================================================
void BotRemove( int iIndex )
{
	if ( iIndex <= 0 )
		return;

	CBaseEntity *pPlayer = UTIL_PlayerByIndex( iIndex );
	if ( pPlayer && FBitSet( pPlayer->pev->flags, FL_FAKECLIENT ) )
	{
		SERVER_COMMAND( UTIL_VarArgs( "kick \"%s\"\n", STRING( pPlayer->pev->netname ) ) );
		SERVER_EXECUTE();
	}
}

//=========================================================
// AimJitter - normalise the look direction; then, when the
// view punch is small, a skill roll may kick the view by up
// to 30 degrees times the aim error
//=========================================================
void CBot::AimJitter( void )
{
	Vector vecDir;
	vecDir = m_vecLookDir;
	m_vecLookDir = vecDir.Normalize();

	float flError = 1 - m_Profile.AimSkill() / 100;
	float flRange = ( m_Profile.AimSkill() < RANDOM_FLOAT( 0, 100 ) ) * flError * 30;

	if ( pev->punchangle.Length() < flError )
		pev->punchangle = Vector( RANDOM_FLOAT( -flRange, flRange ), RANDOM_FLOAT( -flRange, flRange ), RANDOM_FLOAT( -flRange, flRange ) );
}

//=========================================================
// AimAtEnemy - look at the enemy, the way the weapon wants:
// mode 1 at its origin, mode 0 at its eyes, mode 2 where it
// will be in a second, at the height of the bot's feet
//=========================================================
void CBot::AimAtEnemy( void )
{
	if ( m_fLookDirSet == TRUE )
		return;

	m_fLookDirSet = TRUE;

	if ( m_Fire.m_iAimMode == 1 )
	{
		CBaseEntity *pEnemy = GetEnemy();
		m_vecLookDir = pEnemy->pev->origin - GetGunPosition();
	}
	else if ( m_Fire.m_iAimMode == 0 )
	{
		CBaseEntity *pEnemy = GetEnemy();
		m_vecLookDir = pEnemy->EyePosition() - GetGunPosition();
	}
	else if ( m_Fire.m_iAimMode == 2 )
	{
		CBaseEntity *pEnemy = GetEnemy();	// sic: unused
		Vector vecTarget = GetEnemy()->pev->origin + GetEnemy()->pev->velocity;
		Vector vecAim = Vector( vecTarget.x, vecTarget.y, vecTarget.z + pev->mins.z );
		m_vecLookDir = vecAim - GetGunPosition();
	}

	AimJitter();
}

//=========================================================
// AimAt - look at pEntity's origin, and fire
//=========================================================
void CBot::AimAt( CBaseEntity *pEntity )
{
	if ( m_fLookDirSet )
		return;

	m_fLookDirSet = TRUE;
	m_vecLookDir = pEntity->pev->origin - GetGunPosition();
	AimJitter();
	PressFire();
}

//=========================================================
// SetLookDir - turn the wanted velocity into the direction
// MoveThink walks in: flat, or along a ladder (climbing up
// or down at 0.6 when m_iLadderDir is set, facing the
// ladder's centre)
//=========================================================
void CBot::SetLookDir( void )
{
	m_fLookDirSet = TRUE;

	Vector vecVel = m_vecMoveVelocity;
	Vector vecDir = vecVel;
	if ( vecVel == g_vecZero )
		UTIL_MakeVectorsPrivate( pev->v_angle, vecDir, NULL, NULL );
	vecDir.z = 0;

	if ( m_iLadderDir )
	{
		if ( IsOnLadder() )
		{
			CBaseEntity *pLadder = NULL;
			while ( 1 )
			{
				pLadder = UTIL_FindEntityByClassname( pLadder, "func_ladder" );
				if ( !pLadder )
					break;
				if ( Intersects( pLadder ) )
				{
					vecDir = ( pLadder->Center() - pev->origin ).Normalize();
					vecDir.z = 0;
					pev->button |= IN_FORWARD;
				}
			}
		}
		vecDir = vecDir.Normalize();
		vecDir.z = m_iLadderDir * 0.6;
	}

	m_vecLookDir = vecDir;
}

// steering constants (names ours): retail reads them from botmove.obj's own
// .rdata (20.0, 0.5, 128.0 in this order at 0x100bb844), not from the shared
// float constants, and compares the int abs() with a float 20 (fild; fcomp)
const float BOT_MAX_TURN = 20;			// AvoidWalls: degrees off the current yaw
const float BOT_TURN_JITTER = 0.5;		// AvoidWalls: the turn is scaled by 1 +- this
const float BOT_SIDE_DIST = 128;		// CheckSides: trace length

//=========================================================
// CheckAhead - is something in the way? Traces 288 units
// (172.8 in water) along vecDir from the origin, shifted
// sideways by 16 one time in two, and then from the eyes
//=========================================================
BOOL CBot::CheckAhead( Vector &vecDir, TraceResult *ptr )
{
	float flDist = 288;
	if ( pev->waterlevel > 1 )
		flDist = 172.8;

	Vector vecSrc = pev->origin;
	Vector vecEnd;
	Vector vecSide;
	Vector vecTemp;
	vecEnd = vecSrc + vecDir.Normalize() * flDist;

	vecSide = vecDir.Normalize();
	vecTemp = vecDir.Normalize();
	vecSide.x = vecSide.x - 1;
	vecTemp.y = vecTemp.y - 1;
	if ( vecTemp.y < -1 )
		vecTemp.y += 2;
	if ( vecSide.x < -1 )
		vecSide.x += 2;
	vecSide = Vector( vecSide.x, vecTemp.y, 0 ) * 16;
	vecSide = vecSide * ( RANDOM_LONG( 0, 1 ) != 0 );
	vecSrc = vecSrc + vecSide;

	gpGlobals->trace_flags = FTRACE_SIMPLEBOX;
	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), ptr );
	if ( ptr->flFraction != 1.0 )
		return TRUE;

	vecSrc = EyePosition();
	vecEnd = vecSrc + vecDir.Normalize() * flDist;
	gpGlobals->trace_flags = FTRACE_SIMPLEBOX;
	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), ptr );
	if ( ptr->flFraction != 1.0 )
		return TRUE;

	return FALSE;
}

//=========================================================
// CheckSides - traces 128 units to the left and right.
// vecAngles are angles, but they are normalised like a
// direction (sic), and its yaw is left 45 degrees off.
//=========================================================
BOOL CBot::CheckSides( Vector &vecAngles, TraceResult *ptrLeft, TraceResult *ptrRight )
{
	Vector vecSrc = pev->origin;
	Vector vecEnd;

	vecAngles.y -= 45;
	vecEnd = vecSrc + vecAngles.Normalize() * BOT_SIDE_DIST;	// sic
	UTIL_TraceLine( vecSrc, vecEnd, ignore_monsters, ENT( pev ), ptrLeft );

	vecAngles.y += 90;
	vecEnd = vecSrc + vecAngles.Normalize() * BOT_SIDE_DIST;	// sic
	UTIL_TraceLine( vecSrc, vecEnd, ignore_monsters, ENT( pev ), ptrRight );

	if ( ptrLeft->flFraction == 1.0 && ptrRight->flFraction == 1.0 )
		return FALSE;
	return TRUE;
}

//=========================================================
// MoveThink - face the move direction (smoothed over the
// last four frames unless the goal position is within 128
// units), steer around walls, and set the move speeds
// that BotThink hands to RunPlayerMove
//=========================================================
void CBot::MoveThink( void )
{
	BOOL fSmooth = TRUE;
	Vector vecTemp;
	Vector vecMove;

	vecTemp = m_vecLookDir;
	vecMove = vecTemp;

	if ( m_fHasGoalPos && !HasGoalItem() )
	{
		Vector vecGoal = m_vecGoalPos;
		vecTemp = Vector( vecGoal.x - pev->origin.x, vecGoal.y - pev->origin.y, vecGoal.z - pev->origin.z );
		if ( vecTemp.Length2D() < 128 )
			fSmooth = FALSE;
	}

	if ( m_fDuck )
		pev->button |= IN_DUCK;

	if ( m_fJump && ( pev->flags & FL_ONGROUND ) )
	{
		pev->velocity.z = sqrt( 2 * 800 * 56.0 );
		pev->button |= IN_JUMP;
		m_fJump = FALSE;
	}

	if ( fSmooth )
		vecMove = vecMove + m_Memory.m_vecLookDir + m_Memory.m_vecLookDir1 + m_Memory.m_vecLookDir2 + m_Memory.m_vecLookDir3;

	Vector vecAngles = UTIL_VecToAngles( vecMove );
	if ( vecAngles != g_vecZero )
		pev->v_angle = vecAngles;

	if ( pev->v_angle.x > 180 )
		pev->v_angle.x -= 360;
	pev->v_angle.x = pev->v_angle.x * -1;
	pev->angles.x = pev->v_angle.x * -0.5;
	pev->angles.y = pev->v_angle.y;

	float flSpeed = CVAR_GET_FLOAT( "sv_maxspeed" );
	if ( gpGlobals->time < m_flPauseTime )
		flSpeed = 0;

	Vector vecVelocity = m_vecMoveVelocity;
	MAKE_VECTORS( pev->v_angle );
	m_vecMoveAngles = AvoidWalls( vecVelocity );

	float flMoveYaw = m_vecMoveAngles.y;
	float flYaw = pev->v_angle.y;
	while ( flMoveYaw < 0 )
		flMoveYaw += 360;
	while ( flYaw < 0 )
		flYaw += 360;
	float flDelta = flYaw - flMoveYaw;
	while ( flDelta < 0 )
		flDelta += 360;

	float flAngle = flDelta;
	if ( flAngle > 180 )
		flAngle -= 180;
	if ( flAngle > 90 )
		flAngle = flAngle - ( ( flAngle - 90 ) + ( flAngle - 90 ) );

	float flForward = ( flAngle * -1 + 90 ) / 90 * flSpeed;
	m_flForwardMove = flForward;
	if ( 90 < flDelta && flDelta < 270 )
		m_flForwardMove = flForward * -1;

	flSpeed = flSpeed - m_flForwardMove;
	m_flSideMove = flSpeed;
	if ( flDelta > 180 )
		m_flSideMove = flSpeed * -1;

	if ( pev->waterlevel == 3 )
		m_flUpMove = 2;
}

//=========================================================
// AvoidWalls - the angles to move at: vecDir's, turned away
// from a wall ahead (when the goal is in sight) by up to
// the angle to the wall's side, scaled by how near it is,
// at most 20 degrees off the current yaw
//=========================================================
Vector CBot::AvoidWalls( Vector &vecDir )
{
	TraceResult tr;
	TraceResult trSides;

	Vector vecAngles = UTIL_VecToAngles( vecDir );

	if ( !CheckAhead( vecDir, &tr ) || !m_fGoalVisible )
	{
		m_iDodgeDir = 0;
	}
	else
	{
		float flWallYaw = UTIL_VecToAngles( tr.vecPlaneNormal ).y;

		float flYaw = vecAngles.y;
		while ( flYaw < 0 )
			flYaw += 360;
		while ( flYaw > 360 )
			flYaw -= 360;

		float flWall = flWallYaw - 180;
		while ( flWall < 0 )
			flWall += 360;
		while ( flWall > 360 )
			flWall -= 360;

		float flLeft = flWall - 90;
		if ( flLeft < 0 )
			flLeft += 360;
		float flRight = flWall + 90;
		if ( flRight > 360 )
			flRight -= 360;

		float flDiffLeft = flYaw - flLeft;
		if ( flDiffLeft < -180 )
			flDiffLeft += 360;
		else if ( flDiffLeft > 180 )
			flDiffLeft -= 360;

		float flDiffRight = flYaw - flRight;
		if ( flDiffRight < -180 )
			flDiffRight += 360;
		else if ( flDiffRight > 180 )
			flDiffRight -= 360;

		float flScale = ( 288 - tr.flFraction * 288 ) / 256;

		if ( !m_iDodgeDir )
		{
			m_iDodgeDir = ( abs( (int)flDiffLeft ) <= abs( (int)flDiffRight ) ) + 1;
			if ( !RANDOM_LONG( 0, 3 ) )
				m_iDodgeDir = ( m_iDodgeDir == 1 ) ? 2 : 1;
		}

		float flTurn;
		if ( m_iDodgeDir == 1 )
		{
			flTurn = RANDOM_FLOAT( 1 - BOT_TURN_JITTER, BOT_TURN_JITTER + 1 ) * flScale * flDiffLeft;
		}
		else
		{
			m_iDodgeDir = 2;
			flTurn = RANDOM_FLOAT( 1 - BOT_TURN_JITTER, BOT_TURN_JITTER + 1 ) * flScale * flDiffRight;
		}

		vecAngles.y = vecAngles.y - flTurn;
		float flOff = flYaw - vecAngles.y;
		if ( abs( (int)flOff ) > BOT_MAX_TURN )
		{
			if ( flOff < 0 )
				vecAngles.y = flYaw - BOT_MAX_TURN;
			else
				vecAngles.y = BOT_MAX_TURN + flYaw;
		}

		while ( vecAngles.y > 180 )
			vecAngles.y -= 360;
		while ( vecAngles.y < -180 )
			vecAngles.y += 360;
	}

	Vector vecCheck = vecAngles;
	CheckSides( vecCheck, &tr, &trSides );

	return vecAngles;
}

//=========================================================
// Unstick - jump (and swim up) at an enemy; otherwise walk
// forward, or backward once it stops moving, and so on
//=========================================================
void CBot::Unstick( void )
{
	if ( HasEnemy() )
	{
		pev->button |= IN_JUMP;
		float flSpeed = CVAR_GET_FLOAT( "sv_maxspeed" );
		m_flUpMove = flSpeed;
	}
	else
	{
		if ( m_flMoved <= 1 )
			m_fDuckToggle = !m_fDuckToggle;

		if ( m_fDuckToggle )
			pev->button |= IN_FORWARD;
		else
			pev->button |= IN_BACK;
	}
}

// The move goals below set m_vecMoveVelocity (at sv_maxspeed
// unless said otherwise). A goal of higher priority than the
// current one replaces it; goals of the same priority are
// averaged. All are names ours; most are never called.

//=========================================================
// FlockAlign - move with the velocity of the living players
// in sight within 128 units
//=========================================================
void CBot::FlockAlign( int iPriority )
{
	if ( iPriority > GetMovePriority() )
	{
		AlignVelocity();
		SetMovePriority( iPriority );
		SetMoveGoals( 1 );
	}
	else if ( iPriority == GetMovePriority() )
	{
		Vector vecOld = Vector( m_vecMoveVelocity ) * GetMoveGoals();
		AlignVelocity();
		Vector vecTotal = GetMoveVelocity() + vecOld;
		SetMoveGoals( GetMoveGoals() + 1 );
		SetMoveVelocity( vecTotal / GetMoveGoals() );
	}
}

//=========================================================
// FlockCohere - move to the centre of the living players in
// sight within 128 units
//=========================================================
void CBot::FlockCohere( int iPriority )
{
	if ( iPriority > GetMovePriority() )
	{
		CohereVelocity();
		SetMovePriority( iPriority );
		SetMoveGoals( 1 );
	}
	else if ( iPriority == GetMovePriority() )
	{
		Vector vecOld = Vector( m_vecMoveVelocity ) * GetMoveGoals();
		CohereVelocity();
		Vector vecTotal = GetMoveVelocity() + vecOld;
		SetMoveGoals( GetMoveGoals() + 1 );
		SetMoveVelocity( vecTotal / GetMoveGoals() );
	}
}

//=========================================================
// MoveTowards - head for vecDest
//=========================================================
void CBot::MoveTowards( const Vector &vecDest )
{
	Vector vecDir = ( vecDest - pev->origin ).Normalize();
	vecDir = vecDir * CVAR_GET_FLOAT( "sv_maxspeed" );
	m_vecMoveVelocity = vecDir;
}

//=========================================================
// ApproachEntity - head for the spot 80 units short of
// where pEntity will be (see MoveToLead)
//=========================================================
void CBot::ApproachEntity( CBaseEntity *pEntity, int iPriority )
{
	if ( iPriority > GetMovePriority() )
	{
		FollowLead( pEntity );
		SetMoveGoals( 1 );
		SetMovePriority( iPriority );
	}
	else if ( iPriority == GetMovePriority() )
	{
		Vector vecOld = Vector( m_vecMoveVelocity ) * GetMoveGoals();
		FollowLead( pEntity );
		Vector vecTotal = GetMoveVelocity() + vecOld;
		SetMoveGoals( GetMoveGoals() + 1 );
		SetMoveVelocity( vecTotal / GetMoveGoals() );
	}
}

//=========================================================
// MoveToLead - head for where pEntity will be when the bot
// gets there (its velocity times the distance), plus
// vecOffset
//=========================================================
void CBot::MoveToLead( CBaseEntity *pEntity, Vector vecOffset )
{
	float flDist = ( pEntity->pev->origin - pev->origin ).Length();
	HeadFor( pEntity->pev->origin + pEntity->pev->velocity * flDist + vecOffset );
}

//=========================================================
// Flock - align, cohere and separate (each at priority 2)
//=========================================================
void CBot::Flock( int iPriority )
{
	if ( iPriority > GetMovePriority() )
	{
		FlockAlign( 2 );
		FlockCohere( 2 );
		FlockSeparate( 2 );
		SetMovePriority( iPriority );
		SetMoveGoals( 1 );
	}
	else if ( iPriority == GetMovePriority() )
	{
		Vector vecOld = Vector( m_vecMoveVelocity ) * GetMoveGoals();
		FlockAlign( 2 );
		FlockCohere( 2 );
		FlockSeparate( 2 );
		Vector vecTotal = GetMoveVelocity() + vecOld;
		SetMoveGoals( GetMoveGoals() + 1 );
		SetMoveVelocity( vecTotal / GetMoveGoals() );
	}
}

//=========================================================
// FollowEntity - the same as ApproachEntity
//=========================================================
void CBot::FollowEntity( CBaseEntity *pEntity, int iPriority )
{
	if ( iPriority > GetMovePriority() )
	{
		FollowLead( pEntity );
		SetMoveGoals( 1 );
		SetMovePriority( iPriority );
	}
	else if ( iPriority == GetMovePriority() )
	{
		Vector vecOld = Vector( m_vecMoveVelocity ) * GetMoveGoals();
		FollowLead( pEntity );
		Vector vecTotal = GetMoveVelocity() + vecOld;
		SetMoveGoals( GetMoveGoals() + 1 );
		SetMoveVelocity( vecTotal / GetMoveGoals() );
	}
}

//=========================================================
// FlockSeparate - keep away from the living players in
// sight within 128 units, the nearer the more. The sum of
// directions is passed to MoveTowards as a spot (sic).
//=========================================================
void CBot::FlockSeparate( int iPriority )
{
	if ( iPriority > GetMovePriority() )
	{
		SeparateVelocity();
		SetMoveGoals( 1 );
		SetMovePriority( iPriority );
	}
	else if ( iPriority == GetMovePriority() )
	{
		Vector vecOld = Vector( m_vecMoveVelocity ) * GetMoveGoals();
		SeparateVelocity();
		Vector vecTotal = GetMoveVelocity() + vecOld;
		SetMoveGoals( GetMoveGoals() + 1 );
		SetMoveVelocity( vecTotal / GetMoveGoals() );
	}
}

//=========================================================
// MoveToEntity - run from where pEntity is heading (its
// origin plus its velocity times the distance): used for
// grenades, bear traps, scorpions and cannon balls
//=========================================================
void CBot::MoveToEntity( CBaseEntity *pEntity, int iPriority )
{
	if ( iPriority > GetMovePriority() )
	{
		FleeLead( pEntity );
		SetMoveGoals( 1 );
		SetMovePriority( iPriority );
	}
	else if ( iPriority == GetMovePriority() )
	{
		Vector vecOld = Vector( m_vecMoveVelocity ) * GetMoveGoals();
		FleeLead( pEntity );
		Vector vecTotal = GetMoveVelocity() + vecOld;
		SetMoveGoals( GetMoveGoals() + 1 );
		SetMoveVelocity( vecTotal / GetMoveGoals() );
	}
}

//=========================================================
// MoveAwayFrom - head straight away from vecSpot
//=========================================================
void CBot::MoveAwayFrom( const Vector &vecSpot )
{
	Vector vecDir = ( vecSpot - pev->origin ).Normalize();
	vecDir = vecDir * -1;
	m_vecMoveVelocity = vecDir * CVAR_GET_FLOAT( "sv_maxspeed" );
}

//=========================================================
// FleeEntity - run from pEntity
//=========================================================
void CBot::FleeEntity( CBaseEntity *pEntity, int iPriority )
{
	if ( iPriority > GetMovePriority() )
	{
		MoveAwayFrom( pEntity->pev->origin );
		SetMoveGoals( 1 );
		SetMovePriority( iPriority );
	}
	else if ( iPriority == GetMovePriority() )
	{
		Vector vecOld = Vector( m_vecMoveVelocity ) * GetMoveGoals();
		FleeFromEntity( pEntity );
		Vector vecTotal = GetMoveVelocity() + vecOld;
		SetMoveGoals( GetMoveGoals() + 1 );
		SetMoveVelocity( vecTotal / GetMoveGoals() );
	}
}

//=========================================================
// FleeSpot - run from vecSpot
//=========================================================
void CBot::FleeSpot( const Vector &vecSpot, int iPriority )
{
	if ( iPriority > GetMovePriority() )
	{
		RunFrom( vecSpot );
		SetMoveGoals( 1 );
		SetMovePriority( iPriority );
	}
	else if ( iPriority == GetMovePriority() )
	{
		Vector vecOld = Vector( m_vecMoveVelocity ) * GetMoveGoals();
		RunFrom( vecSpot );
		Vector vecTotal = GetMoveVelocity() + vecOld;
		SetMoveGoals( GetMoveGoals() + 1 );
		SetMoveVelocity( vecTotal / GetMoveGoals() );
	}
}

//=========================================================
// UnstickGoal - Unstick as a move goal
//=========================================================
void CBot::UnstickGoal( int iPriority )
{
	if ( iPriority > GetMovePriority() )
	{
		Unstick();
		SetMovePriority( iPriority );
		SetMoveGoals( 1 );
	}
	else if ( iPriority == GetMovePriority() )
	{
		Vector vecOld = Vector( m_vecMoveVelocity ) * GetMoveGoals();
		Unstick();
		Vector vecTotal = GetMoveVelocity() + vecOld;
		SetMoveGoals( GetMoveGoals() + 1 );
		SetMoveVelocity( vecTotal / GetMoveGoals() );
	}
}

//=========================================================
// MoveToEntityOffset - MoveToLead as a move goal
//=========================================================
void CBot::MoveToEntityOffset( CBaseEntity *pEntity, int iPriority, Vector vecOffset )
{
	if ( iPriority > GetMovePriority() )
	{
		MoveToLead( pEntity, vecOffset );
		SetMoveGoals( 1 );
		SetMovePriority( iPriority );
	}
	else if ( iPriority == GetMovePriority() )
	{
		Vector vecOld = Vector( m_vecMoveVelocity ) * GetMoveGoals();
		MoveToLead( pEntity, vecOffset );
		Vector vecTotal = GetMoveVelocity() + vecOld;
		SetMoveGoals( GetMoveGoals() + 1 );
		SetMoveVelocity( vecTotal / GetMoveGoals() );
	}
}

//=========================================================
// MoveToItem - head for pItem's centre
//=========================================================
void CBot::MoveToItem( CBaseEntity *pItem, int iPriority )
{
	if ( iPriority > GetMovePriority() )
	{
		MoveTowards( pItem->Center() );
		SetMoveGoals( 1 );
		SetMovePriority( iPriority );
	}
	else if ( iPriority == GetMovePriority() )
	{
		Vector vecOld = Vector( m_vecMoveVelocity ) * GetMoveGoals();
		HeadForItem( pItem );
		Vector vecTotal = GetMoveVelocity() + vecOld;
		SetMoveGoals( GetMoveGoals() + 1 );
		SetMoveVelocity( vecTotal / GetMoveGoals() );
	}
}

//=========================================================
// MoveToSpot - head for vecSpot
//=========================================================
void CBot::MoveToSpot( const Vector &vecSpot, int iPriority )
{
	if ( iPriority > GetMovePriority() )
	{
		HeadFor( vecSpot );
		SetMovePriority( iPriority );
		SetMoveGoals( 1 );
	}
	else if ( iPriority == GetMovePriority() )
	{
		Vector vecOld = Vector( m_vecMoveVelocity ) * GetMoveGoals();
		HeadFor( vecSpot );
		Vector vecTotal = GetMoveVelocity() + vecOld;
		SetMoveGoals( GetMoveGoals() + 1 );
		SetMoveVelocity( vecTotal / GetMoveGoals() );
	}
}

//=========================================================
// Wobble - add a random push of up to flPercent per cent of
// the move direction, in each axis
//=========================================================
void CBot::Wobble( float flPercent, int iPriority )
{
	if ( iPriority > GetMovePriority() )
	{
		WobbleVelocity( flPercent );
		SetMoveGoals( 1 );
		SetMovePriority( iPriority );
	}
	else if ( iPriority == GetMovePriority() )
	{
		Vector vecOld = Vector( m_vecMoveVelocity ) * GetMoveGoals();
		WobbleVelocity( flPercent );
		Vector vecTotal = GetMoveVelocity() + vecOld;
		SetMoveGoals( GetMoveGoals() + 1 );
		SetMoveVelocity( vecTotal / GetMoveGoals() );
	}
}

//=========================================================
// SeekGoal - Roam's step (emitted after it): get a new goal
// position from RouteGoal when there is none (mode 1), it is
// within 32 units (mode 2) or out of sight (mode 3), then
// head for it; without one walk the way the bot faces, unless
// the route is done or the bot is in state 3 (pause 0.1 s).
// Roam's first branch expands it with too little budget left
// for anything above 40 nodes: its calls, and the unused
// TraceResult's constructor, are out of line there.
//=========================================================
inline void CBot::SetGoalVisible( BOOL fVisible ) { m_fGoalVisible = fVisible; }
inline int CBot::GetState( void ) { return m_iState; }
inline void CBot::SetPauseTime( float flTime ) { m_flPauseTime = flTime; }

inline void CBot::SeekGoal( void )
{
	TraceResult tr;	// sic: unused; retail constructs it (Roam's out-of-line TraceResult() at 0x10014a70)
	Vector vecForward;
	float flDist = ( GetGoalPos() - pev->origin ).Length2D();
	int iMode = 0;

	if ( !m_fHasGoalPos )
		iMode = 1;
	else if ( flDist < 32 )
		iMode = 2;
	else if ( !FVisible( GetGoalPos() ) )
		iMode = 3;

	if ( iMode )
	{
		m_fHasGoalPos = FALSE;
		SetMoveVelocity( g_vecZero );
		m_vecGoalPos = RouteGoal( iMode );
		if ( m_fHasGoalPos )
			m_flGoalTime = gpGlobals->time;
	}

	if ( m_fHasGoalPos )
	{
		if ( m_fGoalVisible && m_pRoute )
		{
			if ( CanSeeSpot( GetGoalPos() ) )
				SetGoalVisible( FALSE );
		}
		SetMoveVelocity( ( GetGoalPos() - pev->origin ).Normalize() * CVAR_GET_FLOAT( "sv_maxspeed" ) );
	}
	else if ( GetState() )
	{
		if ( GetState() == 3 )
			SetPauseTime( gpGlobals->time + 0.1 );
	}
	else
	{
		if ( m_fRouteDone )
			SetPauseTime( gpGlobals->time + 0.1 );
		else
		{
			UTIL_MakeVectorsPrivate( m_vecMoveAngles, vecForward, NULL, NULL );
			SetMoveVelocity( vecForward * CVAR_GET_FLOAT( "sv_maxspeed" ) );
		}
	}
}

//=========================================================
// Roam - head for the goal position. A new one is asked of
// RouteGoal when there is none (mode 1), when it is within
// 32 units (mode 2) or when it is out of sight (mode 3);
// without one the bot walks the way it faces, unless its
// route is done or it is in state 3, when it pauses 0.1 s.
//=========================================================
void CBot::Roam( int iPriority )
{
	if ( iPriority > GetMovePriority() )
	{
		SeekGoal();
		SetMoveGoals( 1 );
		SetMovePriority( iPriority );
	}
	else if ( iPriority == GetMovePriority() )
	{
		Vector vecVel = GetMoveVelocity();
		Vector vecOld = vecVel * GetMoveGoals();
		SeekGoal();
		Vector vecTotal = GetMoveVelocity() + vecOld;
		SetMoveGoals( GetMoveGoals() + 1 );
		SetMoveVelocity( vecTotal / GetMoveGoals() );
	}
}
