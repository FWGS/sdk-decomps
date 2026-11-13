/***
*
*	Counter-Strike 1.4, hostage_localnav.h: the hostages' local navigation
*	(CLocalNav).
*
*	Every hostage owns a CLocalNav. It searches a grid of nodes
*	HOSTAGE_STEPSIZE apart (at most MAX_NODES, kept in m_nodeArr) for a way
*	from the hostage to a point near its leader, testing each step with the
*	hostage's hull (slopes, steps, steps to jump). The searches of all
*	hostages are rationed: a hostage asks with RequestNav and is called back
*	(CHostage::NavReady) at once or from the queue Think works through.
*
*	The file's name, the class, its members, the static members, the
*	methods and their parameters, node_index_t and localnode_t with its
*	fields are Counter-Strike 1.6's (debug information of its Linux
*	library; the declarations stand on its lines where the text allowed);
*	the methods and their parameter types are also the 1.4 Linux library's
*	symbols. The macros' names are ours.
*
****/
#ifndef HOSTAGE_LOCALNAV_H
#define HOSTAGE_LOCALNAV_H

#define HOSTAGE_STEPSIZE		26.0	// names ours
#define MAX_HOSTAGES_NAV		20

#define NODE_INVALID_EMPTY		-1

// the absolute value as CLocalNav takes it: the argument is evaluated twice and multiplied by its sign as
// an int (AddPathNode, GetBestNode; the Linux compiler folds it to a negation)
#define SIGNOF( x )				( ( x ) < 0 ? -1 : 1 )
#define ABSOF( x )				( SIGNOF( x ) * ( x ) )

#define PATH_TRAVERSABLE_EMPTY	0
#define PATH_TRAVERSABLE_SLOPE	1
#define PATH_TRAVERSABLE_STEP	2
#define PATH_TRAVERSABLE_STEPJUMPABLE	3

typedef int node_index_t;


typedef struct localnode_s
{
	Vector vecLoc;
	int offsetX;
	int offsetY;
	byte bDepth;
	BOOL fSearched;
	node_index_t nindexParent;

}
localnode_t;

class CHostage;
class CLocalNav
{
public:
	static float s_flStepSize;

	CLocalNav( CHostage *pOwner );
	virtual ~CLocalNav( void );



	void SetTargetEnt( CBaseEntity *pTarget )
	{
		if ( pTarget )
			m_pTargetEnt = pTarget->edict();
		else
			m_pTargetEnt = NULL;
	}





	node_index_t FindPath( Vector &vecStart, Vector &vecDest, float flTargetRadius, int fNoMonsters );
	int SetupPathNodes( node_index_t nindex, Vector *vecNodes, int fNoMonsters );
	int GetFurthestTraversableNode( Vector &vecStartingLoc, Vector *vecNodes, int nTotalNodes, int fNoMonsters );

	int PathTraversable( Vector &vecSource, Vector &vecDest, int fNoMonsters );
	BOOL PathClear( Vector &vecOrigin, Vector &vecDest, int fNoMonsters, TraceResult &tr );
	BOOL PathClear( Vector &vecSource, Vector &vecDest, int fNoMonsters )
	{
		TraceResult tr;

		return PathClear( vecSource, vecDest, fNoMonsters, tr );
	}

	static void Think( void );

	void RequestNav( CHostage *pCaller );
	static void Reset( void );

private:
	static void HostagePrethink( void );

	CHostage *m_pOwner;
	edict_t *m_pTargetEnt;
	BOOL m_fTargetEntHit;
	localnode_t *m_nodeArr;
	node_index_t m_nindexAvailableNode;
	Vector m_vecStartingLoc;

	node_index_t AddNode( node_index_t nindexParent, Vector &vecLoc, int offsetX = 0, int offsetY = 0, byte bDepth = 0 );
	localnode_t *GetNode( node_index_t nindex );
	node_index_t NodeExists( int offsetX, int offsetY );
	void AddPathNodes( node_index_t nindexSource, int fNoMonsters );
	void AddPathNode( node_index_t nindexSource, int offsetX, int offsetY, int fNoMonsters );
	node_index_t GetBestNode( Vector &vecOrigin, Vector &vecDest );
	BOOL SlopeTraversable( Vector &vecSource, Vector &vecDest, int fNoMonsters, TraceResult &tr );
	BOOL LadderTraversable( Vector &vecSource, Vector &vecDest, int fNoMonsters, TraceResult &tr );
	BOOL StepTraversable( Vector &vecSource, Vector &vecDest, int fNoMonsters, TraceResult &tr );
	BOOL StepJumpable( Vector &vecSource, Vector &vecDest, int fNoMonsters, TraceResult &tr );
	node_index_t FindDirectPath( Vector &vecStart, Vector &vecDest, float flTargetRadius, int fNoMonsters );
	BOOL LadderHit( Vector &vecSource, Vector &vecDest, TraceResult &tr );

	static EHANDLE queue[ MAX_HOSTAGES_NAV ];
	static int qptr;
	static int tot_inqueue;
	static float nodeval;
	static float flNextCvarCheck;
	static float flLastThinkTime;

	static EHANDLE hostages[ MAX_HOSTAGES_NAV ];
	static int tot_hostages;
};

// hostage_localnav.cpp (the 1.4 Linux library has the three in that object, ahead of CLocalNav's functions)
extern void CONSOLE_ECHO( char *pszMsg, ... );
extern void CONSOLE_ECHO_LOGGED( char *pszMsg, ... );
extern void UTIL_DrawBeamPoints( Vector vecStart, Vector vecEnd, int iLifetime, byte bRed, byte bGreen, byte bBlue );

#endif // HOSTAGE_LOCALNAV_H
