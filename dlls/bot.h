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

===== bot.h ========================================================

  The Cow-Bots: Wanted's server-side bots. A bot is a fake client
  (pfnCreateFakeClient) whose player entity is a CBot; the deathmatch
  rules run every bot's BotThink each server frame, and BotThink ends
  with pfnRunPlayerMove. Bots read a profile from wantedhl/bots/*.bot and
  navigate by info_botnode / info_botlandmark entities and a grid of
  visit counts kept in wantedhl/maps/<map>.nav.

  Every name in this file is ours; retail has no symbols for the bot
  code. Offsets are retail's.

*/
#ifndef BOT_H
#define BOT_H

class CBot;

// a bot's name and model when it has no profile file (the table in
// botmove.cpp)
typedef struct
{
	char	*pszName;
	char	*pszModel;
	int		iUnused;
} botname_t;

// one step of a route from the node graph (0x14 bytes)
class CBotRoute
{
public:
	int			m_iAction;		// +0x00 the node's action: 0 walk, 1 jump, 2 duck, 3 ladder
	Vector		m_vecOrigin;	// +0x04
	float		m_flRadius;		// +0x10 how close counts as there
};

#define MAX_BOTNODE_LINKS	16

// a navigation node: an info_botnode, kept in CBotNav's list (0x70 bytes)
class CBotNode
{
public:
	int			m_cLinks;		// +0x00 nodes in m_pLinks
	int			m_iGroup;		// +0x04
	int			m_iGroupTrans;	// +0x08
	int			m_iAction;		// +0x0c
	int			m_fRandomLinks;	// +0x10
	int			m_fFlags;		// +0x14 spawnflags & 1
	Vector		m_vecOrigin;	// +0x18
	CBotNode	*m_pNext;		// +0x24 the graph's list
	CBotNode	*m_pPrev;		// +0x28
	CBotNode	*m_pLinks[MAX_BOTNODE_LINKS];	// +0x2c the nodes this one leads to (m_cLinks of them)
	int			m_fVisited;		// +0x6c on the path being searched
};

#define MAX_LANDMARK_LINKS	3

// an info_botlandmark, kept in CBotNav's landmark list (0x24 bytes)
class CBotLandmark
{
public:
	string_t	m_iszName;		// +0x00 targetname
	Vector		m_vecOrigin;	// +0x04
	CBotLandmark *m_pLinks[MAX_LANDMARK_LINKS];	// +0x10
	CBotLandmark *m_pNext;		// +0x1c
	int			m_fVisited;		// +0x20
};

// one step of the path FindPath is building (0x14 bytes)
class CBotPathStep
{
public:
	Vector		m_vecOrigin;	// +0x00
	int			m_iAction;		// +0x0c
	CBotPathStep *m_pNext;		// +0x10
};

// info_botnode: a navigation node; it adds itself to g_BotNavigator and is
// removed (0x30 bytes)
class CInfoBotnode : public CBaseEntity
{
public:
	void	Spawn( void );
	void	KeyValue( KeyValueData *pkvd );

	int		m_iGroup;		// nodes link only within a group, or to their grouptrans group
	int		m_iGroupTrans;
	int		m_iAction;		// 0 walk, 1 jump, 2 duck, 3 ladder
	int		m_fRandomLinks;	// the search tries a random link first
};

#define BOTNAV_GRID		160		// cells per axis, 64 units each, from -5120
#define BOTNAV_CELL		64

// the navigation graph: the node list, the landmark list, and a grid that
// counts the cells human players walk through (saved as
// wantedhl/maps/<map>.nav); one global, g_BotNavigator
class CBotNav
{
public:
	CBotNav() { m_pHead = NULL; m_pTail = NULL; m_pLandmarks = NULL; }

