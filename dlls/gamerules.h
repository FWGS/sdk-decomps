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
//=========================================================
// GameRules
//=========================================================

//#include "weapons.h"
//#include "items.h"
class CBasePlayerItem;
class CBasePlayer;
class CItem;
class CBasePlayerAmmo;

// weapon respawning return codes
enum
{	
	GR_NONE = 0,
	
	GR_WEAPON_RESPAWN_YES,
	GR_WEAPON_RESPAWN_NO,
	
	GR_AMMO_RESPAWN_YES,
	GR_AMMO_RESPAWN_NO,
	
	GR_ITEM_RESPAWN_YES,
	GR_ITEM_RESPAWN_NO,

	GR_PLR_DROP_GUN_ALL,
	GR_PLR_DROP_GUN_ACTIVE,
	GR_PLR_DROP_GUN_NO,

	GR_PLR_DROP_AMMO_ALL,
	GR_PLR_DROP_AMMO_ACTIVE,
	GR_PLR_DROP_AMMO_NO,
};

// Player relationship return codes
enum
{
	GR_NOTTEAMMATE = 0,
	GR_TEAMMATE,
	GR_ENEMY,
	GR_ALLY,
	GR_NEUTRAL,
};

class CGameRules
{
public:
	virtual void RefreshSkillData( void );// fill skill data struct with proper values
	virtual void Think( void ) = 0;// GR_Think - runs every server frame, should handle any timer tasks, periodic events, etc.
	virtual BOOL IsAllowedToSpawn( CBaseEntity *pEntity ) = 0;  // Can this item spawn (eg monsters don't spawn in deathmatch).

	virtual BOOL FAllowFlashlight( void ) = 0;// Are players allowed to switch on their flashlight?
	virtual BOOL FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon ) = 0;// should the player switch to this weapon?
	virtual BOOL GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon ) = 0;// I can't use this weapon anymore, get me the next best one.

// Functions to verify the single/multiplayer status of a game
	virtual BOOL IsMultiplayer( void ) = 0;// is this a multiplayer game? (either coop or deathmatch)
	virtual BOOL IsDeathmatch( void ) = 0;//is this a deathmatch game?
	virtual BOOL IsTeamplay( void ) { return FALSE; };// is this deathmatch game being played with team rules?
	virtual BOOL IsCoOp( void ) = 0;// is this a coop game?
	virtual const char *GetGameDescription( void ) { return "CounterStrike"; }  // this is the game name that gets seen in the server browser
	
// Client connection/disconnection
	virtual BOOL ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] ) = 0;// a client just connected to the server (player hasn't spawned yet)
	virtual void InitHUD( CBasePlayer *pl ) = 0;		// the client dll is ready for updating
	virtual void ClientDisconnected( edict_t *pClient ) = 0;// a client just disconnected from the server
	virtual void UpdateGameMode( CBasePlayer *pPlayer ) {}  // the client needs to be informed of the current game mode

// Client damage rules
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer ) = 0;// this client just hit the ground after a fall. How much damage?
	virtual BOOL  FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker ) {return TRUE;};// can this player take damage from this attacker?
	virtual BOOL ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target ) { return TRUE; }

// Client spawn/respawn control
	virtual void PlayerSpawn( CBasePlayer *pPlayer ) = 0;// called by CBasePlayer::Spawn just before releasing player into the game
	virtual void PlayerThink( CBasePlayer *pPlayer ) = 0; // called by CBasePlayer::PreThink every frame, before physics are run and after keys are accepted
	virtual BOOL FPlayerCanRespawn( CBasePlayer *pPlayer ) = 0;// is this player allowed to respawn now?
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer ) = 0;// When in the future will this player be able to spawn?
	virtual edict_t *GetPlayerSpawnSpot( CBasePlayer *pPlayer );// Place this player on their spawnspot and face them the proper direction.

	virtual BOOL AllowAutoTargetCrosshair( void ) { return TRUE; };
	virtual BOOL ClientCommand( CBasePlayer *pPlayer, const char *pcmd ) { return FALSE; };  // handles the user commands;  returns TRUE if command handled properly
	virtual void ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer ) {}		// the player has changed userinfo;  can change it now

