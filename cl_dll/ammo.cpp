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
// Ammo.cpp
//
// implementation of CHudAmmo class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "pm_shared.h"

#include <string.h>
#include <stdio.h>

#include "ammohistory.h"
#include "vgui_TeamFortressViewport.h"

WEAPON *gpActiveSel;	// NULL means off, 1 means just the menu bar, otherwise
						// this points to the active weapon menu item
WEAPON *gpLastSel;		// Last weapon menu selection 

client_sprite_t *GetSpriteList(client_sprite_t *pList, const char *psz, int iRes, int iCount);

WeaponsResource gWR;

int g_weaponselect = 0;
int g_iShotsFired;	// Counter-Strike: the fire events count the local player's shots; the crosshair spreads by it (1.6's name and place)

// Counter-Strike 1.4 (names and place Counter-Strike 1.6's): set by HUD_WeaponsPostThink
int g_iPlayerFlags;
int g_iWeaponFlags;
float g_flPlayerSpeed;

void WeaponsResource :: LoadAllWeaponSprites( void )
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if ( rgWeapons[i].iId )
			LoadWeaponSprites( &rgWeapons[i] );
	}
}

int WeaponsResource :: CountAmmo( int iId ) 
{ 
	if ( iId < 0 )
		return 0;

	return riAmmo[iId];
}

int WeaponsResource :: HasAmmo( WEAPON *p )
{
	if ( !p )
		return FALSE;

	// weapons with no max ammo can always be selected
	if ( p->iMax1 == -1 )
		return TRUE;

	return (p->iAmmoType == -1) || p->iClip > 0 || CountAmmo(p->iAmmoType) 
		|| CountAmmo(p->iAmmo2Type) || ( p->iFlags & WEAPON_FLAGS_SELECTONEMPTY );
}


void WeaponsResource :: LoadWeaponSprites( WEAPON *pWeapon )
{
	int i, iRes;

	if (ScreenWidth < 640)
		iRes = 320;
	else
		iRes = 640;

	char sz[128];

	if ( !pWeapon )
		return;

	memset( &pWeapon->rcActive, 0, sizeof(wrect_t) );
	memset( &pWeapon->rcInactive, 0, sizeof(wrect_t) );
	memset( &pWeapon->rcAmmo, 0, sizeof(wrect_t) );
	memset( &pWeapon->rcAmmo2, 0, sizeof(wrect_t) );
	pWeapon->hInactive = 0;
	pWeapon->hActive = 0;
	pWeapon->hAmmo = 0;
	pWeapon->hAmmo2 = 0;

	sprintf(sz, "sprites/%s.txt", pWeapon->szName);
	client_sprite_t *pList = SPR_GetList(sz, &i);

	if (!pList)
		return;

	client_sprite_t *p;
	
	p = GetSpriteList( pList, "crosshair", iRes, i );
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hCrosshair = SPR_Load(sz);
		pWeapon->rcCrosshair = p->rc;
	}
	else
		pWeapon->hCrosshair = NULL;

	p = GetSpriteList(pList, "autoaim", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hAutoaim = SPR_Load(sz);
		pWeapon->rcAutoaim = p->rc;
	}
	else
		pWeapon->hAutoaim = 0;

	p = GetSpriteList( pList, "zoom", iRes, i );
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hZoomedCrosshair = SPR_Load(sz);
		pWeapon->rcZoomedCrosshair = p->rc;
	}
	else
	{
		pWeapon->hZoomedCrosshair = pWeapon->hCrosshair; //default to non-zoomed crosshair
		pWeapon->rcZoomedCrosshair = pWeapon->rcCrosshair;
	}

	p = GetSpriteList(pList, "zoom_autoaim", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hZoomedAutoaim = SPR_Load(sz);
		pWeapon->rcZoomedAutoaim = p->rc;
	}
	else
	{
		pWeapon->hZoomedAutoaim = pWeapon->hZoomedCrosshair;  //default to zoomed crosshair
		pWeapon->rcZoomedAutoaim = pWeapon->rcZoomedCrosshair;
	}

	p = GetSpriteList(pList, "weapon", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hInactive = SPR_Load(sz);
		pWeapon->rcInactive = p->rc;

		gHR.iHistoryGap = max( gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top );
	}
	else
		pWeapon->hInactive = 0;

	p = GetSpriteList(pList, "weapon_s", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hActive = SPR_Load(sz);
		pWeapon->rcActive = p->rc;
	}
	else
		pWeapon->hActive = 0;

	p = GetSpriteList(pList, "ammo", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hAmmo = SPR_Load(sz);
		pWeapon->rcAmmo = p->rc;

		gHR.iHistoryGap = max( gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top );
	}
	else
		pWeapon->hAmmo = 0;

	p = GetSpriteList(pList, "ammo2", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hAmmo2 = SPR_Load(sz);
		pWeapon->rcAmmo2 = p->rc;

		gHR.iHistoryGap = max( gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top );
	}
	else
		pWeapon->hAmmo2 = 0;

}

// Returns the first weapon for a given slot.
WEAPON *WeaponsResource :: GetFirstPos( int iSlot )
{
	WEAPON *pret = NULL;

	for (int i = 0; i < MAX_WEAPON_POSITIONS; i++)
	{
		if ( rgSlots[iSlot][i] )
		{
			pret = rgSlots[iSlot][i];
			break;
		}
	}

	return pret;
}


WEAPON* WeaponsResource :: GetNextActivePos( int iSlot, int iSlotPos )
{
	if ( iSlotPos >= MAX_WEAPON_POSITIONS || iSlot >= MAX_WEAPON_SLOTS )
		return NULL;

	WEAPON *p = gWR.rgSlots[ iSlot ][ iSlotPos+1 ];
	
	if ( !p )
		return GetNextActivePos( iSlot, iSlotPos + 1 );

	return p;
}


int giBucketHeight, giBucketWidth, giABHeight, giABWidth; // Ammo Bar width and height

HSPRITE ghsprBuckets;					// Sprite for top row of weapons menu

DECLARE_MESSAGE(m_Ammo, CurWeapon );	// Current weapon and clip
DECLARE_MESSAGE(m_Ammo, WeaponList);	// new weapon type
DECLARE_MESSAGE(m_Ammo, AmmoX);			// update known ammo type's count
DECLARE_MESSAGE(m_Ammo, AmmoPickup);	// flashes an ammo pickup record
DECLARE_MESSAGE(m_Ammo, WeapPickup);    // flashes a weapon pickup record
DECLARE_MESSAGE(m_Ammo, HideWeapon);	// hides the weapon, ammo, and crosshair displays temporarily
DECLARE_MESSAGE(m_Ammo, ItemPickup);
DECLARE_MESSAGE(m_Ammo, Crosshair);	// shows or hides the observer's crosshair

