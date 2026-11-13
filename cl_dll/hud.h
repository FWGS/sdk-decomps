/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
//  hud.h
//
// class CHud declaration
//
// CHud handles the message, calculation, and drawing the HUD
//


#define RGB_YELLOWISH 0x00FFA000 //255,160,0
#define RGB_REDISH 0x00FF1010 //255,160,0
#define RGB_GREENISH 0x0000A000 //0,160,0

#include "wrect.h"
#include "cl_dll.h"
#include "ammo.h"

#define DHN_DRAWZERO 1
#define DHN_2DIGITS  2
#define DHN_3DIGITS  4
#define DHN_PADZERO  8	// name ours
#define MIN_ALPHA	 100	

#define		HUDELEM_ACTIVE	1

typedef struct {
	int x, y;
} POSITION;

enum 
{ 
	MAX_PLAYERS = 64,
	MAX_TEAMS = 64,
	MAX_TEAM_NAME = 16,
	MAX_HOSTAGES = 24,	// name ours
};

typedef struct {
	unsigned char r,g,b,a;
} RGBA;

typedef struct cvar_s cvar_t;


#define HUD_ACTIVE	1
#define HUD_INTERMISSION 2

#define MAX_PLAYER_NAME_LENGTH		32

#define	MAX_MOTD_LENGTH				1536

//
//-----------------------------------------------------
//
class CHudBase
{
public:
	POSITION  m_pos;
	int   m_type;
	int	  m_iFlags; // active, moving, 
	virtual int Init( void ) {return 0;}
	virtual int VidInit( void ) {return 0;}
	virtual int Draw(float flTime) {return 0;}
	virtual void Think(void) {return;}
	virtual void Reset(void) {return;}
	virtual void InitHUDData( void ) {}		// called every time a server is connected to

};

struct HUDLIST {
	CHudBase	*p;
	HUDLIST		*pNext;
};



//
//-----------------------------------------------------
//
#include "..\game_shared\voice_status.h"
#include "hud_spectator.h"


