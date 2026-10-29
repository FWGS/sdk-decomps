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

===== botnav.cpp ========================================================

  The Cow-Bots, part 4: navigation. info_botnode (keyvalues group,
  grouptrans, randomlinks, action) and info_botlandmark (link0-2) build
  a node graph (g_BotNavigator) that bots search for routes; the visit grid is
  kept in wantedhl/maps/<map>.nav. Also the bot profiles, read from
  wantedhl/bots/<name>.bot (Name:, Model:, Skill:, Topcolor:,
  Bottomcolor:).

  The file name is ours, and so are all names but the entity classnames
  and keyvalues.

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

#include <fstream.h>

extern cvar_t sv_botskill;

// the CTC rules' per-team chicken state: nonzero while the team's chicken
// is away from its hen-house (ctc_chicken's file; name ours)
extern float g_flChickenAway[];

// retail: g_flBotQueueTime 0x100f38c4, g_flNavSaveTime 0x100f38c8, 4
// unused bytes 0x100f38cc, g_cFindPaths 0x100f38d0, g_BotNavigator
// 0x100f38d8. VC6 lays out uninitialised globals by a hash of their
// names, not in declaration order; these names (all ours) give retail's
// layout.
CBotNav		g_BotNavigator;
int			g_cFindPaths;	// FindPath calls for the current route (name ours)
float		g_flNavSaveTime;	// next time the .nav file is written
int			g_iNavUnused;	// sic: retail's .bss has an int here that no code reads or writes (name ours)


//=========================================================
// info_botnode
//=========================================================
LINK_ENTITY_TO_CLASS( info_botnode, CInfoBotnode );

void CInfoBotnode :: Spawn( void )
{
	g_BotNavigator.AddNode( this, 0 );
	REMOVE_ENTITY( ENT( pev ) );
}

void CInfoBotnode :: KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "group" ) )
	{
		m_iGroup = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "grouptrans" ) )
	{
		m_iGroupTrans = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "randomlinks" ) )
	{
		m_fRandomLinks = atoi( pkvd->szValue );
		if ( m_fRandomLinks < 0 || m_fRandomLinks > 1 )
			m_fRandomLinks = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "action" ) )
	{
		m_iAction = atoi( pkvd->szValue );
		if ( m_iAction < 0 || m_iAction >= 4 )
			m_iAction = 0;
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}


//=========================================================
// the bot's route
//=========================================================
void CBot :: ClearRoute( void )
{
	if ( m_pRouteStart )
		delete m_pRouteStart;

	m_pRoute = NULL;
	m_pRouteStart = NULL;
	m_cRouteSteps = 0;
	m_cRouteLeft = 0;
	m_fRouteDone = FALSE;
	m_fJump = FALSE;
	m_fDuck = FALSE;
	m_iLadderDir = 0;
}

void CBot :: NextRouteNode( void )
{
	m_pRoute++;
	if ( --m_cRouteLeft == 0 )
	{
		ClearRoute();
		m_fRouteDone = TRUE;
	}
	else
		m_fRouteDone = FALSE;
}

//=========================================================
// RouteGoal's accessors (names ours)
//=========================================================
inline BOOL CBotMemory :: Seen( void ) { return m_fSeen; }
inline BOOL CBotMemory :: ChickenCarrier( void ) { return m_fChickenCarrier; }
inline int CBot :: GetState( void ) { return m_iState; }
inline BOOL CBot :: RouteDone( void ) { return m_fRouteDone; }

