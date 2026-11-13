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

===== player.cpp ========================================================

  functions dealing with the player

*/

#include "extdll.h"
#include "util.h"

#include "cbase.h"
#include "player.h"
#include "trains.h"
#include "nodes.h"
#include "weapons.h"
#include "soundent.h"
#include "monsters.h"
#include "../engine/shake.h"
#include "decals.h"
#include "gamerules.h"
#include "hostage.h"		// Counter-Strike 1.4: SendHostagePos reads CHostage::m_iHostageIndex (the place in the list is ours)
#include "studio.h"

// #define DUCKFIX

extern DLL_GLOBAL ULONG		g_ulModelIndexPlayer;
extern DLL_GLOBAL BOOL		g_fGameOver;
extern DLL_GLOBAL	BOOL	g_fDrawLines;
int gEvilImpulse101;
char g_szMapBriefingText[512];	// Beta 5.0 (leak) (Linux symbol): the map's briefing text (world.cpp reads it, JoiningThink and showbriefing show it)
extern DLL_GLOBAL int		g_iSkillLevel, gDisplayTitle;
BOOL gInitHUD = TRUE;

extern void CopyToBodyQue(entvars_t* pev);
extern void respawn(entvars_t *pev, BOOL fCopyCorpse);
extern Vector VecBModelOrigin(entvars_t *pevBModel );
extern edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer );

// the world node graph
extern CGraph	WorldGraph;

extern cvar_t playerid;	// Counter-Strike 1.1: game.cpp's mp_playerid (declared here in Counter-Strike 1.6 too)
extern cvar_t logdetail;	// Counter-Strike 1.3: game.cpp's mp_logdetail (LogAttack)
extern cvar_t startmoney;	// Counter-Strike 1.3: game.cpp's mp_startmoney

char *GetTeam( int teamNo );	// Counter-Strike 1.3: multiplay_gamerules.cpp's (declared here in Counter-Strike 1.6 too)

#define	PLAYER_WALLJUMP_SPEED 300 // how fast we can spring off walls
#define PLAYER_LONGJUMP_SPEED 350 // how fast we longjump

#define TRAIN_ACTIVE	0x80 
#define TRAIN_NEW		0xc0
#define TRAIN_OFF		0x00
#define TRAIN_NEUTRAL	0x01
#define TRAIN_SLOW		0x02
#define TRAIN_MEDIUM	0x03
#define TRAIN_FAST		0x04 
#define TRAIN_BACK		0x05

#define	FLASH_DRAIN_TIME	 1.2 //100 units/3 minutes
#define	FLASH_CHARGE_TIME	 0.2 // 100 units/20 seconds  (seconds per unit)


//#define PLAYER_MAX_SAFE_FALL_DIST	20// falling any farther than this many feet will inflict damage
//#define	PLAYER_FATAL_FALL_DIST		60// 100% damage inflicted if player falls this many feet
//#define	DAMAGE_PER_UNIT_FALLEN		(float)( 100 ) / ( ( PLAYER_FATAL_FALL_DIST - PLAYER_MAX_SAFE_FALL_DIST ) * 12 )
//#define MAX_SAFE_FALL_UNITS			( PLAYER_MAX_SAFE_FALL_DIST * 12 )

// Global Savedata for player
TYPEDESCRIPTION	CBasePlayer::m_playerSaveData[] = 
{
	DEFINE_FIELD( CBasePlayer, m_flFlashLightTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_iFlashBattery, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_afButtonLast, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonPressed, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonReleased, FIELD_INTEGER ),

	DEFINE_ARRAY( CBasePlayer, m_rgItems, FIELD_INTEGER, MAX_ITEMS ),
	DEFINE_FIELD( CBasePlayer, m_afPhysicsFlags, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_flTimeStepSound, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flTimeWeaponIdle, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flSwimTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flDuckTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flWallJumpTime, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_flSuitUpdate, FIELD_TIME ),
	DEFINE_ARRAY( CBasePlayer, m_rgSuitPlayList, FIELD_INTEGER, CSUITPLAYLIST ),
	DEFINE_FIELD( CBasePlayer, m_iSuitPlayNext, FIELD_INTEGER ),
	DEFINE_ARRAY( CBasePlayer, m_rgiSuitNoRepeat, FIELD_INTEGER, CSUITNOREPEAT ),
	DEFINE_ARRAY( CBasePlayer, m_rgflSuitNoRepeatTime, FIELD_TIME, CSUITNOREPEAT ),
	DEFINE_FIELD( CBasePlayer, m_lastDamageAmount, FIELD_INTEGER ),

	DEFINE_ARRAY( CBasePlayer, m_rgpPlayerItems, FIELD_CLASSPTR, MAX_ITEM_TYPES ),
	DEFINE_FIELD( CBasePlayer, m_pActiveItem, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBasePlayer, m_pLastItem, FIELD_CLASSPTR ),
	
	DEFINE_ARRAY( CBasePlayer, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS ),
	DEFINE_FIELD( CBasePlayer, m_idrowndmg, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_idrownrestored, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_tSneaking, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_iTrain, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_bitsHUDDamage, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_flFallVelocity, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_iTargetVolume, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iWeaponVolume, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iExtraSoundTypes, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iWeaponFlash, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_fLongJump, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_fInitHUD, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_tbdPrev, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_pTank, FIELD_EHANDLE ),
	DEFINE_FIELD( CBasePlayer, m_iHideHUD, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iFOV, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_flDisplayHistory, FIELD_INTEGER ),	// Beta 5.0 (leak)
	DEFINE_FIELD( CBasePlayer, m_iJoiningState, FIELD_INTEGER ),	// 1.0
	
	//DEFINE_FIELD( CBasePlayer, m_fDeadTime, FIELD_FLOAT ), // only used in multiplayer games
	//DEFINE_FIELD( CBasePlayer, m_fGameHUDInitialized, FIELD_INTEGER ), // only used in multiplayer games
	//DEFINE_FIELD( CBasePlayer, m_flStopExtraSoundTime, FIELD_TIME ),
	//DEFINE_FIELD( CBasePlayer, m_fKnownItem, FIELD_INTEGER ), // reset to zero on load
	//DEFINE_FIELD( CBasePlayer, m_iPlayerSound, FIELD_INTEGER ),	// Don't restore, set in Precache()
	//DEFINE_FIELD( CBasePlayer, m_pentSndLast, FIELD_EDICT ),	// Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flSndRoomtype, FIELD_FLOAT ),	// Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flSndRange, FIELD_FLOAT ),	// Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_fNewAmmo, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flgeigerRange, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_flgeigerDelay, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_igeigerRangePrev, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_iStepLeft, FIELD_INTEGER ), // Don't need to restore
	//DEFINE_ARRAY( CBasePlayer, m_szTextureName, FIELD_CHARACTER, CBTEXTURENAMEMAX ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_chTextureType, FIELD_CHARACTER ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_fNoPlayerSound, FIELD_BOOLEAN ), // Don't need to restore, debug
	//DEFINE_FIELD( CBasePlayer, m_iUpdateTime, FIELD_INTEGER ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_iClientHealth, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_iClientBattery, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_iClientHideHUD, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_fWeapon, FIELD_BOOLEAN ),  // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_nCustomSprayFrames, FIELD_INTEGER ), // Don't restore, depends on server message after spawning and only matters in multiplayer
	//DEFINE_FIELD( CBasePlayer, m_vecAutoAim, FIELD_VECTOR ), // Don't save/restore - this is recomputed
	//DEFINE_ARRAY( CBasePlayer, m_rgAmmoLast, FIELD_INTEGER, MAX_AMMO_SLOTS ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_fOnTarget, FIELD_BOOLEAN ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_nCustomSprayFrames, FIELD_INTEGER ), // Don't need to restore
	
};	


int giPrecacheGrunt = 0;
int gmsgShake = 0;
int gmsgFade = 0;
int gmsgFlashlight = 0;
int gmsgFlashBattery = 0;
int gmsgResetHUD = 0;
int gmsgInitHUD = 0;
int gmsgViewMode = 0;	// Counter-Strike 1.3: message ViewMode, no data (Linux symbol)
int gmsgCheckModels = 0;	// Counter-Strike 1.4: message CheckModels, no data (Linux symbol)
int gmsgShowGameTitle = 0;
int gmsgCurWeapon = 0;
int gmsgHealth = 0;
int gmsgDamage = 0;
int gmsgBattery = 0;
int gmsgTrain = 0;
int gmsgLogo = 0;
int gmsgWeaponList = 0;
int gmsgAmmoX = 0;
int gmsgHudText = 0;
int gmsgDeathMsg = 0;
int gmsgScoreAttrib = 0;	// Beta 5.0 (leak)
int gmsgScoreInfo = 0;
int gmsgTeamInfo = 0;
int gmsgTeamScore = 0;
int gmsgGameMode = 0;
int gmsgMOTD = 0;
int gmsgServerName = 0;	// Counter-Strike 1.3: SDK 2.2's
int gmsgAmmoPickup = 0;
int gmsgWeapPickup = 0;
int gmsgItemPickup = 0;
int gmsgHideWeapon = 0;
int gmsgSayText = 0;
int gmsgTextMsg = 0;
int gmsgSetFOV = 0;
int gmsgShowMenu = 0;
int gmsgSendAudio = 0;	// Beta 3.0's radio: a sentence and the entity it comes from (client.dll plays it)
int gmsgRoundTime = 0;	// Beta 4.0
// Beta 5.0 (leak)'s new messages (Beta 4.0's GiveItem and RemoveItem are gone)
int gmsgMoney = 0;
int gmsgBlinkAcct = 0;
int gmsgArmorType = 0;
int gmsgStatusValue = 0;
int gmsgStatusText = 0;
int gmsgStatusIcon = 0;
int gmsgBarTime = 0;
int gmsgReloadSound = 0;
int gmsgCrosshair = 0;	// Beta 5.0 (Linux symbol): the observers' crosshair on and off
int gmsgNVGToggle = 0;	// Beta 6.0: the night vision on and off (message NVGToggle, 1 byte; Linux symbol)
int gmsgRadar = 0;		// Beta 6.0: a team mate's position on the radar (message Radar, 7 bytes; Linux symbol)
int gmsgSpectator = 0;	// Beta 6.5: a player has become an observer or left (message Spectator, 2 bytes; Linux symbol)
int gmsgVGUIMenu = 0;	// Beta 7.0: a VGUI menu to show (message VGUIMenu, variable length; Linux symbol)
int gmsgAllowSpec = 0;	// Counter-Strike 1.1: the value of allow_spectators for the team menu (message AllowSpec, 1 byte; Linux symbol)
// Counter-Strike 1.4 (Linux symbols; 1.3's WClose, WClose2, SelAmmo and SetCurWeap are gone)
int gmsgBombDrop = 0;		// the bomb was dropped at this place (message BombDrop, three coordinates)
int gmsgBombPickup = 0;		// the bomb was picked up (message BombPickup, no data)
int gmsgHostagePos = 0;		// a hostage's place for the radar (message HostagePos: a flag, the hostage's number, three coordinates)
int gmsgHostageK = 0;		// a hostage was killed (message HostageK: his number)
int gmsgGeigerRange = 0;
int gmsgSendCorpse = 0;		// Counter-Strike 1.4: a dead player's body, made by the client (message ClCorpse; Linux symbol)

// Beta 6.5: SDK 2.0's function (client.cpp's ClientPrecache calls it) with the
// registrations Beta 6.1 had in CBasePlayer::Precache
void LinkUserMessages( void )
{
	// Already taken care of?
	if ( gmsgCurWeapon )
	{
		return;
	}

	gmsgCurWeapon = REG_USER_MSG("CurWeapon", 3);
	gmsgGeigerRange = REG_USER_MSG("Geiger", 1);
	gmsgFlashlight = REG_USER_MSG("Flashlight", 2);
	gmsgFlashBattery = REG_USER_MSG("FlashBat", 1);
	gmsgHealth = REG_USER_MSG( "Health", 1 );
	gmsgDamage = REG_USER_MSG( "Damage", 12 );
	gmsgBattery = REG_USER_MSG( "Battery", 2);
	gmsgTrain = REG_USER_MSG( "Train", 1);
	gmsgHudText = REG_USER_MSG( "HudText", -1 );
	gmsgSayText = REG_USER_MSG( "SayText", -1 );
	gmsgTextMsg = REG_USER_MSG( "TextMsg", -1 );
	gmsgWeaponList = REG_USER_MSG("WeaponList", -1);
	gmsgResetHUD = REG_USER_MSG("ResetHUD", 0);		// called every respawn (Beta 5.0 (leak): no byte)
	gmsgInitHUD = REG_USER_MSG("InitHUD", 0 );		// called every time a new player joins the server
	gmsgViewMode = REG_USER_MSG("ViewMode", 0 );	// Counter-Strike 1.3
	gmsgCheckModels = REG_USER_MSG("CheckModels", 0 );	// Counter-Strike 1.4
	gmsgShowGameTitle = REG_USER_MSG("GameTitle", 1);
	gmsgDeathMsg = REG_USER_MSG( "DeathMsg", -1 );
	gmsgScoreAttrib = REG_USER_MSG( "ScoreAttrib", 2 );	// Beta 5.0 (leak)
	gmsgScoreInfo = REG_USER_MSG( "ScoreInfo", 9 );	// Beta 6.6: two more shorts (was 5)
	gmsgTeamInfo = REG_USER_MSG( "TeamInfo", -1 );  // sets the name of a player's team
	gmsgTeamScore = REG_USER_MSG( "TeamScore", -1 );  // sets the score of a team on the scoreboard
	gmsgGameMode = REG_USER_MSG( "GameMode", 1 );
	gmsgMOTD = REG_USER_MSG( "MOTD", -1 );
	gmsgServerName = REG_USER_MSG( "ServerName", -1 );
	gmsgAmmoPickup = REG_USER_MSG( "AmmoPickup", 2 );
	gmsgWeapPickup = REG_USER_MSG( "WeapPickup", 1 );
	gmsgItemPickup = REG_USER_MSG( "ItemPickup", -1 );
	gmsgHideWeapon = REG_USER_MSG( "HideWeapon", 1 );
	gmsgSetFOV = REG_USER_MSG( "SetFOV", 1 );
	gmsgShowMenu = REG_USER_MSG( "ShowMenu", -1 );
	gmsgShake = REG_USER_MSG("ScreenShake", sizeof(ScreenShake));
	gmsgFade = REG_USER_MSG("ScreenFade", sizeof(ScreenFade));
	gmsgAmmoX = REG_USER_MSG("AmmoX", 2);
	gmsgSendAudio = REG_USER_MSG( "SendAudio", -1 );
	gmsgRoundTime = REG_USER_MSG( "RoundTime", 2 );	// Beta 4.0; Beta 5.0 (leak): seconds in a short
	// Beta 5.0 (leak)
	gmsgMoney = REG_USER_MSG( "Money", 5 );
	gmsgArmorType = REG_USER_MSG( "ArmorType", 1 );
	gmsgBlinkAcct = REG_USER_MSG( "BlinkAcct", 1 );
	gmsgStatusValue = REG_USER_MSG( "StatusValue", -1 );
	gmsgStatusText = REG_USER_MSG( "StatusText", -1 );
	gmsgStatusIcon = REG_USER_MSG( "StatusIcon", -1 );
	gmsgBarTime = REG_USER_MSG( "BarTime", 2 );		// Counter-Strike 1.4: a short (-1 before)
	gmsgReloadSound = REG_USER_MSG( "ReloadSound", 2 );
	gmsgCrosshair = REG_USER_MSG( "Crosshair", 1 );	// Beta 5.0
	gmsgNVGToggle = REG_USER_MSG( "NVGToggle", 1 );	// Beta 6.0
	gmsgRadar = REG_USER_MSG( "Radar", 7 );			// Beta 6.0
	gmsgSpectator = REG_USER_MSG( "Spectator", 2 );	// Beta 6.5
	gmsgVGUIMenu = REG_USER_MSG( "VGUIMenu", -1 );	// Beta 7.0
	gmsgAllowSpec = REG_USER_MSG( "AllowSpec", 1 );	// Counter-Strike 1.1
	// Counter-Strike 1.4
	gmsgBombDrop = REG_USER_MSG( "BombDrop", 6 );
	gmsgBombPickup = REG_USER_MSG( "BombPickup", 0 );
	gmsgSendCorpse = REG_USER_MSG( "ClCorpse", -1 );
	gmsgHostagePos = REG_USER_MSG( "HostagePos", 8 );
	gmsgHostageK = REG_USER_MSG( "HostageK", 1 );
}

LINK_ENTITY_TO_CLASS( player, CBasePlayer );

// Counter-Strike 1.0b: the model names are string literals again (Beta 7.0 to
// 1.0 wrote them character by character from client.cpp's 36 letter globals,
// which this file no longer names; 1.0b's image does not have them)


//=========================================================
// GetCSModelName - Beta 5.0 (leak) (Linux symbol): the world
// model of a weapon dropped in a weaponbox; the C4 is the
// backpack. Linux symbol, a static function as there: VC6
// emits it after its first user, PackDeadPlayerItems
//=========================================================
static char *GetCSModelName( int item_id )
{
	char *modelName;	// name ours

	switch ( item_id )
	{
	case WEAPON_AK47:		modelName = "models/w_ak47.mdl"; break;
	case WEAPON_AWP:		modelName = "models/w_awp.mdl"; break;
	case WEAPON_DEAGLE:		modelName = "models/w_deagle.mdl"; break;
	case WEAPON_G3SG1:		modelName = "models/w_g3sg1.mdl"; break;
	case WEAPON_SG550:		modelName = "models/w_sg550.mdl"; break;	// 1.0
	case WEAPON_GLOCK18:	modelName = "models/w_glock18.mdl"; break;
	case WEAPON_M249:		modelName = "models/w_m249.mdl"; break;
	case WEAPON_M3:			modelName = "models/w_m3.mdl"; break;
	case WEAPON_M4A1:		modelName = "models/w_m4a1.mdl"; break;
	case WEAPON_MP5N:		modelName = "models/w_mp5.mdl"; break;
	case WEAPON_SG552:		modelName = "models/w_sg552.mdl"; break;
	case WEAPON_AUG:		modelName = "models/w_aug.mdl"; break;	// Beta 6.0
	case WEAPON_TMP:		modelName = "models/w_tmp.mdl"; break;
	case WEAPON_USP:		modelName = "models/w_usp.mdl"; break;
	case WEAPON_ELITE:		modelName = "models/w_elite.mdl"; break;	// Beta 7.0
	case WEAPON_FIVESEVEN:	modelName = "models/w_fiveseven.mdl"; break;	// 1.0
	case WEAPON_P90:		modelName = "models/w_p90.mdl"; break;
	case WEAPON_UMP45:		modelName = "models/w_ump45.mdl"; break;	// 1.0
	case WEAPON_MAC10:		modelName = "models/w_mac10.mdl"; break;	// Beta 6.0
	case WEAPON_P228:		modelName = "models/w_p228.mdl"; break;
	case WEAPON_SCOUT:		modelName = "models/w_scout.mdl"; break;
	case WEAPON_KNIFE:		modelName = "models/w_knife.mdl"; break;
	case WEAPON_FLASHBANG:	modelName = "models/w_flashbang.mdl"; break;
	case WEAPON_HEGRENADE:	modelName = "models/w_hegrenade.mdl"; break;
	case WEAPON_SMOKEGRENADE:	modelName = "models/w_smokegrenade.mdl"; break;	// Beta 6.5
	case WEAPON_XM1014:		modelName = "models/w_xm1014.mdl"; break;
	case WEAPON_C4:			modelName = "models/w_backpack.mdl"; break;
	default:
		modelName = NULL;
		ALERT( at_console, "CBasePlayer::PackDeadPlayerItems(): Unhandled item- not creating weaponbox\n" );
	}

	return modelName;
}

//=========================================================
// SetPlayerModel - Beta 5.0 (leak) (Linux symbol): the model
// of the player's appearance (m_iAppearance; Beta 6.5: 1
// urban, 2 terror, 3 leet, 4 arctic, 5 gsg9, 6 gign, 7 sas,
// 8 guerilla, 9 vip). Counter-Strike 1.0b: the names are
// literals again (Beta 7.0 to 1.0 put them together from
// letters at run time, eighteen buffers on every call), the
// team decides first (a counter-terrorist can only have one
// of his team's five models, a terrorist one of his four;
// anyone else is urban), and the "model" key is written only
// when it differs; no "model ..." command is sent to the
// client any more. HasC4 is not read. The locals' names are
// 1.6's.
//=========================================================
void CBasePlayer::SetPlayerModel( BOOL HasC4 )
{
	char *infobuffer = g_engfuncs.pfnGetInfoKeyBuffer( edict() );
	char *model;

	if ( m_iTeam == TEAM_CT )
	{
		switch ( m_iAppearance )
		{
		case 1:
			model = "urban";
			break;
		case 5:
			model = "gsg9";
			break;
		case 7:
			model = "sas";
			break;
		case 6:
			model = "gign";
			break;
		case 9:
			model = "vip";
			break;
		default:
			model = "urban";
			break;
		}
	}
	else if ( m_iTeam == TEAM_TERRORIST )
	{
		switch ( m_iAppearance )
		{
		case 2:
			model = "terror";
			break;
		case 3:
			model = "leet";
			break;
		case 4:
			model = "arctic";
			break;
		case 8:
			model = "guerilla";
			break;
		default:
			model = "terror";
			break;
		}
	}
	else
		model = "urban";

	if ( strcmp( g_engfuncs.pfnInfoKeyValue( infobuffer, "model" ), model ) )
	{
		g_engfuncs.pfnSetClientKeyValue( entindex(), infobuffer, "model", model );
	}
}

//=========================================================
// Radio - Beta 5.0 (leak): "<name> (RADIO): <msg_verbose>" in
// the chat of every living team mate who has not turned the
// radio off, and the sentence msg_id (the callers pass
// "%!MRAD_..." themselves) sent to him with his own entity
// index (sic, as Beta 3.0's); then the radio sprite over the
// speaker's head, for each of them alone.
// Counter-Strike 1.1: the chat line is the client's "#Game_radio"
// with the speaker's name and msg_verbose (a "#..." title of the
// client's too) as its arguments; 1.0b built "%s (RADIO): %s\n"
// here with UTIL_VarArgs.
//=========================================================
void CBasePlayer::Radio( const char *msg_id, const char *msg_verbose )
{
	if ( !IsPlayer() )
		return;

	// Counter-Strike 1.4: 1.3's sentence said by the speaker himself is gone again

	CBaseEntity *pPlayer = NULL;
	pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
	{
		if ( pPlayer->IsPlayer() && pPlayer->pev->flags != FL_DORMANT )
		{
			CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

			if ( player->pev->deadflag == DEAD_NO && player->m_iTeam == m_iTeam && !player->m_fIgnoreRadio )
			{
				// Beta 6.0: a radio call without text (msg_verbose NULL) sends only the sound
				if ( msg_verbose )
					ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#Game_radio", STRING( pev->netname ), msg_verbose );

				// Beta 6.0: the speaker's index (the client's radar blinks him; Beta 5.2 sent the hearer's)
				MESSAGE_BEGIN( MSG_ONE, gmsgSendAudio, NULL, pPlayer->pev );
					WRITE_BYTE( ENTINDEX( edict() ) );
					WRITE_STRING( msg_id );
				MESSAGE_END();

				// the radio sprite over the speaker's head for 1.5 seconds
				if ( msg_verbose )
				{
					MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY, NULL, pPlayer->pev );
						WRITE_BYTE( TE_PLAYERATTACHMENT );
						WRITE_BYTE( ENTINDEX( edict() ) );
						WRITE_COORD( 35 );
						WRITE_SHORT( g_sModelIndexRadio );
						WRITE_SHORT( 15 );
					MESSAGE_END();
				}
			}
		}
		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	}
}



// Beta 3.0: the sound of the hit, by where it struck: a helmet or kevlar
// sound where armour took it (not on the legs), else one of three flesh or
// head-shot sounds
void CBasePlayer :: Pain( int iLastHitGroup, bool bHasArmour )
{
	int temp = RANDOM_LONG( 0, 2 );

	switch ( iLastHitGroup )
	{
	case HITGROUP_HEAD:
		if ( m_iKevlar != 2 )	// Beta 6.0: the helmet sound only with a helmet (the assault suit)
		{
			if ( temp == 0 )
				EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/headshot1.wav", 1, ATTN_NORM );
			else if ( temp == 1 )
				EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/headshot2.wav", 1, ATTN_NORM );
			else
				EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/headshot3.wav", 1, ATTN_NORM );
		}
		else
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/bhit_helmet-1.wav", 1, ATTN_NORM );
		break;
	case HITGROUP_LEFTLEG:
		if ( temp == 0 )
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/bhit_flesh-1.wav", 1, ATTN_NORM );
		else if ( temp == 1 )
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/bhit_flesh-2.wav", 1, ATTN_NORM );
		else
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/bhit_flesh-3.wav", 1, ATTN_NORM );
		break;
	case HITGROUP_RIGHTLEG:
		if ( temp == 0 )
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/bhit_flesh-1.wav", 1, ATTN_NORM );
		else if ( temp == 1 )
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/bhit_flesh-2.wav", 1, ATTN_NORM );
		else
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/bhit_flesh-3.wav", 1, ATTN_NORM );
		break;
	default:
		if ( !bHasArmour )
		{
			if ( temp == 0 )
				EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/bhit_flesh-1.wav", 1, ATTN_NORM );
			else if ( temp == 1 )
				EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/bhit_flesh-2.wav", 1, ATTN_NORM );
			else
				EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/bhit_flesh-3.wav", 1, ATTN_NORM );
		}
		else
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/bhit_kevlar-1.wav", 1, ATTN_NORM );
		break;
	}
}

/* 
 *
 */
Vector VecVelocityForDamage(float flDamage)
{
	Vector vec(RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));

	if (flDamage > -50)
		vec = vec * 0.7;
	else if (flDamage > -200)
		vec = vec * 2;
	else
		vec = vec * 10;
	
	return vec;
}

#if 0 /*
static void ThrowGib(entvars_t *pev, char *szGibModel, float flDamage)
{
	edict_t *pentNew = CREATE_ENTITY();
	entvars_t *pevNew = VARS(pentNew);

	pevNew->origin = pev->origin;
	SET_MODEL(ENT(pevNew), szGibModel);
	UTIL_SetSize(pevNew, g_vecZero, g_vecZero);

	pevNew->velocity		= VecVelocityForDamage(flDamage);
	pevNew->movetype		= MOVETYPE_BOUNCE;
	pevNew->solid			= SOLID_NOT;
	pevNew->avelocity.x		= RANDOM_FLOAT(0,600);
	pevNew->avelocity.y		= RANDOM_FLOAT(0,600);
	pevNew->avelocity.z		= RANDOM_FLOAT(0,600);
	CHANGE_METHOD(ENT(pevNew), em_think, SUB_Remove);
	pevNew->ltime		= gpGlobals->time;
	pevNew->nextthink	= gpGlobals->time + RANDOM_FLOAT(10,20);
	pevNew->frame		= 0;
	pevNew->flags		= 0;
}
	
	
static void ThrowHead(entvars_t *pev, char *szGibModel, floatflDamage)
{
	SET_MODEL(ENT(pev), szGibModel);
	pev->frame			= 0;
	pev->nextthink		= -1;
	pev->movetype		= MOVETYPE_BOUNCE;
	pev->takedamage		= DAMAGE_NO;
	pev->solid			= SOLID_NOT;
	pev->view_ofs		= Vector(0,0,8);
	UTIL_SetSize(pev, Vector(-16,-16,0), Vector(16,16,56));
	pev->velocity		= VecVelocityForDamage(flDamage);
	pev->avelocity		= RANDOM_FLOAT(-1,1) * Vector(0,600,0);
	pev->origin.z -= 24;
	ClearBits(pev->flags, FL_ONGROUND);
}


*/ 
#endif

int TrainSpeed(int iSpeed, int iMax)
{
	float fSpeed, fMax;
	int iRet = 0;

	fMax = (float)iMax;
	fSpeed = iSpeed;

	fSpeed = fSpeed/fMax;

	if (iSpeed < 0)
		iRet = TRAIN_BACK;
	else if (iSpeed == 0)
		iRet = TRAIN_NEUTRAL;
	else if (fSpeed < 0.33)
		iRet = TRAIN_SLOW;
	else if (fSpeed < 0.66)
		iRet = TRAIN_MEDIUM;
	else
		iRet = TRAIN_FAST;

	return iRet;
}

void CBasePlayer :: DeathSound( void )
{
	// water death sounds
	/*
	if (pev->waterlevel == 3)
	{
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/h2odeath.wav", 1, ATTN_NONE);
		return;
	}
	*/

	// Beta 4.0: death6.wav added; the console line is gone
	switch (RANDOM_LONG(1,4))
	{
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/die1.wav", 1, ATTN_NORM);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/die2.wav", 1, ATTN_NORM);
		break;
	case 3:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/die3.wav", 1, ATTN_NORM);
		break;
	case 4:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/death6.wav", 1, ATTN_NORM);
		break;
	}
}

// override takehealth
// bitsDamageType indicates type of damage healed. 

int CBasePlayer :: TakeHealth( float flHealth, int bitsDamageType )
{
	return CBaseMonster :: TakeHealth (flHealth, bitsDamageType);

}

Vector CBasePlayer :: GetGunPosition( )
{
//	UTIL_MakeVectors(pev->v_angle);
//	m_HackedGunPos = pev->view_ofs;
	Vector origin;
	
	origin = pev->origin + pev->view_ofs;
	return origin;
}

//=========================================================
// TraceAttack
//=========================================================
void CBasePlayer :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	bool bShouldBleed = true;	// name ours up to 1.3; 1.6 has both names
	bool bShouldSpark = false;

	// Counter-Strike 1.1: a team mate's hit draws no blood when friendly fire is off
	CBasePlayer *pAttacker = (CBasePlayer *)CBaseEntity::Instance( pevAttacker );

	if ( CVAR_GET_FLOAT( "mp_friendlyfire" ) == 0 && m_iTeam == pAttacker->m_iTeam )
		bShouldBleed = false;

	if ( pev->takedamage == 0 )
		return;

	m_LastHitGroup = ptr->iHitgroup;

	// Beta 3.0: the hit kicks the view by the damage, by where it struck;
	// a hit where the kevlar is (head, body, arms) draws no blood. Beta 3.1:
	// much less kick (the head up to 12 degrees up and 9 sideways, the body
	// a tenth of the damage up to 4 degrees, arms and legs none; the
	// readme's "disorientation system changed") and both legs 0.75.
	// Counter-Strike 1.4: no kick where no blood is drawn
	switch ( ptr->iHitgroup )
	{
	case HITGROUP_GENERIC:
		break;
	case HITGROUP_HEAD:
		if ( m_iKevlar == 2 )	// Beta 6.0: only the helmet (the assault suit) keeps the head from bleeding
		{
			bShouldBleed = false;
			bShouldSpark = true;
		}
		flDamage *= 4;
		if ( bShouldBleed == true )
		{
			pev->punchangle.x = flDamage * -0.5;
			if ( pev->punchangle.x < -12 )
				pev->punchangle.x = -12;
			pev->punchangle.z = flDamage * RANDOM_FLOAT( -1, 1 );
			if ( pev->punchangle.z < -9 )
				pev->punchangle.z = -9;
			else if ( pev->punchangle.z > 9 )
				pev->punchangle.z = 9;
		}
		break;
	case HITGROUP_CHEST:	// Beta 4.0 to 4.1: the chest took 1.25 times the damage; Beta 5.0 (leak): the stomach again, as Beta 3.1
		if ( m_iKevlar )
			bShouldBleed = false;
		if ( bShouldBleed == true )
		{
			pev->punchangle.x = flDamage * -0.1;
			if ( pev->punchangle.x < -4 )
				pev->punchangle.x = -4;
		}
		break;
	case HITGROUP_STOMACH:
		if ( m_iKevlar )
			bShouldBleed = false;
		flDamage *= 1.25;
		if ( bShouldBleed == true )
		{
			pev->punchangle.x = flDamage * -0.1;
			if ( pev->punchangle.x < -4 )
				pev->punchangle.x = -4;
		}
		break;
	case HITGROUP_LEFTARM:
	case HITGROUP_RIGHTARM:
		if ( m_iKevlar )
			bShouldBleed = false;
		break;
	case HITGROUP_LEFTLEG:
	case HITGROUP_RIGHTLEG:
		flDamage *= 0.75;
		break;
	default:
		break;
	}

	if ( bShouldBleed == true )
	{
		// Beta 6.0: a spray of blood out of the wound as well
		BloodSplat( ptr->vecEndPos, vecDir, ptr->iHitgroup, flDamage * 5 );
		SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage);// a little surface blood.
		TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
	}
	else if ( ptr->iHitgroup == HITGROUP_HEAD && bShouldSpark == true )
	{
		// Beta 6.0: a hit on the helmet throws sparks off its surface
		Vector vecNormal = ptr->vecPlaneNormal;	// name ours

		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, ptr->vecEndPos );
			WRITE_BYTE( TE_STREAK_SPLASH );
			WRITE_COORD( ptr->vecEndPos.x );
			WRITE_COORD( ptr->vecEndPos.y );
			WRITE_COORD( ptr->vecEndPos.z );
			WRITE_COORD( vecNormal.x );
			WRITE_COORD( vecNormal.y );
			WRITE_COORD( vecNormal.z );
			WRITE_BYTE( 5 );	// color
			WRITE_SHORT( 22 );	// count
			WRITE_SHORT( 25 );	// base speed
			WRITE_SHORT( 65 );	// random velocity
		MESSAGE_END();
	}

	AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
}