DECLARE_COMMAND(m_Ammo, Slot1);
DECLARE_COMMAND(m_Ammo, Slot2);
DECLARE_COMMAND(m_Ammo, Slot3);
DECLARE_COMMAND(m_Ammo, Slot4);
DECLARE_COMMAND(m_Ammo, Slot5);
DECLARE_COMMAND(m_Ammo, Slot6);
DECLARE_COMMAND(m_Ammo, Slot7);
DECLARE_COMMAND(m_Ammo, Slot8);
DECLARE_COMMAND(m_Ammo, Slot9);
DECLARE_COMMAND(m_Ammo, Slot10);
DECLARE_COMMAND(m_Ammo, Close);
DECLARE_COMMAND(m_Ammo, NextWeapon);
DECLARE_COMMAND(m_Ammo, PrevWeapon);
DECLARE_COMMAND(m_Ammo, Adjust_Crosshair);

// width of ammo fonts
#define AMMO_SMALL_WIDTH 10
#define AMMO_LARGE_WIDTH 20

#define HISTORY_DRAW_TIME	"5"

int CHudAmmo::Init(void)
{
	gHUD.AddHudElem(this);

	HOOK_MESSAGE(CurWeapon);
	HOOK_MESSAGE(WeaponList);
	HOOK_MESSAGE(AmmoPickup);
	HOOK_MESSAGE(WeapPickup);
	HOOK_MESSAGE(ItemPickup);
	HOOK_MESSAGE(HideWeapon);
	HOOK_MESSAGE(AmmoX);
	HOOK_MESSAGE(Crosshair);

	HOOK_COMMAND("slot1", Slot1);
	HOOK_COMMAND("slot2", Slot2);
	HOOK_COMMAND("slot3", Slot3);
	HOOK_COMMAND("slot4", Slot4);
	HOOK_COMMAND("slot5", Slot5);
	HOOK_COMMAND("slot6", Slot6);
	HOOK_COMMAND("slot7", Slot7);
	HOOK_COMMAND("slot8", Slot8);
	HOOK_COMMAND("slot9", Slot9);
	HOOK_COMMAND("slot10", Slot10);
	HOOK_COMMAND("cancelselect", Close);
	HOOK_COMMAND("invnext", NextWeapon);
	HOOK_COMMAND("invprev", PrevWeapon);
	HOOK_COMMAND("adjust_crosshair", Adjust_Crosshair);

	Reset();

	CVAR_CREATE( "hud_drawhistory_time", HISTORY_DRAW_TIME, 0 );
	CVAR_CREATE( "hud_fastswitch", "0", FCVAR_ARCHIVE );		// controls whether or not weapons can be selected in one keypress
	CVAR_CREATE( "cl_observercrosshair", "1", 0 );

	m_iFlags |= HUD_ACTIVE | HUD_INTERMISSION; //!!!
	m_bObserverCrosshair = 0;
	m_flCrosshairDistance = 0;
	m_iAmmoLastCheck = 0;
	m_iAlpha = 200;
	m_R = 50;
	m_G = 250;
	m_B = 50;
	m_iCrossColor = 0;

	gWR.Init();
	gHR.Init();

	return 1;
};

void CHudAmmo::Reset(void)
{
	m_fFade = 0;
	m_iFlags |= HUD_ACTIVE; //!!!

	gpActiveSel = NULL;
	gHUD.m_iHideHUDDisplay = 0;

	gWR.Reset();
	gHR.Reset();

	//	VidInit();

}

int CHudAmmo::VidInit(void)
{
	// Load sprites for buckets (top row of weapon menu)
	m_HUD_bucket0 = gHUD.GetSpriteIndex( "bucket1" );
	m_HUD_selection = gHUD.GetSpriteIndex( "selection" );

	ghsprBuckets = gHUD.GetSprite(m_HUD_bucket0);
	giBucketWidth = gHUD.GetSpriteRect(m_HUD_bucket0).right - gHUD.GetSpriteRect(m_HUD_bucket0).left;
	giBucketHeight = gHUD.GetSpriteRect(m_HUD_bucket0).bottom - gHUD.GetSpriteRect(m_HUD_bucket0).top;

	gHR.iHistoryGap = max( gHR.iHistoryGap, gHUD.GetSpriteRect(m_HUD_bucket0).bottom - gHUD.GetSpriteRect(m_HUD_bucket0).top);

	// If we've already loaded weapons, let's get new sprites
	gWR.LoadAllWeaponSprites();

	if (ScreenWidth >= 640)
	{
		giABWidth = 20;
		giABHeight = 4;
	}
	else
	{
		giABWidth = 10;
		giABHeight = 2;
	}

	// the crosshair an observer sees (names ours)
	int iCount;
	client_sprite_t *pList = SPR_GetList( "sprites/observer.txt", &iCount );
	int iRes = ScreenWidth < 640 ? 320 : 640;
	if ( pList )
	{
		client_sprite_t *p = GetSpriteList( pList, "crosshair", iRes, iCount );
		if ( p )
		{
			char sz[32];
			sprintf( sz, "sprites/%s.spr", p->szSprite );
			m_hObserverCrosshair = SPR_Load( sz );
			m_rcObserverCrosshair = p->rc;
		}
		else
			m_hObserverCrosshair = 0;
	}

	return 1;
}

//
// Think:
//  Used for selection of weapon menu item.
//
void CHudAmmo::Think(void)
{
	if ( gHUD.m_fPlayerDead )
		return;

	if ( gHUD.m_iWeaponBits != gWR.iOldWeaponBits )
	{
		gWR.iOldWeaponBits = gHUD.m_iWeaponBits;

		for (int i = MAX_WEAPONS-1; i > 0; i-- )
		{
			WEAPON *p = gWR.GetWeapon(i);

			if ( p )
			{
				if ( gHUD.m_iWeaponBits & ( 1 << p->iId ) )
					gWR.PickupWeapon( p );
				else
					gWR.DropWeapon( p );
			}
		}
	}

	if (!gpActiveSel)
		return;

	// has the player selected one?
	if (gHUD.m_iKeyBits & IN_ATTACK)
	{
		if (gpActiveSel != (WEAPON *)1)
		{
			ServerCmd(gpActiveSel->szName);
			g_weaponselect = gpActiveSel->iId;
		}

		gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
		gHUD.m_iKeyBits &= ~IN_ATTACK;

		PlaySound("common/wpn_select.wav", 1);
	}

}