//=========================================================
// RouteGoal - where to head for: the next step of the
// route, or (in Capture the Chicken) a new route to the
// chicken, the capture point or the enemy's last position;
// otherwise wander.
//=========================================================
// Retail's inline decisions: the step distance's operator- return
// copies are called out of line (their share 101-102 against the 104
// a second copy needs), the found route's are not; the free accessor
// sites after them thin the shares. The chase and the chicken away
// from home share one if ( pGoal ) test (retail jumps into it), and
// the goal search checks for no candidate before its loop as well.
Vector CBot :: RouteGoal( int iMode )
{
	char		*pszGoal;
	CBaseEntity	*pGoal = NULL;
	int			iTeam;
	float		flDist;

	if ( !g_BotNavigator.m_pHead )
		return Wander();

	if ( HasEnemy() && m_Memory.Seen() && GetState() != 3 )
		return Wander();

	CBaseEntity	tmp;		// the enemy's last position, as an entity to route to
	entvars_t	vars;

	if ( g_pGameRules->GetGameType() == 2 )
	{
		if ( m_pRoute )
		{
			if ( m_pRoute->m_iAction == 3 )
				flDist = ( m_pRoute->m_vecOrigin - pev->origin ).Length();
			else
				flDist = ( m_pRoute->m_vecOrigin - pev->origin ).Length2D();

			if ( flDist < m_pRoute->m_flRadius )
			{
				// reached this step
				if ( m_pRoute->m_iAction == 1 )
					m_fJump = TRUE;
				if ( m_pRoute->m_iAction == 2 )
					m_fDuck = FALSE;
				if ( m_pRoute->m_iAction == 3 )
					m_flNextStuckCheck = gpGlobals->time + 1.5;

				NextRouteNode();

				if ( !RouteDone() )
				{
					m_fGoalVisible = FALSE;
					m_fHasGoalPos = TRUE;

					if ( m_pRoute->m_iAction == 2 )
						m_fDuck = TRUE;

					if ( m_pRoute->m_iAction == 3 )
					{
						if ( m_pRoute->m_vecOrigin.z - pev->origin.z > 0 )
							m_iLadderDir = 1;
						else
							m_iLadderDir = -1;
					}
					else
						m_iLadderDir = 0;

					return m_pRoute->m_vecOrigin;
				}
			}
			else if ( !CanSeeSpot( m_pRoute->m_vecOrigin ) )
			{
				if ( !FVisible( m_pRoute->m_vecOrigin ) )
					ClearRoute();
				else
				{
					m_fGoalVisible = TRUE;
					m_fHasGoalPos = TRUE;
					return m_pRoute->m_vecOrigin;
				}
			}
			else if ( iMode == 2 && flDist >= m_pRoute->m_flRadius )
			{
				m_fHasGoalPos = TRUE;
				return m_pRoute->m_vecOrigin;
			}
		}

		pszGoal = "ctc_chicken";
		iTeam = 0;
		g_BotNavigator.m_fRandomLinks = TRUE;
		g_BotNavigator.m_fUnused = TRUE;

		if ( HasNamedPlayerItem( "ctc_chicken" ) )
		{
			// carrying the chicken: take it home
			pszGoal = "ctc_capturepoint";
			iTeam = 1;
		}
		else if ( HasEnemy() && GetState() == 3 )
		{
			if ( iMode == 1 && ( m_Memory.ChickenCarrier() || m_Profile.Aggression() > RANDOM_LONG( 0, 100 ) ) )
			{
				// chase the enemy to where it was last seen
				memset( &vars, 0, sizeof( vars ) );
				tmp.pev = &vars;
				Vector vecLast = m_Memory.m_vecEnemyOrigin;
				vars.origin = vars.absmin = vars.absmax = vecLast;
				vars.view_ofs = g_vecZero;
				vars.flags = 0;
				vars.waterlevel = 0;
				vars.classname = ALLOC_STRING( "LastEnemyPos" );

				g_BotNavigator.m_fUnused = FALSE;
				g_BotNavigator.m_fRandomLinks = FALSE;
				pGoal = &tmp;
			}
			else if ( !m_Memory.Seen() )
			{
				m_Memory.Forget();
				return pev->origin;
			}
		}
		else
		{
			if ( !( pev->flags & FL_ONGROUND ) && pev->waterlevel <= 1 )
				return Wander();

			CBaseEntity *pChicken = UTIL_FindEntityByClassname( NULL, "ctc_chicken" );
			while ( pChicken && !pChicken->IsTriggered( this ) )
				pChicken = UTIL_FindEntityByClassname( pChicken, "ctc_chicken" );

			if ( g_flChickenAway[g_pGameRules->GetTeamIndex( TeamID() )] != 0 )
			{
				iTeam = 1;
				pGoal = pChicken;
			}
		}

		if ( pGoal )
			goto found;

		pGoal = UTIL_FindEntityByClassname( NULL, pszGoal );
		if ( !pGoal )
			return Wander();

		while ( pGoal )
		{
			if ( pGoal->IsTriggered( this ) == iTeam )
				break;
			pGoal = UTIL_FindEntityByClassname( pGoal, pszGoal );
		}

		if ( !pGoal )
			return Wander();

found:
		ClearRoute();
		m_pRoute = m_pRouteStart = g_BotNavigator.BuildRoute( this, pGoal );

		if ( m_pRoute )
		{
			if ( g_BotNavigator.m_iMaxDepth == 1 && ( m_pRoute->m_vecOrigin - pev->origin ).Length2D() < 32 )
			{
				ClearRoute();
				m_fRouteDone = TRUE;
				return pev->origin;
			}

			m_fGoalVisible = FALSE;
			m_fHasGoalPos = TRUE;
			m_cRouteLeft = g_BotNavigator.m_iMaxDepth;
			m_cRouteSteps = g_BotNavigator.m_iMaxDepth;
			return m_pRoute->m_vecOrigin;
		}
	}

	return Wander();
}