	int			m_iUnused;		// +0x00
	CBotNode	*m_pHead;		// +0x04
	CBotNode	*m_pTail;		// +0x08
	CBotLandmark *m_pLandmarks;	// +0x0c
	short		m_sGrid[BOTNAV_GRID][BOTNAV_GRID][BOTNAV_GRID];	// +0x10
	int			m_iDepth;			// +0x7d0010 steps on the path being searched
	int			m_iMaxDepth;		// +0x7d0014 48, then the shortest path found + 1
	BOOL		m_fRandomLinks;		// +0x7d0018 honour the nodes' randomlinks
	BOOL		m_fUnused;			// +0x7d001c set with it, never read
	CBotPathStep *m_pPathHead;		// +0x7d0020
	CBotPathStep *m_pPathTail;		// +0x7d0024
	CBotRoute	*m_pRoute;			// +0x7d0028 the route found (malloc'd)

	float	IndexToCoord( int i );
	Vector	IndexToPos( const Vector &vecIndex );
	int		GridIndex( float fl );
	Vector	PosToIndex( const Vector &vecPos );
	short	GetVisits( int x, int y, int z );
	BOOL	Load( const char *pszMapName );
	BOOL	ReadNavFile( short *pGrid, char *pszFile );
	void	ParseLine( short *pGrid, char *pszLine );
	void	HalveVisits( void );
	void	ClearGrid( void );
	BOOL	InGrid( const Vector &vec );
	void	AddVisit( const Vector &vec );
	void	AddVisit2( const Vector &vec );
	void	Save( void );
	CBotNode *AddNode( CInfoBotnode *pEntity, int fNoBacklinks );
	void	LinkNodes( CBotNode *pFrom, CBotNode *pTo, int fNoBacklink );
	void	RemoveNode( CBotNode *pNode );
	CBotNode *FindVisibleNode( CBaseEntity *pLooker, CBotNode *pAfter );
	CBotNode *FindPath( CBaseEntity *pGoal, CBotNode *pNode );
	CBotRoute *BuildRoute( CBot *pBot, CBaseEntity *pGoal );
};

extern CBotNav g_BotNavigator;

// the bot's weapon handling (CBot +0x6d0, 0x14 bytes)
class CBotFire
{
public:
	CBotFire( CBot *pBot );
	~CBotFire();

	void	Update( CBasePlayerItem *pItem );
	void	RollAimMode( int iChance );
	void	RollSecondary( int iChance );
	void	SetAimMode( int iMode );	// Update's steps (names ours), botattack.cpp
	void	AimBow( void );

	int		m_iAimMode;		// 0 or 1 by the roll; 2 for lobbed weapons (AimAtEnemy)
	BOOL	m_fKeepFiring;	// hold the trigger down
	BOOL	m_fSecondary;	// use the secondary attack
	int		m_iClip;		// set from a non-weapon_ item's m_iClip only
	CBot	*m_pBot;
};

// what the bot remembers of its enemy and its own movement (CBot +0x6e4,
// 0xbc bytes): four frames of history, newest first
class CBotMemory
{
public:
	CBotMemory( CBot *pBot );
	~CBotMemory();

	void	Forget( void );
	void	SawEnemy( CBaseEntity *pEnemy );
	void	LostEnemy( void );
	void	SetEnemy( CBaseEntity *pEnemy, int fChickenCarrier );
	void	Update( CBot *pBot );
	float	SeenTime( void ) { return m_flSeenTime; }
	Vector	GetOrigin( void ) { return m_vecOrigin; }	// name ours
	Vector	GetLookDir( void );		// Update's accessors (names ours), botattack.cpp
	Vector	GetLookDir1( void );
	Vector	GetLookDir2( void );
	BOOL	Seen( void );			// RouteGoal's accessors (names ours), botnav.cpp
	BOOL	ChickenCarrier( void );

	float	m_flTime;
	BOOL	m_fSeen;
	BOOL	m_fChickenCarrier;	// the enemy carries the CTC chicken
	Vector	m_vecEnemyOrigin;
	float	m_flSeenTime;
	Vector	m_vecEnemyVelocity;
	Vector	m_vecLookDir;
	Vector	m_vecVelocity;
	Vector	m_vecEnemyVelocity1;
	Vector	m_vecLookDir1;
	Vector	m_vecVelocity1;
	Vector	m_vecEnemyVelocity2;
	Vector	m_vecLookDir2;
	Vector	m_vecVelocity2;
	Vector	m_vecEnemyVelocity3;
	Vector	m_vecLookDir3;
	Vector	m_vecVelocity3;
	Vector	m_vecOrigin;
	CBot	*m_pBot;
};