// Client kills/scoring
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled ) = 0;// how many points do I award whoever kills this player?
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor ) = 0;// Called each time a player dies
	virtual void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )=  0;// Call this from within a GameRules class to report an obituary.
// Weapon retrieval
	virtual BOOL CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );// The player is touching an CBasePlayerItem, do I give it to him?
	virtual void PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon ) = 0;// Called each time a player picks up a weapon from the ground

// Weapon spawn/respawn control
	virtual int WeaponShouldRespawn( CBasePlayerItem *pWeapon ) = 0;// should this weapon respawn?
	virtual float FlWeaponRespawnTime( CBasePlayerItem *pWeapon ) = 0;// when may this weapon respawn?
	virtual float FlWeaponTryRespawn( CBasePlayerItem *pWeapon ) = 0; // can i respawn now,  and if not, when should i try again?
	virtual Vector VecWeaponRespawnSpot( CBasePlayerItem *pWeapon ) = 0;// where in the world should this weapon respawn?

// Item retrieval
	virtual BOOL CanHaveItem( CBasePlayer *pPlayer, CItem *pItem ) = 0;// is this player allowed to take this item?
	virtual void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem ) = 0;// call each time a player picks up an item (battery, healthkit, longjump)

// Item spawn/respawn control
	virtual int ItemShouldRespawn( CItem *pItem ) = 0;// Should this item respawn?
	virtual float FlItemRespawnTime( CItem *pItem ) = 0;// when may this item respawn?
	virtual Vector VecItemRespawnSpot( CItem *pItem ) = 0;// where in the world should this item respawn?

// Ammo retrieval
	virtual BOOL CanHaveAmmo( CBasePlayer *pPlayer, const char *pszAmmoName, int iMaxCarry );// can this player take more of this ammo?
	virtual void PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount ) = 0;// called each time a player picks up some ammo in the world

// Ammo spawn/respawn control
	virtual int AmmoShouldRespawn( CBasePlayerAmmo *pAmmo ) = 0;// should this ammo item respawn?
	virtual float FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo ) = 0;// when should this ammo item respawn?
	virtual Vector VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo ) = 0;// where in the world should this ammo item respawn?
																			// by default, everything spawns

// Healthcharger respawn control
	virtual float FlHealthChargerRechargeTime( void ) = 0;// how long until a depleted HealthCharger recharges itself?
	virtual float FlHEVChargerRechargeTime( void ) { return 0; }// how long until a depleted HealthCharger recharges itself?

// What happens to a dead player's weapons
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer ) = 0;// what do I do with a player's weapons when he's killed?

// What happens to a dead player's ammo	
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer ) = 0;// Do I drop ammo when the player dies? How much?

// Teamplay stuff
	virtual const char *GetTeamID( CBaseEntity *pEntity ) = 0;// what team is this entity on?
	virtual int PlayerRelationship( CBasePlayer *pPlayer, CBaseEntity *pTarget ) = 0;// What is the player's relationship with this entity?
	virtual int GetTeamIndex( const char *pTeamName ) { return -1; }
	virtual const char *GetIndexedTeamName( int teamIndex ) { return ""; }
	virtual BOOL IsValidTeam( const char *pTeamName ) { return TRUE; }
	virtual void ChangePlayerTeam( CBasePlayer *pPlayer, const char *pTeamName, BOOL bKill, BOOL bGib ) {}
	virtual const char *SetDefaultPlayerTeam( CBasePlayer *pPlayer ) { return ""; }

// Sounds
	virtual BOOL PlayTextureSounds( void ) { return TRUE; }
	virtual BOOL PlayFootstepSounds( CBasePlayer *pl, float fvol ) { return TRUE; }