// GetWeaponName - Counter-Strike 1.3: the name of what did the damage, for the log; the text is
// CHalfLifeMultiplay::DeathNotice's (the names are Counter-Strike 1.6's)
const char *GetWeaponName( entvars_t *pevInflictor, entvars_t *pKiller )
{
	int killer_index = 0;
	const char *killer_weapon_name = "world";		// by default, the player is killed by the world

	if ( pKiller->flags & FL_CLIENT )
	{
		killer_index = ENTINDEX(ENT(pKiller));
		
		if ( pevInflictor )
		{
			if ( pevInflictor == pKiller )
			{
				// If the inflictor is the killer,  then it must be their current weapon doing the damage
				CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance( pKiller );
				
				if ( pPlayer && pPlayer->m_pActiveItem )
				{
					killer_weapon_name = pPlayer->m_pActiveItem->pszName();
				}
			}
			else
			{
				killer_weapon_name = STRING( pevInflictor->classname );  // it's just that easy
			}
		}
	}
	else
	{
		killer_weapon_name = STRING( pevInflictor->classname );
	}

	// strip the monster_* or weapon_* from the inflictor's classname
	if ( strncmp( killer_weapon_name, "weapon_", 7 ) == 0 )
		killer_weapon_name += 7;
	else if ( strncmp( killer_weapon_name, "monster_", 8 ) == 0 )
		killer_weapon_name += 8;
	else if ( strncmp( killer_weapon_name, "func_", 5 ) == 0 )
		killer_weapon_name += 5;

	return killer_weapon_name;
}

// LogAttack - Counter-Strike 1.3: one log line for a hit, when mp_logdetail asks for it (1: hits on
// enemies, 2: hits on team mates, 3: both)
void LogAttack( CBasePlayer *pAttacker, CBasePlayer *pVictim, int teamAttack, int healthHit, int armorHit, int newHealth, int newArmor, const char *killer_weapon_name )
{
	int detail = logdetail.value;

	if ( !detail )
		return;

	if ( !pAttacker || !pVictim )
		return;

	if ( ( teamAttack && ( detail & 2 ) ) || ( !teamAttack && ( detail & 1 ) ) )
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" attacked \"%s<%i><%s><%s>\" with \"%s\" (damage \"%d\") (damage_armor \"%d\") (health \"%d\") (armor \"%d\")\n",
			STRING( pAttacker->pev->netname ),
			GETPLAYERUSERID( pAttacker->edict() ),
			GETPLAYERAUTHID( pAttacker->edict() ),
			GetTeam( pAttacker->m_iTeam ),
			STRING( pVictim->pev->netname ),
			GETPLAYERUSERID( pVictim->edict() ),
			GETPLAYERAUTHID( pVictim->edict() ),
			GetTeam( pVictim->m_iTeam ),
			killer_weapon_name,
			healthHit,
			armorHit,
			newHealth,
			newArmor );
	}
}

/*
	Take some damage.  
	NOTE: each call to TakeDamage with bitsDamageType set to a time-based damage
	type will cause the damage time countdown to be reset.  Thus the ongoing effects of poison, radiation
	etc are implemented with subsequent calls to TakeDamage using DMG_GENERIC.
*/

#define ARMOR_RATIO	 0.5	// Armor Takes 80% of the damage (Counter-Strike Beta 2.0: 60%, 3.0: 50% unless the attacker's weapon says otherwise)
#define ARMOR_BONUS  0.5	// Each Point of Armor is work 1/x points of health

int CBasePlayer :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Counter-Strike 1.4: the suit's diagnosis at the end is gone, and with it SDK 2.0's locals for it
	int fTookDamage;
	float flBonus;
	float flRatio;
	float flHealthPrev = pev->health;
	int iGunType;	// Beta 7.0: the attacker's weapon (Counter-Strike 1.6's name)
	// Counter-Strike 1.3: what LogAttack is told (Counter-Strike 1.6's names)
	int teamAttack = FALSE, armorHit = 0;
	CBasePlayer *pAttack = NULL;

	// Counter-Strike 1.4: a blast, a fall and a grenade hit no part of the body
	if ( bitsDamageType & ( DMG_BLAST | DMG_FALL | DMG_HEGRENADE ) )
		m_LastHitGroup = HITGROUP_GENERIC;

	flBonus = ARMOR_BONUS;
	flRatio = ARMOR_RATIO;

	// Beta 6.0: the VIP's armour lets only a quarter through
	if ( m_bIsVIP == true )
		flRatio = 0.25;

	// Beta 4.0: a blast (the C4's, an HE grenade's) is taken apart from the rest:
	// no flinch first, armour takes half of it at no extra cost, and a fatal one
	// throws the body (SetAnimation, Killed)
	if ( bitsDamageType & ( DMG_BLAST | DMG_HEGRENADE ) )
	{
		if ( !IsAlive() )
			return 0;

		// an HE grenade spares the thrower's team unless mp_friendlyfire (Beta 5.0 leak;
		// Beta 4.0 to 4.1: mp_friendly_grenade_damage)
		// Counter-Strike 1.3: with mp_friendlyfire the thrower is the attacker of the log line
		// (the locals' names are Counter-Strike 1.6's)
		if ( bitsDamageType & DMG_HEGRENADE )
		{
			CGrenade *pGrenade;
			CBaseEntity *temp = GetClassPtr( (CBaseEntity *)pevInflictor );

			if ( !strcmp( STRING( temp->pev->classname ), "grenade" ) )
			{
				pGrenade = GetClassPtr( (CGrenade *)pevInflictor );

				if ( CVAR_GET_FLOAT( "mp_friendlyfire" ) == 0 )
				{
					// Beta 5.0: the thrower's own grenade hurts him
					if ( pGrenade->m_iTeam == m_iTeam && &edict()->v != pevAttacker )
						return 0;
				}
				else
				{
					if ( pGrenade->m_iTeam == m_iTeam )
						teamAttack = TRUE;

					pAttack = (CBasePlayer *)CBaseEntity::Instance( pevAttacker );
				}
			}
		}

		if ( !FNullEnt( ENT( pevInflictor ) ) )
			m_vecBlastDir = pev->origin - pevInflictor->origin;

		// Counter-Strike 1.4: IsArmored; the armour pays half of what it takes (flBonus)
		if ( pev->armorvalue != 0.0f && IsArmored( m_LastHitGroup ) )
		{
			float flNew = flDamage * flRatio;
			float flArmor;

			flArmor = (flDamage - flNew) * flBonus;

			// Does this use more armor than we have?
			// Counter-Strike 1.3: armorHit, the armour lost (when there is too little: what was asked for)
			if (flArmor > pev->armorvalue)
			{
				armorHit = flArmor;
				flArmor = pev->armorvalue;
				flNew = flDamage - flArmor;
				pev->armorvalue = 0;
			}
			else
			{
				int oldValue = pev->armorvalue;

				pev->armorvalue -= flArmor;
				armorHit = oldValue - (int)pev->armorvalue;
			}

			flDamage = flNew;

			if ( pev->armorvalue <= 0.0 )
				m_iKevlar = 0;

			Pain( m_LastHitGroup, true );
		}
		else
			Pain( m_LastHitGroup, false );

		m_lastDamageAmount = flDamage;

		if ( (int)flDamage >= pev->health )
		{
			if ( bitsDamageType & DMG_BLAST )
				m_fKilledByBlast = true;
			else if ( bitsDamageType & DMG_HEGRENADE )
				m_fKilledByHEGrenade = true;
		}
		else
		{
			SetAnimation( PLAYER_FLINCH );
			Pain( m_LastHitGroup, false );
		}

		LogAttack( pAttack, this, teamAttack, (int)flDamage, armorHit, (int)pev->health - (int)flDamage, (int)pev->armorvalue, GetWeaponName( pevInflictor, pevAttacker ) );

		float fTookDamage = CBaseMonster::TakeDamage( pevInflictor, pevAttacker, (int)flDamage, bitsDamageType );

		// reset damage time countdown for each type of time based damage player just sustained
		for ( int i = 0; i < CDMG_TIMEBASED; i++ )
			if ( bitsDamageType & ( DMG_PARALYZE << i ) )
				m_rgbTimeBasedDamage[i] = 0;

		// tell director about it
		MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
			WRITE_BYTE ( 9 );	// command length in bytes
			WRITE_BYTE ( DRC_CMD_EVENT );	// take damage event
			WRITE_SHORT( ENTINDEX(this->edict()) );	// index number of primary entity
			WRITE_SHORT( ENTINDEX(ENT(pevInflictor)) );	// index number of secondary entity
			WRITE_LONG( 5 );   // eventflags (priority and flags)
		MESSAGE_END();

		ALERT( at_console, "EXPLOSION DAMAGE\n" );	// Beta 5.0 (leak)

		return fTookDamage;
	}

	CBaseEntity *pAttacker = CBaseEntity::Instance( pevAttacker );

	// Beta 4.0: a grenade's damage is taken whatever the rules say
	if ( !g_pGameRules->FPlayerCanTakeDamage( this, pAttacker ) && strcmp( "grenade", STRING( pevInflictor->classname ) ) )
	{
		// Refuse the damage
		return 0;
	}

	// Counter-Strike 1.4: SDK 2.0's lines are back (a blast never comes this far)
	if ( ( bitsDamageType & DMG_BLAST ) && g_pGameRules->IsMultiplayer() )
	{
		// blasts damage armor more.
		flBonus *= 2;
	}

	// Already dead
	if ( !IsAlive() )
		return 0;
	// go take the damage first


	// a player's hits: half damage to a team mate (and to himself), who is told
	// so (Beta 5.0 leak: the money for hitting an enemy is gone)
	// Beta 6.5: the attacker's entity is checked before it is asked
	CBaseEntity *attacker = GetClassPtr( (CBaseEntity *)pevAttacker );	// Counter-Strike 1.6's name

	if ( attacker && attacker->IsPlayer() )
	{
		pAttack = GetClassPtr( (CBasePlayer *)pevAttacker );

		if ( pAttack->m_iTeam == m_iTeam )
		{
			// the victim's history decides whether the attacker hears the hint (sic)
			if ( !( m_flDisplayHistory & 0x200 ) )
			{
				m_flDisplayHistory |= 0x200;
				pAttack->HintMessage( "#Hint_try_not_to_injure_teammates", FALSE, FALSE );
			}

			teamAttack = TRUE;

			// Beta 7.0: the attacker's team reads it in the chat, at most once in
			// 0.6 seconds (6.6: "You injured a teammate!" in the middle of the
			// attacker's screen). Counter-Strike 1.1: the client's title
			// "#Game_teammate_attack" with the attacker's name, sent here to
			// every player of the victim's team (1.0b: "%s attacked a teammate\n"
			// written to a static buffer and passed to TeamPrint, which 1.1 no
			// longer has). The locals' names are Counter-Strike 1.6's.
			if ( ( pAttack->m_flLastAttackedTeammate + 0.6 ) < gpGlobals->time )
			{
				pAttack->m_flLastAttackedTeammate = gpGlobals->time;

				CBaseEntity *pBasePlayer = NULL;
				CBasePlayer *basePlayer;

				while ( ( pBasePlayer = UTIL_FindEntityByClassname( pBasePlayer, "player" ) ) != NULL )
				{
					if ( FNullEnt( pBasePlayer->edict() ) )
						break;

					basePlayer = GetClassPtr( (CBasePlayer *)pBasePlayer->pev );

					if ( basePlayer->m_iTeam == m_iTeam )
						ClientPrint( basePlayer->pev, HUD_PRINTTALK, "#Game_teammate_attack", STRING( pAttack->pev->netname ) );
				}
			}
		}

		// Beta 7.0: 35% (6.6: half)
		if ( pAttack->m_iTeam == m_iTeam )
			flDamage *= 0.35;

		// Beta 3.0: how much of the damage the armour lets through depends
		// on the attacker's weapon, a multiple of ARMOR_RATIO
		// (the multipliers are what the constant pool shows: VC5 folds
		// 0.5 * 1.4 itself and keys the product at double precision, so
		// the float 0.7 of PlayerUse's VIEW_FIELD_NARROW later gets an
		// unreferenced pool entry of its own, as in retail; a literal
		// flRatio = 0.7 shares one with it)
		// (one case per weapon: the object's constants come in the order of
		// their first use, M4A1's 0.775 first, the code in the order of the
		// cases kept after VC5 merges equal ones, the later copy of each)
		// Beta 5.0 (leak): no active weapon, the default; the MP5 Navy has
		// the default's ratio too now (Beta 4.1: 0.45), and so has the TMP,
		// whose case is gone
		// Beta 7.0: the weapon's number is kept for the flinch below; without
		// a weapon it is not set (sic)
		if ( pAttack->m_pActiveItem )
		{
		iGunType = pAttack->m_pActiveItem->m_iId;

		switch ( iGunType )
		{
		case WEAPON_USP:	// Beta 4.0: times 1, which VC5 folds away; the case keeps its jump-table slot, as retail's (an empty case gets none)
			flRatio *= 1;
			break;
		case WEAPON_M3:
			flRatio *= 1;
			break;
		// Counter-Strike 1.0: the cases with one number stand together (the
		// order of retail's blocks). M4A1 1.4 (Beta 7.1: 1.375), SG-552 and
		// AUG 1.4 (1.45), P90 1.5 (1.35); new: Five-seveN 1.5, SG-550 1.45
		case WEAPON_AUG:
		case WEAPON_M4A1:
		case WEAPON_SG552:
			flRatio *= 1.4;
			break;
		case WEAPON_AWP:
			flRatio *= 1.95;
			break;
		case WEAPON_UMP45:	// Counter-Strike 1.4: a slot of the jump table with the three other cases of 1 (its place in the text is ours)
		case WEAPON_MP5N:	// Beta 4.0 (Beta 4.1: 0.9; Beta 5.0 leak: 1)
			flRatio *= 1;
			break;
		case WEAPON_G3SG1:
			flRatio *= 1.65;
			break;
		case WEAPON_SG550:
			flRatio *= 1.45;
			break;
		case WEAPON_M249:
		case WEAPON_DEAGLE:
		case WEAPON_FIVESEVEN:
		case WEAPON_P90:
			flRatio *= 1.5;
			break;
		case WEAPON_AK47:	// Beta 6.0 (Beta 5.2: 1.65)
			flRatio *= 1.55;
			break;
		case WEAPON_GLOCK18:	// Counter-Strike 1.4: behind the AK-47's case (the order of retail's blocks)
		case WEAPON_ELITE:	// Beta 7.0
			flRatio *= 1.05;
			break;
		case WEAPON_MAC10:	// Beta 6.0
			flRatio *= 0.95;
			break;
		case WEAPON_P228:	// Beta 4.0 (Beta 4.1: 1.3, was 1.4; Beta 5.0 leak: 1.4 again; Beta 6.0: 1.25)
			flRatio *= 1.25;
			break;
		case WEAPON_KNIFE:	// Beta 3.1
		case WEAPON_SCOUT:	// Beta 4.0 (Beta 6.0: 1.7, was 1.6)
			flRatio *= 1.7;
			break;
		}
		}

		// Beta 7.0: a standing player hit by a rifle, a shotgun or the Desert
		// Eagle is pushed away from the attacker by 170 units a second (while
		// he moves slower than 300) and slowed to 65%, with the large flinch;
		// any other hit is 6.6's: slowed to half (Beta 5.2: 0.35), a heavy hit
		// being more than 60 to the head or more than 20 anywhere else.
		// Counter-Strike 1.4: ShouldDoLargeFlinch; not for a hit in a leg
		if ( ShouldDoLargeFlinch( m_LastHitGroup, iGunType ) )
		{
			if ( pev->velocity.Length() < 300 )
			{
				Vector attack_velocity;	// Counter-Strike 1.6's name

				attack_velocity = pev->origin - pAttack->pev->origin;
				attack_velocity = attack_velocity.Normalize();
				m_flVelocityModifier = 0.65;
				attack_velocity = pev->velocity + attack_velocity * 170;
				pev->velocity = attack_velocity;
			}

			SetAnimation( PLAYER_LARGE_FLINCH );
		}
		else
		{
			m_flVelocityModifier = 0.5;

			if ( ( m_LastHitGroup == HITGROUP_HEAD && flDamage > 60 ) || ( m_LastHitGroup != HITGROUP_HEAD && flDamage > 20 ) )
				m_fHeavyDamage = TRUE;
			else
				m_fHeavyDamage = FALSE;

			SetAnimation( PLAYER_FLINCH );
		}
	}

	// keep track of amount of damage last sustained
	m_lastDamageAmount = flDamage;

	// Armor: kevlar covers the body and the arms, kevlar and helmet the head too;
	// a hit anywhere sounds like armour while there is armour left
	if ( pev->armorvalue != 0.0f && !(bitsDamageType & (DMG_FALL | DMG_DROWN)) && IsArmored( m_LastHitGroup ) )// armor doesn't protect against fall or drown damage!
	{
		float flNew = flDamage * flRatio;
		float flArmor;

		flArmor = (flDamage - flNew) * flBonus;

		// Does this use more armor than we have?
		if (flArmor > pev->armorvalue)
		{
			armorHit = flArmor;
			flArmor = pev->armorvalue;
			flArmor *= (1/flBonus);
			flNew = flDamage - flArmor;
			pev->armorvalue = 0;
		}
		else
		{
			int oldValue = pev->armorvalue;

			pev->armorvalue -= flArmor;
			armorHit = oldValue - (int)pev->armorvalue;
		}

		flDamage = flNew;

		if ( pev->armorvalue <= 0.0 )
			m_iKevlar = 0;

		Pain( m_LastHitGroup, true );
	}
	else
		Pain( m_LastHitGroup, false );

	LogAttack( pAttack, this, teamAttack, (int)flDamage, armorHit, (int)pev->health - (int)flDamage, (int)pev->armorvalue, GetWeaponName( pevInflictor, pevAttacker ) );

	// this cast to INT is critical!!! If a player ends up with 0.5 health, the engine will get that
	// as an int (zero) and think the player is dead! (this will incite a clientside screentilt, etc)
	fTookDamage = CBaseMonster::TakeDamage(pevInflictor, pevAttacker, (int)flDamage, bitsDamageType);

	// reset damage time countdown for each type of time based damage player just sustained

	{
		for (int i = 0; i < CDMG_TIMEBASED; i++)
			if (bitsDamageType & (DMG_PARALYZE << i))
				m_rgbTimeBasedDamage[i] = 0;
	}

	// tell director about it
	MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
		WRITE_BYTE ( 9 );	// command length in bytes
		WRITE_BYTE ( DRC_CMD_EVENT );	// take damage event
		WRITE_SHORT( ENTINDEX(this->edict()) );	// index number of primary entity
		WRITE_SHORT( ENTINDEX(ENT(pevInflictor)) );	// index number of secondary entity
		WRITE_LONG( 5 );   // eventflags (priority and flags)
	MESSAGE_END();


	m_bitsDamageType |= bitsDamageType; // Save this so we can report it to the client
	m_bitsHUDDamage = -1;  // make sure the damage bits get resent

	return fTookDamage;
}

//=========================================================
// PackDeadPlayerItems - Beta 5.0 (leak): the dead player drops
// his heaviest weapon of the first three HUD slots (primary,
// pistol, knife: the knife cannot be dropped otherwise) in a
// weaponbox with that weapon's model, three quarters of his
// velocity, gone after five minutes, with all his ammunition
// of its kind unless the rules keep the ammunition; the rest
// is removed. Nothing is dropped where the rules say so.
//=========================================================
void CBasePlayer::PackDeadPlayerItems( void )
{
	bool bPackGun = ( g_pGameRules->DeadPlayerWeapons( this ) != GR_PLR_DROP_GUN_NO );	// names ours
	bool bPackAmmo = ( g_pGameRules->DeadPlayerAmmo( this ) != GR_PLR_DROP_AMMO_NO );

	if ( !bPackGun )
	{
		RemoveAllItems( TRUE );
		return;
	}

	int nBestWeight = 0;	// names ours
	CBasePlayerItem *pBestItem = NULL;

	for ( int n = 0; n < MAX_ITEM_TYPES; n++ )
	{
		CBasePlayerItem *pPlayerItem = m_rgpPlayerItems[ n ];

		while ( pPlayerItem )
		{
			ItemInfo II;

			if ( pPlayerItem->iItemSlot() < 3 && pPlayerItem->GetItemInfo( &II ) )
			{
				if ( II.iWeight > nBestWeight )
				{
					nBestWeight = II.iWeight;
					pBestItem = pPlayerItem;
				}
			}

			pPlayerItem = pPlayerItem->m_pNext;
		}
	}

	if ( pBestItem )
	{
		char *modelName = GetCSModelName( pBestItem->m_iId );

		if ( modelName )
		{
			CWeaponBox *pWeaponBox = (CWeaponBox *)CBaseEntity::Create( "weaponbox", pev->origin, pev->angles, edict() );

			pWeaponBox->pev->angles.x = 0;// don't let weaponbox tilt.
			pWeaponBox->pev->angles.z = 0;
			pWeaponBox->pev->velocity = pev->velocity * 0.75;
			pWeaponBox->SetThink( &CWeaponBox::Kill );
			pWeaponBox->pev->nextthink = gpGlobals->time + 300;
			pWeaponBox->PackWeapon( pBestItem );

			if ( bPackAmmo )
				pWeaponBox->PackAmmo( MAKE_STRING( pBestItem->pszAmmo1() ), m_rgAmmo[ pBestItem->PrimaryAmmoIndex() ] );

			SET_MODEL( ENT( pWeaponBox->pev ), modelName );
		}
	}

	RemoveAllItems( TRUE );// now strip off everything that wasn't handled by the code above.
}

// Beta 5.0 (Linux symbol): everything goes, and the player gets his team's
// knife and pistol with ammunition (CHalfLifeMultiplay::PlayerSpawn for a
// player who did not live through the round, and a complete reset)
void CBasePlayer::GiveDefaultItems( void )
{
	RemoveAllItems( FALSE );
	m_bHasPrimary = false;	// Beta 6.0

	if ( m_iTeam == TEAM_CT )
	{
		GiveNamedItem( "weapon_knife" );
		GiveNamedItem( "weapon_usp" );
		// Counter-Strike 1.4: the VIP gets one magazine less; the limit is 100 (48)
		if ( !m_bIsVIP )
			GiveAmmo( 24, "45acp", 100 );
		else
			GiveAmmo( 12, "45acp", 100 );
	}
	else if ( m_iTeam == TEAM_TERRORIST )
	{
		GiveNamedItem( "weapon_knife" );
		GiveNamedItem( "weapon_glock18" );
		GiveAmmo( 40, "9mm", 120 );
	}
}

void CBasePlayer::RemoveAllItems( BOOL removeSuit )
{
	// Counter-Strike Beta 5.0: the defuse kit and the C4 go too, with their
	// icons and the model's pack, and a progress bar running stops
	BOOL bKillProgBar = FALSE;	// name ours

	if ( m_fHasDefuser )
	{
		m_fHasDefuser = false;
		pev->body = 0;

		MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_STRING( "defuser" );
		MESSAGE_END();

		bKillProgBar = TRUE;
	}

	if ( m_bHasC4 )
	{
		m_bHasC4 = false;
		pev->body = 0;

		MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_STRING( "c4" );
		MESSAGE_END();

		bKillProgBar = TRUE;
	}

	if ( bKillProgBar )
		SetProgressBarTime( 0 );	// Counter-Strike 1.4

	if (m_pActiveItem)
	{
		ResetAutoaim( );
		m_pActiveItem->Holster( );
		m_pActiveItem = NULL;
	}

	m_pLastItem = NULL;

	int i;
	CBasePlayerItem *pPendingItem;
	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		m_pActiveItem = m_rgpPlayerItems[i];
		while (m_pActiveItem)
		{
			pPendingItem = m_pActiveItem->m_pNext; 
			m_pActiveItem->Drop( );
			m_pActiveItem = pPendingItem;
		}
		m_rgpPlayerItems[i] = NULL;
	}
	m_pActiveItem = NULL;

	pev->viewmodel		= 0;
	pev->weaponmodel	= 0;
	
	if ( removeSuit )
		pev->weapons = 0;
	else
		pev->weapons &= ~WEAPON_ALLWEAPONS;

	for ( i = 0; i < MAX_AMMO_SLOTS;i++)
		m_rgAmmo[i] = 0;

	UpdateClientData();
	// send Selected Weapon Message to our client
	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
		WRITE_BYTE(0);
		WRITE_BYTE(0);
		WRITE_BYTE(0);
	MESSAGE_END();
}

/*
 * GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
 *
 * ENTITY_METHOD(PlayerDie)
 */
entvars_t *g_pevLastInflictor;  // Set in combat.cpp.  Used to pass the damage inflictor for death messages.
								// Better solution:  Add as parameter to all Killed() functions.

//=========================================================
// Beta 5.0 (leak): the bomb icon on the HUD while the player
// carries the C4 (flashing in a bomb target), its scoreboard
// mark (SetBombIcon is a Linux symbol)
//=========================================================
void CBasePlayer::SetBombIcon( BOOL bFlash )
{
	if ( m_bHasC4 )
	{
		// Beta 5.0: green (the leak: blue, 15 15 200)
		MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
			WRITE_BYTE( bFlash ? 2 : 1 );
			WRITE_STRING( "c4" );
			WRITE_BYTE( 0 );
			WRITE_BYTE( 160 );
			WRITE_BYTE( 0 );
		MESSAGE_END();
	}
	else
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_STRING( "c4" );
		MESSAGE_END();
	}

	SetScoreboardAttributes();
}

// Counter-Strike 1.4: the progress bar (BarTime), for the player and for
// every observer who watches through his eyes
void CBasePlayer::SetProgressBarTime( int time )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgBarTime, NULL, pev );
		WRITE_SHORT( time );
	MESSAGE_END();

	CBaseEntity *pPlayer = NULL;
	pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	int myIndex = entindex();

	while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
	{
		CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

		if ( player->pev->iuser1 == OBS_IN_EYE && player->pev->iuser2 == myIndex )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgBarTime, NULL, player->pev );
				WRITE_SHORT( time );
			MESSAGE_END();
		}

		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	}
}

// Beta 5.0 (leak): the buy zone's icon goes, and with it an open buy menu
// (name ours). It and the other zone helpers are inline functions: the Linux
// build has no copy of them (Beta 5.0's), while it has one of the small
// static CheckPowerups; VC6 (/Ob0) emits them after their first user.
inline void BuyZoneIcon_Clear( CBasePlayer *player )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, player->pev );
		WRITE_BYTE( 0 );
		WRITE_STRING( "buyzone" );
	MESSAGE_END();

	if ( player->m_iMenu >= Menu_Buy && player->m_iMenu <= Menu_BuyItem )
		CLIENT_COMMAND( player->edict(), "slot10\n" );
}

void CBasePlayer::Killed( entvars_t *pevAttacker, int iGib )
{
	CSound *pSound;

	// Beta 7.1: killed by a hit to the head (DeathNotice reads it)
	if ( m_LastHitGroup == HITGROUP_HEAD )
		m_bHeadshotKilled = true;

	// Beta 4.0: a blast death (the C4) is not reported to the rules
	if ( m_fKilledByBlast == false )
		g_pGameRules->PlayerKilled( this, pevAttacker, g_pevLastInflictor );

	// Beta 6.0: the night vision goes off (Beta 6.5: Beta 6.1's "lambert" command is gone)
	MESSAGE_BEGIN( MSG_ONE, gmsgNVGToggle, NULL, pev );
		WRITE_BYTE( 0 );
	MESSAGE_END();

	m_bNightVisionOn = false;

	if ( m_pTank != NULL )
	{
		m_pTank->Use( this, this, USE_OFF, 0 );
		m_pTank = NULL;
	}

	// this client isn't going to be thinking for a while, so reset the sound until they respawn
	pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( edict() ) );
	{
		if ( pSound )
		{
			pSound->Reset();
		}
	}

	SetAnimation( PLAYER_DIE );

	// Counter-Strike 1.3: a grenade held with the pin out (the attack button down) leaves his hand.
	// Retail passes his angles for the throw's velocity.
	// Counter-Strike 1.4: only while he has one left
	if ( m_pActiveItem && m_pActiveItem->m_pPlayer )
	{
		if ( m_pActiveItem->m_iId == WEAPON_HEGRENADE )
		{
			if ( ( pev->button & IN_ATTACK ) && m_rgAmmo[ ((CBasePlayerWeapon *)m_pActiveItem)->m_iPrimaryAmmoType ] )
				CGrenade::ShootTimed2( pev, pev->origin + pev->view_ofs, pev->angles, 1.5, m_iTeam );
		}
		else if ( m_pActiveItem->m_iId == WEAPON_FLASHBANG )
		{
			if ( ( pev->button & IN_ATTACK ) && m_rgAmmo[ ((CBasePlayerWeapon *)m_pActiveItem)->m_iPrimaryAmmoType ] )
				CGrenade::ShootTimed( pev, pev->origin + pev->view_ofs, pev->angles, 1.5 );
		}
		else if ( m_pActiveItem->m_iId == WEAPON_SMOKEGRENADE )
		{
			CSmokeGrenade *pSmoke = (CSmokeGrenade *)m_pActiveItem;	// Counter-Strike 1.6's name

			if ( ( pev->button & IN_ATTACK ) && m_rgAmmo[ ((CBasePlayerWeapon *)m_pActiveItem)->m_iPrimaryAmmoType ] )
				CGrenade::ShootSmokeGrenade( pev, pev->origin + pev->view_ofs, pev->angles, 1.5, pSmoke->m_usCreateSmoke );
		}
	}

	pev->modelindex = g_ulModelIndexPlayer;    // don't use eyes

	pev->deadflag		= DEAD_DYING;
	pev->movetype		= MOVETYPE_TOSS;
	pev->takedamage		= DAMAGE_NO;	// Beta 6.5
	pev->flags &= ~FL_ONGROUND;

	// Counter-Strike 1.4: the death camera: the dead player watches his own body in the free
	// chase mode, and iuser3 names his killer (pm_shared and the client read it)
	pev->iuser1 = OBS_CHASE_FREE;
	pev->iuser2 = ENTINDEX( edict() );
	pev->iuser3 = ENTINDEX( ENT( pevAttacker ) );
	m_hObserverTarget = UTIL_PlayerByIndex( pev->iuser3 );

	// Beta 6.5: the scoreboard marks him dead at once (Beta 5.0 (leak)'s
	// "Throw Direction" console line is gone)
	SetScoreboardAttributes();

	// throw the body as the death animation says
	if ( m_iDeathThrow )
	{
		switch ( m_iDeathThrow )
		{
		case 1:		// forwards
			UTIL_MakeVectors( pev->angles );
			pev->velocity = gpGlobals->v_forward * RANDOM_FLOAT( 100, 200 );
			pev->velocity.z = RANDOM_FLOAT( 50, 100 );
			break;
		case 2:		// backwards
			UTIL_MakeVectors( pev->angles );
			pev->velocity = gpGlobals->v_forward * RANDOM_FLOAT( -100, -200 );
			pev->velocity.z = RANDOM_FLOAT( 50, 100 );
			break;
		case 3:		// away from the attacker (Beta 3.0: only a player attacker)
			// (FClassnameIs here and in case 6, not strcmp: the same code, and its inline expansions
			// give the second Length() of case 4 retail's x87 order under the pack's /Ob2)
			if ( FClassnameIs( pevAttacker, "player" ) )
			{
				UTIL_MakeVectors( pevAttacker->angles );
				pev->velocity = gpGlobals->v_forward * RANDOM_FLOAT( 200, 300 );
				pev->velocity.z = RANDOM_FLOAT( 200, 300 );
			}
			break;
		case 4:		// Beta 4.0: blown away by the C4, the harder the nearer
			pev->velocity = ( m_vecBlastDir / m_vecBlastDir.Length() ) * ( 2300 - m_vecBlastDir.Length() ) / 4;
			pev->velocity.z = ( 2300 - m_vecBlastDir.Length() ) / 2.75;
			break;
		case 5:		// Beta 4.0: blown away by an HE grenade
			pev->velocity = ( m_vecBlastDir / m_vecBlastDir.Length() ) * ( 500 - m_vecBlastDir.Length() );
			pev->velocity.z = ( 350 - m_vecBlastDir.Length() ) * 1.5;
			break;
		case 6:		// Beta 7.0: as 3, without the lift
			if ( FClassnameIs( pevAttacker, "player" ) )
			{
				UTIL_MakeVectors( pevAttacker->angles );
				pev->velocity = gpGlobals->v_forward * RANDOM_FLOAT( 200, 300 );
			}
			break;
		}

		m_iDeathThrow = 0;
	}

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate(NULL, FALSE, 0);

	// send "health" update message to zero
	m_iClientHealth = 0;
	MESSAGE_BEGIN( MSG_ONE, gmsgHealth, NULL, pev );
		WRITE_BYTE( m_iClientHealth );
	MESSAGE_END();

	// Tell Ammo Hud that the player is dead
	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
		WRITE_BYTE(0);
		WRITE_BYTE(0XFF);
		WRITE_BYTE(0xFF);
	MESSAGE_END();

	// reset FOV
	m_iFOV = m_iClientFOV = 0;

	MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
		WRITE_BYTE(0);
	MESSAGE_END();

	((CHalfLifeMultiplay *)g_pGameRules)->CheckWinConditions();

	// a dead player starts the next round with nothing
	m_fNotKilled = FALSE;

	// Beta 5.0 (leak): the C4 is dropped as a weapon; a defuse kit falls
	// where he died (item_thighpack); the progress bar and the icons go
	if ( m_bHasC4 == true )
	{
		DropPlayerItem( "weapon_c4" );
		SetProgressBarTime( 0 );
	}
	else if ( m_fHasDefuser == true )
	{
		m_fHasDefuser = false;
		pev->body = 0;
		GiveNamedItem( "item_thighpack" );

		MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_STRING( "defuser" );
		MESSAGE_END();

		SetProgressBarTime( 0 );
	}

	m_bIsDefusing = false;	// Counter-Strike 1.4 (Beta 6.0 to 1.3: m_bLeadsHostage)
	BuyZoneIcon_Clear( this );

	SetThink(PlayerDeathThink);
	pev->nextthink = gpGlobals->time + 0.1;
	pev->solid = SOLID_NOT;	// Counter-Strike 1.1

	// (Counter-Strike 1.1 to 1.3 blacked the screen out here when mp_fadetoblack is on)

	// Counter-Strike 1.1: the team killer killed at the round's start is told why
	if ( m_bPunishedForTK )
	{
		m_bPunishedForTK = false;
		HintMessage( "#Hint_cannot_play_because_tk", TRUE, TRUE );
	}

	// UNDONE: Put this in, but add FFADE_PERMANENT and make fade time 8.8 instead of 4.12
	// UTIL_ScreenFade( edict(), Vector(128,0,0), 6, 15, 255, FFADE_OUT | FFADE_MODULATE );

	// Beta 4.0: gibbed only below -9000 health (was -40)
	if ( ( pev->health < -9000 && iGib != GIB_NEVER ) || iGib == GIB_ALWAYS )
	{
		pev->solid			= SOLID_NOT;
		GibMonster();	// This clears pev->model
		pev->effects |= EF_NODRAW;
		((CHalfLifeMultiplay *)g_pGameRules)->CheckWinConditions();
		return;
	}

	DeathSound();
	
	pev->angles.x = 0;
	pev->angles.z = 0;

	// (Beta 5.0 to 1.3 cleared m_fTeamChanged here: a dead player could change teams once more)
}

