/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
int MapTextureTypeStepType(char chTextureType);

// the world node graph
extern CGraph	WorldGraph;

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
int gmsgSelAmmo = 0;
int gmsgFlashlight = 0;
int gmsgFlashBattery = 0;
int gmsgResetHUD = 0;
int gmsgInitHUD = 0;
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
int gmsgAmmoPickup = 0;
int gmsgWeapPickup = 0;
int gmsgItemPickup = 0;
int gmsgHideWeapon = 0;
int gmsgSetCurWeap = 0;
int gmsgSayText = 0;
int gmsgTextMsg = 0;
int gmsgSetFOV = 0;
int gmsgShowMenu = 0;
int gmsgSendAudio = 0;	// Beta 3.0's radio: a sentence and the entity it comes from (client.dll plays it)
// Registered from Beta 4.0 on; before, never registered: the name is from Beta 3.1's
// Linux symbols, which have it here, after gmsgSendAudio. It gives retail's .bss of
// Betas 2.0 to 3.1: VC5 lays out zero-initialised data in definition order, and
// retail has 4 unreferenced bytes between gmsgSendAudio and PlayStepSound's
// iSkipStep (Betas 2.0 and 2.1 the same after gmsgShowMenu).
//
int gmsgRoundTime = 0;
// Beta 5.0 (leak)'s new messages (Beta 4.0's GiveItem and RemoveItem are gone)
int gmsgMoney = 0;
int gmsgBlinkAcct = 0;
int gmsgArmorType = 0;
int gmsgStatusValue = 0;
int gmsgStatusText = 0;
int gmsgStatusIcon = 0;
int gmsgBarTime = 0;
int gmsgReloadSound = 0;
int gmsgCrosshair = 0;	// Beta 5.0 (Linux symbol): the observers' crosshair on and off (CObserver)
int gmsgNVGToggle = 0;	// Beta 6.0: the night vision on and off (message NVGToggle, 1 byte; Linux symbol)
int gmsgRadar = 0;		// Beta 6.0: a team mate's position on the radar (message Radar, 7 bytes; Linux symbol)

LINK_ENTITY_TO_CLASS( player, CBasePlayer );

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
	case WEAPON_GLOCK18:	modelName = "models/w_glock18.mdl"; break;
	case WEAPON_M249:		modelName = "models/w_m249.mdl"; break;
	case WEAPON_M3:			modelName = "models/w_m3.mdl"; break;
	case WEAPON_M4A1:		modelName = "models/w_m4a1.mdl"; break;
	case WEAPON_MP5N:		modelName = "models/w_mp5.mdl"; break;
	case WEAPON_SG552:		modelName = "models/w_sg552.mdl"; break;
	case WEAPON_AUG:		modelName = "models/w_aug.mdl"; break;	// Beta 6.0
	case WEAPON_TMP:		modelName = "models/w_tmp.mdl"; break;
	case WEAPON_USP:		modelName = "models/w_usp.mdl"; break;
	case WEAPON_P90:		modelName = "models/w_p90.mdl"; break;
	case WEAPON_MAC10:		modelName = "models/w_mac10.mdl"; break;	// Beta 6.0
	case WEAPON_P228:		modelName = "models/w_p228.mdl"; break;
	case WEAPON_SCOUT:		modelName = "models/w_scout.mdl"; break;
	case WEAPON_KNIFE:		modelName = "models/w_knife.mdl"; break;
	case WEAPON_FLASHBANG:	modelName = "models/w_flashbang.mdl"; break;
	case WEAPON_HEGRENADE:	modelName = "models/w_hegrenade.mdl"; break;
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
// of the player's appearance (m_iAppearance: 1 urban, 2
// terror, 3 arab, 4 gsg9, 5 sas; anything else urban). The
// C4 carrier's flag picks between the same two models (sic).
//=========================================================
void CBasePlayer::SetPlayerModel( BOOL HasC4 )
{
	int clientIndex = entindex();	// name ours

	switch ( m_iAppearance )
	{
	case 1:
		CLIENT_COMMAND( edict(), "model urban\n" );
		g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", "urban" );
		break;
	case 2:
		if ( HasC4 )
			CLIENT_COMMAND( edict(), "model terror\n" );
		else
			CLIENT_COMMAND( edict(), "model terror\n" );
		g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", "terror" );
		break;
	case 3:
		if ( HasC4 )
			CLIENT_COMMAND( edict(), "model arab\n" );
		else
			CLIENT_COMMAND( edict(), "model arab\n" );
		g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", "arab" );
		break;
	// Beta 6.0: the arctic terrorists and the GIGN are new, the VIP has a model
	// of his own (4 arctic, 5 gsg9, 6 gign, 7 sas, 8 vip)
	case 4:
		if ( HasC4 )
			CLIENT_COMMAND( edict(), "model arctic\n" );
		else
			CLIENT_COMMAND( edict(), "model arctic\n" );
		g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", "arctic" );
		break;
	case 5:
		CLIENT_COMMAND( edict(), "model gsg9\n" );
		g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", "gsg9" );
		break;
	case 6:
		CLIENT_COMMAND( edict(), "model gign\n" );
		g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", "gign" );
		break;
	case 7:
		CLIENT_COMMAND( edict(), "model sas\n" );
		g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", "sas" );
		break;
	case 8:
		CLIENT_COMMAND( edict(), "model vip\n" );
		g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", "vip" );
		break;
	default:
		CLIENT_COMMAND( edict(), "model urban\n" );
		g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", "urban" );
		break;
	}
}

