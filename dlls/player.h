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

// Beta 6.5: the observer's camera mode, pev->iuser1 (Valve's names, pm_shared.h of later SDKs)
#define OBS_NONE			0
#define OBS_CHASE_LOCKED	1
#define OBS_CHASE_FREE		2
#define OBS_ROAMING			3

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
	PLAYER_ATTACK2,		// Counter-Strike (Beta 7.0): the Elites' second gun (the Counter-Strike names here are 1.6's too)
	PLAYER_FLINCH,		// Counter-Strike: SetAnimation's flinches, 7 and 8 from Beta 7.0
	PLAYER_LARGE_FLINCH,
	PLAYER_RELOAD,		// Counter-Strike (Beta 7.0): 9, every Reload
	PLAYER_HOLDBOMB,	// Counter-Strike 1.0: 10, the C4 while it is armed (Counter-Strike 1.6's name)
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

// Counter-Strike: the zone flags (buy zone, bomb target, rescue zone) that HandleSignals
// compares from one frame to the next (class and member names ours).
class CPlayerSignals
{
public:
	CPlayerSignals( void ) { m_iSignals = 0; m_iState = 0; }
	void Signal( int flags ) { m_iSignals |= flags; }
	int Update( void ) { int old = m_iState; m_iState = m_iSignals; m_iSignals = 0; return m_iState ^ old; }
	int GetState( void ) { return m_iState; }
	int		m_iSignals;		// the zones touched this frame
	int		m_iState;		// the zones of the frame before
};

class CBasePlayer : public CBaseMonster
{
public:
	int					random_seed;    // See that is shared between client & server for shared weapons code