//
//-----------------------------------------------------
//
class CHudAmmo: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	void Think(void);
	void Reset(void);
	int DrawWList(float flTime);
	int MsgFunc_CurWeapon(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_WeaponList(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_AmmoX(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_AmmoPickup( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_WeapPickup( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_ItemPickup( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_HideWeapon( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_Crosshair( const char *pszName, int iSize, void *pbuf );

	void SlotInput( int iSlot );
	void _cdecl UserCmd_Slot1( void );
	void _cdecl UserCmd_Slot2( void );
	void _cdecl UserCmd_Slot3( void );
	void _cdecl UserCmd_Slot4( void );
	void _cdecl UserCmd_Slot5( void );
	void _cdecl UserCmd_Slot6( void );
	void _cdecl UserCmd_Slot7( void );
	void _cdecl UserCmd_Slot8( void );
	void _cdecl UserCmd_Slot9( void );
	void _cdecl UserCmd_Slot10( void );
	void _cdecl UserCmd_Close( void );
	void _cdecl UserCmd_NextWeapon( void );
	void _cdecl UserCmd_PrevWeapon( void );
	void _cdecl UserCmd_Adjust_Crosshair( void );	// name ours

	int DrawCrosshair( float flTime, int weaponid );	// Counter-Strike 1.4

	friend class CHudSpectator;	// SetModes sets the observer's crosshair from the members below (retail 0x190f831)

private:
	float m_fFade;
	RGBA  m_rgba;
	WEAPON *m_pWeapon;
	int	m_HUD_bucket0;
	int m_HUD_selection;
	HSPRITE m_hObserverCrosshair;		// name ours
	wrect_t m_rcObserverCrosshair;		// name ours
	int m_bObserverCrosshair;			// name ours
	int m_iAmmoLastCheck;
	float m_flCrosshairDistance;
	int m_iAlpha;
	int m_R;							// the crosshair colour adjust_crosshair cycles
	int m_G;
	int m_B;
	int m_iCrossColor;					// name ours: 0 green, 1 red, 2 blue, 3 yellow, 4 cyan

};

//
//-----------------------------------------------------
//

class CHudAmmoSecondary: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	void Reset( void );
	int Draw(float flTime);

	int MsgFunc_SecAmmoVal( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_SecAmmoIcon( const char *pszName, int iSize, void *pbuf );

private:
	enum {
		MAX_SEC_AMMO_VALUES = 4
	};

	int m_HUD_ammoicon; // sprite indices
	int m_iAmmoAmounts[MAX_SEC_AMMO_VALUES];
	float m_fFade;
};


#include "health.h"


#define FADE_TIME 100


//
//-----------------------------------------------------
//
class CHudGeiger: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	int MsgFunc_Geiger(const char *pszName, int iSize, void *pbuf);
	
private:
	int m_iGeigerRange;

};

//
//-----------------------------------------------------
//
class CHudTrain: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	int MsgFunc_Train(const char *pszName, int iSize, void *pbuf);

private:
	HSPRITE m_hSprite;
	int m_iPos;

};

//
//-----------------------------------------------------
//
// REMOVED: Vgui has replaced this.
//
/*
class CHudMOTD : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );

	int MsgFunc_MOTD( const char *pszName, int iSize, void *pbuf );

protected:
	static int MOTD_DISPLAY_TIME;
	char m_szMOTD[ MAX_MOTD_LENGTH ];
	float m_flActiveRemaining;
	int m_iLines;
};
*/

//
//-----------------------------------------------------
//
class CHudStatusBar : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );
	void ParseStatusString( int line_num );

	int MsgFunc_StatusText( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_StatusValue( const char *pszName, int iSize, void *pbuf );

protected:
	enum { 
		MAX_STATUSTEXT_LENGTH = 128,
		MAX_STATUSBAR_VALUES = 8,
		MAX_STATUSBAR_LINES = 2,
	};

	char m_szStatusText[MAX_STATUSBAR_LINES][MAX_STATUSTEXT_LENGTH];  // a text string describing how the status bar is to be drawn
	char m_szStatusBar[MAX_STATUSBAR_LINES][MAX_STATUSTEXT_LENGTH];	// the constructed bar that is drawn
	int m_iStatusValues[MAX_STATUSBAR_VALUES];  // an array of values for use in the status bar

	int m_bReparseString; // set to TRUE whenever the m_szStatusBar needs to be recalculated

	// an array of colors...one color for each line
	float *m_pflNameColors[MAX_STATUSBAR_LINES];
};

//
//-----------------------------------------------------
//
// REMOVED: Vgui has replaced this.
//
/*
class CHudScoreboard: public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	int Draw( float flTime );
	int DrawPlayers( int xoffset, float listslot, int nameoffset = 0, int team = 0 ); // returns the ypos where it finishes drawing
	void DrawRadar( float flTime );	// name ours
	void UserCmd_ShowScores( void );
	void UserCmd_HideScores( void );
	void UserCmd_ShowRadar( void );
	void UserCmd_HideRadar( void );
	int MsgFunc_ScoreAttrib( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_ScoreInfo( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamInfo( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamScore( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_Radar( const char *pszName, int iSize, void *pbuf );
	void DeathMsg( int killer, int victim );

	int m_iNumTeams;

	int m_iLastKilledBy;
	int m_fLastKillTime;
	int m_iPlayerNum;
	int m_iShowscoresHeld;
	int m_HUD_smallskull;	// name ours
	int m_HUD_smallc4;		// name ours
	int m_HUD_smallvip;		// name ours
	float m_flNextCvarCheck;	// name ours
	float m_flHideFrags;		// name ours: cl_hidefrags
	wrect_t *m_prcRadar;		// name ours
	HSPRITE m_hRadar;			// name ours
	wrect_t *m_prcRadarBg;		// name ours
	HSPRITE m_hRadarBg;			// name ours

	void GetAllPlayersInfo( void );
private:
	struct cvar_s *cl_showpacketloss;

};
*/

struct extra_player_info_t 
{
	short frags;
	short deaths;
	short unused;			// name ours: no function of Beta 6.5 refers to it
	int has_c4;				// name ours
	int is_vip;				// name ours
	Vector radar_pos;		// name ours
	float flash_next;		// name ours
	int flash_on;			// name ours
	int flash_left;			// name ours
	short playerclass;
	short teamnumber;
	char teamname[MAX_TEAM_NAME];
	bool dead;				// name ours
	float health_time;		// name ours: the status bar shows the health until then
	int health;				// name ours
};

struct hostage_info_t
{
	bool dead;
	Vector origin;
	int health;
	float radarflash;
	int radarflashon;
	int radarflashes;
	char teamname[MAX_TEAM_NAME];
};

struct team_info_t 
{
	char name[MAX_TEAM_NAME];
	short unused;			// sic: two bytes nothing reads
	short frags;
	short deaths;
	short ping;
	short packetloss;
	short ownteam;
	short players;
	int already_drawn;
	int scores_overriden;
	int teamnumber;
};

extern hud_player_info_t	g_PlayerInfoList[MAX_PLAYERS+1];	   // player info from the engine
extern extra_player_info_t  g_PlayerExtraInfo[MAX_PLAYERS+1];   // additional player info sent directly to the client dll
extern hostage_info_t		g_HostageInfo[MAX_HOSTAGES+1];
extern team_info_t			g_TeamInfo[MAX_TEAMS+1];
extern int					g_IsSpectator[MAX_PLAYERS+1];


//
//-----------------------------------------------------
//
class CHudDeathNotice : public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_DeathMsg( const char *pszName, int iSize, void *pbuf );

private:
	int m_HUD_d_skull;  // sprite index of skull icon
};

//
//-----------------------------------------------------
//
class CHudMenu : public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	void Reset( void );
	int Draw( float flTime );
	int MsgFunc_ShowMenu( const char *pszName, int iSize, void *pbuf );

	void SelectMenuItem( int menu_item );

	int m_fMenuDisplayed;
	int m_bitsValidSlots;
	float m_flShutoffTime;
	int m_fWaitingForMore;
};

//
//-----------------------------------------------------
//
class CHudSayText : public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_SayText( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_SendAudio( const char *pszName, int iSize, void *pbuf );
	void SayTextPrint( const char *pszBuf, int iBufSize, int clientIndex = -1 );
	void SendAudioPlay( const char *pszBuf, int iBufSize );	// name ours
	void EnsureTextFitsInOneLineAndWrapIfHaveTo( int line );
friend class CHudSpectator;

private:

	struct cvar_s *	m_HUD_saytext;
	struct cvar_s *	m_HUD_saytext_time;
};

//
//-----------------------------------------------------
//
class CHudBattery: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	int MsgFunc_Battery(const char *pszName,  int iSize, void *pbuf );
	int MsgFunc_ArmorType(const char *pszName,  int iSize, void *pbuf );
	
private:
	int m_HUD_suit_empty;			// name ours
	int m_HUD_suit_full;			// name ours
	int m_HUD_suithelmet_empty;		// name ours
	int m_HUD_suithelmet_full;		// name ours
	int	  m_iBat;	
	float m_fFade;
	int	  m_enArmorType;			// name ours
};