//=========================================================
// Radio - Beta 5.0 (leak): "<name> (RADIO): <msg_verbose>" in
// the chat of every living team mate who has not turned the
// radio off, and the sentence msg_id (the callers pass
// "%!MRAD_..." themselves) sent to him with his own entity
// index (sic, as Beta 3.0's); then the radio sprite over the
// speaker's head, for each of them alone.
//=========================================================
void CBasePlayer::Radio( const char *msg_id, const char *msg_verbose )
{
	if ( !IsPlayer() )
		return;

	// Beta 6.0: a radio call without text (msg_verbose NULL) sends only the
	// sound (sic: szMessage is not set then, and not used)
	char *szMessage;	// name ours

	if ( msg_verbose )
		szMessage = UTIL_VarArgs( "%s (RADIO): %s\n", STRING( pev->netname ), msg_verbose );

	CBaseEntity *pPlayer = NULL;
	pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
	{
		if ( pPlayer->IsPlayer() && pPlayer->pev->flags != FL_DORMANT )
		{
			CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

			if ( player->pev->deadflag == DEAD_NO && player->m_iTeam == m_iTeam && !player->m_fIgnoreRadio )
			{
				if ( msg_verbose )
					ClientPrint( pPlayer->pev, HUD_PRINTTALK, szMessage );

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
	return pev->origin + pev->view_ofs;
}

//=========================================================
// TraceAttack
//=========================================================
void CBasePlayer :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	bool bShouldBleed = true;	// name ours

	if ( pev->takedamage )
	{
		m_LastHitGroup = ptr->iHitgroup;

		// Beta 3.0: the hit kicks the view by the damage, by where it struck;
		// a hit where the kevlar is (head, body, arms) draws no blood. Beta 3.1:
		// much less kick (the head up to 12 degrees up and 9 sideways, the body
		// a tenth of the damage up to 4 degrees, arms and legs none; the
		// readme's "disorientation system changed") and both legs 0.75
		switch ( ptr->iHitgroup )
		{
		case HITGROUP_GENERIC:
			break;
		case HITGROUP_HEAD:
			if ( m_iKevlar == 2 )	// Beta 6.0: only the helmet (the assault suit) keeps the head from bleeding
				bShouldBleed = false;
			flDamage *= 4;
			pev->punchangle.x = flDamage * -0.5;
			if ( pev->punchangle.x < -12 )
				pev->punchangle.x = -12;
			pev->punchangle.z = flDamage * RANDOM_FLOAT( -1, 1 );
			if ( pev->punchangle.z < -9 )
				pev->punchangle.z = -9;
			else if ( pev->punchangle.z > 9 )
				pev->punchangle.z = 9;
			break;
		case HITGROUP_CHEST:	// Beta 4.0 to 4.1: the chest took 1.25 times the damage; Beta 5.0 (leak): the stomach again, as Beta 3.1
			if ( m_iKevlar )
				bShouldBleed = false;
			pev->punchangle.x = flDamage * -0.1;
			if ( pev->punchangle.x < -4 )
				pev->punchangle.x = -4;
			break;
		case HITGROUP_STOMACH:
			if ( m_iKevlar )
				bShouldBleed = false;
			flDamage *= 1.25;
			pev->punchangle.x = flDamage * -0.1;
			if ( pev->punchangle.x < -4 )
				pev->punchangle.x = -4;
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
		else if ( ptr->iHitgroup == HITGROUP_HEAD )
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
	// have suit diagnose the problem - ie: report damage type
	int bitsDamage = bitsDamageType;
	int ffound = TRUE;
	int fmajor;
	int fcritical;
	int fTookDamage;
	int ftrivial;
	float flBonus;
	float flRatio;
	float flHealthPrev = pev->health;

	flBonus = ARMOR_BONUS;
	flRatio = ARMOR_RATIO;

	// Beta 6.0: the VIP's armour lets only a quarter through
	if ( m_bIsVIP == true )
		flRatio = 0.25;

	// Beta 4.0: a blast (the C4's, an HE grenade's) is taken apart from the rest:
	// no flinch first, armour takes half of it at no extra cost, and a fatal one
	// throws the body (SetAnimation, Killed)
	if ( bitsDamageType & DMG_BLAST || bitsDamageType & DMG_HEGRENADE )
	{
		if ( !IsAlive() )
			return 0;

		// an HE grenade spares the thrower's team unless mp_friendlyfire (Beta 5.0 leak;
		// Beta 4.0 to 4.1: mp_friendly_grenade_damage)
		if ( CVAR_GET_FLOAT( "mp_friendlyfire" ) == 0 && ( bitsDamageType & DMG_HEGRENADE ) )
		{
			CBaseEntity *pInflictor = GetClassPtr( (CBaseEntity *)pevInflictor );	// name ours

			if ( !strcmp( STRING( pInflictor->pev->classname ), "grenade" ) )
			{
				// Beta 5.0: the thrower's own grenade hurts him
				if ( GetClassPtr( (CGrenade *)pevInflictor )->m_iTeam == m_iTeam && &edict()->v != pevAttacker )
					return 0;
			}
		}

		if ( !FNullEnt( ENT( pevInflictor ) ) )
			m_vecBlastDir = pev->origin - pevInflictor->origin;

		if ( pev->armorvalue )
		{
			if ( ( m_iKevlar == 1 && ( m_LastHitGroup == HITGROUP_CHEST || m_LastHitGroup == HITGROUP_STOMACH || m_LastHitGroup == HITGROUP_GENERIC || m_LastHitGroup == HITGROUP_LEFTARM || m_LastHitGroup == HITGROUP_RIGHTARM ) ) ||
				 ( m_iKevlar == 2 && ( m_LastHitGroup == HITGROUP_CHEST || m_LastHitGroup == HITGROUP_STOMACH || m_LastHitGroup == HITGROUP_GENERIC || m_LastHitGroup == HITGROUP_LEFTARM || m_LastHitGroup == HITGROUP_RIGHTARM || m_LastHitGroup == HITGROUP_HEAD ) ) )
			{
				float flNew = flDamage * flRatio;

				float flArmor;

				flArmor = flDamage - flNew;

				// Does this use more armor than we have?
				if (flArmor > pev->armorvalue)
				{
					flArmor = pev->armorvalue;
					flNew = flDamage - flArmor;
					pev->armorvalue = 0;
				}
				else
					pev->armorvalue -= flArmor;

				flDamage = flNew;

				if ( pev->armorvalue <= 0.0 )
					m_iKevlar = 0;
			}

			Pain( m_LastHitGroup, true );
		}
		else
			Pain( m_LastHitGroup, false );

		flDamage = m_lastDamageAmount = (int)flDamage;

		if ( flDamage >= pev->health )
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

		flDamage = CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );

		// reset damage time countdown for each type of time based damage player just sustained
		for ( int i = 0; i < CDMG_TIMEBASED; i++ )
			if ( bitsDamageType & ( DMG_PARALYZE << i ) )
				m_rgbTimeBasedDamage[i] = 0;

		ALERT( at_console, "EXPLOSION DAMAGE\n" );	// Beta 5.0 (leak)

		return flDamage;
	}

	CBasePlayer *pAttacker = (CBasePlayer *)CBaseEntity::Instance( pevAttacker );

	// Beta 4.0: a grenade's damage is taken whatever the rules say
	if ( !g_pGameRules->FPlayerCanTakeDamage( this, pAttacker ) && strcmp( "grenade", STRING( pevInflictor->classname ) ) )
	{
		// Refuse the damage
		return 0;
	}

	// slow the player down (Beta 6.0: to half; Beta 5.2: 0.35)
	m_flVelocityModifier = 0.5;

	// a heavy hit: more than 60 to the head, more than 20 anywhere else
	if ( ( m_LastHitGroup == HITGROUP_HEAD && flDamage > 60 ) || ( m_LastHitGroup != HITGROUP_HEAD && flDamage > 20 ) )
		m_fHeavyDamage = TRUE;
	else
		m_fHeavyDamage = FALSE;

	SetAnimation( PLAYER_FLINCH );

	// Already dead
	if ( !IsAlive() )
		return 0;
	// go take the damage first


	// a player's hits: half damage to a team mate (and to himself), who is told
	// so (Beta 5.0 leak: the money for hitting an enemy is gone)
	if ( GetClassPtr( (CBaseEntity *)pevAttacker )->IsPlayer() )
	{
		pAttacker = GetClassPtr( (CBasePlayer *)pevAttacker );

		if ( pAttacker->m_iTeam == m_iTeam )
		{
			ClientPrint( pAttacker->pev, HUD_PRINTCENTER, "You injured a teammate!\n" );

			// the victim's history decides whether the attacker hears the hint (sic)
			if ( !( m_flDisplayHistory & 0x200 ) )
			{
				m_flDisplayHistory |= 0x200;
				pAttacker->HintMessage( "Try not to injure your teammates.\n", FALSE, FALSE );
			}
		}

		if ( pAttacker->m_iTeam == m_iTeam )
			flDamage *= 0.5;

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
		if ( pAttacker->m_pActiveItem )
		switch ( pAttacker->m_pActiveItem->m_iId )
		{
		case WEAPON_USP:	// Beta 4.0: times 1, which VC5 folds away; the case keeps its jump-table slot, as retail's (an empty case gets none)
			flRatio *= 1;
			break;
		case WEAPON_M3:
			flRatio *= 1;
			break;
		case WEAPON_M4A1:	// Beta 6.0 (Beta 5.2: 1.55)
			flRatio *= 1.375;
			break;
		case WEAPON_AWP:
			flRatio *= 1.95;
			break;
		case WEAPON_MP5N:	// Beta 4.0 (Beta 4.1: 0.9; Beta 5.0 leak: 1)
			flRatio *= 1;
			break;
		case WEAPON_G3SG1:
			flRatio *= 1.65;
			break;
		case WEAPON_M249:
			flRatio *= 1.5;
			break;
		case WEAPON_DEAGLE:
			flRatio *= 1.5;
			break;
		case WEAPON_AK47:	// Beta 6.0 (Beta 5.2: 1.65)
			flRatio *= 1.55;
			break;
		case WEAPON_SG552:	// Beta 6.0 (Beta 5.2: 1.55)
			flRatio *= 1.45;
			break;
		case WEAPON_AUG:	// Beta 6.0
			flRatio *= 1.45;
			break;
		case WEAPON_GLOCK18:
			flRatio *= 1.05;
			break;
		case WEAPON_P90:	// Beta 3.1: the M3's 0.4 is gone (the default 0.5); Beta 6.0 1.35 (Beta 5.2: 1.4)
			flRatio *= 1.35;
			break;
		case WEAPON_MAC10:	// Beta 6.0
			flRatio *= 0.95;
			break;
		case WEAPON_KNIFE:	// Beta 3.1
			flRatio *= 1.7;
			break;
		case WEAPON_P228:	// Beta 4.0 (Beta 4.1: 1.3, was 1.4; Beta 5.0 leak: 1.4 again; Beta 6.0: 1.25)
			flRatio *= 1.25;
			break;
		case WEAPON_SCOUT:	// Beta 4.0 (Beta 6.0: 1.7, was 1.6)
			flRatio *= 1.7;
			break;
		}
	}

	// keep track of amount of damage last sustained
	m_lastDamageAmount = flDamage;

	// Armor: kevlar covers the body and the arms, kevlar and helmet the head too;
	// a hit anywhere sounds like armour while there is armour left
	if (pev->armorvalue && !(bitsDamageType & (DMG_FALL | DMG_DROWN)) )// armor doesn't protect against fall or drown damage!
	{
		if ( ( m_iKevlar == 1 && ( m_LastHitGroup == HITGROUP_CHEST || m_LastHitGroup == HITGROUP_STOMACH || m_LastHitGroup == HITGROUP_GENERIC || m_LastHitGroup == HITGROUP_LEFTARM || m_LastHitGroup == HITGROUP_RIGHTARM ) ) ||
			 ( m_iKevlar == 2 && ( m_LastHitGroup == HITGROUP_CHEST || m_LastHitGroup == HITGROUP_STOMACH || m_LastHitGroup == HITGROUP_GENERIC || m_LastHitGroup == HITGROUP_LEFTARM || m_LastHitGroup == HITGROUP_RIGHTARM || m_LastHitGroup == HITGROUP_HEAD ) ) )
		{
			float flNew = flDamage * flRatio;

			float flArmor;

			flArmor = (flDamage - flNew) * flBonus;

			// Does this use more armor than we have?
			if (flArmor > pev->armorvalue)
			{
				flArmor = pev->armorvalue;
				flArmor *= (1/flBonus);
				flNew = flDamage - flArmor;
				pev->armorvalue = 0;
			}
			else
				pev->armorvalue -= flArmor;

			flDamage = flNew;

			if ( pev->armorvalue <= 0.0 )
				m_iKevlar = 0;
		}

		Pain( m_LastHitGroup, true );
	}
	else
		Pain( m_LastHitGroup, false );

	// this cast to INT is critical!!! If a player ends up with 0.5 health, the engine will get that
	// as an int (zero) and think the player is dead! (this will incite a clientside screentilt, etc)
	fTookDamage = CBaseMonster::TakeDamage(pevInflictor, pevAttacker, (int)flDamage, bitsDamageType);

	// reset damage time countdown for each type of time based damage player just sustained

	{
		for (int i = 0; i < CDMG_TIMEBASED; i++)
			if (bitsDamageType & (DMG_PARALYZE << i))
				m_rgbTimeBasedDamage[i] = 0;
	}


	// how bad is it, doc?

	ftrivial = (pev->health > 75 || m_lastDamageAmount < 5);
	fmajor = (m_lastDamageAmount > 25);
	fcritical = (pev->health < 30);

	// handle all bits set in this damage message,
	// let the suit give player the diagnosis

	// UNDONE: add sounds for types of damage sustained (ie: burn, shock, slash )

	// UNDONE: still need to record damage and heal messages for the following types

		// DMG_BURN	
		// DMG_FREEZE
		// DMG_BLAST
		// DMG_SHOCK

	m_bitsDamageType |= bitsDamage; // Save this so we can report it to the client
	m_bitsHUDDamage = -1;  // make sure the damage bits get resent

	while (fTookDamage && (!ftrivial || (bitsDamage & DMG_TIMEBASED)) && ffound && bitsDamage)
	{
		ffound = FALSE;

		if (bitsDamage & DMG_CLUB)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG4", FALSE, SUIT_NEXT_IN_30SEC);	// minor fracture
			bitsDamage &= ~DMG_CLUB;
			ffound = TRUE;
		}
		if (bitsDamage & (DMG_FALL | DMG_CRUSH))
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG5", FALSE, SUIT_NEXT_IN_30SEC);	// major fracture
			else
				SetSuitUpdate("!HEV_DMG4", FALSE, SUIT_NEXT_IN_30SEC);	// minor fracture
	
			bitsDamage &= ~(DMG_FALL | DMG_CRUSH);
			ffound = TRUE;
		}
		
		if (bitsDamage & DMG_BULLET)
		{
			if (m_lastDamageAmount > 5)
				SetSuitUpdate("!HEV_DMG6", FALSE, SUIT_NEXT_IN_30SEC);	// blood loss detected
			//else
			//	SetSuitUpdate("!HEV_DMG0", FALSE, SUIT_NEXT_IN_30SEC);	// minor laceration
			
			bitsDamage &= ~DMG_BULLET;
			ffound = TRUE;
		}

		if (bitsDamage & DMG_SLASH)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG1", FALSE, SUIT_NEXT_IN_30SEC);	// major laceration
			else
				SetSuitUpdate("!HEV_DMG0", FALSE, SUIT_NEXT_IN_30SEC);	// minor laceration

			bitsDamage &= ~DMG_SLASH;
			ffound = TRUE;
		}
		
		if (bitsDamage & DMG_SONIC)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG2", FALSE, SUIT_NEXT_IN_1MIN);	// internal bleeding
			bitsDamage &= ~DMG_SONIC;
			ffound = TRUE;
		}

		if (bitsDamage & (DMG_POISON | DMG_PARALYZE))
		{
			SetSuitUpdate("!HEV_DMG3", FALSE, SUIT_NEXT_IN_1MIN);	// blood toxins detected
			bitsDamage &= ~(DMG_POISON | DMG_PARALYZE);
			ffound = TRUE;
		}

		if (bitsDamage & DMG_ACID)
		{
			SetSuitUpdate("!HEV_DET1", FALSE, SUIT_NEXT_IN_1MIN);	// hazardous chemicals detected
			bitsDamage &= ~DMG_ACID;
			ffound = TRUE;
		}

		if (bitsDamage & DMG_NERVEGAS)
		{
			SetSuitUpdate("!HEV_DET0", FALSE, SUIT_NEXT_IN_1MIN);	// biohazard detected
			bitsDamage &= ~DMG_NERVEGAS;
			ffound = TRUE;
		}

		if (bitsDamage & DMG_RADIATION)
		{
			SetSuitUpdate("!HEV_DET2", FALSE, SUIT_NEXT_IN_1MIN);	// radiation detected
			bitsDamage &= ~DMG_RADIATION;
			ffound = TRUE;
		}
		if (bitsDamage & DMG_SHOCK)
		{
			bitsDamage &= ~DMG_SHOCK;
			ffound = TRUE;
		}
	}

	if (fTookDamage && !ftrivial && fmajor && flHealthPrev >= 75) 
	{
		// first time we take major damage...
		// turn automedic on if not on
		SetSuitUpdate("!HEV_MED1", FALSE, SUIT_NEXT_IN_30MIN);	// automedic on

		// give morphine shot if not given recently
		SetSuitUpdate("!HEV_HEAL7", FALSE, SUIT_NEXT_IN_30MIN);	// morphine shot
	}
	
	if (fTookDamage && !ftrivial && fcritical && flHealthPrev < 75)
	{

		// already took major damage, now it's critical...
		if (pev->health < 6)
			SetSuitUpdate("!HEV_HLTH3", FALSE, SUIT_NEXT_IN_10MIN);	// near death
		else if (pev->health < 20)
			SetSuitUpdate("!HEV_HLTH2", FALSE, SUIT_NEXT_IN_10MIN);	// health critical
	
		// give critical health warnings
		if (!RANDOM_LONG(0,3) && flHealthPrev < 50)
			SetSuitUpdate("!HEV_DMG7", FALSE, SUIT_NEXT_IN_5MIN); //seek medical attention
	}

	// if we're taking time based damage, warn about its continuing effects
	if (fTookDamage && (bitsDamageType & DMG_TIMEBASED) && flHealthPrev < 75)
		{
			if (flHealthPrev < 50)
			{
				if (!RANDOM_LONG(0,3))
					SetSuitUpdate("!HEV_DMG7", FALSE, SUIT_NEXT_IN_5MIN); //seek medical attention
			}
			else
				SetSuitUpdate("!HEV_HLTH1", FALSE, SUIT_NEXT_IN_10MIN);	// health dropping
		}

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
		GiveAmmo( 24, "45acp", 48 );
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

	if ( m_fHasC4 )
	{
		m_fHasC4 = false;
		pev->body = 0;

		MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_STRING( "c4" );
		MESSAGE_END();

		bKillProgBar = TRUE;
	}

	if ( bKillProgBar )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgBarTime, NULL, pev );
			WRITE_SHORT( 0 );
		MESSAGE_END();
	}

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
	if ( m_fHasC4 )
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

	// Beta 4.0: a blast death (the C4) is not reported to the rules
	if ( m_fKilledByBlast == false )
		g_pGameRules->PlayerKilled( this, pevAttacker, g_pevLastInflictor );

	// Beta 6.0: the night vision goes off
	MESSAGE_BEGIN( MSG_ONE, gmsgNVGToggle, NULL, pev );
		WRITE_BYTE( 0 );
	MESSAGE_END();

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
	
	pev->modelindex = g_ulModelIndexPlayer;    // don't use eyes

#if !defined(DUCKFIX)
	pev->view_ofs		= Vector(0,0,-8);
#endif
	pev->deadflag		= DEAD_DYING;
	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_TOSS;
	ClearBits(pev->flags, FL_ONGROUND);

	ALERT( at_console, "Throw Direction : %i\n", m_iDeathThrow );	// Beta 5.0 (leak)

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
			if ( !strcmp( STRING( pevAttacker->classname ), "player" ) )
			{
				UTIL_MakeVectors( pevAttacker->angles );
				pev->velocity = gpGlobals->v_forward * RANDOM_FLOAT( 200, 300 );
				pev->velocity.z = RANDOM_FLOAT( 200, 300 );
			}
			break;
		case 4:		// Beta 4.0: blown away by the C4, the harder the nearer
			pev->velocity = ( m_vecBlastDir / m_vecBlastDir.Length() ) * ( 2300 - m_vecBlastDir.Length() ) / 4;
			pev->velocity.z = ( 2300 - m_vecBlastDir.Length() ) / 2.75;
			ALERT( at_console, "Fly with the Bomb's blast!!\n" );	// Beta 5.0 (leak)
			break;
		case 5:		// Beta 4.0: blown away by an HE grenade
			pev->velocity = ( m_vecBlastDir / m_vecBlastDir.Length() ) * ( 500 - m_vecBlastDir.Length() );
			pev->velocity.z = ( 350 - m_vecBlastDir.Length() ) * 1.5;
			ALERT( at_console, "Fly with the HE Grenade's blast!!\n" );	// Beta 5.0 (leak)
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
	if ( m_fHasC4 == true )
	{
		DropPlayerItem( "weapon_c4" );

		MESSAGE_BEGIN( MSG_ONE, gmsgBarTime, NULL, pev );
			WRITE_SHORT( 0 );
		MESSAGE_END();
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

		MESSAGE_BEGIN( MSG_ONE, gmsgBarTime, NULL, pev );
			WRITE_SHORT( 0 );
		MESSAGE_END();
	}

	m_bLeadsHostage = false;	// Beta 6.0
	BuyZoneIcon_Clear( this );

	SetThink(PlayerDeathThink);
	pev->nextthink = gpGlobals->time + 0.1;

	// UNDONE: Put this in, but add FFADE_PERMANENT and make fade time 8.8 instead of 4.12
	// UTIL_ScreenFade( edict(), Vector(128,0,0), 6, 15, 255, FFADE_OUT | FFADE_MODULATE );

	// Beta 4.0: gibbed only below -9000 health (was -40)
	if ( ( pev->health < -9000 && iGib != GIB_NEVER ) || iGib == GIB_ALWAYS )
	{
		GibMonster();	// This clears pev->model
		pev->effects |= EF_NODRAW;
		((CHalfLifeMultiplay *)g_pGameRules)->CheckWinConditions();
		return;
	}

	DeathSound();
	
	pev->angles.x = 0;
	pev->angles.z = 0;

	// Beta 5.0: a dead player may change teams once (chooseteam)
	m_fTeamChanged = false;
}