// Counter-Strike 1.4 (Linux symbol): whether the player carries the bomb
BOOL CBasePlayer::IsBombGuy( void )
{
	if ( !g_pGameRules->IsMultiplayer() )
		return FALSE;

	return m_bHasC4;
}


// Set the activity based on an event or current state
// Counter-Strike Beta 7.0: the second attack (ref_shoot2_/crouch_shoot2_) and
// the reload (reload_<weapon>) are animated like the first attack; a shot
// stamps m_flLastFired, and a player running who has not shot for four
// seconds gets run_<weapon> (activity ACT_RUN) where the model has it.
// Counter-Strike 1.0, for the new player models (nine-way blending in the
// client): jumps are animated (the gait is the hop or leap sequence while the
// body aims, shoots or reloads; it stays until another activity resets it),
// running starts at 135 (Beta 7.1: 190), the flinches are two sequences
// (head_flinch, gut_flinch) and no longer timed here, the deaths go by
// activity (ACT_DIE_HEADSHOT, ...) and four sequences (left, right,
// crouch_die), holding the armed C4 (PLAYER_HOLDBOMB) aims, a standing
// player's gait is ACT_IDLE (deep_idle).
void CBasePlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	int animDesired;
	float speed;
	char szAnim[64];
	int hopSeq, leapSeq;	// Counter-Strike 1.6's names

	// Beta 6.0: nothing to animate without a model (a player who has disappeared)
	if ( !pev->modelindex )
		return;

	// a flinch plays to its end, unless a new flinch comes or the player is dead
	if ( playerAnim != PLAYER_FLINCH && playerAnim != PLAYER_LARGE_FLINCH && m_flFlinchTime >= gpGlobals->time && pev->health > 0.0 )
		return;

	speed = pev->velocity.Length2D();

	if (pev->flags & FL_FROZEN)
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

	hopSeq = LookupActivity( ACT_HOP );
	leapSeq = LookupActivity( ACT_LEAP );

	switch (playerAnim) 
	{
	case PLAYER_JUMP:
		switch( m_Activity )
		{
		case ACT_HOVER:
		case ACT_SWIM:
		case ACT_DIESIMPLE:
			m_IdealActivity = m_Activity;
			break;
		default:
			m_IdealActivity = ACT_HOP;
			break;
		}
		break;
	
	case PLAYER_SUPERJUMP:
		switch( m_Activity )
		{
		case ACT_HOVER:
		case ACT_SWIM:
		case ACT_DIESIMPLE:
			m_IdealActivity = m_Activity;
			break;
		default:
			m_IdealActivity = ACT_LEAP;
			break;
		}
		break;
	
	case PLAYER_DIE:
		m_IdealActivity = ACT_DIESIMPLE;
		DeathSound();
		break;

	case PLAYER_ATTACK1:	
		switch( m_Activity )
		{
		case ACT_HOVER:
		case ACT_SWIM:
		case ACT_DIESIMPLE:
			m_IdealActivity = m_Activity;
			break;
		default:
			m_IdealActivity = ACT_RANGE_ATTACK1;
			break;
		}
		break;

	// Beta 7.0: the Elites' second gun
	case PLAYER_ATTACK2:
		switch( m_Activity )
		{
		case ACT_HOVER:
		case ACT_SWIM:
		case ACT_DIESIMPLE:
			m_IdealActivity = m_Activity;
			break;
		default:
			m_IdealActivity = ACT_RANGE_ATTACK2;
			break;
		}
		break;

	// Beta 7.0: reloading
	case PLAYER_RELOAD:
		switch( m_Activity )
		{
		case ACT_HOVER:
		case ACT_SWIM:
		case ACT_DIESIMPLE:
			m_IdealActivity = m_Activity;
			break;
		default:
			m_IdealActivity = ACT_RELOAD;
			break;
		}
		break;

	case PLAYER_IDLE:
	case PLAYER_WALK:
		if ( !FBitSet( pev->flags, FL_ONGROUND ) && (m_Activity == ACT_HOP || m_Activity == ACT_LEAP) )	// Still jumping
		{
			m_IdealActivity = m_Activity;
		}
		else if ( pev->waterlevel > 1 )
		{
			if ( speed == 0 )
				m_IdealActivity = ACT_HOVER;
			else
				m_IdealActivity = ACT_SWIM;
		}
		else
		{
			m_IdealActivity = ACT_WALK;
		}
		break;

	// Counter-Strike 1.0: the C4 while it is armed
	case PLAYER_HOLDBOMB:
		m_IdealActivity = ACT_HOLDBOMB;
		break;

	case PLAYER_FLINCH:
		m_IdealActivity = ACT_FLINCH;
		break;

	// Beta 7.0: the flinch of a hit that pushes the player (TakeDamage)
	case PLAYER_LARGE_FLINCH:
		m_IdealActivity = ACT_LARGE_FLINCH;
		break;
	}

	switch (m_IdealActivity)
	{
	// Counter-Strike 1.0: a jump keeps the upper body's animation and sets
	// the gait
	case ACT_HOP:
	case ACT_LEAP:
		if ( m_Activity == m_IdealActivity)
			return;

		switch ( m_Activity )
		{
		case ACT_RANGE_ATTACK1:
			strcpy( szAnim, "ref_shoot_" );
			break;
		case ACT_RANGE_ATTACK2:
			strcpy( szAnim, "ref_shoot2_" );
			break;
		case ACT_RELOAD:
			strcpy( szAnim, "ref_reload_" );
			break;
		default:
			strcpy( szAnim, "ref_aim_" );
			break;
		}
		strcat( szAnim, m_szAnimExtention );
		animDesired = LookupSequence( szAnim );
		if (animDesired == -1)
			animDesired = 0;

		if ( pev->sequence != animDesired || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		if (!m_fSequenceLoops)
		{
			pev->effects |= EF_NOINTERP;
		}

		if ( m_IdealActivity == ACT_LEAP )
			pev->gaitsequence = LookupActivity( ACT_LEAP );
		else
			pev->gaitsequence = LookupActivity( ACT_HOP );

		m_Activity = m_IdealActivity;
		break;

	case ACT_RANGE_ATTACK1:
		m_flLastFired = gpGlobals->time;	// Beta 7.0
		if ( FBitSet( pev->flags, FL_DUCKING ) )	// crouching
			strcpy( szAnim, "crouch_shoot_" );
		else
			strcpy( szAnim, "ref_shoot_" );
		strcat( szAnim, m_szAnimExtention );
		animDesired = LookupSequence( szAnim );
		if (animDesired == -1)
			animDesired = 0;

		pev->sequence		= animDesired;
		pev->frame			= 0;
		ResetSequenceInfo( );

		m_Activity = m_IdealActivity;
		break;

	// Beta 7.0: the second gun
	case ACT_RANGE_ATTACK2:
		m_flLastFired = gpGlobals->time;	// Beta 7.0
		if ( FBitSet( pev->flags, FL_DUCKING ) )	// crouching
			strcpy( szAnim, "crouch_shoot2_" );
		else
			strcpy( szAnim, "ref_shoot2_" );
		strcat( szAnim, m_szAnimExtention );
		animDesired = LookupSequence( szAnim );
		if (animDesired == -1)
			animDesired = 0;

		pev->sequence		= animDesired;
		pev->frame			= 0;
		ResetSequenceInfo( );

		m_Activity = m_IdealActivity;
		break;

	// Beta 7.0: reload_<weapon>. Counter-Strike 1.0: ref_reload_<weapon>,
	// crouch_reload_<weapon>
	case ACT_RELOAD:
		if ( FBitSet( pev->flags, FL_DUCKING ) )	// crouching
			strcpy( szAnim, "crouch_reload_" );
		else
			strcpy( szAnim, "ref_reload_" );
		strcat( szAnim, m_szAnimExtention );
		animDesired = LookupSequence( szAnim );
		if (animDesired == -1)
			animDesired = 0;

		if ( pev->sequence != animDesired || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		if (!m_fSequenceLoops)
		{
			pev->effects |= EF_NOINTERP;
		}

		m_Activity = m_IdealActivity;
		break;

	// Counter-Strike 1.0
	case ACT_HOLDBOMB:
		if ( FBitSet( pev->flags, FL_DUCKING ) )	// crouching
			strcpy( szAnim, "crouch_aim_" );
		else
			strcpy( szAnim, "ref_aim_" );
		strcat( szAnim, m_szAnimExtention );
		animDesired = LookupSequence( szAnim );
		if (animDesired == -1)
			animDesired = 0;

		m_Activity = m_IdealActivity;
		break;

	case ACT_WALK:
		if ( ( m_Activity != ACT_RANGE_ATTACK1 || m_fSequenceFinished ) &&
			 ( m_Activity != ACT_RANGE_ATTACK2 || m_fSequenceFinished ) &&
			 ( m_Activity != ACT_FLINCH || m_fSequenceFinished ) &&
			 ( m_Activity != ACT_LARGE_FLINCH || m_fSequenceFinished ) &&
			 ( m_Activity != ACT_RELOAD || m_fSequenceFinished ) )
		{
			// Beta 7.0: running, four seconds after the last shot: run_<weapon>
			// for the whole body where the model has it
			if ( speed > 135 && m_flLastFired + 4 < gpGlobals->time )
			{
				strcpy( szAnim, "run_" );
				strcat( szAnim, m_szAnimExtention );
				animDesired = LookupSequence( szAnim );
				if ( animDesired == -1 )
				{
					if ( FBitSet( pev->flags, FL_DUCKING ) )	// crouching
						strcpy( szAnim, "crouch_aim_" );
					else
						strcpy( szAnim, "ref_aim_" );
					strcat( szAnim, m_szAnimExtention );
					animDesired = LookupSequence( szAnim );
					if (animDesired == -1)
						animDesired = 0;
					m_Activity = ACT_RUN;
					pev->gaitsequence = LookupActivity( ACT_RUN );
				}
				else
				{
					m_Activity = ACT_RUN;
					pev->gaitsequence = animDesired;
				}
			}
			else
			{
				if ( FBitSet( pev->flags, FL_DUCKING ) )	// crouching
					strcpy( szAnim, "crouch_aim_" );
				else
					strcpy( szAnim, "ref_aim_" );
				strcat( szAnim, m_szAnimExtention );
				animDesired = LookupSequence( szAnim );
				if (animDesired == -1)
					animDesired = 0;
				m_Activity = ACT_WALK;
			}
		}
		else
		{
			animDesired = pev->sequence;
		}

		// Counter-Strike 1.0 (sic: the run branch's gait is overwritten)
		if ( speed > 135 )
			pev->gaitsequence = LookupActivity( ACT_RUN );
		else
			pev->gaitsequence = LookupActivity( ACT_WALK );
		break;

	// Counter-Strike 1.0: one of two sequences, by the hit's place
	case ACT_FLINCH:
	case ACT_LARGE_FLINCH:
		m_Activity = m_IdealActivity;

		switch ( m_LastHitGroup )
		{
		case HITGROUP_GENERIC:
			if ( RANDOM_LONG( 0, 1 ) )
				animDesired = LookupSequence( "head_flinch" );
			else
				animDesired = LookupSequence( "gut_flinch" );
			break;

		case HITGROUP_HEAD:
		case HITGROUP_CHEST:
			animDesired = LookupSequence( "head_flinch" );
			break;

		case HITGROUP_STOMACH:
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			animDesired = LookupSequence( "gut_flinch" );
			break;

		default:
			animDesired = LookupSequence( "gut_flinch" );
			break;
		}

		if (animDesired == -1)
			animDesired = 0;
		break;

	case ACT_DIESIMPLE:
		if ( m_Activity == m_IdealActivity)
			return;
		m_Activity = m_IdealActivity;

		m_iUnused15C = 0;
		m_iDeathThrow = 0;

		// the death animation goes by where the last hit was. Counter-Strike
		// 1.0: by activity (the model's weights choose among its sequences)
		switch ( m_LastHitGroup )
		{
		case HITGROUP_GENERIC:
			switch ( RANDOM_LONG( 0, 8 ) )
			{
			case 0:
				animDesired = LookupActivity( ACT_DIE_HEADSHOT );
				m_iDeathThrow = 2;
				break;
			case 1:
				animDesired = LookupActivity( ACT_DIE_GUTSHOT );
				break;
			case 2:
				animDesired = LookupActivity( ACT_DIE_BACKSHOT );
				m_iDeathThrow = 3;
				break;
			case 3:
				animDesired = LookupActivity( ACT_DIESIMPLE );
				break;
			case 4:
				animDesired = LookupActivity( ACT_DIEBACKWARD );
				m_iDeathThrow = 3;
				break;
			case 5:
				animDesired = LookupActivity( ACT_DIEFORWARD );
				m_iDeathThrow = 1;
				break;
			case 6:
				animDesired = LookupActivity( ACT_DIE_CHESTSHOT );
				break;
			case 7:
				animDesired = LookupActivity( ACT_DIE_GUTSHOT );
				break;
			case 8:
				animDesired = LookupActivity( ACT_DIE_HEADSHOT );
				break;
			default:
				animDesired = LookupActivity( ACT_DIESIMPLE );
				break;
			}
			break;

		case HITGROUP_HEAD:
			{
			int iRand = RANDOM_LONG( 0, 8 );

			m_bHeadshotKilled = true;	// Beta 7.1

			if ( m_fHeavyDamage == TRUE )
				iRand += 1;	// Beta 2.0 (Beta 1.0: 2)

			if ( iRand == 1 || iRand == 2 )
			{
				m_iDeathThrow = 2;
				animDesired = LookupActivity( ACT_DIE_HEADSHOT );
			}
			else if ( iRand == 3 || iRand == 4 )
			{
				m_iDeathThrow = 1;
				animDesired = LookupActivity( ACT_DIE_HEADSHOT );
			}
			else if ( iRand == 5 || iRand == 6 )
			{
				m_iDeathThrow = 3;
				animDesired = LookupActivity( ACT_DIE_HEADSHOT );
			}
			else
			{
				m_iDeathThrow = 0;
				animDesired = LookupActivity( ACT_DIE_HEADSHOT );
			}
			}
			break;

		case HITGROUP_CHEST:
			animDesired = LookupActivity( ACT_DIE_CHESTSHOT );
			break;

		case HITGROUP_STOMACH:
			animDesired = LookupActivity( ACT_DIE_GUTSHOT );
			break;

		case HITGROUP_LEFTARM:
			animDesired = LookupSequence( "left" );
			break;

		case HITGROUP_RIGHTARM:
			m_iDeathThrow = RANDOM_LONG( 0, 1 ) ? 3 : 6;
			animDesired = LookupSequence( "right" );
			break;

		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			animDesired = LookupActivity( ACT_DIESIMPLE );
			break;

		default:
			animDesired = LookupActivity( ACT_DIESIMPLE );
			break;
		}

		if ( FBitSet( pev->flags, FL_DUCKING ) )
		{
			animDesired = LookupSequence( "crouch_die" );
			m_iDeathThrow = 2;
		}
		else if ( m_fKilledByBlast == true || m_fKilledByHEGrenade == true )
		{
			// Beta 4.0: a blast throws the body (Killed: 4 the C4's, 5 an HE grenade's)
			UTIL_MakeVectors( pev->angles );

			float flDot = DotProduct( gpGlobals->v_forward, m_vecBlastDir );	// Counter-Strike 1.6's name; the local gives retail 1.4's x87 order

			if ( flDot > 0 )
				animDesired = LookupSequence( "left" );
			else if ( RANDOM_LONG( 0, 1 ) )
				animDesired = LookupSequence( "crouch_die" );
			else
				animDesired = LookupActivity( ACT_DIE_HEADSHOT );

			if ( m_fKilledByBlast == true )
				m_iDeathThrow = 4;
			else if ( m_fKilledByHEGrenade == true )
				m_iDeathThrow = 5;
		}

		if (animDesired == -1)
			animDesired = 0;

		// Already using the desired animation?
		if (pev->sequence == animDesired)
			return;

		pev->gaitsequence = 0;
		pev->sequence		= animDesired;
		pev->frame			= 0;
		ResetSequenceInfo( );
		return;

	case ACT_HOVER:
	case ACT_SWIM:
	default:
		if ( m_Activity == m_IdealActivity)
			return;
		m_Activity = m_IdealActivity;

		animDesired = LookupActivity( m_Activity );
		// Already using the desired animation?
		if (pev->sequence == animDesired)
			return;

		pev->gaitsequence = 0;
		pev->sequence		= animDesired;
		pev->frame			= 0;
		ResetSequenceInfo( );
		return;
	}

	// Counter-Strike 1.0: a jump's gait stays
	if ( pev->gaitsequence != hopSeq && pev->gaitsequence != leapSeq )
	{
		if ( FBitSet( pev->flags, FL_DUCKING ) )
		{
			if ( speed == 0)
			{
				pev->gaitsequence	= LookupActivity( ACT_CROUCHIDLE );
				// pev->gaitsequence	= LookupActivity( ACT_CROUCH );
			}
			else
			{
				pev->gaitsequence	= LookupActivity( ACT_CROUCH );
			}
		}
		else if ( speed > 135 )	// Beta 6.0: the run animation from 190 (Beta 5.2: 220); Counter-Strike 1.0: 135
		{
			// Beta 7.0: as in the ACT_WALK case above. Counter-Strike 1.0: not
			// while flinching (sic: no check of the second lookup's result)
			if ( m_flLastFired + 4 < gpGlobals->time && m_Activity != ACT_FLINCH && m_Activity != ACT_LARGE_FLINCH )
			{
				strcpy( szAnim, "run_" );
				strcat( szAnim, m_szAnimExtention );
				animDesired = LookupSequence( szAnim );
				if ( animDesired == -1 )
				{
					if ( FBitSet( pev->flags, FL_DUCKING ) )	// crouching
						strcpy( szAnim, "crouch_aim_" );
					else
						strcpy( szAnim, "ref_aim_" );
					strcat( szAnim, m_szAnimExtention );
					animDesired = LookupSequence( szAnim );
					m_Activity = ACT_RUN;
					pev->gaitsequence = LookupActivity( ACT_RUN );
				}
				else
				{
					m_Activity = ACT_RUN;
					pev->gaitsequence = animDesired;
				}
			}
			else
				pev->gaitsequence	= LookupActivity( ACT_RUN );
		}
		else if (speed > 0)
		{
			pev->gaitsequence	= LookupActivity( ACT_WALK );
		}
		else
		{
			// pev->gaitsequence	= LookupActivity( ACT_WALK );
			pev->gaitsequence	= LookupActivity( ACT_IDLE );
		}
	}


	// Already using the desired animation?
	if (pev->sequence == animDesired)
		return;

	//ALERT( at_console, "Set animation to %d\n", animDesired );
	// Reset to first frame of desired animation
	pev->sequence		= animDesired;
	pev->frame			= 0;
	ResetSequenceInfo( );
}


/*
===========
WaterMove
============
*/
#define AIRTIME	12		// lung full of air lasts this many seconds

void CBasePlayer::WaterMove()
{
	int air;

	if (pev->movetype == MOVETYPE_NOCLIP || pev->movetype == MOVETYPE_NONE)	// Counter-Strike 1.1: nor for a player who does not move (an observer)
		return;

	if (pev->health < 0)
		return;

	// waterlevel 0 - not in water
	// waterlevel 1 - feet in water
	// waterlevel 2 - waist in water
	// waterlevel 3 - head in water

	if (pev->waterlevel != 3) 
	{
		// not underwater
		
		// play 'up for air' sound
		if (pev->air_finished < gpGlobals->time)
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_wade1.wav", 1, ATTN_NORM);
		else if (pev->air_finished < gpGlobals->time + 9)
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_wade2.wav", 1, ATTN_NORM);

		pev->air_finished = gpGlobals->time + AIRTIME;
		pev->dmg = 2;

		// if we took drowning damage, give it back slowly
		if (m_idrowndmg > m_idrownrestored)
		{
			// set drowning damage bit.  hack - dmg_drownrecover actually
			// makes the time based damage code 'give back' health over time.
			// make sure counter is cleared so we start count correctly.
			
			// NOTE: this actually causes the count to continue restarting
			// until all drowning damage is healed.

			m_bitsDamageType |= DMG_DROWNRECOVER;
			m_bitsDamageType &= ~DMG_DROWN;
			m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
		}

	}
	else
	{	// fully under water
		// stop restoring damage while underwater
		m_bitsDamageType &= ~DMG_DROWNRECOVER;
		m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;

		if (pev->air_finished < gpGlobals->time)		// drown!
		{
			if (pev->pain_finished < gpGlobals->time)
			{
				// take drowning damage
				pev->dmg += 1;
				if (pev->dmg > 5)
					pev->dmg = 5;
				TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), pev->dmg, DMG_DROWN);
				pev->pain_finished = gpGlobals->time + 1;
				
				// track drowning damage, give it back when
				// player finally takes a breath

				m_idrowndmg += pev->dmg;
			} 
		}
		else
		{
			m_bitsDamageType &= ~DMG_DROWN;
		}
	}

	if (!pev->waterlevel)
	{
		if (FBitSet(pev->flags, FL_INWATER))
		{       
#if 0
			// play leave water sound
			switch (RANDOM_LONG(0,3))
			{
			case 0: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade1.wav", 1, ATTN_NORM); break;
			case 1: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade2.wav", 1, ATTN_NORM); break;
			case 2: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade3.wav", 1, ATTN_NORM); break;
			case 3: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade4.wav", 1, ATTN_NORM); break;
			}
#endif

			ClearBits(pev->flags, FL_INWATER);
		}
		return;
	}
	
	// make bubbles

	air = (int)(pev->air_finished - gpGlobals->time);
	if (!RANDOM_LONG(0,0x1f) && RANDOM_LONG(0,AIRTIME-1) >= air)
	{
		switch (RANDOM_LONG(0,3))
			{
			case 0:	EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim1.wav", 0.8, ATTN_NORM); break;
			case 1:	EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim2.wav", 0.8, ATTN_NORM); break;
			case 2:	EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim3.wav", 0.8, ATTN_NORM); break;
			case 3:	EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim4.wav", 0.8, ATTN_NORM); break;
		}
	}

	if (pev->watertype == CONTENT_LAVA)		// do damage
	{
		if (pev->dmgtime < gpGlobals->time)
			TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), 10 * pev->waterlevel, DMG_BURN);
	}
	else if (pev->watertype == CONTENT_SLIME)		// do damage
	{
		pev->dmgtime = gpGlobals->time + 1;
		TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), 4 * pev->waterlevel, DMG_ACID);
	}
	
	if (!FBitSet(pev->flags, FL_INWATER))
	{
#if 0
		// player enter water sound
		if (pev->watertype == CONTENT_WATER)
		{
			switch (RANDOM_LONG(0,3))
			{
			case 0:	EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade1.wav", 1, ATTN_NORM); break;
			case 1:	EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade2.wav", 1, ATTN_NORM); break;
			case 2:	EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade3.wav", 1, ATTN_NORM); break;
			case 3:	EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade4.wav", 1, ATTN_NORM); break;
			}
		}
#endif
	
		SetBits(pev->flags, FL_INWATER);
		pev->dmgtime = 0;
	}

	// Counter-Strike 1.4: SDK 2.3's text, the water's drag on the velocity is no longer applied here
}


// TRUE if the player is attached to a ladder
BOOL CBasePlayer::IsOnLadder( void )
{ 
	return (pev->movetype == MOVETYPE_FLY);
}


//
// ThrowWeapon - drop the named weapon, if the player has it
//
void CBasePlayer::ThrowWeapon( char *pszItemName )
{
	CBasePlayerItem *pItem;
	int i;

	for ( i = 0 ; i <= 4 ; i++ )	// Beta 5.0 (leak): five slots
	{
		pItem = m_rgpPlayerItems[ i ];

		while (pItem)
		{
			if ( !strcmp( pszItemName, STRING( pItem->pev->classname ) ) )
			{
				DropPlayerItem( pszItemName );
				return;
			}
			pItem = pItem->m_pNext;
		}
	}
}

//
// ThrowPrimary - drop whichever primary weapon the player has
//
void CBasePlayer::ThrowPrimary( void )
{
	ThrowWeapon( "weapon_m249" );
	ThrowWeapon( "weapon_g3sg1" );
	ThrowWeapon( "weapon_sg550" );	// 1.0
	ThrowWeapon( "weapon_awp" );
	ThrowWeapon( "weapon_mp5navy" );
	ThrowWeapon( "weapon_tmp" );
	ThrowWeapon( "weapon_p90" );
	ThrowWeapon( "weapon_ump45" );	// 1.0
	ThrowWeapon( "weapon_m4a1" );
	ThrowWeapon( "weapon_m3" );
	ThrowWeapon( "weapon_sg552" );	// sic: not the AK-47
	ThrowWeapon( "weapon_scout" );	// Beta 4.0
}

// AddAccount - money in and out, kept between 0 and 16000 (name from the
// later versions' Linux symbols). Beta 5.0 (leak): the client is sent the
// money (Money), with whether to show the change
void CBasePlayer::AddAccount( int amount, bool bTrackChange )
{
	m_iAccount += amount;

	if ( m_iAccount < 0 )
		m_iAccount = 0;
	else if ( m_iAccount > 16000 )
		m_iAccount = 16000;

	MESSAGE_BEGIN( MSG_ONE, gmsgMoney, NULL, pev );
		WRITE_LONG( m_iAccount );
		WRITE_BYTE( bTrackChange );
	MESSAGE_END();
}

// Beta 5.0 (leak) (Linux symbol): close the client's menu
void CBasePlayer::ResetMenu( void )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgShowMenu, NULL, pev );
		WRITE_SHORT( 0 );
		WRITE_CHAR( 0 );
		WRITE_BYTE( 0 );
		WRITE_STRING( "" );
	MESSAGE_END();
}

// Beta 5.0 (leak) (Linux symbol): the round's seconds left for the client's timer
void CBasePlayer::SyncRoundTimer( void )
{
	float tmRemaining;	// name ours

	if ( g_pGameRules->IsMultiplayer() )
		tmRemaining = ((CHalfLifeMultiplay *)g_pGameRules)->TimeRemaining();
	else
		tmRemaining = 0;

	MESSAGE_BEGIN( MSG_ONE, gmsgRoundTime, NULL, pev );
		WRITE_SHORT( max( 0, (int)tmRemaining ) );
	MESSAGE_END();
}

// Beta 5.0 (leak) (Linux symbol): the map's briefing goes, and with the intro
// cameras the screen fades in
void CBasePlayer::RemoveLevelText( void )
{
	ResetMenu();

	// Beta 6.6: no screen fade on a map with intro cameras any more
}

// Beta 5.0 (leak) (Linux symbol): a text shown to pl in the menu space, sent in
// pieces of 50 characters
void CBasePlayer::MenuPrint( CBasePlayer *pl, const char *msg )
{
	const char *msg_portion = msg;	// names ours
	char sbuf[51];

	while ( strlen( msg_portion ) >= 50 )
	{
		strncpy( sbuf, msg_portion, 50 );
		sbuf[50] = 0;
		msg_portion += 50;

		MESSAGE_BEGIN( MSG_ONE, gmsgShowMenu, NULL, pl->pev );
			WRITE_SHORT( 0xFFFF );
			WRITE_CHAR( -1 );
			WRITE_BYTE( 1 );	// more to come
			WRITE_STRING( sbuf );
		MESSAGE_END();
	}

	MESSAGE_BEGIN( MSG_ONE, gmsgShowMenu, NULL, pl->pev );
		WRITE_SHORT( 0xFFFF );
		WRITE_CHAR( -1 );
		WRITE_BYTE( 0 );
		WRITE_STRING( msg_portion );
	MESSAGE_END();
}

//=========================================================
// MakeVIP - Beta 6.0 (Linux symbol): the player becomes the
// VIP of an assassination map: a counter-terrorist with the
// vip model (appearance 8), the C4 or kit off his model; the
// game rules count his first round as VIP. Counter-Strike
// 1.0b: the model's name is a literal again (Beta 7.0 to 1.0
// put it together at run time) and no "model vip" command is
// sent to the client
//=========================================================
void CBasePlayer::MakeVIP( void )
{
	pev->body = 0;
	m_iAppearance = 9;	// Beta 6.5 (the guerilla took 5)

	g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", "vip" );

	// Counter-Strike 1.1: a line in the server's log
	UTIL_LogPrintf( "\"%s<%i><%s><CT>\" triggered \"Became_VIP\"\n", STRING( pev->netname ), GETPLAYERUSERID( edict() ), GETPLAYERAUTHID( edict() ) );

	m_iTeam = TEAM_CT;
	m_bIsVIP = true;
	m_fNotKilled = FALSE;	// Counter-Strike 1.4: the new VIP does not keep his weapons into the next round
	((CHalfLifeMultiplay *)g_pGameRules)->m_pVIP = this;
	((CHalfLifeMultiplay *)g_pGameRules)->m_iConsecutiveVIP = 1;
}

void CheckStartMoney( void );	// Counter-Strike 1.3: client.cpp's (declared at this place in Counter-Strike 1.6 too)

//=========================================================
// JoiningThink - Beta 5.0 (leak) (Linux symbol): a player
// who has connected but not joined (m_iJoiningState):
//  start: (to Beta 6.6: the map's briefing, g_szMapBriefingText,
//   in the menu space with the HUD hidden, unless the map has
//   none or the client's "dm" key is 0 or off; Beta 7.0: no
//   text, the team menu follows at once;) the defuser icon
//   cleared; his "ghosts" key (Beta 5.0; the leak read
//   "ignoreghosts", sic: 0 or off set the flag);
//  reading the text: fire, jump or attack 2 ends it;
//  the team and appearance menus (PlayerThink, client.cpp);
//  spawn: the per-player state reset, then spawned, or an
//   observer where the rules let him not respawn; the first
//   of his team is its commander (a terrorist on a bomb map
//   gets the C4 instead).
// Meanwhile he looks through the map's trigger_cameras, the
// next one every 6 seconds.
//=========================================================
void CBasePlayer::JoiningThink( void )
{
	CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;	// Counter-Strike 1.6's names (mp, MPRules, pentSpawnSpot)

	switch ( m_iJoiningState )
	{
	case JOIN_DONE:
		return;

	case JOIN_START:
	{
		ResetMenu();

		// Beta 7.0: the briefing is no longer shown here (the client's VGUI
		// has it): straight to the team menu.
		// Counter-Strike 1.4: the user info keys "dm" and "ghosts" are no longer read
		m_iJoiningState = JOIN_TEAM_MENU;

		MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_STRING( "defuser" );
		MESSAGE_END();

		m_fHasDefuser = false;
		m_flLastActivity = gpGlobals->time;
		m_fShowingLevelText = false;
		break;
	}

	case JOIN_READING_TEXT:
		if ( m_afButtonPressed & ( IN_ATTACK | IN_ATTACK2 | IN_JUMP ) )
		{
			m_afButtonPressed &= ~( IN_ATTACK | IN_ATTACK2 | IN_JUMP );
			RemoveLevelText();
			m_iJoiningState = JOIN_TEAM_MENU;
		}
		break;

	case JOIN_TEAM_MENU:
	case JOIN_CHOOSING:
		break;

	case JOIN_SPAWN:
	{
		m_fNotKilled = false;
		m_iIgnoreMessages = 0;
		m_iTeamKills = 0;
		m_iFOV = 90;
		m_fJustConnected = false;
		ResetMaxSpeed();
		m_iJoiningState = JOIN_DONE;

		CHalfLifeMultiplay *MPRules = (CHalfLifeMultiplay *)g_pGameRules;

		// Beta 6.0: on an escape map a counter-terrorist starts with 3000
		if ( MPRules->m_bMapHasEscapeZone == true && m_iTeam == TEAM_CT )
		{
			m_iAccount = 0;
			// Counter-Strike 1.3: mp_startmoney (Beta 6.0 to 1.1: 3000)
			CheckStartMoney();
			AddAccount( (int)startmoney.value, TRUE );
		}

		if ( !g_pGameRules->FPlayerCanRespawn( this ) )
		{
			pev->deadflag = DEAD_RESPAWNABLE;
			pev->classname = MAKE_STRING( "player" );
			pev->flags &= FL_PROXY;	// keep proxy flag sey by engine
			pev->flags |= FL_SPECTATOR | FL_CLIENT;
			edict_t *pentSpawnSpot = MPRules->GetPlayerSpawnSpot( this );
			StartObserver( pev->origin, VARS( pentSpawnSpot )->angles );
			Observer_SetMode( OBS_ROAMING );	// Counter-Strike 1.4
			MPRules->CheckWinConditions();

			// Beta 7.0: everybody learns his team and score, as Spawn tells them
			MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
				WRITE_BYTE( entindex() );
				if ( m_iTeam == TEAM_CT )
					WRITE_STRING( "CT" );
				else if ( m_iTeam == TEAM_TERRORIST )
					WRITE_STRING( "TERRORIST" );
				else if ( m_iTeam == TEAM_SPECTATOR )	// Counter-Strike 1.1
					WRITE_STRING( "SPECTATOR" );
				else
					WRITE_STRING( "UNASSIGNED" );
			MESSAGE_END();

			MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
				WRITE_BYTE( ENTINDEX(edict()) );
				WRITE_SHORT( pev->frags );
				WRITE_SHORT( m_iDeaths );
				WRITE_SHORT( 0 );
				WRITE_SHORT( m_iTeam );
			MESSAGE_END();
			return;
		}

		Spawn();
		MPRules->CheckWinConditions();

		// Counter-Strike 1.4: a bomb map without a bomb and without a carrier gets one now
		// (to 1.3 the first of each team to spawn became its commander here)
		if ( MPRules->m_flRestartRoundTime == 0 && MPRules->m_fMapHasBombTarget && !MPRules->IsThereABomber() && !MPRules->IsThereABomb() )
			MPRules->GiveC4();

		// Beta 6.0: one more terrorist who can escape this round
		if ( m_iTeam == TEAM_TERRORIST )
			MPRules->m_iNumEscapers++;
		return;
	}
	}

	// the intro cameras
	if ( m_pIntroCamera && gpGlobals->time >= m_flIntroCameraTime )
	{
		CBaseEntity *Target;	// Counter-Strike 1.6's names
		Vector CamAngles;

		m_pIntroCamera = UTIL_FindEntityByClassname( m_pIntroCamera, "trigger_camera" );
		if ( !m_pIntroCamera )
			m_pIntroCamera = UTIL_FindEntityByClassname( NULL, "trigger_camera" );

		// Beta 5.0: the camera looks at its target (the leak took the camera's
		// angles); one without a target ends the tour
		Target = UTIL_FindEntityByTargetname( NULL, STRING( m_pIntroCamera->pev->target ) );
		if ( !Target )
		{
			m_pIntroCamera = NULL;
			return;
		}

		CamAngles = Target->pev->origin - m_pIntroCamera->pev->origin;
		CamAngles = CamAngles.Normalize();
		CamAngles = UTIL_VecToAngles( CamAngles );
		CamAngles.x = -CamAngles.x;

		UTIL_SetOrigin( pev, m_pIntroCamera->pev->origin );
		pev->angles = CamAngles;
		pev->v_angle = pev->angles;
		pev->velocity = g_vecZero;
		pev->punchangle = g_vecZero;
		pev->fixangle = TRUE;
		pev->view_ofs = g_vecZero;
		m_flIntroCameraTime = gpGlobals->time + 6;
	}
}


