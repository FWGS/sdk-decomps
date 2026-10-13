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
// the weapon customization menu (STUB: being reconstructed)
//

#ifndef VGUI_CUSTOMIZATIONMENU_H
#define VGUI_CUSTOMIZATIONMENU_H

char *GetConfigText( const char *pszName, char *pszText );

class CCustomizationMenu : public CMenuPanel
{
public:
	int			m_iUnused[62];
	TextPanel	*m_pTitles[6];
	CImageLabel	*m_pSlots[4][5];
	CImageLabel	*m_pArrows[2][4];
	TextPanel	*m_pLabels[4];
	TextPanel	*m_pItems[4][5];
	int			m_iState;

public:
	CCustomizationMenu( int iTrans, int iRemoveMe, int x, int y, int wide, int tall );

	void HideAll( void );

	virtual void paint( void );
	virtual void Reset( void );
	virtual void Open( void );
	virtual void Initialize( void );
	virtual void Update( void );
};

#endif // VGUI_CUSTOMIZATIONMENU_H