//
// Helper function to return a Ammo pointer from id
//

HSPRITE* WeaponsResource :: GetAmmoPicFromWeapon( int iAmmoId, wrect_t& rect )
{
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		if ( rgWeapons[i].iAmmoType == iAmmoId )
		{
			rect = rgWeapons[i].rcAmmo;
			return &rgWeapons[i].hAmmo;
		}
		else if ( rgWeapons[i].iAmmo2Type == iAmmoId )
		{
			rect = rgWeapons[i].rcAmmo2;
			return &rgWeapons[i].hAmmo2;
		}
	}

	return NULL;
}


// Menu Selection Code

void WeaponsResource :: SelectSlot( int iSlot, int fAdvance, int iDirection )
{
	if ( gHUD.m_Menu.m_fMenuDisplayed && (fAdvance == FALSE) && (iDirection == 1) )	
	{ // menu is overriding slot use commands
		gHUD.m_Menu.SelectMenuItem( iSlot + 1 );  // slots are one off the key numbers
		return;
	}

	if ( iSlot > MAX_WEAPON_SLOTS )
		return;

	if ( gHUD.m_fPlayerDead || gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) )
		return;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return;

	if ( ! ( gHUD.m_iWeaponBits & ~(1<<(WEAPON_SUIT)) ))
		return;

	WEAPON *p = NULL;
	bool fastSwitch = CVAR_GET_FLOAT( "hud_fastswitch" ) != 0;

	if ( (gpActiveSel == NULL) || (gpActiveSel == (WEAPON *)1) || (iSlot != gpActiveSel->iSlot) )
	{
		PlaySound( "common/wpn_hudon.wav", 1 );
		p = GetFirstPos( iSlot );

		if ( p && fastSwitch ) // check for fast weapon switch mode
		{
			// if fast weapon switch is on, then weapons can be selected in a single keypress
			// but only if there is only one item in the bucket
			WEAPON *p2 = GetNextActivePos( p->iSlot, p->iSlotPos );
			if ( !p2 )
			{	// only one active item in bucket, so change directly to weapon
				ServerCmd( p->szName );
				return;
			}
		}
	}
	else
	{
		PlaySound("common/wpn_moveselect.wav", 1);
		if ( gpActiveSel )
			p = GetNextActivePos( gpActiveSel->iSlot, gpActiveSel->iSlotPos );
		if ( !p )
			p = GetFirstPos( iSlot );
	}

	
	if ( !p )  // no selection found
	{
		// just display the weapon list, unless fastswitch is on just ignore it
		if ( !fastSwitch )
			gpActiveSel = (WEAPON *)1;
		else
			gpActiveSel = NULL;
	}
	else 
		gpActiveSel = p;
}

//------------------------------------------------------------------------
// Message Handlers
//------------------------------------------------------------------------

//
// AmmoX  -- Update the count of a known type of ammo
// 
int CHudAmmo::MsgFunc_AmmoX(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	int iIndex = READ_BYTE();
	int iCount = READ_BYTE();

	gWR.SetAmmo( iIndex, abs(iCount) );

	return 1;
}

int CHudAmmo::MsgFunc_AmmoPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int iIndex = READ_BYTE();
	int iCount = READ_BYTE();

	// Add ammo to the history
	gHR.AddToHistory( HISTSLOT_AMMO, iIndex, abs(iCount) );

	return 1;
}

int CHudAmmo::MsgFunc_WeapPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int iIndex = READ_BYTE();

	// Add the weapon to the history
	gHR.AddToHistory( HISTSLOT_WEAP, iIndex );

	return 1;
}

int CHudAmmo::MsgFunc_ItemPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	const char *szName = READ_STRING();

	// Add the weapon to the history
	gHR.AddToHistory( HISTSLOT_ITEM, szName );

	return 1;
}


int CHudAmmo::MsgFunc_HideWeapon( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	
	gHUD.m_iHideHUDDisplay = READ_BYTE();

	if ( gEngfuncs.IsSpectateOnly() )
		return 1;

	if ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) )
	{
		if ( !m_bObserverCrosshair )
		{
			static wrect_t nullrc;
			gpActiveSel = NULL;
			SetCrosshair( 0, nullrc, 0, 0, 0 );
		}
	}
	else
	{
		if ( m_pWeapon )
			SetCrosshair( m_pWeapon->hCrosshair, m_pWeapon->rcCrosshair, 255, 255, 255 );

		m_bObserverCrosshair = 0;
	}

	return 1;
}

// Message handler for Crosshair message: a byte, show (1) or hide (0) the observer's crosshair
int CHudAmmo::MsgFunc_Crosshair( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int iOn = READ_BYTE();	// name ours

	if ( gEngfuncs.IsSpectateOnly() )
		return 1;

	if ( CVAR_GET_FLOAT( "cl_observercrosshair" ) != 0 )
	{
		if ( iOn )
		{
			SetCrosshair( m_hObserverCrosshair, m_rcObserverCrosshair, 255, 255, 255 );
		}
		else
		{
			static wrect_t nullrc;
			SetCrosshair( 0, nullrc, 0, 0, 0 );
		}

		m_bObserverCrosshair = 1;
	}

	return 1;
}

// 
//  CurWeapon: Update hud state with the current weapon and clip count. Ammo
//  counts are updated with AmmoX. Server assures that the Weapon ammo type 
//  numbers match a real ammo type.
//
int CHudAmmo::MsgFunc_CurWeapon(const char *pszName, int iSize, void *pbuf )
{
	static wrect_t nullrc;
	int fOnTarget = FALSE;

	BEGIN_READ( pbuf, iSize );

	int iState = READ_BYTE();
	int iId = READ_CHAR();
	int iClip = READ_CHAR();

	// detect if we're also on target
	if ( iState > 1 )
	{
		fOnTarget = TRUE;
	}

	if ( iId < 1 && !m_bObserverCrosshair )
	{
		SetCrosshair(0, nullrc, 0, 0, 0);
		return 0;
	}

	if ( g_iUser1 != OBS_IN_EYE )
	{
		// Is player dead???
		if ((iId == -1) && (iClip == -1))
		{
			gHUD.m_fPlayerDead = TRUE;
			gpActiveSel = NULL;
			return 1;
		}
		gHUD.m_fPlayerDead = FALSE;
	}

	WEAPON *pWeapon = gWR.GetWeapon( iId );

	if ( !pWeapon )
		return 0;

	if ( iClip < -1 )
		pWeapon->iClip = abs(iClip);
	else
		pWeapon->iClip = iClip;


	if ( iState == 0 )	// we're not the current weapon, so update no more
		return 1;

	m_pWeapon = pWeapon;

	// Counter-Strike 1.4: whatever the HUD hides
	if ( gHUD.m_iFOV >= 90 )
	{ // normal crosshairs: Draw draws its own
		SetCrosshair(0, nullrc, 0, 0, 0);
	}
	else
	{ // zoomed crosshairs
		if (fOnTarget && m_pWeapon->hZoomedAutoaim)
			SetCrosshair(m_pWeapon->hZoomedAutoaim, m_pWeapon->rcZoomedAutoaim, 255, 255, 255);
		else
			SetCrosshair(m_pWeapon->hZoomedCrosshair, m_pWeapon->rcZoomedCrosshair, 255, 255, 255);
	}

	m_bObserverCrosshair = 0;

	m_fFade = 200.0f; //!!!
	m_iFlags |= HUD_ACTIVE;
	
	return 1;
}

