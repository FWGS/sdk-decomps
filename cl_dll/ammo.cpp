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
//
// Ammo.cpp
//
// implementation of CHudAmmo class
//
// Gunman: slots 1-9 open a VGUI weapon menu; slot 10 lists the current
// weapon's customization items, whose settings the weapon keys step through
// and that are drawn here (chemgun vials, beamgun dials, gauss modes ...).
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

#include "ammohistory.h"
#include "vgui_TeamFortressViewport.h"

WEAPON *gpActiveSel;	// NULL means off, 1 means just the menu bar, otherwise
						// this points to the active weapon menu item
WEAPON *gpLastSel;		// the weapon lastinv goes back to
WEAPON *gpMenuWeapon;	// the weapon held when the menu opened

int giActiveSlot;		// the slot the menu shows
float g_flSelTime;		// cleared when the selection moves; never read
int g_fWeaponSelected = 1;	// set when a weapon is picked; the heat gauge reads it

int g_iHeatWeapon;		// TempX: the weapon whose heat the gauge shows
int g_iHeat;			// and its heat

client_sprite_t *GetSpriteList(client_sprite_t *pList, const char *psz, int iRes, int iCount);

WeaponsResource gWR;

int g_weaponselect = 0;

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

	if (ScreenWidth <= 640)
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
// Gunman: a customization item counts when it belongs to the current weapon.
WEAPON *WeaponsResource :: GetFirstPos( int iSlot )
{
	WEAPON *pret = NULL;

	for (int i = 0; i < MAX_WEAPON_POSITIONS; i++)
	{
		if ( rgSlots[iSlot][i] && rgSlots[iSlot][i]->iIsCustomization == 1 )
		{
			if ( gHUD.m_Ammo.m_pWeapon && !strcmp( rgSlots[iSlot][i]->szCustomizes, gHUD.m_Ammo.m_pWeapon->szName ) )
				return rgSlots[iSlot][i];
		}
		else if ( rgSlots[iSlot][i] && HasAmmo( rgSlots[iSlot][i] ) )
		{
			pret = rgSlots[iSlot][i];
			break;
		}
	}

	return pret;
}

// Gunman: hides the customization items of the other weapons
WEAPON* WeaponsResource :: GetWeaponSlot( int slot, int pos )
{
	WEAPON *p = rgSlots[slot][pos];

	if ( p && p->iIsCustomization == 1 )
	{
		if ( gHUD.m_Ammo.m_pWeapon && !strcmp( p->szCustomizes, gHUD.m_Ammo.m_pWeapon->szName ) )
			return p;

		return NULL;
	}

	return p;
}

WEAPON* WeaponsResource :: GetNextActivePos( int iSlot, int iSlotPos )
{
	if ( iSlotPos >= MAX_WEAPON_POSITIONS || iSlot >= MAX_WEAPON_SLOTS )
		return NULL;

	WEAPON *p = gWR.rgSlots[ iSlot ][ iSlotPos+1 ];
	
	if ( p && p->iIsCustomization == 1 )
	{
		if ( gHUD.m_Ammo.m_pWeapon && !strcmp( p->szCustomizes, gHUD.m_Ammo.m_pWeapon->szName ) )
			return p;
	}
	else if ( p && gWR.HasAmmo(p) )
		return p;

	return GetNextActivePos( iSlot, iSlotPos + 1 );
}


int giBucketHeight, giBucketWidth, giABHeight, giABWidth; // Ammo Bar width and height

HSPRITE ghsprBuckets;					// Sprite for top row of weapons menu

// Gunman's HUD sprites (hud.txt)
int g_HUD_g_arrow, g_HUD_g_bar, g_HUD_g_slider;
int g_HUD_vial_ec, g_HUD_vial_el, g_HUD_vial_er;	// chemgun vials: empty
int g_HUD_vial_fc, g_HUD_vial_fl, g_HUD_vial_fr;	// and full
int g_HUD_chem_slider, g_HUD_chem_bar;
int g_HUD_beamdials, g_HUD_pointer;
int g_HUD_fired, g_HUD_locked, g_HUD_guided, g_HUD_homing, g_HUD_spiral;
int g_HUD_impact, g_HUD_proximity, g_HUD_timed, g_HUD_explosive, g_HUD_cluster;
int g_HUD_launch, g_HUD_flightpath, g_HUD_detonate, g_HUD_payload, g_HUD_while, g_HUD_on;
int g_HUD_pulseAct, g_HUD_pulse, g_HUD_chargeAct, g_HUD_charge;	// gauss pistol modes
int g_HUD_rapidAct, g_HUD_rapid, g_HUD_rapidSnipe, g_HUD_rapidSnipeAct;
int g_HUD_sniper, g_HUD_sniperAct;
int g_HUD_tripped, g_HUD_degrees, g_HUD_mechaArrow, g_HUD_shells;
int g_HUD_glow_beamgun, g_HUD_glow_chemgun, g_HUD_glow_dml, g_HUD_glow_grenade;	// weapon menu
int g_HUD_glow_pistol, g_HUD_glow_knife, g_HUD_glow_minigun, g_HUD_glow_shotgun;
int g_HUD_glow_bracketleft, g_HUD_glow_bracketright;
int g_HUD_number_10;
int g_HUD_glow_beamgun_no, g_HUD_glow_chemgun_no, g_HUD_glow_dml_no, g_HUD_glow_grenade_no;
int g_HUD_glow_pistol_no, g_HUD_glow_minigun_no, g_HUD_glow_shotgun_no;
int g_HUD_selectLeft, g_HUD_selectRight;

DECLARE_MESSAGE(m_Ammo, CurWeapon );	// Current weapon and clip
DECLARE_MESSAGE(m_Ammo, WeaponList);	// new weapon type
DECLARE_MESSAGE(m_Ammo, AmmoX);			// update known ammo type's count
DECLARE_MESSAGE(m_Ammo, AmmoPickup);	// flashes an ammo pickup record
DECLARE_MESSAGE(m_Ammo, WeapPickup);    // flashes a weapon pickup record
DECLARE_MESSAGE(m_Ammo, HideWeapon);	// hides the weapon, ammo, and crosshair displays temporarily
DECLARE_MESSAGE(m_Ammo, ItemPickup);
DECLARE_MESSAGE(m_Ammo, SliderX);		// sets a customization item's setting
DECLARE_MESSAGE(m_Ammo, TempX);			// a weapon's heat

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
DECLARE_COMMAND(m_Ammo, LastInv);

DECLARE_MESSAGE(m_Ammo, Tank);			// gets in and out of the tank

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
	HOOK_MESSAGE(SliderX);
	HOOK_MESSAGE(TempX);
	HOOK_MESSAGE(Tank);

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
	HOOK_COMMAND("lastinv", LastInv);

	Reset();

	CVAR_CREATE( "hud_drawhistory_time", HISTORY_DRAW_TIME, 0 );
	CVAR_CREATE( "hud_fastswitch", "0", FCVAR_ARCHIVE );		// controls whether or not weapons can be selected in one keypress
	CVAR_CREATE( "hud_smallhud", "0", FCVAR_ARCHIVE );
	CVAR_CREATE( "hud_reverseweapons", "0", FCVAR_ARCHIVE );	// swaps invnext and invprev
	CVAR_CREATE( "hud_autoswitch", "1", FCVAR_ARCHIVE );		// selecting a weapon in the menu switches to it
	CVAR_CREATE( "hud_customwrap", "0", FCVAR_ARCHIVE );		// customization settings wrap around
	CVAR_CREATE( "violence_germany", "0", FCVAR_ARCHIVE );

	m_iFlags |= HUD_ACTIVE; //!!!

	m_fCustomizing = 0;
	m_fCustomPrev = 0;
	m_fCustomNext = 0;
	m_flNextFrame = 0;
	m_iFrame = 0;
	m_fGrenadesPicked = 0;

	gWR.Init();
	gHR.Init();

	gpActiveSel = NULL;

	return 1;
};