//=========================================================
// Disappear - Beta 6.0 (Linux symbol): a player who leaves
// the round alive (an escaped terrorist, the rescued VIP) is
// taken out as Killed takes out the dead, without the death:
// no animation, no throw, no gibs, no death sound
//=========================================================
void CBasePlayer::Disappear( void )
{
	CSound *pSound;	// name ours

	if ( m_pTank != NULL )
	{
		m_pTank->Use( this, this, USE_OFF, 0 );
		m_pTank = NULL;
	}

	pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( edict() ) );
	{
		if ( pSound )
		{
			pSound->Reset();
		}
	}

	m_fSequenceFinished = TRUE;
	pev->modelindex = g_ulModelIndexPlayer;
	pev->view_ofs = Vector( 0, 0, -8 );
	pev->deadflag = DEAD_DYING;
	pev->solid = SOLID_NOT;
	ClearBits( pev->flags, FL_ONGROUND );

	SetSuitUpdate( NULL, FALSE, 0 );

	m_iClientHealth = 0;
	MESSAGE_BEGIN( MSG_ONE, gmsgHealth, NULL, pev );
		WRITE_BYTE( m_iClientHealth );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 0xFF );
		WRITE_BYTE( 0xFF );
	MESSAGE_END();

	m_iFOV = m_iClientFOV = 0;

	MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
		WRITE_BYTE( 0 );
	MESSAGE_END();

	((CHalfLifeMultiplay *)g_pGameRules)->CheckWinConditions();

	m_fNotKilled = FALSE;

	if ( m_bHasC4 == true )
	{
		DropPlayerItem( "weapon_c4" );
		SetProgressBarTime( 0 );
	}
	else if ( m_fHasDefuser == true )
	{
		m_fHasDefuser = false;
		pev->body = 0;
		GiveNamedItem( "item_thighpack" );

		MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_STRING( "defuser" );
		MESSAGE_END();

		SetProgressBarTime( 0 );
	}

	BuyZoneIcon_Clear( this );

	SetThink( PlayerDeathThink );
	pev->nextthink = gpGlobals->time + 0.1;

	pev->angles.x = 0;
	pev->angles.z = 0;
}

void CBasePlayer::PlayerDeathThink(void)
{
	float flForward;

	// Beta 5.0 (leak): not while joining
	if ( m_iJoiningState != JOIN_DONE )
		return;

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		flForward = pev->velocity.Length() - 20;
		if (flForward <= 0)
			pev->velocity = g_vecZero;
		else    
			pev->velocity = flForward * pev->velocity.Normalize();
	}

	if ( HasWeapons() )
	{
		// we drop the guns here because weapons that have an area effect and can kill their user
		// will sometimes crash coming back from CBasePlayer::Killed() if they kill their owner because the
		// player class sometimes is freed. It's safer to manipulate the weapons once we know
		// we aren't calling into any of their code anymore through the player pointer.
		PackDeadPlayerItems();
	}


	if (pev->modelindex && (!m_fSequenceFinished) && (pev->deadflag == DEAD_DYING))
	{
		// Counter-Strike 1.4: the death animation plays to its end (the SDK's count of 120 frames,
		// m_iRespawnFrames, is gone, and with it Beta 7.0's second test)
		StudioFrameAdvance( );
		return;
	}

	// Counter-Strike 1.1: the body stays where it has come to lie
	if ( pev->movetype != MOVETYPE_NONE && ( pev->flags & FL_ONGROUND ) )
		pev->movetype = MOVETYPE_NONE;

	// Beta 5.0 (leak): the time of death is when the body comes to rest
	if (pev->deadflag == DEAD_DYING)
	{
		m_fDeadTime = gpGlobals->time;
		pev->deadflag = DEAD_DEAD;
	}
	
	StopAnimation();

	pev->effects |= EF_NOINTERP;

	BOOL fAnyButtonDown = (pev->button & ~IN_SCORE );
	
	// three seconds after his death, a player who is not observing yet goes to the dead camera
	// Counter-Strike 1.4: whatever mp_forcechasecam says (mp_fadetoblack is gone), and the
	// clients are told to make his body first
	if ( g_pGameRules->IsMultiplayer() && ( gpGlobals->time > (m_fDeadTime + 3) ) && !(m_afPhysicsFlags & PFLAG_OBSERVER) )
	{
		SpawnClientSideCorpse();

		// go to dead camera. 
		StartDeathCam();
	}

	// wait for all buttons released
	if ( pev->deadflag == DEAD_DEAD && m_iTeam != TEAM_UNASSIGNED && m_iTeam != TEAM_SPECTATOR )	// Counter-Strike 1.1: nor a spectator
	{
		if (fAnyButtonDown)
			return;

		if ( g_pGameRules->FPlayerCanRespawn( this ) )
		{
			pev->deadflag = DEAD_RESPAWNABLE;

			if ( g_pGameRules->IsMultiplayer() )
				((CHalfLifeMultiplay *)g_pGameRules)->CheckWinConditions();
		}

		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	// Beta 5.0 (leak): a respawnable player respawns at once
	if ( pev->deadflag == DEAD_RESPAWNABLE )
	{
		respawn( pev, FALSE );
		pev->button = 0;
		pev->nextthink = -1;
	}
}

//=========================================================
// RoundRespawn - Beta 5.0 (leak) (Linux symbol): the new
// round respawns every player but one still choosing his
// appearance
//=========================================================
void CBasePlayer::RoundRespawn( void )
{
	// Beta 6.1: a team killer sits the next round out ("new TK 1 round
	// time-out penalty"): he is made to kill himself as he respawns
	// Beta 6.5: only when mp_tkpunish is set
	// Counter-Strike 1.1: Killed shows the hint (1.0b: here, "You're not
	// allowed to play this round because/n you TK'ed last round")
	if ( m_bJustKilledTeammate == true && CVAR_GET_FLOAT( "mp_tkpunish" ) != 0.0 )
	{
		m_bJustKilledTeammate = false;
		CLIENT_COMMAND( edict(), "kill\n" );
		m_bPunishedForTK = true;
	}

	if ( m_iMenu != Menu_ChooseAppearance )
	{
		respawn( pev, FALSE );
		pev->button = 0;
		pev->nextthink = -1;
	}
}

//=========================================================
// StartDeathCam - find an intermission spot and send the
// player off into observer mode
//=========================================================
void CBasePlayer::StartDeathCam( void )
{
	if ( pev->view_ofs == g_vecZero )
	{
		// don't accept subsequent attempts to StartDeathCam()
		return;
	}

	// Counter-Strike 1.4: the observer starts where the player died, looking as he looked
	// (to 1.3: at an info_intermission, or 128 units above the body)
	StartObserver( pev->origin, pev->angles );
}

// Beta 6.5: the observer of observer.cpp (SDK 2.x's pev->iuser1 and
// iuser2: the engine's player movement flies the camera). The attachments
// on the player go, the weapon is holstered, the HUD shows neither weapons
// nor health; the camera starts free (Observer_SetMode: or locked to a team
// mate under mp_forcechasecam), and every client is told that this player
// is a spectator now (message Spectator).
void CBasePlayer::StartObserver( Vector vecPosition, Vector vecViewAngle )
{
	// clear any clientside entities attached to this player
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_KILLPLAYERATTACHMENTS );
		WRITE_BYTE( (BYTE)entindex() );
	MESSAGE_END();

	// Holster weapon immediately, to allow it to cleanup
	if (m_pActiveItem)
		m_pActiveItem->Holster( );

	if ( m_pTank != NULL )
	{
		m_pTank->Use( this, this, USE_OFF, 0 );
		m_pTank = NULL;
	}

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate(NULL, FALSE, 0);

	// Tell Ammo Hud that the player is dead
	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
		WRITE_BYTE(0);
		WRITE_BYTE(0XFF);
		WRITE_BYTE(0xFF);
	MESSAGE_END();

	// reset FOV
	m_iFOV = m_iClientFOV = 0;
	pev->fov = m_iFOV;
	MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
		WRITE_BYTE(0);
	MESSAGE_END();

	// Setup flags
	m_iHideHUD = (HIDEHUD_HEALTH | HIDEHUD_WEAPONS);
	m_afPhysicsFlags |= PFLAG_OBSERVER;
	pev->effects = EF_NODRAW;
	pev->view_ofs = g_vecZero;
	pev->angles = pev->v_angle = vecViewAngle;
	pev->fixangle = TRUE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;

	// Move them to the new position
	UTIL_SetOrigin( pev, vecPosition );

	ClearBits( m_afPhysicsFlags, PFLAG_DUCKING );
	ClearBits( pev->flags, FL_DUCKING );
	pev->health = 1;

	// Find a player to watch
	// Counter-Strike 1.4: nothing is known of a watched player yet, and the mode is the last one used
	m_iObserverC4State = 0;
	m_bObserverHasDefuser = false;
	m_iObserverWeapon = 0;
	m_flNextObserverInput = 0;
	pev->iuser1 = OBS_NONE;
	Observer_SetMode( m_iObserverLastMode );
	ResetMaxSpeed();

	MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );
		WRITE_BYTE( ENTINDEX( edict() ) );
		WRITE_BYTE( 1 );
	MESSAGE_END();
}

// 
// PlayerUse - handles USE keypress
//
#define	PLAYER_SEARCH_RADIUS	(float)64

void CBasePlayer::PlayerUse ( void )
{
	// Was use pressed or released?
	if ( ! ((pev->button | m_afButtonPressed | m_afButtonReleased) & IN_USE) )
		return;

	// Hit Use on a train?
	if ( m_afButtonPressed & IN_USE )
	{
		if ( m_pTank != NULL )
		{
			// Stop controlling the tank
			// TODO: Send HUD Update
			m_pTank->Use( this, this, USE_OFF, 0 );
			m_pTank = NULL;
			return;
		}
		else
		{
			if ( m_afPhysicsFlags & PFLAG_ONTRAIN )
			{
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW|TRAIN_OFF;

				// Beta 7.1: a vehicle loses its driver
				CBaseEntity *pTrain = CBaseEntity::Instance( pev->groundentity );

				if ( pTrain && pTrain->Classify() == CLASS_VEHICLE )
					((CFuncVehicle *)pTrain)->m_pDriver = NULL;

				return;
			}
			else
			{	// Start controlling the train!
				CBaseEntity *pTrain = CBaseEntity::Instance( pev->groundentity );

				if ( pTrain && !(pev->button & IN_JUMP) && FBitSet(pev->flags, FL_ONGROUND) && (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) && pTrain->OnControls(pev) )
				{
					m_afPhysicsFlags |= PFLAG_ONTRAIN;
					m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
					m_iTrain |= TRAIN_NEW;

					// Beta 7.0: a vehicle starts with its own sound.
					// Beta 7.1: and knows its driver
					if ( pTrain->Classify() == CLASS_VEHICLE )
					{
						EMIT_SOUND( ENT(pev), CHAN_ITEM, "plats/vehicle_ignition.wav", 0.8, ATTN_NORM);
						((CFuncVehicle *)pTrain)->m_pDriver = this;
					}
					else
						EMIT_SOUND( ENT(pev), CHAN_ITEM, "plats/train_use1.wav", 0.8, ATTN_NORM);
					return;
				}
			}
		}
	}

	CBaseEntity *pObject = NULL;
	CBaseEntity *pClosest = NULL;
	Vector		vecLOS;
	float flMaxDot = VIEW_FIELD_NARROW;
	float flDot;

	UTIL_MakeVectors ( pev->v_angle );// so we know which way we are facing
	
	while ((pObject = UTIL_FindEntityInSphere( pObject, pev->origin, PLAYER_SEARCH_RADIUS )) != NULL)
	{

		if (pObject->ObjectCaps() & (FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE))
		{
			// !!!PERFORMANCE- should this check be done on a per case basis AFTER we've determined that
			// this object is actually usable? This dot is being done for every object within PLAYER_SEARCH_RADIUS
			// when player hits the use key. How many objects can be in that area, anyway? (sjb)
			vecLOS = (VecBModelOrigin( pObject->pev ) - (pev->origin + pev->view_ofs));
			
			// This essentially moves the origin of the target to the corner nearest the player to test to see 
			// if it's "hull" is in the view cone
			vecLOS = UTIL_ClampVectorToBox( vecLOS, pObject->pev->size * 0.5 );
			
			flDot = DotProduct (vecLOS , gpGlobals->v_forward);
			if (flDot > flMaxDot )
			{// only if the item is in front of the user
				pClosest = pObject;
				flMaxDot = flDot;
//				ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
			}
//			ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
		}
	}
	pObject = pClosest;

	// Found an object
	if (pObject )
	{
		//!!!UNDONE: traceline here to prevent USEing buttons through walls			
		int caps = pObject->ObjectCaps();

		if ( m_afButtonPressed & IN_USE )
			EMIT_SOUND( ENT(pev), CHAN_ITEM, "common/wpn_select.wav", 0.4, ATTN_NORM);

		if ( ( (pev->button & IN_USE) && (caps & FCAP_CONTINUOUS_USE) ) ||
			 ( (m_afButtonPressed & IN_USE) && (caps & (FCAP_IMPULSE_USE|FCAP_ONOFF_USE)) ) )
		{
			if ( caps & FCAP_CONTINUOUS_USE )
				m_afPhysicsFlags |= PFLAG_USING;

			pObject->Use( this, this, USE_SET, 1 );
		}
		// UNDONE: Send different USE codes for ON/OFF.  Cache last ONOFF_USE object to send 'off' if you turn away
		else if ( (m_afButtonReleased & IN_USE) && (pObject->ObjectCaps() & FCAP_ONOFF_USE) )	// BUGBUG This is an "off" use
		{
			pObject->Use( this, this, USE_SET, 0 );
		}
	}
	else
	{
		if ( m_afButtonPressed & IN_USE )
			EMIT_SOUND( ENT(pev), CHAN_ITEM, "common/wpn_denyselect.wav", 0.4, ATTN_NORM);
	}

	// Beta 5.0 (leak): Beta 2.0 to 4.1's use of the hostages within 64 units is
	// gone: the hostage is used as any entity (CHostage::ObjectCaps)
}



// Beta 5.0 (leak): the hostage's Use calls it: the first use's hint
void CBasePlayer::HostageUsed( void )
{
	if ( m_flDisplayHistory & 0x10 )	// hostage used
		return;

	switch ( m_iTeam )
	{
	case TEAM_TERRORIST:
		HintMessage( "#Hint_use_hostage_to_stop_him", FALSE, FALSE );	// Counter-Strike 1.1: the hints are titles of the client's
		break;
	case TEAM_CT:
		HintMessage( "#Hint_lead_hostage_to_rescue_point", FALSE, FALSE );
		break;
	}

	m_flDisplayHistory |= 0x10;
}



void CBasePlayer::Jump()
{
	Vector		vecWallCheckDir;// direction we're tracing a line to find a wall when walljumping
	Vector		vecAdjustedVelocity;
	Vector		vecSpot;
	TraceResult	tr;
	
	if (FBitSet(pev->flags, FL_WATERJUMP))
		return;
	
	if (pev->waterlevel >= 2)
	{
		return;
	}

	// jump velocity is sqrt( height * gravity * 2)

	// If this isn't the first frame pressing the jump button, break out.
	if ( !FBitSet( m_afButtonPressed, IN_JUMP ) )
		return;         // don't pogo stick

	if ( !(pev->flags & FL_ONGROUND) || !pev->groundentity )
	{
		return;
	}

// many features in this function use v_forward, so makevectors now.
	UTIL_MakeVectors (pev->angles);

	SetAnimation( PLAYER_JUMP );

	if ( FBitSet(pev->flags, FL_DUCKING ) || FBitSet(m_afPhysicsFlags, PFLAG_DUCKING) )
	{
		if ( m_fLongJump && (pev->button & IN_DUCK) && gpGlobals->time - m_flDuckTime < 1 && pev->velocity.Length() > 50 )
		{
			SetAnimation( PLAYER_SUPERJUMP );
		}
	}
	
	// If you're standing on a conveyor, add it's velocity to yours (for momentum)
	entvars_t *pevGround = VARS(pev->groundentity);
	if ( pevGround )
	{
		if ( pevGround->flags & FL_CONVEYOR )
			pev->velocity = pev->velocity + pev->basevelocity;

	}

	// Beta 6.0: a jump off a train keeps the train's velocity. Beta 7.0: off a
	// vehicle too; its test stands outside the one for the ground's presence
	// (the function has left by then when there is no ground entity)
	if ( ( pevGround && ( !strcmp( "func_tracktrain", STRING( pevGround->classname ) ) || !strcmp( "func_train", STRING( pevGround->classname ) ) ) ) || !strcmp( "func_vehicle", STRING( pevGround->classname ) ) )
		pev->velocity = pev->velocity + pevGround->velocity;
}



// This is a glorious hack to find free space when you've crouched into some solid space
// Our crouching collisions do not work correctly for some reason and this is easier
// than fixing the problem :(
void FixPlayerCrouchStuck( edict_t *pPlayer )
{
	TraceResult trace;

	// Move up as many as 18 pixels if the player is stuck.
	for ( int i = 0; i < 18; i++ )
	{
		UTIL_TraceHull( pPlayer->v.origin, pPlayer->v.origin, dont_ignore_monsters, head_hull, pPlayer, &trace );
		if ( trace.fStartSolid )
			pPlayer->v.origin.z ++;
		else
			break;
	}
}

void CBasePlayer::Duck( )
{
	if (pev->button & IN_DUCK) 
	{
		SetAnimation( PLAYER_WALK );
	}
}

//
// ID's player as such.
//
int  CBasePlayer::Classify ( void )
{
	return CLASS_PLAYER;
}


void CBasePlayer::AddPoints( int score, BOOL bAllowNegativeScore )
{
	// Positive score always adds
	if ( score < 0 )
	{
		if ( !bAllowNegativeScore )
		{
			if ( pev->frags < 0 )		// Can't go more negative
				return;
			
			if ( -score > pev->frags )	// Will this go negative?
			{
				score = -pev->frags;		// Sum will be 0
			}
		}
	}

	pev->frags += score;

	MESSAGE_BEGIN( MSG_BROADCAST, gmsgScoreInfo );	// Beta 6.0: unreliable (was MSG_ALL)
		WRITE_BYTE( ENTINDEX(edict()) );
		WRITE_SHORT( pev->frags );
		WRITE_SHORT( m_iDeaths );
		WRITE_SHORT( 0 );			// Beta 6.6: two more shorts, a zero and the team
		WRITE_SHORT( m_iTeam );
	MESSAGE_END();
}


void CBasePlayer::AddPointsToTeam( int score, BOOL bAllowNegativeScore )
{
	int index = entindex();

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer && i != index )
		{
			if ( g_pGameRules->PlayerRelationship( this, pPlayer ) == GR_TEAMMATE )
			{
				pPlayer->AddPoints( score, bAllowNegativeScore );
			}
		}
	}
}

#if 0
void CBasePlayer::CheckWeapon(void)
{
	// play a weapon idle anim if it's time!
	if ( gpGlobals->time > m_flTimeWeaponIdle )
	{
		WeaponIdle ( );
	}
}
#endif


// (Counter-Strike 1.4: PlayStepSound, MapTextureTypeStepType and UpdateStepSound are gone, as in
// SDK 2.3: the footsteps are pm_shared's alone; neither retail nor the Linux library has them)

#define CLIMB_SHAKE_FREQUENCY	22	// how many frames in between screen shakes when climbing
#define	MAX_CLIMB_SPEED			200	// fastest vertical climbing speed possible
#define	CLIMB_SPEED_DEC			15	// climbing deceleration rate
#define	CLIMB_PUNCH_X			-7  // how far to 'punch' client X axis when climbing
#define CLIMB_PUNCH_Z			7	// how far to 'punch' client Z axis when climbing

//=========================================================
// CanPlayerBuy - Beta 6.0 (Linux symbol): buying is over 60
// seconds into the round (Beta 7.0: 90), the VIP never buys, and
// info_map_parameters can forbid it to either team; display
// says why in the centre of the screen
//=========================================================
bool CBasePlayer::CanPlayerBuy( bool display )
{
	// 1.0: the training rules decide alone (the buy zone's signal)
	if ( !g_pGameRules->IsMultiplayer() )
		return ((CHalfLifeTraining *)g_pGameRules)->PlayerCanBuy( this );

	CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;	// Counter-Strike 1.6's name

	// Counter-Strike 1.1: the buying time is mp_buytime, in minutes, and at least
	// half a minute (Beta 7.0 to 1.0b: 90 seconds; 6.6: 60). Every reason has a
	// title of the client's now and returns at once; 1.0b copied an English
	// sentence to a buffer and printed the last one that applied.
	// Counter-Strike 1.3: at least a quarter of a minute
	int buyTime = (int)( CVAR_GET_FLOAT( "mp_buytime" ) * 60 );	// Counter-Strike 1.6's name

	if ( buyTime < 15 )
	{
		buyTime = 15;
		CVAR_SET_FLOAT( "mp_buytime", 0.25 );
	}

	if ( gpGlobals->time - mp->m_flRoundStartTime > buyTime )
	{
		if ( display == true )
			ClientPrint( pev, HUD_PRINTCENTER, "#Cant_buy", UTIL_dtos1( buyTime ) );
		return false;
	}

	if ( m_bIsVIP )
	{
		if ( display == true )
			ClientPrint( pev, HUD_PRINTCENTER, "#VIP_cant_buy" );
		return false;
	}

	if ( mp->m_bCTCantBuy == true && m_iTeam == TEAM_CT )
	{
		if ( display == true )
			ClientPrint( pev, HUD_PRINTCENTER, "#CT_cant_buy" );
		return false;
	}

	if ( mp->m_bTCantBuy == true && m_iTeam == TEAM_TERRORIST )
	{
		if ( display == true )
			ClientPrint( pev, HUD_PRINTCENTER, "#Terrorist_cant_buy" );
		return false;
	}

	return true;
}

void CBasePlayer::PreThink(void)
{
	int buttonsChanged = (m_afButtonLast ^ pev->button);	// These buttons have changed this frame

	// Counter-Strike 1.4: any change of the buttons is activity (the idle check below; to 1.3: a
	// button held when the check ran)
	if ( buttonsChanged )
		m_flLastActivity = gpGlobals->time;

	// Debounced button codes for pressed/released
	// UNDONE: Do we need auto-repeat?
	m_afButtonPressed =  buttonsChanged & pev->button;		// The changed ones still down are "pressed"
	m_afButtonReleased = buttonsChanged & (~pev->button);	// The ones not down are "released"

	m_hintMessageQueue.Update( *this );

	g_pGameRules->PlayerThink( this );

	if ( g_fGameOver )
		return;         // intermission or finale

	if ( m_iJoiningState != JOIN_DONE )
		JoiningThink();

	// (Beta 7.0 to 1.3 read the user info keys lefthand, vgui_menus and name again here every three
	// seconds; Counter-Strike 1.4 has SetPrefsFromUserinfo and the deferred name change instead)

	// fire or secondary fire takes the map's text away (and does nothing else); Beta 6.5: no longer jump
	if ( m_fShowingLevelText == true && ( m_afButtonPressed & ( IN_ATTACK | IN_ATTACK2 ) ) )
	{
		m_afButtonPressed &= ~( IN_ATTACK | IN_ATTACK2 );
		RemoveLevelText();
		m_fShowingLevelText = false;
	}

	UTIL_MakeVectors(pev->v_angle);             // is this still used?
	
	ItemPreFrame( );
	WaterMove();

	// a hit slows the player down: get back to full speed by 0.01 a frame on the ground
	// (Counter-Strike 1.4: the limit of 1 is tested on the ground only)
	if ( FBitSet( pev->flags, FL_ONGROUND ) )
	{
		if ( m_flVelocityModifier < 1.0 )
		{
			m_flVelocityModifier += 0.01;
			pev->velocity = pev->velocity * m_flVelocityModifier;
		}

		if ( m_flVelocityModifier > 1.0 )
			m_flVelocityModifier = 1;
	}

	// Beta 5.0 (leak): a player who presses no button for longer than the
	// game rules' limit (mp_roundtime times 120 seconds) is kicked
	// when mp_autokick is set.
	// Counter-Strike 1.4: looked at every 5 seconds (m_flIdleCheckTime, 1.6's name; to 1.3 this
	// was the hostages' trail timer of 0.2 seconds, and UpdatePath ran here)
	if ( m_flIdleCheckTime <= gpGlobals->time || m_flIdleCheckTime == 0.0 )
	{
		m_flIdleCheckTime = gpGlobals->time + 5;

		if ( gpGlobals->time - m_flLastActivity > ((CHalfLifeMultiplay *)g_pGameRules)->m_flMaxIdlePeriod && CVAR_GET_FLOAT( "mp_autokick" ) != 0 )
		{
			UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "#Game_idle_kick", STRING( pev->netname ) );	// Counter-Strike 1.1: a title (1.0b: "%s has been idle for too long and has been kicked.\n")
			SERVER_COMMAND( UTIL_VarArgs( "kick %s\n", STRING( pev->netname ) ) );
			m_flLastActivity = gpGlobals->time;
		}
	}

	if ( g_pGameRules && g_pGameRules->FAllowFlashlight() )
		m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
	else
		m_iHideHUD |= HIDEHUD_FLASHLIGHT;


	// JOHN: checks if new client data (for HUD and view control) needs to be sent to the client
	UpdateClientData();
	
	CheckTimeBasedDamage();

	CheckSuitUpdate();

	// So the correct flags get sent to client asap.
	// (Beta 7.1: before the observer's and the dead player's returns)
	if ( m_afPhysicsFlags & PFLAG_ONTRAIN )
		pev->flags |= FL_ONTRAIN;
	else 
		pev->flags &= ~FL_ONTRAIN;

	// Beta 6.5: an observer's buttons work his camera (observer.cpp)
	// Counter-Strike 1.4: only once StartObserver has run (the dead player's iuser1 is set by Killed)
	if ( pev->iuser1 != OBS_NONE && ( m_afPhysicsFlags & PFLAG_OBSERVER ) )
	{
		Observer_HandleButtons();
		Observer_CheckTarget();
		Observer_CheckProperties();
		return;
	}

	if ( pev->deadflag >= DEAD_DYING && pev->deadflag != DEAD_RESPAWNABLE )
	{
		PlayerDeathThink();
		return;
	}

	// Beta 7.1: iuser4 tells the clients that he stands on a vehicle
	// (AddToFullPack sends it; the client does not interpolate him then)
	CBaseEntity *pGroundEntity = CBaseEntity::Instance( pev->groundentity );	// name ours

	if ( pGroundEntity && pGroundEntity->Classify() == CLASS_VEHICLE )
		pev->iuser4 = 1;
	else
		pev->iuser4 = 0;

	// Train speed control
	if ( m_afPhysicsFlags & PFLAG_ONTRAIN )
	{
		CBaseEntity *pTrain = CBaseEntity::Instance( pev->groundentity );
		float vel;
		
		if ( !pTrain )
		{
			TraceResult trainTrace;
			// Maybe this is on the other side of a level transition
			UTIL_TraceLine( pev->origin, pev->origin + Vector(0,0,-38), ignore_monsters, ENT(pev), &trainTrace );

			// HACKHACK - Just look for the func_tracktrain classname
			if ( trainTrace.flFraction != 1.0 && trainTrace.pHit )
			pTrain = CBaseEntity::Instance( trainTrace.pHit );


			if ( !pTrain || !(pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) || !pTrain->OnControls(pev) )
			{
				//ALERT( at_error, "In train mode with no train!\n" );
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW|TRAIN_OFF;
				((CFuncVehicle *)pTrain)->m_pDriver = NULL;	// Beta 7.1 (sic: pTrain may be NULL here, and need not be a vehicle)
				return;
			}
		}
		else if ( !FBitSet( pev->flags, FL_ONGROUND ) || FBitSet( pTrain->pev->spawnflags, SF_TRACKTRAIN_NOCONTROL ) )
		{
			// Turn off the train if you jump, strafe, or the train controls go dead
			// (Beta 7.0: no longer when the player strafes: a vehicle is steered so)
			m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
			m_iTrain = TRAIN_NEW|TRAIN_OFF;
			((CFuncVehicle *)pTrain)->m_pDriver = NULL;	// Beta 7.1 (sic: any train's 0x148)
			return;
		}

		pev->velocity = g_vecZero;
		vel = 0;

		// Beta 7.0: a vehicle (func_vehicle) takes the buttons while they are
		// held, forward and back as a train, left and right as 20 and 30
		if ( pTrain->Classify() == CLASS_VEHICLE )
		{
			if ( pev->button & IN_FORWARD )
			{
				vel = 1;
				pTrain->Use( this, this, USE_SET, (float)vel );
			}
			if ( pev->button & IN_BACK )
			{
				vel = -1;
				pTrain->Use( this, this, USE_SET, (float)vel );
			}
			if ( pev->button & IN_MOVELEFT )
			{
				vel = 20;
				pTrain->Use( this, this, USE_SET, (float)vel );
			}
			if ( pev->button & IN_MOVERIGHT )
			{
				vel = 30;
				pTrain->Use( this, this, USE_SET, (float)vel );
			}
		}
		else
		{
			if ( m_afButtonPressed & IN_FORWARD )
			{
				vel = 1;
				pTrain->Use( this, this, USE_SET, (float)vel );
			}
			else if ( m_afButtonPressed & IN_BACK )
			{
				vel = -1;
				pTrain->Use( this, this, USE_SET, (float)vel );
			}
		}

		if (vel)
		{
			m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
			m_iTrain |= TRAIN_ACTIVE|TRAIN_NEW;
		}

	} else if (m_iTrain & TRAIN_ACTIVE)
		m_iTrain = TRAIN_NEW; // turn off train

	if (pev->button & IN_JUMP)
	{
		// If on a ladder, jump off the ladder
		// else Jump
		Jump();
	}


	// If trying to duck, already ducked, or in the process of ducking
	if ((pev->button & IN_DUCK) || FBitSet(pev->flags,FL_DUCKING) || (m_afPhysicsFlags & PFLAG_DUCKING) )
		Duck();

	// (Counter-Strike 1.4: the server plays no footsteps any more; UpdateStepSound is gone)

	if ( !FBitSet ( pev->flags, FL_ONGROUND ) )
	{
		m_flFallVelocity = -pev->velocity.z;
	}

	// StudioFrameAdvance( );//!!!HACKHACK!!! Can't be hit by traceline when not animating?

	// Clear out ladder pointer
	m_hEnemy = NULL;

	if ( m_afPhysicsFlags & PFLAG_ONBARNACLE )
	{
		pev->velocity = g_vecZero;
	}

	// Beta 6.0: only when he can buy (sic: CanPlayerBuy( false ) never says yes, see there)
	if ( !( m_flDisplayHistory & 0x2 ) && CanPlayerBuy( false ) == true )	// buy key
	{
		HintMessage( "#Hint_press_buy_to_purchase", FALSE, FALSE );	// Counter-Strike 1.1: a title
		m_flDisplayHistory |= 0x2;
	}
}
/* Time based Damage works as follows: 
	1) There are several types of timebased damage:

		#define DMG_PARALYZE		(1 << 14)	// slows affected creature down
		#define DMG_NERVEGAS		(1 << 15)	// nerve toxins, very bad
		#define DMG_POISON			(1 << 16)	// blood poisioning
		#define DMG_RADIATION		(1 << 17)	// radiation exposure
		#define DMG_DROWNRECOVER	(1 << 18)	// drown recovery
		#define DMG_ACID			(1 << 19)	// toxic chemicals or acid burns
		#define DMG_SLOWBURN		(1 << 20)	// in an oven
		#define DMG_SLOWFREEZE		(1 << 21)	// in a subzero freezer

	2) A new hit inflicting tbd restarts the tbd counter - each monster has an 8bit counter,
		per damage type. The counter is decremented every second, so the maximum time 
		an effect will last is 255/60 = 4.25 minutes.  Of course, staying within the radius
		of a damaging effect like fire, nervegas, radiation will continually reset the counter to max.

	3) Every second that a tbd counter is running, the player takes damage.  The damage
		is determined by the type of tdb.  
			Paralyze		- 1/2 movement rate, 30 second duration.
			Nervegas		- 5 points per second, 16 second duration = 80 points max dose.
			Poison			- 2 points per second, 25 second duration = 50 points max dose.
			Radiation		- 1 point per second, 50 second duration = 50 points max dose.
			Drown			- 5 points per second, 2 second duration.
			Acid/Chemical	- 5 points per second, 10 second duration = 50 points max.
			Burn			- 10 points per second, 2 second duration.
			Freeze			- 3 points per second, 10 second duration = 30 points max.

	4) Certain actions or countermeasures counteract the damaging effects of tbds:

		Armor/Heater/Cooler - Chemical(acid),burn, freeze all do damage to armor power, then to body
							- recharged by suit recharger
		Air In Lungs		- drowning damage is done to air in lungs first, then to body
							- recharged by poking head out of water
							- 10 seconds if swiming fast
		Air In SCUBA		- drowning damage is done to air in tanks first, then to body
							- 2 minutes in tanks. Need new tank once empty.
		Radiation Syringe	- Each syringe full provides protection vs one radiation dosage
		Antitoxin Syringe	- Each syringe full provides protection vs one poisoning (nervegas or poison).
		Health kit			- Immediate stop to acid/chemical, fire or freeze damage.
		Radiation Shower	- Immediate stop to radiation damage, acid/chemical or fire damage.
		
	
*/

