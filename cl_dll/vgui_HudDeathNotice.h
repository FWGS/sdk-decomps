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
// vgui_HudDeathNotice.h
//
// the death notices as a VGUI panel: killer, weapon icon and victim, in
// lines down the top right corner of the screen
//

#ifndef VGUI_HUDDEATHNOTICE_H
#define VGUI_HUDDEATHNOTICE_H

#define MAX_DEATH_ICONS			10
#define MAX_DEATH_NOTICE_LINES	4
#define DEATH_NOTICE_TIME		5.0		// seconds a line stays up

typedef struct
{
	CImageLabel	*pIcon;
	float		flHideTime;
	TextPanel	*pKiller;
	TextPanel	*pVictim;
	int			iLine;			// line on screen, from the top
} deathnotice_t;

class CDeathNoticePanel : public CMenuPanel
{
public:
	BitmapTGA		*m_pIcons[MAX_DEATH_ICONS];
	deathnotice_t	m_Notices[MAX_DEATH_NOTICE_LINES];
	int				m_iNumShown;
	int				m_iNext;		// notice the next death goes into
	int				m_iLine;		// line the next death goes on
	int				m_iMaxLines;

public:
	CDeathNoticePanel( int iTrans, int iRemoveMe, int x, int y, int wide, int tall );

	void AddNotice( char *pszKilledWith, char *pszKiller, char *pszVictim );

	virtual void paintBackground( void );
	virtual void Reset( void );
	virtual void Open( void );
	virtual void Initialize( void );
	virtual void Update( void );
};

#endif // VGUI_HUDDEATHNOTICE_H