void CHudAmmo::Reset(void)
{
	m_fFade = 0;
	m_iFlags |= HUD_ACTIVE; //!!!

	m_fCustomizing = 0;
	m_fCustomPrev = 0;
	m_fCustomNext = 0;

	gpActiveSel = NULL;
	gHUD.m_iHideHUDDisplay = 0;
	m_fMenuOpen = false;

	gWR.Reset();
	gHR.Reset();

	gHUD.m_Tank.Reset();
	gHUD.m_Zoom.Reset();
	gHUD.m_DmlCross.Reset();

	//	VidInit();

}

int CHudAmmo::VidInit(void)
{
	// Load sprites for buckets (top row of weapon menu)
	m_HUD_bucket0 = gHUD.GetSpriteIndex( "bucket1" );
	m_HUD_selection = gHUD.GetSpriteIndex( "selection" );

	g_HUD_g_arrow = gHUD.GetSpriteIndex( "g_arrow" );
	g_HUD_g_bar = gHUD.GetSpriteIndex( "g_bar" );
	g_HUD_g_slider = gHUD.GetSpriteIndex( "g_slider" );
	g_HUD_vial_ec = gHUD.GetSpriteIndex( "vial_ec" );
	g_HUD_vial_el = gHUD.GetSpriteIndex( "vial_el" );
	g_HUD_vial_er = gHUD.GetSpriteIndex( "vial_er" );
	g_HUD_vial_fc = gHUD.GetSpriteIndex( "vial_fc" );
	g_HUD_vial_fl = gHUD.GetSpriteIndex( "vial_fl" );
	g_HUD_vial_fr = gHUD.GetSpriteIndex( "vial_fr" );
	g_HUD_chem_slider = gHUD.GetSpriteIndex( "chem_slider" );
	g_HUD_chem_bar = gHUD.GetSpriteIndex( "chem_bar" );
	g_HUD_beamdials = gHUD.GetSpriteIndex( "beamdials" );
	g_HUD_pointer = gHUD.GetSpriteIndex( "pointer" );
	g_HUD_fired = gHUD.GetSpriteIndex( "fired" );
	g_HUD_locked = gHUD.GetSpriteIndex( "locked" );
	g_HUD_guided = gHUD.GetSpriteIndex( "guided" );
	g_HUD_homing = gHUD.GetSpriteIndex( "homing" );
	g_HUD_spiral = gHUD.GetSpriteIndex( "spiral" );
	g_HUD_impact = gHUD.GetSpriteIndex( "impact" );
	g_HUD_proximity = gHUD.GetSpriteIndex( "proximity" );
	g_HUD_timed = gHUD.GetSpriteIndex( "timed" );
	g_HUD_explosive = gHUD.GetSpriteIndex( "explosive" );
	g_HUD_cluster = gHUD.GetSpriteIndex( "cluster" );
	g_HUD_launch = gHUD.GetSpriteIndex( "launch" );
	g_HUD_flightpath = gHUD.GetSpriteIndex( "flightpath" );
	g_HUD_detonate = gHUD.GetSpriteIndex( "detonate" );
	g_HUD_payload = gHUD.GetSpriteIndex( "payload" );
	g_HUD_while = gHUD.GetSpriteIndex( "while" );
	g_HUD_on = gHUD.GetSpriteIndex( "on" );
	g_HUD_pulseAct = gHUD.GetSpriteIndex( "pulseAct" );
	g_HUD_pulse = gHUD.GetSpriteIndex( "pulse" );
	g_HUD_chargeAct = gHUD.GetSpriteIndex( "chargeAct" );
	g_HUD_charge = gHUD.GetSpriteIndex( "charge" );
	g_HUD_rapidAct = gHUD.GetSpriteIndex( "rapidAct" );
	g_HUD_rapid = gHUD.GetSpriteIndex( "rapid" );
	g_HUD_rapidSnipe = gHUD.GetSpriteIndex( "rapidSnipe" );
	g_HUD_rapidSnipeAct = gHUD.GetSpriteIndex( "rapidSnipeAct" );
	g_HUD_sniper = gHUD.GetSpriteIndex( "sniper" );
	g_HUD_sniperAct = gHUD.GetSpriteIndex( "sniperAct" );
	g_HUD_tripped = gHUD.GetSpriteIndex( "tripped" );
	g_HUD_degrees = gHUD.GetSpriteIndex( "degrees" );
	g_HUD_mechaArrow = gHUD.GetSpriteIndex( "mechaArrow" );
	g_HUD_shells = gHUD.GetSpriteIndex( "shells" );
	g_HUD_glow_beamgun = gHUD.GetSpriteIndex( "glow_beamgun" );
	g_HUD_glow_chemgun = gHUD.GetSpriteIndex( "glow_chemgun" );
	g_HUD_glow_dml = gHUD.GetSpriteIndex( "glow_dml" );
	g_HUD_glow_grenade = gHUD.GetSpriteIndex( "glow_grenade" );
	g_HUD_glow_pistol = gHUD.GetSpriteIndex( "glow_pistol" );
	g_HUD_glow_knife = gHUD.GetSpriteIndex( "glow_knife" );
	g_HUD_glow_minigun = gHUD.GetSpriteIndex( "glow_minigun" );
	g_HUD_glow_shotgun = gHUD.GetSpriteIndex( "glow_shotgun" );
	g_HUD_glow_bracketleft = gHUD.GetSpriteIndex( "glow_bracketleft" );
	g_HUD_glow_bracketright = gHUD.GetSpriteIndex( "glow_bracketright" );
	g_HUD_number_10 = gHUD.GetSpriteIndex( "number_10" );
	g_HUD_glow_beamgun_no = gHUD.GetSpriteIndex( "glow_beamgun_no" );
	g_HUD_glow_chemgun_no = gHUD.GetSpriteIndex( "glow_chemgun_no" );
	g_HUD_glow_dml_no = gHUD.GetSpriteIndex( "glow_dml_no" );
	g_HUD_glow_grenade_no = gHUD.GetSpriteIndex( "glow_grenade_no" );
	g_HUD_glow_pistol_no = gHUD.GetSpriteIndex( "glow_pistol_no" );
	g_HUD_glow_minigun_no = gHUD.GetSpriteIndex( "glow_minigun_no" );
	g_HUD_glow_shotgun_no = gHUD.GetSpriteIndex( "glow_shotgun_no" );
	g_HUD_selectLeft = gHUD.GetSpriteIndex( "selectLeft" );
	g_HUD_selectRight = gHUD.GetSpriteIndex( "selectRight" );

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

	// no menu in the tank
	if ( m_fCustomizing && gHUD.m_Tank.m_fInTank )
	{
		gpActiveSel = NULL;
		gHUD.m_iKeyBits &= ~IN_ATTACK;
		m_fCustomizing = 0;
		return;
	}

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

	// a customization item with settings opens them
	if ( gpActiveSel != (WEAPON *)1 && gpActiveSel->iIsCustomization == 1 )
		m_fCustomizing = gpActiveSel->iCustomSettings != 0;
	else
		m_fCustomizing = 0;

	// has the player selected one?
	if (gHUD.m_iKeyBits & IN_ATTACK)
	{
		g_flSelTime = 0;
		g_fWeaponSelected = 1;

		if (gpActiveSel != (WEAPON *)1)
		{
			if ( gpActiveSel->iIsCustomization == 1 )
			{
				if ( gpActiveSel->iCustomSettings )
				{
					// opens and closes its settings
					if ( m_fCustomizing )
						m_fCustomizing = 0;
					else
					{
						m_fCustomizing = 1;
						gHUD.m_iKeyBits &= ~IN_ATTACK;
					}
				}
				else
				{
					// an on/off item
					gpActiveSel->iCustomSetting = !gpActiveSel->iCustomSetting;
				}
			}
			else
			{
				gpLastSel = gpMenuWeapon;
				gpMenuWeapon = NULL;
				ServerCmd(gpActiveSel->szName);
				g_weaponselect = gpActiveSel->iId;

				if ( gViewPort )
					gViewPort->HideVGUIMenu();
				m_fMenuOpen = false;

				PlaySound("common/wpn_select.wav", 1);
			}

			if ( gpActiveSel != (WEAPON *)1 && gpActiveSel->iIsCustomization )
			{
				SendCustomization();
				PlaySound("common/wpn_hudoff.wav", 1);
				gViewPort->m_pCustomMenu->HideAll();
			}
		}

		gpActiveSel = NULL;
		gHUD.m_iKeyBits &= ~IN_ATTACK;
	}

	if ( !m_fCustomizing || gpActiveSel == (WEAPON *)1 )
		return;

	// sic: gpActiveSel is NULL here if the attack key just closed the menu
	if ( m_fCustomPrev )
	{
		gpActiveSel->iCustomSetting--;
		m_fCustomPrev = 0;

		if ( gpActiveSel->iCustomSetting < 1 )
		{
			if ( CVAR_GET_FLOAT( "hud_customwrap" ) && strcmp( gpActiveSel->szCustomizes, "weapon_SPchemicalgun" ) )
				gpActiveSel->iCustomSetting = gpActiveSel->iCustomSettings;
			else
				gpActiveSel->iCustomSetting = 1;
		}
	}

	if ( m_fCustomNext )
	{
		gpActiveSel->iCustomSetting++;
		m_fCustomNext = 0;

		if ( gpActiveSel->iCustomSetting > gpActiveSel->iCustomSettings )
		{
			if ( CVAR_GET_FLOAT( "hud_customwrap" ) && strcmp( gpActiveSel->szCustomizes, "weapon_SPchemicalgun" ) )
				gpActiveSel->iCustomSetting = 1;
			else
				gpActiveSel->iCustomSetting = gpActiveSel->iCustomSettings;
		}

		// the gauss pistol's sniper mode needs the scope
		if ( m_pWeapon && gpActiveSel->iCustomSetting == 4 && !strcmp( gHUD.m_Ammo.m_pWeapon->szName, "weapon_gausspistol" ) )
		{
			if ( !gWR.HasAmmo( gWR.GetWeaponSlot( 9, 19 ) ) )
			{
				if ( CVAR_GET_FLOAT( "hud_customwrap" ) )
					gpActiveSel->iCustomSetting = 1;
				else
					gpActiveSel->iCustomSetting = 3;
			}
		}
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
	if ( gHUD.m_Tank.m_fInTank )
		return;

	// the AI core has no menu
	if ( gHUD.m_Ammo.m_pWeapon && !strcmp( gHUD.m_Ammo.m_pWeapon->szName, "weapon_aicore" ) )
	{
		gHUD.m_Ammo.m_fCustomizing = 0;
		gpActiveSel = NULL;
	}

	if ( giActiveSlot != iSlot )
		g_flSelTime = 0;

	if ( gHUD.m_Menu.m_fMenuDisplayed && (fAdvance == FALSE) && (iDirection == 1) )	
	{ // menu is overriding slot use commands
		gHUD.m_Menu.SelectMenuItem( iSlot + 1 );  // slots are one off the key numbers
		return;
	}

	if ( iSlot > MAX_WEAPON_SLOTS )
		return;

	if ( gHUD.m_fPlayerDead || gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) )
		return;

	WEAPON *p;

	if ( gpActiveSel && gpActiveSel != (WEAPON *)1 && iSlot == gpActiveSel->iSlot )
	{
		p = GetNextActivePos( gpActiveSel->iSlot, gpActiveSel->iSlotPos );
		if ( !p )
			p = GetFirstPos( iSlot );
	}
	else
	{
		p = GetFirstPos( iSlot );

		if ( !p || iSlot != 9 )
			PlaySound( p ? "common/wpn_moveselect.wav" : "common/wpn_denyselect.wav", 1 );

		if ( p && strcmp( p->szCustomizes, "weapon_gausspistol" ) )
		{
			WEAPON *p2 = GetNextActivePos( p->iSlot, p->iSlotPos );

			if ( !gpActiveSel && iSlot == 9 )
			{
				if ( p2 )
					PlaySound( "common/wpn_hudon.wav", 1 );
				else
					PlaySound( "common/wpn_moveselect.wav", 1 );
			}

			if ( !p2 || !strcmp( p2->szName, "cust_2MinigunCooled" ) )
			{	// only one active item in bucket, so change directly to weapon
				if ( CVAR_GET_FLOAT( "hud_autoswitch" ) || iSlot == 9 )
				{
					ServerCmd( p->szName );
					g_weaponselect = p->iId;
				}

				if ( !p->iIsCustomization )
					gpActiveSel = p;
				return;
			}
		}
		else if ( !gpActiveSel && iSlot == 9 )
			PlaySound( "common/wpn_hudon.wav", 1 );
	}

	if ( !p )  // no selection found
		gpActiveSel = NULL;
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

//
// SliderX -- sets a customization item's setting
//
int CHudAmmo::MsgFunc_SliderX( const char *pszName, int iSize, void *pbuf )
{
	if ( pbuf && pszName && iSize )
	{
		BEGIN_READ( pbuf, iSize );

		int iId = READ_BYTE();
		int iSetting = READ_BYTE();

		if ( iId && iSetting )
		{
			WEAPON *p = gWR.GetWeapon( iId );

			if ( p )
				p->iCustomSetting = iSetting;
		}
	}

	return 1;
}

int CHudAmmo::MsgFunc_Tank( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int iState = READ_SHORT();
	gHUD.m_Tank.SetTank( iState );

	return 1;
}

//
// TempX -- the current weapon's heat
//
int CHudAmmo::MsgFunc_TempX( const char *pszName, int iSize, void *pbuf )
{
	if ( pbuf && pszName && iSize )
	{
		BEGIN_READ( pbuf, iSize );

		int iId = READ_BYTE();
		int iHeat = READ_BYTE();

		if ( iId && iHeat && m_pWeapon && iHeat > 0 && m_pWeapon->iId == iId )
		{
			g_iHeatWeapon = iId;
			g_iHeat = iHeat;
		}
	}

	return 1;
}

int CHudAmmo::MsgFunc_AmmoPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int iIndex = READ_BYTE();
	int iCount = READ_BYTE();

	// Add ammo to the history
	if ( gViewPort && gViewPort->m_pTankPanel )
		gViewPort->m_pTankPanel->AddToHistory( iIndex, 0 );

	// the first grenades picked up get selected
	WEAPON *p = gWR.GetWeapon( 13 );	// WEAPON_DMLGRENADE
	if ( !m_fGrenadesPicked && p && p->iAmmoType == iIndex && !gWR.HasAmmo( p ) )
	{
		m_fGrenadesPicked = 1;

		if ( m_fMenuOpen && gViewPort && gViewPort->m_pWeaponMenu && gViewPort->m_pWeaponMenu->m_pSlots[p->iSlot] )
			gViewPort->m_pWeaponMenu->m_pSlots[p->iSlot]->setVisible( true );

		if ( ( CVAR_GET_FLOAT( "hud_autoswitch" ) || !m_pWeapon || !gWR.HasAmmo( m_pWeapon ) ) && !m_fCustomizing )
		{
			ServerCmd( p->szName );
			g_weaponselect = p->iId;
		}
	}

	// light up the slot of the weapon that uses it
	for ( int i = 0; i < 8; i++ )
	{
		WEAPON *pw = gWR.GetWeaponSlot( i, 0 );

		if ( pw && pw->iAmmoType == iIndex )
		{
			if ( gViewPort->m_pWeaponMenu->m_pSlots[i] )
				gViewPort->m_pWeaponMenu->m_pSlots[i]->setVisible( true );
			break;
		}
	}

	return 1;
}