// a bot's profile (CBot +0x7a0, 0xa0 bytes), read from wantedhl/bots/*.bot
class CBotProfile
{
public:
	CBotProfile( CBot *pBot );
	~CBotProfile();

	float	SkillAdjust( float flSkill );
	float	AimSkill( void ) { return SkillAdjust( m_flAim ); }
	float	NoticeSkill( void ) { return SkillAdjust( m_flNotice ); }
	float	JumpSkill( void );
	float	RateWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pItem, float flDist );
	BOOL	Load( char *pszFile );
	BOOL	ParseFile( class ifstream &file );	// Load's loop (name ours), botnav.cpp
	float	Aggression( void ) { return m_flAggression; }

	char	m_szName[64];
	char	m_szModel[64];
	char	m_szTopColor[4];
	char	m_szBottomColor[4];
	float	m_flAim;		// +0x88
	float	m_flAggression;	// +0x8c
	float	m_flJump;		// +0x90
	float	m_flNotice;		// +0x94
	float	m_flPause;		// +0x98
	CBot	*m_pBot;
};

// the bot list's state (one global, g_BotManager)
class CBotManager
{
public:
	CBotManager();
	~CBotManager();

	void	AdjustSkill( void );

	float	m_flNextAdjust;
	float	m_flSkillScale;
	char	m_szLastProfile[100];
};

extern CBotManager g_BotManager;

class CBot : public CBasePlayer
{
public:
	CBot();
	~CBot();

	// bot.cpp
	void	Spawn( void );

	void	PressFire( void );
	void	ChooseWeapon( void );
	BOOL	CheckReload( void );
	void	Say( char *pszText );
	void	BotThink( void );
	void	BeginFrame( void );
	void	LookAround( int iRange );
	float	StrafeInterval( void ) { return m_flStrafeInterval; }
	BOOL	IsValidEnemy( CBasePlayer *pPlayer );
	float	NextEnemyCheck( void ) { return m_flNextEnemyCheck; }
	float	GiveUpItemTime( void ) { return m_flGiveUpItemTime; }	// CheckGoalItem's (name ours)
	void	PickGoalItem( void );
	BOOL	HasGun( void );
	BOOL	CanNotice( CBaseEntity *pEntity );
	BOOL	CanSee( CBaseEntity *pEntity );
	float NextFrameCount( void ) { return m_flNextFrameCount; }
	void	CountFrames( void );
	void	DeadThink( void );
	void	CheckMelee( void );
	void	FightThink( void );
	void	CheckEnemy( void );
	void	CheckGoalItem( void );
	BOOL	CanWalkTo( const Vector &vecDest );
	void SetGoalItem( CBaseEntity *pItem )
	{
		if ( pItem )
		{
			m_iGoalItem = ENTINDEX( pItem->edict() );
			m_iGoalItemSerial = pItem->edict()->serialnumber;
		}
		else
			m_iGoalItem = 0;
	}

	BOOL HasEnemy( void )
	{
		edict_t *pent;
		return m_iEnemy && ( pent = INDEXENT( m_iEnemy ) ) != NULL && pent->serialnumber == m_iEnemySerial && !pent->free && pent->pvPrivateData;
	}
	CBaseEntity *GetEnemy( void )
	{
		if ( m_iEnemy )
		{
			edict_t *pent = INDEXENT( m_iEnemy );
			if ( pent && pent->serialnumber == m_iEnemySerial && !pent->free )
				return (CBaseEntity *)pent->pvPrivateData;
		}
		return NULL;
	}
	BOOL HasGoalItem( void )
	{
		edict_t *pent;
		return m_iGoalItem && ( pent = INDEXENT( m_iGoalItem ) ) != NULL && pent->serialnumber == m_iGoalItemSerial && pent->pvPrivateData;
	}
	CBaseEntity *GetGoalItem( void )
	{
		if ( m_iGoalItem )
		{
			edict_t *pent = INDEXENT( m_iGoalItem );
			if ( pent && pent->serialnumber == m_iGoalItemSerial )
				return (CBaseEntity *)pent->pvPrivateData;
		}
		return NULL;
	}

