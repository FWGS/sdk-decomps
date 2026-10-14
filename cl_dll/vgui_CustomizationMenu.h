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
// vgui_CustomizationMenu.h
//
// the weapon customization menu: the choices of the weapon being customized,
// read from text/configtext.txt, with the current ones highlighted
//

#ifndef VGUI_CUSTOMIZATIONMENU_H
#define VGUI_CUSTOMIZATIONMENU_H

// looks up "#pszName" in text/configtext.txt and copies the line after it
// into pszText; returns pszText, or NULL when there's no such entry
char *GetConfigText( const char *pszName, char *pszText );

class CCustomizationMenu : public CMenuPanel
{
public:
	int			m_iUnused[62];		// never used

	// [0] the number of shells, [1] to [3] the spreads (rifle, shotgun,
	// riotgun), [4] and [5] the titles of shells and spread
	TextPanel	*m_pShotgun[6];
	TextPanel	*m_pDml[4][5];		// launch, flightpath, detonate, payload: title then choices
	TextPanel	*m_pGrenade[2][4];	// detonate, payload: title then choices
	TextPanel	*m_pChem[4];		// acid, neutralizer, base, pressure
	TextPanel	*m_pBeam[4][5];		// range, power, accuracy, effect: title then choices

	// 1 when created, 2 after the first frame; the second frame hides the menu
	int			m_iStartFrames;

public:
	CCustomizationMenu( int iTrans, int iRemoveMe, int x, int y, int wide, int tall );

	void ShowShotgun( int iShells, int iSpread, int iWeapon );
	void HideAll( void );
	void ShowDml( int iLaunch, int iFlightpath, int iDetonate, int iPayload, int iWeapon );
	void ShowBeamgun( int iFrequency, int iCurrent, int iVoltage, int iRow );
	void ShowGrenade( int iDetonate, int iPayload, int iWeapon );
	void ShowChemgun( int iSelected );

	virtual void paint( void );
	virtual void Reset( void );
	virtual void Open( void );
	virtual void Initialize( void );
	virtual void Update( void );
};

#endif // VGUI_CUSTOMIZATIONMENU_H