int CHudAmmo::MsgFunc_WeapPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int iIndex = READ_BYTE();

	WEAPON *p = gWR.GetWeapon( iIndex );

	if ( !p )
		return 1;

	if ( m_fMenuOpen && gViewPort && gViewPort->m_pWeaponMenu && !p->iIsCustomization )
	{
		if ( !gViewPort->m_pWeaponMenu->m_pSlots[p->iSlot] )
			return 1;

		gViewPort->m_pWeaponMenu->m_pSlots[p->iSlot]->setVisible( true );
	}

	if ( !p->iIsCustomization )
	{
		if ( ( CVAR_GET_FLOAT( "hud_autoswitch" ) || !m_pWeapon || !gWR.HasAmmo( m_pWeapon ) ) && !m_fCustomizing )
		{
			ServerCmd( p->szName );
			g_weaponselect = p->iId;
			gpLastSel = m_pWeapon;
		}

		// Add the weapon to the history
		if ( gViewPort && gViewPort->m_pTankPanel && p->iId != 13 && p->iId != 1 )	// not the grenades or the fists
			gViewPort->m_pTankPanel->AddToHistory( iIndex, 1 );

		return 1;
	}

	if ( gViewPort && gViewPort->m_pTankPanel )
		gViewPort->m_pTankPanel->AddToHistory( iIndex, 1 );

	// the minigun's coolers and the gauss pistol's scope set their weapons up
	if ( !strcmp( p->szName, "cust_2MinigunCooled" ) )
	{
		ClientCmd( p->szCustomizes );
		ClientCmd( "cust_20" );
		return 1;
	}

	if ( !strcmp( p->szName, "cust_2GaussPistolSniper" ) && CVAR_GET_FLOAT( "hud_autoswitch" ) )
	{
		ClientCmd( p->szCustomizes );
		ClientCmd( "cust_14" );
		gWR.GetWeapon( 31 )->iCustomSetting = 4;	// cust_1GaussPistolFireType: sniper
		gpLastSel = m_pWeapon;
	}

	return 1;
}

