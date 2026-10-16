/***
*
*	Wanted! code that retail defines in its own source files, which the
*	reconstruction hasn't written yet. Each item moves to its file when that
*	file is reconstructed; this file then goes away.
*
****/

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"teamplay_gamerules.h"
#include	"highnoon.h"
#include	"bot.h"

// ---- the Cow-Bots (bot.h): the bot job replaces these placeholders ----

CBotNav		g_BotNav;			// 0x100f38d8
float		g_flNavSaveTime;	// 0x100f38c8
float		g_flBotQueueTime;	// 0x100f38c4
CBotManager	g_BotManager;		// 0x100f3858

void CBotNav::RemoveNode( CBotNode *pNode ) { delete pNode; }	// 0x100166f0
void CBotNav::ClearGrid( void ) { memset( m_sGrid, 0, sizeof( m_sGrid ) ); }	// 0x100162a0
BOOL CBotNav::Load( const char *pszMapName ) { return FALSE; }	// 0x10015fc0
int CBotNav::GridIndex( float flCoord ) { return ( flCoord + 5120 ) * ( 1.0 / 64 ); }	// 0x10015ef0
void CBotNav::AddVisit( const Vector &vecOrigin ) {}	// 0x10016340
void CBotNav::Save( void ) { g_flNavSaveTime = gpGlobals->time + 300; }	// 0x10016420
void CBotManager::AdjustSkill( void ) {}	// 0x10010510
void CBot::BotThink( void ) {}	// 0x1000d210
BOOL BotCreate( edict_t *pCaller ) { return FALSE; }	// 0x10010b90
void BotRemove( int iIndex ) {}	// 0x10010cc0