//=========================================================
// Wander - with no route: jump out of water, else head for
// the least visited cell of the visit grid ahead.
//=========================================================
Vector CBot :: Wander( void )
{
	Vector vecForward;

	g_engfuncs.pfnAngleVectors( pev->v_angle, vecForward, NULL, NULL );

	if ( pev->flags & FL_WATERJUMP )
	{
		Vector vecSpot = pev->origin + vecForward * 48 + Vector( 0, 0, 16 );

		if ( FVisible( vecSpot ) )
		{
			m_fGoalVisible = FALSE;
			m_fHasGoalPos = TRUE;
			return vecSpot;
		}
	}

	m_fGoalVisible = TRUE;

	Vector vecAhead = pev->origin + vecForward * 64;
	if ( !FVisible( vecAhead ) )
		return Vector( 0, 0, 0 );

	Vector vecCell = g_BotNavigator.PosToIndex( pev->origin ) + Vector( 0, 0, 1 );
	Vector vecBest = Vector( 0, 0, 0 );
	float flBest = -60000;

	MAKE_VECTORS( pev->v_angle );

	Vector vecFwd = gpGlobals->v_forward * 64;
	Vector vecFwdCell = g_BotNavigator.PosToIndex( pev->origin + vecFwd ) - vecCell;
	Vector vecRight = gpGlobals->v_right * 64;
	Vector vecRightCell = g_BotNavigator.PosToIndex( pev->origin + vecRight ) - vecCell;

	int iDist = 20;
	do
	{
		int iSide = RANDOM_LONG( 0, 1 ) ? 1 : -1;

		for ( int i = -10; i <= 10; i++ )
		{
			Vector vecSideDir = Vector( vecRightCell.x * iSide, vecRightCell.y * iSide, 0 );
			Vector vecSide = vecSideDir * 10;	// sic: not * i
			Vector vecStep = Vector( vecFwdCell.x * iDist, vecFwdCell.y * iDist, 0 );
			Vector vecTest = vecCell + vecStep + vecSide;
			Vector vecTestCell = vecTest;

			if ( g_BotNavigator.GetVisits( vecTest.x, vecTest.y, vecTest.y ) > flBest )	// sic: y for z
			{
				Vector vecPos = g_BotNavigator.IndexToPos( vecTestCell );
				Vector vecSpot = vecPos;
				vecSpot.z = pev->origin.z;
				Vector vecDir = vecSpot - pev->origin;
				vecSpot = vecSpot + vecDir.Normalize() * 64;

				if ( FVisible( vecSpot ) )
				{
					vecBest = vecPos;
					flBest = g_BotNavigator.GetVisits( vecTestCell.x, vecTestCell.y, vecTestCell.y );	// sic
					if ( flBest == 0 )
						flBest = 0.5;
					m_fHasGoalPos = TRUE;
				}
			}
		}

		if ( iDist - RANDOM_LONG( 1, 3 ) <= 1 )
			break;
		iDist -= RANDOM_LONG( 1, 3 );
	} while ( iDist > 3 );

	return vecBest;
}


//=========================================================
// the visit grid: 160 cells of 64 units per axis, centred
// on the origin; players moving into a cell count a visit
//=========================================================
float CBotNav :: IndexToCoord( int i )
{
	return ( i - BOTNAV_GRID / 2 ) * BOTNAV_CELL;
}

Vector CBotNav :: IndexToPos( const Vector &vecIndex )
{
	return Vector( IndexToCoord( vecIndex.x ), IndexToCoord( vecIndex.y ), IndexToCoord( vecIndex.z ) );
}

int CBotNav :: GridIndex( float fl )
{
	return ( fl + BOTNAV_GRID / 2 * BOTNAV_CELL ) / BOTNAV_CELL;
}

Vector CBotNav :: PosToIndex( const Vector &vecPos )
{
	return Vector( GridIndex( vecPos.x ), GridIndex( vecPos.y ), GridIndex( vecPos.z ) );
}

short CBotNav :: GetVisits( int x, int y, int z )
{
	short sVisits = m_sGrid[x][y][z];

	if ( sVisits < 0 )
		return 0;
	return sVisits;
}

BOOL CBotNav :: Load( const char *pszMap )
{
	char szFile[50] = "default";

	sprintf( szFile, "wantedhl/maps/%s.nav", pszMap );
	ALERT( at_console, "Loading cowbot navigation file: " );
	ALERT( at_console, szFile );
	ALERT( at_console, "..." );
	return ReadNavFile( &g_BotNavigator.m_sGrid[0][0][0], szFile );
}

// one "x.y.z=visits" line per nonzero cell
BOOL CBotNav :: ReadNavFile( short *pGrid, char *pszFile )
{
	ifstream file( pszFile, ios::in | ios::nocreate );
	char szLine[32];

	if ( file.bad() )
		return FALSE;

	while ( file.getline( szLine, 32 ) )
		ParseLine( pGrid, szLine );

	file.close();
	return TRUE;
}

static void GetToken( char *pszOut, char **ppsz, char cDelim, char cEnd );	// name ours

void CBotNav :: ParseLine( short *pGrid, char *pszLine )
{
	char szX[4], szY[4], szZ[4], szVisits[8];

	GetToken( szX, &pszLine, '.', 0 );
	GetToken( szY, &pszLine, '.', 0 );
	GetToken( szZ, &pszLine, '=', 0 );
	GetToken( szVisits, &pszLine, '\n', 0 );

	int x = atoi( szX );
	int y = atoi( szY );
	int z = atoi( szZ );
	pGrid[( x * BOTNAV_GRID + y ) * BOTNAV_GRID + z] = atoi( szVisits );
}

// copy up to cDelim, cEnd or the end of the string, and step past the
// delimiter
static void GetToken( char *pszOut, char **ppsz, char cDelim, char cEnd )
{
	while ( **ppsz != cDelim && **ppsz && **ppsz != cEnd )
	{
		*pszOut++ = **ppsz;
		(*ppsz)++;
	}

	if ( **ppsz != cEnd && strlen( *ppsz ) )
		(*ppsz)++;

	*pszOut = 0;
}

void CBotNav :: HalveVisits( void )
{
	for ( int x = 0; x < BOTNAV_GRID; x++ )
	{
		for ( int y = 0; y < BOTNAV_GRID; y++ )
		{
			for ( int z = 0; z < BOTNAV_GRID; z++ )
			{
				if ( m_sGrid[x][y][z] != 1 )
					m_sGrid[x][y][z] /= 2;
			}
		}
	}
}

