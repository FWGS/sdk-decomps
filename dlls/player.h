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
#ifndef PLAYER_H
#define PLAYER_H

#define PLAYER_FATAL_FALL_SPEED		1100// approx 60 feet (Counter-Strike Beta 2.0: 1100)
#define PLAYER_MAX_SAFE_FALL_SPEED	500// approx 20 feet (Counter-Strike: 450 in Beta 1.1, 500 in 2.0)
#define DAMAGE_FOR_FALL_SPEED		(float) 100 / ( PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED )// damage per unit per second.
#define PLAYER_MIN_BOUNCE_SPEED		200
#define PLAYER_FALL_PUNCH_THRESHHOLD (float)250 // won't punch player's screen/make scrape noise unless player falling at least this fast.

//
// Player PHYSICS FLAGS bits
//
#define		PFLAG_ONLADDER		( 1<<0 )
#define		PFLAG_ONSWING		( 1<<0 )
#define		PFLAG_ONTRAIN		( 1<<1 )
#define		PFLAG_ONBARNACLE	( 1<<2 )
#define		PFLAG_DUCKING		( 1<<3 )		// In the process of ducking, but totally squatted yet
#define		PFLAG_USING			( 1<<4 )		// Using a continuous entity
#define		PFLAG_OBSERVER		( 1<<5 )		// player is locked in stationary cam mode. Spectators can move, observers can't.

//
// generic player
//
//-----------------------------------------------------
//This is Half-Life player entity
//-----------------------------------------------------
#define CSUITPLAYLIST	4		// max of 4 suit sentences queued up at any time

#define SUIT_GROUP			TRUE
#define	SUIT_SENTENCE		FALSE

#define	SUIT_REPEAT_OK		0
#define SUIT_NEXT_IN_30SEC	30
#define SUIT_NEXT_IN_1MIN	60
#define SUIT_NEXT_IN_5MIN	300
#define SUIT_NEXT_IN_10MIN	600
#define SUIT_NEXT_IN_30MIN	1800
#define SUIT_NEXT_IN_1HOUR	3600

#define CSUITNOREPEAT		32

#define	SOUND_FLASHLIGHT_ON		"items/flashlight1.wav"
#define	SOUND_FLASHLIGHT_OFF	"items/flashlight1.wav"

#define TEAM_NAME_LENGTH	16

typedef enum
{
	PLAYER_IDLE,
	PLAYER_WALK,
	PLAYER_JUMP,
	PLAYER_SUPERJUMP,
	PLAYER_DIE,
	PLAYER_ATTACK1,
	PLAYER_FLINCH,		// Counter-Strike: 6, SetAnimation's flinch (name ours)
} PLAYER_ANIM;

// Counter-Strike: the teams (CBasePlayer::m_iTeam). Names ours. Beta 5.0 (leak):
// a player joins without a team (ClientPutInServer), the terrorists are 1 and the
// counter-terrorists 2 (Beta 1.0 to 4.1: 0 and 1); the client's scoreboard names
// them Observer, Terrorist, Counter-Terrorist.
#define TEAM_UNASSIGNED		0
#define TEAM_TERRORIST		1
#define TEAM_CT				2

// Counter-Strike: the menu a player has open (CBasePlayer::m_iMenu; client.cpp's
// menuselect). Menu_Buy to Menu_BuyItem are retail's names: the enum's text, two
// tabs deep, was left in titles.txt (Beta 1.0's to 4.1's has Menu_BuyWeapon,
// Beta 5.0's Menu_Buy). The other names are ours. Beta 5.0 (leak): the team and
// appearance menus first; no ammunition menu, three radio menus.
enum
{
	Menu_OFF,				// name ours: no menu
	Menu_ChooseTeam,		// name ours
	Menu_ChooseTeam2,		// name ours: menuselect treats it as the team menu; nothing sets it
	Menu_ChooseAppearance,	// name ours
	Menu_Buy,
	Menu_BuyPistol,
	Menu_BuyRifle,
	Menu_BuyMachineGun,
	Menu_BuyShotgun,
	Menu_BuySubMachineGun,
	Menu_BuyItem,
	Menu_Radio1,			// name ours (titles.txt: RadioA)
	Menu_Radio2,			// name ours (RadioB)
	Menu_Radio3				// name ours (RadioC)
};