//
//-----------------------------------------------------
//
class CHudFlashlight: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	void Reset( void );
	int MsgFunc_Flashlight(const char *pszName,  int iSize, void *pbuf );
	int MsgFunc_FlashBat(const char *pszName,  int iSize, void *pbuf );
	
private:
	HSPRITE m_hSprite1;
	HSPRITE m_hSprite2;
	HSPRITE m_hBeam;
	wrect_t *m_prc1;
	wrect_t *m_prc2;
	wrect_t *m_prcBeam;
	float m_flBat;	
	int	  m_iBat;	
	int	  m_fOn;
	float m_fFade;
	int	  m_iWidth;		// width of the battery innards
};

//
//-----------------------------------------------------
//
const int maxHUDMessages = 16;
struct message_parms_t
{
	client_textmessage_t	*pMessage;
	float	time;
	int x, y;
	int	totalWidth, totalHeight;
	int width;
	int lines;
	int lineLength;
	int length;
	int r, g, b;
	int text;
	int fadeBlend;
	float charTime;
	float fadeTime;
};

//
//-----------------------------------------------------
//

class CHudTextMessage: public CHudBase
{
public:
	int Init( void );
	static char *LocaliseTextString( const char *msg, char *dst_buffer, int buffer_size );
	static char *BufferedLocaliseTextString( const char *msg );
	char *LookupString( const char *msg_name, int *msg_dest = NULL );
	int MsgFunc_TextMsg(const char *pszName, int iSize, void *pbuf);
};