void CBotNav :: ClearGrid( void )
{
	int *p = (int *)m_sGrid;

	for ( int i = 0; i < sizeof( m_sGrid ) / sizeof( int ); i++ )
		*p++ = 0;
}

BOOL CBotNav :: InGrid( const Vector &vec )
{
	if ( vec.x > 4096 )
		return FALSE;
	if ( vec.y > 4096 )
		return FALSE;
	if ( vec.z > 4096 )
		return FALSE;
	if ( vec.x < -4096 )
		return FALSE;
	if ( vec.y < -4096 )
		return FALSE;
	if ( vec.z < -4096 )
		return FALSE;
	return TRUE;
}

void CBotNav :: AddVisit( const Vector &vec )
{
	if ( InGrid( vec ) )
	{
		int x = GridIndex( vec.x );
		int y = GridIndex( vec.y );
		int z = GridIndex( vec.z );

		if ( m_sGrid[x][y][z] < 32767 )
			m_sGrid[x][y][z]++;
		else
			HalveVisits();
	}
}

// a bot stuck here: count one visit less
void CBotNav :: AddVisit2( const Vector &vec )
{
	if ( InGrid( vec ) )
	{
		int x = GridIndex( vec.x );
		int y = GridIndex( vec.y );
		int z = GridIndex( vec.z );

		if ( m_sGrid[x][y][z] > -32767 )
			m_sGrid[x][y][z]--;
		else
			HalveVisits();
	}
}

void CBotNav :: Save( void )
{
	char szFile[64] = "";

	sprintf( szFile, "wantedhl/maps/%s.nav", STRING( gpGlobals->mapname ) );
	FILE *pFile = fopen( szFile, "w" );

	if ( pFile )
	{
		char szLine[25] = "error";

		for ( int x = 0; x < BOTNAV_GRID; x++ )
		{
			for ( int y = 0; y < BOTNAV_GRID; y++ )
			{
				for ( int z = 0; z < BOTNAV_GRID; z++ )
				{
					if ( g_BotNavigator.m_sGrid[x][y][z] )
					{
						sprintf( szLine, "%d.%d.%d=%d\n", x, y, z, g_BotNavigator.m_sGrid[x][y][z] );
						fprintf( pFile, szLine );
					}
				}
			}
		}

		fclose( pFile );
		g_flNavSaveTime = gpGlobals->time + 300;
	}
}


//=========================================================
// the node graph
//=========================================================

// is the floor below vecGoal in plain sight of vecNode?
BOOL NodeReachesGoal( const Vector &vecNode, const Vector &vecGoal )	// name ours
{
	TraceResult tr;
	Vector vecDown = Vector( vecGoal.x, vecGoal.y, vecGoal.z - 4096 );

	UTIL_TraceLine( vecGoal, vecDown, ignore_monsters, NULL, &tr );
	if ( tr.flFraction == 1 )
		return FALSE;

	UTIL_TraceLine( vecNode, tr.vecEndPos, ignore_monsters, NULL, &tr );
	if ( tr.flFraction != 1 )
		return FALSE;

	return TRUE;
}

// append a node for pEntity and link it to the nodes it can see that
// share its group (or its grouptrans group)
CBotNode *CBotNav :: AddNode( CInfoBotnode *pEntity, int fNoBacklinks )
{
	CBotNode *pNode;

	if ( !m_pHead )
	{
		pNode = m_pHead = new CBotNode;
		pNode->m_pPrev = NULL;
	}
	else
	{
		pNode = m_pTail->m_pNext = new CBotNode;
		pNode->m_pPrev = m_pTail;
	}

	pNode->m_pNext = NULL;
	pNode->m_vecOrigin = pEntity->pev->origin;
	pNode->m_cLinks = 0;
	pNode->m_fVisited = FALSE;
	pNode->m_iGroup = pEntity->m_iGroup;
	pNode->m_iGroupTrans = pEntity->m_iGroupTrans;
	pNode->m_iAction = pEntity->m_iAction;
	pNode->m_fRandomLinks = pEntity->m_fRandomLinks;
	pNode->m_fFlags = pEntity->pev->spawnflags & 1;
	m_pTail = pNode;

	for ( CBotNode *pOther = m_pHead; pOther && pOther != pNode; pOther = pOther->m_pNext )
	{
		if ( pEntity->FVisible( pOther->m_vecOrigin ) )
		{
			if ( pNode->m_iGroup == pOther->m_iGroup || pNode->m_iGroupTrans == pOther->m_iGroup ||
				pNode->m_iGroup == pOther->m_iGroupTrans || pNode->m_iGroup == -1 )
				LinkNodes( pNode, pOther, fNoBacklinks );
		}
	}

	return pNode;
}

void CBotNav :: LinkNodes( CBotNode *pFrom, CBotNode *pTo, int fNoBacklink )
{
	if ( pFrom->m_cLinks < MAX_BOTNODE_LINKS )
	{
		pFrom->m_pLinks[pFrom->m_cLinks] = pTo;
		pFrom->m_cLinks++;
	}

	if ( !fNoBacklink && pTo->m_cLinks < MAX_BOTNODE_LINKS )
	{
		pTo->m_pLinks[pTo->m_cLinks] = pFrom;
		pTo->m_cLinks++;
	}
}