// Beta 5.0 (leak): where a joining player is (CBasePlayer::m_iJoiningState;
// JoiningThink, ClientPutInServer, the team menus, CHalfLifeMultiplay::PlayerThink).
// Names ours.
enum
{
	JOIN_DONE,				// in the game
	JOIN_START,				// ClientPutInServer: JoiningThink shows the map's text or goes on
	JOIN_READING_TEXT,		// the map's text is up until a key is pressed
	JOIN_TEAM_MENU,			// PlayerThink shows the team menu
	JOIN_CHOOSING,			// the team and appearance menus are up
	JOIN_SPAWN				// an appearance was chosen: JoiningThink spawns him
};

#include "hintmessage.h"	// Beta 5.0 (leak)

class CObserver;	// Beta 5.0 (leak): observer.cpp

// Beta 5.0 (leak): the status bar names the player in the crosshair
// (InitStatusBar, UpdateStatusBar): SDK 2.3's text, with the leak's values:
// 1024 units (8192 for a spectator), and the values the type of what is seen,
// its name and its health (names ours: SDK 2.3 has name, health and armor)
#define MAX_ID_RANGE 1024
#define MAX_SPEC_ID_RANGE 8192	// name ours
#define SBAR_STRING_SIZE 128

enum sbar_data
{
	SBAR_ID_TARGETTYPE = 1,	// name ours: %c1
	SBAR_ID_TARGETNAME,		// %p2
	SBAR_ID_TARGETHEALTH,	// %i3
	SBAR_END,
};

// what SBAR_ID_TARGETTYPE says (names ours)
#define SBAR_TARGETTYPE_TEAMMATE	1
#define SBAR_TARGETTYPE_ENEMY		2
#define SBAR_TARGETTYPE_HOSTAGE		3

// Beta 5.0 (leak): what the zones a player stands in signal (buy zone, bomb
// target, rescue zone: the zone entities' touch functions and PostThink's
// checks set bits; once in a while PostThink takes them as the new state).
// The class, its members and its inline functions' names are ours; retail
// has the three bodies (Signal, Update in player.obj, GetState first in
// c4.obj). Update's body is in player.cpp: VC6 emits it after the zone
// icons' helpers defined before it (the order of definition).
class CSignals
{
public:
	CSignals( void ) { m_iSignals = 0; m_iState = 0; }	// retail's body: 0x1000e5a0, in client.obj after CBasePlayer's constructor
	void Signal( int flags ) { m_iSignals |= flags; }
	int Update( void );
	int GetState( void ) { return m_iState; }

	int m_iSignals;		// 0x00 the bits signalled since the last Update
	int m_iState;		// 0x04 the bits in force
};