//
//-----------------------------------------------------
//

class CHudMessage: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	int MsgFunc_HudText(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_GameTitle(const char *pszName, int iSize, void *pbuf);

	float FadeBlend( float fadein, float fadeout, float hold, float localTime );
	int	XPosition( float x, int width, int lineWidth );
	int YPosition( float y, int height );

	void MessageAdd( const char *pName, float time, int hintMessage );
	void MessageAdd( const char *pName, float time );
	void MessageAdd(client_textmessage_t * newMessage );
	void MessageDrawScan( client_textmessage_t *pMessage, float time );
	void MessageScanStart( void );
	void MessageScanNextChar( void );
	void Reset( void );

private:
	client_textmessage_t		*m_pMessages[maxHUDMessages];
	float						m_startTime[maxHUDMessages];
	message_parms_t				m_parms;
	float						m_gameTitleTime;
	client_textmessage_t		*m_pGameTitle;

	int m_HUD_title_life;
	int m_HUD_title_half;
};

//
//-----------------------------------------------------
//
#define MAX_SPRITE_NAME_LENGTH	24

class CHudStatusIcons: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	void Reset( void );
	int Draw(float flTime);
	int MsgFunc_StatusIcon(const char *pszName, int iSize, void *pbuf);

	enum { 
		MAX_ICONSPRITENAME_LENGTH = MAX_SPRITE_NAME_LENGTH,
		MAX_ICONSPRITES = 4,
	};

	
	//had to make these public so CHud could access them (to enable concussion icon)
	//could use a friend declaration instead...
	void EnableIcon( char *pszIconName, unsigned char red, unsigned char green, unsigned char blue, bool bFlash );
	void DisableIcon( char *pszIconName );

private:

	typedef struct
	{
		char szSpriteName[MAX_ICONSPRITENAME_LENGTH];
		HSPRITE spr;
		wrect_t rc;
		unsigned char r, g, b;
		int bFlash;			// name ours
	} icon_sprite_t;

	int m_iCrossWidth;		// name ours
	int m_bFlashOn;			// name ours
	float m_flFlashTime;	// name ours
	icon_sprite_t m_IconList[MAX_ICONSPRITES];

};

//
//-----------------------------------------------------
//
class CHudMoney: public CHudBase	// name ours
{
	friend class TeamFortressViewport;	// UpdateSpectatorPanel prints the money (retail 0x195b9ca)
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );
	int MsgFunc_Money( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_BlinkAcct( const char *pszName, int iSize, void *pbuf );

private:
	int m_iMoneyCount;		// name ours (Counter-Strike 1.6: m_iAccount; 1.4 has it first, as 1.6)
	int m_HUD_dollar;		// name ours
	int m_HUD_minus;		// name ours
	int m_HUD_plus;			// name ours
	int m_iDelta;			// name ours
	float m_fFade;
	float m_fFadeDelta;		// name ours
	int m_bShowDelta;		// name ours
	int m_iBlinkAmt;		// name ours
	float m_fBlinkTime;		// name ours
};

//
//-----------------------------------------------------
//
class CHudRoundTimer: public CHudBase	// name ours
{
	friend class TeamFortressViewport;	// UpdateSpectatorPanel prints the time left (retail 0x195ba00; SDK 2.3's commented lines: gHUD.m_roundTimer.m_flTimeEnd)
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );
	int MsgFunc_RoundTime( const char *pszName, int iSize, void *pbuf );

private:
	float m_flEndTime;		// name ours (Counter-Strike 1.6: m_flTimeEnd; 1.4 has it first, as 1.6)
	int m_HUD_timer;		// name ours
	int m_iUnused;			// name ours
	float m_flRoundTime;	// name ours
	float m_flNextToggle;	// name ours
	int m_bPanicColor;		// name ours
};