// Beta 5.0 (leak) (Linux symbol): whether the player is his team's commander
BOOL CBasePlayer::IsCommander( void )
{
	if ( g_pGameRules->IsMultiplayer() )
	{
		CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;	// name ours

		switch ( m_iTeam )
		{
		case TEAM_TERRORIST:
			if ( this == mp->m_pTerroristCommander )
				return TRUE;
			break;
		case TEAM_CT:
			if ( this == mp->m_pCTCommander )
				return TRUE;
			break;
		}
	}

	return FALSE;
}


// Set the activity based on an event or current state
void CBasePlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	int animDesired;
	float speed;
	char szAnim[64];

	// Beta 6.0: nothing to animate without a model (a player who has disappeared)
	if ( !pev->modelindex )
		return;

	// a flinch plays to its end, unless a new flinch comes or the player is dead
	if ( playerAnim != PLAYER_FLINCH && m_flFlinchTime >= gpGlobals->time && pev->health > 0.0 )
		return;

	speed = pev->velocity.Length2D();

	if (pev->flags & FL_FROZEN)
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

	switch (playerAnim) 
	{
	case PLAYER_JUMP:
		m_IdealActivity = ACT_HOP;
		break;
	
	case PLAYER_SUPERJUMP:
		m_IdealActivity = ACT_LEAP;
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
		case ACT_HOP:
		case ACT_LEAP:
		case ACT_DIESIMPLE:
			m_IdealActivity = m_Activity;
			break;
		default:
			m_IdealActivity = ACT_RANGE_ATTACK1;
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

	case PLAYER_FLINCH:
		if ( m_flNextFlinchTime > gpGlobals->time )
			return;
		m_IdealActivity = ACT_FLINCH;
		break;
	}

	switch (m_IdealActivity)
	{
	case ACT_HOVER:
	case ACT_LEAP:
	case ACT_SWIM:
	case ACT_HOP:
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

	case ACT_RANGE_ATTACK1:
		if ( FBitSet( pev->flags, FL_DUCKING ) )	// crouching
			strcpy( szAnim, "crouch_shoot_" );
		else
			strcpy( szAnim, "ref_shoot_" );
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

		pev->sequence		= animDesired;
		ResetSequenceInfo( );
		break;

	case ACT_WALK:
		if (m_Activity != ACT_RANGE_ATTACK1 || m_fSequenceFinished)
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
		else
		{
			animDesired = pev->sequence;
		}
		break;

	case ACT_FLINCH:
		m_Activity = m_IdealActivity;

		switch ( m_LastHitGroup )
		{
		case HITGROUP_GENERIC:	// Beta 5.0 (leak): before the head (the code's order)
		case HITGROUP_CHEST:
			if ( m_fHeavyDamage == TRUE )
				animDesired = LookupSequence( "flinch_stomach1" );
			else
				animDesired = LookupSequence( "sflinch_stomach1" );
			break;

		case HITGROUP_HEAD:
			if ( m_fHeavyDamage == TRUE )
				animDesired = LookupSequence( "flinch_head1" );
			else
				animDesired = LookupSequence( "sflinch_head1" );
			break;

		case HITGROUP_STOMACH:
			if ( m_fHeavyDamage == TRUE )
				animDesired = LookupSequence( "flinch_stomach1" );
			else
				animDesired = LookupSequence( "sflinch_stomach2" );
			break;

		case HITGROUP_LEFTARM:
			if ( m_fHeavyDamage == TRUE )
				animDesired = LookupSequence( "flinch_leftarm1" );
			else
				animDesired = LookupSequence( "sflinch_leftarm1" );
			break;

		case HITGROUP_RIGHTARM:
			if ( m_fHeavyDamage == TRUE )
				animDesired = LookupSequence( "flinch_rightarm1" );
			else
				animDesired = LookupSequence( "sflinch_rightarm1" );
			break;

		case HITGROUP_LEFTLEG:
			if ( m_fHeavyDamage == TRUE )
				animDesired = LookupSequence( "flinch_leftleg1" );
			else
				animDesired = LookupSequence( "sflinch_leftleg1" );
			break;

		case HITGROUP_RIGHTLEG:
			if ( m_fHeavyDamage == TRUE )
				animDesired = LookupSequence( "flinch_rightleg1" );
			else
				animDesired = LookupSequence( "sflinch_rightleg1" );
			break;

		default:	// Counter-Strike Beta 2.0
			animDesired = LookupSequence( "sflinch_stomach1" );
			break;
		}

		if ( FBitSet( pev->flags, FL_DUCKING ) )
		{
			if ( RANDOM_LONG( 0, 1 ) )
				animDesired = LookupSequence( "flinch_crouch1" );
			else
				animDesired = LookupSequence( "flinch_crouch2" );
		}

		if ( m_fHeavyDamage == TRUE )
		{
			m_flNextFlinchTime = gpGlobals->time + 0.1;
			m_flFlinchTime = gpGlobals->time + 1.2;
		}
		else
		{
			m_flNextFlinchTime = gpGlobals->time + 0.1;
			m_flFlinchTime = gpGlobals->time + 0.5;
		}

		pev->gaitsequence = 0;
		pev->sequence		= animDesired;
		pev->frame			= 0;
		ResetSequenceInfo( );
		return;

	case ACT_DIESIMPLE:
		if ( m_Activity == m_IdealActivity)
			return;
		m_Activity = m_IdealActivity;

		m_iUnused15C = 0;
		m_iDeathThrow = 0;

		// the death animation goes by where the last hit was
		switch ( m_LastHitGroup )
		{
		case HITGROUP_GENERIC:
			// Beta 3.0: one of five deaths at random
			switch ( RANDOM_LONG( 0, 4 ) )
			{
			case 0:
				animDesired = LookupSequence( "die_head1" );
				m_iDeathThrow = 2;
				break;
			case 1:
				m_iDeathThrow = 3;
				animDesired = LookupSequence( "die_rightarm1" );
				break;
			case 2:
				animDesired = LookupSequence( "die_legs1" );
				break;
			case 3:
				animDesired = LookupSequence( "die_legs2" );
				break;
			case 4:
				animDesired = LookupSequence( "die_backwards" );
				m_iDeathThrow = 3;
				break;
			default:
				animDesired = LookupSequence( "die_backwards" );
				m_iDeathThrow = 3;
				break;
			}
			break;

		case HITGROUP_HEAD:
			{
			int iRand = RANDOM_LONG( 0, 6 );

			if ( m_fHeavyDamage == TRUE )
				iRand += 1;	// Beta 2.0 (Beta 1.0: 2)

			if ( iRand == 0 )
				animDesired = LookupSequence( "headshot" );
			else if ( iRand == 1 || iRand == 2 )
			{
				animDesired = LookupSequence( "die_head1" );
				m_iDeathThrow = 2;
			}
			else if ( iRand == 3 || iRand == 4 )
			{
				animDesired = LookupSequence( "die_head2" );
				m_iDeathThrow = 1;
			}
			else
			{
				animDesired = LookupSequence( "die_head3" );
				m_iDeathThrow = 3;
			}
			}
			break;

		case HITGROUP_CHEST:
			if ( RANDOM_LONG( 0, 3 ) )
				animDesired = LookupSequence( "die_stomach1" );
			else
				animDesired = LookupSequence( "gutshot" );
			break;

		case HITGROUP_STOMACH:
			if ( m_fHeavyDamage == TRUE || RANDOM_LONG( 0, 3 ) == 0 )
			{
				animDesired = LookupSequence( "die_backwards" );
				m_iDeathThrow = 3;
			}
			else
				animDesired = LookupSequence( "die_stomach1" );
			break;

		case HITGROUP_LEFTARM:
			{
			int iRand = RANDOM_LONG( 0, 1 );

			if ( m_fHeavyDamage == TRUE || iRand == 0 )
			{
				animDesired = LookupSequence( "die_leftarm2" );
				m_iDeathThrow = 3;
			}
			else if ( iRand == 1 )
				animDesired = LookupSequence( "die_leftarm1" );
			}
			break;

		case HITGROUP_RIGHTARM:
			m_iDeathThrow = 3;
			animDesired = LookupSequence( "die_rightarm1" );
			break;

		case HITGROUP_LEFTLEG:
			{
			int iRand = RANDOM_LONG( 0, 4 );

			if ( iRand == 0 || iRand == 1 )
				animDesired = LookupSequence( "die_legs1" );
			else if ( iRand == 2 || iRand == 3 )
				animDesired = LookupSequence( "die_legs2" );
			else
				animDesired = LookupSequence( "die_forwards" );
			}
			break;

		case HITGROUP_RIGHTLEG:
			{
			int iRand = RANDOM_LONG( 0, 4 );

			if ( iRand == 0 || iRand == 1 )
				animDesired = LookupSequence( "die_legs1" );
			else if ( iRand == 2 || iRand == 3 )
				animDesired = LookupSequence( "die_legs2" );
			else
				animDesired = LookupSequence( "die_forwards" );
			}
			break;

		default:	// Counter-Strike Beta 2.0
			animDesired = LookupSequence( "die_forwards" );
			break;
		}

		if ( FBitSet( pev->flags, FL_DUCKING ) )
		{
			animDesired = LookupSequence( "die_crouch1" );
			m_iDeathThrow = 2;
		}
		else if ( m_fKilledByBlast == true || m_fKilledByHEGrenade == true )
		{
			// Beta 4.0: a blast throws the body (Killed: 4 the C4's, 5 an HE grenade's)
			UTIL_MakeVectors( pev->angles );

			if ( DotProduct( gpGlobals->v_forward, m_vecBlastDir ) > 0 )
				animDesired = LookupSequence( "die_leftarm2" );
			else if ( RANDOM_LONG( 0, 1 ) )
				animDesired = LookupSequence( "die_crouch1" );
			else
				animDesired = LookupSequence( "die_head3" );

			if ( m_fKilledByBlast == true )
				m_iDeathThrow = 4;
			else if ( m_fKilledByHEGrenade == true )
				m_iDeathThrow = 5;
		}

		// Already using the desired animation?
		if (pev->sequence == animDesired)
			return;

		pev->gaitsequence = 0;
		pev->sequence		= animDesired;
		pev->frame			= 0;
		ResetSequenceInfo( );
		return;

	}

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
	else if ( speed > 190 )	// Beta 6.0: the run animation from 190 (Beta 5.2: 220)
	{
		pev->gaitsequence	= LookupActivity( ACT_RUN );
	}
	else if (speed > 0)
	{
		pev->gaitsequence	= LookupActivity( ACT_WALK );
	}
	else
	{
		// pev->gaitsequence	= LookupActivity( ACT_WALK );
		pev->gaitsequence	= LookupSequence( "deep_idle" );
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

	if (pev->movetype == MOVETYPE_NOCLIP)
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
			// play leave water sound
			switch (RANDOM_LONG(0,3))
			{
			case 0: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade1.wav", 1, ATTN_NORM); break;
			case 1: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade2.wav", 1, ATTN_NORM); break;
			case 2: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade3.wav", 1, ATTN_NORM); break;
			case 3: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade4.wav", 1, ATTN_NORM); break;
			}

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
	
		SetBits(pev->flags, FL_INWATER);
		pev->dmgtime = 0;
	}
	
	if (!FBitSet(pev->flags, FL_WATERJUMP))
		pev->velocity = pev->velocity - 0.8 * pev->waterlevel * gpGlobals->frametime * pev->velocity;
}


// TRUE if the player is attached to a ladder
BOOL CBasePlayer::IsOnLadder( void )
{ 
	return (pev->movetype == MOVETYPE_FLY);
}