void CBotNav :: RemoveNode( CBotNode *pNode )
{
	if ( m_pHead == pNode )
		m_pHead = pNode->m_pNext;
	if ( m_pTail == pNode )
		m_pTail = pNode->m_pPrev;
	if ( pNode->m_pPrev )
		pNode->m_pPrev->m_pNext = pNode->m_pNext;
	if ( pNode->m_pNext )
		pNode->m_pNext->m_pPrev = pNode->m_pPrev;

	// drop the links to it
	for ( int i = 0; i < pNode->m_cLinks; i++ )
	{
		CBotNode *pLink = pNode->m_pLinks[i];
		int k = -1;

		for ( int j = 0; j < pLink->m_cLinks; j++ )
		{
			if ( pLink->m_pLinks[j] != pNode )
			{
				k++;
				if ( k < j )
					pLink->m_pLinks[k] = pLink->m_pLinks[j];
			}
		}
		pLink->m_cLinks = k + 1;
	}

	delete pNode;
}

// the first node after pAfter that pLooker can see (unused)
CBotNode *CBotNav :: FindVisibleNode( CBaseEntity *pLooker, CBotNode *pAfter )
{
	CBotNode *pNode = m_pHead;

	if ( pAfter )
	{
		while ( pNode && pNode != pAfter )
			pNode = pNode->m_pNext;
		if ( !pNode )
			return NULL;
		pNode = pNode->m_pNext;
		if ( !pNode )
			return NULL;
	}

	while ( pNode )
	{
		if ( pLooker->FVisible( pNode->m_vecOrigin ) )
			break;
		pNode = pNode->m_pNext;
	}

	return pNode;
}

CBotLandmark *NearestLandmark( const Vector &vec );
BOOL LandmarkLeadsTo( CBotLandmark *pTarget, CBotLandmark *pAvoid, CBotLandmark *pFrom );
CBotLandmark *LandmarkBefore( CBotLandmark *pTarget, CBotLandmark *pFrom );
BOOL LandmarkCloser( CBotLandmark *pMark, CBotLandmark *pBest, const Vector &vecGoal );

//=========================================================
// FindPath - depth-first search from pNode for a node whose
// floor below pGoal is in sight; links are tried nearest
// the landmark on the way to the goal first (or a random
// one first). On success m_pRoute holds the steps.
//=========================================================
CBotNode *CBotNav :: FindPath( CBaseEntity *pGoal, CBotNode *pNode )
{
	CBotNode	*pFound = NULL;
	CBotPathStep	*pPrev = NULL;
	int			iFirst;
	float		flBest;
	int			i;

	if ( !pNode || m_iDepth + 1 >= m_iMaxDepth - 1 )
		return NULL;

	g_cFindPaths++;

	if ( !m_pPathHead )
	{
		m_pPathHead = m_pPathTail = new CBotPathStep;
	}
	else
	{
		pPrev = m_pPathTail;
		m_pPathTail->m_pNext = new CBotPathStep;
		m_pPathTail = m_pPathTail->m_pNext;
	}

	if ( g_cFindPaths > 512 )
	{
		m_iDepth--;
		if ( m_pPathTail == m_pPathHead )
			m_pPathHead = NULL;
		delete m_pPathTail;
		if ( pPrev )
			pPrev->m_pNext = NULL;
		m_pPathTail = pPrev;
		return NULL;
	}

	iFirst = 0;
	flBest = 8192;

	if ( !pNode->m_fRandomLinks )
	{
		// head for the landmark on the way to the goal
		CBotLandmark *pMark = NearestLandmark( pNode->m_vecOrigin );
		CBotLandmark *pBest = pMark;
		Vector vecToward = pGoal->Center();

		if ( pMark )
		{
			vecToward = pMark->m_vecOrigin;

			for ( int iLink = 0; iLink < pNode->m_cLinks; iLink++ )
			{
				CBotLandmark *pLinkMark = NearestLandmark( pNode->m_pLinks[iLink]->m_vecOrigin );
				if ( pLinkMark && LandmarkCloser( pLinkMark, pBest, pGoal->Center() ) )
				{
					pBest = pLinkMark;
					vecToward = pLinkMark->m_vecOrigin;
				}
			}

			if ( pBest == pMark )
			{
				CBotLandmark *pNext = LandmarkBefore( pBest, NearestLandmark( pGoal->Center() ) );
				if ( pNext )
					vecToward = pNext->m_vecOrigin;
			}
		}

		for ( int iDist = 0; iDist < pNode->m_cLinks; iDist++ )
		{
			float flDist = ( pNode->m_pLinks[iDist]->m_vecOrigin - vecToward ).Length2D();
			if ( flDist < flBest )
			{
				iFirst = iDist;
				flBest = flDist;
			}
		}
	}
	else
	{
		if ( !m_fRandomLinks || pNode->m_cLinks <= 2 )
			goto search;
		iFirst = RANDOM_LONG( 1, pNode->m_cLinks - 1 );
	}

	if ( iFirst > 0 )
	{
		CBotNode *pSwap = pNode->m_pLinks[0];
		pNode->m_pLinks[0] = pNode->m_pLinks[iFirst];
		pNode->m_pLinks[iFirst] = pSwap;
	}

search:
	m_pPathTail->m_pNext = NULL;
	m_iDepth++;

	for ( i = 0; i < pNode->m_cLinks; i++ )
	{
		if ( pNode->m_pLinks[i] && !pNode->m_pLinks[i]->m_fVisited )
		{
			m_pPathTail->m_vecOrigin = pNode->m_pLinks[i]->m_vecOrigin;
			m_pPathTail->m_iAction = pNode->m_pLinks[i]->m_iAction;

			if ( !pNode->m_pLinks[i]->m_fFlags && NodeReachesGoal( pNode->m_pLinks[i]->m_vecOrigin, pGoal->Center() ) )
			{
				// found: copy the path into a route, ending at the goal
				CBotPathStep *pStep = m_pPathHead;
				int j = 0;

				if ( m_pRoute )
					delete m_pRoute;

				m_iMaxDepth = m_iDepth + 1;
				CBotRoute *pRoute = (CBotRoute *)malloc( m_iMaxDepth * sizeof( CBotRoute ) );
				m_pRoute = pRoute;

				if ( pRoute )
				{
					for ( ; j < m_iDepth; j++ )
					{
						pRoute->m_vecOrigin = pStep->m_vecOrigin;
						pRoute->m_iAction = pStep->m_iAction;
						pRoute->m_flRadius = 32;
						if ( pRoute->m_iAction == 1 )
							pRoute->m_flRadius = 26;
						if ( pRoute->m_iAction == 2 )
							pRoute->m_flRadius = 6;
						pStep = pStep->m_pNext;
						pRoute++;
					}

					Vector vecGoal = pGoal->Center() + Vector( 0, 0, 16 );
					pRoute->m_vecOrigin = vecGoal;
					pRoute->m_iAction = 0;
					pRoute->m_flRadius = 32;
					pFound = pNode->m_pLinks[i];
				}
				break;
			}

			pNode->m_pLinks[i]->m_fVisited = TRUE;
			pFound = FindPath( pGoal, pNode->m_pLinks[i] );
			pNode->m_pLinks[i]->m_fVisited = FALSE;

			if ( pFound )
				return pFound;
		}
	}

	m_iDepth--;
	if ( m_pPathTail == m_pPathHead )
		m_pPathHead = NULL;
	delete m_pPathTail;
	if ( pPrev )
		pPrev->m_pNext = NULL;
	m_pPathTail = pPrev;
	return pFound;
}

