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
// vgui_HudWeaponStatus.h
//
// CWeaponStatusPanel: the history of weapons and items picked up, and the
// tank's gas can (STUB: being reconstructed)
//

#ifndef VGUI_HUDWEAPONSTATUS_H
#define VGUI_HUDWEAPONSTATUS_H

#define MAX_HISTORY_ICONS		19
#define MAX_HISTORY_ITEMS		10

typedef struct
{
	CImageLabel	*pLabel;
	float		flHideTime;
	int			iWeapon;
} historyitem_t;

class CWeaponStatusPanel : public CMenuPanel
{
public:
	BitmapTGA		*m_pIcons[MAX_HISTORY_ICONS];
	historyitem_t	m_Items[MAX_HISTORY_ITEMS];
	int				m_iNumShown;
	int				m_iNextItem;
	float			m_flHideTime;
	CImageLabel		*m_pGasCan;
	CImageLabel		*m_pEmptyGasCan;
	int				m_iGasCanShown;
	int				m_iMaxItems;
	int				m_iTall;
	int				m_iWeaponsShown;

public:
	CWeaponStatusPanel( int iTrans, int iRemoveMe, int x, int y, int wide, int tall );

	void ShowGasCan( void );
	void ShowEmptyGasCan( void );
	void HideGasCans( void );
	void AddToHistory( int iIndex, int iWeapon );

	virtual void paintBackground( void );
	virtual void Reset( void );
	virtual void Open( void );
	virtual void Initialize( void );
	virtual void Update( void );
};

#endif // VGUI_HUDWEAPONSTATUS_H
