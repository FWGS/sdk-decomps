//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: TFC Class Menu
//
// Gunman has no classes: the class menu is the column of weapons the player
// holds, along the right edge of the screen.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "VGUI_Font.h"
#include <VGUI_TextImage.h>

#include "hud.h"
#include "cl_util.h"
#include "camera.h"
#include "kbutton.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"
#include "camera.h"
#include "in_defs.h"
#include "parsemsg.h"

#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_ServerBrowser.h"
#include "ammohistory.h"
#include "effectlist.h"

//======================================================================================
// CLASS MENU: THE WEAPONS HELD
//======================================================================================
static char *sWeaponList[MAX_WEAPONLIST] =
{
	"weapon_fists",
	"weapon_gausspistol",
	"weapon_shotgun",
	"weapon_minigun",
	"weapon_beamgun",
	"weapon_dml",
	"weapon_dmlGrenade",
	"weapon_SPchemicalgun",
	"weapon_aicore",
};

// Creation
CClassMenuPanel::CClassMenuPanel(int iTrans, int iRemoveMe, int x,int y,int wide,int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
{
	char sz[256];
	int iSpacing, iX;

	m_iPos = 0;
	m_iWeapon = 0;

	if ( ScreenWidth <= 640 )
	{
		iSpacing = 30;
		iX = -15;
	}
	else
	{
		iSpacing = 60;
		iX = -35;
	}

	int iY = 0;
	for ( int i = 0; i < MAX_WEAPONLIST; i++ )
	{
		sprintf( sz, "%s%d", sWeaponList[i], 0 );

		m_pWeapons[i] = new CImageLabel( sz, iX, iY, 256, 64 );
		m_pWeapons[i]->setParent( this );
		m_pWeapons[i]->setVisible( true );

		iY += iSpacing;
	}
}

//======================================
// Show the weapons the player has, with ammo
void CClassMenuPanel::Update()
{
	char sz[256];

	for ( int i = 0; i < MAX_WEAPONLIST; i++ )
	{
		if ( !m_pWeapons[i] )
			continue;

		WEAPON *pWeapon = gWR.GetWeaponSlot( i, 0 );
		sprintf( sz, "%s0", sWeaponList[i] );	// sic: never used

		if ( pWeapon && gWR.HasAmmo( pWeapon ) )
			m_pWeapons[i]->setVisible( true );
		else
			m_pWeapons[i]->setVisible( false );
	}
}

bool CClassMenuPanel::SlotInput( int iSlot )
{
	return false;
}

//======================================
// Update the Class menu before opening it
void CClassMenuPanel::Open( void )
{
	Update();
	CMenuPanel::Open();
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void CClassMenuPanel::Initialize( void )
{
	// clears the effects the events started
	while ( g_Effects.m_iCount )
	{
		if ( g_Effects.m_pHead )
		{
			CEffectNode *pNode = g_Effects.m_pHead;
			g_Effects.m_iCount--;
			g_Effects.m_pHead = pNode->m_pNext;
			delete pNode;
		}
	}
}

void CClassMenuPanel::SetActiveInfo( int iInput )
{
}