// all players not solid / solid again (unused; names ours)
void BotNavPlayersNotSolid( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
		{
			pPlayer->pev->solid = SOLID_NOT;
			UTIL_SetOrigin( pPlayer->pev, pPlayer->pev->origin );
		}
	}
}

void BotNavPlayersSolid( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
		{
			pPlayer->pev->solid = SOLID_SLIDEBOX;
			UTIL_SetOrigin( pPlayer->pev, pPlayer->pev->origin );
		}
	}
}

//=========================================================
// BuildRoute - a route for pBot to pGoal: a temporary node
// at the bot, linked one way to the nodes it sees and can
// walk to, searched from and removed again.
//=========================================================
CBotRoute *CBotNav :: BuildRoute( CBot *pBot, CBaseEntity *pGoal )
{
	CInfoBotnode	node;
	entvars_t		vars;

	memset( &vars, 0, sizeof( vars ) );
	node.pev = &vars;
	vars.origin = pBot->pev->origin;
	vars.view_ofs = g_vecZero;
	node.m_iGroup = -1;
	node.m_iGroupTrans = -1;
	node.m_iAction = 0;		// sic: m_fRandomLinks left unset

	CBotNode *pStart = AddNode( &node, 1 );

	int k = -1;
	for ( int i = 0; i < pStart->m_cLinks; i++ )
	{
		if ( !pStart->m_pLinks[i]->m_fFlags && pBot->CanWalkTo( pStart->m_pLinks[i]->m_vecOrigin ) )
		{
			k++;
			if ( k < i )
				pStart->m_pLinks[k] = pStart->m_pLinks[i];
		}
	}
	pStart->m_cLinks = k + 1;

	g_cFindPaths = 0;
	m_iDepth = 0;
	m_iMaxDepth = 48;
	m_pPathHead = NULL;
	m_pRoute = NULL;

	FindPath( pGoal, pStart );
	RemoveNode( pStart );

	return m_pRoute;
}


//=========================================================
// info_botlandmark: landmarks named by targetname and linked
// by link0-link2 (up to three); the path search heads for
// the landmark on the way to the goal
//=========================================================
class CInfoBotlandmark : public CBaseEntity
{
public:
	void	Spawn( void );
	void	KeyValue( KeyValueData *pkvd );
	void	Activate( void );

	string_t	m_iszLink[MAX_LANDMARK_LINKS];
};

LINK_ENTITY_TO_CLASS( info_botlandmark, CInfoBotlandmark );