// Monsters
	virtual BOOL FAllowMonsters( void ) = 0;//are monsters allowed

	// Immediately end a multiplayer game
	virtual void EndMultiplayerGame( void ) {}
};

extern CGameRules *InstallGameRules( void );


//=========================================================
// CHalfLifeRules - rules for the single player Half-Life 
// game.
//=========================================================
class CHalfLifeRules : public CGameRules
{
public:
	CHalfLifeRules ( void );

// GR_Think
	virtual void Think( void );
	virtual BOOL IsAllowedToSpawn( CBaseEntity *pEntity );
	virtual BOOL FAllowFlashlight( void ) { return TRUE; };

	virtual BOOL FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual BOOL GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon );
	
// Functions to verify the single/multiplayer status of a game
	virtual BOOL IsMultiplayer( void );
	virtual BOOL IsDeathmatch( void );
	virtual BOOL IsCoOp( void );

// Client connection/disconnection
	virtual BOOL ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] );
	virtual void InitHUD( CBasePlayer *pl );		// the client dll is ready for updating
	virtual void ClientDisconnected( edict_t *pClient );

// Client damage rules
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer );
	
// Client spawn/respawn control
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerThink( CBasePlayer *pPlayer );
	virtual BOOL FPlayerCanRespawn( CBasePlayer *pPlayer );
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer );

	virtual BOOL AllowAutoTargetCrosshair( void );

// Client kills/scoring
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );

// Weapon retrieval
	virtual void PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );

// Weapon spawn/respawn control
	virtual int WeaponShouldRespawn( CBasePlayerItem *pWeapon );
	virtual float FlWeaponRespawnTime( CBasePlayerItem *pWeapon );
	virtual float FlWeaponTryRespawn( CBasePlayerItem *pWeapon );
	virtual Vector VecWeaponRespawnSpot( CBasePlayerItem *pWeapon );

// Item retrieval
	virtual BOOL CanHaveItem( CBasePlayer *pPlayer, CItem *pItem );
	virtual void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem );

// Item spawn/respawn control
	virtual int ItemShouldRespawn( CItem *pItem );
	virtual float FlItemRespawnTime( CItem *pItem );
	virtual Vector VecItemRespawnSpot( CItem *pItem );

// Ammo retrieval
	virtual void PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount );

// Ammo spawn/respawn control
	virtual int AmmoShouldRespawn( CBasePlayerAmmo *pAmmo );
	virtual float FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo );
	virtual Vector VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo );

// Healthcharger respawn control
	virtual float FlHealthChargerRechargeTime( void );

// What happens to a dead player's weapons
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer );

// What happens to a dead player's ammo	
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer );

// Monsters
	virtual BOOL FAllowMonsters( void );

// Teamplay stuff	
	virtual const char *GetTeamID( CBaseEntity *pEntity ) {return "";};
	virtual int PlayerRelationship( CBasePlayer *pPlayer, CBaseEntity *pTarget );
};

//=========================================================
// CHalfLifeMultiplay - rules for the basic half life multiplayer
// competition
//=========================================================
class CHalfLifeMultiplay : public CGameRules
{
public:
	CHalfLifeMultiplay();

// GR_Think
	virtual void Think( void );
	virtual void RefreshSkillData( void );
	virtual BOOL IsAllowedToSpawn( CBaseEntity *pEntity );
	virtual BOOL FAllowFlashlight( void );

	virtual BOOL FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual BOOL GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon );

// Functions to verify the single/multiplayer status of a game
	virtual BOOL IsMultiplayer( void );
	virtual BOOL IsDeathmatch( void );
	virtual BOOL IsCoOp( void );

