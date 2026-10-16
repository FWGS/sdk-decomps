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
// wantedrules.h: Wanted's multiplayer rules (deathmatch with Cow-Bots),
// the base of team deathmatch, Capture the Chicken and High Noon. The
// file and class names are ours. teamplay_gamerules.h includes it, since
// CHalfLifeTeamplay derives from CWantedMultiplay; that way the include
// lists of the SDK files (and so their precompiled headers) stay as they
// were.
//

#ifndef WANTEDRULES_H
#define WANTEDRULES_H

class CBotNode;

// info_wanteddetect's gametype
#define GAMETYPE_DEATHMATCH		0
#define GAMETYPE_TEAMPLAY		1
#define GAMETYPE_CTC			2	// Capture the Chicken
#define GAMETYPE_HIGHNOON		3

extern int g_iGameType;			// the map's info_wanteddetect gametype
extern BOOL g_fInitialRules;	// the rules the world installed, before info_wanteddetect's

// an entity that needs the game mode spawns again when info_wanteddetect activates
extern void WaitForGameRules( CBaseEntity *pEntity, CBaseEntity *pFirst );

//=========================================================
// CWantedMultiplay - deathmatch with Cow-Bots
//=========================================================
class CWantedMultiplay : public CHalfLifeMultiplay
{
public:
	CWantedMultiplay();

	virtual void Init( void );
	virtual int GetGameType( void ) { return GAMETYPE_DEATHMATCH; }
	virtual void RefreshSkillData( void );
	virtual void Think( void );
	virtual void InitHUD( CBasePlayer *pl );
	virtual void ClientDisconnected( edict_t *pClient );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual BOOL ClientCommand( CBasePlayer *pPlayer, const char *pcmd );
	virtual void EndMultiplayerGame( void );

protected:
	CBotNode *m_pDrawNode;		// "drawnodes": the node whose links are drawn next
	float m_flNextNodeDraw;
};

#endif // WANTEDRULES_H