// If player is taking time based damage, continue doing damage to player -
// this simulates the effect of being poisoned, gassed, dosed with radiation etc -
// anything that continues to do damage even after the initial contact stops.
// Update all time based damage counters, and shut off any that are done.

// The m_bitsDamageType bit MUST be set if any damage is to be taken.
// This routine will detect the initial on value of the m_bitsDamageType
// and init the appropriate counter.  Only processes damage every second.

//#define PARALYZE_DURATION	30		// number of 2 second intervals to take damage
//#define PARALYZE_DAMAGE		0.0		// damage to take each 2 second interval

//#define NERVEGAS_DURATION	16
//#define NERVEGAS_DAMAGE		5.0

//#define POISON_DURATION		25
//#define POISON_DAMAGE		2.0

//#define RADIATION_DURATION	50
//#define RADIATION_DAMAGE	1.0

//#define ACID_DURATION		10
//#define ACID_DAMAGE			5.0

//#define SLOWBURN_DURATION	2
//#define SLOWBURN_DAMAGE		1.0

//#define SLOWFREEZE_DURATION	1.0
//#define SLOWFREEZE_DAMAGE	3.0

/* */


void CBasePlayer::CheckTimeBasedDamage() 
{
	int i;
	BYTE bDuration = 0;

	static float gtbdPrev = 0.0;

	if (!(m_bitsDamageType & DMG_TIMEBASED))
		return;

	// only check for time based damage approx. every 2 seconds
	if (abs(gpGlobals->time - m_tbdPrev) < 2.0)
		return;
	
	m_tbdPrev = gpGlobals->time;

	for (i = 0; i < CDMG_TIMEBASED; i++)
	{
		// make sure bit is set for damage type
		if (m_bitsDamageType & (DMG_PARALYZE << i))
		{
			switch (i)
			{
			case itbd_Paralyze:
				// UNDONE - flag movement as half-speed
				bDuration = PARALYZE_DURATION;
				break;
			case itbd_NerveGas:
//				TakeDamage(pev, pev, NERVEGAS_DAMAGE, DMG_GENERIC);	
				bDuration = NERVEGAS_DURATION;
				break;
			case itbd_Poison:
				TakeDamage(pev, pev, POISON_DAMAGE, DMG_GENERIC);
				bDuration = POISON_DURATION;
				break;
			case itbd_Radiation:
//				TakeDamage(pev, pev, RADIATION_DAMAGE, DMG_GENERIC);
				bDuration = RADIATION_DURATION;
				break;
			case itbd_DrownRecover:
				// NOTE: this hack is actually used to RESTORE health
				// after the player has been drowning and finally takes a breath
				if (m_idrowndmg > m_idrownrestored)
				{
					int idif = min(m_idrowndmg - m_idrownrestored, 10);

					TakeHealth(idif, DMG_GENERIC);
					m_idrownrestored += idif;
				}
				bDuration = 4;	// get up to 5*10 = 50 points back
				break;
			case itbd_Acid:
//				TakeDamage(pev, pev, ACID_DAMAGE, DMG_GENERIC);
				bDuration = ACID_DURATION;
				break;
			case itbd_SlowBurn:
//				TakeDamage(pev, pev, SLOWBURN_DAMAGE, DMG_GENERIC);
				bDuration = SLOWBURN_DURATION;
				break;
			case itbd_SlowFreeze:
//				TakeDamage(pev, pev, SLOWFREEZE_DAMAGE, DMG_GENERIC);
				bDuration = SLOWFREEZE_DURATION;
				break;
			default:
				bDuration = 0;
			}

			if (m_rgbTimeBasedDamage[i])
			{
				// use up an antitoxin on poison or nervegas after a few seconds of damage					
				if (((i == itbd_NerveGas) && (m_rgbTimeBasedDamage[i] < NERVEGAS_DURATION)) ||
					((i == itbd_Poison)   && (m_rgbTimeBasedDamage[i] < POISON_DURATION)))
				{
					if (m_rgItems[ITEM_ANTIDOTE])
					{
						m_rgbTimeBasedDamage[i] = 0;
						m_rgItems[ITEM_ANTIDOTE]--;
						SetSuitUpdate("!HEV_HEAL4", FALSE, SUIT_REPEAT_OK);
					}
				}


				// decrement damage duration, detect when done.
				if (!m_rgbTimeBasedDamage[i] || --m_rgbTimeBasedDamage[i] == 0)
				{
					m_rgbTimeBasedDamage[i] = 0;
					// if we're done, clear damage bits
					m_bitsDamageType &= ~(DMG_PARALYZE << i);	
				}
			}
			else
				// first time taking this damage type - init damage duration
				m_rgbTimeBasedDamage[i] = bDuration;
		}
	}
}

/*
THE POWER SUIT

The Suit provides 3 main functions: Protection, Notification and Augmentation. 
Some functions are automatic, some require power. 
The player gets the suit shortly after getting off the train in C1A0 and it stays
with him for the entire game.

Protection

	Heat/Cold
		When the player enters a hot/cold area, the heating/cooling indicator on the suit 
		will come on and the battery will drain while the player stays in the area. 
		After the battery is dead, the player starts to take damage. 
		This feature is built into the suit and is automatically engaged.
	Radiation Syringe
		This will cause the player to be immune from the effects of radiation for N seconds. Single use item.
	Anti-Toxin Syringe
		This will cure the player from being poisoned. Single use item.
	Health
		Small (1st aid kits, food, etc.)
		Large (boxes on walls)
	Armor
		The armor works using energy to create a protective field that deflects a
		percentage of damage projectile and explosive attacks. After the armor has been deployed,
		it will attempt to recharge itself to full capacity with the energy reserves from the battery.
		It takes the armor N seconds to fully charge. 

Notification (via the HUD)

x	Health
x	Ammo  
x	Automatic Health Care
		Notifies the player when automatic healing has been engaged. 
x	Geiger counter
		Classic Geiger counter sound and status bar at top of HUD 
		alerts player to dangerous levels of radiation. This is not visible when radiation levels are normal.
x	Poison
	Armor
		Displays the current level of armor. 

Augmentation 

	Reanimation (w/adrenaline)
		Causes the player to come back to life after he has been dead for 3 seconds. 
		Will not work if player was gibbed. Single use.
	Long Jump
		Used by hitting the ??? key(s). Caused the player to further than normal.
	SCUBA	
		Used automatically after picked up and after player enters the water. 
		Works for N seconds. Single use.	
	
Things powered by the battery

	Armor		
		Uses N watts for every M units of damage.
	Heat/Cool	
		Uses N watts for every second in hot/cold area.
	Long Jump	
		Uses N watts for every jump.
	Alien Cloak	
		Uses N watts for each use. Each use lasts M seconds.
	Alien Shield	
		Augments armor. Reduces Armor drain by one half
 
*/

// if in range of radiation source, ping geiger counter
#define GEIGERDELAY 0.25

void CBasePlayer :: UpdateGeigerCounter( void )
{
	BYTE range;

	// delay per update ie: don't flood net with these msgs
	if (gpGlobals->time < m_flgeigerDelay)
		return;

	m_flgeigerDelay = gpGlobals->time + GEIGERDELAY;
		
	// send range to radition source to client

	range = (BYTE) (m_flgeigerRange / 4);

	if (range != m_igeigerRangePrev)
	{
		m_igeigerRangePrev = range;

		MESSAGE_BEGIN( MSG_ONE, gmsgGeigerRange, NULL, pev );
			WRITE_BYTE( range );
		MESSAGE_END();
	}

	// reset counter and semaphore
	if (!RANDOM_LONG(0,3))
		m_flgeigerRange = 1000;

}

/*
================
CheckSuitUpdate

Play suit update if it's time
================
*/

#define SUITUPDATETIME	3.5
#define SUITFIRSTUPDATETIME 0.1

void CBasePlayer::CheckSuitUpdate()
{
	int i;
	int isentence = 0;
	int isearch = m_iSuitPlayNext;
	
	// Ignore suit updates if no suit
	if ( !(pev->weapons & (1<<WEAPON_SUIT)) )
		return;

	// if in range of radiation source, ping geiger counter
	UpdateGeigerCounter();

	if ( g_pGameRules->IsMultiplayer() )
	{
		// don't bother updating HEV voice in multiplayer.
		return;
	}

	if ( gpGlobals->time >= m_flSuitUpdate && m_flSuitUpdate > 0)
	{
		// play a sentence off of the end of the queue
		for (i = 0; i < CSUITPLAYLIST; i++)
			{
			if (isentence = m_rgSuitPlayList[isearch])
				break;
			
			if (++isearch == CSUITPLAYLIST)
				isearch = 0;
			}

		if (isentence)
		{
			m_rgSuitPlayList[isearch] = 0;
			if (isentence > 0)
			{
				// play sentence number

				char sentence[CBSENTENCENAME_MAX+1];
				strcpy(sentence, "!");
				strcat(sentence, gszallsentencenames[isentence]);
				EMIT_SOUND_SUIT(ENT(pev), sentence);
			}
			else
			{
				// play sentence group
				EMIT_GROUPID_SUIT(ENT(pev), -isentence);
			}
		m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME;
		}
		else
			// queue is empty, don't check 
			m_flSuitUpdate = 0;
	}
}
 
// add sentence to suit playlist queue. if fgroup is true, then
// name is a sentence group (HEV_AA), otherwise name is a specific
// sentence name ie: !HEV_AA0.  If iNoRepeat is specified in
// seconds, then we won't repeat playback of this word or sentence
// for at least that number of seconds.

void CBasePlayer::SetSuitUpdate(char *name, int fgroup, int iNoRepeatTime)
{
	int i;
	int isentence;
	int iempty = -1;
	
	// Counter-Strike 1.0: no HEV voice any more (retail's function is one
	// return; the training course would be the single-player game that
	// reaches the lines below)
	return;
	
	// Ignore suit updates if no suit
	if ( !(pev->weapons & (1<<WEAPON_SUIT)) )
		return;

	if ( g_pGameRules->IsMultiplayer() )
	{
		// due to static channel design, etc. We don't play HEV sounds in multiplayer right now.
		return;
	}

	// if name == NULL, then clear out the queue

	if (!name)
	{
		for (i = 0; i < CSUITPLAYLIST; i++)
			m_rgSuitPlayList[i] = 0;
		return;
	}
	// get sentence or group number
	if (!fgroup)
	{
		isentence = SENTENCEG_Lookup(name, NULL);
		if (isentence < 0)
			return;
	}
	else
		// mark group number as negative
		isentence = -SENTENCEG_GetIndex(name);

	// check norepeat list - this list lets us cancel
	// the playback of words or sentences that have already
	// been played within a certain time.

	for (i = 0; i < CSUITNOREPEAT; i++)
	{
		if (isentence == m_rgiSuitNoRepeat[i])
			{
			// this sentence or group is already in 
			// the norepeat list

			if (m_rgflSuitNoRepeatTime[i] < gpGlobals->time)
				{
				// norepeat time has expired, clear it out
				m_rgiSuitNoRepeat[i] = 0;
				m_rgflSuitNoRepeatTime[i] = 0.0;
				iempty = i;
				break;
				}
			else
				{
				// don't play, still marked as norepeat
				return;
				}
			}
		// keep track of empty slot
		if (!m_rgiSuitNoRepeat[i])
			iempty = i;
	}

	// sentence is not in norepeat list, save if norepeat time was given

	if (iNoRepeatTime)
	{
		if (iempty < 0)
			iempty = RANDOM_LONG(0, CSUITNOREPEAT-1); // pick random slot to take over
		m_rgiSuitNoRepeat[iempty] = isentence;
		m_rgflSuitNoRepeatTime[iempty] = iNoRepeatTime + gpGlobals->time;
	}

	// find empty spot in queue, or overwrite last spot
	
	m_rgSuitPlayList[m_iSuitPlayNext++] = isentence;
	if (m_iSuitPlayNext == CSUITPLAYLIST)
		m_iSuitPlayNext = 0;

	if (m_flSuitUpdate <= gpGlobals->time)
	{
		if (m_flSuitUpdate == 0)
			// play queue is empty, don't delay too long before playback
			m_flSuitUpdate = gpGlobals->time + SUITFIRSTUPDATETIME;
		else 
			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME; 
	}

}

/*
================
CheckPowerups

Check for turning off powerups

GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
================
*/
	static void
CheckPowerups(entvars_t *pev)
{
	if (pev->health <= 0)
		return;

	pev->modelindex = g_ulModelIndexPlayer;    // don't use eyes
}


//=========================================================
// UpdatePlayerSound - updates the position of the player's
// reserved sound slot in the sound list.
//=========================================================
void CBasePlayer :: UpdatePlayerSound ( void )
{
	int iBodyVolume;
	int iVolume;
	CSound *pSound;

	pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt :: ClientSoundIndex( edict() ) );

	if ( !pSound )
	{
		ALERT ( at_console, "Client lost reserved sound!\n" );
		return;
	}

	pSound->m_iType = bits_SOUND_NONE;

	// now calculate the best target volume for the sound. If the player's weapon
	// is louder than his body/movement, use the weapon volume, else, use the body volume.
	
	if ( FBitSet ( pev->flags, FL_ONGROUND ) )
	{	
		iBodyVolume = pev->velocity.Length(); 

		// clamp the noise that can be made by the body, in case a push trigger,
		// weapon recoil, or anything shoves the player abnormally fast. 
		if ( iBodyVolume > 512 )
		{
			iBodyVolume = 512;
		}
	}
	else
	{
		iBodyVolume = 0;
	}

	if ( pev->button & IN_JUMP )
	{
		iBodyVolume += 100;
	}

// convert player move speed and actions into sound audible by monsters.
	if ( m_iWeaponVolume > iBodyVolume )
	{
		m_iTargetVolume = m_iWeaponVolume;

		// OR in the bits for COMBAT sound if the weapon is being louder than the player. 
		pSound->m_iType |= bits_SOUND_COMBAT;
	}
	else
	{
		m_iTargetVolume = iBodyVolume;
	}

	// decay weapon volume over time so bits_SOUND_COMBAT stays set for a while
	m_iWeaponVolume -= 250 * gpGlobals->frametime;
	if ( m_iWeaponVolume < 0 )
	{
		iVolume = 0;
	}


	// if target volume is greater than the player sound's current volume, we paste the new volume in 
	// immediately. If target is less than the current volume, current volume is not set immediately to the
	// lower volume, rather works itself towards target volume over time. This gives monsters a much better chance
	// to hear a sound, especially if they don't listen every frame.
	iVolume = pSound->m_iVolume;

	if ( m_iTargetVolume > iVolume )
	{
		iVolume = m_iTargetVolume;
	}
	else if ( iVolume > m_iTargetVolume )
	{
		iVolume -= 250 * gpGlobals->frametime;

		if ( iVolume < m_iTargetVolume )
		{
			iVolume = 0;
		}
	}

	if ( m_fNoPlayerSound )
	{
		// debugging flag, lets players move around and shoot without monsters hearing.
		iVolume = 0;
	}

	if ( gpGlobals->time > m_flStopExtraSoundTime )
	{
		// since the extra sound that a weapon emits only lasts for one client frame, we keep that sound around for a server frame or two 
		// after actual emission to make sure it gets heard.
		m_iExtraSoundTypes = 0;
	}

	if ( pSound )
	{
		pSound->m_vecOrigin = pev->origin;
		pSound->m_iType |= ( bits_SOUND_PLAYER | m_iExtraSoundTypes );
		pSound->m_iVolume = iVolume;
	}

	// keep track of virtual muzzle flash
	m_iWeaponFlash -= 256 * gpGlobals->frametime;
	if (m_iWeaponFlash < 0)
		m_iWeaponFlash = 0;

	UTIL_MakeVectors ( pev->angles );
	gpGlobals->v_forward.z = 0;

	// Below are a couple of useful little bits that make it easier to determine just how much noise the 
	// player is making. 
	// UTIL_ParticleEffect ( pev->origin + gpGlobals->v_forward * iVolume, g_vecZero, 255, 25 );
	//ALERT ( at_console, "%d/%d\n", iVolume, m_iTargetVolume );
}


void CBasePlayer::PostThink()
{
	if ( g_fGameOver )
		goto pt_end;         // intermission or finale

	if (!IsAlive())
		goto pt_end;

	// Handle Tank controlling
	if ( m_pTank != NULL )
	{ // if they've moved too far from the gun,  or selected a weapon, unuse the gun
		if ( m_pTank->OnControls( pev ) && !pev->weaponmodel )
		{  
			m_pTank->Use( this, this, USE_SET, 2 );	// try fire the gun
		}
		else
		{  // they've moved off the platform
			m_pTank->Use( this, this, USE_OFF, 0 );
			m_pTank = NULL;
		}
	}

	// Beta 5.0 (leak): Beta 2.0 to 4.1's naming of the player in the crosshair
	// is gone: the status bar names him (UpdateStatusBar)

// do weapon stuff
	ItemPostFrame( );

// check to see if player landed hard enough to make a sound
// falling farther than half of the maximum safe distance, but not as far a max safe distance will
// play a bootscrape sound, and no damage will be inflicted. Fallling a distance shorter than half
// of maximum safe distance will make no sound. Falling farther than max safe distance will play a 
// fallpain sound, and damage will be inflicted based on how far the player fell

	if ( (FBitSet(pev->flags, FL_ONGROUND)) && (pev->health > 0) && m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD )
	{
		// ALERT ( at_console, "%f\n", m_flFallVelocity );

		if (pev->watertype == CONTENT_WATER)
		{
			// Did he hit the world or a non-moving entity?
			// BUG - this happens all the time in water, especially when 
			// BUG - water has current force
			// if ( !pev->groundentity || VARS(pev->groundentity)->velocity.z == 0 )
				// EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade1.wav", 1, ATTN_NORM);
		}
		else if ( m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED )
		{// after this point, we start doing damage
			
			float flFallDamage = g_pGameRules->FlPlayerFallDamage( this );

			if ( flFallDamage > pev->health )
			{//splat
				// note: play on item channel because we play footstep landing on body channel
				EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/bodysplat.wav", 1, ATTN_NORM);
			}

			if ( flFallDamage > 0 )
			{
				TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), flFallDamage, DMG_FALL ); 
				pev->punchangle.x = 0;
			}
		}

		// (Counter-Strike 1.4: SDK 2.3's text, the landing's footstep is no longer played here)

		if ( IsAlive() )
		{
			SetAnimation( PLAYER_WALK );
		}
    }

	if (FBitSet(pev->flags, FL_ONGROUND))
	{		
		if (m_flFallVelocity > 64 && !g_pGameRules->IsMultiplayer())
		{
			CSoundEnt::InsertSound ( bits_SOUND_PLAYER, pev->origin, m_flFallVelocity, 0.2 );
			// ALERT( at_console, "fall %f\n", m_flFallVelocity );
		}
		m_flFallVelocity = 0;
	}

	// select the proper animation for the player character	
	if ( IsAlive() )
	{
		if (!pev->velocity.x && !pev->velocity.y)
		{
			// 1.0: not while the gait is sequence 5
			if ( pev->gaitsequence != 5 )
				SetAnimation( PLAYER_IDLE );
		}
		else if ((pev->velocity.x || pev->velocity.y) && (FBitSet(pev->flags, FL_ONGROUND)))
			SetAnimation( PLAYER_WALK );
		else if (pev->waterlevel > 1)
			SetAnimation( PLAYER_WALK );
	}

	StudioFrameAdvance( );
	CheckPowerups(pev);

	UpdatePlayerSound();

	// (Beta 5.0 (leak) checked the zones here; Beta 5.0: UpdateClientData)

pt_end:
#if defined( CLIENT_WEAPONS )
		// Decay timers on weapons
	// go through all of the weapons and make a list of the ones to pack
	for ( int i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		if ( m_rgpPlayerItems[ i ] )
		{
			CBasePlayerItem *pPlayerItem = m_rgpPlayerItems[ i ];

			while ( pPlayerItem )
			{
				CBasePlayerWeapon *gun;

				gun = (CBasePlayerWeapon *)pPlayerItem->GetWeaponPtr();
				
				if ( gun && gun->UseDecrement() )
				{
					gun->m_flNextPrimaryAttack		= max( gun->m_flNextPrimaryAttack - gpGlobals->frametime, -1.0 );
					gun->m_flNextSecondaryAttack	= max( gun->m_flNextSecondaryAttack - gpGlobals->frametime, -0.001 );

					if ( gun->m_flTimeWeaponIdle != 1000 )
					{
						gun->m_flTimeWeaponIdle		= max( gun->m_flTimeWeaponIdle - gpGlobals->frametime, -0.001 );
					}
				}

				pPlayerItem = pPlayerItem->m_pNext;
			}
		}
	}

	m_flNextAttack -= gpGlobals->frametime;
	if ( m_flNextAttack < -0.001 )
		m_flNextAttack = -0.001;

	// Counter-Strike 1.3: the buttons are kept here, behind pt_end (a dead player's too), and the
	// server animates the gait itself (animation.cpp reads the results for the hit boxes)
	// Track button info so we can detect 'pressed' and 'released' buttons next frame
	m_afButtonLast = pev->button;

	m_iGaitsequence = pev->gaitsequence;

	StudioProcessGait();
#else
	return;
#endif
}


// checks if the spot is clear of players
BOOL IsSpawnPointValid( CBaseEntity *pPlayer, CBaseEntity *pSpot )
{
	CBaseEntity *ent = NULL;

	if ( !pSpot->IsTriggered( pPlayer ) )
	{
		return FALSE;
	}

	while ( (ent = UTIL_FindEntityInSphere( ent, pSpot->pev->origin, 64 )) != NULL )
	{
		// if ent is a client, don't spawn on 'em
		// (Beta 2.1 to 4.1 return TRUE here too, so every spot whose master allows it
		// is valid, occupied or not; Beta 5.0 (leak) has the SDK's FALSE again)
		if ( ent->IsPlayer() && ent != pPlayer )
			return FALSE;
	}

	return TRUE;
}


DLL_GLOBAL CBaseEntity	*g_pLastSpawn;
DLL_GLOBAL CBaseEntity	*g_pLastCTSpawn;			// Beta 4.1 0x1008d538, Beta 5.0 (leak) 0x1008f53c; name ours. world.cpp clears all three
DLL_GLOBAL CBaseEntity	*g_pLastTerroristSpawn;		// Beta 4.1 0x1008d53c, Beta 5.0 (leak) 0x1008f540; name ours
inline int FNullEnt( CBaseEntity *ent ) { return (ent == NULL) || FNullEnt( ent->edict() ); }

/*
============
EntSelectSpawnPoint

Returns the entity to spawn at

USES AND SETS GLOBAL g_pLastSpawn
============
*/
edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer )
{
	CBaseEntity *pSpot;
	edict_t		*player;

	player = pPlayer->edict();

// choose a info_player_deathmatch point
	if (g_pGameRules->IsCoOp())
	{
		pSpot = UTIL_FindEntityByClassname( g_pLastSpawn, "info_player_coop");
		if ( !FNullEnt(pSpot) )
			goto ReturnSpot;
		pSpot = UTIL_FindEntityByClassname( g_pLastSpawn, "info_player_start");
		if ( !FNullEnt(pSpot) ) 
			goto ReturnSpot;
	}
	// Beta 6.0: the VIP starts at the info_vip_start, or as a counter-terrorist
	else if ( g_pGameRules->IsDeathmatch() && ((CBasePlayer *)pPlayer)->m_bIsVIP == true )
	{
		// (Counter-Strike 1.1: the three "Looking for a ... spawn point" console lines of Beta 6.0 are gone)
		pSpot = UTIL_FindEntityByClassname( NULL, "info_vip_start" );
		if ( !FNullEnt( pSpot ) )
			goto ReturnSpot;
		goto CTSpawn;	// label name ours
	}
	// Counter-Terrorists start at info_player_start, terrorists at info_player_deathmatch,
	// each team going round its own points
	else if ( g_pGameRules->IsDeathmatch() && ((CBasePlayer *)pPlayer)->m_iTeam == TEAM_CT )
	{
CTSpawn:
		pSpot = g_pLastCTSpawn;
		pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_start" );
		if ( FNullEnt( pSpot ) )  // skip over the null point
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_start" );

		CBaseEntity *pFirstSpot = pSpot;

		do 
		{
			if ( pSpot )
			{
				// check if pSpot is valid
				if ( IsSpawnPointValid( pPlayer, pSpot ) )
				{
					if ( pSpot->pev->origin == Vector( 0, 0, 0 ) )
					{
						pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_start" );
						continue;
					}

					// if so, go to pSpot
					goto ReturnSpot;
				}
			}
			// increment pSpot
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_start" );
		} while ( pSpot != pFirstSpot ); // loop if we're not back to the start

		// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
		if ( !FNullEnt( pSpot ) )
		{
			CBaseEntity *ent = NULL;
			while ( (ent = UTIL_FindEntityInSphere( ent, pSpot->pev->origin, 64 )) != NULL )	// Beta 5.0 (the leak: 128)
			{
				// if ent is a client, kill em (unless they are ourselves)
				if ( ent->IsPlayer() && !(ent->edict() == player) )
					ent->TakeDamage( VARS(INDEXENT(0)), VARS(INDEXENT(0)), 200, DMG_GENERIC );	// Beta 5.0 (the leak: 300)
			}
			goto ReturnSpot;
		}
	}
	else if ( g_pGameRules->IsDeathmatch() && ((CBasePlayer *)pPlayer)->m_iTeam == TEAM_TERRORIST )
	{
		pSpot = g_pLastTerroristSpawn;
		pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
		if ( FNullEnt( pSpot ) )  // skip over the null point
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );

		CBaseEntity *pFirstSpot = pSpot;

		do 
		{
			if ( pSpot )
			{
				// check if pSpot is valid
				if ( IsSpawnPointValid( pPlayer, pSpot ) )
				{
					if ( pSpot->pev->origin == Vector( 0, 0, 0 ) )
					{
						pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
						continue;
					}

					// if so, go to pSpot
					goto ReturnSpot;
				}
			}
			// increment pSpot
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
		} while ( pSpot != pFirstSpot ); // loop if we're not back to the start

		// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
		if ( !FNullEnt( pSpot ) )
		{
			CBaseEntity *ent = NULL;
			while ( (ent = UTIL_FindEntityInSphere( ent, pSpot->pev->origin, 64 )) != NULL )	// Beta 5.0 (the leak: 128)
			{
				// if ent is a client, kill em (unless they are ourselves)
				if ( ent->IsPlayer() && !(ent->edict() == player) )
					ent->TakeDamage( VARS(INDEXENT(0)), VARS(INDEXENT(0)), 200, DMG_GENERIC );	// Beta 5.0 (the leak: 300)
			}
			goto ReturnSpot;
		}
	}

	// If startspot is set, (re)spawn there.
	if ( FStringNull( gpGlobals->startspot ) || !strlen(STRING(gpGlobals->startspot)))
	{
		pSpot = UTIL_FindEntityByClassname(NULL, "info_player_deathmatch");
		if ( !FNullEnt(pSpot) )
			goto ReturnSpot;
	}
	else
	{
		pSpot = UTIL_FindEntityByTargetname( NULL, STRING(gpGlobals->startspot) );
		if ( !FNullEnt(pSpot) )
			goto ReturnSpot;
	}

ReturnSpot:
	if ( FNullEnt( pSpot ) )
	{
		ALERT(at_error, "PutClientInServer: no info_player_start on level");
		return INDEXENT(0);
	}

	if ( ((CBasePlayer *)pPlayer)->m_iTeam == TEAM_TERRORIST )
		g_pLastTerroristSpawn = pSpot;
	else
		g_pLastCTSpawn = pSpot;

	return pSpot->edict();
}

// Beta 5.0 (leak): the scoreboard's marks of one player as another sees
// them: dead (1), and to his teammates the bomb carrier (2) (the name is
// Counter-Strike 1.6's, and its place ahead of Spawn: retail 1.4 has the
// function there, called by Killed and UpdateClientData)
inline void SetScoreAttrib( CBasePlayer *dest, CBasePlayer *src )
{
	int state = ( src->pev->deadflag != DEAD_NO );

	// Counter-Strike 1.3: the spectators see both marks too
	if ( src->m_bHasC4 && ( dest->m_iTeam == src->m_iTeam || dest->m_iTeam == TEAM_SPECTATOR ) )
		state |= 2;

	// Beta 6.0: the VIP, to his team. Counter-Strike 1.4: to everybody
	if ( src->m_bIsVIP == true )
		state |= 4;

	// 1.0: only once the message is registered
	if ( gmsgScoreAttrib )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgScoreAttrib, NULL, dest->edict() );
			WRITE_BYTE( src->entindex() );
			WRITE_BYTE( state );
		MESSAGE_END();
	}
}

void CBasePlayer::Spawn( void )
{
	// Counter-Strike 1.3: the gait starts afresh
	m_iGaitsequence		= 0;
	m_flGaitframe		= 0;
	m_flGaityaw			= 0;
	m_prevgaitorigin	= Vector( 0, 0, 0 );
	m_flGaitMovement	= 0;

	pev->classname		= MAKE_STRING("player");
	pev->health			= 100;

	// a survivor of the last round keeps his armor and kevlar
	if ( !m_fNotKilled )
	{
		pev->armorvalue		= 0;
		m_iKevlar			= 0;
	}

	pev->maxspeed		= 1000;	// Beta 7.1
	pev->takedamage		= DAMAGE_AIM;
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_WALK;
	pev->max_health		= pev->health;
	pev->flags		   &= FL_PROXY;	// keep proxy flag sey by engine
	pev->flags		   |= FL_CLIENT;
	pev->air_finished	= gpGlobals->time + 12;
	pev->dmg			= 2;				// initial water damage
	pev->effects		= 0;
	pev->deadflag		= DEAD_NO;
	pev->dmg_take		= 0;
	pev->dmg_save		= 0;
	m_bitsHUDDamage		= -1;
	m_bitsDamageType	= 0;
	m_afPhysicsFlags	= 0;
	m_fLongJump			= FALSE;// no longjump module. 
	m_iClientFOV		= -1; // make sure fov reset is sent
	m_pentCurBombTarget	= NULL;
	ResetStamina();		// Counter-Strike 1.4

	// Beta 6.5: SDK 2.0's friction, gravity and physics keys (no long jump;
	// pm_shared's "hl" flag), after Counter-Strike's lines
	pev->friction		= 1.0;
	pev->gravity		= 1.0;

	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", "0" );
	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "hl", "1" );

	m_hintMessageQueue.Reset();

	m_flVelocityModifier = 1.0;
	m_iLastZoom			= 90;
	m_flLastSay			= 0;

	// Beta 3.0: the hostages' trail starts afresh, and 60 radio messages a life
	m_flIdleCheckTime	= 0;
	m_flNextRadio		= 0;
	m_iRadioMessages	= 60;

	// Beta 4.0: no C4, no blast death; Beta 5.0 (leak): the defuse kit is kept
	m_bHasC4			= false;
	m_fKilledByBlast	= false;
	m_fKilledByHEGrenade = false;

	// Beta 5.0 (leak): the hints about the buy key, the dead (multiplay's
	// PlayerKilled), being killed by a teammate and carrying the bomb come again
	m_flDisplayHistory	&= ~( 0x2 | 0x40 | 0x400 | 0x1000 );
	m_flNextZoneCheck	= 0;

	// Beta 5.2: the dead camera starts as mp_chasecam 1 asks (chase), else free
	m_fCamSwitch = 0;
	m_iChaseNext		= 1;

	// Beta 6.0: not escaped, the radar and the kick vote afresh
	m_bEscaped			= false;
	m_flNextRadarUpdate	= gpGlobals->time;
	m_vecLastRadarOrigin = Vector( 0, 0, 0 );
	m_iCurrentKickVote	= 0;
	m_flNextVoteTime	= 0;
	m_bJustKilledTeammate = false;	// Beta 6.1

	// Beta 6.5: no longer an observer (observer.cpp); no shooting in the freeze time
	SET_VIEW( ENT(pev), ENT(pev) );
	m_hObserverTarget = NULL;
	pev->iuser1 = pev->iuser2 = pev->iuser3 = 0;	// Counter-Strike 1.4: iuser3 too (Killed's death camera sets it)
	m_flLastFired = -15;	// Beta 7.0
	m_bHeadshotKilled = false;	// Beta 7.1

	// Counter-Strike 1.4: the icons of the player he watched as an observer go
	if ( m_iObserverC4State )
	{
		m_iObserverC4State = 0;

		MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_STRING( "c4" );
		MESSAGE_END();
	}

	if ( m_bObserverHasDefuser )
	{
		m_bObserverHasDefuser = false;

		MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_STRING( "defuser" );
		MESSAGE_END();
	}

	if ( ((CHalfLifeMultiplay *)g_pGameRules)->IsFreezePeriod() )
		m_bCanShoot = false;
	else
		m_bCanShoot = true;

	m_SbarString0[0]	= 0;
	m_SbarString1[0]	= 0;
	m_iNumSpawns++;

	for ( int i = 0; i < 20; i++ )
		m_vecOldOrigins[i] = Vector( 0, 0, 0 );

	// a sniper rifle hides its view model when zoomed in: show it again
	// (Counter-Strike 1.4: there are no left-handed models on the server any more)
	if ( m_pActiveItem && !pev->viewmodel )
	{
		if ( m_pActiveItem->m_iId == WEAPON_AWP )
			pev->viewmodel = MAKE_STRING( "models/v_awp.mdl" );
		else if ( m_pActiveItem->m_iId == WEAPON_G3SG1 )
			pev->viewmodel = MAKE_STRING( "models/v_g3sg1.mdl" );
		else if ( m_pActiveItem->m_iId == WEAPON_SCOUT )
			pev->viewmodel = MAKE_STRING( "models/v_scout.mdl" );
		else if ( m_pActiveItem->m_iId == WEAPON_SG550 )	// 1.0
			pev->viewmodel = MAKE_STRING( "models/v_sg550.mdl" );
	}

	m_iFOV				= 90;// init field of view.
	m_flNextDecalTime	= 0;// let this player decal as soon as he spawns.

	m_flgeigerDelay = gpGlobals->time + 2.0;	// wait a few seconds until user-defined message registrations
												// are recieved by all clients
	
	m_flTimeStepSound	= 0;
	m_iStepLeft = 0;
	m_flFieldOfView		= 0.5;// some monsters use this to determine whether or not the player is looking at them.

	m_bloodColor	= BLOOD_COLOR_RED;
	m_flNextAttack	= UTIL_WeaponTimeBase();
	StartSneaking();

	m_iFlashBattery = 99;
	m_flFlashLightTime = 1; // force first message

	// Beta 5.0 (leak): a defuse kit shows on the model
	if ( m_fHasDefuser == true )
		pev->body = 1;
	else
		pev->body = 0;

	if ( m_fShowingLevelText == true )
	{
		RemoveLevelText();
		m_fShowingLevelText = false;
	}