//
// check for a jump-out-of-water
//
void CBasePlayer::CheckWaterJump( )
{
	if ( IsOnLadder() )  // Don't water jump if on a ladders?  (This prevents the bug where 
		return;              //  you bounce off water when you are facing it on a ladder).

	Vector vecStart = pev->origin;
	vecStart.z += 8; 

	UTIL_MakeVectors(pev->angles);
	gpGlobals->v_forward.z = 0;
	gpGlobals->v_forward = gpGlobals->v_forward.Normalize();
	
	Vector vecEnd = vecStart + gpGlobals->v_forward * 24;
	
	TraceResult tr;
	UTIL_TraceLine(vecStart, vecEnd, ignore_monsters, ENT(pev)/*pentIgnore*/, &tr);
	if (tr.flFraction < 1)		// solid at waist
	{
		vecStart.z += pev->maxs.z - 8;
		vecEnd = vecStart + gpGlobals->v_forward * 24;
		pev->movedir = tr.vecPlaneNormal * -50;
		UTIL_TraceLine(vecStart, vecEnd, ignore_monsters, ENT(pev)/*pentIgnore*/, &tr);
		if (tr.flFraction == 1)		// open at eye level
		{
			SetBits(pev->flags, FL_WATERJUMP);
			pev->velocity.z = 225;
			pev->teleport_time = gpGlobals->time + 2;  // safety net
		}
	}
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
				ALERT( at_console, "%i \n", pItem->m_iId );	// Beta 5.0 (leak); the weapons bit is DropPlayerItem's now
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
	ThrowWeapon( "weapon_awp" );
	ThrowWeapon( "weapon_mp5navy" );
	ThrowWeapon( "weapon_tmp" );
	ThrowWeapon( "weapon_p90" );
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

	if ( ((CHalfLifeMultiplay *)g_pGameRules)->m_iMapHasCameras == 1 )
		UTIL_ScreenFade( this, Vector( 0, 0, 0 ), 0.5, 0.5, 0, 0 );
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
// game rules count his first round as VIP
//=========================================================
void CBasePlayer::MakeVIP( void )
{
	pev->body = 0;
	m_iAppearance = 8;

	CLIENT_COMMAND( edict(), "model vip\n" );
	g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", "vip" );

	m_iTeam = TEAM_CT;
	m_bIsVIP = true;
	((CHalfLifeMultiplay *)g_pGameRules)->m_pVIP = this;
	((CHalfLifeMultiplay *)g_pGameRules)->m_iConsecutiveVIP = 1;
}

//=========================================================
// JoiningThink - Beta 5.0 (leak) (Linux symbol): a player
// who has connected but not joined (m_iJoiningState):
//  start: the map's briefing (g_szMapBriefingText) in the
//   menu space with the screen faded out and the HUD hidden,
//   unless the map has none or the client's "dm" key is 0 or
//   off; the defuser icon cleared; his "ghosts" key (Beta 5.0;
//   the leak read "ignoreghosts", sic: 0 or off set the flag);
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
	CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;	// names ours

	switch ( m_iJoiningState )
	{
	case JOIN_DONE:
		return;

	case JOIN_START:
	{
		ResetMenu();

		BOOL bShowText = TRUE;
		char *infobuffer = g_engfuncs.pfnGetInfoKeyBuffer( edict() );
		char *value = g_engfuncs.pfnInfoKeyValue( infobuffer, "dm" );

		if ( value && *value )
		{
			if ( !strcmp( value, "0" ) || !strcmp( value, "off" ) )
				bShowText = FALSE;
		}

		if ( g_szMapBriefingText[0] && bShowText )
		{
			if ( mp->m_iMapHasCameras )
				UTIL_ScreenFade( this, Vector( 0, 0, 0 ), 0.5, 15.9, 255, FFADE_OUT | FFADE_MODULATE | FFADE_STAYOUT );
			else
				UTIL_ScreenFade( this, Vector( 10, 10, 10 ), 0, 0, 255, FFADE_OUT | FFADE_STAYOUT );

			m_iHideHUD |= HIDEHUD_WEAPONS | HIDEHUD_HEALTH | HIDEHUD_TIMER | HIDEHUD_MONEY;
			MenuPrint( this, g_szMapBriefingText );
			m_iJoiningState = JOIN_READING_TEXT;
		}
		else
			m_iJoiningState = JOIN_TEAM_MENU;

		MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_STRING( "defuser" );
		MESSAGE_END();

		m_fHasDefuser = false;
		m_flLastActivity = gpGlobals->time;
		m_fShowingLevelText = false;
		m_iIgnoreGhosts = 0;

		value = g_engfuncs.pfnInfoKeyValue( infobuffer, "ghosts" );	// Beta 5.0 (the leak: "ignoreghosts")
		if ( value && *value )
		{
			if ( !strcmp( value, "0" ) || !strcmp( value, "off" ) )
				m_iIgnoreGhosts = 1;
		}
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
		if ( !mp->m_iMapHasCameras )
			UTIL_ScreenFade( this, Vector( 0, 0, 0 ), 0, 0, 255, FFADE_IN );

		m_fNotKilled = false;
		m_fKicked = false;
		m_iIgnoreMessages = 0;
		m_iTeamKills = 0;
		m_iFOV = 90;
		m_fJustConnected = false;
		m_iNumSpawns = 0;
		ResetMaxSpeed();
		m_iJoiningState = JOIN_DONE;

		mp = (CHalfLifeMultiplay *)g_pGameRules;

		// Beta 6.0: on an escape map a counter-terrorist starts with 3000
		if ( mp->m_bMapHasEscapeZone == true && m_iTeam == TEAM_CT )
		{
			m_iAccount = 0;
			AddAccount( 3000, TRUE );
		}

		if ( !g_pGameRules->FPlayerCanRespawn( this ) )
		{
			pev->deadflag = DEAD_RESPAWNABLE;
			pev->classname = MAKE_STRING( "player" );
			pev->flags = FL_SPECTATOR | FL_CLIENT;
			m_fTeamChanged = false;	// Beta 5.0
			StartObserver( pev->origin, VARS( mp->GetPlayerSpawnSpot( this ) )->angles );
			mp->CheckWinConditions();
			return;
		}

		Spawn();
		mp->CheckWinConditions();

		// the first of each team to spawn when no restart is pending commands it
		if ( mp->m_flRestartRoundTime == 0 )
		{
			if ( m_iTeam == TEAM_CT )
			{
				if ( !mp->m_pCTCommander )
					mp->m_pCTCommander = this;
			}
			else
			{
				if ( !mp->m_pTerroristCommander )
				{
					if ( mp->m_fMapHasBombTarget )
						mp->GiveC4();
					else
						mp->m_pTerroristCommander = this;
				}
			}
		}

		// Beta 6.0: one more terrorist who can escape this round
		if ( m_iTeam == TEAM_TERRORIST )
			mp->m_iNumEscapers++;
		return;
	}

	// the intro cameras
	if ( m_pIntroCamera && gpGlobals->time >= m_flIntroCameraTime )
	{
		Vector vecAngles;	// names ours

		m_pIntroCamera = UTIL_FindEntityByClassname( m_pIntroCamera, "trigger_camera" );
		if ( !m_pIntroCamera )
			m_pIntroCamera = UTIL_FindEntityByClassname( NULL, "trigger_camera" );

		// Beta 5.0: the camera looks at its target (the leak took the camera's
		// angles); one without a target ends the tour
		CBaseEntity *pTarget = UTIL_FindEntityByTargetname( NULL, STRING( m_pIntroCamera->pev->target ) );
		if ( !pTarget )
		{
			m_pIntroCamera = NULL;
			return;
		}

		vecAngles = pTarget->pev->origin - m_pIntroCamera->pev->origin;
		vecAngles = vecAngles.Normalize();
		vecAngles = UTIL_VecToAngles( vecAngles );
		vecAngles.x = -vecAngles.x;

		UTIL_SetOrigin( pev, m_pIntroCamera->pev->origin );
		pev->angles = vecAngles;
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

	if ( m_fHasC4 == true )
	{
		DropPlayerItem( "weapon_c4" );

		MESSAGE_BEGIN( MSG_ONE, gmsgBarTime, NULL, pev );
			WRITE_SHORT( 0 );
		MESSAGE_END();
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

		MESSAGE_BEGIN( MSG_ONE, gmsgBarTime, NULL, pev );
			WRITE_SHORT( 0 );
		MESSAGE_END();
	}

	m_bLeadsHostage = false;
	BuyZoneIcon_Clear( this );

	SetThink( PlayerDeathThink );
	pev->nextthink = gpGlobals->time + 0.1;

	pev->angles.x = 0;
	pev->angles.z = 0;

	m_fTeamChanged = false;
}

void CBasePlayer::PlayerDeathThink(void)
{
	float flForward;

	// Beta 5.0 (leak): not while joining
	if ( m_iJoiningState != JOIN_DONE )
		return;

	// Beta 5.2: an observing player's camera
	if ( ( m_afPhysicsFlags & PFLAG_OBSERVER ) && m_cObserverMode == 0 )
		GhostMode();
	else if ( ( m_afPhysicsFlags & PFLAG_OBSERVER ) && m_cObserverMode == 1 )
		ChaseCamMode();

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
		StudioFrameAdvance( );

		m_iRespawnFrames++;
		if ( m_iRespawnFrames < 120 )  // animations should be no longer than this
			return;
	}

	// Beta 5.0 (leak): the time of death is when the body comes to rest
	if (pev->deadflag == DEAD_DYING)
	{
		m_fDeadTime = gpGlobals->time;
		pev->deadflag = DEAD_DEAD;
	}
	
	StopAnimation();

	pev->effects |= EF_NOINTERP;
	pev->framerate = 0.0;

	BOOL fAnyButtonDown = (pev->button);

// three seconds after his death, a player who is not observing yet goes to the dead camera
	if ( g_pGameRules->IsMultiplayer() && ( gpGlobals->time > (m_fDeadTime + 3) ) && !(m_afPhysicsFlags & PFLAG_OBSERVER) )
	{
		// go to dead camera. 
		pev->flags |= FL_SPECTATOR;
		StartDeathCam();

		// Beta 5.2: how to work the camera (Beta 6.0: always the hint with JUMP)
		if ( m_cObserverMode == 1 )
			HintMessage( "You are in ChaseCam Mode. \nPress JUMP to switch back to FreeLook mode.\n Press ATTACK to toggle teammate views.", TRUE, FALSE );
		else if ( m_cObserverMode == 0 )
			HintMessage( "You are in FreeLook Mode. \nPress JUMP to switch back to ChaseCam mode.\n Press ATTACK to toggle teammate views.", TRUE, FALSE );
	}

	// wait for all buttons released
	if ( pev->deadflag == DEAD_DEAD && m_iTeam != TEAM_UNASSIGNED )
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
		m_iRespawnFrames = 0;
		pev->nextthink = -1;
	}
}

//=========================================================
// GetPlayerToChase - Beta 5.2 (Linux symbol): the next
// living player from m_iChaseNext on (a team mate in the
// chase camera), round from 1 again, then anybody living;
// it becomes m_pChaseTarget (NULL when there is none)
//=========================================================
CBaseEntity *CBasePlayer::GetPlayerToChase( void )
{
	int iMyIndex = entindex();	// name ours
	int i;

	if ( m_iChaseNext > gpGlobals->maxClients )
		m_iChaseNext = 1;

	for ( i = m_iChaseNext; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pEntity = UTIL_PlayerByIndex( i );

		if ( pEntity && i != iMyIndex && pEntity->IsPlayer() )
		{
			CBasePlayer *pPlayer = GetClassPtr( (CBasePlayer *)pEntity->pev );

			if ( m_cObserverMode == 1 )
			{
				if ( pPlayer->pev->flags != FL_DORMANT && pPlayer->pev->deadflag == DEAD_NO && pPlayer->m_iTeam == m_iTeam )
				{
					m_pChaseTarget = pEntity;
					m_iChaseNext = i + 1;
					return pEntity;
				}
			}
			else if ( m_cObserverMode == 0 )
			{
				if ( pPlayer->pev->flags != FL_DORMANT && pPlayer->pev->deadflag == DEAD_NO )
				{
					m_pChaseTarget = pEntity;
					m_iChaseNext = i + 1;
					return pEntity;
				}
			}
		}
	}

	m_iChaseNext = 1;

	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pEntity = UTIL_PlayerByIndex( i );

		if ( pEntity && i != iMyIndex && pEntity->IsPlayer() )
		{
			CBasePlayer *pPlayer = GetClassPtr( (CBasePlayer *)pEntity->pev );

			if ( m_cObserverMode == 1 )
			{
				if ( pPlayer->pev->flags != FL_DORMANT && pPlayer->pev->deadflag == DEAD_NO && pPlayer->m_iTeam == m_iTeam )
				{
					m_pChaseTarget = pEntity;
					m_iChaseNext = i + 1;
					return pEntity;
				}
			}
			else if ( m_cObserverMode == 0 )
			{
				if ( pPlayer->pev->flags != FL_DORMANT && pPlayer->pev->deadflag == DEAD_NO )
				{
					m_pChaseTarget = pEntity;
					m_iChaseNext = i + 1;
					return pEntity;
				}
			}
		}
	}

	m_iChaseNext = 1;

	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pEntity = UTIL_PlayerByIndex( i );

		if ( pEntity && i != iMyIndex && pEntity->IsPlayer() )
		{
			CBasePlayer *pPlayer = GetClassPtr( (CBasePlayer *)pEntity->pev );

			if ( pPlayer->pev->flags != FL_DORMANT && pPlayer->pev->deadflag == DEAD_NO )
			{
				m_pChaseTarget = pEntity;
				m_iChaseNext = i + 1;
				return pEntity;
			}
		}
	}

	m_pChaseTarget = NULL;
	m_iChaseNext = 1;
	return NULL;
}

//=========================================================
// GhostMode - Beta 5.2 (Linux symbol): the free camera.
// JUMP goes to the chase camera (mp_chasecam 1 or 2),
// ATTACK jumps to the next living player
//=========================================================
void CBasePlayer::GhostMode( void )
{
	int iButtons = pev->button;	// name ours
	int iChaseCam = (int)CVAR_GET_FLOAT( "mp_chasecam" );	// name ours
	CBaseEntity *pTarget;	// name ours

	if ( ( iButtons & IN_JUMP ) && m_flNextObserverInput < gpGlobals->time && ( iChaseCam == 1 || iChaseCam == 2 ) )
	{
		m_flNextObserverInput = gpGlobals->time + 0.5;
		m_cObserverMode = 1;

		pTarget = GetPlayerToChase();

		if ( pTarget )
		{
			// Beta 6.0: the hint with JUMP whatever mp_chasecam says
			HintMessage( "You are in ChaseCam Mode. \nPress JUMP to switch back to FreeLook mode.\n Press ATTACK to toggle teammate views.", TRUE, FALSE );

			MESSAGE_BEGIN( MSG_ONE, gmsgCrosshair, NULL, pev );
				WRITE_BYTE( 0 );
			MESSAGE_END();

			SET_ORIGIN( edict(), pTarget->pev->origin + Vector( 0, 0, 48 ) );
		}
		else
			m_cObserverMode = 0;
	}
	else if ( ( iButtons & IN_ATTACK ) && m_flNextObserverInput < gpGlobals->time )
	{
		m_flNextObserverInput = gpGlobals->time + 0.5;

		pTarget = GetPlayerToChase();

		if ( pTarget )
			SET_ORIGIN( edict(), pTarget->pev->origin + Vector( 0, 0, 48 ) );
	}

	if ( m_flNextSBarUpdateTime < gpGlobals->time )
	{
		UpdateStatusBar();
		m_flNextSBarUpdateTime = gpGlobals->time + 0.25;
	}
}