	// Counter-Strike's members, 0x178 to 0x398.  Offsets are Beta 6.5's (retail mp.dll); names are
	// Beta 6.1's reconstruction's (ours) unless said; "1.6" = Counter-Strike 1.6's debug information.
	unsigned short		m_usPlayerBleed;		// 0x178 four bytes nothing reads or writes (1.6's name and type, by its place)
	EHANDLE				m_hObserverTarget;		// 0x17c the player an observer watches (observer.cpp; 1.6's name)
	float				m_flNextObserverInput;	// 0x184 Observer_HandleButtons takes no button before this time (1.6's name)
	float				m_flFlinchTime;			// 0x188 the flinch animation plays until then
	float				m_flNextFlinchTime;		// 0x18c no new flinch animation before this time
	bool				m_fHeavyDamage;			// 0x190 the last hit was a heavy one (TakeDamage)
	float				m_flVelocityModifier;	// 0x194 scales the velocity on the ground after a hit
	int					m_iLastZoom;			// 0x198 the field of view DefaultDeploy resets to 90 (Beta 1.0's name)
	bool				m_bResumeZoom;			// 0x19c (Beta 1.0's m_fResumeZoom, now a bool)
	float				m_flEjectBrass;			// 0x1a0 when ItemPostFrame throws the shell late (the AWP's bolt, the M3 0.45 s after its shot); Beta 1.0's name
	int					m_iKevlar;				// 0x1a4 0 none, 1 kevlar, 2 kevlar and helmet
	bool				m_fNotKilled;			// 0x1a8 lived through the round: keeps his weapons
	int					m_iTeam;				// 0x1ac the player's team: the grenade throws pass it (Beta 1.0's name)
	int					m_iAccount;				// 0x1b0 money
	bool				m_bHasPrimary;			// 0x1b4 he carries a primary weapon
	int					m_iUnused15C;			// 0x1b8 only written (Beta 6.1's name, after its offset there)
	int					m_iDeathThrow;			// 0x1bc how Killed throws the body
	float				m_flLastSay;			// 0x1c0 time of the last chat line
	bool				m_fJustConnected;		// 0x1c4
	bool				m_fUnused169;			// 0x1c5 only written (Beta 6.1's name)
	int					m_iJoiningState;		// 0x1c8 JoiningThink's step (the JOIN_ enum)
	CBaseEntity			*m_pIntroCamera;		// 0x1cc the trigger_camera a joining player looks through
	float				m_flIntroCameraTime;	// 0x1d0
	int					m_iIgnoreGhosts;		// 0x1d4
	float				m_flLastActivity;		// 0x1d8 when the player last pressed a button
	bool				m_fShowingLevelText;	// 0x1dc the map's briefing is on the screen
	bool				m_fTeamChanged;			// 0x1dd a dead player changed teams this round
	int					m_iAppearance;			// 0x1e0 the player model
	bool				m_fKicked;				// 0x1e4
	int					m_iTeamKills;			// 0x1e8
	int					m_iIgnoreMessages;		// 0x1ec ignoremsg
	bool				m_bHasNightVision;		// 0x1f0
	bool				m_bNightVisionOn;		// 0x1f1
	Vector				m_vecOldOrigins[20];	// 0x1f4 the trail a hostage follows
	float				m_flNextOldOrigin;		// 0x2e4
	float				m_flNextRadio;			// 0x2e8 no radio message before this time
	int					m_iRadioMessages;		// 0x2ec radio messages left
	bool				m_fIgnoreRadio;			// 0x2f0 the ignoreradio command
	bool				m_bHasC4;				// 0x2f1 the bomb carrier (CC4::PrimaryAttack clears it; 1.6's name)
	bool				m_fHasDefuser;			// 0x2f2 carries a defuse kit
	bool				m_fKilledByBlast;		// 0x2f3 the last hit was a blast
	Vector				m_vecBlastDir;			// 0x2f4 from the inflictor to the player, at the last hit
	bool				m_fKilledByHEGrenade;	// 0x300
	CHintMessageQueue	m_hintMessageQueue;		// 0x304 the hints waiting
	int					m_flDisplayHistory;		// 0x334 (the save table's name) a bit per hint seen
	int					m_iMenu;				// 0x338 the menu the client has open
	int					m_iChaseNext;			// 0x33c Beta 6.1's chase camera words, less its mode byte (the mode is pev->iuser1 now)
	CBaseEntity			*m_pChaseTarget;		// 0x340
	float				m_fCamSwitch;			// 0x344 Beta 6.1's m_flNextObserverInput (1.6's name; the new member at 0x184 has Beta 6.1's name now)
	bool				m_bLeadsHostage;		// 0x348 a hostage follows him; PreThink records his path
	Vector				m_vecLastPathPos;		// 0x34c
	bool				m_bEscaped;				// 0x358 a terrorist who reached an escape zone
	bool				m_bIsVIP;				// 0x359 the VIP of an assassination map (ResetMaxSpeed: 227)
	float				m_flNextRadarUpdate;	// 0x35c
	Vector				m_vecLastRadarOrigin;	// 0x360
	bool				m_bLeftHanded;			// 0x36c userinfo "lefthand": v_*.mdl rather than v_*_r.mdl
	int					m_iCurrentKickVote;		// 0x370 the user id he voted to kick
	float				m_flNextVoteTime;		// 0x374
	bool				m_bVoteKicksAtOnce;		// 0x378 Beta 6.1's two bools (the hidden a12 command; the TK penalty)
	bool				m_bJustKilledTeammate;	// 0x379 (1.6's name)
	int					m_iHostagesKilled;		// 0x37c new in Beta 6.5 (1.6's name, by its place)
	int					m_iMapVote;				// 0x380 new in Beta 6.5 (1.6's name, by its place)
	bool				m_bCanShoot;			// 0x384 cleared in the freeze time, set when the round starts (1.6's name)
	bool				m_bVGUIMenus;			// 0x385 (0x386 to Counter-Strike 1.0: 1.0b drops Beta 6.6's m_bVerifyModel, which stood here, with VerifyModel) new in Beta 7.0: userinfo "vgui_menus", read with the model check in PreThink (1.6's name; it stands after the gait members there)
	float				m_flLastFired;			// 0x388 new in Beta 7.0: the time of the last shot (SetAnimation stamps it and plays the run animation four seconds later; Spawn: -15). 1.6's name: its Spawn stores the same -15 in the member after m_bCanShoot
	float				m_flLastAttackedTeammate;	// 0x38c new in Beta 7.0: TakeDamage's "attacked a teammate" line, at most once in 0.6 s (1.6's name)
	float				m_flTimeCheckAllowed;	// 0x390 new in Beta 7.0: PreThink checks the client's keys and model no sooner, every 3 s (1.6 has int m_iTimeCheckAllowed at this place, the timeleft command's; a float here, the prefix ours)
	bool				m_bHeadshotKilled;		// 0x394 new in Beta 7.1: Killed and SetAnimation set it when the last hit was to the head, DeathNotice then names the weapon "headshot"; Spawn clears it (1.6's name; there it stands after m_flLastAttackedTeammate)
	float				m_flNextZoneCheck;		// 0x398 HandleSignals' next check (0x394 in Beta 7.0, 0x388 to Beta 6.6; 1.6: m_tmHandleSignals)
	CPlayerSignals		m_signals;				// 0x39c
	edict_t				*m_pentCurBombTarget;	// 0x3a4 the bomb target the player is in (Counter-Strike 1.6's name; CC4::Use clears it around the plant)

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
	// Counter-Strike (Beta 6.1's names)
	BOOL		m_fHasAssaultSuit;	// 0x598
	int			m_iNumSpawns;		// 0x59c spawns since the player joined
	CBaseEntity	*m_pObserver;		// 0x5a0 Beta 5.0's observer entity; nothing sets it
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