void CInfoBotlandmark :: Activate( void )
{
	CBotLandmark *pMark;

	if ( !pev->targetname || !g_BotNavigator.m_pLandmarks )
		return;

	// find our own record
	pMark = g_BotNavigator.m_pLandmarks;
	while ( 1 )
	{
		if ( !strcmp( STRING( pMark->m_iszName ), STRING( pev->targetname ) ) )
			break;
		pMark = pMark->m_pNext;
		if ( !pMark )
			break;
	}

	if ( !pMark )
		return;

	if ( g_BotNavigator.m_pLandmarks )
	{
		CBotLandmark **ppLink = pMark->m_pLinks;

		for ( CBotLandmark *pOther = g_BotNavigator.m_pLandmarks; pOther; pOther = pOther->m_pNext )
		{
			CBotLandmark *pFound = NULL;

			for ( int i = 0; i < MAX_LANDMARK_LINKS; i++ )
			{
				if ( m_iszLink[i] && !strcmp( STRING( m_iszLink[i] ), STRING( pOther->m_iszName ) ) )
					pFound = pOther;
			}

			if ( pFound )
			{
				// link both ways
				*ppLink = pFound;
				for ( int j = 0; j < MAX_LANDMARK_LINKS; j++ )
				{
					if ( !pFound->m_pLinks[j] )
					{
						pFound->m_pLinks[j] = pMark;
						break;
					}
				}
				ppLink++;
			}
		}
	}

	pev->flags |= FL_KILLME;
}

void CInfoBotlandmark :: Spawn( void )
{
	CBotLandmark *pMark;

	if ( !g_BotNavigator.m_pLandmarks )
	{
		pMark = new CBotLandmark;
		g_BotNavigator.m_pLandmarks = pMark;
	}
	else
	{
		CBotLandmark *pLast;
		for ( pLast = g_BotNavigator.m_pLandmarks; pLast->m_pNext; pLast = pLast->m_pNext )
			;
		pMark = new CBotLandmark;
		pLast->m_pNext = pMark;
	}

	pMark->m_iszName = pev->targetname;
	pMark->m_vecOrigin = pev->origin;
	pMark->m_fVisited = FALSE;
	for ( int i = 0; i < MAX_LANDMARK_LINKS; i++ )
		pMark->m_pLinks[i] = NULL;
	pMark->m_pNext = NULL;

	pev->effects |= EF_NODRAW;
}

void CInfoBotlandmark :: KeyValue( KeyValueData *pkvd )
{
	if ( strstr( pkvd->szKeyName, "link" ) && strlen( pkvd->szKeyName ) > 4 )
	{
		m_iszLink[atoi( pkvd->szKeyName + 4 )] = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
}

// the landmark nearest vec, by 2D distance
CBotLandmark *NearestLandmark( const Vector &vec )
{
	CBotLandmark *pBest = g_BotNavigator.m_pLandmarks;	// sic: the first one is never measured

	if ( !pBest )
		return NULL;

	float flBest = 8192;

	for ( CBotLandmark *pMark = pBest->m_pNext; pMark; pMark = pMark->m_pNext )
	{
		if ( ( pMark->m_vecOrigin - vec ).Length2D() < flBest )
		{
			flBest = ( pMark->m_vecOrigin - vec ).Length2D();
			pBest = pMark;
		}
	}

	return pBest;
}

// does pFrom lead to pTarget without passing pAvoid?
BOOL LandmarkLeadsTo( CBotLandmark *pTarget, CBotLandmark *pAvoid, CBotLandmark *pFrom )
{
	if ( pTarget == pFrom )
		return TRUE;
	if ( pAvoid == pFrom )
		return FALSE;

	BOOL fFound = FALSE;
	for ( int i = 0; i < MAX_LANDMARK_LINKS; i++ )
	{
		if ( pFrom->m_pLinks[i] )
		{
			if ( pFrom->m_pLinks[i]->m_fVisited )
				continue;
			pFrom->m_pLinks[i]->m_fVisited = TRUE;
			fFound = LandmarkLeadsTo( pTarget, pAvoid, pFrom->m_pLinks[i] );
			pFrom->m_pLinks[i]->m_fVisited = FALSE;
		}
		if ( fFound )
			break;
	}
	return fFound;
}

// the landmark next to pTarget on the way from pFrom
CBotLandmark *LandmarkBefore( CBotLandmark *pTarget, CBotLandmark *pFrom )
{
	int i;

	if ( pFrom == pTarget )
		return NULL;

	for ( i = 0; i < MAX_LANDMARK_LINKS; i++ )
	{
		if ( pFrom->m_pLinks[i] && pFrom->m_pLinks[i] == pTarget )
			return pFrom;
	}

	CBotLandmark *pFound = NULL;
	for ( i = 0; i < MAX_LANDMARK_LINKS; i++ )
	{
		if ( pFrom->m_pLinks[i] )
		{
			if ( pFrom->m_pLinks[i]->m_fVisited )
				continue;
			pFrom->m_pLinks[i]->m_fVisited = TRUE;
			pFound = LandmarkBefore( pTarget, pFrom->m_pLinks[i] );
			pFrom->m_pLinks[i]->m_fVisited = FALSE;
		}
		if ( pFound )
			return pFound;
	}
	return pFound;
}

// is pMark on the way from pBest towards the landmark nearest vecGoal?
BOOL LandmarkCloser( CBotLandmark *pMark, CBotLandmark *pBest, const Vector &vecGoal )
{
	CBotLandmark *pGoalMark = NearestLandmark( vecGoal );

	if ( !pGoalMark )
		return FALSE;
	if ( pMark == pBest )
		return FALSE;
	if ( !pGoalMark->m_pLinks[0] )
		return FALSE;
	if ( pMark == pGoalMark )
		return TRUE;
	if ( pBest == pGoalMark )
		return FALSE;

	return LandmarkLeadsTo( pMark, pBest, pGoalMark->m_pLinks[0] );
}


//=========================================================
// CBotProfile
//=========================================================
CBotProfile :: CBotProfile( CBot *pBot )
{
	m_flAim = RANDOM_FLOAT( 0, 100 );
	m_flAggression = RANDOM_FLOAT( 0, 100 );
	m_flJump = RANDOM_FLOAT( 0, 100 );
	m_flNotice = RANDOM_FLOAT( 0, 100 );
	m_flPause = RANDOM_FLOAT( 0, 100 );
	m_pBot = pBot;
	strcpy( m_szName, "Cowbot" );
	strcpy( m_szModel, "gordon" );
	strcpy( m_szTopColor, "0" );
	strcpy( m_szBottomColor, "0" );
}

CBotProfile :: ~CBotProfile()
{
}

// sv_botskill 50 leaves a skill alone; 0 scales it down to nothing,
// 100 up to 100
float CBotProfile :: SkillAdjust( float flSkill )
{
	float flAdjust = sv_botskill.value - 50;
	BOOL fLower = flAdjust < 0;
	float flScale = (float)abs( (int)flAdjust ) / 50;

	if ( fLower )
		return ( 1 - flScale ) * flSkill;
	return ( 100 - flSkill ) * flScale + flSkill;
}

// how much the bot likes a weapon (flDist unused)
float CBotProfile :: RateWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pItem, float flDist )
{
	if ( FClassnameIs( pItem->pev, "weapon_knife" ) || FClassnameIs( pItem->pev, "weapon_pick" ) )
		return 10;
	if ( FClassnameIs( pItem->pev, "weapon_beartrap" ) || FClassnameIs( pItem->pev, "weapon_scorpion" ) || FClassnameIs( pItem->pev, "weapon_dynamite" ) )
		return 20;

	// guns are no good under water
	if ( FClassnameIs( pItem->pev, "weapon_pistol" ) && pPlayer->pev->waterlevel <= 1 )
		return 30;
	if ( FClassnameIs( pItem->pev, "weapon_colts" ) && pPlayer->pev->waterlevel <= 1 )
		return 40;
	if ( FClassnameIs( pItem->pev, "weapon_bow" ) )
		return 50;
	if ( FClassnameIs( pItem->pev, "weapon_shotgun" ) && pPlayer->pev->waterlevel <= 1 )
		return 70;
	if ( FClassnameIs( pItem->pev, "weapon_buffalo" ) && pPlayer->pev->waterlevel <= 1 )
		return 80;
	if ( FClassnameIs( pItem->pev, "weapon_cannon" ) )
		return 80;
	if ( FClassnameIs( pItem->pev, "weapon_gattlinggun" ) && pPlayer->pev->waterlevel <= 1 )
		return 80;
	if ( FClassnameIs( pItem->pev, "weapon_winchester" ) && pPlayer->pev->waterlevel <= 1 )
		return 90;
	return 1;
}

