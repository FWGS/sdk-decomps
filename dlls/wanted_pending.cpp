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

// ---- Wanted's deathmatch rules: the wantedrules file's ----

BOOL g_fInitialRules = TRUE;
CWantedMultiplay::CWantedMultiplay() {}	// 0x1009de20
void CWantedMultiplay::Init( void ) {}	// 0x1009d290
void CWantedMultiplay::RefreshSkillData( void ) { CHalfLifeMultiplay::RefreshSkillData(); }	// 0x1009deb0
void CWantedMultiplay::Think( void ) { CHalfLifeMultiplay::Think(); }	// 0x1009d870
void CWantedMultiplay::InitHUD( CBasePlayer *pl ) { CHalfLifeMultiplay::InitHUD( pl ); }	// 0x1009d840
void CWantedMultiplay::ClientDisconnected( edict_t *pClient ) { CHalfLifeMultiplay::ClientDisconnected( pClient ); }	// 0x1009ddc0
void CWantedMultiplay::PlayerSpawn( CBasePlayer *pPlayer ) { CHalfLifeMultiplay::PlayerSpawn( pPlayer ); }	// 0x1009d830
BOOL CWantedMultiplay::ClientCommand( CBasePlayer *pPlayer, const char *pcmd ) { return TRUE; }	// 0x1009d3f0
void CWantedMultiplay::EndMultiplayerGame( void ) { GoToIntermission(); }	// 0x1009dec0

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

// ---- High Noon: the highnoon file's ----

void CHighNoon::Init( void ) { CWantedMultiplay::Init(); }	// 0x10046790
void CHighNoon::Think( void ) { CWantedMultiplay::Think(); }	// 0x10046ef0
BOOL CHighNoon::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] ) { return CWantedMultiplay::ClientConnected( pEntity, pszName, pszAddress, szRejectReason ); }	// 0x10046c30
void CHighNoon::ClientDisconnected( edict_t *pClient ) { CWantedMultiplay::ClientDisconnected( pClient ); }	// 0x10046c60
void CHighNoon::PlayerSpawn( CBasePlayer *pPlayer ) { CWantedMultiplay::PlayerSpawn( pPlayer ); }	// 0x100467d0
BOOL CHighNoon::FPlayerCanRespawn( CBasePlayer *pPlayer ) { return CWantedMultiplay::FPlayerCanRespawn( pPlayer ); }	// 0x10046c10
edict_t *CHighNoon::GetPlayerSpawnSpot( CBasePlayer *pPlayer ) { return CWantedMultiplay::GetPlayerSpawnSpot( pPlayer ); }	// 0x10047780
void CHighNoon::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor ) { CWantedMultiplay::PlayerKilled( pVictim, pKiller, pInflictor ); }	// 0x10046d00
float CHighNoon::FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo ) { return CWantedMultiplay::FlAmmoRespawnTime( pAmmo ); }	// 0x10047190
