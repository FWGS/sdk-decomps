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

// ---- Capture the Chicken: the ctc file's ----

char *g_pszTeamNames[2] = { "Outlaws", "Rustlers" };	// 0x100de994
void ShowPlayerMenu( CBasePlayer *pPlayer, char *pszText, int bitsValidSlots, int nDisplayTime, BOOL fNeedMore ) {}	// 0x1002e220
void CCaptureTheChicken::Init( void ) { CWantedMultiplay::Init(); }	// 0x1002e3d0
void CCaptureTheChicken::Think( void ) { CHalfLifeTeamplay::Think(); }	// 0x1002ebb0
BOOL CCaptureTheChicken::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon ) { return CHalfLifeTeamplay::FShouldSwitchWeapon( pPlayer, pWeapon ); }	// 0x1002eee0
void CCaptureTheChicken::InitHUD( CBasePlayer *pl ) { CHalfLifeTeamplay::InitHUD( pl ); }	// 0x1002e4a0
void CCaptureTheChicken::PlayerSpawn( CBasePlayer *pPlayer ) { CHalfLifeTeamplay::PlayerSpawn( pPlayer ); }	// 0x1002e460
BOOL CCaptureTheChicken::ClientCommand( CBasePlayer *pPlayer, const char *pcmd ) { return CHalfLifeTeamplay::ClientCommand( pPlayer, pcmd ); }	// 0x1002e700
void CCaptureTheChicken::ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer ) {}	// 0x1002e660
void CCaptureTheChicken::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor ) { CHalfLifeTeamplay::PlayerKilled( pVictim, pKiller, pInflictor ); }	// 0x1002ef50
BOOL CCaptureTheChicken::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon ) { return CHalfLifeTeamplay::CanHavePlayerItem( pPlayer, pWeapon ); }	// 0x1002ed20
void CCaptureTheChicken::PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon ) {}	// 0x1002eda0
int CCaptureTheChicken::DeadPlayerWeapons( CBasePlayer *pPlayer ) { return CHalfLifeTeamplay::DeadPlayerWeapons( pPlayer ); }	// 0x1002efb0
const char *CCaptureTheChicken::SetDefaultPlayerTeam( CBasePlayer *pPlayer ) { return CHalfLifeTeamplay::SetDefaultPlayerTeam( pPlayer ); }	// 0x1002e980