class CBasePlayer : public CBaseMonster
{
public:
	// Counter-Strike's members: 476 bytes in front of the SDK's (Beta 1.0: 96, 1.1: 88,
	// 2.0: 116, 3.0: 376, 3.1: 380, 4.0: 412). Beta 2.0 makes the two flags at 0x148 bools and adds the members from
	// 0x168 on; Beta 3.0 makes the kevlar flag an int and adds 256 bytes before
	// m_iMenu; Beta 4.0 adds m_iHEGrenades before m_iFlashBangs and
	// 28 bytes after m_fIgnoreRadio. Beta 5.0 (leak) drops the name check, the
	// kick vote, the grenade counters, the night vision and the vote timer,
	// adds the joining state, the intro camera, the idle time, the briefing
	// flag, the hint messages, the display history (retail's save-table name),
	// the zone signals and the bomb target, and moves m_iMenu behind them.
	// Beta 5.0 drops m_fSendResetHUD (the leak's 0x194), so every member
	// after it is 4 bytes earlier up to m_pObserver, and the sixth item slot
	// takes them back (offsets of Beta 5.0, the leak's in brackets from
	// m_iNightVisionAlpha on; before it, Beta 4.1's). Every
	// name here is ours but m_flDisplayHistory; the offsets are retail's
	// (CBaseMonster ends at 0x118).
	float				m_flFlinchTime;			// 0x118 the flinch animation plays until then: SetAnimation ignores everything but PLAYER_FLINCH meanwhile
	float				m_flNextFlinchTime;		// 0x11c no new flinch animation before this time
	bool				m_fHeavyDamage;			// 0x120 (a bool from Beta 2.0 on) the last hit was a heavy one (TakeDamage); picks the long flinch and the thrown deaths
	float				m_flVelocityModifier;	// 0x124 scales pev->velocity on the ground; 0.2 when hit, back to 1 by 0.01 a frame
	float				m_flLastFired;			// 0x128 when the pistol was last fired (USP, G3SG1)
	float				m_flAccuracy;			// 0x12c meaning depends on the weapon: the G3SG1 takes it as accuracy (spread 1 - value, back towards 0.98 with time since the last shot); the M249, M4A1, MP5 Navy and TMP as a spread factor that grows with m_iShotsFired and Reload sets to 0
	float				m_flNextBurstShot;		// 0x130 when FireRemaining fires the burst's next shot; 0 = no burst running
	int					m_iBurstShotsFired;		// 0x134 shots of the burst so far
	int					m_iLastZoom;			// 0x138 the m_iFOV to come back to after the shot (AWP)
	bool				m_fResumeZoom;			// 0x13c (a bool from Beta 2.0 on) zoom back to m_iLastZoom when the weapon can fire again
	int					m_iShotsFired;			// 0x140 shots since the trigger was pulled; ItemPostFrame clears it
	float				m_flEjectBrass;			// 0x144 when ItemPostFrame calls EjectBrassLate; 0 = nothing pending
	int					m_iKevlar;				// 0x148 (a bool before Beta 3.0) 0 none, 1 kevlar (item_kevlar), 2 kevlar and helmet (item_assaultsuit); cleared when the armor is used up
	bool				m_fNotKilled;			// 0x14c set at the end of Spawn, cleared by Killed and ClientPutInServer: a player who lived through the round keeps armor, kevlar and ammunition
	int					m_iTeam;				// 0x150 TEAM_TERRORIST or TEAM_CT
	int					m_iAccount;				// 0x154 money
	int					m_iUnused158;			// 0x158 no function refers to it
	int					m_iUnused15C;			// 0x15c only written: SetAnimation zeroes it when the death animation starts
	int					m_iDeathThrow;			// 0x160 how Killed throws the body: 0 not, 1 forwards, 2 backwards, 3 away from the attacker
	float				m_flLastSay;			// 0x164 time of the last chat line (Host_Say lets one through a second)
	bool				m_fJustConnected;		// 0x168 [0x16c] set by ClientPutInServer; JoiningThink clears it when the player first spawns
	bool				m_fUnused169;			// 0x169 [new] only written: ClientPutInServer sets it
	int					m_iJoiningState;		// 0x16c [new] JoiningThink's step: 0 playing, 1 just connected (camera, briefing), 2 reading the briefing, 3 choosing a team, 4 choosing a model, 5 about to spawn (ClientPutInServer, PreThink, the menus, multiplay's Think and PlayerThink)
	CBaseEntity			*m_pIntroCamera;		// 0x170 [new] the trigger_camera a joining player looks through (ClientPutInServer, JoiningThink: the next one every 6 seconds)
	float				m_flIntroCameraTime;	// 0x174 [new] when JoiningThink moves to the next camera
	int					m_iIgnoreGhosts;		// 0x178 [new] set when the client's ghosts key (Beta 5.0; the leak: ignoreghosts) is 0 or off (JoiningThink); CObserver::CGhosts::Update shows no ghosts to him
	float				m_flLastActivity;		// 0x17c [new] when the player last pressed a button (PreThink); idle longer than the game rules allow, mp_autokick kicks him
	bool				m_fShowingLevelText;	// 0x180 [new] the map's briefing is on the screen (showbriefing, JoiningThink); a button press takes it away (RemoveLevelText)
	bool				m_fTeamChanged;			// 0x181 new in Beta 5.0: a dead player changed teams (HandleMenu_ChooseTeam); chooseteam allows one change a death (Killed and JoiningThink clear it; name ours)
	int					m_iAppearance;			// 0x184 [0x170] the player model (ClientPutInServer 1, HandleMenu_ChooseAppearance; ClientUserInfoChanged and SetPlayerModel map it to a model)
	bool				m_fKicked;				// 0x188 [0x178] sent away (team killer): ClientCommand ignores him, Spawn kills him
	int					m_iTeamKills;			// 0x18c [0x184] team mates killed; the third kicks the killer (PlayerKilled)
	int					m_iIgnoreMessages;		// 0x190 [0x188] ignoremsg: 0 all, 1 no broadcast messages, 2 no team messages either
	int					m_iNightVisionAlpha;	// 0x194 [0x198] only written in the leak: ClientPutInServer and JoiningThink set 60 (no night vision any more)
	Vector				m_vecOldOrigins[20];	// 0x198 [0x19c] the trail a hostage follows; PreThink pushes pev->origin in front every 0.2 seconds
	float				m_flNextOldOrigin;		// 0x288 [0x28c] when PreThink records the next position
	float				m_flNextRadio;			// 0x28c [0x290] no radio message before this time (1.5 seconds after the last)
	int					m_iRadioMessages;		// 0x290 [0x294] radio messages left (Spawn sets the number)
	bool				m_fIgnoreRadio;			// 0x294 [0x298] the ignoreradio command; CBasePlayer::Radio sends such a player nothing
	bool				m_fHasC4;				// 0x295 [0x299] carries the C4 bomb
	bool				m_fHasDefuser;			// 0x296 [0x29a] carries a defuse kit (bought: BuyItem); CGrenade::Use defuses in 5 seconds with it, 10 without
	bool				m_fKilledByBlast;		// 0x297 [0x29b] the last hit was a blast (DMG_BLAST); picks the death animation
	Vector				m_vecBlastDir;			// 0x298 [0x29c] from the inflictor to the player, at the last hit (TakeDamage)
	bool				m_fKilledByHEGrenade;	// 0x2a4 [0x2a8] the last hit was an HE grenade's (DMG_HEGRENADE); a team mate killed so is not counted
	CHintMessageQueue	m_hintMessageQueue;		// 0x2a8 [0x2ac] the hints waiting (HintMessage adds, PreThink shows, Spawn empties)
	int					m_flDisplayHistory;		// 0x2d8 [0x2dc] retail's save-table name (FIELD_INTEGER): a bit per hint the player has seen (PreThink, Spawn, HostageUsed, TakeDamage, multiplay's PlayerKilled, ...)
	int					m_iMenu;				// 0x2dc [0x2e0] the menu the client has open (client.cpp)
	// Beta 5.2: the dead player's camera, which replaces the observer entity
	// (16 bytes; every member after them is 0x10 later than in Beta 5.0)
	char				m_cObserverMode;		// 0x2e0 0 the free camera (GhostMode), 1 the chase camera following m_pChaseTarget (ChaseCamMode); Spawn: 1 when mp_chasecam is 1; JUMP switches
	int					m_iChaseNext;			// 0x2e4 the player index GetPlayerToChase tries first (Spawn: 1)
	CBaseEntity			*m_pChaseTarget;		// 0x2e8 the player watched (GetPlayerToChase), NULL when there is none
	float				m_flNextObserverInput;	// 0x2ec JUMP and ATTACK act again from this time (half a second; Spawn: 0)
	float				m_flNextZoneCheck;		// 0x2e0 [0x2e4] PostThink checks the zones the player stands in every 0.2 seconds (Spawn: 0)
	CSignals			m_signals;				// 0x2e4 [0x2e8] the zones' signals: buy zone 1, bomb target 2 (the buy commands, CC4::PrimaryAttack, CBuyZone, CBombTarget)
	edict_t				*m_pentBombTarget;		// 0x2ec [0x2f0] the bomb target the player is in (CBombTarget::BombTargetTouch); the planted C4 keeps it (ShootSatchelCharge)