//=========================================================
// ChaseCamMode - Beta 5.2 (Linux symbol): the chase camera,
// 48 units above the target's origin and 20 behind it.
// JUMP goes back to the free camera (mp_chasecam 0 or 2),
// ATTACK takes the next team mate
//=========================================================
void CBasePlayer::ChaseCamMode( void )
{
	int iButtons = pev->button;	// name ours
	int iChaseCam = (int)CVAR_GET_FLOAT( "mp_chasecam" );	// name ours

	if ( ( iButtons & IN_JUMP ) && m_flNextObserverInput < gpGlobals->time && ( iChaseCam == 0 || iChaseCam == 2 ) )
	{
		// Beta 6.0: the hint with JUMP whatever mp_chasecam says
		HintMessage( "You are in FreeLook Mode. \nPress JUMP to switch back to ChaseCam mode.\n Press ATTACK to toggle teammate views.", TRUE, FALSE );

		MESSAGE_BEGIN( MSG_ONE, gmsgCrosshair, NULL, pev );
			WRITE_BYTE( 1 );
		MESSAGE_END();

		m_flNextObserverInput = gpGlobals->time + 0.5;
		m_cObserverMode = 0;
		return;
	}

	if ( ( iButtons & IN_ATTACK ) && m_flNextObserverInput < gpGlobals->time )
	{
		m_flNextObserverInput = gpGlobals->time + 0.5;

		if ( GetPlayerToChase() )
		{
			UTIL_MakeVectors( m_pChaseTarget->pev->angles );
			pev->origin = m_pChaseTarget->pev->origin + Vector( 0, 0, 48 ) - gpGlobals->v_forward * 20;
		}
	}

	if ( m_pChaseTarget == NULL )
	{
		m_cObserverMode = 0;
		HintMessage( "You are in FreeLook Mode. \nPress JUMP to switch back to ChaseCam mode.\n Press ATTACK to toggle teammate views.", TRUE, FALSE );
		return;
	}

	if ( m_pChaseTarget->pev->deadflag != DEAD_NO )
		GetPlayerToChase();
	else
	{
		UTIL_MakeVectors( m_pChaseTarget->pev->angles );
		pev->origin = m_pChaseTarget->pev->origin + Vector( 0, 0, 48 ) - gpGlobals->v_forward * 20;
	}
}

//=========================================================
// RoundRespawn - Beta 5.0 (leak) (Linux symbol): the new
// round respawns every player but one still choosing his
// appearance
//=========================================================
void CBasePlayer::RoundRespawn( void )
{
	if ( m_iMenu != Menu_ChooseAppearance )
	{
		respawn( pev, FALSE );
		pev->button = 0;
		m_iRespawnFrames = 0;
		pev->nextthink = -1;
	}
}

//=========================================================
// StartDeathCam - find an intermission spot and send the
// player off into observer mode
//=========================================================
void CBasePlayer::StartDeathCam( void )
{
	edict_t *pSpot, *pNewSpot;
	int iRand;

	if ( pev->view_ofs == g_vecZero )
	{
		// don't accept subsequent attempts to StartDeathCam()
		return;
	}

	pSpot = FIND_ENTITY_BY_CLASSNAME( NULL, "info_intermission");	

	if ( !FNullEnt( pSpot ) )
	{
		// at least one intermission spot in the world.
		iRand = RANDOM_LONG( 0, 3 );

		while ( iRand > 0 )
		{
			pNewSpot = FIND_ENTITY_BY_CLASSNAME( pSpot, "info_intermission");
			
			if ( pNewSpot )
			{
				pSpot = pNewSpot;
			}

			iRand--;
		}

		StartObserver( pSpot->v.origin, pSpot->v.v_angle );
	}
	else
	{
		// no intermission spot. Push them up in the air, looking down at their corpse
		TraceResult tr;
		UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, 128 ), ignore_monsters, edict(), &tr );
		StartObserver( tr.vecEndPos, UTIL_VecToAngles( tr.vecEndPos - pev->origin  ) );
		return;
	}
}

// Beta 5.0 (leak): the observer flies (noclip), unseen, without weapons
// and health on the HUD; no longer ducking. Beta 5.2: he keeps his model
// index, has a 2-unit box, and the free camera shows the crosshair
void CBasePlayer::StartObserver( Vector vecPosition, Vector vecViewAngle )
{
	m_iHideHUD |= HIDEHUD_WEAPONS | HIDEHUD_HEALTH;
	SetBits( m_afPhysicsFlags, PFLAG_OBSERVER );	// Beta 6.0: the SDK's macro (its (int) cast loads this flag first; Beta 5.2's |= loaded it second)

	pev->view_ofs = g_vecZero;
	pev->angles = pev->v_angle = vecViewAngle;
	pev->fixangle = TRUE;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NOCLIP;
	pev->takedamage = DAMAGE_NO;
	pev->effects = EF_NODRAW;
	pev->modelindex = 0;	// Beta 6.0: no model again (Beta 5.2 kept it)
	UTIL_SetOrigin( pev, vecPosition );
	UTIL_SetSize( pev, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ) );

	ClearBits( m_afPhysicsFlags, PFLAG_DUCKING );
	ClearBits( pev->flags, FL_DUCKING );
	ResetMaxSpeed();

	if ( m_cObserverMode == 0 )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgCrosshair, NULL, pev );
			WRITE_BYTE( 1 );
		MESSAGE_END();
	}
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
		HintMessage( "You may USE the hostage again to\nstop him from following.", FALSE, FALSE );
		break;
	case TEAM_CT:
		HintMessage( "Lead the hostage to the rescue point!\nYou may USE the hostage again to\nstop him from following.", FALSE, FALSE );
		break;
	}

	m_flDisplayHistory |= 0x10;
}