//
//-----------------------------------------------------
//
class CHudProgressBar: public CHudBase	// name ours
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );
	int MsgFunc_BarTime( const char *pszName, int iSize, void *pbuf );

private:
	float m_flEndTime;		// name ours
	float m_flStartTime;	// name ours
	float m_flDuration;		// name ours
	int m_iWidth;			// name ours
	int m_iHeight;			// name ours
	int m_x0, m_y0, m_x1, m_y1;	// names ours
};

//
//-----------------------------------------------------
//
class CHudNVG: public CHudBase	// name ours: night vision, and the sniper scope
{
public:
	~CHudNVG();
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );
	int MsgFunc_NVGToggle( const char *pszName, int iSize, void *pbuf );
	void UserCmd_NVGAdjustUp( void );		// name ours
	void UserCmd_NVGAdjustDown( void );		// name ours

	int m_iOn;						// name ours: CHud::Redraw reads it from Beta 7.0

private:
	int m_iAlpha;					// name ours
	wrect_t *m_prcScope[3][8];		// name ours: the scope's pieces at 640, 800 and 1024 wide
	wrect_t *m_prcHorizontal;		// name ours
	wrect_t *m_prcVertical;			// name ours
	HSPRITE m_hScopeSmall[4];		// name ours: Counter-Strike 1.4, the scope's corners below 640 wide
	HSPRITE m_hScope[8];			// name ours
	HSPRITE m_hHorizontal;			// name ours
	HSPRITE m_hVertical;			// name ours
	HSPRITE m_hScope2[8];			// name ours
	int m_iUnknown2[2];				// sic: 8 bytes the image never touches
	HSPRITE m_hScope3[8];			// name ours
	int m_iUnknown3[2];				// sic: 8 bytes the image never touches
};


//
//-----------------------------------------------------
//



class CHud
{
private:
	HUDLIST						*m_pHudList;
	HSPRITE						m_hsprLogo;
	int							m_iLogo;
	client_sprite_t				*m_pSpriteList;
	int							m_iSpriteCount;
	int							m_iSpriteCountAllRes;
	float						m_flMouseSensitivity;
	int							m_iConcussionEffect; 

public:

	HSPRITE						m_hsprCursor;
	float m_flTime;	   // the current client time
	float m_fOldTime;  // the time at which the HUD was last redrawn
	double m_flTimeDelta; // the difference between flTime and fOldTime
	Vector	m_vecOrigin;
	Vector	m_vecAngles;
	int		m_iKeyBits;
	int		m_iHideHUDDisplay;
	int		m_iFOV;
	int		m_Teamplay;
	int		m_iRes;
	cvar_t  *m_pCvarStealMouse;
	cvar_t	*m_pCvarDraw;

	int m_iFontHeight;
	int DrawHudNumber(int x, int y, int iFlags, int iNumber, int r, int g, int b );
	int DrawHudNumber(int x, int y, int iNumber, int r, int g, int b );		// overload ours
	int DrawHudString(int x, int y, int iMaxX, char *szString, int r, int g, int b );
	int DrawHudStringReverse( int xpos, int ypos, int iMinX, char *szString, int r, int g, int b );
	int DrawHudNumberString( int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b );
	int GetNumWidth(int iNumber, int iFlags);

private:
	// the memory for these arrays are allocated in the first call to CHud::VidInit(), when the hud.txt and associated sprites are loaded.
	// freed in ~CHud()
	HSPRITE *m_rghSprites;	/*[HUD_SPRITE_COUNT]*/			// the sprites loaded from hud.txt
	wrect_t *m_rgrcRects;	/*[HUD_SPRITE_COUNT]*/
	char *m_rgszSpriteNames; /*[HUD_SPRITE_COUNT][MAX_SPRITE_NAME_LENGTH]*/

	struct cvar_s *default_fov;
public:
	HSPRITE GetSprite( int index ) 
	{
		return (index < 0) ? 0 : m_rghSprites[index];
	}

	wrect_t& GetSpriteRect( int index )
	{
		return m_rgrcRects[index];
	}