// dont let uninitialized value here hurt the player
	m_flFallVelocity = 0;

	g_pGameRules->GetPlayerSpawnSpot( this );

    SET_MODEL(ENT(pev), "models/player.mdl");
    g_ulModelIndexPlayer = pev->modelindex;
	pev->sequence		= LookupActivity( ACT_IDLE );

	if ( FBitSet(pev->flags, FL_DUCKING) ) 
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);

    pev->view_ofs = VEC_VIEW;
	Precache();
	m_HackedGunPos		= Vector( 0, 32, 0 );

	if ( m_iPlayerSound == SOUNDLIST_EMPTY )
	{
		ALERT ( at_console, "Couldn't alloc player sound slot!\n" );
	}

	m_fNoPlayerSound = FALSE;// normal sound behavior.

	m_pLastItem = NULL;
	m_fWeapon = FALSE;
	m_pClientActiveItem = NULL;
	m_iClientBattery = -1;
	m_fInitHUD = TRUE;
	m_iHideHUD &= ~( HIDEHUD_WEAPONS | HIDEHUD_HEALTH | HIDEHUD_TIMER | HIDEHUD_MONEY );

	// and his ammunition
	if ( !m_fNotKilled )
	{
		m_iClientHideHUD = -1;  // force this to be recalculated

		// reset all ammo values to 0
		for ( int i = 0; i < MAX_AMMO_SLOTS; i++ )
			m_rgAmmo[i] = 0;

		// Beta 6.0: no primary weapon, no night vision
		m_bHasPrimary		= false;
		m_bHasNightVision	= false;
	}
	else
	{
		// Counter-Strike 1.4: a survivor's ammunition is all sent again
		for ( int i = 0; i < MAX_AMMO_SLOTS; i++ )
			m_rgAmmoLast[i] = -1;
	}

	// Beta 6.0: the night vision is off at every spawn; Beta 6.1: the
	// client's lighting is put back too
	MESSAGE_BEGIN( MSG_ONE, gmsgNVGToggle, NULL, pev );
		WRITE_BYTE( 0 );
	MESSAGE_END();
	m_bNightVisionOn = false;

	m_lastx = m_lasty = 0;

	g_pGameRules->PlayerSpawn( this );

	m_fNotKilled = true;
	m_bIsDefusing = false;	// Counter-Strike 1.4

	ResetMaxSpeed();

	UTIL_SetOrigin( pev, pev->origin );	// Counter-Strike 1.3

	// Beta 6.0: the VIP: helmet and 200 armour, only the knife (Beta 7.0: and a USP)
	if ( m_bIsVIP == true )
	{
		m_iKevlar = 2;
		pev->armorvalue = 200;
		HintMessage( "#Hint_you_are_the_vip", TRUE, TRUE );	// Counter-Strike 1.0b: bOverride (1.0: FALSE); 1.1: the client's title
		// (Counter-Strike 1.4: his knife and USP come from GiveDefaultItems)
	}

	SetScoreboardAttributes();

	// Beta 6.0: everybody learns his team
	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( entindex() );
		// Beta 6.5: the team's name (Beta 6.1 sent its number)
		if ( m_iTeam == TEAM_CT )
			WRITE_STRING( "CT" );
		else if ( m_iTeam == TEAM_TERRORIST )
			WRITE_STRING( "TERRORIST" );
		else if ( m_iTeam == TEAM_SPECTATOR )	// Counter-Strike 1.3
			WRITE_STRING( "SPECTATOR" );
		else
			WRITE_STRING( "UNASSIGNED" );
	MESSAGE_END();

	// Beta 6.6: and his score, with the team
	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX(edict()) );
		WRITE_SHORT( pev->frags );
		WRITE_SHORT( m_iDeaths );
		WRITE_SHORT( 0 );
		WRITE_SHORT( m_iTeam );
	MESSAGE_END();


	// Counter-Strike 1.4: a name changed during his life is set now (ClientUserInfoChanged keeps
	// it in m_szNewName)
	if ( m_bHasChangedName )
	{
		char *infobuffer = g_engfuncs.pfnGetInfoKeyBuffer( edict() );

		if ( !FStrEq( m_szNewName, g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ) ) )
			g_engfuncs.pfnSetClientKeyValue( entindex(), infobuffer, "name", m_szNewName );

		m_bHasChangedName = false;
		m_szNewName[0] = '\0';
	}

	// Counter-Strike 1.1: Killed's fade to black (mp_fadetoblack) is taken off
	UTIL_ScreenFade( this, Vector( 0, 0, 0 ), 0.001, 0, 0, 0 );

	// Counter-Strike 1.4: and the round's clock is sent
	SyncRoundTimer();
}


void CBasePlayer :: Precache( void )
{
	// in the event that the player JUST spawned, and the level node graph
	// was loaded, fix all of the node graph pointers before the game starts.
	
	// !!!BUGBUG - now that we have multiplayer, this needs to be moved!
	if ( WorldGraph.m_fGraphPresent && !WorldGraph.m_fGraphPointersSet )
	{
		if ( !WorldGraph.FSetGraphPointers() )
		{
			ALERT ( at_console, "**Graph pointers were not set!\n");
		}
		else
		{
			ALERT ( at_console, "**Graph Pointers Set!\n" );
		} 
	}

	// SOUNDS / MODELS ARE PRECACHED in ClientPrecache() (game specific)
	// because they need to precache before any clients have connected

	// init geiger counter vars during spawn and each time
	// we cross a level transition

	m_flgeigerRange = 1000;
	m_igeigerRangePrev = 1000;

	m_bitsDamageType = 0;
	m_bitsHUDDamage = -1;

	m_iClientBattery = -1;

	m_iTrain = TRAIN_NEW;

	// Make sure any necessary user messages have been registered
	LinkUserMessages();

	m_iUpdateTime = 5;  // won't update for 1/2 a second

	if ( gInitHUD )
		m_fInitHUD = TRUE;
}


int CBasePlayer::Save( CSave &save )
{
	if ( !CBaseMonster::Save(save) )
		return 0;

	return save.WriteFields( "PLAYER", this, m_playerSaveData, ARRAYSIZE(m_playerSaveData) );
}


// Beta 5.0 (leak): this player's marks to one player, or to all
void CBasePlayer::SetScoreboardAttributes( CBasePlayer *destination )
{
	if ( destination )
	{
		SetScoreAttrib( destination, this );
		return;
	}

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );

		// Beta 5.0: joining players too (the leak: IsPlayer only)
		if ( pPlayer && !FNullEnt( pPlayer->edict() ) )
			SetScoreAttrib( pPlayer, this );
	}
}

//
// Marks everything as new so the player will resend this to the hud.
//
void CBasePlayer::RenewItems(void)
{

}


int CBasePlayer::Restore( CRestore &restore )
{
	if ( !CBaseMonster::Restore(restore) )
		return 0;

	int status = restore.ReadFields( "PLAYER", this, m_playerSaveData, ARRAYSIZE(m_playerSaveData) );

	SAVERESTOREDATA *pSaveData = (SAVERESTOREDATA *)gpGlobals->pSaveData;
	// landmark isn't present.
	if ( !pSaveData->fUseLandmark )
	{
		ALERT( at_console, "No Landmark:%s\n", pSaveData->szLandmarkName );

		// default to normal spawn
		edict_t* pentSpawnSpot = EntSelectSpawnPoint( this );
		pev->origin = VARS(pentSpawnSpot)->origin + Vector(0,0,1);
		pev->angles = VARS(pentSpawnSpot)->angles;
	}
	pev->v_angle.z = 0;	// Clear out roll
	pev->angles = pev->v_angle;

	pev->fixangle = TRUE;           // turn this way immediately

// Copied from spawn() for now
	m_bloodColor	= BLOOD_COLOR_RED;

    g_ulModelIndexPlayer = pev->modelindex;

	if ( FBitSet(pev->flags, FL_DUCKING) ) 
	{
		// Use the crouch HACK
		// FixPlayerCrouchStuck( edict() );
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	}
	else
	{
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
	}

	RenewItems();

	// Beta 5.0 (leak): after a saved game most hints come again
	// (Counter-Strike 1.4: 0x200000 too)
	m_flDisplayHistory &= ~( 0x4 | 0x8 | 0x10 | 0x20 | 0x80 | 0x100 | 0x200 | 0x800 | 0x8000 | 0x10000 | 0x20000 | 0x40000 | 0x80000 | 0x200000 );	// Beta 5.0 to 5.2: 0x40000 stayed; Beta 6.0: the escape and VIP zone hints (0x40000, 0x80000) come again too
	SetScoreboardAttributes();

	// (1.0 to 1.3 read the user info key vgui_menus here; SetPrefsFromUserinfo does that in 1.4)
	return status;
}

// Beta 5.0 (Linux symbol): a complete reset (sv_restartround) takes the
// player's score and deaths to 0 and his money to the start's $800
void CBasePlayer::Reset( void )
{
	pev->frags = 0;
	m_iDeaths = 0;
	m_iAccount = 0;
	AddAccount( -16000, false );

	// Counter-Strike 1.3: his ammunition goes at the next spawn (Beta 6.5 to 1.1: no armour)
	m_fNotKilled = false;

	// Beta 6.0: a counter-terrorist on an escape map starts with 3000 (sic: the
	// console says the map has no escape zone for a terrorist on one).
	// Counter-Strike 1.3: everybody starts with mp_startmoney (1.1: 3000 and 800)
	// Counter-Strike 1.4: the money stands once, behind the two console lines
	if ( UTIL_FindEntityByClassname( NULL, "func_escapezone" ) && m_iTeam == TEAM_CT )
		ALERT( at_console, "This map has an escape zone\n" );
	else
		ALERT( at_console, "This map DOES NOT have an escape zone\n" );

	CheckStartMoney();
	AddAccount( (int)startmoney.value, true );

	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX(edict()) );
		WRITE_SHORT( 0 );
		WRITE_SHORT( 0 );
		WRITE_SHORT( 0 );			// Beta 6.6
		WRITE_SHORT( m_iTeam );
	MESSAGE_END();
}


void CBasePlayer::SelectNextItem( int iItem )
{
	CBasePlayerItem *pItem;

	pItem = m_rgpPlayerItems[ iItem ];
	
	if (!pItem)
		return;

	if (pItem == m_pActiveItem)
	{
		// select the next one in the chain
		pItem = m_pActiveItem->m_pNext; 
		if (! pItem)
		{
			return;
		}

		CBasePlayerItem *pLast;
		pLast = pItem;
		while (pLast->m_pNext)
			pLast = pLast->m_pNext;

		// relink chain
		pLast->m_pNext = m_pActiveItem;
		m_pActiveItem->m_pNext = NULL;
		m_rgpPlayerItems[ iItem ] = pItem;
	}

	ResetAutoaim( );

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem)
	{
		m_pActiveItem->Holster( );
	}
	
	m_pActiveItem = pItem;

	if (m_pActiveItem)
	{
		m_pActiveItem->Deploy( );
		m_pActiveItem->UpdateItemInfo( );
		ResetMaxSpeed( );	// Beta 5.0 (leak)
	}
}

void CBasePlayer::SelectItem(const char *pstr)
{
	if (!pstr)
		return;

	CBasePlayerItem *pItem = NULL;

	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			pItem = m_rgpPlayerItems[i];
	
			while (pItem)
			{
				if (FClassnameIs(pItem->pev, pstr))
					break;
				pItem = pItem->m_pNext;
			}
		}

		if (pItem)
			break;
	}

	if (!pItem)
		return;

	
	if (pItem == m_pActiveItem)
		return;

	ResetAutoaim( );

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem)
		m_pActiveItem->Holster( );
	
	m_pLastItem = m_pActiveItem;
	m_pActiveItem = pItem;

	if (m_pActiveItem)
	{
		m_pActiveItem->Deploy( );
		m_pActiveItem->UpdateItemInfo( );
		ResetMaxSpeed( );	// Beta 5.0 (leak)
	}
}


void CBasePlayer::SelectLastItem(void)
{
	if (!m_pLastItem)
	{
		return;
	}

	if ( m_pActiveItem && !m_pActiveItem->CanHolster() )
	{
		return;
	}

	ResetAutoaim( );

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem)
		m_pActiveItem->Holster( );
	
	CBasePlayerItem *pTemp = m_pActiveItem;
	m_pActiveItem = m_pLastItem;
	m_pLastItem = pTemp;
	m_pActiveItem->Deploy( );
	m_pActiveItem->UpdateItemInfo( );
	ResetMaxSpeed( );	// Beta 5.0 (leak)
}

//==============================================
// HasWeapons - do I have any weapons at all?
//==============================================
BOOL CBasePlayer::HasWeapons( void )
{
	int i;

	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		if ( m_rgpPlayerItems[ i ] )
		{
			return TRUE;
		}
	}

	return FALSE;
}

void CBasePlayer::SelectPrevItem( int iItem )
{
}


const char *CBasePlayer::TeamID( void )
{
	if ( pev == NULL )		// Not fully connected yet
		return "";

	// return their team name
	return m_szTeamName;
}


//==============================================
// !!!UNDONE:ultra temporary SprayCan entity to apply
// decal frame at a time. For PreAlpha CD
//==============================================
class CSprayCan : public CBaseEntity
{
public:
	void	Spawn ( entvars_t *pevOwner );
	void	Think( void );

	virtual int	ObjectCaps( void ) { return FCAP_DONT_SAVE; }
};

void CSprayCan::Spawn ( entvars_t *pevOwner )
{
	pev->origin = pevOwner->origin + Vector ( 0 , 0 , 32 );
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT(pevOwner);
	pev->frame = 0;

	pev->nextthink = gpGlobals->time + 0.1;
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/sprayer.wav", 1, ATTN_NORM);
}

void CSprayCan::Think( void )
{
	TraceResult	tr;	
	int playernum;
	int nFrames;
	CBasePlayer *pPlayer;
	
	pPlayer = (CBasePlayer *)GET_PRIVATE(pev->owner);

	if (pPlayer)
		nFrames = pPlayer->GetCustomDecalFrames();
	else
		nFrames = -1;

	playernum = ENTINDEX(pev->owner);
	
	// ALERT(at_console, "Spray by player %i, %i of %i\n", playernum, (int)(pev->frame + 1), nFrames);

	UTIL_MakeVectors(pev->angles);
	UTIL_TraceLine ( pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, & tr);

	// No customization present.
	if (nFrames == -1)
	{
		UTIL_DecalTrace( &tr, DECAL_LAMBDA6 );
		UTIL_Remove( this );
	}
	else
	{
		UTIL_PlayerDecalTrace( &tr, playernum, pev->frame, TRUE );
		// Just painted last custom frame.
		if ( pev->frame++ >= (nFrames - 1))
			UTIL_Remove( this );
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

class	CBloodSplat : public CBaseEntity
{
public:
	void	Spawn ( entvars_t *pevOwner );
	void	Spray ( void );
};

void CBloodSplat::Spawn ( entvars_t *pevOwner )
{
	pev->origin = pevOwner->origin + Vector ( 0 , 0 , 32 );
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT(pevOwner);

	SetThink ( Spray );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBloodSplat::Spray ( void )
{
	TraceResult	tr;	
	
	if ( g_Language != LANGUAGE_GERMAN )
	{
		UTIL_MakeVectors(pev->angles);
		UTIL_TraceLine ( pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, & tr);

		UTIL_BloodDecalTrace( &tr, BLOOD_COLOR_RED );
	}
	SetThink ( SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;
}

//==============================================



void CBasePlayer::GiveNamedItem( const char *pszName )
{
	edict_t	*pent;

	int istr = MAKE_STRING(pszName);

	pent = CREATE_NAMED_ENTITY(istr);
	if ( FNullEnt( pent ) )
	{
		ALERT ( at_console, "NULL Ent in GiveNamedItem!\n" );
		return;
	}
	VARS( pent )->origin = pev->origin;
	pent->v.spawnflags |= SF_NORESPAWN;

	DispatchSpawn( pent );
	DispatchTouch( pent, ENT( pev ) );
}



CBaseEntity *FindEntityForward( CBaseEntity *pMe )
{
	TraceResult tr;

	UTIL_MakeVectors(pMe->pev->v_angle);
	UTIL_TraceLine(pMe->pev->origin + pMe->pev->view_ofs,pMe->pev->origin + pMe->pev->view_ofs + gpGlobals->v_forward * 8192,dont_ignore_monsters, pMe->edict(), &tr );
	if ( tr.flFraction != 1.0 && !FNullEnt( tr.pHit) )
	{
		CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
		return pHit;
	}
	return NULL;
}


BOOL CBasePlayer :: FlashlightIsOn( void )
{
	return FBitSet(pev->effects, EF_DIMLIGHT);
}


void CBasePlayer :: FlashlightTurnOn( void )
{
	if ( !g_pGameRules->FAllowFlashlight() )
	{
		return;
	}

	if ( (pev->weapons & (1<<WEAPON_SUIT)) )
	{
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, SOUND_FLASHLIGHT_ON, 1.0, ATTN_NORM, 0, PITCH_NORM );
		SetBits(pev->effects, EF_DIMLIGHT);
		MESSAGE_BEGIN( MSG_ONE, gmsgFlashlight, NULL, pev );
		WRITE_BYTE(1);
		WRITE_BYTE(m_iFlashBattery);
		MESSAGE_END();

		m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->time;

	}
}


void CBasePlayer :: FlashlightTurnOff( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, SOUND_FLASHLIGHT_OFF, 1.0, ATTN_NORM, 0, PITCH_NORM );
    ClearBits(pev->effects, EF_DIMLIGHT);
	MESSAGE_BEGIN( MSG_ONE, gmsgFlashlight, NULL, pev );
	WRITE_BYTE(0);
	WRITE_BYTE(m_iFlashBattery);
	MESSAGE_END();

	m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->time;

}

/*
===============
ForceClientDllUpdate

When recording a demo, we need to have the server tell us the entire client state
so that the client side .dll can behave correctly.
Reset stuff so that the state is transmitted.
===============
*/
void CBasePlayer :: ForceClientDllUpdate( void )
{
	m_iClientHealth  = -1;
	m_iClientBattery = -1;
	m_iTrain |= TRAIN_NEW;  // Force new train message.
	m_fWeapon = FALSE;          // Force weapon send
	m_fKnownItem = FALSE;    // Force weaponinit messages.
	m_fInitHUD = TRUE;		// Force HUD gmsgResetHUD message

	// Now force all the necessary messages
	//  to be sent.
	UpdateClientData();
}

/*
============
ImpulseCommands
============
*/
extern float g_flWeaponCheat;

void CBasePlayer::ImpulseCommands( )
{
	TraceResult	tr;// UNDONE: kill me! This is temporary for PreAlpha CDs

	// Handle use events
	PlayerUse();
		
	int iImpulse = (int)pev->impulse;
	switch (iImpulse)
	{
	case 99:
		{

		int iOn;

		if (!gmsgLogo)
		{
			iOn = 1;
			gmsgLogo = REG_USER_MSG("Logo", 1);
		} 
		else 
		{
			iOn = 0;
		}
		
		ASSERT( gmsgLogo > 0 );
		// send "health" update message
		MESSAGE_BEGIN( MSG_ONE, gmsgLogo, NULL, pev );
			WRITE_BYTE(iOn);
		MESSAGE_END();

		if(!iOn)
			gmsgLogo = 0;
		break;
		}
	case 100:
        // temporary flashlight for level designers
        if ( FlashlightIsOn() )
		{
			FlashlightTurnOff();
		}
        else 
		{
			FlashlightTurnOn();
		}
		break;

	case	201:// paint decal
		
		if ( gpGlobals->time < m_flNextDecalTime )
		{
			// too early!
			break;
		}

		UTIL_MakeVectors(pev->v_angle);
		UTIL_TraceLine ( pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, ENT(pev), & tr);

		if ( tr.flFraction != 1.0 )
		{// line hit something, so paint a decal
			m_flNextDecalTime = gpGlobals->time + CVAR_GET_FLOAT("decalfrequency");
			CSprayCan *pCan = GetClassPtr((CSprayCan *)NULL);
			pCan->Spawn( pev );
		}

		break;
	// (Counter-Strike 1.4: impulse 204, the demo's ForceClientDllUpdate, is not handled here any more)
	default:
		// check all of the cheat impulse commands now
		CheatImpulseCommands( iImpulse );
		break;
	}
	
	pev->impulse = 0;
}

//=========================================================
//=========================================================
void CBasePlayer::CheatImpulseCommands( int iImpulse )
{
#if !defined( HLDEMO_BUILD )
	if ( g_flWeaponCheat == 0.0 )
	{
		return;
	}

	CBaseEntity *pEntity;
	TraceResult tr;

	switch ( iImpulse )
	{
	case 76:
		{
			if (!giPrecacheGrunt)
			{
				giPrecacheGrunt = 1;
				ALERT(at_console, "You must now restart to use Grunt-o-matic.\n");
			}
			else
			{
				UTIL_MakeVectors( Vector( 0, pev->v_angle.y, 0 ) );
				Create("monster_human_grunt", pev->origin + gpGlobals->v_forward * 128, pev->angles);
			}
			break;
		}

	case 101:
		// Beta 5.0 (leak): the money cheat
		gEvilImpulse101 = TRUE;
		AddAccount( 16000, true );
		ALERT( at_console, "Crediting %s with $16000\n", STRING( pev->netname ) );
		break;


	case 102:
		// Gibbage!!!
		CGib::SpawnRandomGibs( pev, 1, 1 );
		break;

	case 103:
		// What the hell are you doing?
		pEntity = FindEntityForward( this );
		if ( pEntity )
		{
			CBaseMonster *pMonster = pEntity->MyMonsterPointer();
			if ( pMonster )
				pMonster->ReportAIState();
		}
		break;

	case 104:
		// Dump all of the global state varaibles (and global entity names)
		gGlobalState.DumpGlobals();
		break;

	case	105:// player makes no sound for monsters to hear.
		{
			if ( m_fNoPlayerSound )
			{
				ALERT ( at_console, "Player is audible\n" );
				m_fNoPlayerSound = FALSE;
			}
			else
			{
				ALERT ( at_console, "Player is silent\n" );
				m_fNoPlayerSound = TRUE;
			}
			break;
		}

	case 106:
		// Give me the classname and targetname of this entity.
		pEntity = FindEntityForward( this );
		if ( pEntity )
		{
			ALERT ( at_console, "Classname: %s", STRING( pEntity->pev->classname ) );
			
			if ( !FStringNull ( pEntity->pev->targetname ) )
			{
				ALERT ( at_console, " - Targetname: %s\n", STRING( pEntity->pev->targetname ) );
			}
			else
			{
				ALERT ( at_console, " - TargetName: No Targetname\n" );
			}

			ALERT ( at_console, "Model: %s\n", STRING( pEntity->pev->model ) );
			if ( pEntity->pev->globalname )
				ALERT ( at_console, "Globalname: %s\n", STRING( pEntity->pev->globalname ) );
		}
		break;

	case 107:
		{
			TraceResult tr;

			edict_t		*pWorld = g_engfuncs.pfnPEntityOfEntIndex( 0 );

			Vector start = pev->origin + pev->view_ofs;
			Vector end = start + gpGlobals->v_forward * 1024;
			UTIL_TraceLine( start, end, ignore_monsters, edict(), &tr );
			if ( tr.pHit )
				pWorld = tr.pHit;
			const char *pTextureName = TRACE_TEXTURE( pWorld, start, end );
			if ( pTextureName )
				ALERT( at_console, "Texture: %s\n", pTextureName );
		}
		break;
	case	195:// show shortest paths for entire level to nearest node
		{
			Create("node_viewer_fly", pev->origin, pev->angles);
		}
		break;
	case	196:// show shortest paths for entire level to nearest node
		{
			Create("node_viewer_large", pev->origin, pev->angles);
		}
		break;
	case	197:// show shortest paths for entire level to nearest node
		{
			Create("node_viewer_human", pev->origin, pev->angles);
		}
		break;
	case	199:// show nearest node and all connections
		{
			ALERT ( at_console, "%d\n", WorldGraph.FindNearestNode ( pev->origin, bits_NODE_GROUP_REALM ) );
			WorldGraph.ShowNodeConnections ( WorldGraph.FindNearestNode ( pev->origin, bits_NODE_GROUP_REALM ) );
		}
		break;
	case	202:// Random blood splatter
		UTIL_MakeVectors(pev->v_angle);
		UTIL_TraceLine ( pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, ENT(pev), & tr);

		if ( tr.flFraction != 1.0 )
		{// line hit something, so paint a decal
			CBloodSplat *pBlood = GetClassPtr((CBloodSplat *)NULL);
			pBlood->Spawn( pev );
		}
		break;
	case	203:// remove creature.
		pEntity = FindEntityForward( this );
		if ( pEntity )
		{
			if ( pEntity->pev->takedamage )
				pEntity->SetThink(SUB_Remove);
		}
		break;
	}
#endif	// HLDEMO_BUILD
}

// Beta 5.0 (leak): the buy zone's icon comes up (name ours)
inline void BuyZoneIcon_Set( CBasePlayer *player )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, player->pev );
		WRITE_BYTE( 1 );
		WRITE_STRING( "buyzone" );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 160 );
		WRITE_BYTE( 0 );
	MESSAGE_END();
}

// Beta 5.0 (leak): at a bomb target the C4 icon flashes, and the carrier is
// told once how to plant (name ours)
inline void BombTargetFlash_Set( CBasePlayer *player )
{
	if ( player->m_bHasC4 && !( player->m_flDisplayHistory & 0x10000 ) )	// in the target zone
	{
		player->m_flDisplayHistory |= 0x10000;
		player->HintMessage( "#Hint_you_are_in_targetzone", FALSE, FALSE );	// Counter-Strike 1.1: the zone hints are titles
	}

	player->SetBombIcon( TRUE );
}

// name ours
inline void BombTargetFlash_Clear( CBasePlayer *player )
{
	player->SetBombIcon( FALSE );
}

// Beta 5.0: the rescue zone's icon comes up, and a CT is told once what it
// is for (name ours)
inline void RescueZoneIcon_Set( CBasePlayer *player )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, player->pev );
		WRITE_BYTE( 1 );
		WRITE_STRING( "rescue" );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 160 );
		WRITE_BYTE( 0 );
	MESSAGE_END();

	if ( player->m_iTeam == TEAM_CT && !( player->m_flDisplayHistory & 0x20000 ) )	// in a rescue zone
	{
		player->m_flDisplayHistory |= 0x20000;
		player->HintMessage( "#Hint_hostage_rescue_zone", FALSE, FALSE );
	}
}

// Beta 5.0: the rescue zone's icon goes, and with it an open buy menu (sic:
// BuyZoneIcon_Clear's text; name ours)
inline void RescueZoneIcon_Clear( CBasePlayer *player )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, player->pev );
		WRITE_BYTE( 0 );
		WRITE_STRING( "rescue" );
	MESSAGE_END();

	if ( player->m_iMenu >= Menu_Buy && player->m_iMenu <= Menu_BuyItem )
		CLIENT_COMMAND( player->edict(), "slot10\n" );
}

// Beta 6.0: the escape zone's icon, and a CT is told once what it is for
// (name ours)
inline void EscapeZoneIcon_Set( CBasePlayer *player )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, player->pev );
		WRITE_BYTE( 1 );
		WRITE_STRING( "escape" );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 160 );
		WRITE_BYTE( 0 );
	MESSAGE_END();

	if ( player->m_iTeam == TEAM_CT && !( player->m_flDisplayHistory & 0x40000 ) )	// in an escape zone
	{
		player->m_flDisplayHistory |= 0x40000;
		player->HintMessage( "#Hint_terrorist_escape_zone", FALSE, FALSE );
	}
}

// Beta 6.0: the escape zone's icon goes, and an open buy menu (sic: as
// RescueZoneIcon_Clear; name ours)
inline void EscapeZoneIcon_Clear( CBasePlayer *player )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, player->pev );
		WRITE_BYTE( 0 );
		WRITE_STRING( "escape" );
	MESSAGE_END();

	if ( player->m_iMenu >= Menu_Buy && player->m_iMenu <= Menu_BuyItem )
		CLIENT_COMMAND( player->edict(), "slot10\n" );
}

// Beta 6.0: the VIP safety zone's icon, and each team is told once what it is
// for (name ours)
inline void VIP_SafetyZoneIcon_Set( CBasePlayer *player )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, player->pev );
		WRITE_BYTE( 1 );
		WRITE_STRING( "vipsafety" );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 160 );
		WRITE_BYTE( 0 );
	MESSAGE_END();

	if ( player->m_iTeam == TEAM_CT && !( player->m_flDisplayHistory & 0x80000 ) )	// in a VIP safety zone
	{
		player->m_flDisplayHistory |= 0x80000;
		player->HintMessage( "#Hint_ct_vip_zone", TRUE, FALSE );
	}
	else if ( player->m_iTeam == TEAM_TERRORIST && !( player->m_flDisplayHistory & 0x80000 ) )
	{
		player->m_flDisplayHistory |= 0x80000;
		player->HintMessage( "#Hint_terrorist_vip_zone", TRUE, FALSE );
	}
}

// Beta 6.0: the VIP safety zone's icon goes, and an open buy menu (sic: as
// RescueZoneIcon_Clear; name ours)
inline void VIP_SafetyZoneIcon_Clear( CBasePlayer *player )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, player->pev );
		WRITE_BYTE( 0 );
		WRITE_STRING( "vipsafety" );
	MESSAGE_END();

	if ( player->m_iMenu >= Menu_Buy && player->m_iMenu <= Menu_BuyItem )
		CLIENT_COMMAND( player->edict(), "slot10\n" );
}

// Beta 5.0 (leak): a map without buy zones has one within 512 units (Beta 6.0:
// 200) of each spawn point of the player's team
static void OLD_CheckBuyZone( CBasePlayer *player )
{
	const char *pszSpawnClass = NULL;

	switch ( player->m_iTeam )
	{
	case TEAM_TERRORIST:	pszSpawnClass = "info_player_deathmatch"; break;
	case TEAM_CT:			pszSpawnClass = "info_player_start"; break;
	}

	if ( pszSpawnClass )
	{
		CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, pszSpawnClass );

		while ( pSpot )
		{
			if ( ( pSpot->pev->origin - player->pev->origin ).Length() < 200 )	// Beta 6.0: 200 (Beta 5.0 to 5.2: 512)
			{
				player->m_signals.Signal( 1 );	// in a buy zone
				break;
			}

			pSpot = UTIL_FindEntityByClassname( pSpot, pszSpawnClass );
		}
	}
}

// Beta 5.0 (leak): a map without func_bomb_target has a target within 256
// units of each info_bomb_target
static void OLD_CheckBombTarget( CBasePlayer *player )
{
	CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "info_bomb_target" );

	while ( pSpot )
	{
		if ( ( pSpot->pev->origin - player->pev->origin ).Length() <= 256 )
		{
			player->m_signals.Signal( 2 );	// at a bomb target
			break;
		}

		pSpot = UTIL_FindEntityByClassname( pSpot, "info_bomb_target" );
	}
}

// Beta 5.0 (Linux symbol): a map without func_hostage_rescue has a rescue
// zone within 256 units of each info_hostage_rescue
static void OLD_CheckRescueZone( CBasePlayer *player )
{
	CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "info_hostage_rescue" );

	while ( pSpot )
	{
		if ( ( pSpot->pev->origin - player->pev->origin ).Length() <= 256 )
		{
			player->m_signals.Signal( 4 );	// in a rescue zone
			break;
		}

		pSpot = UTIL_FindEntityByClassname( pSpot, "info_hostage_rescue" );
	}
}