	int					m_iPlayerSound;// the index of the sound list slot reserved for this player
	int					m_iTargetVolume;// ideal sound volume. 
	int					m_iWeaponVolume;// how loud the player's weapon is right now.
	int					m_iExtraSoundTypes;// additional classification for this weapon's sound
	int					m_iWeaponFlash;// brightness of the weapon flash
	float				m_flStopExtraSoundTime;
	
	float				m_flFlashLightTime;	// Time until next battery draw/Recharge
	int					m_iFlashBattery;		// Flashlight Battery Draw

	int					m_afButtonLast;
	int					m_afButtonPressed;
	int					m_afButtonReleased;
	
	edict_t			   *m_pentSndLast;			// last sound entity to modify player room type
	float				m_flSndRoomtype;		// last roomtype set by sound entity
	float				m_flSndRange;			// dist from player to sound entity

	float				m_flFallVelocity;
	
	int					m_rgItems[MAX_ITEMS];
	int					m_fKnownItem;		// True when a new item needs to be added
	int					m_fNewAmmo;			// True when a new item has been added

	unsigned int		m_afPhysicsFlags;	// physics flags - set when 'normal' physics should be revisited or overriden
	float				m_fNextSuicideTime; // the time after which the player can next use the suicide command


// these are time-sensitive things that we keep track of
	float				m_flTimeStepSound;	// when the last stepping sound was made
	float				m_flTimeWeaponIdle; // when to play another weapon idle animation.
	float				m_flSwimTime;		// how long player has been underwater
	float				m_flDuckTime;		// how long we've been ducking
	float				m_flWallJumpTime;	// how long until next walljump