//
// WeaponList -- Tells the hud about a new weapon type.
//
int CHudAmmo::MsgFunc_WeaponList(const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	
	WEAPON Weapon;

	strcpy( Weapon.szName, READ_STRING() );
	Weapon.iAmmoType = (int)READ_CHAR();	
	
	Weapon.iMax1 = READ_BYTE();
	if (Weapon.iMax1 == 255)
		Weapon.iMax1 = -1;

	Weapon.iAmmo2Type = READ_CHAR();
	Weapon.iMax2 = READ_BYTE();
	if (Weapon.iMax2 == 255)
		Weapon.iMax2 = -1;

	Weapon.iSlot = READ_CHAR();
	Weapon.iSlotPos = READ_CHAR();
	Weapon.iId = READ_CHAR();
	Weapon.iFlags = READ_BYTE();
	Weapon.iClip = 0;

	gWR.AddWeapon( &Weapon );

	return 1;

}

//------------------------------------------------------------------------
// Command Handlers
//------------------------------------------------------------------------
// Slot button pressed
void CHudAmmo::SlotInput( int iSlot )
{
	// Let the Viewport use it first, for menus
	if ( gViewPort && gViewPort->SlotInput( iSlot ) )
		return;

	gWR.SelectSlot(iSlot, FALSE, 1);
}

void CHudAmmo::UserCmd_Slot1(void)
{
	SlotInput( 0 );
}

void CHudAmmo::UserCmd_Slot2(void)
{
	SlotInput( 1 );
}

void CHudAmmo::UserCmd_Slot3(void)
{
	SlotInput( 2 );
}

void CHudAmmo::UserCmd_Slot4(void)
{
	SlotInput( 3 );
}

void CHudAmmo::UserCmd_Slot5(void)
{
	SlotInput( 4 );
}

void CHudAmmo::UserCmd_Slot6(void)
{
	SlotInput( 5 );
}

void CHudAmmo::UserCmd_Slot7(void)
{
	SlotInput( 6 );
}

void CHudAmmo::UserCmd_Slot8(void)
{
	SlotInput( 7 );
}

void CHudAmmo::UserCmd_Slot9(void)
{
	SlotInput( 8 );
}

void CHudAmmo::UserCmd_Slot10(void)
{
	SlotInput( 9 );
}

// cycles the crosshair's colour (name ours)
// The switch stands one case to a line, as Counter-Strike 1.6's ammo.cpp has this function's
// cases (its lines 911 to 921: these five colours, the same five again for a crosshair that
// is not drawn additively, then this default).  The layout counts: VC6's intermediate code
// carries a line record for every source line with code (5 bytes each), and ammo.obj places
// the headers' inline functions by the intermediate code before each of its own functions.
// Five lines to a case put vgui::Frame's copy constructor before DrawAmmoBar; retail has it
// after.
void CHudAmmo::UserCmd_Adjust_Crosshair(void)
{
	m_iCrossColor++;
	if ( m_iCrossColor > 4 )
		m_iCrossColor = 0;

	switch ( m_iCrossColor )
	{
	case 0: m_R = 50; m_G = 250; m_B = 50; break;
	case 1: m_R = 250; m_G = 50; m_B = 50; break;
	case 2: m_R = 50; m_G = 50; m_B = 250; break;
	case 3: m_R = 250; m_G = 250; m_B = 50; break;
	case 4: m_R = 50; m_G = 250; m_B = 250; break;
	default: m_R = 50; m_G = 250; m_B = 50; break;
	}
}

void CHudAmmo::UserCmd_Close(void)
{
	if (gpActiveSel)
	{
		gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
		PlaySound("common/wpn_hudoff.wav", 1);
	}
	else
		ClientCmd("escape");
}


// Selects the next item in the weapon menu
void CHudAmmo::UserCmd_NextWeapon(void)
{
	if ( gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) )
		return;

	if ( !gpActiveSel || gpActiveSel == (WEAPON*)1 )
		gpActiveSel = m_pWeapon;

	int pos = 0;
	int slot = 0;
	if ( gpActiveSel )
	{
		pos = gpActiveSel->iSlotPos + 1;
		slot = gpActiveSel->iSlot;
	}

	for ( int loop = 0; loop <= 1; loop++ )
	{
		for ( ; slot < MAX_WEAPON_SLOTS; slot++ )
		{
			for ( ; pos < MAX_WEAPON_POSITIONS; pos++ )
			{
				WEAPON *wsp = gWR.GetWeaponSlot( slot, pos );

				if ( wsp )
				{
					gpActiveSel = wsp;
					return;
				}
			}

			pos = 0;
		}

		slot = 0;  // start looking from the first slot again
	}

	gpActiveSel = NULL;
}

// Selects the previous item in the menu
void CHudAmmo::UserCmd_PrevWeapon(void)
{
	if ( gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) )
		return;

	if ( !gpActiveSel || gpActiveSel == (WEAPON*)1 )
		gpActiveSel = m_pWeapon;

	int pos = MAX_WEAPON_POSITIONS-1;
	int slot = MAX_WEAPON_SLOTS-1;
	if ( gpActiveSel )
	{
		pos = gpActiveSel->iSlotPos - 1;
		slot = gpActiveSel->iSlot;
	}
	
	for ( int loop = 0; loop <= 1; loop++ )
	{
		for ( ; slot >= 0; slot-- )
		{
			for ( ; pos >= 0; pos-- )
			{
				WEAPON *wsp = gWR.GetWeaponSlot( slot, pos );

				if ( wsp )
				{
					gpActiveSel = wsp;
					return;
				}
			}

			pos = MAX_WEAPON_POSITIONS-1;
		}
		
		slot = MAX_WEAPON_SLOTS-1;
	}

	gpActiveSel = NULL;
}