void CBasePlayer::Jump()
{
	float		flScale;
	Vector		vecWallCheckDir;// direction we're tracing a line to find a wall when walljumping
	Vector		vecAdjustedVelocity;
	Vector		vecSpot;
	TraceResult	tr;
	
	if (FBitSet(pev->flags, FL_WATERJUMP))
		return;
	
	if (pev->waterlevel >= 2)
	{
		switch ((int)pev->watertype)
		{
		case CONTENT_WATER:	flScale = 100;	break;
		case CONTENT_SLIME:	flScale =  80;	break;
		default:			flScale =  50;	break;
		}

		pev->velocity.z = flScale;

		// play swiming sound
		if (m_flSwimTime < gpGlobals->time)
		{
			m_flSwimTime = gpGlobals->time + 1;
			switch (RANDOM_LONG(0,3))
			{ 
			case 0: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade1.wav", 1, ATTN_NORM); break;
			case 1: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade2.wav", 1, ATTN_NORM); break;
			case 2: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade3.wav", 1, ATTN_NORM); break;
			case 3: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade4.wav", 1, ATTN_NORM); break;
			}
		}
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

	ClearBits(pev->flags, FL_ONGROUND);		// don't stairwalk
	
	SetAnimation( PLAYER_JUMP );

	// play step sound for current texture at full volume

	PlayStepSound(MapTextureTypeStepType(m_chTextureType), 1.0);
	
	if ( FBitSet(pev->flags, FL_DUCKING ) || FBitSet(m_afPhysicsFlags, PFLAG_DUCKING) )
	{
		if ( m_fLongJump && (pev->button & IN_DUCK) && gpGlobals->time - m_flDuckTime < 1 && pev->velocity.Length() > 50 )
		{// If jump pressed within a second of duck while moving, long jump!
//			ALERT ( at_console, "LongJump!" );
			
			// play longjump 'servo' sound
			if ( RANDOM_LONG(0,1) )
			{
				; // UNDONE: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain2.wav", 1, ATTN_NORM);
			}
			else
			{
				; // UNDONE: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain4.wav", 1, ATTN_NORM);
			}

			pev->punchangle.x = -5;
			pev->velocity = gpGlobals->v_forward * (PLAYER_LONGJUMP_SPEED * 1.6);
			pev->velocity.z = sqrt( 2 * 800 * 56.0 ); // jump 56 units
			SetAnimation( PLAYER_SUPERJUMP );
		}
		else
		{// ducking jump
			pev->velocity.z = sqrt( 2 * 800 * 45.0 ); // jump 45 units
		}
	}
	else
	{
		pev->velocity.z = sqrt( 2 * 800 * 45.0 ); // jump 45 units
	}
	
	// If you're standing on a conveyor, add it's velocity to yours (for momentum)
	entvars_t *pevGround = VARS(pev->groundentity);
	if ( pevGround )
	{
		if ( pevGround->flags & FL_CONVEYOR )
			pev->velocity = pev->velocity + pev->basevelocity;

		// Beta 6.0: a jump off a train keeps the train's velocity
		if ( !strcmp( "func_tracktrain", STRING( pevGround->classname ) ) || !strcmp( "func_train", STRING( pevGround->classname ) ) )
			pev->velocity = pev->velocity + pevGround->velocity;
	}
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

// It takes 0.4 seconds to duck
#define TIME_TO_DUCK	0.4

// This is a little more complicated now than it used to be, but I think for the better -
// The player's origin no longer moves when ducking.  If you start ducking while standing on ground,
// your view offset is non-linearly blended to the ducking position and then your hull is changed.
// In air, duck works just like it used to - you pick up your feet (more like tucking).
// PFLAG_DUCKING was added to track the state where the player is in the process of ducking, but hasn't
// reached the crouched position yet.
//
// A few of nice side effects of this are:
//	a) On ground status does not change when ducking (good on trains, etc)
//	b) origin stays constant
//	c) Superjump can begin while still in run animation (nice when looking at others in multiplayer)
//
void CBasePlayer::Duck( )
{
	float time, duckFraction;

	if (pev->button & IN_DUCK) 
	{
#if !defined(DUCKFIX)
		if ( (m_afButtonPressed & IN_DUCK) && !(pev->flags & FL_DUCKING) )
		{
			m_flDuckTime = gpGlobals->time;
			SetBits(m_afPhysicsFlags,PFLAG_DUCKING);		// We are in the process of ducking
		}
		time = (gpGlobals->time - m_flDuckTime);

		// if we're not already done ducking
		if ( FBitSet( m_afPhysicsFlags, PFLAG_DUCKING ) )
		{
			// Finish ducking immediately if duck time is over or not on ground
			if ( time >= TIME_TO_DUCK || !(pev->flags & FL_ONGROUND) )
			{
				// HACKHACK - Fudge for collision bug - no time to fix this properly
				if ( pev->flags & FL_ONGROUND )
				{
					pev->origin = pev->origin - (VEC_DUCK_HULL_MIN - VEC_HULL_MIN);
					FixPlayerCrouchStuck( edict() );
				}


				UTIL_SetOrigin( pev, pev->origin );

				UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
				pev->view_ofs = VEC_DUCK_VIEW;
				SetBits(pev->flags,FL_DUCKING);				// Hull is duck hull
				ClearBits(m_afPhysicsFlags,PFLAG_DUCKING);	// Done ducking (don't ease-in when the player hits the ground)
			}
			else
			{
				// Calc parametric time
				duckFraction = UTIL_SplineFraction( time, (1.0/TIME_TO_DUCK) );
				pev->view_ofs = ((VEC_DUCK_VIEW - (VEC_DUCK_HULL_MIN - VEC_HULL_MIN)) * duckFraction) + (VEC_VIEW * (1-duckFraction));
			}
		}
#endif
		SetAnimation( PLAYER_WALK );
	}
	else 
	{
#if !defined(DUCKFIX)
		TraceResult trace;
		Vector newOrigin = pev->origin;

		// Beta 6.0: not while the duck is still going down
		if ( ( pev->flags & FL_ONGROUND ) && !( m_afPhysicsFlags & PFLAG_DUCKING ) )
			newOrigin = newOrigin + (VEC_DUCK_HULL_MIN - VEC_HULL_MIN);
		
		UTIL_TraceHull( newOrigin, newOrigin, dont_ignore_monsters, human_hull, ENT(pev), &trace );

		if ( !trace.fStartSolid )
		{
			ClearBits(pev->flags,FL_DUCKING);
			ClearBits(m_afPhysicsFlags,PFLAG_DUCKING);
			pev->view_ofs = VEC_VIEW;
			UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
			pev->origin = newOrigin;
		}
#endif
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


// play a footstep if it's time - this will eventually be frame-based. not time based.

#define STEP_CONCRETE	0		// default step sound
#define STEP_METAL		1		// metal floor
#define STEP_DIRT		2		// dirt, sand, rock
#define STEP_VENT		3		// ventillation duct
#define STEP_GRATE		4		// metal grating
#define STEP_TILE		5		// floor tiles
#define STEP_SLOSH		6		// shallow liquid puddle
#define STEP_WADE		7		// wading in liquid
#define STEP_LADDER		8		// climbing ladder

// Play correct step sound for material we're on or in

void CBasePlayer :: PlayStepSound(int step, float fvol)
{
	static int iSkipStep = 0;

	if ( !g_pGameRules->PlayFootstepSounds( this, fvol ) )
		return;

	// irand - 0,1 for right foot, 2,3 for left foot
	// used to alternate left and right foot
	int irand = RANDOM_LONG(0,1) + (m_iStepLeft * 2);

	m_iStepLeft = !m_iStepLeft;

	switch (step)
	{
	default:
	case STEP_CONCRETE:
		switch (irand)
		{
		// right foot
		case 0:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_step1.wav", fvol, ATTN_NORM);	break;
		case 1:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_step3.wav", fvol, ATTN_NORM);	break;
		// left foot
		case 2:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_step2.wav", fvol, ATTN_NORM);	break;
		case 3:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_step4.wav", fvol, ATTN_NORM);	break;
		}
		break;
	case STEP_METAL:
		switch(irand)
		{
		// right foot
		case 0:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_metal1.wav", fvol, ATTN_NORM);	break;
		case 1:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_metal3.wav", fvol, ATTN_NORM);	break;
		// left foot
		case 2:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_metal2.wav", fvol, ATTN_NORM);	break;
		case 3:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_metal4.wav", fvol, ATTN_NORM);	break;
		}
		break;
	case STEP_DIRT:
		switch(irand)
		{
		// right foot
		case 0:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_dirt1.wav", fvol, ATTN_NORM);	break;
		case 1:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_dirt3.wav", fvol, ATTN_NORM);	break;
		// left foot
		case 2:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_dirt2.wav", fvol, ATTN_NORM);	break;
		case 3:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_dirt4.wav", fvol, ATTN_NORM);	break;
		}
		break;
	case STEP_VENT:
		switch(irand)
		{
		// right foot
		case 0:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_duct1.wav", fvol, ATTN_NORM);	break;
		case 1:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_duct3.wav", fvol, ATTN_NORM);	break;
		// left foot
		case 2:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_duct2.wav", fvol, ATTN_NORM);	break;
		case 3:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_duct4.wav", fvol, ATTN_NORM);	break;
		}
		break;
	case STEP_GRATE:
		switch(irand)
		{
		// right foot
		case 0:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_grate1.wav", fvol, ATTN_NORM);	break;
		case 1:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_grate3.wav", fvol, ATTN_NORM);	break;
		// left foot
		case 2:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_grate2.wav", fvol, ATTN_NORM);	break;
		case 3:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_grate4.wav", fvol, ATTN_NORM);	break;
		}
		break;
	case STEP_TILE:
		if (!RANDOM_LONG(0,4))
			irand = 4;
		switch(irand)
		{
		// right foot
		case 0:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_tile1.wav", fvol, ATTN_NORM);	break;
		case 1:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_tile3.wav", fvol, ATTN_NORM);	break;
		// left foot
		case 2:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_tile2.wav", fvol, ATTN_NORM);	break;
		case 3:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_tile4.wav", fvol, ATTN_NORM);	break;
		case 4: EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_tile5.wav", fvol, ATTN_NORM);	break;
		}
		break;
	case STEP_SLOSH:
		switch(irand)
		{
		// right foot
		case 0:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_slosh1.wav", fvol, ATTN_NORM);	break;
		case 1:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_slosh3.wav", fvol, ATTN_NORM);	break;
		// left foot
		case 2:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_slosh2.wav", fvol, ATTN_NORM);	break;
		case 3:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_slosh4.wav", fvol, ATTN_NORM);	break;
		}
		break;
	case 9:	// Beta 6.0: snow (MapTextureTypeStepType's 9 for the 'N' textures; a number of its own, no STEP_ name)
		// three sounds a foot; the left foot's starts at 2 (sic: snow6 is never played, snow5 is the right foot's and the left's)
		switch ( RANDOM_LONG(0,2) + m_iStepLeft * 2 )
		{
		case 0:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_snow1.wav", fvol, ATTN_NORM);	break;
		case 1:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_snow3.wav", fvol, ATTN_NORM);	break;
		case 2:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_snow5.wav", fvol, ATTN_NORM);	break;
		case 3:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_snow2.wav", fvol, ATTN_NORM);	break;
		case 4:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_snow4.wav", fvol, ATTN_NORM);	break;
		case 5:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_snow6.wav", fvol, ATTN_NORM);	break;
		}
		break;
	case STEP_WADE:
		if ( iSkipStep == 0 )
		{
			iSkipStep++;
			break;
		}

		if ( iSkipStep++ == 3 )
		{
			iSkipStep = 0;
		}

		switch (irand)
		{
		// right foot
		case 0:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_wade1.wav", fvol, ATTN_NORM);	break;
		case 1:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_wade2.wav", fvol, ATTN_NORM);	break;
		// left foot
		case 2:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_wade3.wav", fvol, ATTN_NORM);	break;
		case 3:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_wade4.wav", fvol, ATTN_NORM);	break;
		}
		break;
	case STEP_LADDER:
		switch(irand)
		{
		// right foot
		case 0:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_ladder1.wav", fvol, ATTN_NORM);	break;
		case 1:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_ladder3.wav", fvol, ATTN_NORM);	break;
		// left foot
		case 2:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_ladder2.wav", fvol, ATTN_NORM);	break;
		case 3:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_ladder4.wav", fvol, ATTN_NORM);	break;
		}
		break;
	}
}	

// Simple mapping from texture type character to step type

int MapTextureTypeStepType(char chTextureType)
{
switch (chTextureType)
	{
	default:
	case CHAR_TEX_CONCRETE:	return STEP_CONCRETE;	
	case CHAR_TEX_METAL: return STEP_METAL;	
	case CHAR_TEX_DIRT: return STEP_DIRT;	
	case CHAR_TEX_VENT: return STEP_VENT;	
	case CHAR_TEX_GRATE: return STEP_GRATE;	
	case CHAR_TEX_TILE: return STEP_TILE;
	case CHAR_TEX_SLOSH: return STEP_SLOSH;
	case 'N': return 9;	// Beta 6.0: snow (a texture type and step number of their own; no names in the SDK)
	}
}

// Play left or right footstep based on material player is on or in

void CBasePlayer :: UpdateStepSound( void )
{
	int	fWalking;
	float fvol;
	char szbuffer[64];
	const char *pTextureName;
	Vector start, end;
	float rgfl1[3];
	float rgfl2[3];
	Vector knee;
	Vector feet;
	Vector center;
	float height;
	float speed;
	float velrun;
	float velwalk;
	float flduck;
	int	fLadder;
	int step;

	if (gpGlobals->time <= m_flTimeStepSound)
		return;

	if (pev->flags & FL_FROZEN)
		return;

	speed = pev->velocity.Length();

	// determine if we are on a ladder
	fLadder = IsOnLadder();

	// UNDONE: need defined numbers for run, walk, crouch, crouch run velocities!!!!	
	if (FBitSet(pev->flags, FL_DUCKING) || fLadder)
	{
		velwalk = 60;		// These constants should be based on cl_movespeedkey * cl_forwardspeed somehow
		velrun = 80;		// UNDONE: Move walking to server
		flduck = 0.1;
	}
	else
	{
		velwalk = 100;		// Beta 6.0 (Beta 5.2: 120 and 210)
		velrun = 140;
		flduck = 0.0;
	}

	// ALERT (at_console, "vel: %f\n", vecVel.Length());
	
	// if we're on a ladder or on the ground, and we're moving fast enough,
	// play step sound.  Also, if m_flTimeStepSound is zero, get the new
	// sound right away - we just started moving in new level.

	if ((fLadder || FBitSet (pev->flags, FL_ONGROUND)) && pev->velocity != g_vecZero 
		&& (speed >= velwalk || !m_flTimeStepSound))
	{
		SetAnimation( PLAYER_WALK );
		
		fWalking = speed < velrun;		

		center = knee = feet = (pev->absmin + pev->absmax) * 0.5;
		height = pev->absmax.z - pev->absmin.z;

		knee.z = pev->absmin.z + height * 0.2;
		feet.z = pev->absmin.z;

		// find out what we're stepping in or on...
		if (fLadder)
		{
			step = STEP_LADDER;
			fvol = 0.35;
			m_flTimeStepSound = gpGlobals->time + 0.35;
		}
		else if ( UTIL_PointContents ( knee ) == CONTENTS_WATER )
		{
			step = STEP_WADE;
			fvol = 0.65;
			m_flTimeStepSound = gpGlobals->time + 0.6;
		}
		else if (UTIL_PointContents ( feet ) == CONTENTS_WATER )
		{
			step = STEP_SLOSH;
			fvol = fWalking ? 0.2 : 0.5;
			m_flTimeStepSound = fWalking ? gpGlobals->time + 0.4 : gpGlobals->time + 0.3;		
		}
		else
		{
			// find texture under player, if different from current texture, 
			// get material type

			start = end = center;							// center point of player BB
			start.z = end.z = pev->absmin.z;				// copy zmin
			start.z += 4.0;									// extend start up
			end.z -= 24.0;									// extend end down
			
			start.CopyToArray(rgfl1);
			end.CopyToArray(rgfl2);

			pTextureName = TRACE_TEXTURE( ENT( pev->groundentity), rgfl1, rgfl2 );
			if ( pTextureName )
			{
				// strip leading '-0' or '{' or '!'
				if (*pTextureName == '-')
					pTextureName += 2;
				if (*pTextureName == '{' || *pTextureName == '!')
					pTextureName++;
				
				if (_strnicmp(pTextureName, m_szTextureName, CBTEXTURENAMEMAX-1))
				{
					// current texture is different from texture player is on...
					// set current texture
					strcpy(szbuffer, pTextureName);
					szbuffer[CBTEXTURENAMEMAX - 1] = 0;
					strcpy(m_szTextureName, szbuffer);
					
					// ALERT ( at_aiconsole, "texture: %s\n", m_szTextureName );

					// get texture type
					m_chTextureType = TEXTURETYPE_Find(m_szTextureName);	
				}
			}
			
			step = MapTextureTypeStepType(m_chTextureType);

			switch (m_chTextureType)
			{
			default:
			case CHAR_TEX_CONCRETE:						
				fvol = fWalking ? 0.2 : 0.5;
				m_flTimeStepSound = fWalking ? gpGlobals->time + 0.4 : gpGlobals->time + 0.3;
				break;

			case CHAR_TEX_METAL:	
				fvol = fWalking ? 0.2 : 0.5;
				m_flTimeStepSound = fWalking ? gpGlobals->time + 0.4 : gpGlobals->time + 0.3;
				break;

			case CHAR_TEX_DIRT:	
				fvol = fWalking ? 0.25 : 0.55;
				m_flTimeStepSound = fWalking ? gpGlobals->time + 0.4 : gpGlobals->time + 0.3;
				break;

			case CHAR_TEX_VENT:	
				fvol = fWalking ? 0.4 : 0.7;
				m_flTimeStepSound = fWalking ? gpGlobals->time + 0.4 : gpGlobals->time + 0.3;
				break;

			case CHAR_TEX_GRATE:
				fvol = fWalking ? 0.2 : 0.5;
				m_flTimeStepSound = fWalking ? gpGlobals->time + 0.4 : gpGlobals->time + 0.3;
				break;

			case CHAR_TEX_TILE:	
				fvol = fWalking ? 0.2 : 0.5;
				m_flTimeStepSound = fWalking ? gpGlobals->time + 0.4 : gpGlobals->time + 0.3;
				break;

			case CHAR_TEX_SLOSH:
				fvol = fWalking ? 0.2 : 0.5;
				m_flTimeStepSound = fWalking ? gpGlobals->time + 0.4 : gpGlobals->time + 0.3;
				break;
			}
		}
		
		m_flTimeStepSound += flduck; // slower step time if ducking

		// play the sound

		// 35% volume if ducking
		if ( pev->flags & FL_DUCKING )
			fvol *= 0.35;

		PlayStepSound(step, fvol);
	}
}


#define CLIMB_SHAKE_FREQUENCY	22	// how many frames in between screen shakes when climbing
#define	MAX_CLIMB_SPEED			200	// fastest vertical climbing speed possible
#define	CLIMB_SPEED_DEC			15	// climbing deceleration rate
#define	CLIMB_PUNCH_X			-7  // how far to 'punch' client X axis when climbing
#define CLIMB_PUNCH_Z			7	// how far to 'punch' client Z axis when climbing

//=========================================================
// UpdatePath - Beta 6.0 (Linux symbol): while a hostage
// follows him (PreThink, every 0.2 seconds), the player's
// trail gets a way-point where he went round a corner: when
// a hull trace from where he is now towards the newest point
// of m_vecOldOrigins is blocked, the position of the last
// call is pushed in front (so the hostage does not cut it)
//=========================================================
void CBasePlayer::UpdatePath( void )
{
	Vector vecDir = m_vecOldOrigins[0] - pev->origin;	// name ours
	TraceResult tr;

	UTIL_TraceHull( pev->origin + Vector( 0, 0, 2 ), pev->origin + ( vecDir.Normalize() * vecDir.Length() + Vector( 0, 0, 2 ) ), ignore_monsters, human_hull, ENT( pev ), &tr );

	if ( tr.flFraction != 1.0 )
	{
		ALERT( at_console, "Inserting a waypoint : %f , %f , %f\n", m_vecLastPathPos.x, m_vecLastPathPos.y, m_vecLastPathPos.z );

		for ( int i = 19; i > 0; i-- )
			m_vecOldOrigins[i] = m_vecOldOrigins[i - 1];

		m_vecOldOrigins[0] = m_vecLastPathPos;
	}

	m_vecLastPathPos = pev->origin;
}

//=========================================================
// CanPlayerBuy - Beta 6.0 (Linux symbol): buying is over 60
// seconds into the round, the VIP never buys, and
// info_map_parameters can forbid it to either team; display
// says why in the centre of the screen
//=========================================================
bool CBasePlayer::CanPlayerBuy( bool display )
{
	CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;	// name ours
	char szMessage[60];	// name ours; 60 bytes, the longest message's length (the frame says so)
	bool retval;	// name ours. Sic: not set when nothing forbids buying; VC6 keeps it in the slot
					// of display, so the answer is display: CanPlayerBuy( false ) is never true
					// (PreThink's buy hint never shows), CanPlayerBuy( true ) works

	if ( gpGlobals->time - mp->m_flRoundStartTime > 60 )
	{
		strcpy( szMessage, "60 seconds have passed... You can't buy anything now!\n" );
		retval = false;
	}

	if ( m_bIsVIP )
	{
		strcpy( szMessage, "You are the VIP... You can't buy anything!\n" );
		retval = false;
	}

	if ( mp->m_bCTCantBuy == true && m_iTeam == TEAM_CT )
	{
		strcpy( szMessage, "The CTs aren't allowed to buy anything on this map!\n" );
		retval = false;
	}

	if ( mp->m_bTCantBuy == true && m_iTeam == TEAM_TERRORIST )
	{
		strcpy( szMessage, "The Terrorists aren't allowed to buy anything on this map!\n" );
		retval = false;
	}

	if ( !retval )
	{
		if ( display == true )
			ClientPrint( pev, HUD_PRINTCENTER, szMessage );
		return false;
	}

	return true;
}

void CBasePlayer::PreThink(void)
{
	int buttonsChanged = (m_afButtonLast ^ pev->button);	// These buttons have changed this frame
	
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

	// fire, secondary fire or jump takes the map's text away (and does nothing else)
	if ( m_fShowingLevelText == true && ( m_afButtonPressed & ( IN_ATTACK | IN_ATTACK2 | IN_JUMP ) ) )
	{
		m_afButtonPressed &= ~( IN_ATTACK | IN_ATTACK2 | IN_JUMP );
		RemoveLevelText();
		m_fShowingLevelText = false;
	}

	UTIL_MakeVectors(pev->v_angle);             // is this still used?
	
	ItemPreFrame( );
	WaterMove();

	// a hit slows the player down: get back to full speed by 0.01 a frame on the ground
	if ( m_flVelocityModifier < 1.0 && FBitSet( pev->flags, FL_ONGROUND ) )
	{
		m_flVelocityModifier += 0.01;
		pev->velocity = pev->velocity * m_flVelocityModifier;
	}

	if ( m_flVelocityModifier > 1.0 )
		m_flVelocityModifier = 1;

	// Beta 3.0: the trail the hostages follow, the position every 0.2 seconds;
	// Beta 6.0: only way-points, recorded while a hostage follows (UpdatePath)
	if ( m_flNextOldOrigin <= gpGlobals->time || m_flNextOldOrigin == 0.0 )
	{
		m_flNextOldOrigin = gpGlobals->time + 0.2;

		if ( m_bLeadsHostage == true )
			UpdatePath();

		// Beta 5.0 (leak): a player who presses no button for longer than the
		// game rules' limit (mp_roundtime times 120 seconds) is kicked
		// when mp_autokick is set
		if ( pev->button )
			m_flLastActivity = gpGlobals->time;

		if ( gpGlobals->time - m_flLastActivity > ((CHalfLifeMultiplay *)g_pGameRules)->m_flMaxIdlePeriod && CVAR_GET_FLOAT( "mp_autokick" ) != 0 )
		{
			UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "%s has been idle for too long and has been kicked.\n", STRING( pev->netname ) );
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

	if (pev->waterlevel == 2)
		CheckWaterJump();
	else if ( pev->waterlevel == 0 )
	{
		pev->flags &= ~FL_WATERJUMP;	// Clear this if out of water
	}

	if ( pev->deadflag >= DEAD_DYING && pev->deadflag != DEAD_RESPAWNABLE )
	{
		PlayerDeathThink();
		return;
	}

	// So the correct flags get sent to client asap.
	//
	if ( m_afPhysicsFlags & PFLAG_ONTRAIN )
		pev->flags |= FL_ONTRAIN;
	else 
		pev->flags &= ~FL_ONTRAIN;

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
				return;
			}
		}
		else if ( !FBitSet( pev->flags, FL_ONGROUND ) || FBitSet( pTrain->pev->spawnflags, SF_TRACKTRAIN_NOCONTROL ) || (pev->button & (IN_MOVELEFT|IN_MOVERIGHT) ) )
		{
			// Turn off the train if you jump, strafe, or the train controls go dead
			m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
			m_iTrain = TRAIN_NEW|TRAIN_OFF;
			return;
		}

		pev->velocity = g_vecZero;
		vel = 0;
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

	// play a footstep if it's time - this will eventually be frame-based. not time based.
	
	UpdateStepSound();

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
		HintMessage( "Press the BUY key to purchase items.", FALSE, FALSE );
		m_flDisplayHistory |= 0x2;
	}

	if ( m_fHasC4 && !( m_flDisplayHistory & 0x1000 ) )	// have the bomb
	{
		m_flDisplayHistory |= 0x1000;
		HintMessage( "You have the bomb!\nFind the target zone or DROP\nthe bomb for another Terrorist.", FALSE, FALSE );
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
int gmsgGeigerRange;

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
		return;         // intermission or finale

	if (!IsAlive())
		return;

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
		float fvol = 0.5;

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
			
			switch ( RANDOM_LONG(0,1) )
			{
			case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_fallpain2.wav", 1, ATTN_NORM);
			case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_fallpain3.wav", 1, ATTN_NORM);
			}

			float flFallDamage = g_pGameRules->FlPlayerFallDamage( this );

			if ( flFallDamage > pev->health )
			{//splat
				// note: play on item channel because we play footstep landing on body channel
				EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/bodysplat.wav", 1, ATTN_NORM);
			}

			if ( flFallDamage > 0 )
			{
				TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), flFallDamage, DMG_FALL ); 
			}

			fvol = 1.0;
		}
		else if ( m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED / 2 )
		{
			// EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_jumpland2.wav", 1, ATTN_NORM);
			fvol = 0.85;
		}
		else if ( m_flFallVelocity < PLAYER_MIN_BOUNCE_SPEED )
		{
			fvol = 0;
		}

		if ( fvol > 0.0 )
		{
			// get current texture under player right away
			m_flTimeStepSound = 0;
			UpdateStepSound();
			
			// play step sound for current texture
			PlayStepSound(MapTextureTypeStepType(m_chTextureType), fvol);

		// knock the screen around a little bit, temporary effect
			pev->punchangle.x = m_flFallVelocity * 0.018;	// punch x axis

			if ( pev->punchangle.x > 8 )
			{
				pev->punchangle.x = 8;
			}

			if ( RANDOM_LONG(0,1) )
			{
				pev->punchangle.z = m_flFallVelocity * 0.009;
			}
		
		}

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
			SetAnimation( PLAYER_IDLE );
		else if ((pev->velocity.x || pev->velocity.y) && (FBitSet(pev->flags, FL_ONGROUND)))
			SetAnimation( PLAYER_WALK );
		else if (pev->waterlevel > 1)
			SetAnimation( PLAYER_WALK );
	}

	StudioFrameAdvance( );
	CheckPowerups(pev);

	UpdatePlayerSound();

	// (Beta 5.0 (leak) checked the zones here; Beta 5.0: UpdateClientData)

	// Track button info so we can detect 'pressed' and 'released' buttons next frame
	m_afButtonLast = pev->button;
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
		ALERT( at_console, "Looking for a VIP spawn point\n" );
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
		ALERT( at_console, "Looking for a CT spawn point\n" );	// Beta 6.0
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
		ALERT( at_console, "Looking for a TERRORIST spawn point\n" );	// Beta 6.0
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


