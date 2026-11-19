//
// ident.cpp
//
// implementation of CHudIdent class: the target identification display
// (file and class body ours; the class name is retail's RTTI)
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_Ident, IdentUpdate )

int CHudIdent::Init(void)
{
	HOOK_MESSAGE(IdentUpdate);

	gHUD.AddHudElem(this);

	Reset();

	return 1;
}

int CHudIdent::VidInit(void)
{
	Reset();

	return 1;
}

void CHudIdent::Reset(void)
{
	m_iFlags &= ~HUD_ACTIVE;
	m_flHideTime = 0;
}

int CHudIdent::Draw(float flTime)
{
	if ( m_flHideTime > 0 && m_flHideTime < flTime )
	{
		m_iFlags &= ~HUD_ACTIVE;
		return 1;
	}

	char *szRelation[4];
	char szText[256];

	szRelation[0] = "Friendly";
	szRelation[1] = "Neutral";
	szRelation[2] = "Target";
	szRelation[3] = "Hostile";

	if ( m_iArmorType > 0 )
	{
		char *szArmor;

		if ( m_iArmorType == 2 )
			szArmor = "Shield";
		else
			szArmor = "Armor";

		if ( m_iPower > 0 )
			sprintf( szText, "Name: %s\nHealth: %i\n%s: %i\nClass: %s\nPower level: %d\n", m_szName, m_iHealth, szArmor, m_iArmor, szRelation[m_iRelation], m_iPower );
		else if ( m_iPower != 0 )
			sprintf( szText, "Name: %s\nHealth: %i\n%s: %i\nClass: %s\nPower level: ???\n", m_szName, m_iHealth, szArmor, m_iArmor, szRelation[m_iRelation] );
		else
			sprintf( szText, "Name: %s\nHealth: %i\n%s: %i\nClass: %s\n", m_szName, m_iHealth, szArmor, m_iArmor, szRelation[m_iRelation] );
	}
	else
	{
		if ( m_iPower > 0 )
			sprintf( szText, "Name: %s\nHealth: %i\nClass: %s\nPower level: %d\n", m_szName, m_iHealth, szRelation[m_iRelation], m_iPower );
		else if ( m_iPower != 0 )
			sprintf( szText, "Name: %s\nHealth: %i\nClass: %s\nPower level: ???\n", m_szName, m_iHealth, szRelation[m_iRelation] );
		else
			sprintf( szText, "Name: %s\nHealth: %i\nClass: %s\n", m_szName, m_iHealth, szRelation[m_iRelation] );
	}

	float x = ScreenWidth * 0.05;
	float y = ScreenHeight * 0.75;
	Vector color( 255, 0, 0 );

	if ( m_iRelation == 0 )
		color = Vector( 0, 255, 0 );
	else if ( m_iRelation == 1 )
		color = Vector( 0, 0, 255 );

	int len = strlen( szText );

	for ( int i = 0; i < len && szText[i]; i++ )
	{
		if ( szText[i] == '\n' )
		{
			y += gHUD.m_scrinfo.iCharHeight;
			x = ScreenWidth * 0.05;
		}
		else
		{
			int next = x + gHUD.m_scrinfo.charWidths[ szText[i] ];

			if ( x >= 0 && y >= 0 && next <= ScreenWidth )
				TextMessageDrawChar( x, y, szText[i], color.x, color.y, color.z );

			x = next;
		}
	}

	return 1;
}

int CHudIdent::MsgFunc_IdentUpdate(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int iFlags = READ_BYTE();

	// 255: the target is lost, the display stays for two more seconds
	if ( iFlags == 255 )
	{
		m_flHideTime = gEngfuncs.GetClientTime() + 2;
		return 1;
	}

	// an update of the shown target
	if ( iFlags & 0x70 )
	{
		if ( iFlags & 0x10 )
			m_iHealth = READ_SHORT();
		if ( iFlags & 0x20 )
			m_iArmor = READ_SHORT();
		if ( iFlags & 0x40 )
			m_iPower = READ_LONG();
		return 1;
	}

	// a new target
	m_iFlags |= HUD_ACTIVE;
	m_flHideTime = 0;
	m_iHealth = READ_SHORT();
	m_iArmorType = iFlags >> 2;
	m_iRelation = iFlags & 3;
	if ( m_iArmorType )
		m_iArmor = READ_SHORT();
	m_iPower = READ_LONG();

	char *pszTarget = READ_STRING();
	if ( pszTarget[0] )
		strcpy( m_szName, pszTarget );

	return 1;
}