// Beta 5.0 (leak): PostThink's zone check: what the zones signalled since the
// last check is the new state; the icons follow what changed
void CBasePlayer::HandleSignals( void )
{
	if ( g_pGameRules->IsMultiplayer() )
	{
		CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;

		if ( !mp->m_fMapHasBuyZone )
			OLD_CheckBuyZone( this );

		if ( !mp->m_fMapHasBombZone )
			OLD_CheckBombTarget( this );

		if ( !mp->m_fMapHasRescueZone )	// Beta 5.0
			OLD_CheckRescueZone( this );
	}

	int changed = m_signals.Update();
	int state = m_signals.GetState();

	if ( changed & 1 )	// buy zone
	{
		if ( state & 1 )
			BuyZoneIcon_Set( this );
		else
			BuyZoneIcon_Clear( this );
	}

	if ( changed & 2 )	// bomb target
	{
		if ( state & 2 )
			BombTargetFlash_Set( this );
		else
			BombTargetFlash_Clear( this );
	}

	if ( changed & 4 )	// rescue zone (Beta 5.0)
	{
		if ( state & 4 )
			RescueZoneIcon_Set( this );
		else
			RescueZoneIcon_Clear( this );
	}

	if ( changed & 8 )	// escape zone (Beta 6.0)
	{
		if ( state & 8 )
			EscapeZoneIcon_Set( this );
		else
			EscapeZoneIcon_Clear( this );
	}

	if ( changed & 16 )	// VIP safety zone (Beta 6.0)
	{
		if ( state & 16 )
			VIP_SafetyZoneIcon_Set( this );
		else
			VIP_SafetyZoneIcon_Clear( this );
	}
}

//
// Add a weapon to the player (Item == Weapon == Selectable Object)
//
int CBasePlayer::AddPlayerItem( CBasePlayerItem *pItem )
{
	CBasePlayerItem *pInsert;
	
	pInsert = m_rgpPlayerItems[pItem->iItemSlot()];

	while (pInsert)
	{
		if (FClassnameIs( pInsert->pev, STRING( pItem->pev->classname) ))
		{
			if (pItem->AddDuplicate( pInsert ))
			{
				g_pGameRules->PlayerGotWeapon ( this, pItem );
				pItem->CheckRespawn();

				// ugly hack to update clip w/o an update clip message
				pInsert->UpdateItemInfo( );
				if (m_pActiveItem)
					m_pActiveItem->UpdateItemInfo( );

				pItem->Kill( );
			}
			else if (gEvilImpulse101)
			{
				// FIXME: remove anyway for deathmatch testing
				pItem->Kill( );
			}
			return FALSE;
		}
		pInsert = pInsert->m_pNext;
	}


	if (pItem->AddToPlayer( this ))
	{
		g_pGameRules->PlayerGotWeapon ( this, pItem );

		// Beta 6.0: a weapon of the first slot is the primary one
		if ( pItem->iItemSlot() == 1 )
			m_bHasPrimary = true;

		pItem->CheckRespawn();

		pItem->m_pNext = m_rgpPlayerItems[pItem->iItemSlot()];
		m_rgpPlayerItems[pItem->iItemSlot()] = pItem;

		// should we switch to this item?
		if ( g_pGameRules->FShouldSwitchWeapon( this, pItem ) )
		{
			SwitchWeapon( pItem );
		}

		return TRUE;
	}
	else if (gEvilImpulse101)
	{
		// FIXME: remove anyway for deathmatch testing
		pItem->Kill( );
	}
	return FALSE;
}



int CBasePlayer::RemovePlayerItem( CBasePlayerItem *pItem )
{
	if (m_pActiveItem == pItem)
	{
		ResetAutoaim( );
		// Counter-Strike Beta 5.0: no Holster here (CWeaponBox::PackWeapon holsters
		// the weapon in hand before it takes it)
		pItem->pev->nextthink = 0;// crowbar may be trying to swing again, etc.
		pItem->SetThink( NULL );
		m_pActiveItem = NULL;
		pev->viewmodel = 0;
		pev->weaponmodel = 0;
	}
	else if ( m_pLastItem == pItem )
		m_pLastItem = NULL;

	CBasePlayerItem *pPrev = m_rgpPlayerItems[pItem->iItemSlot()];

	if (pPrev == pItem)
	{
		m_rgpPlayerItems[pItem->iItemSlot()] = pItem->m_pNext;
		return TRUE;
	}
	else
	{
		while (pPrev && pPrev->m_pNext != pItem)
		{
			pPrev = pPrev->m_pNext;
		}
		if (pPrev)
		{
			pPrev->m_pNext = pItem->m_pNext;
			return TRUE;
		}
	}
	return FALSE;
}


//
// Returns the unique ID for the ammo, or -1 if error
//
int CBasePlayer :: GiveAmmo( int iCount, char *szName, int iMax )
{
	// spectators pick nothing up (Counter-Strike 1.4: the flag is tested; to 1.3 the flags were
	// compared with it)
	if ( pev->flags & FL_SPECTATOR )
		return -1;

	if ( !szName )
	{
		// no ammo.
		return -1;
	}

	if ( !g_pGameRules->CanHaveAmmo( this, szName, iMax ) )
	{
		// game rules say I can't have any more of this ammo type.
		return -1;
	}

	int i = 0;

	i = GetAmmoIndex( szName );

	if ( i < 0 || i >= MAX_AMMO_SLOTS )
		return -1;

	int iAdd = min( iCount, iMax - m_rgAmmo[i] );
	if ( iAdd < 1 )
		return i;

	m_rgAmmo[ i ] += iAdd;


	if ( gmsgAmmoPickup )  // make sure the ammo messages have been linked first
	{
		// Send the message that ammo has been picked up
		MESSAGE_BEGIN( MSG_ONE, gmsgAmmoPickup, NULL, pev );
			WRITE_BYTE( GetAmmoIndex(szName) );		// ammo ID
			WRITE_BYTE( iAdd );		// amount
		MESSAGE_END();
	}

	TabulateAmmo();

	return i;
}


/*
============
ItemPreFrame

Called every frame by the player PreThink
============
*/
void CBasePlayer::ItemPreFrame()
{
#if defined( CLIENT_WEAPONS )
    if ( m_flNextAttack > 0 )
#else
    if ( gpGlobals->time < m_flNextAttack )
#endif
	{
		return;
	}

	if (!m_pActiveItem)
		return;

	m_pActiveItem->ItemPreFrame( );
}


/*
============
ItemPostFrame

Called every frame by the player PostThink
============
*/
void CBasePlayer::ItemPostFrame()
{
	static int fInSelect = FALSE;

	// check if the player is using a tank
	if ( m_pTank != NULL )
		return;

 #if defined( CLIENT_WEAPONS )
    if ( m_flNextAttack > 0 )
#else
    if ( gpGlobals->time < m_flNextAttack )
#endif
	{
		return;
	}

	ImpulseCommands();

	if (!m_pActiveItem)
		return;

	m_pActiveItem->ItemPostFrame( );
}

int CBasePlayer::AmmoInventory( int iAmmoIndex )
{
	if (iAmmoIndex == -1)
	{
		return -1;
	}

	return m_rgAmmo[ iAmmoIndex ];
}

int CBasePlayer::GetAmmoIndex(const char *psz)
{
	int i;

	if (!psz)
		return -1;

	for (i = 1; i < MAX_AMMO_SLOTS; i++)
	{
		if ( !CBasePlayerItem::AmmoInfoArray[i].pszName )
			continue;

		if (stricmp( psz, CBasePlayerItem::AmmoInfoArray[i].pszName ) == 0)
			return i;
	}

	return -1;
}

// Called from UpdateClientData
// makes sure the client has all the necessary ammo info,  if values have changed
void CBasePlayer::SendAmmoUpdate(void)
{
	for (int i=0; i < MAX_AMMO_SLOTS;i++)
	{
		if (m_rgAmmo[i] != m_rgAmmoLast[i])
		{
			m_rgAmmoLast[i] = m_rgAmmo[i];

			ASSERT( m_rgAmmo[i] >= 0 );
			ASSERT( m_rgAmmo[i] < 255 );

			// send "Ammo" update message
			MESSAGE_BEGIN( MSG_ONE, gmsgAmmoX, NULL, pev );
				WRITE_BYTE( i );
				WRITE_BYTE( max( min( m_rgAmmo[i], 254 ), 0 ) );  // clamp the value to one byte
			MESSAGE_END();
		}
	}
}

// Counter-Strike 1.4 (Linux symbol; inline in UpdateClientData on Windows): a client whose
// HUD starts is told where every hostage is (message HostagePos with flag 1)
void CBasePlayer::SendHostagePos( void )
{
	CBaseEntity *pHostage = NULL;	// Counter-Strike 1.6's name

	while ( ( pHostage = UTIL_FindEntityByClassname( pHostage, "hostage_entity" ) ) != NULL )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgHostagePos, NULL, pev );
			WRITE_BYTE( 1 );
			WRITE_BYTE( ((CHostage *)pHostage)->m_iHostageIndex );
			WRITE_COORD( pHostage->pev->origin.x );
			WRITE_COORD( pHostage->pev->origin.y );
			WRITE_COORD( pHostage->pev->origin.z );
		MESSAGE_END();
	}
}

/*
=========================================================
	UpdateClientData

resends any changed player HUD info to the client.
Called every frame by PlayerPreThink
Also called at start of demo recording and playback by
ForceClientDllUpdate to ensure the demo gets messages
reflecting all of the HUD state info.
=========================================================
*/
void CBasePlayer :: UpdateClientData( void )
{
	if (m_fInitHUD)
	{
		m_fInitHUD = FALSE;
		gInitHUD = FALSE;
		m_signals.Update();	// Beta 5.0: the zones start afresh (their icons come again)

		// (Beta 6.0 and 6.1 wrote a console line numbered 1 to 13 before most of
		// the messages below; Beta 6.5 has none)
		MESSAGE_BEGIN( MSG_ONE, gmsgResetHUD, NULL, pev );
		MESSAGE_END();

		if ( !m_fGameHUDInitialized )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgInitHUD, NULL, pev );
			MESSAGE_END();

			g_pGameRules->InitHUD( this );
			m_fGameHUDInitialized = TRUE;
			if ( g_pGameRules->IsMultiplayer() )
			{
				FireTargets( "game_playerjoin", this, this, USE_TOGGLE, 0 );
			}

			// Counter-Strike 1.4: a new client's observer starts in the free chase mode
			m_iObserverLastMode = OBS_CHASE_FREE;
			m_iObserverC4State = 0;
			m_bObserverHasDefuser = false;
		}
		FireTargets( "game_playerspawn", this, this, USE_TOGGLE, 0 );

		// Beta 5.0 (leak): the HUD gets the money, the defuse kit's icon, the
		// round's time and no C4 icon (Beta 2.0 to 4.1's second ResetHUD for a
		// survivor's new round is gone)
		MESSAGE_BEGIN( MSG_ONE, gmsgMoney, NULL, pev );
			WRITE_LONG( m_iAccount );
			WRITE_BYTE( 0 );
		MESSAGE_END();

		if ( m_fHasDefuser == true )
		{
			// Beta 5.0: green (the leak: blue, 15 15 200)
			MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
				WRITE_BYTE( 1 );
				WRITE_STRING( "defuser" );
				WRITE_BYTE( 0 );
				WRITE_BYTE( 160 );
				WRITE_BYTE( 0 );
			MESSAGE_END();
		}

		SetBombIcon( FALSE );
		SyncRoundTimer();

		// Counter-Strike 1.4: where the hostages are, and the teams' scores
		SendHostagePos();

		if ( g_pGameRules->IsMultiplayer() )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgTeamScore, NULL, pev );
				WRITE_STRING( "CT" );
				WRITE_SHORT( ((CHalfLifeMultiplay *)g_pGameRules)->m_iNumCTWins );
			MESSAGE_END();

			MESSAGE_BEGIN( MSG_ONE, gmsgTeamScore, NULL, pev );
				WRITE_STRING( "TERRORIST" );
				WRITE_SHORT( ((CHalfLifeMultiplay *)g_pGameRules)->m_iNumTerroristWins );
			MESSAGE_END();
		}
	}

	if ( m_iHideHUD != m_iClientHideHUD )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgHideWeapon, NULL, pev );
			WRITE_BYTE( m_iHideHUD );
		MESSAGE_END();

		m_iClientHideHUD = m_iHideHUD;
	}

	if ( m_iFOV != m_iClientFOV )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
			WRITE_BYTE( m_iFOV );
		MESSAGE_END();

		// cache FOV change at end of function, so weapon updates can see that FOV has changed
	}

	// HACKHACK -- send the message to display the game title
	if (gDisplayTitle)
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgShowGameTitle, NULL, pev );
		WRITE_BYTE( 0 );
		MESSAGE_END();
		gDisplayTitle = 0;
	}

	if (pev->health != m_iClientHealth)
	{
		int iHealth = max( pev->health, 0 );  // make sure that no negative health values are sent

		// send "health" update message
		MESSAGE_BEGIN( MSG_ONE, gmsgHealth, NULL, pev );
			WRITE_BYTE( iHealth );
		MESSAGE_END();

		m_iClientHealth = pev->health;
	}


	if (pev->armorvalue != m_iClientBattery)
	{
		m_iClientBattery = pev->armorvalue;

		ASSERT( gmsgBattery > 0 );
		// send "health" update message
		MESSAGE_BEGIN( MSG_ONE, gmsgBattery, NULL, pev );
			WRITE_SHORT( (int)pev->armorvalue);
		MESSAGE_END();
	}

	if (pev->dmg_take || pev->dmg_save || m_bitsHUDDamage != m_bitsDamageType)
	{
		// Comes from inside me if not set
		Vector damageOrigin = pev->origin;
		// send "damage" message
		// causes screen to flash, and pain compass to show direction of damage
		edict_t *other = pev->dmg_inflictor;
		if ( other )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance(other);
			if ( pEntity )
				damageOrigin = pEntity->Center();
		}

		// only send down damage type that have hud art
		int visibleDamageBits = m_bitsDamageType & DMG_SHOWNHUD;

		MESSAGE_BEGIN( MSG_ONE, gmsgDamage, NULL, pev );
			WRITE_BYTE( pev->dmg_save );
			WRITE_BYTE( pev->dmg_take );
			WRITE_LONG( visibleDamageBits );
			WRITE_COORD( damageOrigin.x );
			WRITE_COORD( damageOrigin.y );
			WRITE_COORD( damageOrigin.z );
		MESSAGE_END();
	
		pev->dmg_take = 0;
		pev->dmg_save = 0;
		m_bitsHUDDamage = m_bitsDamageType;
		
		// Clear off non-time-based damage indicators
		m_bitsDamageType &= DMG_TIMEBASED;
	}

	// Update Flashlight
	if ((m_flFlashLightTime) && (m_flFlashLightTime <= gpGlobals->time))
	{
		if (FlashlightIsOn())
		{
			if (m_iFlashBattery)
			{
				m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->time;
				m_iFlashBattery--;
				
				if (!m_iFlashBattery)
					FlashlightTurnOff();
			}
		}
		else
		{
			if (m_iFlashBattery < 100)
			{
				m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->time;
				m_iFlashBattery++;
			}
			else
				m_flFlashLightTime = 0;
		}

		MESSAGE_BEGIN( MSG_ONE, gmsgFlashBattery, NULL, pev );
		WRITE_BYTE(m_iFlashBattery);
		MESSAGE_END();
	}


	if (m_iTrain & TRAIN_NEW)
	{
		ASSERT( gmsgTrain > 0 );
		// send "health" update message
		MESSAGE_BEGIN( MSG_ONE, gmsgTrain, NULL, pev );
			WRITE_BYTE(m_iTrain & 0xF);
		MESSAGE_END();

		m_iTrain &= ~TRAIN_NEW;
	}

	//
	// New Weapon?
	//
	if (!m_fKnownItem)
	{
		m_fKnownItem = TRUE;

	// WeaponInit Message
	// byte  = # of weapons
	//
	// for each weapon:
	// byte		name str length (not including null)
	// bytes... name
	// byte		Ammo Type
	// byte		Ammo2 Type
	// byte		bucket
	// byte		bucket pos
	// byte		flags	
	// ????		Icons
		
		// Send ALL the weapon info now
		int i;

		for (i = 0; i < MAX_WEAPONS; i++)
		{
			ItemInfo& II = CBasePlayerItem::ItemInfoArray[i];

			if ( !II.iId )
				continue;

			const char *pszName;
			if (!II.pszName)
				pszName = "Empty";
			else
				pszName = II.pszName;

			MESSAGE_BEGIN( MSG_ONE, gmsgWeaponList, NULL, pev );  
				WRITE_STRING(pszName);			// string	weapon name
				WRITE_BYTE(GetAmmoIndex(II.pszAmmo1));	// byte		Ammo Type
				WRITE_BYTE(II.iMaxAmmo1);				// byte     Max Ammo 1
				WRITE_BYTE(GetAmmoIndex(II.pszAmmo2));	// byte		Ammo2 Type
				WRITE_BYTE(II.iMaxAmmo2);				// byte     Max Ammo 2
				WRITE_BYTE(II.iSlot);					// byte		bucket
				WRITE_BYTE(II.iPosition);				// byte		bucket pos
				WRITE_BYTE(II.iId);						// byte		id (bit index into pev->weapons)
				WRITE_BYTE(II.iFlags);					// byte		Flags
			MESSAGE_END();
		}
	}


	SendAmmoUpdate();

	// Update all the items
	for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		if ( m_rgpPlayerItems[i] )  // each item updates it's successors
			m_rgpPlayerItems[i]->UpdateClientData( this );
	}

	// Cache and client weapon change
	m_pClientActiveItem = m_pActiveItem;
	m_iClientFOV = m_iFOV;

	// Beta 5.0 (leak): the status bar, every 0.2 seconds
	if ( m_flNextSBarUpdateTime < gpGlobals->time )
	{
		UpdateStatusBar();
		m_flNextSBarUpdateTime = gpGlobals->time + 0.2;
	}

	// Beta 5.0 (leak): once, a weapon in hand whose ammunition is all gone
	if ( !( m_flDisplayHistory & 0x8000 ) && m_pActiveItem && m_pActiveItem->IsWeapon() )
	{
		CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)m_pActiveItem;

		// Counter-Strike 1.1: a weapon that is not used up (1.0b asked for one that is, a grenade)
		if ( !( pWeapon->iFlags() & ITEM_FLAG_EXHAUSTIBLE ) && !AmmoInventory( pWeapon->m_iPrimaryAmmoType ) && !pWeapon->m_iClip )
		{
			m_flDisplayHistory |= 0x8000;	// out of ammunition
			HintMessage( "#Hint_out_of_ammo", FALSE, FALSE );
		}
	}

	// Beta 5.0: the zones the player stands in, every 0.2 seconds (the leak:
	// PostThink); Beta 6.0: every half second
	if ( gpGlobals->time > m_flNextZoneCheck )
	{
		m_flNextZoneCheck = gpGlobals->time + 0.5;
		HandleSignals();
	}

	// Beta 6.0: the radar: once a second, a living player who has moved 64
	// units since the last time shows his team mates where he is
	if ( pev->deadflag == DEAD_NO && gpGlobals->time > m_flNextRadarUpdate )
	{
		m_flNextRadarUpdate = gpGlobals->time + 1.0;

		if ( ( pev->origin - m_vecLastRadarOrigin ).Length() >= 64 )
		{
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBaseEntity *pEntity = UTIL_PlayerByIndex( i );	// name ours

				if ( pEntity && i != entindex() )
				{
					CBasePlayer *pPlayer = GetClassPtr( (CBasePlayer *)pEntity->pev );	// name ours

					if ( pEntity->pev->flags != FL_DORMANT && pEntity->pev->deadflag == DEAD_NO && pPlayer->m_iTeam == m_iTeam )
					{
						MESSAGE_BEGIN( MSG_ONE, gmsgRadar, NULL, pEntity->pev );
							WRITE_BYTE( entindex() );
							WRITE_COORD( pev->origin.x );
							WRITE_COORD( pev->origin.y );
							WRITE_COORD( pev->origin.z );
						MESSAGE_END();
					}
				}
			}
		}

		m_vecLastRadarOrigin = pev->origin;
	}
}


//=========================================================
// FBecomeProne - Overridden for the player to set the proper
// physics flags when a barnacle grabs player.
//=========================================================
BOOL CBasePlayer :: FBecomeProne ( void )
{
	m_afPhysicsFlags |= PFLAG_ONBARNACLE;
	return TRUE;
}

//=========================================================
// BarnacleVictimBitten - bad name for a function that is called
// by Barnacle victims when the barnacle pulls their head
// into its mouth. For the player, just die.
//=========================================================
void CBasePlayer :: BarnacleVictimBitten ( entvars_t *pevBarnacle )
{
	TakeDamage ( pevBarnacle, pevBarnacle, pev->health + pev->armorvalue, DMG_SLASH | DMG_ALWAYSGIB );
}

//=========================================================
// BarnacleVictimReleased - overridden for player who has
// physics flags concerns. 
//=========================================================
void CBasePlayer :: BarnacleVictimReleased ( void )
{
	m_afPhysicsFlags &= ~PFLAG_ONBARNACLE;
}


//=========================================================
// Illumination 
// return player light level plus virtual muzzle flash
//=========================================================
int CBasePlayer :: Illumination( void )
{
	int iIllum = CBaseEntity::Illumination( );

	iIllum += m_iWeaponFlash;
	if (iIllum > 255)
		return 255;
	return iIllum;
}


void CBasePlayer :: EnableControl(BOOL fControl)
{
	if (!fControl)
		pev->flags |= FL_FROZEN;
	else
		pev->flags &= ~FL_FROZEN;

}

//=========================================================
// ResetMaxSpeed - Beta 5.0 (leak) (Linux symbol): the client's
// maximum speed: 320 observing, almost none in the round's
// freeze period (Beta 7.0: 1, was 0.001), else what the
// weapon in his hands allows (260 with none)
//=========================================================
void CBasePlayer::ResetMaxSpeed( void )
{
	float speed;	// name ours

	if ( pev->iuser1 )	// Beta 6.5: an observer (IsObserver) flies at 900 (Beta 6.1: 350)
		speed = 900;
	else if ( g_pGameRules->IsMultiplayer() && ((CHalfLifeMultiplay *)g_pGameRules)->IsFreezePeriod() )
		speed = 1;	// Beta 7.0 (6.6: 0.001)
	else if ( m_bIsVIP == true )	// Beta 6.0: the VIP walks at 220 (Beta 6.5: 227)
		speed = 227;
	else if ( m_pActiveItem )
		speed = m_pActiveItem->GetMaxSpeed();
	else
		speed = 240;	// Beta 6.0 (Beta 5.2: 260)

	g_engfuncs.pfnSetClientMaxspeed( ENT( pev ), speed );
}

//=========================================================
// HintMessage - Beta 5.0 (leak) (Linux symbol): queue a hint
// for the player; a dead player gets none unless asked for.
// With the client's "ah" (auto help) key 0 or missing, the
// hint counts as shown
//=========================================================
bool CBasePlayer::HintMessage( const char *pMessage, BOOL bDisplayIfPlayerDead, BOOL bOverride )
{
	// Beta 5.0: IsAlive (the leak: the dead flag), and a forced hint is
	// queued whatever the client's autohelp setting says.
	// Counter-Strike 1.4: m_bShowHints, read from the user info by
	// SetPrefsFromUserinfo (1.3 read the key "ah" here)
	if ( !bDisplayIfPlayerDead && !IsAlive() )
		return false;

	if ( bOverride )
		return m_hintMessageQueue.AddMessage( pMessage );

	if ( m_bShowHints )
		return m_hintMessageQueue.AddMessage( pMessage );

	return true;
}


#define DOT_1DEGREE   0.9998476951564
#define DOT_2DEGREE   0.9993908270191
#define DOT_3DEGREE   0.9986295347546
#define DOT_4DEGREE   0.9975640502598
#define DOT_5DEGREE   0.9961946980917
#define DOT_6DEGREE   0.9945218953683
#define DOT_7DEGREE   0.9925461516413
#define DOT_8DEGREE   0.9902680687416
#define DOT_9DEGREE   0.9876883405951
#define DOT_10DEGREE  0.9848077530122
#define DOT_15DEGREE  0.9659258262891
#define DOT_20DEGREE  0.9396926207859
#define DOT_25DEGREE  0.9063077870367

//=========================================================
// Autoaim
// set crosshair position to point to enemey
//=========================================================
Vector CBasePlayer :: GetAutoaimVector( float flDelta )
{
	if (g_iSkillLevel == SKILL_HARD)
	{
		UTIL_MakeVectors( pev->v_angle + pev->punchangle );
		return gpGlobals->v_forward;
	}

	Vector vecSrc = GetGunPosition( );
	float flDist = 8192;

	// always use non-sticky autoaim
	// UNDONE: use sever variable to chose!
	if (1 || g_iSkillLevel == SKILL_MEDIUM)
	{
		m_vecAutoAim = Vector( 0, 0, 0 );
		// flDelta *= 0.5;
	}

	BOOL m_fOldTargeting = m_fOnTarget;
	Vector angles = AutoaimDeflection(vecSrc, flDist, flDelta );

	// update ontarget if changed
	if ( !g_pGameRules->AllowAutoTargetCrosshair() )
		m_fOnTarget = 0;
	else if (m_fOldTargeting != m_fOnTarget)
	{
		m_pActiveItem->UpdateItemInfo( );
	}

	if (angles.x > 180)
		angles.x -= 360;
	if (angles.x < -180)
		angles.x += 360;
	if (angles.y > 180)
		angles.y -= 360;
	if (angles.y < -180)
		angles.y += 360;

	if (angles.x > 25)
		angles.x = 25;
	if (angles.x < -25)
		angles.x = -25;
	if (angles.y > 12)
		angles.y = 12;
	if (angles.y < -12)
		angles.y = -12;


	// always use non-sticky autoaim
	// UNDONE: use sever variable to chose!
	if (0 || g_iSkillLevel == SKILL_EASY)
	{
		m_vecAutoAim = m_vecAutoAim * 0.67 + angles * 0.33;
	}
	else
	{
		m_vecAutoAim = angles * 0.9;
	}

	// m_vecAutoAim = m_vecAutoAim * 0.99;

	// Don't send across network if sv_aim is 0
	if ( CVAR_GET_FLOAT( "sv_aim" ) != 0 )
	{
		if ( m_vecAutoAim.x != m_lastx ||
			 m_vecAutoAim.y != m_lasty )
		{
			SET_CROSSHAIRANGLE( edict(), -m_vecAutoAim.x, m_vecAutoAim.y );
			
			m_lastx = m_vecAutoAim.x;
			m_lasty = m_vecAutoAim.y;
		}
	}

	// ALERT( at_console, "%f %f\n", angles.x, angles.y );

	UTIL_MakeVectors( pev->v_angle + pev->punchangle + m_vecAutoAim );
	return gpGlobals->v_forward;
}


Vector CBasePlayer :: AutoaimDeflection( Vector &vecSrc, float flDist, float flDelta  )
{
	edict_t		*pEdict = g_engfuncs.pfnPEntityOfEntIndex( 1 );
	CBaseEntity	*pEntity;
	float		bestdot;
	Vector		bestdir;
	edict_t		*bestent;
	TraceResult tr;

	if ( CVAR_GET_FLOAT("sv_aim") == 0 )
	{
		m_fOnTarget = FALSE;
		return g_vecZero;
	}

	UTIL_MakeVectors( pev->v_angle + pev->punchangle + m_vecAutoAim );

	// try all possible entities
	bestdir = gpGlobals->v_forward;
	bestdot = flDelta; // +- 10 degrees
	bestent = NULL;

	m_fOnTarget = FALSE;

	UTIL_TraceLine( vecSrc, vecSrc + bestdir * flDist, dont_ignore_monsters, edict(), &tr );


	if ( tr.pHit && tr.pHit->v.takedamage != DAMAGE_NO)
	{
		// don't look through water
		if (!((pev->waterlevel != 3 && tr.pHit->v.waterlevel == 3) 
			|| (pev->waterlevel == 3 && tr.pHit->v.waterlevel == 0)))
		{
			if (tr.pHit->v.takedamage == DAMAGE_AIM)
				m_fOnTarget = TRUE;

			return m_vecAutoAim;
		}
	}

	for ( int i = 1; i < gpGlobals->maxEntities; i++, pEdict++ )
	{
		Vector center;
		Vector dir;
		float dot;

		if ( pEdict->free )	// Not in use
			continue;
		
		if (pEdict->v.takedamage != DAMAGE_AIM)
			continue;
		if (pEdict == edict())
			continue;
//		if (pev->team > 0 && pEdict->v.team == pev->team)
//			continue;	// don't aim at teammate
		if ( !g_pGameRules->ShouldAutoAim( this, pEdict ) )
			continue;

		pEntity = Instance( pEdict );
		if (pEntity == NULL)
			continue;

		if (!pEntity->IsAlive())
			continue;

		// don't look through water
		if ((pev->waterlevel != 3 && pEntity->pev->waterlevel == 3) 
			|| (pev->waterlevel == 3 && pEntity->pev->waterlevel == 0))
			continue;

		center = pEntity->BodyTarget( vecSrc );

		dir = (center - vecSrc).Normalize( );

		// make sure it's in front of the player
		if (DotProduct (dir, gpGlobals->v_forward ) < 0)
			continue;

		dot = fabs( DotProduct (dir, gpGlobals->v_right ) ) 
			+ fabs( DotProduct (dir, gpGlobals->v_up ) ) * 0.5;

		// tweek for distance
		dot *= 1.0 + 0.2 * ((center - vecSrc).Length() / flDist);

		if (dot > bestdot)
			continue;	// to far to turn

		UTIL_TraceLine( vecSrc, center, dont_ignore_monsters, edict(), &tr );
		if (tr.flFraction != 1.0 && tr.pHit != pEdict)
		{
			// ALERT( at_console, "hit %s, can't see %s\n", STRING( tr.pHit->v.classname ), STRING( pEdict->v.classname ) );
			continue;
		}

		// don't shoot at friends
		if (IRelationship( pEntity ) < 0)
		{
			if ( !pEntity->IsPlayer() && !g_pGameRules->IsDeathmatch())
				// ALERT( at_console, "friend\n");
				continue;
		}

		// Beta 5.0 (leak): nor at teammates (two tests, one inside the other:
		// with one && test VC6's inline budget leaves the copy of
		// bestdir - pev->v_angle below out of line, which retail has inline)
		if ( pEntity->IsPlayer() )
		{
			if ( ((CBasePlayer *)pEntity)->m_iTeam == m_iTeam )
				continue;
		}

		// can shoot at this one
		bestdot = dot;
		bestent = pEdict;
		bestdir = dir;
	}

	if (bestent)
	{
		bestdir = UTIL_VecToAngles (bestdir);
		bestdir.x = -bestdir.x;
		bestdir = bestdir - pev->v_angle - pev->punchangle;

		if (bestent->v.takedamage == DAMAGE_AIM)
			m_fOnTarget = TRUE;

		return bestdir;
	}

	return Vector( 0, 0, 0 );
}


void CBasePlayer :: ResetAutoaim( )
{
	if (m_vecAutoAim.x != 0 || m_vecAutoAim.y != 0)
	{
		m_vecAutoAim = Vector( 0, 0, 0 );
		SET_CROSSHAIRANGLE( edict(), 0, 0 );
	}
	m_fOnTarget = FALSE;
}

/*
=============
SetCustomDecalFrames

  UNDONE:  Determine real frame limit, 8 is a placeholder.
  Note:  -1 means no custom frames present.
=============
*/
void CBasePlayer :: SetCustomDecalFrames( int nFrames )
{
	if (nFrames > 0 &&
		nFrames < 8)
		m_nCustomSprayFrames = nFrames;
	else
		m_nCustomSprayFrames = -1;
}

/*
=============
GetCustomDecalFrames

  Returns the # of custom frames this player's custom clan logo contains.
=============
*/
int CBasePlayer :: GetCustomDecalFrames( void )
{
	return m_nCustomSprayFrames;
}


// Beta 5.0 (leak): SDK 2.3's status bar. It names the player in the
// crosshair, a teammate (any player to a spectator) with his health; a
// hostage with its health. A chasing observer gets the chased player.
void CBasePlayer::InitStatusBar()
{
	m_flStatusBarDisappearDelay = 0;
	m_SbarString1[0] = m_SbarString0[0] = 0; 
}