//=========================================================
// ParseFile - Load's loop (name ours): read the profile's
// lines: "Name: ", "Model: ", "Skill: n" (each skill becomes
// n times 1..20), "Topcolor: ", "Bottomcolor: "; lines
// starting with / are comments
//=========================================================
// An inline step: its share of Load's budget leaves getline's
// unlock out of line, as in retail (whose ios::unlock copy
// follows Load); in Load itself getline would get it all.
inline BOOL CBotProfile :: ParseFile( ifstream &file )
{
	char szLine[100];

	while ( 1 )
	{
		file.getline( szLine, 100 );

		if ( !file )
		{
			file.close();
			return TRUE;
		}

		if ( !strlen( szLine ) || szLine[0] == '/' )
			continue;

		if ( strstr( szLine, "Name:" ) )
		{
			if ( strlen( szLine ) > 6 )
				strcpy( m_szName, szLine + 6 );
		}
		else if ( strstr( szLine, "Model:" ) )
		{
			if ( strlen( szLine ) > 7 )
				strcpy( m_szModel, szLine + 7 );
		}
		else if ( strstr( szLine, "Skill:" ) )
		{
			if ( strlen( szLine ) > 7 )
			{
				int iSkill = atoi( szLine + 7 );
				m_flAim = RANDOM_FLOAT( 1, 20 ) * iSkill;
				m_flAggression = RANDOM_FLOAT( 1, 20 ) * iSkill;
				m_flJump = RANDOM_FLOAT( 1, 20 ) * iSkill;
				m_flNotice = RANDOM_FLOAT( 1, 20 ) * iSkill;
				m_flPause = RANDOM_FLOAT( 1, 20 ) * iSkill;
			}
		}
		else if ( strstr( szLine, "Topcolor:" ) )
		{
			if ( strlen( szLine ) > 10 )
				strcpy( m_szTopColor, szLine + 10 );
		}
		else if ( strstr( szLine, "Bottomcolor:" ) )
		{
			if ( strlen( szLine ) > 13 )
				strcpy( m_szBottomColor, szLine + 13 );
		}
	}
}

//=========================================================
// Load - read a profile from wantedhl/bots/*.bot
//=========================================================
BOOL CBotProfile :: Load( char *pszFile )
{
	ifstream file( pszFile );

	if ( file.bad() )
		return FALSE;

	return ParseFile( file );
}