int CHudAmmo::MsgFunc_ItemPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	const char *szName = READ_STRING();

	// Add the item to the history
	if ( gViewPort && gViewPort->m_pTankPanel )
	{
		if ( !strcmp( szName, "item_healthkit" ) )
			gViewPort->m_pTankPanel->AddToHistory( 7, 0 );

		if ( !strcmp( szName, "item_armor" ) )
			gViewPort->m_pTankPanel->AddToHistory( 8, 0 );
	}

	return 1;
}


int CHudAmmo::MsgFunc_HideWeapon( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	
	gHUD.m_iHideHUDDisplay = READ_BYTE();

	if ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) )
	{
		static wrect_t nullrc;
		gpActiveSel = NULL;
		SetCrosshair( 0, nullrc, 0, 0, 0 );
	}
	else
	{
		if ( m_pWeapon )
			SetCrosshair( m_pWeapon->hCrosshair, m_pWeapon->rcCrosshair, 255, 255, 255 );
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

	if ( iId < 1 )
	{
		SetCrosshair(0, nullrc, 0, 0, 0);
		m_pWeapon = NULL;

		if ( gViewPort && gViewPort->m_pCustomMenu )
			gViewPort->m_pCustomMenu->HideAll();
		return 0;
	}

	// Is player dead???
	if ((iId == -1) && (iClip == -1))
	{
		gHUD.m_fPlayerDead = TRUE;
		gpActiveSel = NULL;
		return 1;
	}
	gHUD.m_fPlayerDead = FALSE;

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

	if ( !(gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL )) )
	{
		if ( gHUD.m_iFOV >= 90 )
		{ // normal crosshairs
			if (fOnTarget && m_pWeapon->hAutoaim)
				SetCrosshair(m_pWeapon->hAutoaim, m_pWeapon->rcAutoaim, 255, 255, 255);
			else
				SetCrosshair(m_pWeapon->hCrosshair, m_pWeapon->rcCrosshair, 255, 255, 255);
		}
		else
		{ // zoomed crosshairs
			if (fOnTarget && m_pWeapon->hZoomedAutoaim)
				SetCrosshair(m_pWeapon->hZoomedAutoaim, m_pWeapon->rcZoomedAutoaim, 255, 255, 255);
			else
				SetCrosshair(m_pWeapon->hZoomedCrosshair, m_pWeapon->rcZoomedCrosshair, 255, 255, 255);
		}
	}

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

	strcpy( Weapon.szCustomizes, READ_STRING() );
	Weapon.iIsCustomization = READ_CHAR();
	Weapon.iCustomSettings = READ_CHAR();
	Weapon.iCustomSetting = READ_CHAR();

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

	if ( m_fCustomizing )
		return;

	if ( CVAR_GET_FLOAT( "hud_fastswitch" ) && !m_fCustomizing && !gHUD.m_Tank.m_fInTank )
	{
		// fast switch: the slot key picks the slot's weapon at once
		WEAPON *p = gWR.GetWeaponSlot( iSlot, 0 );

		if ( p && !p->iIsCustomization && gWR.HasAmmo( p ) )
		{
			gpLastSel = m_pWeapon;
			ServerCmd( p->szName );
			g_weaponselect = p->iId;
			PlaySound( "common/wpn_moveselect.wav", 1 );
			return;
		}

		PlaySound( "common/wpn_denyselect.wav", 1 );
		return;
	}

	if ( !gpMenuWeapon )
		gpMenuWeapon = m_pWeapon;

	// any slot key goes back to the customization items
	if ( gpActiveSel && gpActiveSel != (WEAPON *)1 && gpActiveSel->iIsCustomization && !m_fCustomizing )
	{
		PlaySound( "common/wpn_hudon.wav", 1 );
		gWR.SelectSlot( 9, FALSE, 1 );
		return;
	}

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

// +custom: opens the current weapon's customization items
void CHudAmmo::UserCmd_Customize(void)
{
	if ( !m_pWeapon )
		return;

	if ( gpActiveSel && gpActiveSel != (WEAPON *)1 && gpActiveSel->iIsCustomization && strcmp( gpActiveSel->szCustomizes, "weapon_minigun" ) )
		SendCustomization();

	if ( m_fMenuOpen )
	{
		if ( gViewPort )
			gViewPort->HideVGUIMenu();
		m_fMenuOpen = false;
	}

	if ( gEngfuncs.GetMaxClients() < 2 )
		ServerCmd( "custupdate" );

	gWR.SelectSlot( 9, FALSE, 1 );
}

// switches back to the previous weapon
void CHudAmmo::UserCmd_LastInv(void)
{
	if ( gHUD.m_Tank.m_fInTank || m_fCustomizing || !gpLastSel )
		return;

	if ( gWR.HasAmmo( gpLastSel ) )
	{
		WEAPON *p = m_pWeapon;

		ServerCmd( gpLastSel->szName );
		g_weaponselect = gpLastSel->iId;
		gpLastSel = p;
		return;
	}

	PlaySound( "common/wpn_denyselect.wav", 1 );
}

void CHudAmmo::UserCmd_Close(void)
{
	m_fCustomizing = 0;

	if (gpActiveSel)
	{
		gpActiveSel = NULL;

		if ( gViewPort && gViewPort->m_pCustomMenu )
			gViewPort->m_pCustomMenu->HideAll();
	}
	else
		ClientCmd("escape");
}


// Selects the next item in the weapon menu
// Gunman: zooms in the scope, steps through customization settings
void CHudAmmo::UserCmd_NextWeapon(void)
{
	if ( gHUD.m_Tank.m_fInTank )
		return;

	if ( gHUD.m_Zoom.m_fZoomed )
	{
		gHUD.m_Zoom.ZoomIn();
		return;
	}

	if ( gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) )
		return;

	if ( m_pWeapon && !strcmp( m_pWeapon->szName, "weapon_aicore" ) )
		m_fCustomizing = 0;

	if ( m_fCustomizing )
	{
		m_fCustomNext = 1;
		return;
	}

	if ( gpActiveSel && gpActiveSel != (WEAPON *)1 && gpActiveSel->iIsCustomization )
	{
		gWR.SelectSlot( 9, FALSE, 1 );
		return;
	}

	if ( CVAR_GET_FLOAT( "hud_reverseweapons" ) == 0 )
		NextWeapon();
	else
		PrevWeapon();
}

void CHudAmmo::NextWeapon(void)
{
	if ( CVAR_GET_FLOAT( "hud_fastswitch" ) )
	{
		// fast switch: straight to the next slot's weapon
		gpLastSel = m_pWeapon;

		int slot = 1;
		if ( m_pWeapon )
		{
			slot = m_pWeapon->iSlot + 1;
			if ( slot > 9 )
				slot = 1;
		}

		for ( int loop = 0; loop <= 1; loop++ )
		{
			for ( ; slot <= 9; slot++ )
			{
				WEAPON *p = gWR.GetWeaponSlot( slot, 0 );

				if ( p && !p->iIsCustomization && gWR.HasAmmo( p ) )
				{
					ServerCmd( p->szName );
					g_weaponselect = p->iId;
					return;
				}
			}

			slot = 0;
		}

		return;
	}

	if ( !gpMenuWeapon )
		gpMenuWeapon = m_pWeapon;

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

				if ( wsp && gWR.HasAmmo(wsp) && wsp->iIsCustomization != 1 )
				{
					PlaySound( "common/wpn_moveselect.wav", 1 );
					gpActiveSel = wsp;

					if ( CVAR_GET_FLOAT( "hud_autoswitch" ) )
					{
						ServerCmd( gpActiveSel->szName );
						g_weaponselect = gpActiveSel->iId;
					}
					return;
				}
			}

			pos = 0;
		}

		slot = 0;  // start looking from the first slot again
	}

	gpActiveSel = NULL;
}