// Client connection/disconnection
	// If ClientConnected returns FALSE, the connection is rejected and the user is provided the reason specified in
	//  svRejectReason
	// Only the client's name and remote address are provided to the dll for verification.
	virtual BOOL ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] );
	virtual void InitHUD( CBasePlayer *pl );		// the client dll is ready for updating
	virtual void ClientDisconnected( edict_t *pClient );
	virtual void UpdateGameMode( CBasePlayer *pPlayer );  // the client needs to be informed of the current game mode

// Client damage rules
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer );
	virtual BOOL  FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker );

// Client spawn/respawn control
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerThink( CBasePlayer *pPlayer );
	virtual BOOL FPlayerCanRespawn( CBasePlayer *pPlayer );
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer );
	virtual edict_t *GetPlayerSpawnSpot( CBasePlayer *pPlayer );

	virtual BOOL AllowAutoTargetCrosshair( void );

// Client kills/scoring
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );

// Weapon retrieval
	virtual void PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual BOOL CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );// The player is touching an CBasePlayerItem, do I give it to him?

// Weapon spawn/respawn control
	virtual int WeaponShouldRespawn( CBasePlayerItem *pWeapon );
	virtual float FlWeaponRespawnTime( CBasePlayerItem *pWeapon );
	virtual float FlWeaponTryRespawn( CBasePlayerItem *pWeapon );
	virtual Vector VecWeaponRespawnSpot( CBasePlayerItem *pWeapon );

// Item retrieval
	virtual BOOL CanHaveItem( CBasePlayer *pPlayer, CItem *pItem );
	virtual void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem );

// Item spawn/respawn control
	virtual int ItemShouldRespawn( CItem *pItem );
	virtual float FlItemRespawnTime( CItem *pItem );
	virtual Vector VecItemRespawnSpot( CItem *pItem );

// Ammo retrieval
	virtual void PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount );

// Ammo spawn/respawn control
	virtual int AmmoShouldRespawn( CBasePlayerAmmo *pAmmo );
	virtual float FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo );
	virtual Vector VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo );

// Healthcharger respawn control
	virtual float FlHealthChargerRechargeTime( void );
	virtual float FlHEVChargerRechargeTime( void );

// What happens to a dead player's weapons
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer );

// What happens to a dead player's ammo	
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer );

// Teamplay stuff	
	virtual const char *GetTeamID( CBaseEntity *pEntity ) {return "";}
	virtual int PlayerRelationship( CBasePlayer *pPlayer, CBaseEntity *pTarget );

	virtual BOOL PlayTextureSounds( void ) { return FALSE; }
	virtual BOOL PlayFootstepSounds( CBasePlayer *pl, float fvol );

// Monsters
	virtual BOOL FAllowMonsters( void );

	// Immediately end a multiplayer game
	virtual void EndMultiplayerGame( void ) { GoToIntermission(); }

	// Counter-Strike 1.0b: sets the player's model again (1.6 declares it at this place)
	virtual void ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer );

// Counter-Strike: the round. g_pGameRules is a CGameRules *: client.cpp and player.cpp
// reach these through a cast, ((CHalfLifeMultiplay *)g_pGameRules)->CheckWinConditions().
	virtual void CleanUpMap( void );
	virtual void RestartRound( void );
	virtual void CheckWinConditions( void );
	virtual void PickNewCommander( int iTeam );
	// Beta 4.0 (Linux symbols): weapon boxes cleared at a new round, the bomb defusers
	// and the C4, the round's starting weapons
	virtual void RemoveGuns( void );
	virtual void GiveC4( void );

	// Beta 5.0 (leak) (Linux symbols): a team is full when it has as many
	// players as the map has spawn points for it; the round's freeze period
	// and the seconds left in it or in the round
	BOOL TeamFull( int team_id );
	BOOL TeamStacked( int newTeam_id, int curTeam_id );	// Beta 5.0 (Linux symbol)
	BOOL IsFreezePeriod( void ) { return m_bFreezePeriod; }
	float TimeRemaining( void ) { return m_iRoundTimeSecs - gpGlobals->time + m_flRoundStartTime; }

	// member names ours; the offsets are retail's
	float m_flRestartRoundTime;		// 0x04 Think calls RestartRound at this time; 0 = no restart pending
	// (Beta 5.0's offsets; the leak's in brackets where a member moved)
	int m_iUnused08;				// 0x08 no function refers to it
	float m_flRoundStartTime;		// 0x0c RestartRound sets it, and Think when the freeze period ends; TimeRemaining counts from it; FPlayerCanRespawn: no spawning more than 20 seconds after it
	int m_iRoundTime;				// 0x10 [0x90] mp_roundtime, in minutes, 3 to 9
	int m_iRoundTimeSecs;			// 0x14 new in Beta 5.0 (leak): the seconds TimeRemaining counts down: the freeze time, then (Think) the round time
	int m_iFreezeTime;				// 0x18 new: mp_freezetime, in seconds, 0 to 15 (the constructor adds 2)
	float m_flRoundStartTime2;		// 0x1c [0x14] only written: the time of RestartRound, like m_flRoundStartTime
	int m_iAccountTerrorist;		// 0x20 [0x18] what every terrorist gets at the next RestartRound
	int m_iAccountCT;				// 0x24 [0x1c] the same for the counter-terrorists
	int m_iNumTerrorist;			// 0x28 [0x20] players per team, counted by CheckWinConditions
	int m_iNumCT;					// 0x2c [0x24]
	int m_iNumSpawnableTerrorist;	// 0x30 new: of them, those past the appearance menu (CheckWinConditions; the round counts only when both teams have one)
	int m_iNumSpawnableCT;			// 0x34 new
	int m_iSpawnPointCount_Terrorist;	// 0x38 new: info_player_deathmatch entities (Think counts them once a round; TeamFull)
	int m_iSpawnPointCount_CT;		// 0x3c new: info_player_start entities
	int m_iHostagesRescued;			// 0x40 [0x28] this round; each is worth 150 to the CT account
	int m_iUnused2C;				// 0x44 [0x2c] new in Beta 1.1: only RestartRound refers to it (zeroes it)
	int m_iRoundWinStatus;			// 0x48 [0x30] 0 round running, 1 CTs won, 2 terrorists won, 3 nobody
	short m_iNumCTWins;				// 0x4c [0x9c] rounds won (TeamScore)
	short m_iNumTerroristWins;		// 0x4e [0x9e]
	int m_iUnused88;				// 0x50 [0xa0] new in Beta 4.0: only RestartRound refers to it (sets 2)
	bool m_fTargetBombed;			// 0x54 [0xa4] new in Beta 4.0: the C4 went off near a bomb target this round (Explode2)
	bool m_fBombDefused;			// 0x55 [0xa5] new in the leak: the planted C4 was defused (C4Think)
	bool m_fMapHasBombTarget;		// 0x56 [0xa6] the map has a func_bomb_target or an info_bomb_target (RestartRound looks)
	bool m_fMapHasBombZone;			// 0x57 [0xa7] new in the leak: it has a func_bomb_target
	bool m_fMapHasBuyZone;			// 0x58 [0xa8] new in the leak: it has a func_buyzone
	bool m_fMapHasRescueZone;		// 0x59 [0xa9] new in the leak: it has a func_hostage_rescue
	// Beta 6.0 (names ours; Beta 6.0's offsets from here on, Beta 5.2's in brackets)
	bool m_bMapHasEscapeZone;		// 0x5a new: the map has a func_escapezone (RestartRound)
	int m_iMapHasVIPSafetyZone;		// 0x5c new: 0 not known yet (the constructor), 1 the map has a func_vip_safetyzone, 2 it has none (RestartRound)
	int m_iMapHasCameras;			// 0x60 [0x5c] new in the leak: 2 not known yet (the constructor), else whether a trigger_camera exists (ClientPutInServer: the joining player's view)
	int m_iC4Timer;					// 0x64 [0x60] mp_c4timer, in seconds, 10 to 90
	int m_iC4Turn;					// 0x68 [0x64] GiveC4's count through the terrorists
	int m_iLoserBonus;				// 0x6c [0x68] new in the leak: the losing team's round money: 1400, then 500 more after each loss in a row (Beta 6.5: from the second, while under 3000; RestartRound)
	int m_iNumConsecutiveCTLoses;	// 0x70 [0x6c] new in the leak: rounds lost in a row (RestartRound)
	int m_iNumConsecutiveTerroristLoses;	// 0x74 [0x70] new
	float m_flMaxIdlePeriod;		// 0x78 [0x74] new in the leak: mp_roundtime times 120 (the constructor and RestartRound, before the clamp); CBasePlayer::PreThink kicks a player idle longer when mp_autokick is set (name ours)
	int m_iLimitTeams;				// 0x7c [0x78] new in Beta 5.0: mp_limitteams, 0 to 20 (ReadMultiplayCvars); TeamStacked lets a team have that many players more than the other
	bool m_fCountedSpawnPoints;		// 0x80 [0x7c] new in the leak: Think has counted the spawn points this round (RestartRound and GoToIntermission clear it)
	bool m_fRoundTerminating;		// 0x81 [0x7d] new in the leak: the round is decided and the next one is due (RestartRound clears it)
	bool m_fCompleteReset;			// 0x82 [0x7e] new in the leak: the next RestartRound zeroes the team scores too (the first round, sv_restartround)
	float m_flRequiredEscapeRatio;	// 0x84 new: the share of the terrorists that must escape (0.5; CheckWinConditions)
	int m_iNumEscapers;				// 0x88 new: terrorists at the round's start on an escape map (RestartRound)
	int m_iHaveEscaped;				// 0x8c new: terrorists escaped this round (CheckWinConditions counts them)
	bool m_bCTCantBuy;				// 0x90 new: info_map_parameters' "buying" forbids the counter-terrorists to buy (RestartRound; CBasePlayer::CanPlayerBuy)
	bool m_bTCantBuy;				// 0x91 new: the same for the terrorists
	float m_flBombRadius;			// 0x94 new: info_map_parameters' "bombradius" (500 by default; RestartRound)
	int m_iConsecutiveVIP;			// 0x98 new: rounds the VIP has been the VIP (PickNextVIP picks a new one from 3)
	int m_iTotalGunCount;			// 0x9c new: the guns the map's armoury_entity items hold (CArmoury::Restart counts them once)
	int m_iTotalGrenadeCount;		// 0xa0 new: their flashbangs and HE grenades
	int m_iTotalArmourCount;		// 0xa4 new: their kevlar and assault suits
	int m_iUnBalancedRounds;		// 0xa8 new: rounds in a row with uneven teams (RestartRound; mp_autoteambalance)
	int m_iNumEscapeRounds;			// 0xac new: rounds played on an escape map; at 8 the teams swap (SwapAllPlayers; Beta 6.5: at 3)
	int m_iMapVotes[100];			// 0xb0 new in Beta 6.5: the votes each map of the cycle has, map 1 first (VoteMap counts them; Counter-Strike 1.6's name)
	int m_iLastPick;				// 0x240 new in Beta 6.5: the counter-terrorist PickNextVIP took last (1.6's name)
	int m_iMaxMapTime;				// 0x244 new in Beta 7.0: mp_timelimit as the constructor found it, put back at a complete restart (1.6's name). Beta 7.1: no function refers to it
	int m_iMaxRounds;				// 0x248 new in Beta 7.1: mp_maxrounds, read at a complete restart and by Think (1.6's name); Counter-Strike 1.0: the constructor reads it too
	int m_iTotalRoundsPlayed;		// 0x24c new in Beta 7.1: RestartRound counts the rounds; Think changes the map at m_iMaxRounds (1.6's name)
	// (Beta 7.1's offsets from here on, Beta 7.0's plus 8; Beta 6.1's in brackets)
	CBasePlayer *m_pCTCommander;	// 0x250 [0xb0] new in Beta 3.0: the counter-terrorists' commander (PickNewCommander)
	CBasePlayer *m_pTerroristCommander;	// 0x254 [0xb4] the terrorists'
	CBasePlayer *m_pVIP;			// 0x258 [0xb8] new in Beta 6.0: the VIP (PickNextVIP, MakeVIP)
	CBasePlayer *m_pVIPQueue[5];	// 0x25c [0xbc] new in Beta 6.0: counter-terrorists waiting to be the VIP (AddToVIPQueue)

	// Beta 6.0 (Linux symbols): team balance, the escape maps' team swap, the VIP
	void BalanceTeams( void );
	void SwapAllPlayers( void );
	void PickNextVIP( void );
	void ResetCurrentVIP( void );
	void StackVIPQueue( void );
	bool IsVIPQueueEmpty( void );
	bool AddToVIPQueue( CBasePlayer *toAdd );	// parameter name ours

	// Beta 6.5 (Linux symbols; the parameters' names are Counter-Strike 1.6's):
	// the team scores to every client, the map vote
	void UpdateTeamScores( void );
	void DisplayMaps( CBasePlayer *player, int iVote );
	void VoteMap( CBasePlayer *player, int iVote );