	int GetSpriteWidth( int index )		// name ours
	{
		return m_rgrcRects[index].right - m_rgrcRects[index].left;
	}

	int GetSpriteHeight( int index )	// name ours
	{
		return m_rgrcRects[index].bottom - m_rgrcRects[index].top;
	}

	
	int GetSpriteIndex( const char *SpriteName );	// gets a sprite index, for use in the m_rghSprites[] array

	CHudAmmo		m_Ammo;
	CHudHealth		m_Health;
	CHudSpectator		m_Spectator;
	CHudGeiger		m_Geiger;
	CHudBattery		m_Battery;
	CHudTrain		m_Train;
	CHudFlashlight	m_Flash;
	CHudMessage		m_Message;
	CHudStatusBar   m_StatusBar;
	CHudDeathNotice m_DeathNotice;
	CHudSayText		m_SayText;
	CHudMenu		m_Menu;
	CHudAmmoSecondary	m_AmmoSecondary;
	CHudTextMessage m_TextMessage;
	CHudStatusIcons m_StatusIcons;
	CHudMoney		m_Money;		// name ours
	CHudRoundTimer	m_RoundTimer;	// name ours
	CHudProgressBar	m_ProgressBar;	// name ours
	CHudNVG			m_NVG;			// name ours

	void Init( void );
	void VidInit( void );
	void Think(void);
	int Redraw( float flTime, int intermission );
	int UpdateClientData( client_data_t *cdata, float time );

	CHud();			// Counter-Strike 1.3: not in the class body (the constructor's code is hud.obj's, after Init)
	~CHud();			// destructor, frees allocated memory

	// user messages
	int _cdecl MsgFunc_Damage(const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_Logo(const char *pszName,  int iSize, void *pbuf);
	int _cdecl MsgFunc_ResetHUD(const char *pszName,  int iSize, void *pbuf);
	void _cdecl MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf );
	void _cdecl MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf );
	void _cdecl MsgFunc_CheckModels( const char *pszName, int iSize, void *pbuf );	// name ours
	void _cdecl MsgFunc_BombDrop( const char *pszName, int iSize, void *pbuf );
	void _cdecl MsgFunc_BombPickup( const char *pszName, int iSize, void *pbuf );
	void _cdecl MsgFunc_HostagePos( const char *pszName, int iSize, void *pbuf );
	void _cdecl MsgFunc_HostageK( const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_SetFOV(const char *pszName,  int iSize, void *pbuf);
	int  _cdecl MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf );
	int  _cdecl MsgFunc_ReloadSound( const char *pszName, int iSize, void *pbuf );
	// Screen information
	SCREENINFO	m_scrinfo;

	int	m_iWeaponBits;
	bool	m_fPlayerDead;	// a byte from Beta 7.0 (SDK 2.0: int)
	int m_iIntermission;

	// sprite indexes
	int m_HUD_number_0;

	float m_flCvarCheckTime;	// name ours
	float m_flWindowCheckTime;	// name ours: VidInit sets it to -1 with m_flCvarCheckTime; nothing reads it in Beta 7.1
	bool m_bGunSmoke;			// name ours: the gunsmoke command toggles it
	float m_flTexSort;			// name ours: gl_texsort as Init found it
	float m_flZMax;				// name ours: gl_zmax as Init found it
	bool m_bNoAlphaMin;			// name ours: gl_alphamin read 0 at Init


	void AddHudElem(CHudBase *p);

	float GetSensitivity();

};

class TeamFortressViewport;

extern CHud gHUD;

// name ours: the times at which shells (or smoke puffs) were made, so that no more than
// m_iMax of them are around at once (max_shells, max_smokepuffs); CHud::Think drops those
// older than m_flLife
class CTempList
{
public:
	class Node
	{
	public:
		Node( float flTime ) { m_flTime = flTime; }

		float m_flTime;
		Node *m_pNext;
		Node *m_pPrev;
	};

	CTempList()
	{
		m_iMax = 60;
		m_iCount = 0;
		m_pHead = NULL;
		m_pLast = NULL;
		m_pTail = NULL;
		m_flLife = 2.0;
	}

	~CTempList()
	{
		Expire( 99999 );
	}