	float				m_flSuitUpdate;					// when to play next suit update
	int					m_rgSuitPlayList[CSUITPLAYLIST];// next sentencenum to play for suit update
	int					m_iSuitPlayNext;				// next sentence slot for queue storage;
	int					m_rgiSuitNoRepeat[CSUITNOREPEAT];		// suit sentence no repeat list
	float				m_rgflSuitNoRepeatTime[CSUITNOREPEAT];	// how long to wait before allowing repeat
	int					m_lastDamageAmount;		// Last damage taken
	float				m_tbdPrev;				// Time-based damage timer

	float				m_flgeigerRange;		// range to nearest radiation source
	float				m_flgeigerDelay;		// delay per update of range msg to client
	int					m_igeigerRangePrev;
	int					m_iStepLeft;			// alternate left/right foot stepping sound
	char				m_szTextureName[CBTEXTURENAMEMAX];	// current texture name we're standing on
	char				m_chTextureType;		// current texture type

	int					m_idrowndmg;			// track drowning damage taken
	int					m_idrownrestored;		// track drowning damage restored

	int					m_bitsHUDDamage;		// Damage bits for the current fame. These get sent to 
												// the hude via the DAMAGE message
	BOOL				m_fInitHUD;				// True when deferred HUD restart msg needs to be sent
	BOOL				m_fGameHUDInitialized;
	int					m_iTrain;				// Train control position
	BOOL				m_fWeapon;				// Set this to FALSE to force a reset of the current weapon HUD info

	EHANDLE				m_pTank;				// the tank which the player is currently controlling,  NULL if no tank
	float				m_fDeadTime;			// the time at which the player died  (used in PlayerDeathThink())

	BOOL			m_fNoPlayerSound;	// a debugging feature. Player makes no sound if this is true. 
	BOOL			m_fLongJump; // does this player have the longjump module?

	float       m_tSneaking;
	int			m_iUpdateTime;		// stores the number of frame ticks before sending HUD update messages
	int			m_iClientHealth;	// the health currently known by the client.  If this changes, send a new
	int			m_iClientBattery;	// the Battery currently known by the client.  If this changes, send a new
	int			m_iHideHUD;		// the players hud weapon info is to be hidden
	int			m_iClientHideHUD;
	int			m_iFOV;			// field of view
	int			m_iClientFOV;	// client's known FOV
	// Beta 5.0 (leak), names ours (Beta 5.2's offsets, 0x10 more than Beta 5.0's):
	BOOL		m_fHasAssaultSuit;	// 0x500 GiveNamedItem: TRUE for item_assaultsuit, FALSE for item_kevlar; HasAssaultSuit wants it and armor
	int			m_iNumSpawns;		// 0x504 spawns since the player joined (Spawn counts, JoiningThink clears; FPlayerCanRespawn)
	CObserver	*m_pObserver;		// 0x508 Beta 5.0's observer entity; from Beta 5.2 on nothing sets it, ClientDisconnected still removes it (sic)
	// usable player items 
	CBasePlayerItem	*m_rgpPlayerItems[MAX_ITEM_TYPES];
	CBasePlayerItem *m_pActiveItem;
	CBasePlayerItem *m_pClientActiveItem;  // client version of the active item
	CBasePlayerItem *m_pLastItem;
	// shared ammo slots
	int	m_rgAmmo[MAX_AMMO_SLOTS];
	int	m_rgAmmoLast[MAX_AMMO_SLOTS];

