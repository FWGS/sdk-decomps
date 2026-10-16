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

  The Cow-Bots, as the game rules see them: the bot player class, the
  bot list's skill balancer, the navigation graph and the functions that
  add and remove bots. Only what the rules call is declared here; the
  bot code itself (0x1000cb70-0x10017bc6 in retail) is another job, and
  until it exists wanted_pending.cpp holds placeholders. Every name in
  this file is ours; the comments give the retail addresses.

*/

#ifndef BOT_H
#define BOT_H

// a navigation node (info_botnode / the .nav file)
class CBotNode
{
public:
	int			m_cLinks;		// +0x00 nodes in m_pLinks
	int			m_iUnknown[5];	// +0x04
	Vector		m_vecOrigin;	// +0x18
	CBotNode	*m_pNext;		// +0x24 the graph's list
	CBotNode	*m_pPrev;		// +0x28
	CBotNode	*m_pLinks[1];	// +0x2c the nodes this one leads to (m_cLinks of them)
};

#define BOTNAV_GRID		160		// cells per axis, 64 units each, from -5120

// the navigation graph: the node list, and a grid that counts the cells
// human players walk through (saved as wantedhl/maps/<map>.nav)
class CBotNav
{
public:
	void		RemoveNode( CBotNode *pNode );			// 0x100166f0: unlinks and deletes it
	void		ClearGrid( void );						// 0x100162a0
	BOOL		Load( const char *pszMapName );			// 0x10015fc0: reads wantedhl/maps/<map>.nav
	int			GridIndex( float flCoord );				// 0x10015ef0: ( flCoord + 5120 ) / 64
	void		AddVisit( const Vector &vecOrigin );	// 0x10016340: counts the cell
	void		Save( void );							// 0x10016420: writes the .nav file, sets g_flNavSaveTime

	int			m_iUnknown;		// +0x00
	CBotNode	*m_pHead;		// +0x04
	CBotNode	*m_pTail;		// +0x08
	int			m_cNodes;		// +0x0c
	short		m_sGrid[BOTNAV_GRID][BOTNAV_GRID][BOTNAV_GRID];	// +0x10
};

extern CBotNav	g_BotNav;			// 0x100f38d8
extern float	g_flNavSaveTime;	// 0x100f38c8: next time the .nav file is saved
extern float	g_flBotQueueTime;	// 0x100f38c4: next time a bot may be added or removed

// the bots' shared state: sv_adjbotskill's balancing, the last profile used
class CBotManager
{
public:
	void		AdjustSkill( void );	// 0x10010510: moves sv_botskill by the bots' and humans' scores

	float		m_flNextAdjust;			// +0x00
	float		m_flUnknown;			// +0x04
	char		m_szLastProfile[80];	// +0x08 (size unknown)
};

extern CBotManager g_BotManager;	// 0x100f3858

// the bot: a fake client's player (vtable 0x100bb0d8)
class CBot : public CBasePlayer
{
public:
	void		BotThink( void );		// 0x1000d210: runs the bot for a server frame

	int			m_iUnknown[ ( 0x6cc - 0x5e4 ) / 4 ];	// +0x5e4: the bot job's fields
	CBaseEntity	*m_pCamera;				// +0x6cc: a dynamite.mdl entity that follows the bot ("specon" looks through it)
};

extern BOOL BotCreate( edict_t *pCaller );	// 0x10010b90: adds a bot; pCaller gets the error messages
extern void BotRemove( int iIndex );		// 0x10010cc0: kicks player iIndex if it is a bot

#endif // BOT_H