//-------------------------------------------------------------------------
// Drawing code
//-------------------------------------------------------------------------

int CHudAmmo::Draw(float flTime)
{
	int a, x, y, r, g, b;
	int AmmoWidth;

	if ( gEngfuncs.IsSpectateOnly() )
	{
		// Counter-Strike 1.4: the spectator in first person sees the crosshair
		if ( g_iUser1 == OBS_IN_EYE )
			DrawCrosshair( flTime, 0 );
		return 1;
	}

	if ( gHUD.m_iIntermission )
	{
		static wrect_t nullrc;
		SetCrosshair( 0, nullrc, 0, 0, 0 );
		return 1;
	}

	if ( g_iUser1 != OBS_IN_EYE )
	{
		if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
			return 1;

		if ( (gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL )) )
			return 1;

		// Draw Weapon Menu
		DrawWList(flTime);

		// Draw ammo pickup history
		gHR.DrawAmmoHistory( flTime );

		if (!(m_iFlags & HUD_ACTIVE))
			return 0;
	}

	if (!m_pWeapon)
		return 0;

	WEAPON *pw = m_pWeapon; // shorthand

	if ( gHUD.m_iFOV >= 90 && pw->iId != 18 && pw->iId != 24 && pw->iId != 3 && pw->iId != 13 )
		DrawCrosshair( flTime, pw->iId );

	if ( g_iUser1 == OBS_IN_EYE )
		return 1;

	// SPR_Draw Ammo
	if ((pw->iAmmoType < 0) && (pw->iAmmo2Type < 0))
		return 0;


	int iFlags = DHN_DRAWZERO; // draw 0 values

	AmmoWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).right - gHUD.GetSpriteRect(gHUD.m_HUD_number_0).left;

	a = (int) max( MIN_ALPHA, m_fFade );

	if (m_fFade > 0)
		m_fFade -= (gHUD.m_flTimeDelta * 20);

	UnpackRGB(r,g,b, RGB_YELLOWISH);

	ScaleColors(r, g, b, a );

	// Does this weapon have a clip?
	y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight/2;

	// Does weapon have any ammo at all?
	if (m_pWeapon->iAmmoType > 0)
	{
		int iIconWidth = m_pWeapon->rcAmmo.right - m_pWeapon->rcAmmo.left;
		
		if (pw->iClip >= 0)
		{
			// room for the number and the '|' and the current ammo
			
			x = ScreenWidth - (8 * AmmoWidth) - iIconWidth;
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, pw->iClip, r, g, b);

			wrect_t rc;
			rc.top = 0;
			rc.left = 0;
			rc.right = AmmoWidth;
			rc.bottom = 100;

			int iBarWidth =  AmmoWidth/10;

			x += AmmoWidth/2;

			UnpackRGB(r,g,b, RGB_YELLOWISH);

			// draw the | bar
			FillRGBA(x, y, iBarWidth, gHUD.m_iFontHeight, r, g, b, a);

			x += iBarWidth + AmmoWidth/2;;

			// GL Seems to need this
			ScaleColors(r, g, b, a );
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, gWR.CountAmmo(pw->iAmmoType), r, g, b);		


		}
		else
		{
			// SPR_Draw a bullets only line
			x = ScreenWidth - 4 * AmmoWidth - iIconWidth;
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, gWR.CountAmmo(pw->iAmmoType), r, g, b);
		}

		// Draw the ammo Icon
		int iOffset = (m_pWeapon->rcAmmo.bottom - m_pWeapon->rcAmmo.top)/8;
		SPR_Set(m_pWeapon->hAmmo, r, g, b);
		SPR_DrawAdditive(0, x, y - iOffset, &m_pWeapon->rcAmmo);
	}

	// Does weapon have seconday ammo?
	if (pw->iAmmo2Type > 0) 
	{
		int iIconWidth = m_pWeapon->rcAmmo2.right - m_pWeapon->rcAmmo2.left;

		// Do we have secondary ammo?
		if ((pw->iAmmo2Type != 0) && (gWR.CountAmmo(pw->iAmmo2Type) > 0))
		{
			y -= gHUD.m_iFontHeight + gHUD.m_iFontHeight/4;
			x = ScreenWidth - 4 * AmmoWidth - iIconWidth;
			x = gHUD.DrawHudNumber(x, y, iFlags|DHN_3DIGITS, gWR.CountAmmo(pw->iAmmo2Type), r, g, b);

			// Draw the ammo Icon
			SPR_Set(m_pWeapon->hAmmo2, r, g, b);
			int iOffset = (m_pWeapon->rcAmmo2.bottom - m_pWeapon->rcAmmo2.top)/8;
			SPR_DrawAdditive(0, x, y - iOffset, &m_pWeapon->rcAmmo2);
		}
	}
	return 1;
}


//
// Draws the ammo bar on the hud
//
int DrawBar(int x, int y, int width, int height, float f)
{
	int r, g, b;

	if (f < 0)
		f = 0;
	if (f > 1)
		f = 1;

	if (f)
	{
		int w = f * width;

		// Always show at least one pixel if we have ammo.
		if (w <= 0)
			w = 1;
		UnpackRGB(r, g, b, RGB_GREENISH);
		FillRGBA(x, y, w, height, r, g, b, 255);
		x += w;
		width -= w;
	}

	UnpackRGB(r, g, b, RGB_YELLOWISH);

	FillRGBA(x, y, width, height, r, g, b, 128);

	return (x + width);
}



void DrawAmmoBar(WEAPON *p, int x, int y, int width, int height)
{
	if ( !p )
		return;
	
	if (p->iAmmoType != -1)
	{
		if (!gWR.CountAmmo(p->iAmmoType))
			return;

		float f = (float)gWR.CountAmmo(p->iAmmoType)/(float)p->iMax1;
		
		x = DrawBar(x, y, width, height, f);


		// Do we have secondary ammo too?

		if (p->iAmmo2Type != -1)
		{
			f = (float)gWR.CountAmmo(p->iAmmo2Type)/(float)p->iMax2;

			x += 5; //!!!

			DrawBar(x, y, width, height, f);
		}
	}
}