void CHudAmmo::PrevWeapon(void)
{
	if ( CVAR_GET_FLOAT( "hud_fastswitch" ) )
	{
		// fast switch: straight to the previous slot's weapon
		gpLastSel = m_pWeapon;

		int slot = 1;
		if ( m_pWeapon )
		{
			slot = m_pWeapon->iSlot - 1;
			if ( slot < 0 )
				slot = 9;
		}

		for ( int loop = 0; loop <= 1; loop++ )
		{
			for ( ; slot >= 0; slot-- )
			{
				WEAPON *p = gWR.GetWeaponSlot( slot, 0 );

				if ( p && !p->iIsCustomization && gWR.HasAmmo( p ) )
				{
					ServerCmd( p->szName );
					g_weaponselect = p->iId;
					return;
				}
			}

			slot = 9;
		}

		return;
	}

	if ( !gpMenuWeapon )
		gpMenuWeapon = m_pWeapon;

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

				if ( wsp && gWR.HasAmmo(wsp) && wsp->iIsCustomization != 1 )
				{
					PlaySound( "common/wpn_moveselect.wav", 1 );
					gpActiveSel = wsp;

					if ( CVAR_GET_FLOAT( "hud_autoswitch" ) )
					{
						ServerCmd( gpActiveSel->szName );
						g_weaponselect = gpActiveSel->iId;
					}
					return;
				}
			}

			pos = MAX_WEAPON_POSITIONS-1;
		}
		
		slot = MAX_WEAPON_SLOTS-1;
	}

	gpActiveSel = NULL;
}

// Selects the previous item in the menu
// Gunman: zooms out the scope, steps through customization settings
void CHudAmmo::UserCmd_PrevWeapon(void)
{
	if ( gHUD.m_Tank.m_fInTank )
		return;

	if ( gHUD.m_Zoom.m_fZoomed )
	{
		gHUD.m_Zoom.ZoomOut();
		return;
	}

	if ( gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) )
		return;

	if ( m_pWeapon && !strcmp( m_pWeapon->szName, "weapon_aicore" ) )
		m_fCustomizing = 0;

	if ( m_fCustomizing )
	{
		m_fCustomPrev = 1;
		return;
	}

	if ( gpActiveSel && gpActiveSel != (WEAPON *)1 && gpActiveSel->iIsCustomization )
	{
		gWR.SelectSlot( 9, FALSE, 1 );
		return;
	}

	if ( CVAR_GET_FLOAT( "hud_reverseweapons" ) )
		NextWeapon();
	else
		PrevWeapon();
}



//-------------------------------------------------------------------------
// Drawing code
//-------------------------------------------------------------------------

int CHudAmmo::Draw(float flTime)
{
	int a, x, y, r, g, b;
	int AmmoWidth;

	if ( (gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL )) )
		return 1;

	// Draw the customization settings
	if ( m_fCustomizing )
		DrawCustomization();

	// Draw Weapon Menu
	DrawWList(flTime);

	// Draw ammo pickup history
	gHR.DrawAmmoHistory( flTime );

	if (!(m_iFlags & HUD_ACTIVE))
		return 0;

	if (!m_pWeapon)
		return 0;

	WEAPON *pw = m_pWeapon; // shorthand

	if ( gViewPort->m_pAmmoPanel->m_fActive )
		gViewPort->m_pAmmoPanel->SetAmmo( m_pWeapon->iAmmoType, m_pWeapon->iMax1 );

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
			
			x = ScreenWidth - (6 * AmmoWidth) - iIconWidth;
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, pw->iClip, r, g, b);

			int iBarWidth =  AmmoWidth/10;

			x += AmmoWidth/4;

			UnpackRGB(r,g,b, RGB_CYANISH);

			// draw the | bar
			FillRGBA(x, y, iBarWidth, gHUD.m_iFontHeight, r, g, b, a);

			x += iBarWidth + AmmoWidth/4 - 20;

			// GL Seems to need this
			ScaleColors(r, g, b, a );
			gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, gWR.CountAmmo(pw->iAmmoType), r, g, b);		
		}
		else
		{
			// SPR_Draw a bullets only line
			x = ScreenWidth - 4 * AmmoWidth - iIconWidth;
			gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, gWR.CountAmmo(pw->iAmmoType), r, g, b);
		}
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

	UnpackRGB(r, g, b, RGB_CYANISH);

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