	Vector				m_vecAutoAim;
	BOOL				m_fOnTarget;
	int					m_iDeaths;
	float				m_iRespawnFrames;	// used in PlayerDeathThink() to make sure players can always respawn

	// Beta 5.0 (leak): SDK 2.3's status bar, here (0x638 to 0x750)
	//Player ID
	void InitStatusBar( void );
	void UpdateStatusBar( void );
	int m_izSBarState[ SBAR_END ];
	float m_flNextSBarUpdateTime;
	float m_flStatusBarDisappearDelay;
	char m_SbarString0[ SBAR_STRING_SIZE ];
	char m_SbarString1[ SBAR_STRING_SIZE ];

	int m_lastx, m_lasty;  // These are the previous update's crosshair angles, DON"T SAVE/RESTORE

	int m_nCustomSprayFrames;// Custom clan logo frames for this player
	float	m_flNextDecalTime;// next time this player can spray a decal

	char m_szTeamName[TEAM_NAME_LENGTH];

	virtual void Spawn( void );
	void Pain( int iLastHitGroup, bool bHasArmour );	// parameter names ours

//	virtual void Think( void );
	virtual void Jump( void );
	virtual void Duck( void );
	virtual void PreThink( void );
	virtual void PostThink( void );
	virtual Vector GetGunPosition( void );
	virtual int TakeHealth( float flHealth, int bitsDamageType );
	virtual void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	virtual int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	virtual void	Killed( entvars_t *pevAttacker, int iGib );
	virtual Vector BodyTarget( const Vector &posSrc ) { return Center( ) + pev->view_ofs * RANDOM_FLOAT( 0.5, 1.1 ); };		// position to shoot at
	virtual void StartSneaking( void ) { m_tSneaking = gpGlobals->time - 1; }
	virtual void StopSneaking( void ) { m_tSneaking = gpGlobals->time + 30; }
	virtual BOOL IsSneaking( void ) { return m_tSneaking <= gpGlobals->time; }
	virtual BOOL IsAlive( void ) { return (pev->deadflag == DEAD_NO) && pev->health > 0; }
	virtual BOOL ShouldFadeOnDeath( void ) { return FALSE; }
	virtual	BOOL IsPlayer( void ) { return !( pev->flags & FL_SPECTATOR ); }	// Beta 5.0 (leak): a joining player (FL_SPECTATOR, ClientPutInServer) is not one