	//Player ID
	void InitStatusBar( void );
	void UpdateStatusBar( void );
	int m_izSBarState[ SBAR_END ];					// 0x6e0
	float m_flNextSBarUpdateTime;					// 0x6f0
	float m_flStatusBarDisappearDelay;				// 0x6f4
	char m_SbarString0[ SBAR_STRING_SIZE ];			// 0x6f8
	char m_SbarString1[ SBAR_STRING_SIZE ];			// 0x778

	int m_lastx, m_lasty;  // These are the previous update's crosshair angles, DON"T SAVE/RESTORE

	int m_nCustomSprayFrames;// Custom clan logo frames for this player
	float	m_flNextDecalTime;// next time this player can spray a decal

	char m_szTeamName[TEAM_NAME_LENGTH];


	virtual void Spawn( void );
	void Pain( int iLastHitGroup, bool bHasArmour );

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
	virtual	BOOL IsPlayer( void ) { return !(pev->flags & FL_SPECTATOR); }			// Spectators should return FALSE for this, they aren't "players" as far as game logic is concerned

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

	// Counter-Strike (names the Linux library's; parameter names and return types ours)
	virtual void ResetMaxSpeed( void );
	void Radio( const char *msg_id, const char *msg_verbose );
	void SetBombIcon( int bFlash );
	void AddAccount( int amount, bool bTrackChange );
	bool CanPlayerBuy( bool display );
	void WindowCheck( void );				// Beta 7.1 (Linux symbol): the client is told which windows to close (message WClose)
	void Disappear( void );
	void GiveDefaultItems( void );
	void HandleSignals( void );
	BOOL HasAssaultSuit( void );
	bool HintMessage( const char *pMessage, BOOL bDisplayIfPlayerDead, BOOL bOverride );
	void HostageUsed( void );
	BOOL IsCommander( void );
	bool IsObserver( void );
	void JoiningThink( void );
	void MakeVIP( void );
	void MenuPrint( CBasePlayer *pPlayer, const char *msg );
	void Observer_FindNextPlayer( bool bReverse );
	void Observer_HandleButtons( void );
	void Observer_SetMode( int iMode );
	void RemoveLevelText( void );
	void Reset( void );
	void ResetMenu( void );
	void RoundRespawn( void );
	void SetPlayerModel( BOOL HasC4 );
	void SetScoreboardAttributes( CBasePlayer *destination = NULL );
	void SwitchTeam( void );
	void SyncRoundTimer( void );
	void TabulateAmmo( void );
	void ThrowPrimary( void );
	void ThrowWeapon( char *pszItemName );
	void UpdatePath( void );
	void CheatImpulseCommands( int iImpulse );

	void StartDeathCam( void );
	void StartObserver( Vector vecPosition, Vector vecViewAngle );

	void AddPoints( int score, BOOL bAllowNegativeScore );
	void AddPointsToTeam( int score, BOOL bAllowNegativeScore );
	BOOL AddPlayerItem( CBasePlayerItem *pItem );
	BOOL RemovePlayerItem( CBasePlayerItem *pItem );
	void DropPlayerItem ( const char *pszItemName );
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