void CBasePlayer::Spawn( void )
{
	pev->classname		= MAKE_STRING("player");
	pev->health			= 100;

	// a survivor of the last round keeps his armor and kevlar
	if ( !m_fNotKilled )
	{
		pev->armorvalue		= 0;
		m_iKevlar			= 0;
	}

	pev->takedamage		= DAMAGE_AIM;
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_WALK;
	pev->max_health		= pev->health;
	pev->flags			= FL_CLIENT;
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
	m_fHasAssaultSuit	= FALSE;
	m_pentBombTarget	= NULL;

	m_hintMessageQueue.Reset();

	m_flVelocityModifier = 1.0;
	m_flLastFired		= 0;
	m_flAccuracy		= 0.8;
	m_flNextBurstShot	= 0;
	m_iLastZoom			= 90;
	m_flLastSay			= 0;

	// Beta 3.0: the hostages' trail starts afresh, and 60 radio messages a life
	m_flNextOldOrigin	= 0;
	m_flNextRadio		= 0;
	m_iRadioMessages	= 60;

	// Beta 4.0: no C4, no blast death; Beta 5.0 (leak): the defuse kit is kept
	m_fHasC4			= false;
	m_fKilledByBlast	= false;
	m_fKilledByHEGrenade = false;

	// Beta 5.0 (leak): the hints about the buy key, the dead (multiplay's
	// PlayerKilled), being killed by a teammate and carrying the bomb come again
	m_flDisplayHistory	&= ~( 0x2 | 0x40 | 0x400 | 0x1000 );
	m_flNextZoneCheck	= 0;

	// Beta 5.2: the dead camera starts as mp_chasecam 1 asks (chase), else free
	m_flNextObserverInput = 0;
	m_iChaseNext		= 1;

	// Beta 6.0: no hostage path, not escaped, the radar and the kick vote afresh
	m_bLeadsHostage		= false;
	m_bEscaped			= false;
	m_flNextRadarUpdate	= gpGlobals->time;
	m_vecLastRadarOrigin = Vector( 0, 0, 0 );
	m_iCurrentKickVote	= 0;
	m_flNextVoteTime	= 0;

	// Beta 6.0: mp_chasecam 2 starts free; any other value leaves the mode as it was (sic)
	int iChaseCam = (int)CVAR_GET_FLOAT( "mp_chasecam" );	// name ours
	if ( iChaseCam == 0 )
		m_cObserverMode = 0;
	else if ( iChaseCam == 1 )
		m_cObserverMode = 1;
	else if ( iChaseCam == 2 )
		m_cObserverMode = 0;

	m_SbarString0[0]	= 0;
	m_SbarString1[0]	= 0;
	m_iNumSpawns++;

	for ( int i = 0; i < 20; i++ )
		m_vecOldOrigins[i] = Vector( 0, 0, 0 );

	// a sniper rifle hides its view model when zoomed in: show it again
	// (Beta 6.0: when there is none, the left- or right-handed model)
	if ( m_pActiveItem && !pev->viewmodel )
	{
		if ( m_bLeftHanded == true )
		{
			if ( m_pActiveItem->m_iId == WEAPON_AWP )
				pev->viewmodel = MAKE_STRING( "models/v_awp.mdl" );
			else if ( m_pActiveItem->m_iId == WEAPON_G3SG1 )
				pev->viewmodel = MAKE_STRING( "models/v_g3sg1.mdl" );
			else if ( m_pActiveItem->m_iId == WEAPON_SCOUT )
				pev->viewmodel = MAKE_STRING( "models/v_scout.mdl" );
		}
		else
		{
			if ( m_pActiveItem->m_iId == WEAPON_AWP )
				pev->viewmodel = MAKE_STRING( "models/v_awp_r.mdl" );
			else if ( m_pActiveItem->m_iId == WEAPON_G3SG1 )
				pev->viewmodel = MAKE_STRING( "models/v_g3sg1_r.mdl" );
			else if ( m_pActiveItem->m_iId == WEAPON_SCOUT )
				pev->viewmodel = MAKE_STRING( "models/v_scout_r.mdl" );
		}
	}

	m_iFOV				= 90;// init field of view.
	m_flNextDecalTime	= 0;// let this player decal as soon as he spawns.

	m_flgeigerDelay = gpGlobals->time + 2.0;	// wait a few seconds until user-defined message registrations
												// are recieved by all clients
	
	m_flTimeStepSound	= 0;
	m_iStepLeft = 0;
	m_flFieldOfView		= 0.5;// some monsters use this to determine whether or not the player is looking at them.

	m_bloodColor	= BLOOD_COLOR_RED;
	m_flNextAttack	= gpGlobals->time;
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
		{
			m_rgAmmo[i] = 0;
			m_rgAmmoLast[i] = 0;  // client ammo values also have to be reset  (the death hud clear messages does on the client side)
		}

		// Beta 6.0: no primary weapon, no night vision
		m_bHasPrimary		= false;
		m_bHasNightVision	= false;
		m_bNightVisionOn	= false;
	}

	// Beta 6.0: the night vision is off at every spawn
	MESSAGE_BEGIN( MSG_ONE, gmsgNVGToggle, NULL, pev );
		WRITE_BYTE( 0 );
	MESSAGE_END();
	m_bNightVisionOn = false;

	m_lastx = m_lasty = 0;

	g_pGameRules->PlayerSpawn( this );

	m_fNotKilled = true;

	// a kicked player stays dead
	if ( m_fKicked == true )
	{
		pev->health = 0;
		Killed( pev, GIB_NEVER );
	}

	ResetMaxSpeed();

	// Beta 6.0: the VIP: helmet and 200 armour, only the knife
	if ( m_bIsVIP == true )
	{
		m_iKevlar = 2;
		pev->armorvalue = 200;
		HintMessage( "You are the VIP\n Make your way to the safety zones!\n", TRUE, FALSE );
		RemoveAllItems( FALSE );
		GiveNamedItem( "weapon_knife" );
	}

	SetScoreboardAttributes();

	// Beta 6.0: everybody learns his team
	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( entindex() );
		WRITE_BYTE( m_iTeam );
	MESSAGE_END();
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

	gmsgSelAmmo = REG_USER_MSG("SelAmmo", sizeof(SelAmmo));
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
	gmsgShowGameTitle = REG_USER_MSG("GameTitle", 1);
	gmsgDeathMsg = REG_USER_MSG( "DeathMsg", -1 );
	gmsgScoreAttrib = REG_USER_MSG( "ScoreAttrib", 2 );	// Beta 5.0 (leak)
	gmsgScoreInfo = REG_USER_MSG( "ScoreInfo", 5 );
	gmsgTeamInfo = REG_USER_MSG( "TeamInfo", -1 );  // sets the name of a player's team
	gmsgTeamScore = REG_USER_MSG( "TeamScore", -1 );  // sets the score of a team on the scoreboard
	gmsgGameMode = REG_USER_MSG( "GameMode", 1 );
	gmsgMOTD = REG_USER_MSG( "MOTD", -1 );
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
	gmsgBarTime = REG_USER_MSG( "BarTime", -1 );
	gmsgReloadSound = REG_USER_MSG( "ReloadSound", 2 );
	gmsgCrosshair = REG_USER_MSG( "Crosshair", 1 );	// Beta 5.0
	gmsgNVGToggle = REG_USER_MSG( "NVGToggle", 1 );	// Beta 6.0
	gmsgRadar = REG_USER_MSG( "Radar", 7 );			// Beta 6.0

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


// Beta 5.0 (leak): the scoreboard's marks of one player as another sees
// them: dead (1), and to his teammates the bomb carrier (2) (name ours)
static void SetScoreAttrib( CBasePlayer *dest, CBasePlayer *src )
{
	int state = ( src->pev->deadflag != DEAD_NO );

	if ( src->m_fHasC4 && dest->m_iTeam == src->m_iTeam )
		state |= 2;

	// Beta 6.0: the VIP, to his team
	if ( src->m_bIsVIP == true && dest->m_iTeam == src->m_iTeam )
		state |= 4;

	MESSAGE_BEGIN( MSG_ONE, gmsgScoreAttrib, NULL, dest->edict() );
		WRITE_BYTE( src->entindex() );
		WRITE_BYTE( state );
	MESSAGE_END();
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
		FixPlayerCrouchStuck( edict() );
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	}
	else
	{
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
	}

	RenewItems();

	// Beta 5.0 (leak): after a saved game most hints come again
	m_flDisplayHistory &= ~( 0x4 | 0x8 | 0x10 | 0x20 | 0x80 | 0x100 | 0x200 | 0x800 | 0x8000 | 0x10000 | 0x20000 | 0x40000 | 0x80000 );	// Beta 5.0 to 5.2: 0x40000 stayed; Beta 6.0: the escape and VIP zone hints (0x40000, 0x80000) come again too
	SetScoreboardAttributes();

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

	// Beta 6.0: a counter-terrorist on an escape map starts with 3000 (sic: the
	// console says the map has no escape zone for a terrorist on one)
	if ( UTIL_FindEntityByClassname( NULL, "func_escapezone" ) && m_iTeam == TEAM_CT )
	{
		ALERT( at_console, "This map has an escape zone\n" );
		AddAccount( 3000, true );
	}
	else
	{
		ALERT( at_console, "This map DOES NOT have an escape zone\n" );
		AddAccount( 800, true );
	}

	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX(edict()) );
		WRITE_SHORT( 0 );
		WRITE_SHORT( 0 );
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

	// Beta 5.0 (leak): the kind of armor given (HasAssaultSuit)
	if ( !strcmp( pszName, "item_assaultsuit" ) )
		m_fHasAssaultSuit = TRUE;
	else if ( !strcmp( pszName, "item_kevlar" ) )
		m_fHasAssaultSuit = FALSE;
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
	case    204:  //  Demo recording, update client dll specific data again.
		ForceClientDllUpdate(); 
		break;
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