//
// Draw Weapon Menu
// Gunman: slots 1-9 are a VGUI menu, with the selected slot's glow and its
// brackets drawn here; slot 10 lists the current weapon's customization items.
//
int CHudAmmo::DrawWList(float flTime)
{
	int r,g,b,x,y,i;

	if ( !gpActiveSel )
	{
		gViewPort->m_pWeaponMenu->setVisible( false );
		m_fMenuOpen = false;
		return 0;
	}

	if ( gpActiveSel == (WEAPON *)1 )
		giActiveSlot = -1;	// current slot has no weapons
	else 
		giActiveSlot = gpActiveSel->iSlot;

	int fCustom = FALSE;
	if ( giActiveSlot == 9 )
		fCustom = TRUE;


	// Ensure that there are available choices in the active slot
	if ( giActiveSlot > 0 )
	{
		if ( !gWR.GetFirstPos( giActiveSlot ) )
		{
			gpActiveSel = (WEAPON *)1;
			giActiveSlot = -1;
		}
	}

	if ( !fCustom )
	{
		if ( !gpActiveSel || gpActiveSel == (WEAPON *)1 )
			return 1;

		if ( !m_fMenuOpen )
		{
			gViewPort->ShowVGUIMenu( MENU_CLASS );
			gViewPort->m_pWeaponMenu->Update();
			gViewPort->m_pWeaponMenu->setVisible( true );
			m_fMenuOpen = true;
		}

		gViewPort->m_pWeaponMenu->setVisible( true );
		m_fMenuOpen = true;

		int iSlot = gpActiveSel->iSlot;
		int iRight, iStep;

		if ( ScreenWidth <= 640 )
		{
			iRight = 128;
			iStep = 30;
		}
		else
		{
			iRight = 256;
			iStep = 60;
		}

		// the selected slot glows, between brackets
		SPR_Set( gHUD.GetSprite( g_HUD_glow_knife + iSlot ), 0, 255, 0 );
		y = iStep * iSlot;
		SPR_DrawAdditive( 0, ScreenWidth - iRight, y, &gHUD.GetSpriteRect( g_HUD_glow_knife + gpActiveSel->iSlot ) );

		if ( ScreenWidth <= 640 )
		{
			SPR_Set( gHUD.GetSprite( g_HUD_glow_bracketleft ), 0, 255, 0 );
			SPR_DrawAdditive( m_iFrame, ScreenWidth - iRight + 13, y, &gHUD.GetSpriteRect( g_HUD_glow_bracketleft ) );
			SPR_Set( gHUD.GetSprite( g_HUD_glow_bracketright ), 0, 255, 0 );
			SPR_DrawAdditive( m_iFrame, ScreenWidth - 25, y, &gHUD.GetSpriteRect( g_HUD_glow_bracketright ) );
		}
		else
		{
			SPR_Set( gHUD.GetSprite( g_HUD_glow_bracketleft ), 0, 255, 0 );
			SPR_DrawAdditive( m_iFrame, ScreenWidth - iRight + 30, y, &gHUD.GetSpriteRect( g_HUD_glow_bracketleft ) );
			SPR_Set( gHUD.GetSprite( g_HUD_glow_bracketright ), 0, 255, 0 );
			SPR_DrawAdditive( m_iFrame, ScreenWidth - 60, y, &gHUD.GetSpriteRect( g_HUD_glow_bracketright ) );
		}

		if ( gEngfuncs.GetClientTime() > m_flNextFrame )
		{
			m_iFrame++;
			if ( m_iFrame > 4 )
				m_iFrame = 0;

			m_flNextFrame = gEngfuncs.GetClientTime() + 0.1;
		}

		// the slots without ammo get crossed out
		for ( i = 0, y = 0; i < 9; i++, y += iStep )
		{
			WEAPON *p = gWR.GetWeaponSlot( i, 0 );

			if ( p && !gWR.HasAmmo( p ) )
			{
				SPR_Set( gHUD.GetSprite( g_HUD_glow_pistol_no + i - 1 ), 255, 255, 255 );
				SPR_DrawAdditive( 0, ScreenWidth - iRight, y, &gHUD.GetSpriteRect( g_HUD_glow_pistol_no + i - 1 ) );
			}
		}

		return 1;
	}

	// Draw top line
	for ( i = 0; i < MAX_WEAPON_SLOTS; i++ )
	{
		int a;

		UnpackRGB(r,g,b, RGB_CYANISH);
	
		if ( giActiveSlot == i )
			a = 255;
		else
			a = 192;

		ScaleColors(r, g, b, 255);
		SPR_Set(gHUD.GetSprite(m_HUD_bucket0 + i), r, g, b );
	}

	x = 230; //!!!
	y = 55;

	// the weapons without customization items have nothing to list
	if ( m_pWeapon && gHUD.m_Ammo.m_pWeapon )
	{
		if ( !strcmp( gHUD.m_Ammo.m_pWeapon->szName, "weapon_beamgun" ) )
			return 1;
		if ( !strcmp( gHUD.m_Ammo.m_pWeapon->szName, "weapon_dml" ) )
			return 1;
		if ( !strcmp( gHUD.m_Ammo.m_pWeapon->szName, "weapon_gausspistol" ) )
			return 1;
		if ( !strcmp( gHUD.m_Ammo.m_pWeapon->szName, "weapon_dmlGrenade" ) )
			return 1;
		if ( !strcmp( gHUD.m_Ammo.m_pWeapon->szName, "weapon_shotgun" ) )
			return 1;
		if ( !strcmp( gHUD.m_Ammo.m_pWeapon->szName, "weapon_fists" ) )
			return 1;
		if ( !strcmp( gHUD.m_Ammo.m_pWeapon->szName, "weapon_minigun" ) )
			return 1;
		if ( !strcmp( gHUD.m_Ammo.m_pWeapon->szName, "weapon_SPchemicalgun" ) )
			return 1;
	}

	// Draw all of the buckets
	for (i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		// If this is the active slot, draw the bigger pictures,
		// otherwise just draw boxes
		if ( i == giActiveSlot )
		{
			WEAPON *p = gWR.GetFirstPos( i );

			for ( int iPos = 0; iPos < MAX_WEAPON_POSITIONS; iPos++ )
			{
				p = gWR.GetWeaponSlot( i, iPos );

				if ( !p || !p->iId )
					continue;

				UnpackRGB( r,g,b, RGB_CYANISH );
			
				// if active, then we must have ammo.

				if ( gpActiveSel == p )
				{
					if ( i == 9 )
					{
						if ( m_pWeapon && ( !strcmp( gpActiveSel->szName, "cust_1MinigunSpin" )
							|| !strcmp( gHUD.m_Ammo.m_pWeapon->szName, "weapon_fists" )
							|| !strcmp( gpActiveSel->szName, "cust_2MinigunCooled" ) ) )
						{
							// on/off items show their state
							if ( gpActiveSel->iCustomSetting == 1 )
							{
								SPR_Set( p->hActive, 0, 255, 255 );
								SPR_DrawAdditive( 0, x, y, &p->rcActive );
							}
							else
							{
								SPR_Set( p->hInactive, 0, 255, 255 );
								SPR_DrawAdditive( 0, x, y, &p->rcInactive );
							}

							int xArrow = x - 35;
							int yArrow = y + 15;
							if ( ScreenWidth <= 640 )
								yArrow = y;

							SPR_Set( gHUD.GetSprite( g_HUD_mechaArrow ), 255, 255, 255 );
							SPR_DrawAdditive( 0, xArrow, yArrow, &gHUD.GetSpriteRect( g_HUD_mechaArrow ) );
						}
						else
						{
							SPR_Set( p->hActive, 0, 255, 255 );
							SPR_DrawAdditive( 0, x, y, &p->rcActive );

							SPR_Set( gHUD.GetSprite( g_HUD_mechaArrow ), 255, 255, 255 );
							SPR_DrawAdditive( 0, x - 35, y - 8, &gHUD.GetSpriteRect( g_HUD_mechaArrow ) );
						}
					}
				}
				else
				{
					// Draw Weapon if Red if no ammo

					if ( gWR.HasAmmo(p) )
						ScaleColors(r, g, b, 192);
					else
					{
						UnpackRGB(r,g,b, RGB_REDISH);
						ScaleColors(r, g, b, 128);
					}

					if ( i == 9 )
					{
						SPR_Set( p->hInactive, r, g, b );
						SPR_DrawAdditive( 0, x, y, &p->rcInactive );
					}
				}

				// Draw Ammo Bar

				DrawAmmoBar(p, x + giABWidth/2, y, giABWidth, giABHeight);
				
				if ( ScreenWidth > 640 )
					y += 10;
				y += p->rcActive.bottom - p->rcActive.top + 9;
			}
		}
	}	

	return 1;

}

//
// Gunman: sends the selected customization item's setting to the server,
// as "cust_N<setting>"
//
void CHudAmmo::SendCustomization(void)
{
	if ( gpActiveSel->iIsCustomization )
	{
		char szCmd[] = "                             ";
		char *p;

		sprintf( szCmd, gpActiveSel->szName );

		for ( int i = 0; i < 30; i++ )
		{
			if ( szCmd[i] == '_' )
			{
				p = &szCmd[i + 2];
				break;
			}
		}

		sprintf( p, "%d", gpActiveSel->iCustomSetting );
		ConsolePrint( szCmd );
		ServerCmd( szCmd );
	}
}

//
// Gunman: draws the selected customization item's settings
//
void CHudAmmo::DrawCustomization(void)
{
	if ( !gpActiveSel || gpActiveSel == (WEAPON *)1 || !m_pWeapon )
		return;

	// the weapon changed: back to the list
	if ( strcmp( gpActiveSel->szCustomizes, m_pWeapon->szName ) )
	{
		if ( gViewPort && gViewPort->m_pCustomMenu )
			gViewPort->m_pCustomMenu->HideAll();

		gWR.SelectSlot( 9, FALSE, 1 );
	}

	if ( !gpActiveSel || gpActiveSel == (WEAPON *)1 )
		return;

	if ( gpActiveSel->iCustomSetting < 1 )
		gpActiveSel->iCustomSetting = 1;
	if ( gpActiveSel->iCustomSetting > gpActiveSel->iCustomSettings )
		gpActiveSel->iCustomSetting = gpActiveSel->iCustomSettings;

	if ( !strcmp( gpActiveSel->szCustomizes, "weapon_shotgun" ) )
	{
		DrawShotgunCustom();
		return;
	}

	if ( !strcmp( gpActiveSel->szCustomizes, "weapon_dml" ) )
	{
		DrawDmlCustom();
		return;
	}

	gViewPort->m_pCustomMenu->setVisible( false );

	if ( !strcmp( gpActiveSel->szCustomizes, "weapon_SPchemicalgun" ) )
	{
		DrawChemCustom();
		return;
	}

	if ( !strcmp( gpActiveSel->szCustomizes, "weapon_beamgun" ) )
	{
		DrawBeamCustom();
		return;
	}

	if ( !strcmp( gpActiveSel->szCustomizes, "weapon_gausspistol" ) )
	{
		DrawGaussCustom();
		return;
	}

	if ( !strcmp( gpActiveSel->szCustomizes, "weapon_dmlGrenade" ) )
	{
		DrawGrenadeCustom();
		return;
	}

	// the others get a slider
	int iSettings = gpActiveSel->iCustomSettings;

	if ( iSettings )
	{
		int xSlider = ( iSettings * 32 / iSettings ) * gpActiveSel->iCustomSetting + 6;
		for ( int i = 0; i < gpActiveSel->iCustomSettings; i++ )
		{
			SPR_Set( gHUD.GetSprite( g_HUD_chem_bar ), 0, 160, 0 );
			SPR_DrawAdditive( 0, i * 32 + 38, 180, &gHUD.GetSpriteRect( g_HUD_chem_bar ) );
		}

		SPR_Set( gHUD.GetSprite( g_HUD_chem_slider ), 0, 255, 255 );
		SPR_DrawAdditive( 0, xSlider, 180, &gHUD.GetSpriteRect( g_HUD_chem_slider ) );
	}
}