	// the CBaseEntity overrides come after the accessors: retail emits their
	// out-of-line copies after GetGoalItem's, at the end of bot.obj
	int		Classify( void ) { return CLASS_PLAYER; }
	BOOL	IsPlayer( void ) { return TRUE; }
	BOOL	IsNetClient( void ) { return FALSE; }

	// botmove.cpp
	BOOL	CanSeeSpot( const Vector &vecSpot );
	BOOL	FVisible( CBaseEntity *pEntity );
	BOOL	FVisible( const Vector &vecOrigin );
	void	AimJitter( void );
	void	AimAtEnemy( void );
	void	AimAt( CBaseEntity *pEntity );
	void	LookAlong( const Vector &vecDir );	// the look direction's accessors (names ours), botmove.cpp
	int		GetLadderDir( void );
	Vector	GetMoveVelocity( void ) { return m_vecMoveVelocity; }
	// BotThink's accessors (names ours)
	Vector	GetGoalPos( void ) { return m_vecGoalPos; }
	float	GetForwardMove( void ) { return m_flForwardMove; }
	float	GetSideMove( void ) { return m_flSideMove; }
	float	GetUpMove( void ) { return m_flUpMove; }
	void	SetMoveVelocity( const Vector &vec ) { m_vecMoveVelocity = vec; }
	int		GetMovePriority( void ) { return m_iMovePriority; }
	void	SetMovePriority( int iPriority ) { m_iMovePriority = iPriority; }
	int		GetMoveGoals( void ) { return m_cMoveGoals; }
	void	SetMoveGoals( int cGoals ) { m_cMoveGoals = cGoals; }
	void	SetLookDir( void );
	Vector	GetLookDir( void );		// CBotMemory::Update's (name ours), botattack.cpp
	int		GetState( void );		// RouteGoal's accessors (names ours), botnav.cpp
	BOOL	RouteDone( void );
	BOOL	CheckAhead( Vector &vecDir, TraceResult *ptr );
	BOOL	CheckSides( Vector &vecAngles, TraceResult *ptrLeft, TraceResult *ptrRight );
	void	MoveThink( void );
	Vector	AvoidWalls( Vector &vecDir );
	void	Unstick( void );
	void	FlockAlign( int iPriority );
	// FlockAlign's step (name ours): the mean velocity of the living players
	// in sight within 128 units
	void AlignVelocity( void )
	{
		int cPlayers = 0;
		Vector vecSum = Vector( 0, 0, 0 );
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
			if ( pPlayer && pPlayer != this && pPlayer->IsAlive() && CanSee( pPlayer ) )
			{
				if ( ( pev->origin - pPlayer->pev->origin ).Length() < 128 )
				{
					cPlayers++;
					vecSum = vecSum + pPlayer->pev->velocity;
				}
			}
		}
		Vector vecNew = vecSum / cPlayers;
		m_vecMoveVelocity = vecNew;
	}
	void	FlockCohere( int iPriority );
	void	MoveTowards( const Vector &vecDest );
	// FlockCohere's step (name ours): head for the centre of the living
	// players in sight within 128 units
	void CohereVelocity( void )
	{
		int cPlayers = 0;
		Vector vecSum = Vector( 0, 0, 0 );
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
			if ( pPlayer && pPlayer != this && pPlayer->IsAlive() && CanSee( pPlayer ) )
			{
				if ( ( pev->origin - pPlayer->pev->origin ).Length() < 128 )
				{
					cPlayers++;
					vecSum = vecSum + pPlayer->pev->origin;
				}
			}
		}
		Vector vecCentre = vecSum / cPlayers;
		MoveTowards( vecCentre );
	}
	void	ApproachEntity( CBaseEntity *pEntity, int iPriority );
	void	MoveToLead( CBaseEntity *pEntity, Vector vecOffset );
	// the move goals' common step (name ours): head for vecPos at full speed
	void	HeadFor( const Vector &vecPos ) { m_vecMoveVelocity = ( vecPos - pev->origin ).Normalize() * CVAR_GET_FLOAT( "sv_maxspeed" ); }
	void	Flock( int iPriority );
	void	FollowEntity( CBaseEntity *pEntity, int iPriority );
	// ApproachEntity's and FollowEntity's step (name ours): head for the
	// spot 80 units short of where pEntity will be
	void FollowLead( CBaseEntity *pEntity )
	{
		Vector vecOffset = ( pev->origin - pEntity->pev->origin ).Normalize() * 80;
		MoveToLead( pEntity, vecOffset );
	}
	void	FlockSeparate( int iPriority );
	// FlockSeparate's step (name ours): keep away from the living players in
	// sight within 128 units, the nearer the more; the sum of directions is
	// passed to MoveTowards as a spot (sic)
	void SeparateVelocity( void )
	{
		Vector vecSum = Vector( 0, 0, 0 );
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
			if ( pPlayer && pPlayer != this && pPlayer->IsAlive() && CanSee( pPlayer ) )
			{
				Vector vecAway;
				vecAway = pev->origin - pPlayer->pev->origin;
				float flDist = vecAway.Length();
				if ( flDist < 128 )
				{
					vecAway = vecAway.Normalize();
					vecSum = vecSum + vecAway / ( flDist * flDist );
				}
			}
		}
		MoveTowards( vecSum );	// sic
	}
	void	MoveToEntity( CBaseEntity *pEntity, int iPriority );					// 0x10013750
	// MoveToEntity's step (name ours): run from where pEntity will be when
	// the bot gets there
	void FleeLead( CBaseEntity *pEntity )
	{
		float flDist = ( pEntity->pev->origin - pev->origin ).Length();
		MoveAwayFrom( pEntity->pev->velocity * flDist + pEntity->pev->origin );
	}
	void	MoveAwayFrom( const Vector &vecSpot );
	void	FleeEntity( CBaseEntity *pEntity, int iPriority );
	// FleeEntity's and FleeSpot's step (names ours): run from vecSpot (or
	// pEntity) at full speed
	void RunFrom( const Vector &vecSpot ) { m_vecMoveVelocity = ( vecSpot - pev->origin ).Normalize() * -1 * CVAR_GET_FLOAT( "sv_maxspeed" ); }
	void FleeFromEntity( CBaseEntity *pEntity ) { RunFrom( pEntity->pev->origin ); }
	void	FleeSpot( const Vector &vecSpot, int iPriority );
	void	UnstickGoal( int iPriority );
	void	MoveToEntityOffset( CBaseEntity *pEntity, int iPriority, Vector vecOffset );	// 0x10013f30
	void	MoveToItem( CBaseEntity *pItem, int iPriority );					// 0x100140a0
	// MoveToItem's step (name ours): head for pItem's centre at full speed
	void HeadForItem( CBaseEntity *pItem ) { HeadFor( pItem->Center() ); }
	void	MoveToSpot( const Vector &vecSpot, int iPriority );
	void	Wobble( float flPercent, int iPriority );
	// Wobble's step (name ours): a random push of up to flPercent per cent of
	// the move direction, at full speed
	void WobbleVelocity( float flPercent )
	{
		flPercent = flPercent / 100;
		Vector vecRandom = Vector( RANDOM_FLOAT( -flPercent, flPercent ), RANDOM_FLOAT( -flPercent, flPercent ), RANDOM_FLOAT( -flPercent, flPercent ) );
		SetMoveVelocity( GetMoveVelocity().Normalize() + vecRandom );
		SetMoveVelocity( GetMoveVelocity().Normalize() * CVAR_GET_FLOAT( "sv_maxspeed" ) );
	}
	void	Roam( int fForce );												// 0x10014720

	// Roam's step and its accessors (names ours; defined in botmove.cpp,
	// before Roam)
	void	SeekGoal( void );
	void	SetGoalVisible( BOOL fVisible );
	// GetState: declared with RouteGoal's accessors (botnav.cpp defines it too)
	void	SetPauseTime( float flTime );

	// botnav.cpp
	void	ClearRoute( void );
	void	NextRouteNode( void );
	Vector	RouteGoal( int iMode );
	Vector	Wander( void );

	BOOL		m_fLookDirSet;		// +0x5e4 m_vecLookDir was set this frame (aiming wins over SetLookDir)
	Vector		m_vecMoveVelocity;	// +0x5e8 the move goals' wanted velocity
	Vector		m_vecGoalPos;		// +0x5f4
	Vector		m_vecUnused;		// +0x600
	Vector		m_vecMoveAngles;	// +0x60c
	BOOL		m_fDuckToggle;		// +0x618
	Vector		m_vecAimError;		// +0x61c
	Vector		m_vecLookDir;		// +0x628 the direction the bot looks and aims in
	float		m_flLastMoveTime;	// +0x634
	float		m_flGoalDist;		// +0x638
	float		m_flGoalTime;		// +0x63c
	float		m_flMoved;			// +0x640
	float		m_flForwardMove;	// +0x644
	float		m_flSideMove;		// +0x648
	float		m_flUpMove;			// +0x64c
	float		m_flMsec;			// +0x650
	int			m_cFrames;			// +0x654
	int			m_iEnemy;			// +0x658
	int			m_iEnemySerial;		// +0x65c
	int			m_cRouteSteps;		// +0x660
	int			m_cRouteLeft;		// +0x664
	CBotRoute	*m_pRoute;			// +0x668
	CBotRoute	*m_pRouteStart;		// +0x66c
	int			m_iGoalItem;		// +0x670
	int			m_iGoalItemSerial;	// +0x674
	int			m_iLadderDir;		// +0x678
	int			m_cMoveGoals;		// +0x67c
	int			m_iMovePriority;	// +0x680
	int			m_iState;			// +0x684 3: going after the enemy's last position
	float		m_flNextStrafe;		// +0x688
	float		m_flStrafeInterval;	// +0x68c
	float		m_flNextFrameCount;	// +0x690
	float		m_flNextEnemyCheck;	// +0x694
	float		m_flNextItemCheck;	// +0x698
	float		m_flGiveUpItemTime;	// +0x69c
	float		m_flPauseTime;		// +0x6a0
	float		m_flNextStuckCheck;	// +0x6a4
	float		m_flItemRadius;		// +0x6a8
	int			m_iDodgeDir;		// +0x6ac
	BOOL		m_fMelee;			// +0x6b0
	BOOL		m_fHasGoalPos;		// +0x6b4
	BOOL		m_fBlocked;			// +0x6b8
	BOOL		m_fGoalVisible;		// +0x6bc
	BOOL		m_fRouteDone;		// +0x6c0
	BOOL		m_fDuck;			// +0x6c4
	BOOL		m_fJump;			// +0x6c8
	CBaseEntity	*m_pCamera;			// +0x6cc a dynamite.mdl entity that follows the bot's view ("specon" looks through it)
	CBotFire	m_Fire;				// +0x6d0
	CBotMemory	m_Memory;			// +0x6e4
	CBotProfile	m_Profile;			// +0x7a0
};

// botmove.cpp
extern botname_t g_rgBotNames[];
botname_t *BotPickName( void );
BOOL BotFindProfile( void );
void BotInit( edict_t *pEdict, botname_t *pName );
BOOL BotCreate( edict_t *pCaller );
void BotRemove( int iIndex );

extern cvar_t sv_adjbotskill;
extern cvar_t sv_botskill;

// botnav.cpp
extern float g_flBotQueueTime;		// 0x100f38c4: next time the rules may add or remove a bot
extern float g_flNavSaveTime;		// 0x100f38c8: next time the .nav file is saved

#endif // BOT_H