// Beta 5.0 (leak): the zones' signals since the last update are the new state
inline int CSignals::Update( void )
{
	int iOldState = m_iState;
	m_iState = m_iSignals;
	m_iSignals = 0;
	return iOldState ^ m_iState;	// the bits that changed
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
	if ( player->m_fHasC4 && !( player->m_flDisplayHistory & 0x10000 ) )	// in the target zone
	{
		player->m_flDisplayHistory |= 0x10000;
		player->HintMessage( "You are in the target zone.\nSelect the bomb in your inventory\nand plant it by holding FIRE!", FALSE, FALSE );
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
		player->HintMessage( "You are in a hostage rescue zone.\nFind the hostages and bring them here!", FALSE, FALSE );
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
		player->HintMessage( "You are in a terrorist escape zone.\nPrevent the terrorists from getting here!", FALSE, FALSE );
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
		player->HintMessage( "You are in a VIP escape zone.\nEscort the VIP to any one of these zones!", TRUE, FALSE );
	}
	else if ( player->m_iTeam == TEAM_TERRORIST && !( player->m_flDisplayHistory & 0x80000 ) )
	{
		player->m_flDisplayHistory |= 0x80000;
		player->HintMessage( "You are in a VIP escape zone.\nPrevent the VIP from reaching any one of these zones", TRUE, FALSE );
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
	// spectators pick nothing up
	if ( pev->flags == FL_SPECTATOR )
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
    if ( gpGlobals->time < m_flNextAttack )
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

    if ( gpGlobals->time < m_flNextAttack )
		return;

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

		// Beta 6.0: a console line numbered 1 to 13 before most of the messages
		// below (sic: left from tracking down a message that broke the client)
		ALERT( at_console, "1\n" );
		MESSAGE_BEGIN( MSG_ONE, gmsgResetHUD, NULL, pev );
		MESSAGE_END();

		if ( !m_fGameHUDInitialized )
		{
			ALERT( at_console, "2\n" );
			MESSAGE_BEGIN( MSG_ONE, gmsgInitHUD, NULL, pev );
			MESSAGE_END();

			g_pGameRules->InitHUD( this );
			m_fGameHUDInitialized = TRUE;
			if ( g_pGameRules->IsMultiplayer() )
			{
				FireTargets( "game_playerjoin", this, this, USE_TOGGLE, 0 );
			}
		}
		FireTargets( "game_playerspawn", this, this, USE_TOGGLE, 0 );

		// Beta 5.0 (leak): the HUD gets the money, the defuse kit's icon, the
		// round's time and no C4 icon (Beta 2.0 to 4.1's second ResetHUD for a
		// survivor's new round is gone)
		ALERT( at_console, "3\n" );
		MESSAGE_BEGIN( MSG_ONE, gmsgMoney, NULL, pev );
			WRITE_LONG( m_iAccount );
			WRITE_BYTE( 0 );
		MESSAGE_END();

		if ( m_fHasDefuser == true )
		{
			// Beta 5.0: green (the leak: blue, 15 15 200)
			ALERT( at_console, "4\n" );
			MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
				WRITE_BYTE( 1 );
				WRITE_STRING( "defuser" );
				WRITE_BYTE( 0 );
				WRITE_BYTE( 160 );
				WRITE_BYTE( 0 );
			MESSAGE_END();
		}

		if ( m_iTeam != TEAM_UNASSIGNED )
			SyncRoundTimer();

		SetBombIcon( FALSE );
	}

	if ( m_iHideHUD != m_iClientHideHUD )
	{
		ALERT( at_console, "5\n" );
		MESSAGE_BEGIN( MSG_ONE, gmsgHideWeapon, NULL, pev );
			WRITE_BYTE( m_iHideHUD );
		MESSAGE_END();

		m_iClientHideHUD = m_iHideHUD;
	}

	if ( m_iFOV != m_iClientFOV )
	{
		ALERT( at_console, "6\n" );
		MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
			WRITE_BYTE( m_iFOV );
		MESSAGE_END();

		// cache FOV change at end of function, so weapon updates can see that FOV has changed
	}

	// HACKHACK -- send the message to display the game title
	if (gDisplayTitle)
	{
		ALERT( at_console, "7\n" );
		MESSAGE_BEGIN( MSG_ONE, gmsgShowGameTitle, NULL, pev );
		WRITE_BYTE( 0 );
		MESSAGE_END();
		gDisplayTitle = 0;
	}

	if (pev->health != m_iClientHealth)
	{
		int iHealth = max( pev->health, 0 );  // make sure that no negative health values are sent

		// send "health" update message
		ALERT( at_console, "8\n" );
		MESSAGE_BEGIN( MSG_ONE, gmsgHealth, NULL, pev );
			WRITE_BYTE( iHealth );
		MESSAGE_END();

		m_iClientHealth = pev->health;
	}


	if (pev->armorvalue != m_iClientBattery)
	{
		m_iClientBattery = pev->armorvalue;

		ALERT( at_console, "9\n" );
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

		ALERT( at_console, "10\n" );
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

		ALERT( at_console, "11\n" );
		MESSAGE_BEGIN( MSG_ONE, gmsgFlashBattery, NULL, pev );
		WRITE_BYTE(m_iFlashBattery);
		MESSAGE_END();
	}


	if (m_iTrain & TRAIN_NEW)
	{
		ALERT( at_console, "12\n" );
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

			ALERT( at_console, "13\n" );
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

		if ( ( pWeapon->iFlags() & ITEM_FLAG_EXHAUSTIBLE ) && !AmmoInventory( pWeapon->m_iPrimaryAmmoType ) && !pWeapon->m_iClip )
		{
			m_flDisplayHistory |= 0x8000;	// out of ammunition
			HintMessage( "You are out of ammunition.\nReturn to a buy zone to purchase more.\n", FALSE, FALSE );
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
// freeze period, else what the weapon in his hands allows
// (260 with none)
//=========================================================
void CBasePlayer::ResetMaxSpeed( void )
{
	float speed;	// name ours

	if ( m_afPhysicsFlags & PFLAG_OBSERVER )
		speed = 350;	// Beta 6.0 (Beta 5.2: 320)
	else if ( g_pGameRules->IsMultiplayer() && ((CHalfLifeMultiplay *)g_pGameRules)->IsFreezePeriod() )
		speed = 0.001;
	else if ( m_bIsVIP == true )	// Beta 6.0: the VIP walks at 220
		speed = 220;
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
bool CBasePlayer::HintMessage( const char *pMessage, BOOL bDisplayIfPlayerDead, BOOL bForce )
{
	// Beta 5.0: IsAlive (the leak: the dead flag), and a forced hint is
	// queued whatever the client's autohelp key says
	if ( !bDisplayIfPlayerDead && !IsAlive() )
		return false;

	if ( bForce )
		return m_hintMessageQueue.AddMessage( pMessage );

	char *infobuffer = g_engfuncs.pfnGetInfoKeyBuffer( edict() );	// names ours
	const char *autohelp = g_engfuncs.pfnInfoKeyValue( infobuffer, "ah" );

	if ( !autohelp || *autohelp == '0' )
		return true;

	return m_hintMessageQueue.AddMessage( pMessage );
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

		// Beta 5.0 (leak): nor at teammates
		if ( pEntity->IsPlayer() && ((CBasePlayer *)pEntity)->m_iTeam == m_iTeam )
			continue;

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

	// Beta 5.2: the chase camera's target
	if ( ( m_afPhysicsFlags & PFLAG_OBSERVER ) && m_cObserverMode == 1 )
	{
		CBasePlayer *pTarget = NULL;

		if ( m_pChaseTarget )
			pTarget = GetClassPtr( (CBasePlayer *)m_pChaseTarget->pev );

		if ( !FNullEnt( pTarget ) )
		{
			strcpy( sbuf1, "1 (Chase mode) %c1: %p2\n2  Health: %i3%%" );
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
					CBasePlayer *pPlayer = (CBasePlayer *)pEntity;

					newSBarState[ SBAR_ID_TARGETNAME ] = ENTINDEX( pEntity->edict() );
					newSBarState[ SBAR_ID_TARGETTYPE ] = ( pPlayer->m_iTeam == m_iTeam ) ? SBAR_TARGETTYPE_TEAMMATE : SBAR_TARGETTYPE_ENEMY;

					if ( m_iTeam != pPlayer->m_iTeam && !( pev->flags & FL_SPECTATOR ) )
					{
						strcpy( sbuf1, "1 %c1: %p2" );

						if ( !( m_flDisplayHistory & 0x100 ) )	// enemy spotted
						{
							m_flDisplayHistory |= 0x100;
							HintMessage( "You have spotted an enemy.", FALSE, FALSE );
						}
					}
					else
					{
						// teammates (and spectators) see the health
						strcpy( sbuf1, "1 %c1: %p2\n2  Health: %i3%%" );
						newSBarState[ SBAR_ID_TARGETHEALTH ] = 100 * ( pEntity->pev->health / pEntity->pev->max_health );

						if ( !( m_flDisplayHistory & 0x80 ) && !( pev->flags & FL_SPECTATOR ) )	// friend spotted
						{
							m_flDisplayHistory |= 0x80;
							HintMessage( "You have spotted a friend.", FALSE, FALSE );
						}
					}
				}
				else if ( pEntity->Classify() == CLASS_HUMAN_PASSIVE )
				{
					strcpy( sbuf1, "1 %c1  Health: %i3%%" );
					newSBarState[ SBAR_ID_TARGETTYPE ] = SBAR_TARGETTYPE_HOSTAGE;
					newSBarState[ SBAR_ID_TARGETHEALTH ] = 100 * ( pEntity->pev->health / pEntity->pev->max_health );

					// a hostage seen from afar: the team's task; close by: how to lead it
					if ( !( m_flDisplayHistory & 0x4 ) && tr.flFraction > 0.1 )
					{
						m_flDisplayHistory |= 0x4;

						switch ( m_iTeam )
						{
						case TEAM_TERRORIST:
							HintMessage( "Prevent the Counter-terrorists from\nrescuing the hostages!", TRUE, FALSE );
							break;
						case TEAM_CT:
							HintMessage( "Rescue the hostages for money!", TRUE, FALSE );
							break;
						}
					}
					else if ( !( m_flDisplayHistory & 0x8 ) && tr.flFraction <= 0.1 )
					{
						m_flDisplayHistory |= 0x4 | 0x8;
						HintMessage( "Press USE to get the hostage to follow you.", FALSE, FALSE );
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
				ClientPrint( pev, HUD_PRINTCENTER, "This weapon cannot be dropped" );
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
				m_fHasC4 = false;
				pev->body = 0;
				SetBombIcon( FALSE );

				// Beta 5.0: an arming bomb's progress bar goes
				MESSAGE_BEGIN( MSG_ONE, gmsgBarTime, NULL, pWeapon->m_pPlayer->pev );
					WRITE_SHORT( 0 );
				MESSAGE_END();

				if ( ((CHalfLifeMultiplay *)g_pGameRules)->m_flRestartRoundTime == 0 )
					UTIL_ClientPrintAll( HUD_PRINTCENTER, "%s dropped the bomb\n", STRING( pev->netname ) );	// Beta 5.0 (the leak: "The bomb was dropped by %s")
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
// arab, sas arctic, the rest terror; terror becomes gsg9,
// arctic sas, the rest urban
//=========================================================
void CBasePlayer::SwitchTeam( void )
{
	char szCommand[32];	// names ours
	char szModel[32];

	if ( m_iTeam == TEAM_CT )
	{
		m_iTeam = TEAM_TERRORIST;

		switch ( m_iAppearance )
		{
		case 1:
			m_iAppearance = 3;
			strcpy( szCommand, "model arab\n" );
			strcpy( szModel, "arab" );
			break;
		case 5:
		case 6:
			m_iAppearance = 2;
			strcpy( szCommand, "model terror\n" );
			strcpy( szModel, "terror" );
			break;
		case 7:
			m_iAppearance = 4;
			strcpy( szCommand, "model arctic\n" );
			strcpy( szModel, "arctic" );
			break;
		default:
			m_iAppearance = 2;
			strcpy( szCommand, "model terror\n" );
			strcpy( szModel, "terror" );
			break;
		}

		CLIENT_COMMAND( edict(), szCommand );
		g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", szModel );
	}
	else if ( m_iTeam == TEAM_TERRORIST )
	{
		m_iTeam = TEAM_CT;

		switch ( m_iAppearance )
		{
		case 2:
			m_iAppearance = 5;
			strcpy( szCommand, "model gsg9\n" );
			strcpy( szModel, "gsg9" );
			break;
		case 3:
			m_iAppearance = 1;
			strcpy( szCommand, "model urban\n" );
			strcpy( szModel, "urban" );
			break;
		case 4:
			m_iAppearance = 7;
			strcpy( szCommand, "model sas\n" );
			strcpy( szModel, "sas" );
			break;
		default:
			m_iAppearance = 1;
			strcpy( szCommand, "model urban\n" );
			strcpy( szModel, "urban" );
			break;
		}

		CLIENT_COMMAND( edict(), szCommand );
		g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", szModel );
	}
}

// Beta 5.0 (leak): kevlar and helmet, with armor left
BOOL CBasePlayer::HasAssaultSuit( void )
{
	if ( !m_fHasAssaultSuit || pev->armorvalue == 0 )
		return FALSE;

	return TRUE;
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