	virtual BOOL IsNetClient( void ) { return TRUE; }		// Bots should return FALSE for this, they can't receive NET messages
															// Spectators should return TRUE for this
	virtual const char *TeamID( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	void RenewItems(void);
	void PackDeadPlayerItems( void );
	void RemoveAllItems( BOOL removeSuit );
	BOOL SwitchWeapon( CBasePlayerItem *pWeapon );

	// JOHN:  sends custom messages if player HUD data has changed  (eg health, ammo)
	virtual void UpdateClientData( void );
	
	static	TYPEDESCRIPTION m_playerSaveData[];

	// Player is moved across the transition by other means
	virtual int		ObjectCaps( void ) { return CBaseMonster :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual void	Precache( void );
	BOOL			IsOnLadder( void );
	BOOL			FlashlightIsOn( void );
	void			FlashlightTurnOn( void );
	void			FlashlightTurnOff( void );
	
	void UpdatePlayerSound ( void );
	void DeathSound ( void );

	int Classify ( void );
	void SetAnimation( PLAYER_ANIM playerAnim );
	void SetWeaponAnimType( const char *szExtention );
	char m_szAnimExtention[32];

	// custom player functions
	virtual void ImpulseCommands( void );
	void CheatImpulseCommands( int iImpulse );

	void StartDeathCam( void );
	void StartObserver( Vector vecPosition, Vector vecViewAngle );
	// Beta 5.2 (Linux symbols): the chase camera
	CBaseEntity *GetPlayerToChase( void );
	void GhostMode( void );
	void ChaseCamMode( void );

	void AddPoints( int score, BOOL bAllowNegativeScore );
	void AddPointsToTeam( int score, BOOL bAllowNegativeScore );
	BOOL AddPlayerItem( CBasePlayerItem *pItem );
	BOOL RemovePlayerItem( CBasePlayerItem *pItem );
	void ThrowWeapon( char *pszItemName );
	void ThrowPrimary( void );

	// Beta 5.0 (leak), Linux symbols; parameter counts from retail's stack cleanup
	// (names ours)
	void AddAccount( int amount, bool bTrackChange );
	void Radio( const char *msg_id, const char *msg_verbose );
	void DropPlayerItem( const char *pszItemName );
	void SetPlayerModel( BOOL HasC4 );
	void SetBombIcon( BOOL bFlash );
	BOOL IsCommander( void );
	void SyncRoundTimer( void );
	void MenuPrint( CBasePlayer *pPlayer, const char *msg );
	void RoundRespawn( void );
	void HostageUsed( void );
	bool HintMessage( const char *pMessage, BOOL bDisplayIfPlayerDead, BOOL bForce );	// Beta 5.0: a third flag, forcing the hint past the client's autohelp key (Linux symbol: three ints; name ours)
	BOOL HasAssaultSuit( void );
	void SetScoreboardAttributes( CBasePlayer *destination = NULL );	// NULL: every player sees this one's marks
	void ResetMenu( void );
	void RemoveLevelText( void );
	void JoiningThink( void );
	void HandleSignals( void );
	virtual void ResetMaxSpeed( void );
	BOOL HasPlayerItem( CBasePlayerItem *pCheckItem );
	BOOL HasNamedPlayerItem( const char *pszItemName );
	BOOL HasWeapons( void );// do I have ANY weapons?
	void SelectPrevItem( int iItem );
	void SelectNextItem( int iItem );
	void SelectLastItem(void);
	void SelectItem(const char *pstr);
	void ItemPreFrame( void );
	void ItemPostFrame( void );
	void GiveNamedItem( const char *szName );
	void EnableControl(BOOL fControl);

	int  GiveAmmo( int iAmount, char *szName, int iMax );
	void SendAmmoUpdate(void);

	void WaterMove( void );
	void CheckWaterJump( void );
	void EXPORT PlayerDeathThink( void );
	void PlayerUse( void );

	void CheckSuitUpdate();
	void SetSuitUpdate(char *name, int fgroup, int iNoRepeat);
	void UpdateGeigerCounter( void );
	void CheckTimeBasedDamage( void );
	void UpdateStepSound( void );
	void PlayStepSound(int step, float fvol);

	BOOL FBecomeProne ( void );
	void BarnacleVictimBitten ( entvars_t *pevBarnacle );
	void BarnacleVictimReleased ( void );
	static int GetAmmoIndex(const char *psz);
	int AmmoInventory( int iAmmoIndex );
	int Illumination( void );

	void ResetAutoaim( void );
	void Reset( void );				// Beta 5.0 (Linux symbol): a complete reset (RestartRound)
	void GiveDefaultItems( void );	// Beta 5.0 (Linux symbol): the starting weapons
	Vector GetAutoaimVector( float flDelta  );
	Vector AutoaimDeflection( Vector &vecSrc, float flDist, float flDelta  );

	void ForceClientDllUpdate( void );  // Forces all client .dll specific data to be resent to client.

	void DeathMessage( entvars_t *pevKiller );

	void SetCustomDecalFrames( int nFrames );
	int GetCustomDecalFrames( void );
};

#define AUTOAIM_2DEGREES  0.0348994967025
#define AUTOAIM_5DEGREES  0.08715574274766
#define AUTOAIM_8DEGREES  0.1391731009601
#define AUTOAIM_10DEGREES 0.1736481776669


extern int	gmsgHudText;
extern BOOL gInitHUD;

#endif // PLAYER_H