// Counter-Strike 1.4 (names and place Counter-Strike 1.6's): how the crosshair of each weapon spreads:
// 1 in the air, 2 when moving, 4 when ducking, 8 and 16 wider
int GetWeaponAccuracyFlags( int iWeaponID )
{
	switch ( iWeaponID )
	{
	case 16:	// WEAPON_USP
		return ( g_iWeaponFlags & 1 ) ? 15 : 7;
	case 17:	// WEAPON_GLOCK18
		return ( g_iWeaponFlags & 2 ) ? 23 : 7;
	case 8:		// WEAPON_AUG
		return 3;
	case 20:	// WEAPON_M249
		return 3;
	case 27:	// WEAPON_SG552
		return 3;
	case 28:	// WEAPON_AK47
		return 3;
	case 30:	// WEAPON_P90
		return 3;
	case 22:	// WEAPON_M4A1
		return ( g_iWeaponFlags & 4 ) ? 11 : 3;
	case 7:		// WEAPON_MAC10
		return 1;
	case 12:	// WEAPON_UMP45
		return 1;
	case 19:	// WEAPON_MP5N
		return 1;
	case 23:	// WEAPON_TMP
		return 1;
	case 1:		// WEAPON_P228
		return 7;
	case 11:	// WEAPON_FIVESEVEN
		return 7;
	case 26:	// WEAPON_DEAGLE
		return 7;
	case 3:		// WEAPON_SCOUT
		return 0;
	case 4:		// WEAPON_HEGRENADE
		return 0;
	case 5:		// WEAPON_XM1014
		return 0;
	case 6:		// WEAPON_C4
		return 0;
	case 9:		// WEAPON_SMOKEGRENADE
		return 0;
	case 10:	// WEAPON_ELITE
		return 0;
	case 13:	// WEAPON_SG550
		return 0;
	case 18:	// WEAPON_AWP
		return 0;
	case 21:	// WEAPON_M3
		return 0;
	case 24:	// WEAPON_G3SG1
		return 0;
	case 25:	// WEAPON_FLASHBANG
		return 0;
	case 29:	// WEAPON_KNIFE
		return 0;
	}

	return 0;
}

cvar_t *cl_dynamiccrosshair;

int CHudAmmo::DrawCrosshair( float flTime, int weaponid )
{
	int iDistance;
	int iDeltaDistance;
	int iWeaponAccuracyFlags;

	switch ( weaponid )
	{
	case 1: iDistance = 8; iDeltaDistance = 3; break;
	case 4: iDistance = 8; iDeltaDistance = 3; break;
	case 11: iDistance = 8; iDeltaDistance = 3; break;
	case 16: iDistance = 8; iDeltaDistance = 3; break;
	case 17: iDistance = 8; iDeltaDistance = 3; break;
	case 18: iDistance = 8; iDeltaDistance = 3; break;
	case 26: iDistance = 8; iDeltaDistance = 3; break;
	case 19: iDistance = 6; iDeltaDistance = 2; break;
	case 21: iDistance = 8; iDeltaDistance = 6; break;
	case 24: iDistance = 6; iDeltaDistance = 4; break;
	case 28: iDistance = 4; iDeltaDistance = 4; break;
	case 23: iDistance = 7; iDeltaDistance = 3; break;
	case 25: iDistance = 7; iDeltaDistance = 3; break;
	case 29: iDistance = 7; iDeltaDistance = 3; break;
	case 30: iDistance = 7; iDeltaDistance = 3; break;
	case 5: iDistance = 9; iDeltaDistance = 4; break;
	case 7: iDistance = 9; iDeltaDistance = 3; break;
	case 8: iDistance = 3; iDeltaDistance = 3; break;
	case 6: iDistance = 6; iDeltaDistance = 3; break;
	case 12: iDistance = 6; iDeltaDistance = 3; break;
	case 20: iDistance = 6; iDeltaDistance = 3; break;
	case 3: iDistance = 5; iDeltaDistance = 3; break;
	case 13: iDistance = 5; iDeltaDistance = 3; break;
	case 27: iDistance = 5; iDeltaDistance = 3; break;
	default:
		iDistance = 4;
		iDeltaDistance = 3;
		break;
	}

	iWeaponAccuracyFlags = GetWeaponAccuracyFlags( weaponid );

	if ( iWeaponAccuracyFlags && cl_dynamiccrosshair && cl_dynamiccrosshair->value != 0 && !( gHUD.m_iHideHUDDisplay & HIDEHUD_WEAPONS ) )
	{
		if ( !( g_iPlayerFlags & FL_ONGROUND ) && ( iWeaponAccuracyFlags & 1 ) )
		{
			iDistance *= 2.0f;
		}
		else if ( ( g_iPlayerFlags & FL_DUCKING ) && ( iWeaponAccuracyFlags & 4 ) )
		{
			iDistance *= 0.5f;
		}
		else
		{
			int iWeaponMaxSpeed;	// name ours

			switch ( weaponid )
			{
			case 1:		// WEAPON_P228
			case 11:	// WEAPON_FIVESEVEN
			case 16:	// WEAPON_USP
			case 17:	// WEAPON_GLOCK18
			case 26:	// WEAPON_DEAGLE
				iWeaponMaxSpeed = 0;
				break;
			case 8:		// WEAPON_AUG
			case 20:	// WEAPON_M249
			case 22:	// WEAPON_M4A1
			case 27:	// WEAPON_SG552
			case 28:	// WEAPON_AK47
				iWeaponMaxSpeed = 140;
				break;
			case 30:	// WEAPON_P90
				iWeaponMaxSpeed = 170;
				break;
			default:
				iWeaponMaxSpeed = 0;
				break;
			}

			if ( (int)g_flPlayerSpeed > iWeaponMaxSpeed && ( iWeaponAccuracyFlags & 2 ) )
				iDistance *= 1.5f;
		}

		if ( iWeaponAccuracyFlags & 8 )
			iDistance *= 1.4f;

		if ( iWeaponAccuracyFlags & 16 )
			iDistance *= 1.4f;
	}

	if ( g_iShotsFired > m_iAmmoLastCheck )
	{
		m_flCrosshairDistance += iDeltaDistance;
		m_iAlpha -= 40;

		if ( m_flCrosshairDistance > 15 )
			m_flCrosshairDistance = 15;
		if ( m_iAlpha < 120 )
			m_iAlpha = 120;
	}
	else
	{
		m_flCrosshairDistance -= ( m_flCrosshairDistance * 0.013 ) + 0.1;
		m_iAlpha += 2;
	}

	if ( g_iShotsFired > 600 )
		g_iShotsFired = 1;

	m_iAmmoLastCheck = g_iShotsFired;

	if ( m_flCrosshairDistance < iDistance )
		m_flCrosshairDistance = iDistance;
	if ( m_iAlpha > 255 )
		m_iAlpha = 255;

	int iBarSize = 5 - (int)( ( m_flCrosshairDistance - iDistance ) * -0.5f );

	FillRGBA( ScreenWidth / 2 - (int)m_flCrosshairDistance - iBarSize + 1, ScreenHeight / 2, iBarSize, 1, m_R, m_G, m_B, m_iAlpha );
	FillRGBA( ScreenWidth / 2 + (int)m_flCrosshairDistance, ScreenHeight / 2, iBarSize, 1, m_R, m_G, m_B, m_iAlpha );
	FillRGBA( ScreenWidth / 2, ScreenHeight / 2 - (int)m_flCrosshairDistance - iBarSize, 1, iBarSize, m_R, m_G, m_B, m_iAlpha );
	FillRGBA( ScreenWidth / 2, ScreenHeight / 2 + (int)m_flCrosshairDistance, 1, iBarSize, m_R, m_G, m_B, m_iAlpha );

	return 1;
}