//
// the chemgun: a vial per chemical (acid, nuet, base), filled to its setting,
// and the pressure slider
//
void CHudAmmo::DrawChemCustom(void)
{
	int r, g, b;
	int x = 50;
	int y = 54;

	if ( ScreenWidth > 640 )
		x = 214;
	if ( ScreenWidth == 640 )
		x += 130;
	x += 195;
	if ( ScreenWidth > 800 )
		x += 150;
	if ( ScreenWidth > 1200 )
		x += 100;

	int iItem = -1;
	WEAPON *pSave = gpActiveSel;

	for ( int i = 24; i <= 26; i++ )	// WEAPON_CUST_ACID, _NUET, _BASE
	{
		gpActiveSel = gWR.GetWeapon( i );

		if ( pSave == gpActiveSel )
			iItem = 24 - i;	// sic: 0, -1, -2

		if ( !gpActiveSel )
			return;

		if ( gpActiveSel->iCustomSetting == 1 )
		{
			UnpackRGB( r, g, b, RGB_YELLOWISH );
			SPR_Set( gHUD.GetSprite( g_HUD_vial_el ), r, g, b );
			SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( g_HUD_vial_el ) );
		}
		else
		{
			if ( i == 26 )
				UnpackRGB( r, g, b, RGB_REDISH );
			else if ( i == 24 )
				UnpackRGB( r, g, b, RGB_GREENISH );
			else if ( i == 25 )
				UnpackRGB( r, g, b, RGB_BLUEISH );
			SPR_Set( gHUD.GetSprite( g_HUD_vial_fl ), r, g, b );
			SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( g_HUD_vial_fl ) );
		}

		if ( gpActiveSel->iCustomSetting <= 2 )
		{
			UnpackRGB( r, g, b, RGB_YELLOWISH );
			SPR_Set( gHUD.GetSprite( g_HUD_vial_ec ), r, g, b );
			SPR_DrawAdditive( 0, x + gHUD.GetSpriteRect( g_HUD_vial_ec ).right - gHUD.GetSpriteRect( g_HUD_vial_ec ).left, y, &gHUD.GetSpriteRect( g_HUD_vial_ec ) );
		}
		else
		{
			if ( i == 26 )
				UnpackRGB( r, g, b, RGB_REDISH );
			else if ( i == 24 )
				UnpackRGB( r, g, b, RGB_GREENISH );
			else if ( i == 25 )
				UnpackRGB( r, g, b, RGB_BLUEISH );
			SPR_Set( gHUD.GetSprite( g_HUD_vial_fc ), r, g, b );
			SPR_DrawAdditive( 0, x + gHUD.GetSpriteRect( g_HUD_vial_ec ).right - gHUD.GetSpriteRect( g_HUD_vial_ec ).left, y, &gHUD.GetSpriteRect( g_HUD_vial_fc ) );
		}

		if ( gpActiveSel->iCustomSetting <= 3 )
		{
			UnpackRGB( r, g, b, RGB_YELLOWISH );
			SPR_Set( gHUD.GetSprite( g_HUD_vial_ec ), r, g, b );
			SPR_DrawAdditive( 0, x + ( gHUD.GetSpriteRect( g_HUD_vial_ec ).right - gHUD.GetSpriteRect( g_HUD_vial_ec ).left ) * 2, y, &gHUD.GetSpriteRect( g_HUD_vial_ec ) );
		}
		else
		{
			if ( i == 26 )
				UnpackRGB( r, g, b, RGB_REDISH );
			else if ( i == 24 )
				UnpackRGB( r, g, b, RGB_GREENISH );
			else if ( i == 25 )
				UnpackRGB( r, g, b, RGB_BLUEISH );
			SPR_Set( gHUD.GetSprite( g_HUD_vial_fc ), r, g, b );
			SPR_DrawAdditive( 0, x + ( gHUD.GetSpriteRect( g_HUD_vial_ec ).right - gHUD.GetSpriteRect( g_HUD_vial_ec ).left ) * 2, y, &gHUD.GetSpriteRect( g_HUD_vial_fc ) );
		}

		if ( gpActiveSel->iCustomSetting == 5 )
		{
			if ( i == 26 )
				UnpackRGB( r, g, b, RGB_REDISH );
			else if ( i == 24 )
				UnpackRGB( r, g, b, RGB_GREENISH );
			else if ( i == 25 )
				UnpackRGB( r, g, b, RGB_BLUEISH );
			SPR_Set( gHUD.GetSprite( g_HUD_vial_fr ), r, g, b );
			SPR_DrawAdditive( 0, x + ( gHUD.GetSpriteRect( g_HUD_vial_ec ).right - gHUD.GetSpriteRect( g_HUD_vial_ec ).left ) * 3, y, &gHUD.GetSpriteRect( g_HUD_vial_fr ) );
		}
		else
		{
			UnpackRGB( r, g, b, RGB_YELLOWISH );
			SPR_Set( gHUD.GetSprite( g_HUD_vial_er ), r, g, b );
			SPR_DrawAdditive( 0, x + ( gHUD.GetSpriteRect( g_HUD_vial_ec ).right - gHUD.GetSpriteRect( g_HUD_vial_ec ).left ) * 3, y, &gHUD.GetSpriteRect( g_HUD_vial_er ) );
		}

		if ( ScreenWidth >= 640 )
			y += 10;

		int iGap = 20;
		if ( ScreenWidth < 640 )
			iGap = 10;
		y += iGap + 14;
	}

	// the pressure
	gpActiveSel = gWR.GetWeapon( 27 );	// WEAPON_CUST_PRESSURE
	if ( !gpActiveSel )
		return;

	if ( iItem == -1 )
		iItem = 3;
	gViewPort->m_pCustomMenu->ShowChemgun( iItem );

	int xbar = x;
	for ( int j = 0; j < 5; j++ )
	{
		SPR_Set( gHUD.GetSprite( g_HUD_chem_bar ), 255, 160, 0 );
		SPR_DrawAdditive( 0, xbar, y, &gHUD.GetSpriteRect( g_HUD_chem_bar ) );
		xbar += 32;
	}

	xbar = ( 160 / gpActiveSel->iCustomSettings ) * ( gpActiveSel->iCustomSetting - 1 ) + x;
	SPR_Set( gHUD.GetSprite( g_HUD_chem_slider ), 255, 160, 0 );
	SPR_DrawAdditive( 0, xbar, y, &gHUD.GetSpriteRect( g_HUD_chem_slider ) );

	gpActiveSel = pSave;

	if ( gpActiveSel )
	{
		switch ( gpActiveSel->iId )
		{
		case 24:	iItem = 0;	break;
		case 25:	iItem = 1;	break;
		case 26:	iItem = 2;	break;
		default:	iItem = 3;	break;
		}

		gViewPort->m_pCustomMenu->ShowChemgun( iItem );
	}
}

