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
// teamplay_gamerules.h
//

#define MAX_TEAMNAME_LENGTH	16
#define MAX_TEAMS			32

#define TEAMPLAY_TEAMLISTLENGTH		MAX_TEAMS*MAX_TEAMNAME_LENGTH

#include "wantedrules.h"

class CHalfLifeTeamplay : public CWantedMultiplay
{
public:
	CHalfLifeTeamplay();

	virtual BOOL ClientCommand( CBasePlayer *pPlayer, const char *pcmd );
	virtual void ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer );
	virtual BOOL IsTeamplay( void );
	virtual BOOL FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker );
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );
	virtual const char *GetTeamID( CBaseEntity *pEntity );
	virtual BOOL ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target );
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void InitHUD( CBasePlayer *pl );
	virtual void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor );
	virtual const char *GetGameDescription( void ) { return "HL Teamplay"; }  // this is the game name that gets seen in the server browser
	virtual void UpdateGameMode( CBasePlayer *pPlayer );  // the client needs to be informed of the current game mode
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual void Think ( void );
	virtual int GetTeamIndex( const char *pTeamName );
	virtual const char *GetIndexedTeamName( int teamIndex );
	virtual BOOL IsValidTeam( const char *pTeamName );
	const char *SetDefaultPlayerTeam( CBasePlayer *pPlayer );
	virtual void ChangePlayerTeam( CBasePlayer *pPlayer, const char *pTeamName, BOOL bKill, BOOL bGib );

private:
	void RecountTeams( void );
	const char *TeamWithFewestPlayers( void );

	BOOL m_DisableDeathMessages;
	BOOL m_DisableDeathPenalty;
	BOOL m_teamLimit;				// This means the server set only some teams as valid
	char m_szTeamList[TEAMPLAY_TEAMLISTLENGTH];
};

//=========================================================
// CHalfLifeCTC - Capture the Chicken: two teams, Outlaws and
// Rustlers, carry the other team's chicken to their capture point.
// Class name ours: not CCaptureTheChicken, whose Think would put the
// SDK's last_frags before last_time in .bss (VC6 orders a file's
// uninitialised data by a hash of the decorated names; retail has
// last_time first, as CHalfLifeTeamplay's own copy has).
//=========================================================
class CHalfLifeCTC : public CHalfLifeTeamplay
{
public:
	virtual void Init( void );
	virtual void Think( void );
	virtual BOOL FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual const char *GetGameDescription( void ) { return "Wanted! CTC"; }
	virtual int GetGameType( void ) { return GAMETYPE_CTC; }
	virtual BOOL IsTeamplay( void ) { return TRUE; }
	virtual void InitHUD( CBasePlayer *pl );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual BOOL ClientCommand( CBasePlayer *pPlayer, const char *pcmd );
	virtual void ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer );
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual BOOL CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual void PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer );
	virtual const char *SetDefaultPlayerTeam( CBasePlayer *pPlayer );

	void UpdateTeamScore( CBasePlayer *pPlayer, const char *pszChickenTeam );

	BOOL m_fAutoTeam;		// the player asked for the team with fewer players
	int m_iTeamScore[2];
	int m_iTeamDeaths[2];
};

extern char *g_pszTeamNames[2];		// "Outlaws", "Rustlers"
extern char *g_pszTeamModels[2];	// their player models: "rogan", "twnwest"
extern float g_flChickenAway[2];	// per team: when the dropped chicken goes home; -1 carried, 0 home
extern float g_flChickenQuietTime;
extern CBaseEntity *g_pWaitingChickens;
extern void ShowPlayerMenu( CBasePlayer *pPlayer, char *pszText, int bitsValidSlots, int nDisplayTime, BOOL fNeedMore );