//
// Draw Weapon Menu
//
int CHudAmmo::DrawWList(float flTime)
{
	int r,g,b,x,y,a,i;

	if ( !gpActiveSel )
		return 0;

	int iActiveSlot;

	if ( gpActiveSel == (WEAPON *)1 )
		iActiveSlot = -1;	// current slot has no weapons
	else 
		iActiveSlot = gpActiveSel->iSlot;

	// sic: the SDK's `x = 10;` stays (a dead store) and the new value follows as an if/else on
	// one line, here and before the buckets below.  The image gives only the value; the text is
	// read from two measurements.  Counter-Strike 1.6 has this conditional on one source line,
	// one line below the place of the SDK's `x = 10;`, at both places.  And retail's ammo.obj
	// has vgui::ActionSignal's constructor between the two money hooks: with the colour switch
	// above at one case a line, DrawAmmoBar and this function need 109 to 122 bytes more
	// intermediate code than `x = ScreenWidth >= 640 ? 138 : 70;` gives in Betas 6.5 to 7.1
	// together (7.1 alone, by full builds: 110 to 129).  This text gives 118; the conditional
	// expression with the kept stores 38, the one-line if/else without them 80.  An if/else on
	// four lines without the stores gives 110 and builds the same file in all four versions:
	// the image cannot tell the two apart.
	x = 10; //!!!
	if ( ScreenWidth >= 640 ) x = 138; else x = 70;
	y = 10; //!!!
	

	// Ensure that there are available choices in the active slot
	if ( iActiveSlot > 0 )
	{
		if ( !gWR.GetFirstPos( iActiveSlot ) )
		{
			gpActiveSel = (WEAPON *)1;
			iActiveSlot = -1;
		}
	}
		
	// Draw top line
	for ( i = 0; i < MAX_WEAPON_SLOTS; i++ )
	{
		int iWidth;

		UnpackRGB(r,g,b, RGB_YELLOWISH);
	
		if ( iActiveSlot == i )
			a = 255;
		else
			a = 192;

		ScaleColors(r, g, b, 255);
		SPR_Set(gHUD.GetSprite(m_HUD_bucket0 + i), r, g, b );

		// make active slot wide enough to accomodate gun pictures
		if ( i == iActiveSlot )
		{
			WEAPON *p = gWR.GetFirstPos(iActiveSlot);
			if ( p )
				iWidth = p->rcActive.right - p->rcActive.left;
			else
				iWidth = giBucketWidth;
		}
		else
			iWidth = giBucketWidth;

		SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_bucket0 + i));
		
		x += iWidth + 5;
	}


	a = 128; //!!!
	x = 10;
	if ( ScreenWidth >= 640 ) x = 138; else x = 70;	// sic: as above
	// Draw all of the buckets
	for (i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		y = giBucketHeight + 10;

		// If this is the active slot, draw the bigger pictures,
		// otherwise just draw boxes
		if ( i == iActiveSlot )
		{
			WEAPON *p = gWR.GetFirstPos( i );
			int iWidth = giBucketWidth;
			if ( p )
				iWidth = p->rcActive.right - p->rcActive.left;

			for ( int iPos = 0; iPos < MAX_WEAPON_POSITIONS; iPos++ )
			{
				p = gWR.GetWeaponSlot( i, iPos );

				if ( !p || !p->iId )
					continue;

				UnpackRGB( r,g,b, RGB_YELLOWISH );
			
				// Draw Weapon if Red if no ammo

				if ( gWR.HasAmmo(p) )
					ScaleColors(r, g, b, 192);
				else
				{
					UnpackRGB(r,g,b, RGB_REDISH);
					ScaleColors(r, g, b, 128);
				}

				if ( gpActiveSel == p )
				{
					SPR_Set(p->hActive, r, g, b );
					SPR_DrawAdditive(0, x, y, &p->rcActive);

					SPR_Set(gHUD.GetSprite(m_HUD_selection), r, g, b );
					SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_selection));
				}
				else
				{
					SPR_Set( p->hInactive, r, g, b );
					SPR_DrawAdditive( 0, x, y, &p->rcInactive );
				}

				// Draw Ammo Bar

				DrawAmmoBar(p, x + giABWidth/2, y + 3, giABWidth, giABHeight);
				
				y += p->rcActive.bottom - p->rcActive.top + 5;
			}

			x += iWidth + 5;

		}
		else
		{
			// Draw Row of weapons.

			UnpackRGB(r,g,b, RGB_YELLOWISH);

			for ( int iPos = 0; iPos < MAX_WEAPON_POSITIONS; iPos++ )
			{
				WEAPON *p = gWR.GetWeaponSlot( i, iPos );
				
				if ( !p || !p->iId )
					continue;

				if ( gWR.HasAmmo(p) )
				{
					UnpackRGB(r,g,b, RGB_YELLOWISH);
					a = 128;
				}
				else
				{
					UnpackRGB(r,g,b, RGB_REDISH);
					a = 96;
				}

				FillRGBA( x, y, giBucketWidth, giBucketHeight, r, g, b, a );

				y += giBucketHeight + 5;
			}

			x += giBucketWidth + 5;
		}
	}	

	return 1;

}


//
//-----------------------------------------------------
// the money
//
DECLARE_MESSAGE(m_Money, Money)
DECLARE_MESSAGE(m_Money, BlinkAcct)

int CHudMoney::Init( void )
{
	HOOK_MESSAGE(Money);
	HOOK_MESSAGE(BlinkAcct);

	gHUD.AddHudElem(this);

	Reset();

	return 1;
}

int CHudMoney::VidInit( void )
{
	m_HUD_dollar = gHUD.GetSpriteIndex( "dollar" );
	m_HUD_minus = gHUD.GetSpriteIndex( "minus" );
	m_HUD_plus = gHUD.GetSpriteIndex( "plus" );

	return 1;
}