void CBasePlayer::UpdateStatusBar()
{
	int newSBarState[ SBAR_END ];
	char sbuf0[ SBAR_STRING_SIZE ];
	char sbuf1[ SBAR_STRING_SIZE ];

	memset( newSBarState, 0, sizeof(newSBarState) );
	strcpy( sbuf0, m_SbarString0 );
	strcpy( sbuf1, m_SbarString1 );

	// Beta 6.5: an observer following a player sees him (Beta 6.1: its own
	// chase camera's target)
	// Counter-Strike 1.4: in the watched player's own view and in the map's chase mode too
	if ( m_hObserverTarget != NULL && ( pev->iuser1 == OBS_CHASE_LOCKED || pev->iuser1 == OBS_CHASE_FREE || pev->iuser1 == OBS_IN_EYE || pev->iuser1 == OBS_MAP_CHASE ) )
	{
		CBasePlayer *pTarget = (CBasePlayer *)UTIL_PlayerByIndex( ENTINDEX( m_hObserverTarget->edict() ) );

		if ( pTarget && !FNullEnt( pTarget->edict() ) )
		{
			strcpy( sbuf1, "1 %c1: %p2\n2  %h: %i3%%" );	// Counter-Strike 1.1: %h, the client's word for "Health"
			newSBarState[ SBAR_ID_TARGETNAME ] = ENTINDEX( pTarget->edict() );
			newSBarState[ SBAR_ID_TARGETTYPE ] = ( pTarget->m_iTeam == m_iTeam ) ? SBAR_TARGETTYPE_TEAMMATE : SBAR_TARGETTYPE_ENEMY;
			newSBarState[ SBAR_ID_TARGETHEALTH ] = pTarget->pev->health * 100 / pTarget->pev->max_health;
		}
		else if ( m_flStatusBarDisappearDelay > gpGlobals->time )
		{
			// Beta 5.0: hold the previous target (SDK 2.3's text)
			newSBarState[ SBAR_ID_TARGETTYPE ] = m_izSBarState[ SBAR_ID_TARGETTYPE ];
			newSBarState[ SBAR_ID_TARGETNAME ] = m_izSBarState[ SBAR_ID_TARGETNAME ];
			newSBarState[ SBAR_ID_TARGETHEALTH ] = m_izSBarState[ SBAR_ID_TARGETHEALTH ];
		}

		// Beta 5.0: every update in chase mode (sic: so the previous target is
		// held as long as the observer chases)
		m_flStatusBarDisappearDelay = gpGlobals->time + 10.0;
	}
	else
	{
		// Find an ID Target
		TraceResult tr;
		UTIL_MakeVectors( pev->v_angle + pev->punchangle );
		Vector vecSrc = EyePosition();
		Vector vecEnd = vecSrc + ( gpGlobals->v_forward * ( ( pev->flags & FL_SPECTATOR ) ? MAX_SPEC_ID_RANGE : MAX_ID_RANGE ) );
		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, edict(), &tr );

		if ( tr.flFraction != 1.0 )
		{
			if ( !FNullEnt( tr.pHit ) )
			{
				CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

				if ( pEntity->Classify() == CLASS_PLAYER )
				{
					// Counter-Strike 1.0: no local for the player (Counter-Strike 1.6 has
					// none either; the count of locals orders the x87 operands of
					// v_angle + punchangle above, which is how the image shows it)
					newSBarState[ SBAR_ID_TARGETNAME ] = ENTINDEX( pEntity->edict() );
					newSBarState[ SBAR_ID_TARGETTYPE ] = ( ((CBasePlayer *)pEntity)->m_iTeam == m_iTeam ) ? SBAR_TARGETTYPE_TEAMMATE : SBAR_TARGETTYPE_ENEMY;

					if ( m_iTeam != ((CBasePlayer *)pEntity)->m_iTeam && !pev->iuser1 )	// Beta 6.5: IsObserver (Beta 6.1: FL_SPECTATOR)
					{
						// Counter-Strike 1.1: mp_playerid 1 or 2 hides an enemy's name,
						// 2 a team mate's and a hostage's too (not from an observer)
						// (the observer test is the one two lines up again: no code)
						if ( ( playerid.value == 1 || playerid.value == 2 ) && !pev->iuser1 )
							strcpy( sbuf1, " " );
						else
							strcpy( sbuf1, "1 %c1: %p2" );

						if ( !( m_flDisplayHistory & 0x100 ) )	// enemy spotted
						{
							m_flDisplayHistory |= 0x100;
							HintMessage( "#Hint_spotted_an_enemy", FALSE, FALSE );
						}
					}
					else
					{
						// teammates (and spectators) see the health
						if ( playerid.value == 2 && !pev->iuser1 )
							strcpy( sbuf1, " " );
						else
							strcpy( sbuf1, "1 %c1: %p2\n2  %h: %i3%%" );
						newSBarState[ SBAR_ID_TARGETHEALTH ] = 100 * ( pEntity->pev->health / pEntity->pev->max_health );

						if ( !( m_flDisplayHistory & 0x80 ) && !( pev->flags & FL_SPECTATOR ) )	// friend spotted
						{
							m_flDisplayHistory |= 0x80;
							HintMessage( "#Hint_spotted_a_friend", FALSE, FALSE );
						}
					}
				}
				else if ( pEntity->Classify() == CLASS_HUMAN_PASSIVE )
				{
					if ( playerid.value == 2 && !pev->iuser1 )
						strcpy( sbuf1, " " );
					else
						strcpy( sbuf1, "1 %c1  %h: %i3%%" );
					newSBarState[ SBAR_ID_TARGETTYPE ] = SBAR_TARGETTYPE_HOSTAGE;
					newSBarState[ SBAR_ID_TARGETHEALTH ] = 100 * ( pEntity->pev->health / pEntity->pev->max_health );

					// a hostage seen from afar: the team's task; close by: how to lead it
					if ( !( m_flDisplayHistory & 0x4 ) && tr.flFraction > 0.1 )
					{
						m_flDisplayHistory |= 0x4;

						switch ( m_iTeam )
						{
						case TEAM_TERRORIST:
							HintMessage( "#Hint_prevent_hostage_rescue", TRUE, FALSE );
							break;
						case TEAM_CT:
							HintMessage( "#Hint_rescue_the_hostages", TRUE, FALSE );
							break;
						}
					}
					else if ( m_iTeam == TEAM_CT && !( m_flDisplayHistory & 0x8 ) && tr.flFraction <= 0.1 )	// Counter-Strike 1.4: only a counter-terrorist
					{
						m_flDisplayHistory |= 0x4 | 0x8;
						HintMessage( "#Hint_press_use_so_hostage_will_follow", FALSE, FALSE );
					}
				}

				m_flStatusBarDisappearDelay = gpGlobals->time + 10.0;	// Beta 5.0 (the leak: 1)
			}
		}
		else if ( m_flStatusBarDisappearDelay > gpGlobals->time )
		{
			// hold the last status bar
			newSBarState[ SBAR_ID_TARGETTYPE ] = m_izSBarState[ SBAR_ID_TARGETTYPE ];
			newSBarState[ SBAR_ID_TARGETNAME ] = m_izSBarState[ SBAR_ID_TARGETNAME ];
			newSBarState[ SBAR_ID_TARGETHEALTH ] = m_izSBarState[ SBAR_ID_TARGETHEALTH ];
		}
	}

	BOOL bForceResend = FALSE;

	if ( strcmp( sbuf0, m_SbarString0 ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStatusText, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_STRING( sbuf0 );
		MESSAGE_END();

		strcpy( m_SbarString0, sbuf0 );

		// make sure everything's resent
		bForceResend = TRUE;
	}

	if ( strcmp( sbuf1, m_SbarString1 ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStatusText, NULL, pev );
			WRITE_BYTE( 1 );
			WRITE_STRING( sbuf1 );
		MESSAGE_END();

		strcpy( m_SbarString1, sbuf1 );

		// make sure everything's resent
		bForceResend = TRUE;
	}

	// Check values and send if they don't match
	for (int i = 1; i < SBAR_END; i++)
	{
		if ( newSBarState[i] != m_izSBarState[i] || bForceResend )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgStatusValue, NULL, pev );
				WRITE_BYTE( i );
				WRITE_SHORT( newSBarState[ i ] );
			MESSAGE_END();

			m_izSBarState[i] = newSBarState[i];
		}
	}
}


//=========================================================
// DropPlayerItem - drop the named item, or if no name,
// the active item. 
//=========================================================
void CBasePlayer::DropPlayerItem( const char *pszItemName )
{
	if ( !strlen( pszItemName ) )
	{
		// if this string has no length, the client didn't type a name!
		// assume player wants to drop the active item.
		// make the string null to make future operations in this function easier
		pszItemName = NULL;
	} 

	// Counter-Strike 1.4: the VIP drops nothing
	if ( m_bIsVIP )
	{
		ClientPrint( pev, HUD_PRINTCENTER, "#Weapon_Cannot_Be_Dropped" );
		return;
	}

	CBasePlayerItem *pWeapon;
	int i;

	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		pWeapon = m_rgpPlayerItems[ i ];

		while ( pWeapon )
		{
			if ( pszItemName )
			{
				// try to match by name. 
				if ( !strcmp( pszItemName, STRING( pWeapon->pev->classname ) ) )
				{
					// match! 
					break;
				}
			}
			else
			{
				// trying to drop active item
				if ( pWeapon == m_pActiveItem )
				{
					// active item!
					break;
				}
			}

			pWeapon = pWeapon->m_pNext; 
		}

		
		// if we land here with a valid pWeapon pointer, that's because we found the 
		// item we want to drop and hit a BREAK;  pWeapon is the item.
		if ( pWeapon )
		{
			// Beta 5.0 (leak): the knife and the grenades stay
			if ( !pWeapon->CanDrop() )
			{
				ClientPrint( pev, HUD_PRINTCENTER, "#Weapon_Cannot_Be_Dropped" );
				return;
			}

			g_pGameRules->GetNextBestWeapon( this, pWeapon );

			UTIL_MakeVectors ( pev->angles ); 

			pev->weapons &= ~(1<<pWeapon->m_iId);// take item off hud

			// Beta 6.0: dropping the primary weapon
			if ( pWeapon->iItemSlot() == 1 )
				m_bHasPrimary = false;

			// Beta 5.0 (leak): the bomb is a weapon; everybody hears it was
			// dropped, unless the round is about to restart
			if ( !strcmp( STRING( pWeapon->pev->classname ), "weapon_c4" ) )
			{
				m_bHasC4 = false;
				pev->body = 0;
				SetBombIcon( FALSE );

				// Beta 5.0: an arming bomb's progress bar goes
				pWeapon->m_pPlayer->SetProgressBarTime( 0 );

				if ( ((CHalfLifeMultiplay *)g_pGameRules)->m_flRestartRoundTime == 0 )
				{
					// Counter-Strike 1.1: a line in the server's log
					UTIL_LogPrintf( "\"%s<%i><%s><TERRORIST>\" triggered \"Dropped_The_Bomb\"\n", STRING( pev->netname ), GETPLAYERUSERID( edict() ), GETPLAYERAUTHID( edict() ) );

					// Counter-Strike 1.4: only the living terrorists read that the bomb was dropped
					// [everybody], and their radars are told where (message BombDrop).
					// The locals' names are Counter-Strike 1.6's
					CBaseEntity *pEntity = NULL;
					CBasePlayer *pTempPlayer;

					pEntity = UTIL_FindEntityByClassname( pEntity, "player" );
					while ( pEntity != NULL && !FNullEnt( pEntity->edict() ) )
					{
						if ( pEntity->IsPlayer() && pEntity->pev->flags != FL_DORMANT )
						{
							pTempPlayer = GetClassPtr( (CBasePlayer *)pEntity->pev );

							if ( pTempPlayer->pev->deadflag == DEAD_NO && pTempPlayer->m_iTeam == TEAM_TERRORIST )
							{
								ClientPrint( pTempPlayer->pev, HUD_PRINTCENTER, "#Game_bomb_drop", STRING( pev->netname ) );

								MESSAGE_BEGIN( MSG_ONE, gmsgBombDrop, NULL, pTempPlayer->pev );
									WRITE_COORD( pev->origin.x );
									WRITE_COORD( pev->origin.y );
									WRITE_COORD( pev->origin.z );
								MESSAGE_END();
							}
						}

						pEntity = UTIL_FindEntityByClassname( pEntity, "player" );
					}
				}
			}

			CWeaponBox *pWeaponBox = (CWeaponBox *)CBaseEntity::Create( "weaponbox", pev->origin + gpGlobals->v_forward * 10, pev->angles, edict() );
			pWeaponBox->pev->angles.x = 0;
			pWeaponBox->pev->angles.z = 0;
			pWeaponBox->SetThink( CWeaponBox::Kill );
			pWeaponBox->pev->nextthink = gpGlobals->time + 300;	// Beta 3.0 (2.0: 30)
			pWeaponBox->PackWeapon( pWeapon );
			pWeaponBox->pev->velocity = gpGlobals->v_forward * 400;

			// Beta 5.0 (leak): only a weapon that is its own ammunition (a grenade)
			// takes it along, all of it; sic: the box gets a count of one if the
			// player had any. Other weapons leave their spare ammunition with him
			if ( pWeapon->iFlags() & ITEM_FLAG_EXHAUSTIBLE )
			{
				int iAmmoIndex = GetAmmoIndex( pWeapon->pszAmmo1() );

				if ( iAmmoIndex != -1 )
				{
					pWeaponBox->PackAmmo( MAKE_STRING( pWeapon->pszAmmo1() ), m_rgAmmo[ iAmmoIndex ] > 0 );
					m_rgAmmo[ iAmmoIndex ] = 0; 
				}
			}

			// the box shows the weapon
			const char *modelname = GetCSModelName( pWeapon->m_iId );

			if ( modelname )
				SET_MODEL( ENT( pWeaponBox->pev ), modelname );

			return;// we're done, so stop searching with the FOR loop.
		}
	}
}

//=========================================================
// HasPlayerItem Does the player already have this item?
//=========================================================
BOOL CBasePlayer::HasPlayerItem( CBasePlayerItem *pCheckItem )
{
	CBasePlayerItem *pItem = m_rgpPlayerItems[pCheckItem->iItemSlot()];

	while (pItem)
	{
		if (FClassnameIs( pItem->pev, STRING( pCheckItem->pev->classname) ))
		{
			return TRUE;
		}
		pItem = pItem->m_pNext;
	}

	return FALSE;
}

//=========================================================
// HasNamedPlayerItem Does the player already have this item?
//=========================================================
BOOL CBasePlayer::HasNamedPlayerItem( const char *pszItemName )
{
	CBasePlayerItem *pItem;
	int i;
 
	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		pItem = m_rgpPlayerItems[ i ];
		
		while (pItem)
		{
			if ( !strcmp( pszItemName, STRING( pItem->pev->classname ) ) )
			{
				return TRUE;
			}
			pItem = pItem->m_pNext;
		}
	}

	return FALSE;
}

//=========================================================
// SwitchTeam - Beta 6.0 (Linux symbol): the player goes over
// to the other team with a model of the other side (an
// escape map's teams swap, SwapAllPlayers): urban becomes
// leet, sas arctic, gign guerilla, the rest terror; terror
// becomes gsg9, arctic sas, guerilla gign, the rest urban.
// Counter-Strike 1.0b: each case writes the model key from a
// literal (Beta 7.0 to 1.0 copied the names from buffers put
// together at run time and sent a "model ..." command too).
//=========================================================
void CBasePlayer::SwitchTeam( void )
{
	int oldTeam = m_iTeam;	// Counter-Strike 1.1 (1.6's name)

	if ( m_iTeam == TEAM_CT )
	{
		m_iTeam = TEAM_TERRORIST;

		switch ( m_iAppearance )
		{
		case 1:
			m_iAppearance = 3;
			g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", "leet" );
			break;
		case 5:
			m_iAppearance = 2;
			g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", "terror" );
			break;
		case 7:
			m_iAppearance = 4;
			g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", "arctic" );
			break;
		case 6:		// Beta 6.5: the GIGN becomes a guerilla
			m_iAppearance = 8;
			g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", "guerilla" );
			break;
		default:
			m_iAppearance = 2;
			g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", "terror" );
			break;
		}
	}
	else if ( m_iTeam == TEAM_TERRORIST )
	{
		m_iTeam = TEAM_CT;

		switch ( m_iAppearance )
		{
		case 3:
			m_iAppearance = 1;
			g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", "urban" );
			break;
		case 2:
			m_iAppearance = 5;
			g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", "gsg9" );
			break;
		case 4:
			m_iAppearance = 7;
			g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", "sas" );
			break;
		case 8:		// Beta 6.5: the guerilla becomes GIGN
			m_iAppearance = 6;
			g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", "gign" );
			break;
		default:
			m_iAppearance = 1;
			g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", "urban" );
			break;
		}
	}

	// Counter-Strike 1.1: everything from here on. Everybody learns the new team
	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( entindex() );
		if ( m_iTeam == TEAM_CT )
			WRITE_STRING( "CT" );
		else if ( m_iTeam == TEAM_TERRORIST )
			WRITE_STRING( "TERRORIST" );
		else if ( m_iTeam == TEAM_SPECTATOR )
			WRITE_STRING( "SPECTATOR" );
		else
			WRITE_STRING( "UNASSIGNED" );
	MESSAGE_END();

	if ( m_iTeam != TEAM_UNASSIGNED )
		SetScoreboardAttributes();

	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, ( m_iTeam == TEAM_TERRORIST ) ? "#Game_join_terrorist_auto" : "#Game_join_ct_auto",
		( pev->netname && STRING( pev->netname )[0] != 0 ) ? STRING( pev->netname ) : "<unconnected>" );

	// a counter-terrorist's defuse kit does not change sides with him
	if ( m_fHasDefuser )
	{
		m_fHasDefuser = false;
		pev->body = 0;

		MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_STRING( "defuser" );
		MESSAGE_END();

		SetProgressBarTime( 0 );

		for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
		{
			m_pActiveItem = m_rgpPlayerItems[ i ];

			if ( m_pActiveItem && FClassnameIs( m_pActiveItem->pev, "item_thighpack" ) )
			{
				m_pActiveItem->Drop();
				m_rgpPlayerItems[ i ] = NULL;
			}
		}
	}

	// Counter-Strike 1.3: both names are GetTeam's (1.1: a switch for the old team and "UNASSIGNED"
	// for a new team that is none of the three)
	char *szOldTeam = GetTeam( oldTeam );	// Counter-Strike 1.6's names
	char *szNewTeam = GetTeam( m_iTeam );

	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" joined team \"%s\" (auto)\n", STRING( pev->netname ), GETPLAYERUSERID( edict() ), GETPLAYERAUTHID( edict() ), szOldTeam, szNewTeam );
}

//=========================================================
// 
//=========================================================
BOOL CBasePlayer :: SwitchWeapon( CBasePlayerItem *pWeapon ) 
{
	if ( !pWeapon->CanDeploy() )
	{
		return FALSE;
	}
	
	ResetAutoaim( );
	
	if (m_pActiveItem)
	{
		m_pActiveItem->Holster( );
	}

	m_pActiveItem = pWeapon;
	pWeapon->Deploy( );

	// Beta 5.0 (leak): the speed the new weapon allows
	if ( pWeapon->m_pPlayer )
		pWeapon->m_pPlayer->ResetMaxSpeed();

	return TRUE;
}

//=========================================================
// TabulateAmmo - Beta 6.5 (Linux symbol): the carried
// ammunition by type in CBaseEntity's ammo_ members, which
// UpdateClientData sends to the client's weapon prediction
//=========================================================
void CBasePlayer::TabulateAmmo( void )
{
	ammo_buckshot = AmmoInventory( GetAmmoIndex( "buckshot" ) );
	ammo_9mm = AmmoInventory( GetAmmoIndex( "9mm" ) );
	ammo_556nato = AmmoInventory( GetAmmoIndex( "556Nato" ) );
	ammo_556natobox = AmmoInventory( GetAmmoIndex( "556NatoBox" ) );
	ammo_762nato = AmmoInventory( GetAmmoIndex( "762Nato" ) );
	ammo_45acp = AmmoInventory( GetAmmoIndex( "45acp" ) );
	ammo_50ae = AmmoInventory( GetAmmoIndex( "50AE" ) );
	ammo_338mag = AmmoInventory( GetAmmoIndex( "338Magnum" ) );
	ammo_57mm = AmmoInventory( GetAmmoIndex( "57mm" ) );
	ammo_357sig = AmmoInventory( GetAmmoIndex( "357SIG" ) );
}

//=========================================================
// Dead HEV suit prop
//=========================================================
class CDeadHEV : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_HUMAN_MILITARY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[4];
};

char *CDeadHEV::m_szPoses[] = { "deadback", "deadsitting", "deadstomach", "deadtable" };

void CDeadHEV::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_hevsuit_dead, CDeadHEV );

//=========================================================
// ********** DeadHEV SPAWN **********
//=========================================================
void CDeadHEV :: Spawn( void )
{
	PRECACHE_MODEL("models/player.mdl");
	SET_MODEL(ENT(pev), "models/player.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	pev->body			= 1;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );

	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead hevsuit with bad pose\n" );
		pev->sequence = 0;
		pev->effects = EF_BRIGHTFIELD;
	}

	// Corpses have less health
	pev->health			= 8;

	MonsterInitDead();
}


class CStripWeapons : public CPointEntity
{
public:
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

private:
};

LINK_ENTITY_TO_CLASS( player_weaponstrip, CStripWeapons );

void CStripWeapons :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = NULL;

	if ( pActivator && pActivator->IsPlayer() )
	{
		pPlayer = (CBasePlayer *)pActivator;
	}
	else if ( !g_pGameRules->IsDeathmatch() )
	{
		pPlayer = (CBasePlayer *)CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );
	}

	if ( pPlayer )
		pPlayer->RemoveAllItems( FALSE );
}


class CRevertSaved : public CPointEntity
{
public:
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	EXPORT MessageThink( void );
	void	EXPORT LoadThink( void );
	void	KeyValue( KeyValueData *pkvd );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	inline	float	Duration( void ) { return pev->dmg_take; }
	inline	float	HoldTime( void ) { return pev->dmg_save; }
	inline	float	MessageTime( void ) { return m_messageTime; }
	inline	float	LoadTime( void ) { return m_loadTime; }

	inline	void	SetDuration( float duration ) { pev->dmg_take = duration; }
	inline	void	SetHoldTime( float hold ) { pev->dmg_save = hold; }
	inline	void	SetMessageTime( float time ) { m_messageTime = time; }
	inline	void	SetLoadTime( float time ) { m_loadTime = time; }

private:
	float	m_messageTime;
	float	m_loadTime;
};

LINK_ENTITY_TO_CLASS( player_loadsaved, CRevertSaved );

TYPEDESCRIPTION	CRevertSaved::m_SaveData[] = 
{
	DEFINE_FIELD( CRevertSaved, m_messageTime, FIELD_FLOAT ),	// These are not actual times, but durations, so save as floats
	DEFINE_FIELD( CRevertSaved, m_loadTime, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CRevertSaved, CPointEntity );

void CRevertSaved :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "duration"))
	{
		SetDuration( atof(pkvd->szValue) );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "holdtime"))
	{
		SetHoldTime( atof(pkvd->szValue) );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "messagetime"))
	{
		SetMessageTime( atof(pkvd->szValue) );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "loadtime"))
	{
		SetLoadTime( atof(pkvd->szValue) );
		pkvd->fHandled = TRUE;
	}
	else 
		CPointEntity::KeyValue( pkvd );
}

void CRevertSaved :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	UTIL_ScreenFadeAll( pev->rendercolor, Duration(), HoldTime(), pev->renderamt, FFADE_OUT );
	pev->nextthink = gpGlobals->time + MessageTime();
	SetThink( MessageThink );
}


void CRevertSaved :: MessageThink( void )
{
	UTIL_ShowMessageAll( STRING(pev->message) );
	float nextThink = LoadTime() - MessageTime();
	if ( nextThink > 0 ) 
	{
		pev->nextthink = gpGlobals->time + nextThink;
		SetThink( LoadThink );
	}
	else
		LoadThink();
}


void CRevertSaved :: LoadThink( void )
{
	if ( !gpGlobals->deathmatch )
	{
		SERVER_COMMAND("reload\n");
	}
}


//=========================================================
// Multiplayer intermission spots.
//=========================================================
class CInfoIntermission:public CPointEntity
{
	void Spawn( void );
	void Think( void );
};

void CInfoIntermission::Spawn( void )
{
	UTIL_SetOrigin( pev, pev->origin );
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->v_angle = g_vecZero;

	pev->nextthink = gpGlobals->time + 2;// let targets spawn!

}

void CInfoIntermission::Think ( void )
{
	edict_t *pTarget;

	// find my target
	pTarget = FIND_ENTITY_BY_TARGETNAME( NULL, STRING(pev->target) );

	if ( !FNullEnt(pTarget) )
	{
		pev->v_angle = UTIL_VecToAngles( (pTarget->v.origin - pev->origin).Normalize() );
		pev->v_angle.x = -pev->v_angle.x;
	}
}

LINK_ENTITY_TO_CLASS( info_intermission, CInfoIntermission );

// Counter-Strike 1.3: four bytes of player.obj's uninitialised data that no code
// reads or writes (retail has them behind g_pLastTerroristSpawn, at 0x100e0ef0).
// The 1.3 Linux library has a file-local pstudiohdr of four bytes at that place
// of player.cpp's data, behind g_pLastTerroristSpawn too: the name is its symbol,
// the type is the name's usual one, and where in the file it stood is not shown
// (VC6 orders uninitialised data by a hash of the names).
static studiohdr_t *pstudiohdr;

/*
====================
StudioEstimateGait

====================
*/
void CBasePlayer::StudioEstimateGait( void )
{
	float dt;

	dt = gpGlobals->frametime;
	dt = max( 0.0, dt );
	dt = min( 1.0, dt );

	if (dt == 0)
	{
		m_flGaitMovement = 0;
		return;
	}

	// (the vector is initialised where it is declared: with a declaration at the top and an
	// assignment here the pack's compiler sums the squares of Length() as z, x, y; retail: z, y, x)
	Vector est_velocity = pev->origin - m_prevgaitorigin;
	m_prevgaitorigin = pev->origin;
	m_flGaitMovement = est_velocity.Length();
	if (dt <= 0 || m_flGaitMovement / dt < 5)
	{
		m_flGaitMovement = 0;
		est_velocity.x = 0;
		est_velocity.y = 0;
	}

	if (est_velocity.y == 0 && est_velocity.x == 0)
	{
		float flYawDiff = pev->angles.y - m_flGaityaw;
		flYawDiff = flYawDiff - (int)(flYawDiff / 360) * 360;
		if (flYawDiff > 180)
			flYawDiff -= 360;
		if (flYawDiff < -180)
			flYawDiff += 360;

		if (dt < 0.25)
			flYawDiff *= dt * 4;
		else
			flYawDiff *= dt;

		m_flGaityaw += flYawDiff;
		m_flGaityaw = m_flGaityaw - (int)(m_flGaityaw / 360) * 360;

		m_flGaitMovement = 0;
	}
	else
	{
		m_flGaityaw = (atan2(est_velocity.y, est_velocity.x) * 180 / M_PI);
		if (m_flGaityaw > 180)
			m_flGaityaw = 180;
		if (m_flGaityaw < -180)
			m_flGaityaw = -180;
	}
}

/*
====================
StudioPlayerBlend

====================
*/
void CBasePlayer::StudioPlayerBlend( int *pBlend, float *pPitch )
{
	// calc up/down pointing
	float range = 45.0;

	*pBlend = (*pPitch * 3);

	if ( *pBlend <= -range )
	{
		*pBlend = 255;
	}
	else if ( *pBlend >= range )
	{
		*pBlend = 0;
	}
	else
	{
		*pBlend = 255 * ( range - *pBlend ) / ( 2 * range );
	}

	*pPitch = 0;
}

/*
====================
CalculatePitchBlend

====================
*/
void CBasePlayer::CalculatePitchBlend( void )
{
	int iBlend;
	float temp;

	temp = pev->angles.x;
	StudioPlayerBlend( &iBlend, &temp );

	pev->blending[1] = iBlend;
	m_flPitch = iBlend;
}

/*
====================
CalculateYawBlend

====================
*/
void CBasePlayer::CalculateYawBlend( void )
{
	float dt;
	float flYaw;	 // view direction relative to movement

	dt = gpGlobals->frametime;
	dt = max( 0.0, dt );
	dt = min( 1.0, dt );

	StudioEstimateGait();

	// calc side to side turning
	flYaw = pev->angles.y - m_flGaityaw;

	flYaw = fmod( flYaw, 360.0 );

	if (flYaw < -180)
	{
		flYaw = flYaw + 360;
	}
	else if (flYaw > 180)
	{
		flYaw = flYaw - 360;
	}

	float maxyaw = 120.0;

	if (flYaw > maxyaw)
	{
		m_flGaityaw = m_flGaityaw - 180;
		m_flGaitMovement = -m_flGaitMovement;
		flYaw = flYaw - 180;
	}
	else if (flYaw < -maxyaw)
	{
		m_flGaityaw = m_flGaityaw + 180;
		m_flGaitMovement = -m_flGaitMovement;
		flYaw = flYaw + 180;
	}

	float blend_yaw = ( flYaw / 90.0 ) * 128.0 + 127.0;
	blend_yaw = min( 255.0, blend_yaw );
	blend_yaw = max( 0.0, blend_yaw );
	
	blend_yaw = 255.0 - blend_yaw;

	pev->blending[0] = (int)(blend_yaw);
	m_flYaw = blend_yaw;
}

/*
====================
StudioProcessGait

====================
*/
void CBasePlayer::StudioProcessGait( void )
{
	mstudioseqdesc_t	*pseqdesc;
	float dt;

	dt = gpGlobals->frametime;
	dt = max( 0.0, dt );
	dt = min( 1.0, dt );

	// (1.3's stand-in here, float *pdt = &dt, is not needed by 1.4's compiler: measured, the
	// function is retail's 378 bytes without it)
	CalculateYawBlend();
	CalculatePitchBlend();

	studiohdr_t *pstudiohdr = (studiohdr_t *)GET_MODEL_PTR( ENT(pev) );

	if (!pstudiohdr)
		return;

	pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex) + pev->gaitsequence;

	// Calc gait frame
	if (pseqdesc->linearmovement[0] > 0)
	{
		m_flGaitframe += (m_flGaitMovement / pseqdesc->linearmovement[0]) * pseqdesc->numframes;
	}
	else
	{
		m_flGaitframe += pseqdesc->fps * dt * pev->framerate;
	}

	// Do modulo
	m_flGaitframe = m_flGaitframe - (int)(m_flGaitframe / pseqdesc->numframes) * pseqdesc->numframes;
	if (m_flGaitframe < 0)
	{
		m_flGaitframe += pseqdesc->numframes;
	}
}

// Counter-Strike 1.4 (Linux symbol; inline on Windows): the jump's stamina, pev->fuser2, is
// pm_shared's; Spawn clears it
void CBasePlayer::ResetStamina( void )
{
	pev->fuser2 = 0;
}

/*
====================
GetPlayerPitch

====================
*/
float GetPlayerPitch( const edict_t *pEdict )
{
	CBasePlayer *pPlayer = NULL;

	if ( pEdict )
		pPlayer = (CBasePlayer *)CBaseEntity::Instance( VARS( (edict_t *)pEdict ) );

	if ( pPlayer )
		return pPlayer->m_flPitch;

	return 0;
}

/*
====================
GetPlayerYaw

====================
*/
float GetPlayerYaw( const edict_t *pEdict )
{
	CBasePlayer *pPlayer = NULL;

	if ( pEdict )
		pPlayer = (CBasePlayer *)CBaseEntity::Instance( VARS( (edict_t *)pEdict ) );

	if ( pPlayer )
		return pPlayer->m_flYaw;

	return 0;
}

/*
====================
GetPlayerGaitsequence

====================
*/
int GetPlayerGaitsequence( const edict_t *pEdict )
{
	CBasePlayer *pPlayer = NULL;

	if ( pEdict )
		pPlayer = (CBasePlayer *)CBaseEntity::Instance( VARS( (edict_t *)pEdict ) );

	if ( pPlayer )
		return pPlayer->m_iGaitsequence;

	return 1;
}

// Counter-Strike 1.4: the dead player's body is made by the clients (message
// ClCorpse): his model, place, angles, the animation's time and sequence and
// the body group. PlayerDeathThink sends it when the dead camera starts.
void CBasePlayer::SpawnClientSideCorpse( void )
{
	char *infobuffer = g_engfuncs.pfnGetInfoKeyBuffer( edict() );	// Counter-Strike 1.6's names
	char *pModel = g_engfuncs.pfnInfoKeyValue( infobuffer, "model" );

	MESSAGE_BEGIN( MSG_ALL, gmsgSendCorpse );
		WRITE_STRING( pModel );
		WRITE_LONG( pev->origin.x * 128.0 );
		WRITE_LONG( pev->origin.y * 128.0 );
		WRITE_LONG( pev->origin.z * 128.0 );
		WRITE_COORD( pev->angles.x );
		WRITE_COORD( pev->angles.y );
		WRITE_COORD( pev->angles.z );
		WRITE_LONG( ( pev->animtime - gpGlobals->time ) * 100 );
		WRITE_BYTE( pev->sequence );
		WRITE_BYTE( pev->body );
	MESSAGE_END();
}

// Counter-Strike 1.4: does the armour cover the hit place? The kevlar
// covers the body and the arms, kevlar and helmet the head too (TakeDamage
// had this test twice, written out). Windows has it inline in TakeDamage.
BOOL CBasePlayer::IsArmored( int nHitGroup )
{
	BOOL fApplyArmor = FALSE;

	if ( m_iKevlar == 0 )
		return FALSE;

	switch ( nHitGroup )
	{
	case HITGROUP_HEAD:
		if ( m_iKevlar == 2 )
			fApplyArmor = TRUE;
		break;
	case HITGROUP_GENERIC:
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
	case HITGROUP_LEFTARM:
	case HITGROUP_RIGHTARM:
		fApplyArmor = TRUE;
		break;
	}

	return fApplyArmor;
}

// Counter-Strike 1.4: TakeDamage's test for the large flinch: a standing
// player hit by a rifle, a shotgun or the Desert Eagle, and (new) not in a
// leg. Inline in TakeDamage on Windows.
BOOL CBasePlayer::ShouldDoLargeFlinch( int nHitGroup, int nGunType )
{
	if ( FBitSet( pev->flags, FL_DUCKING ) )
		return FALSE;

	if ( nHitGroup == HITGROUP_LEFTLEG || nHitGroup == HITGROUP_RIGHTLEG )
		return FALSE;

	switch ( nGunType )
	{
	case WEAPON_SCOUT:
	case WEAPON_AUG:
	case WEAPON_SG550:
	case WEAPON_AWP:
	case WEAPON_M3:
	case WEAPON_M4A1:
	case WEAPON_G3SG1:
	case WEAPON_DEAGLE:
	case WEAPON_SG552:
	case WEAPON_AK47:
		return TRUE;
	}

	return FALSE;
}

// Counter-Strike 1.4 (Linux symbol): the client's settings out of the user info:
// _cl_autowepswitch (m_iAutoWepSwitch; 1 when the key is missing), _vgui_menus
// (m_bVGUIMenus) and _ah, the auto-help (m_bShowHints), both on when missing.
// ClientPutInServer and ClientUserInfoChanged call it; to 1.3 Spawn, Restore and
// PreThink read "lefthand" and "vgui_menus", and HintMessage read "ah".
void CBasePlayer::SetPrefsFromUserinfo( char *infobuffer )
{
	const char *pszKeyVal;	// Counter-Strike 1.6's name

	pszKeyVal = g_engfuncs.pfnInfoKeyValue( infobuffer, "_cl_autowepswitch" );

	if ( !strcmp( pszKeyVal, "" ) )
		m_iAutoWepSwitch = 1;
	else
		m_iAutoWepSwitch = atoi( pszKeyVal );

	pszKeyVal = g_engfuncs.pfnInfoKeyValue( infobuffer, "_vgui_menus" );

	if ( !strcmp( pszKeyVal, "" ) )
		m_bVGUIMenus = true;
	else
		m_bVGUIMenus = atoi( pszKeyVal ) != 0;

	pszKeyVal = g_engfuncs.pfnInfoKeyValue( infobuffer, "_ah" );

	if ( !strcmp( pszKeyVal, "" ) )
		m_bShowHints = true;
	else
		m_bShowHints = atoi( pszKeyVal ) != 0;
}
