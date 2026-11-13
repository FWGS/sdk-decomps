/***
*
*	Counter-Strike 1.4, hostage_localnav.cpp: the hostages' local navigation
*	(CLocalNav; see hostage_localnav.h).
*
*	Written from the Windows DLL, the 1.4 Linux library (which has every
*	function, also the ones the Windows compiler expanded inline or the
*	linker dropped) and Counter-Strike 1.6's debug information (names of
*	functions, parameters and locals, the order of the definitions).
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "hostage.h"
#include "hostage_localnav.h"

// The 1.4 Linux library has these three ahead of CLocalNav's functions in
// this object. The two printing functions do nothing in either library, yet
// the Windows DLL keeps the 1,024 bytes of each one's buffer among this
// object's uninitialised data and calls them (an empty function would have
// been expanded inline), and the Linux functions set up the register for
// global data they never use: the bodies are there and unreachable. They
// are Counter-Strike 1.6's (cbase.cpp, bot_util.cpp: its names, its calls);
// how 1.4's text switched them off is not known.
void CONSOLE_ECHO( char *pszMsg, ... )
{
	return;

	va_list argptr;
	static char szStr[1024];

	va_start( argptr, pszMsg );
	vsprintf( szStr, pszMsg, argptr );
	va_end( argptr );

	(*g_engfuncs.pfnServerPrint)( szStr );
}

void CONSOLE_ECHO_LOGGED( char *pszMsg, ... )
{
	return;

	va_list argptr;
	static char szStr[1024];

	va_start( argptr, pszMsg );
	vsprintf( szStr, pszMsg, argptr );
	va_end( argptr );

	(*g_engfuncs.pfnServerPrint)( szStr );
	UTIL_LogPrintf( szStr );
}

// only the sprite is left of the beam (names ours)
void UTIL_DrawBeamPoints( Vector vecStart, Vector vecEnd, int iLifetime, byte bRed, byte bGreen, byte bBlue )
{
	static short s_sModelIndex = PRECACHE_MODEL( "sprites/smoke.spr" );
}

float CLocalNav::s_flStepSize = 0;

EHANDLE CLocalNav::queue[ MAX_HOSTAGES_NAV ];
int CLocalNav::qptr = 0;
int CLocalNav::tot_inqueue = 0;
float CLocalNav::nodeval = 0;
float CLocalNav::flNextCvarCheck = 0;
float CLocalNav::flLastThinkTime = 0;

EHANDLE CLocalNav::hostages[ MAX_HOSTAGES_NAV ];
int CLocalNav::tot_hostages = 0;


CLocalNav::CLocalNav( CHostage *pOwner )
{
	m_pOwner = pOwner;
	m_pTargetEnt = NULL;
	m_nodeArr = new localnode_t[ MAX_NODES ];

	CONSOLE_ECHO( "Allocated m_nodeArr: %d nodes, %lu bytes\n", MAX_NODES, sizeof( localnode_t ) * MAX_NODES );

	if ( tot_hostages < MAX_HOSTAGES_NAV )
	{
		hostages[ tot_hostages ] = pOwner;
		tot_hostages++;
	}
	else
		CONSOLE_ECHO( "queue full (creation)\n" );
}

CLocalNav::~CLocalNav( void )
{
	delete [] m_nodeArr;
}

node_index_t CLocalNav::AddNode( node_index_t nindexParent, Vector &vecLoc, int offsetX, int offsetY, byte bDepth )
{
	localnode_t *nodeNew;

	if ( m_nindexAvailableNode == MAX_NODES )
		return NODE_INVALID_EMPTY;

	nodeNew = GetNode( m_nindexAvailableNode );

	nodeNew->vecLoc = vecLoc;
	nodeNew->offsetX = offsetX;
	nodeNew->offsetY = offsetY;
	nodeNew->bDepth = bDepth;
	nodeNew->fSearched = FALSE;
	nodeNew->nindexParent = nindexParent;

	return m_nindexAvailableNode++;
}

// (inline: Counter-Strike 1.6's debug information has it declared inline at this place, and retail's link had no
// copy of it among its functions: the kept copies of folded functions need one 16-byte function fewer here)
inline localnode_t *CLocalNav::GetNode( node_index_t nindex )
{
	return &m_nodeArr[ nindex ];
}

node_index_t CLocalNav::NodeExists( int offsetX, int offsetY )
{
	node_index_t nindexCurrent;
	localnode_t *nodeCurrent;

	nindexCurrent = m_nindexAvailableNode;
	nodeCurrent = GetNode( m_nindexAvailableNode );

	while ( nindexCurrent-- )
	{
		nodeCurrent--;

		if ( nodeCurrent->offsetX == offsetX && nodeCurrent->offsetY == offsetY )
			return nindexCurrent;
	}

	return NODE_INVALID_EMPTY;
}

void CLocalNav::AddPathNodes( node_index_t nindexSource, int fNoMonsters )
{
	AddPathNode( nindexSource, 1, 0, fNoMonsters );
	AddPathNode( nindexSource, -1, 0, fNoMonsters );
	AddPathNode( nindexSource, 0, 1, fNoMonsters );
	AddPathNode( nindexSource, 0, -1, fNoMonsters );
	AddPathNode( nindexSource, 1, 1, fNoMonsters );
	AddPathNode( nindexSource, 1, -1, fNoMonsters );
	AddPathNode( nindexSource, -1, 1, fNoMonsters );
	AddPathNode( nindexSource, -1, -1, fNoMonsters );
}

void CLocalNav::AddPathNode( node_index_t nindexSource, int offsetX, int offsetY, int fNoMonsters )
{
	int bDepth;
	Vector vecSource, vecDest;
	int offsetXAbs, offsetYAbs;

	if ( nindexSource == NODE_INVALID_EMPTY )
	{
		offsetXAbs = offsetX;
		offsetYAbs = offsetY;

		vecSource = m_vecStartingLoc;
		vecDest = vecSource + Vector( offsetX, offsetY, 0 ) * HOSTAGE_STEPSIZE;

		bDepth = 1;
	}
	else
	{
		localnode_t *nodeSource;
		localnode_t *nodeCurrent;
		node_index_t nindexCurrent;

		nodeSource = GetNode( nindexSource );
		offsetXAbs = offsetX + nodeSource->offsetX;
		offsetYAbs = offsetY + nodeSource->offsetY;

		// a node at this place already: nothing to add
		if ( NodeExists( offsetXAbs, offsetYAbs ) != NODE_INVALID_EMPTY )
			return;

		vecSource = nodeSource->vecLoc;
		vecDest = vecSource + Vector( offsetX, offsetY, 0 ) * HOSTAGE_STEPSIZE;

		// of the neighbours of the new node's place that can reach it, the
		// first node made becomes its parent
		nindexCurrent = m_nindexAvailableNode;
		nodeCurrent = GetNode( m_nindexAvailableNode );

		while ( nindexCurrent-- )
		{
			nodeCurrent--;

			if ( ABSOF( nodeCurrent->offsetX - offsetXAbs ) > 1 )
				continue;

			if ( ABSOF( nodeCurrent->offsetY - offsetYAbs ) > 1 )
				continue;

			if ( PathTraversable( nodeCurrent->vecLoc, vecDest, fNoMonsters ) != PATH_TRAVERSABLE_EMPTY )
			{
				nodeSource = nodeCurrent;
				nindexSource = nindexCurrent;
			}
		}

		vecSource = nodeSource->vecLoc;
		bDepth = nodeSource->bDepth + 1;
	}

	if ( PathTraversable( vecSource, vecDest, fNoMonsters ) != PATH_TRAVERSABLE_EMPTY )
	{
		AddNode( nindexSource, vecDest, offsetXAbs, offsetYAbs, bDepth );
	}
}

node_index_t CLocalNav::GetBestNode( Vector &vecOrigin, Vector &vecDest )
{
	node_index_t nindexCurrent;
	localnode_t *nodeCurrent;
	node_index_t nindexBest;
	float flBestVal;

	nindexBest = NODE_INVALID_EMPTY;
	nindexCurrent = 0;
	nodeCurrent = GetNode( nindexCurrent );
	flBestVal = 1000000.0;

	while ( nindexCurrent < m_nindexAvailableNode )
	{
		if ( nodeCurrent->fSearched != TRUE )
		{
			float flCurrentVal;
			float flDistFromStart;
			float flDistToDest;
			float flZDiff;

			flDistFromStart = nodeCurrent->bDepth * HOSTAGE_STEPSIZE;
			flDistToDest = ( nodeCurrent->vecLoc - vecDest ).Length();

			// a node more than a step above or below the goal counts a quarter more
			if ( ABSOF( nodeCurrent->vecLoc.z - vecDest.z ) > s_flStepSize )
				flZDiff = 1.25;
			else
				flZDiff = 1.0;

			flCurrentVal = ( flDistFromStart + flDistToDest ) * flZDiff;

			if ( flCurrentVal < flBestVal )
			{
				flBestVal = flCurrentVal;
				nindexBest = nindexCurrent;
			}
		}

		nindexCurrent++;
		nodeCurrent++;
	}

	return nindexBest;
}

int CLocalNav::SetupPathNodes( node_index_t nindex, Vector *vecNodes, int fNoMonsters )
{
	node_index_t nCurrentIndex;
	int nNodeCount;

	nCurrentIndex = nindex;
	nNodeCount = 0;

	while ( nCurrentIndex != NODE_INVALID_EMPTY )
	{
		localnode_t *nodeCurrent;
		Vector vecCurrentLoc;

		nodeCurrent = GetNode( nCurrentIndex );
		vecCurrentLoc = nodeCurrent->vecLoc;
		vecNodes[ nNodeCount++ ] = vecCurrentLoc;

		nCurrentIndex = nodeCurrent->nindexParent;
	}

	return nNodeCount;
}

int CLocalNav::GetFurthestTraversableNode( Vector &vecStartingLoc, Vector *vecNodes, int nTotalNodes, int fNoMonsters )
{
	int nCount;

	for ( nCount = 0; nCount < nTotalNodes; nCount++ )
	{
		if ( PathTraversable( vecStartingLoc, vecNodes[ nCount ], fNoMonsters ) != PATH_TRAVERSABLE_EMPTY )
			return nCount;
	}

	return NODE_INVALID_EMPTY;
}

node_index_t CLocalNav::FindPath( Vector &vecStart, Vector &vecDest, float flTargetRadius, int fNoMonsters )
{
	node_index_t nIndexBest;

	CONSOLE_ECHO( "findpath: %f\n", gpGlobals->time );

	nIndexBest = FindDirectPath( vecStart, vecDest, flTargetRadius, fNoMonsters );

	if ( nIndexBest != NODE_INVALID_EMPTY )
	{
		return nIndexBest;
	}

	m_vecStartingLoc = vecStart;
	m_nindexAvailableNode = 0;

	AddPathNodes( NODE_INVALID_EMPTY, fNoMonsters );
	nIndexBest = GetBestNode( vecStart, vecDest );

	while ( nIndexBest != NODE_INVALID_EMPTY )
	{
		localnode_t *node;
		Vector vecNodeLoc;
		float flDistToDest;

		node = GetNode( nIndexBest );
		node->fSearched = TRUE;

		vecNodeLoc = node->vecLoc;
		flDistToDest = ( vecDest - vecNodeLoc ).Length2D();

		if ( flDistToDest <= flTargetRadius )
			break;

		if ( flDistToDest <= HOSTAGE_STEPSIZE )
			break;

		// too far for the nodes that are left
		if ( ( flDistToDest - flTargetRadius ) > ( MAX_NODES - m_nindexAvailableNode ) * HOSTAGE_STEPSIZE || m_nindexAvailableNode == MAX_NODES )
		{
			nIndexBest = NODE_INVALID_EMPTY;
			break;
		}

		AddPathNodes( nIndexBest, fNoMonsters );
		nIndexBest = GetBestNode( vecNodeLoc, vecDest );
	}

	// the cost of the search: Think lets it decay, and no search starts
	// from the queue while it is above 17
	if ( m_nindexAvailableNode <= 10 )
		nodeval += 2;
	else if ( m_nindexAvailableNode <= 20 )
		nodeval += 4;
	else if ( m_nindexAvailableNode <= 30 )
		nodeval += 8;
	else if ( m_nindexAvailableNode <= 40 )
		nodeval += 13;
	else if ( m_nindexAvailableNode <= 50 )
		nodeval += 19;
	else if ( m_nindexAvailableNode <= 60 )
		nodeval += 26;
	else if ( m_nindexAvailableNode <= 70 )
		nodeval += 34;
	else if ( m_nindexAvailableNode <= 80 )
		nodeval += 43;
	else if ( m_nindexAvailableNode <= 90 )
		nodeval += 53;
	else if ( m_nindexAvailableNode <= 100 )
		nodeval += 64;
	else if ( m_nindexAvailableNode <= 110 )
		nodeval += 76;
	else if ( m_nindexAvailableNode <= 120 )
		nodeval += 89;
	else if ( m_nindexAvailableNode <= 130 )
		nodeval += 103;
	else if ( m_nindexAvailableNode <= 140 )
		nodeval += 118;
	else if ( m_nindexAvailableNode <= 150 )
		nodeval += 134;
	else if ( m_nindexAvailableNode <= 160 )
		nodeval += 151;
	else
		nodeval += 169;

	return nIndexBest;
}

node_index_t CLocalNav::FindDirectPath( Vector &vecStart, Vector &vecDest, float flTargetRadius, int fNoMonsters )
{
	Vector vecActualDest;
	Vector vecPathDir;
	Vector vecNodeLoc;
	node_index_t nindexLast;

	vecPathDir = ( vecDest - vecStart ).Normalize();
	vecActualDest = vecDest - ( vecPathDir * flTargetRadius );

	if ( PathTraversable( vecStart, vecActualDest, fNoMonsters ) == PATH_TRAVERSABLE_EMPTY )
	{
		return NODE_INVALID_EMPTY;
	}

	m_nindexAvailableNode = 0;
	nindexLast = NODE_INVALID_EMPTY;
	vecNodeLoc = vecStart;

	while ( ( vecNodeLoc - vecActualDest ).Length2D() >= HOSTAGE_STEPSIZE )
	{
		node_index_t nindexCurrent;

		vecNodeLoc = vecNodeLoc + ( vecPathDir * HOSTAGE_STEPSIZE );
		nindexCurrent = AddNode( nindexLast, vecNodeLoc );

		if ( nindexCurrent == NODE_INVALID_EMPTY )
			break;

		nindexLast = nindexCurrent;
	}

	return nindexLast;
}

BOOL CLocalNav::PathClear( Vector &vecOrigin, Vector &vecDest, int fNoMonsters, TraceResult &tr )
{
	TRACE_MONSTER_HULL( m_pOwner->edict(), vecOrigin, vecDest, fNoMonsters, m_pOwner->edict(), &tr );

	if ( tr.fStartSolid )
		return FALSE;

	if ( tr.flFraction == 1.0 )
		return TRUE;

	if ( tr.pHit == m_pTargetEnt )
	{
		CONSOLE_ECHO( "target edict hit!\n" );
		m_fTargetEntHit = TRUE;
		return TRUE;
	}

	return FALSE;
}

// Walks from vecSource towards vecDest a step (s_flStepSize) at a time and
// says what the way asks of the hostage (PATH_TRAVERSABLE_*); vecDest becomes
// the point reached. As in retail, a way of one unit or less leaves the loop
// alone and copies vecDestTmp, never set, into vecDest (the result is
// PATH_TRAVERSABLE_EMPTY then).
// Two spellings leave no trace in the Windows code but in the order of the
// squares of the first Normalize's length (y*y, z*z, x*x): the class name is
// tested through VARS (Counter-Strike 1.6 expands FClassnameIs( entvars_t *,
// ... ) here; the edict form gives y, x, z), and the distance left is taken
// from vecSrcTmp, just copied from vecDestTmp (the 1.4 Linux library reads
// vecSrcTmp's slot; vecDestTmp gives z, y, x and four bytes less).
int CLocalNav::PathTraversable( Vector &vecSource, Vector &vecDest, int fNoMonsters )
{
	TraceResult tr;
	Vector vecSrcTmp;
	Vector vecDestTmp;
	Vector vecDir;
	float flTotal;
	int retval = PATH_TRAVERSABLE_EMPTY;

	vecSrcTmp = vecSource;
	vecDir = ( vecDest - vecSource ).Normalize();
	vecDir.z = 0;

	flTotal = ( vecDest - vecSource ).Length2D();

	while ( flTotal > 1 )
	{
		BOOL fProgressThisTime;
		Vector vecSrcThisTime;

		if ( flTotal < s_flStepSize )
			vecDestTmp = vecDest;
		else
			vecDestTmp = vecSrcTmp + ( vecDir * s_flStepSize );

		m_fTargetEntHit = FALSE;
		fProgressThisTime = FALSE;
		vecSrcThisTime = vecSrcTmp;

		if ( PathClear( vecSrcTmp, vecDestTmp, fNoMonsters, tr ) )
		{
			vecDestTmp = tr.vecEndPos;
			fProgressThisTime = TRUE;

			if ( retval == PATH_TRAVERSABLE_EMPTY )
				retval = PATH_TRAVERSABLE_SLOPE;
		}
		else
		{
			if ( tr.fStartSolid )
				return PATH_TRAVERSABLE_EMPTY;

			// another hostage in the way
			if ( tr.pHit && !fNoMonsters && tr.pHit->v.classname )
			{
				if ( FClassnameIs( VARS( tr.pHit ), "hostage_entity" ) )
					return PATH_TRAVERSABLE_EMPTY;
			}

			vecSrcTmp = tr.vecEndPos;

			if ( tr.vecPlaneNormal.z > 0.7 )
			{
				if ( SlopeTraversable( vecSrcTmp, vecDestTmp, fNoMonsters, tr ) )
				{
					fProgressThisTime = TRUE;

					if ( retval == PATH_TRAVERSABLE_EMPTY )
						retval = PATH_TRAVERSABLE_SLOPE;
				}
			}
			else
			{
				if ( StepTraversable( vecSrcTmp, vecDestTmp, fNoMonsters, tr ) )
				{
					fProgressThisTime = TRUE;

					if ( retval == PATH_TRAVERSABLE_EMPTY )
						retval = PATH_TRAVERSABLE_STEP;
				}
				else if ( StepJumpable( vecSrcTmp, vecDestTmp, fNoMonsters, tr ) )
				{
					fProgressThisTime = TRUE;

					if ( retval == PATH_TRAVERSABLE_EMPTY )
						retval = PATH_TRAVERSABLE_STEPJUMPABLE;
				}
			}
		}

		if ( !fProgressThisTime )
			return PATH_TRAVERSABLE_EMPTY;

		// nothing to stand on within 300 units below: not a way
		Vector vecDropDest;

		vecDropDest = vecDestTmp;
		vecDropDest.z -= 300;

		if ( PathClear( vecDestTmp, vecDropDest, fNoMonsters, tr ) )
			return PATH_TRAVERSABLE_EMPTY;

		if ( !tr.fStartSolid )
			vecDestTmp = tr.vecEndPos;

		vecSrcTmp = vecDestTmp;

		if ( m_fTargetEntHit )
			flTotal = 0;
		else
			flTotal = ( vecDest - vecSrcTmp ).Length2D();
	}

	vecDest = vecDestTmp;

	return retval;
}

BOOL CLocalNav::SlopeTraversable( Vector &vecSource, Vector &vecDest, int fNoMonsters, TraceResult &tr )
{
	Vector vecSlopeEnd;
	Vector vecDown;

	vecSlopeEnd = vecDest;
	vecSlopeEnd.z = ( vecDest - vecSource ).Length2D() * tan( ( 90 - UTIL_VecToAngles( tr.vecPlaneNormal ).x ) * ( M_PI / 180 ) ) + vecSource.z;

	if ( !PathClear( vecSource, vecSlopeEnd, fNoMonsters, tr ) )
	{
		if ( tr.fStartSolid )
			return FALSE;

		if ( ( tr.vecEndPos - vecSource ).Length2D() < 1 )
			return FALSE;
	}

	vecSlopeEnd = tr.vecEndPos;

	vecDown = vecSlopeEnd;
	vecDown.z -= s_flStepSize;

	if ( !PathClear( vecSlopeEnd, vecDown, fNoMonsters, tr ) )
	{
		if ( tr.fStartSolid )
			vecDest = vecSlopeEnd;
		else
			vecDest = tr.vecEndPos;
	}
	else
		vecDest = tr.vecEndPos;

	return TRUE;
}

// nothing calls it (the Windows linker dropped it): written from the Linux library
BOOL CLocalNav::LadderTraversable( Vector &vecSource, Vector &vecDest, int fNoMonsters, TraceResult &tr )
{
	Vector vecLadderStart;
	Vector vecLadderDest;

	vecLadderStart = tr.vecEndPos;
	vecLadderDest = vecLadderStart;
	vecLadderDest.z += HOSTAGE_STEPSIZE;

	if ( !PathClear( vecLadderStart, vecLadderDest, fNoMonsters, tr ) )
	{
		if ( tr.fStartSolid )
			return FALSE;

		if ( ( tr.vecEndPos - vecLadderStart ).Length() < 1 )
			return FALSE;
	}

	vecLadderStart = tr.vecEndPos;
	vecDest.z = vecLadderStart.z;

	return PathTraversable( vecLadderStart, vecDest, fNoMonsters );
}

BOOL CLocalNav::StepTraversable( Vector &vecSource, Vector &vecDest, int fNoMonsters, TraceResult &tr )
{
	Vector vecStepStart;
	Vector vecStepDest;
	BOOL fFwdTrace = FALSE;
	float flFwdFraction;

	vecStepStart = vecSource;
	vecStepDest = vecDest;

	vecStepStart.z += s_flStepSize;
	vecStepDest.z = vecStepStart.z;

	if ( !PathClear( vecStepStart, vecStepDest, fNoMonsters, tr ) )
	{
		if ( tr.fStartSolid )
			return FALSE;

		flFwdFraction = ( tr.vecEndPos - vecStepStart ).Length();

		if ( flFwdFraction < 1 )
			return FALSE;
	}

	vecStepStart = tr.vecEndPos;

	vecStepDest = vecStepStart;
	vecStepDest.z -= s_flStepSize;

	if ( !PathClear( vecStepStart, vecStepDest, fNoMonsters, tr ) )
	{
		if ( tr.fStartSolid )
			vecDest = vecStepStart;
		else
			vecDest = tr.vecEndPos;
	}
	else
		vecDest = tr.vecEndPos;

	return TRUE;
}

BOOL CLocalNav::StepJumpable( Vector &vecSource, Vector &vecDest, int fNoMonsters, TraceResult &tr )
{
	Vector vecStepStart;
	Vector vecStepDest;
	BOOL fFwdTrace = FALSE;
	float flFwdFraction;
	float flJumpHeight = s_flStepSize + 1;
	BOOL fJumpClear = FALSE;
	edict_t *hit;

	vecStepStart = vecSource;
	vecStepStart.z += flJumpHeight;

	while ( flJumpHeight < 40 )
	{
		vecStepDest = vecDest;
		vecStepDest.z = vecStepStart.z;

		if ( !PathClear( vecStepStart, vecStepDest, fNoMonsters, tr ) )
		{
			if ( tr.fStartSolid )
				break;

			flFwdFraction = ( tr.vecEndPos - vecStepStart ).Length2D();

			if ( flFwdFraction < 1 )
			{
				flJumpHeight += 10;
				vecStepStart.z += 10;

				continue;
			}
		}

		fJumpClear = TRUE;
		break;
	}

	if ( !fJumpClear )
		return FALSE;

	vecStepStart = tr.vecEndPos;

	vecStepDest = vecStepStart;
	vecStepDest.z -= s_flStepSize;

	if ( !PathClear( vecStepStart, vecStepDest, fNoMonsters, tr ) )
	{
		if ( tr.fStartSolid )
			vecDest = vecStepStart;
		else
			vecDest = tr.vecEndPos;
	}
	else
		vecDest = tr.vecEndPos;

	return TRUE;
}

// nothing calls it (the Windows linker dropped it): written from the Linux
// library. Is one of six points 15 units into the wall on a ladder: 36
// above and below, and those two 16 units to either side?
BOOL CLocalNav::LadderHit( Vector &vecSource, Vector &vecDest, TraceResult &tr )
{
	Vector vecFwd;
	Vector vecRight;
	Vector vecUp;

	UTIL_MakeVectorsPrivate( UTIL_VecToAngles( -tr.vecPlaneNormal ), vecFwd, vecRight, vecUp );

	if ( UTIL_PointContents( tr.vecEndPos + ( vecFwd * 15 ) + ( vecUp * 36 ) ) == CONTENTS_LADDER )
		return TRUE;

	if ( UTIL_PointContents( tr.vecEndPos + ( vecFwd * 15 ) - ( vecUp * 36 ) ) == CONTENTS_LADDER )
		return TRUE;

	if ( UTIL_PointContents( tr.vecEndPos + ( vecFwd * 15 ) + ( vecRight * 16 ) + ( vecUp * 36 ) ) == CONTENTS_LADDER )
		return TRUE;

	if ( UTIL_PointContents( tr.vecEndPos + ( vecFwd * 15 ) - ( vecRight * 16 ) + ( vecUp * 36 ) ) == CONTENTS_LADDER )
		return TRUE;

	if ( UTIL_PointContents( tr.vecEndPos + ( vecFwd * 15 ) + ( vecRight * 16 ) - ( vecUp * 36 ) ) == CONTENTS_LADDER )
		return TRUE;

	if ( UTIL_PointContents( tr.vecEndPos + ( vecFwd * 15 ) - ( vecRight * 16 ) - ( vecUp * 36 ) ) == CONTENTS_LADDER )
		return TRUE;

	return FALSE;
}

void CLocalNav::Think( void )
{
	float flElapsedTime;
	EHANDLE hCallback;

	if ( gpGlobals->time >= flNextCvarCheck )
	{
		flNextCvarCheck = gpGlobals->time + 1.0;
		s_flStepSize = CVAR_GET_FLOAT( "sv_stepsize" );
	}

	flElapsedTime = gpGlobals->time - flLastThinkTime;
	flLastThinkTime = gpGlobals->time;

	HostagePrethink();

	nodeval -= flElapsedTime * 250;

	if ( nodeval < 0 )
		nodeval = 0;
	else if ( nodeval > 17 )
		return;

	if ( tot_inqueue )
	{
		hCallback = NULL;
		hCallback = queue[ qptr ];

		while ( hCallback == NULL )
		{
			if ( tot_inqueue <= 0 )
				break;

			if ( ++qptr == MAX_HOSTAGES_NAV )
				qptr = 0;

			if ( !--tot_inqueue )
			{
				hCallback = NULL;
				break;
			}

			hCallback = queue[ qptr ];
		}

		if ( hCallback != NULL )
		{
			CHostage *pHostage = GetClassPtr( (CHostage *)hCallback->pev );

			if ( ++qptr == MAX_HOSTAGES_NAV )
				qptr = 0;

			tot_inqueue--;

			pHostage->NavReady();
		}
	}
}

void CLocalNav::RequestNav( CHostage *pCaller )
{
	if ( nodeval <= 17 && !tot_inqueue )
	{
		pCaller->NavReady();
		return;
	}

	if ( tot_inqueue < MAX_HOSTAGES_NAV )
	{
		int curr = qptr;
		int found = 0;

		for ( int i = 0; i < tot_inqueue; i++ )
		{
			CHostage *pQueueItem = GetClassPtr( (CHostage *)queue[ curr ]->pev );

			if ( pQueueItem == pCaller )
			{
				found = 1;
				break;
			}

			if ( ++curr == MAX_HOSTAGES_NAV )
				curr = 0;
		}

		if ( !found )
		{
			queue[ curr ] = pCaller;
			tot_inqueue++;
		}
	}
	else
		CONSOLE_ECHO( "queue full\n" );
}

void CLocalNav::Reset( void )
{
	flNextCvarCheck = 0;
	flLastThinkTime = 0;

	tot_inqueue = 0;
	qptr = 0;
	nodeval = 0;
	tot_hostages = 0;
}

void CLocalNav::HostagePrethink( void )
{
	for ( int iCount = 0; iCount < tot_hostages; iCount++ )
	{
		if ( hostages[ iCount ] )
		{
			GetClassPtr( (CHostage *)hostages[ iCount ]->pev )->PreThink();
		}
	}
}