protected:
	virtual void ChangeLevel( void );	// (Beta 7.0 only: a char *szMap parameter; Linux symbols)
	virtual void GoToIntermission( void );
	float m_flIntermissionEndTime;	// 0x270 [0xd0]
	BOOL m_iEndIntermissionButtonHit;	// 0x274 [0xd4]
	void SendMOTDToClient( edict_t *client );

public:
	// Beta 5.0 (leak), after the SDK's members (names ours; Beta 7.1's offsets, Beta 6.1's in brackets)
	BOOL m_bFreezePeriod;			// 0x278 [0xd8] TRUE from RestartRound until Think sees the freeze time run out (IsFreezePeriod)
	float m_flNextRestartCheck;		// 0x27c [0xdc] when Think reads sv_restartround next (every half second; Beta 6.1: every second)
	bool m_fGameCommenced;			// 0x280 [0xe0] both teams have had spawnable players (CheckWinConditions; until then a round ends at once)
};

//=========================================================
// CHalfLifeTraining - Counter-Strike 1.0: the rules of the training course
// (training_gamerules.cpp; InstallGameRules makes them when the game is not
// deathmatch).  Member names and the declarations' order are Counter-Strike 1.6's.
//=========================================================
class CHalfLifeTraining : public CHalfLifeMultiplay
{
	float FillAccountTime, ServerRestartTime;	// 0x284 when PlayerThink fills the account up to 16000 (0 = not due); 0x288 no function refers to it
	BOOL fInBuyArea, fVisitedBuyArea;			// 0x28c the player stood in a buy zone last frame; 0x290 the buy zone's money was given once
	bool fVGUIMenus;							// 0x294 the player's m_bVGUIMenus, kept from his spawn

public:
	void HostageDied( void );
	bool PlayerCanBuy( CBasePlayer *pPlayer );

	CHalfLifeTraining();

	virtual BOOL IsMultiplayer( void );
	virtual BOOL IsDeathmatch( void );
	virtual void InitHUD( CBasePlayer *pl );
	virtual edict_t *GetPlayerSpawnSpot( CBasePlayer *pPlayer );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual int ItemShouldRespawn( CItem *pItem );
	virtual BOOL FPlayerCanRespawn( CBasePlayer *pPlayer );
	virtual void PlayerThink( CBasePlayer *pPlayer );
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual void CheckWinConditions( void );
};

extern DLL_GLOBAL CGameRules*	g_pGameRules;