void CHudMoney::Reset( void )
{
	m_iFlags |= HUD_ACTIVE;

	m_iMoneyCount = 0;
	m_iDelta = 0;
	m_fFade = 0;
	m_fFadeDelta = 0;
	m_iBlinkAmt = 0;
	m_fBlinkTime = 0;
}

int CHudMoney::Draw( float flTime )
{
	int r, g, b, x, y, a;
	int r2, g2, b2;	// names ours

	if ( gEngfuncs.IsSpectateOnly() )
		return 1;

	UnpackRGB( r, g, b, RGB_YELLOWISH );

	// Has the money changed? Flash it, green for more, red for less
	if ( m_fFade )
	{
		m_fFade -= gHUD.m_flTimeDelta;

		if ( m_fFade <= 0 )
		{
			a = MIN_ALPHA;
			m_fFade = 0;
			m_iDelta = 0;
		}
		else
		{
			a = MIN_ALPHA + m_fFade * 25.6f;

			// Counter-Strike 1.3 blends every component with the flash colour (green 0/255/0, red 255/0/0);
			// the spelling is ours (the constants and the operand order are retail's)
			if ( m_iDelta > 0 )
			{
				r = r * ( 5 - m_fFade ) / 5 + 0 * m_fFade / 5;
				g = g * ( 5 - m_fFade ) / 5 + 255 * m_fFade / 5;
				b = b * ( 5 - m_fFade ) / 5 + 0 * m_fFade / 5;
			}
			else if ( m_iDelta < 0 )
			{
				r = r * ( 5 - m_fFade ) / 5 + 255 * m_fFade / 5;
				g = g * ( 5 - m_fFade ) / 5 + 0 * m_fFade / 5;
				b = b * ( 5 - m_fFade ) / 5 + 0 * m_fFade / 5;
			}
		}
	}
	else
		a = MIN_ALPHA;

	ScaleColors( r, g, b, a );

	// the change of the last transaction
	if ( m_bShowDelta && m_fFadeDelta )
	{
		m_fFadeDelta -= gHUD.m_flTimeDelta;

		if ( m_fFadeDelta <= 0 )
		{
			m_fFadeDelta = 0;
			m_bShowDelta = 0;
		}
		else
		{
			r2 = r;
			g2 = g;
			b2 = b;
			ScaleColors( r2, g2, b2, m_fFadeDelta * 102.4 );
		}
	}

	// blink the account
	if ( m_iBlinkAmt > 0 )
	{
		if ( flTime > m_fBlinkTime )
		{
			m_fBlinkTime = flTime + 0.05;
			m_iBlinkAmt--;
		}

		if ( m_iBlinkAmt & 1 )
			return 1;
	}

	if ( gHUD.m_iHideHUDDisplay & HIDEHUD_MONEY )
		return 1;

	// Counter-Strike 1.4: not for spectators
	if ( g_iUser1 )
		return 1;

	x = ScreenWidth - gHUD.GetSpriteWidth( gHUD.m_HUD_number_0 ) * 5 - ( ScreenWidth < 640 ? 12 : 16 );
	if ( !( gHUD.m_iWeaponBits & ( 1<<(WEAPON_SUIT) ) ) )
		y = ScreenHeight - gHUD.m_iFontHeight * 5;
	else
		y = ScreenHeight - gHUD.m_iFontHeight * 3;

	SPR_Set( gHUD.GetSprite( m_HUD_dollar ), r, g, b );
	SPR_DrawAdditive( 0, x - gHUD.GetSpriteWidth( m_HUD_dollar ), y, &gHUD.GetSpriteRect( m_HUD_dollar ) );

	gHUD.DrawHudNumber( x, y, m_iMoneyCount, r, g, b );

	if ( m_bShowDelta )
	{
		int iIcon;	// name ours

		if ( m_iDelta < 0 )
			iIcon = m_HUD_minus;
		else
			iIcon = m_HUD_plus;

		y -= gHUD.m_iFontHeight + gHUD.m_iFontHeight / 2;

		SPR_Set( gHUD.GetSprite( iIcon ), r2, g2, b2 );
		SPR_DrawAdditive( 0, x - gHUD.GetSpriteWidth( iIcon ), y, &gHUD.GetSpriteRect( iIcon ) );

		gHUD.DrawHudNumber( x, y, m_iDelta < 0 ? -m_iDelta : m_iDelta, r2, g2, b2 );
	}

	// sic: a stand-in.  Retail's money functions carried more intermediate code than ours for
	// the same machine code.  ammo.obj builds its own precompiled header and emits the VGUI
	// classes' inline copy constructors and assignments by the intermediate code before each
	// of its own functions, so their places among VidInit .. GetSpriteList and vgui::Dar<T>'s
	// members measure it: 83 to 86 empty statements here give retail's order in Betas 6.5, 6.6
	// and 7.0 (6.5 alone takes 82 to 86, 6.6 79 to 86, 7.0 83 to 86); Beta 7.1 takes 84 to 87
	// (full builds), so 84 to 86 serve the four.  At least 8 of them must stand before the end
	// of this function (6.6 has App's copy constructor before MsgFunc_Money); the others give
	// the same order between MsgFunc_BlinkAcct and GetSpriteList (measured in an unused inline
	// function there), where Counter-Strike 1.6's ammo.cpp has nothing.  What text it was, the
	// image does not say.  Counter-Strike 1.0 takes 85 and
	// 86 (full builds; 84 and 87 do not).
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;

	return 1;
}

int CHudMoney::MsgFunc_Money( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int iMoney = READ_LONG();	// name ours
	m_bShowDelta = READ_BYTE();

	m_iDelta = iMoney - m_iMoneyCount;
	m_iMoneyCount = iMoney;

	if ( m_iDelta )
	{
		m_fFade = 5.0;
		m_fFadeDelta = 2.5;
	}
	else if ( m_bShowDelta )
		m_bShowDelta = 0;

	return 1;
}

int CHudMoney::MsgFunc_BlinkAcct( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iBlinkAmt = READ_BYTE() * 2;
	m_fBlinkTime = 0;

	return 1;
}

/* =================================
	GetSpriteList

Finds and returns the matching 
sprite name 'psz' and resolution 'iRes'
in the given sprite list 'pList'
iCount is the number of items in the pList
================================= */
client_sprite_t *GetSpriteList(client_sprite_t *pList, const char *psz, int iRes, int iCount)
{
	if (!pList)
		return NULL;

	int i = iCount;
	client_sprite_t *p = pList;

	while(i--)
	{
		if ((!strcmp(psz, p->szName)) && (p->iRes == iRes))
			return p;
		p++;
	}

	return NULL;
}