//
// the beamgun: frequency, current and voltage
//
void CHudAmmo::DrawBeamCustom(void)
{
	WEAPON *pSave = gpActiveSel;
	int iFrequency, iCurrent, iVoltage, iItem;
	int x = -( gHUD.GetSpriteRect( g_HUD_beamdials ).right - gHUD.GetSpriteRect( g_HUD_beamdials ).left ) * 3 / 2 + ScreenWidth / 2;

	for ( int i = 2; i <= 4; i++ )	// WEAPON_CUST_FREQUENCY, _CURRENT, _VOLTAGE
	{
		gpActiveSel = gWR.GetWeapon( i );

		if ( gpActiveSel )
		{
			if ( gpActiveSel == pSave )
				iItem = i - 2;

			switch ( i )
			{
			case 2:	iFrequency = gpActiveSel->iCustomSetting;	break;
			case 3:	iCurrent = gpActiveSel->iCustomSetting;		break;
			case 4:	iVoltage = gpActiveSel->iCustomSetting;		break;
			}

			x += gHUD.GetSpriteRect( g_HUD_beamdials ).right - gHUD.GetSpriteRect( g_HUD_beamdials ).left + 25;
		}
	}

	if ( gViewPort && gViewPort->m_pCustomMenu )
		gViewPort->m_pCustomMenu->ShowBeamgun( iFrequency, iCurrent, iVoltage, iItem );

	gpActiveSel = pSave;
}

//
// the DML: launch, flight path, detonation and payload
//
void CHudAmmo::DrawDmlCustom(void)
{
	WEAPON *pSave = gpActiveSel;
	int iLaunch, iFlightpath, iDetonate, iPayload, iItem;
	int y = 47;

	for ( int i = 9; i <= 12; i++ )	// WEAPON_CUST_LAUNCH ... _PAYLOAD
	{
		gpActiveSel = gWR.GetWeapon( i );

		if ( gpActiveSel )
		{
			switch ( i )
			{
			case 9:		iLaunch = gpActiveSel->iCustomSetting;		break;
			case 10:	iFlightpath = gpActiveSel->iCustomSetting;	break;
			case 11:	iDetonate = gpActiveSel->iCustomSetting;	break;
			case 12:	iPayload = gpActiveSel->iCustomSetting;		break;
			}

			if ( pSave == gpActiveSel )
				iItem = i;

			y += gpActiveSel->rcActive.top - gpActiveSel->rcActive.bottom + 65;
		}
	}

	gViewPort->m_pCustomMenu->ShowDml( iLaunch, iFlightpath, iDetonate, iPayload, iItem );

	gpActiveSel = pSave;
}

//
// the DML grenades: detonation and payload
//
void CHudAmmo::DrawGrenadeCustom(void)
{
	WEAPON *pSave = gpActiveSel;
	int iDetonate, iPayload, iItem;
	int y = 55;

	for ( int i = 5; i <= 6; i++ )	// WEAPON_CUST_GRENDETONATE, _GRENPAYLOAD
	{
		gpActiveSel = gWR.GetWeapon( i );

		if ( gpActiveSel )
		{
			switch ( i )
			{
			case 5:	iDetonate = gpActiveSel->iCustomSetting;	break;
			case 6:	iPayload = gpActiveSel->iCustomSetting;		break;
			}

			if ( pSave == gpActiveSel )
				iItem = i;

			y += gpActiveSel->rcActive.top - gpActiveSel->rcActive.bottom + 65;
		}
	}

	gViewPort->m_pCustomMenu->ShowGrenade( iDetonate, iPayload, iItem );

	gpActiveSel = pSave;
}

//
// the gauss pistol: pulse, charge, rapid and, with the scope, sniper
//
void CHudAmmo::DrawGaussCustom(void)
{
	int x = -( gHUD.GetSpriteRect( g_HUD_pulse ).right - gHUD.GetSpriteRect( g_HUD_pulse ).left ) * 3 / 2 + ScreenWidth / 2;

	gpActiveSel = gWR.GetWeapon( 31 );	// WEAPON_CUST_FIRETYPE

	WEAPON *pScope = gWR.GetWeaponSlot( 9, 19 );
	int iOffset;

	if ( gWR.HasAmmo( pScope ) )
		x -= ( gHUD.GetSpriteRect( g_HUD_sniper ).right - gHUD.GetSpriteRect( g_HUD_sniper ).left ) / 2;

	if ( gpActiveSel->iCustomSetting == 1 )
	{
		SPR_Set( gHUD.GetSprite( g_HUD_pulseAct ), 255, 255, 255 );
		SPR_DrawAdditive( 0, x, 35, &gHUD.GetSpriteRect( g_HUD_pulseAct ) );
	}
	else
	{
		SPR_Set( gHUD.GetSprite( g_HUD_pulse ), 255, 255, 255 );
		SPR_DrawAdditive( 0, x, 35, &gHUD.GetSpriteRect( g_HUD_pulse ) );
	}

	iOffset = ScreenWidth >= 640 ? 64 : 32;

	if ( gpActiveSel->iCustomSetting == 2 )
	{
		SPR_Set( gHUD.GetSprite( g_HUD_chargeAct ), 255, 255, 255 );
		SPR_DrawAdditive( 0, x + iOffset, 35, &gHUD.GetSpriteRect( g_HUD_chargeAct ) );
	}
	else
	{
		SPR_Set( gHUD.GetSprite( g_HUD_charge ), 255, 255, 255 );
		SPR_DrawAdditive( 0, x + iOffset, 35, &gHUD.GetSpriteRect( g_HUD_charge ) );
	}

	iOffset = ScreenWidth >= 640 ? 124 : 62;

	if ( gpActiveSel->iCustomSetting == 3 )
	{
		if ( gWR.HasAmmo( pScope ) )
		{
			SPR_Set( gHUD.GetSprite( g_HUD_rapidSnipeAct ), 255, 255, 255 );
			SPR_DrawAdditive( 0, x + iOffset, 35, &gHUD.GetSpriteRect( g_HUD_rapidSnipeAct ) );
		}
		else
		{
			SPR_Set( gHUD.GetSprite( g_HUD_rapidAct ), 255, 255, 255 );
			SPR_DrawAdditive( 0, x + iOffset, 35, &gHUD.GetSpriteRect( g_HUD_rapidAct ) );
		}
	}
	else
	{
		if ( gWR.HasAmmo( pScope ) )
		{
			SPR_Set( gHUD.GetSprite( g_HUD_rapidSnipe ), 255, 255, 255 );
			SPR_DrawAdditive( 0, x + iOffset, 35, &gHUD.GetSpriteRect( g_HUD_rapidSnipe ) );
		}
		else
		{
			SPR_Set( gHUD.GetSprite( g_HUD_rapid ), 255, 255, 255 );
			SPR_DrawAdditive( 0, x + iOffset, 35, &gHUD.GetSpriteRect( g_HUD_rapid ) );
		}
	}

	iOffset = ScreenWidth >= 640 ? 175 : 87;

	if ( gpActiveSel->iCustomSetting == 4 )
	{
		if ( gWR.HasAmmo( pScope ) )
		{
			SPR_Set( gHUD.GetSprite( g_HUD_sniperAct ), 255, 255, 255 );
			SPR_DrawAdditive( 0, x + iOffset, 35, &gHUD.GetSpriteRect( g_HUD_sniperAct ) );
		}
		else
		{
			// lost the scope: back to rapid fire
			gpActiveSel->iCustomSetting = 3;
			SendCustomization();
		}
	}
	else
	{
		if ( !gWR.HasAmmo( pScope ) )
			return;

		SPR_Set( gHUD.GetSprite( g_HUD_sniper ), 255, 255, 255 );
		SPR_DrawAdditive( 0, x + iOffset, 35, &gHUD.GetSpriteRect( g_HUD_sniper ) );
	}
}

//
// the shotgun: spread and shells
//
void CHudAmmo::DrawShotgunCustom(void)
{
	int iSpread;

	switch ( gWR.GetWeapon( 20 )->iCustomSetting )	// WEAPON_CUST_SPREAD
	{
	case 1:		iSpread = 1;	break;
	case 2:		iSpread = 2;	break;
	case 3:		iSpread = 3;	break;
	case 4:		iSpread = 4;	break;
	default:	iSpread = 3;	break;
	}

	if ( gpActiveSel == gWR.GetWeapon( 32 ) )	// WEAPON_CUST_SHELLS
		gViewPort->m_pCustomMenu->ShowShotgun( gWR.GetWeapon( 32 )->iCustomSetting, iSpread, 4 );
	else
		gViewPort->m_pCustomMenu->ShowShotgun( gWR.GetWeapon( 32 )->iCustomSetting, iSpread, 5 );
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