	bool Add( float flTime )
	{
		if ( m_iCount < m_iMax )
		{
			if ( !m_pHead )
			{
				m_pHead = new Node( flTime );
				m_pHead->m_pNext = NULL;
				m_pHead->m_pPrev = NULL;
				m_pTail = m_pLast = m_pHead;
			}
			else
			{
				m_pTail->m_pNext = new Node( flTime );
				m_pTail->m_pNext->m_pPrev = m_pTail;
				m_pLast = m_pTail = m_pTail->m_pNext;
			}

			m_iCount++;
			return true;
		}

		return false;
	}

	void Expire( float flTime )
	{
		while ( m_pLast && m_flLife + m_pLast->m_flTime < flTime )
		{
			Node *pPrev = m_pLast->m_pPrev;
			delete m_pLast;
			m_pLast = pPrev;
			m_pTail = pPrev;
			m_iCount--;

			if ( !m_pLast )
			{
				m_pHead = NULL;
				m_pTail = NULL;
			}
		}
	}

	int m_iCount;
	int m_iMax;
	float m_flLife;
	Node *m_pHead;
	Node *m_pLast;
	Node *m_pTail;
};

extern CTempList g_ShellList;	// name ours
extern CTempList g_SmokeList;	// name ours
extern int g_iShotsFired;		// name ours (ammo.cpp): the fire events count the local player's shots

bool CheckModelBounds( void );	// name ours (model_check.cpp)
extern TeamFortressViewport *gViewPort;

extern int g_iPlayerClass;
extern int g_iTeamNumber;
extern int g_iUser1;
extern int g_iUser2;
extern int g_iUser3;

// sic: declared names that Counter-Strike's headers had and this text has not (the count modulo 64 orders two
// operands in the constructors of CMessageWindowPanel and CClassMenuPanel and in CRifleMenuPanel::Update).  Counter-
// Strike 1.1 took 13 to 17 or 20; 1.3 takes 45, 46 or 49 (40 to 51 built in full)
enum r11_hud_unknown_e
{
	R11_HUD_UNKNOWN0,
	R11_HUD_UNKNOWN1,
	R11_HUD_UNKNOWN2,
	R11_HUD_UNKNOWN3,
	R11_HUD_UNKNOWN4,
	R11_HUD_UNKNOWN5,
	R11_HUD_UNKNOWN6,
	R11_HUD_UNKNOWN7,
	R11_HUD_UNKNOWN8,
	R11_HUD_UNKNOWN9,
	R11_HUD_UNKNOWN10,
	R11_HUD_UNKNOWN11,
	R11_HUD_UNKNOWN12,
	R11_HUD_UNKNOWN13,
	R11_HUD_UNKNOWN14,
	R11_HUD_UNKNOWN15,
	R11_HUD_UNKNOWN16,
	R11_HUD_UNKNOWN17,
	R11_HUD_UNKNOWN18,
	R11_HUD_UNKNOWN19,
	R11_HUD_UNKNOWN20,
	R11_HUD_UNKNOWN21,
	R11_HUD_UNKNOWN22,
	R11_HUD_UNKNOWN23,
	R11_HUD_UNKNOWN24,
	R11_HUD_UNKNOWN25,
	R11_HUD_UNKNOWN26,
	R11_HUD_UNKNOWN27,
	R11_HUD_UNKNOWN28,
	R11_HUD_UNKNOWN29,
	R11_HUD_UNKNOWN30,
	R11_HUD_UNKNOWN31,
	R11_HUD_UNKNOWN32,
	R11_HUD_UNKNOWN33,
	R11_HUD_UNKNOWN34,
	R11_HUD_UNKNOWN35,
	R11_HUD_UNKNOWN36,
	R11_HUD_UNKNOWN37,
	R11_HUD_UNKNOWN38,
	R11_HUD_UNKNOWN39,
	R11_HUD_UNKNOWN40,
	R11_HUD_UNKNOWN41,
	R11_HUD_UNKNOWN42,
	R11_HUD_UNKNOWN43,
	R11_HUD_UNKNOWN44,
	R11_HUD_UNKNOWN45,
};
