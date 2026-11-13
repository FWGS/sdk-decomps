/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
// Health.cpp
//
// implementation of CHudHealth class
//

#include "STDIO.H"
#include "STDLIB.H"
#include "MATH.H"

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>


DECLARE_COMMAND(m_Health, TrackPlayer )
DECLARE_COMMAND(m_Health, ClearPlayers )
DECLARE_COMMAND(m_Health, DrawRadar )
DECLARE_COMMAND(m_Health, HideRadar )
DECLARE_MESSAGE(m_Health, Health )
DECLARE_MESSAGE(m_Health, Damage )
DECLARE_MESSAGE(m_Health, ScoreAttrib )
DECLARE_MESSAGE(m_Health, Radar )

#define PAIN_NAME "sprites/%d_pain.spr"
#define DAMAGE_NAME "sprites/%d_dmg.spr"

int giDmgHeight, giDmgWidth;

int giDmgFlags[NUM_DMG_TYPES] = 
{
	DMG_POISON,
	DMG_ACID,
	DMG_FREEZE|DMG_SLOWFREEZE,
	DMG_DROWN,
	DMG_BURN|DMG_SLOWBURN,
	DMG_NERVEGAS, 
	DMG_RADIATION,
	DMG_SHOCK,
	DMG_CALTROP,
	DMG_TRANQ,
	DMG_CONCUSS,
	DMG_HALLUC
};

int CHudHealth::Init(void)
{
	HOOK_MESSAGE(Health);
	HOOK_MESSAGE(Damage);
	HOOK_MESSAGE(Radar);
	HOOK_MESSAGE(ScoreAttrib);
	HOOK_COMMAND("trackplayer", TrackPlayer);
	HOOK_COMMAND("clearplayers", ClearPlayers);
	HOOK_COMMAND("drawradar", DrawRadar);
	HOOK_COMMAND("hideradar", HideRadar);
	m_iHealth = 100;
	m_fFade = 0;
	m_iFlags = 0;
	m_bitsDamage = 0;
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
	giDmgHeight = 0;
	giDmgWidth = 0;

	memset(m_dmg, 0, sizeof(DAMAGE_IMAGE) * NUM_DMG_TYPES);
	for ( int i = 0; i <= 32; i++ )
		m_bTrackArray[i] = false;	// one more than the array holds
	m_iPlayerTarget = 0;
	m_iPlayerNum = 0;
	m_bDrawRadar = true;

	gHUD.AddHudElem(this);
	return 1;
}

void CHudHealth::Reset( void )
{
	// make sure the pain compass is cleared when the player respawns
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;


	// force all the flashing damage icons to expire
	m_bitsDamage = 0;
	for ( int i = 0; i < NUM_DMG_TYPES; i++ )
	{
		m_dmg[i].fExpire = 0;
	}
}

int CHudHealth::VidInit(void)
{
	m_hSprite = 0;

	m_HUD_dmg_bio = gHUD.GetSpriteIndex( "dmg_bio" ) + 1;
	m_HUD_cross = gHUD.GetSpriteIndex( "cross" );

	giDmgHeight = gHUD.GetSpriteRect(m_HUD_dmg_bio).right - gHUD.GetSpriteRect(m_HUD_dmg_bio).left;
	giDmgWidth = gHUD.GetSpriteRect(m_HUD_dmg_bio).bottom - gHUD.GetSpriteRect(m_HUD_dmg_bio).top;

	int iRadar = gHUD.GetSpriteIndex( "radar" );	// names ours
	if ( ScreenWidth >= 640 )
	{
		m_prcRadar = &gHUD.GetSpriteRect( iRadar );
		m_prcRadar->left = 0;
		m_prcRadar->top = 0;
		m_prcRadar->right = 128;
		m_prcRadar->bottom = 128;
	}
	else
	{
		m_prcRadar = &gHUD.GetSpriteRect( iRadar );
		m_prcRadar->left = 0;
		m_prcRadar->top = 0;
		m_prcRadar->right = 64;
		m_prcRadar->bottom = 64;
	}
	m_hRadar = gHUD.GetSprite( iRadar );

	int iRadarBg = gHUD.GetSpriteIndex( "radarback" );
	m_prcRadarBg = &gHUD.GetSpriteRect( iRadarBg );
	m_hRadarBg = gHUD.GetSprite( iRadarBg );

	return 1;
}

int CHudHealth:: MsgFunc_Health(const char *pszName,  int iSize, void *pbuf )
{
	// TODO: update local health data
	BEGIN_READ( pbuf, iSize );
	int x = READ_BYTE();

	m_iFlags |= HUD_ACTIVE;

	// Only update the fade if we've changed health
	if (x != m_iHealth)
	{
		m_fFade = FADE_TIME;
		m_iHealth = x;
	}

	return 1;
}


int CHudHealth:: MsgFunc_Damage(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int armor = READ_BYTE();	// armor
	int damageTaken = READ_BYTE();	// health
	long bitsDamage = READ_LONG(); // damage bits

	vec3_t vecFrom;

	for ( int i = 0 ; i < 3 ; i++)
		vecFrom[i] = READ_COORD();

	UpdateTiles(gHUD.m_flTime, bitsDamage);

	// Actually took damage?
	if ( damageTaken > 0 || armor > 0 )
		CalcDamageDirection(vecFrom);

	return 1;
}


// Returns back a color from the
// Green <-> Yellow <-> Red ramp
void CHudHealth::GetPainColor( int &r, int &g, int &b )
{
	int iHealth = m_iHealth;

	if (iHealth > 25)
		iHealth -= 25;
	else if ( iHealth < 0 )
		iHealth = 0;
#if 0
	g = iHealth * 255 / 100;
	r = 255 - g;
	b = 0;
#else
	if (m_iHealth > 25)
	{
		UnpackRGB(r,g,b, RGB_YELLOWISH);
	}
	else
	{
		r = 250;
		g = 0;
		b = 0;
	}
#endif 
}

int CHudHealth::Draw(float flTime)
{
	int r, g, b;
	int a = 0, x, y;
	int HealthWidth;

//	if (m_iHealth <= 0)
//		return 1;

	GetAllPlayersInfo();

	if ( (gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH) || gEngfuncs.IsSpectateOnly() )
		return 1;

	if ( !gHUD.m_fPlayerDead && m_bDrawRadar == true )
		DrawRadar( flTime );

	if ( !m_hSprite )
		m_hSprite = LoadSprite(PAIN_NAME);
	
	// Has health changed? Flash the health #
	if (m_fFade)
	{
		m_fFade -= (gHUD.m_flTimeDelta * 20);
		if (m_fFade <= 0)
		{
			a = MIN_ALPHA;
			m_fFade = 0;
		}

		// Fade the health number back to dim

		a = MIN_ALPHA +  (m_fFade/FADE_TIME) * 128;

	}
	else
		a = MIN_ALPHA;

	// If health is getting low, make it bright red
	if (m_iHealth <= 15)
		a = 255;
		
	GetPainColor( r, g, b );
	ScaleColors(r, g, b, a );

	// Only draw health if we have the suit.
	if (gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)))
	{
		HealthWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).right - gHUD.GetSpriteRect(gHUD.m_HUD_number_0).left;
		int CrossWidth = gHUD.GetSpriteRect(m_HUD_cross).right - gHUD.GetSpriteRect(m_HUD_cross).left;

		y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
		x = CrossWidth /2;

		SPR_Set(gHUD.GetSprite(m_HUD_cross), r, g, b);
		SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_cross));

		x = CrossWidth + HealthWidth / 2;

		x = gHUD.DrawHudNumber(x, y, DHN_3DIGITS | DHN_DRAWZERO, m_iHealth, r, g, b);
	}

	DrawDamage(flTime);
	return DrawPain(flTime);
}

void CHudHealth::CalcDamageDirection(vec3_t vecFrom)
{
	vec3_t	forward, right, up;
	float	side, front;
	vec3_t vecOrigin, vecAngles;

	if (!vecFrom[0] && !vecFrom[1] && !vecFrom[2])
	{
		m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
		return;
	}


	memcpy(vecOrigin, gHUD.m_vecOrigin, sizeof(vec3_t));
	memcpy(vecAngles, gHUD.m_vecAngles, sizeof(vec3_t));


	VectorSubtract (vecFrom, vecOrigin, vecFrom);

	float flDistToTarget = vecFrom.Length();

	vecFrom = vecFrom.Normalize();
	AngleVectors (vecAngles, forward, right, up);

	front = DotProduct (vecFrom, right);
	side = DotProduct (vecFrom, forward);

	if (flDistToTarget <= 50)
	{
		m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 1;
	}
	else 
	{
		if (side > 0)
		{
			if (side > 0.3)
				m_fAttackFront = max(m_fAttackFront, side);
		}
		else
		{
			float f = fabs(side);
			if (f > 0.3)
				m_fAttackRear = max(m_fAttackRear, f);
		}

		if (front > 0)
		{
			if (front > 0.3)
				m_fAttackRight = max(m_fAttackRight, front);
		}
		else
		{
			float f = fabs(front);
			if (f > 0.3)
				m_fAttackLeft = max(m_fAttackLeft, f);
		}
	}
}

int CHudHealth::DrawPain(float flTime)
{
	if (!(m_fAttackFront || m_fAttackRear || m_fAttackLeft || m_fAttackRight))
		return 1;

	int r, g, b;
	int x, y, a, shade;

	// TODO:  get the shift value of the health
	a = 255;	// max brightness until then

	float fFade = gHUD.m_flTimeDelta * 2;
	
	// SPR_Draw top
	if (m_fAttackFront > 0.4)
	{
		GetPainColor(r,g,b);
		shade = a * max( m_fAttackFront, 0.5 );
		ScaleColors(r, g, b, shade);
		SPR_Set(m_hSprite, r, g, b );

		x = ScreenWidth/2 - SPR_Width(m_hSprite, 0)/2;
		y = ScreenHeight/2 - SPR_Height(m_hSprite,0) * 3;
		SPR_DrawAdditive(0, x, y, NULL);
		m_fAttackFront = max( 0, m_fAttackFront - fFade );
	} else
		m_fAttackFront = 0;

	if (m_fAttackRight > 0.4)
	{
		GetPainColor(r,g,b);
		shade = a * max( m_fAttackRight, 0.5 );
		ScaleColors(r, g, b, shade);
		SPR_Set(m_hSprite, r, g, b );

		x = ScreenWidth/2 + SPR_Width(m_hSprite, 1) * 2;
		y = ScreenHeight/2 - SPR_Height(m_hSprite,1)/2;
		SPR_DrawAdditive(1, x, y, NULL);
		m_fAttackRight = max( 0, m_fAttackRight - fFade );
	} else
		m_fAttackRight = 0;

	if (m_fAttackRear > 0.4)
	{
		GetPainColor(r,g,b);
		shade = a * max( m_fAttackRear, 0.5 );
		ScaleColors(r, g, b, shade);
		SPR_Set(m_hSprite, r, g, b );

		x = ScreenWidth/2 - SPR_Width(m_hSprite, 2)/2;
		y = ScreenHeight/2 + SPR_Height(m_hSprite,2) * 2;
		SPR_DrawAdditive(2, x, y, NULL);
		m_fAttackRear = max( 0, m_fAttackRear - fFade );
	} else
		m_fAttackRear = 0;

	if (m_fAttackLeft > 0.4)
	{
		GetPainColor(r,g,b);
		shade = a * max( m_fAttackLeft, 0.5 );
		ScaleColors(r, g, b, shade);
		SPR_Set(m_hSprite, r, g, b );

		x = ScreenWidth/2 - SPR_Width(m_hSprite, 3) * 3;
		y = ScreenHeight/2 - SPR_Height(m_hSprite,3)/2;
		SPR_DrawAdditive(3, x, y, NULL);

		m_fAttackLeft = max( 0, m_fAttackLeft - fFade );
	} else
		m_fAttackLeft = 0;

	return 1;
}

int CHudHealth::DrawDamage(float flTime)
{
	int r, g, b, a;
	DAMAGE_IMAGE *pdmg;

	if (!m_bitsDamage)
		return 1;

	UnpackRGB(r,g,b, RGB_YELLOWISH);
	
	a = (int)( fabs(sin(flTime*2)) * 256.0);

	ScaleColors(r, g, b, a);

	// Draw all the items
	for (int i = 0; i < NUM_DMG_TYPES; i++)
	{
		if (m_bitsDamage & giDmgFlags[i])
		{
			pdmg = &m_dmg[i];
			SPR_Set(gHUD.GetSprite(m_HUD_dmg_bio + i), r, g, b );
			SPR_DrawAdditive(0, pdmg->x, pdmg->y, &gHUD.GetSpriteRect(m_HUD_dmg_bio + i));
		}
	}


	// check for bits that should be expired
	for ( i = 0; i < NUM_DMG_TYPES; i++ )
	{
		DAMAGE_IMAGE *pdmg = &m_dmg[i];

		if ( m_bitsDamage & giDmgFlags[i] )
		{
			pdmg->fExpire = min( flTime + DMG_IMAGE_LIFE, pdmg->fExpire );

			if ( pdmg->fExpire <= flTime		// when the time has expired
				&& a < 40 )						// and the flash is at the low point of the cycle
			{
				pdmg->fExpire = 0;

				int y = pdmg->y;
				pdmg->x = pdmg->y = 0;

				// move everyone above down
				for (int j = 0; j < NUM_DMG_TYPES; j++)
				{
					pdmg = &m_dmg[j];
					if ((pdmg->y) && (pdmg->y < y))
						pdmg->y += giDmgHeight;

				}

				m_bitsDamage &= ~giDmgFlags[i];  // clear the bits
			}
		}
	}

	return 1;
}
 

void CHudHealth::UpdateTiles(float flTime, long bitsDamage)
{	
	DAMAGE_IMAGE *pdmg;

	// Which types are new?
	long bitsOn = ~m_bitsDamage & bitsDamage;
	
	for (int i = 0; i < NUM_DMG_TYPES; i++)
	{
		pdmg = &m_dmg[i];

		// Is this one already on?
		if (m_bitsDamage & giDmgFlags[i])
		{
			pdmg->fExpire = flTime + DMG_IMAGE_LIFE; // extend the duration
			if (!pdmg->fBaseline)
				pdmg->fBaseline = flTime;
		}

		// Are we just turning it on?
		if (bitsOn & giDmgFlags[i])
		{
			// put this one at the bottom
			pdmg->x = giDmgWidth/8;
			pdmg->y = ScreenHeight - giDmgHeight * 2;
			pdmg->fExpire=flTime + DMG_IMAGE_LIFE;
			
			// move everyone else up
			for (int j = 0; j < NUM_DMG_TYPES; j++)
			{
				if (j == i)
					continue;

				pdmg = &m_dmg[j];
				if (pdmg->y)
					pdmg->y -= giDmgHeight;

			}
			pdmg = &m_dmg[i];
		}	
	}	

	// damage bits are only turned on here;  they are turned off when the draw time has expired (in DrawDamage())
	m_bitsDamage |= bitsDamage;
}


//
//-----------------------------------------------------
// the round timer
//
DECLARE_MESSAGE(m_RoundTimer, RoundTime)

int CHudRoundTimer::Init( void )
{
	HOOK_MESSAGE(RoundTime);

	gHUD.AddHudElem(this);

	Reset();

	return 1;
}

int CHudRoundTimer::VidInit( void )
{
	m_HUD_timer = gHUD.GetSpriteIndex( "stopwatch" );

	return 1;
}

void CHudRoundTimer::Reset( void )
{
	m_iFlags |= HUD_ACTIVE;

	m_iUnused = 0;
	m_flEndTime = 0;
	m_flRoundTime = 0;
	m_flNextToggle = 0;
	m_bPanicColor = 0;
}

int CHudRoundTimer::Draw( float flTime )
{
	int r, g, b;
	int x, y;

	// a new round time has come in: count down from now
	if ( m_flRoundTime != 0 )
	{
		m_flEndTime = flTime + m_flRoundTime;
		m_flRoundTime = 0;
	}

	if ( (gHUD.m_iHideHUDDisplay & HIDEHUD_TIMER) || gEngfuncs.IsSpectateOnly() == 2 )
		return 1;

	int iTimeLeft = m_flEndTime - flTime;	// name ours

	if ( iTimeLeft <= 0 )
	{
		iTimeLeft = 0;
		m_bPanicColor = 1;
	}
	else if ( iTimeLeft <= 2 )
	{
		if ( flTime > m_flNextToggle )
		{
			m_flNextToggle = flTime + 0.05;
			m_bPanicColor = 1 - m_bPanicColor;
		}
	}
	else if ( iTimeLeft <= 5 )
	{
		if ( flTime > m_flNextToggle )
		{
			m_flNextToggle = flTime + 0.1;
			m_bPanicColor = 1 - m_bPanicColor;
		}
	}
	else if ( iTimeLeft <= 10 )
	{
		if ( flTime > m_flNextToggle )
		{
			m_flNextToggle = flTime + 0.2;
			m_bPanicColor = 1 - m_bPanicColor;
		}
	}
	else if ( iTimeLeft <= 20 )
	{
		if ( flTime > m_flNextToggle )
		{
			m_flNextToggle = flTime + 0.4;
			m_bPanicColor = 1 - m_bPanicColor;
		}
	}
	else if ( iTimeLeft <= 30 )
	{
		if ( flTime > m_flNextToggle )
		{
			m_flNextToggle = flTime + 0.8;
			m_bPanicColor = 1 - m_bPanicColor;
		}
	}

	int iMinutes = iTimeLeft / 60;	// name ours
	int iSeconds = iTimeLeft % 60;	// name ours

	if ( m_bPanicColor )
	{
		UnpackRGB( r, g, b, RGB_REDISH );
	}
	else
	{
		UnpackRGB( r, g, b, RGB_YELLOWISH );
		ScaleColors( r, g, b, MIN_ALPHA );
	}

	x = ( ScreenWidth - gHUD.GetSpriteWidth( gHUD.m_HUD_number_0 ) * ( ScreenWidth < 640 ? 5 : 4 ) ) / 2;

	// without the suit the timer goes to the top of the screen
	if ( !( gHUD.m_iWeaponBits & ( 1<<(WEAPON_SUIT) ) ) )
		y = gHUD.m_iFontHeight;
	else
		y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;

	// the stopwatch only when there is room for it
	if ( ScreenWidth >= 400 )
	{
		SPR_Set( gHUD.GetSprite( m_HUD_timer ), r, g, b );
		SPR_DrawAdditive( 0, x - gHUD.GetSpriteWidth( m_HUD_timer ), y, &gHUD.GetSpriteRect( m_HUD_timer ) );
	}

	x = gHUD.DrawHudNumber( x, y, DHN_2DIGITS | DHN_DRAWZERO, iMinutes, r, g, b );
	x = gHUD.DrawHudString( x, y + ( ScreenWidth < 640 ? 1 : 4 ), ScreenWidth - 1, ":", r, g, b );
	gHUD.DrawHudNumber( x, y, DHN_PADZERO | DHN_2DIGITS | DHN_DRAWZERO, iSeconds, r, g, b );

	return 1;
}

int CHudRoundTimer::MsgFunc_RoundTime( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_flRoundTime = READ_SHORT();
	m_bPanicColor = 0;
	m_flNextToggle = 0;

	return 1;
}


//
//-----------------------------------------------------
// the progress bar
//
DECLARE_MESSAGE(m_ProgressBar, BarTime)

int CHudProgressBar::Init( void )
{
	HOOK_MESSAGE(BarTime);

	gHUD.AddHudElem(this);

	Reset();

	return 1;
}

int CHudProgressBar::VidInit( void )
{
	return 1;
}

void CHudProgressBar::Reset( void )
{
	m_flEndTime = 0;
	m_flDuration = 0;
	m_iFlags |= HUD_ACTIVE;

	m_iWidth = ScreenWidth * 0.5f;
	m_iHeight = 10;
	m_x0 = m_iWidth / 2;
	m_y0 = ScreenHeight * 0.5f + 64;
	m_x1 = m_x0 + m_iWidth - 1;
	m_y1 = m_y0 + 9;
}

int CHudProgressBar::Draw( float flTime )
{
	int r, g, b;

	// a new bar has come in: it runs from now
	if ( m_flDuration != 0 )
	{
		m_flEndTime = flTime + m_flDuration;
		m_flStartTime = flTime;
		m_flDuration = 0;
	}

	if ( flTime > m_flEndTime )
		return 1;

	UnpackRGB( r, g, b, RGB_YELLOWISH );

	// the frame
	FillRGBA( m_x0, m_y0, 1, m_iHeight, r, g, b, 80 );
	FillRGBA( m_x1, m_y0, 1, m_iHeight, r, g, b, 80 );
	FillRGBA( m_x0 + 1, m_y0, m_iWidth - 2, 1, r, g, b, 80 );
	FillRGBA( m_x0 + 1, m_y1, m_iWidth - 2, 1, r, g, b, 80 );

	// the bar
	int iWidth = ( flTime - m_flStartTime ) * ( m_iWidth - 4 ) / ( m_flEndTime - m_flStartTime );	// name ours
	if ( iWidth > m_iWidth - 4 )
		iWidth = m_iWidth - 4;

	FillRGBA( m_x0 + 2, m_y0 + 2, iWidth, m_iHeight - 4, r, g, b, 160 );

	return 1;
}

int CHudProgressBar::MsgFunc_BarTime( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int iTime = READ_SHORT();	// name ours

	if ( iTime )
		m_flDuration = iTime;
	else
		m_flDuration = -1;

	return 1;
}


// the radar: team mates as dots around the player, top left (locals: names ours)
void CHudHealth :: DrawRadar( float flTime )
{
	Vector origin;
	origin = gHUD.m_vecOrigin;
	char szTeamName[20];	// sic: 17 to 20 give the image\x27s frame
	strcpy( szTeamName, g_PlayerExtraInfo[m_iPlayerNum].teamname );

	if ( g_PlayerExtraInfo[m_iPlayerNum].dead == true )
		return;

	SPR_Set( m_hRadar, 25, 75, 25 );
	SPR_DrawAdditive( 0, 0, 0, m_prcRadar );

	for ( int i = 0; i < 32; i++ )
	{
		if ( i == m_iPlayerNum || strcmp( szTeamName, g_PlayerExtraInfo[i].teamname ) || g_PlayerExtraInfo[i].dead )
			continue;

		float dx = g_PlayerExtraInfo[i].radar_pos.x - origin.x;
		float dy = g_PlayerExtraInfo[i].radar_pos.y - origin.y;
		float dz = g_PlayerExtraInfo[i].radar_pos.z - origin.z;

		// the bearing of the team mate, in degrees from 0 to 360
		float angle = atan( dy / dx );
		angle = angle * 180 / 3.141592654;
		if ( dx < 0 && dy >= 0 )
			angle += 180;
		else if ( dx < 0 && dy < 0 )
			angle += 180;
		else if ( dx >= 0 && dy < 0 )
			angle += 360;

		dy = sqrt( dx * dx + dy * dy ) * -1;	// the distance, negative

		// relative to the view, in radians: turn ( 0, distance ) by it
		angle = ( gHUD.m_vecAngles.y - angle ) * 3.141592654;
		angle = angle / 180;
		float x0 = 0, y0 = dy;	// names ours
		float x = x0 * cos( angle ) - y0 * sin( angle );
		float y = x0 * sin( angle ) + y0 * cos( angle );

		float range;
		int size, dot;
		if ( ScreenWidth < 640 )
		{
			range = 1024;
			size = 64;
			dot = 1;
		}
		else
		{
			range = 2048;
			size = 128;
			dot = 2;
		}

		if ( dy * -1 > range )
		{
			float k = dy * -1 / range;
			x = x / k;
			y = y / k;
		}

		int px = size / 2 + (int)( x / 32 );
		int py = size / 2 + (int)( y / 32 );

		if ( px < 0 || py < 0 || px > size || py > size )
			continue;

		int r, g, b;
		if ( !g_PlayerExtraInfo[i].has_c4 && !g_PlayerExtraInfo[i].is_vip )
		{
			if ( m_bTrackArray[i - 1] == true )
			{
				r = 185;
				g = 20;
				b = 20;
				dot *= 2;
			}
			else
			{
				r = 150;
				g = 75;
				b = 250;
			}
		}
		else
		{
			r = 250;
			g = 0;
			b = 0;
		}

		if ( dz < -128 )
		{
			float h = dz * -1;
			if ( h > 3096 )
				h = 3096;
			int len = (int)( h / 400 );
			FillRGBA( px, py, 1, len + dot * 2, r, g, b, 235 );
			FillRGBA( px - 2, py + len + dot * 2, 5, 1, r, g, b, 235 );
		}
		else if ( dz > 128 )
		{
			if ( dz > 3096 )
				dz = 3096;
			FillRGBA( px - 2, py, 5, 1, r, g, b, 235 );
			FillRGBA( px, py + 1, 1, (int)( dz / 400 ) + dot * 2 - 1, r, g, b, 235 );
		}
		else
		{
			FillRGBA( px, py, dot, dot, r, g, b, 235 );
		}

		if ( g_PlayerExtraInfo[i].flash_next != -1 && flTime > g_PlayerExtraInfo[i].flash_next && g_PlayerExtraInfo[i].flash_left > 0 )
		{
			g_PlayerExtraInfo[i].flash_next = flTime + 0.15;
			g_PlayerExtraInfo[i].flash_on = 1 - g_PlayerExtraInfo[i].flash_on;
			g_PlayerExtraInfo[i].flash_left--;
		}

		if ( g_PlayerExtraInfo[i].flash_on == 1 && g_PlayerExtraInfo[i].flash_left > 0 )
			FillRGBA( px - 3, py - 3, dot + 3, dot + 3, 230, 110, 25, 245 );
	}
}

void CHudHealth :: UserCmd_TrackPlayer( void )
{
	if ( m_iPlayerTarget >= 1 && m_iPlayerTarget <= 32 )
		m_bTrackArray[m_iPlayerTarget - 1] = true;
}

void CHudHealth :: UserCmd_ClearPlayers( void )
{
	for ( int i = 0; i < 32; i++ )
		m_bTrackArray[i] = false;
}

void CHudHealth :: UserCmd_DrawRadar( void )
{
	m_bDrawRadar = true;
}

void CHudHealth :: UserCmd_HideRadar( void )
{
	m_bDrawRadar = false;
}

// Message handler for Radar message
// accepts four values:
//		byte: client number
//		coord, coord, coord: the client's origin
int CHudHealth :: MsgFunc_Radar( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int cl = READ_BYTE();
	float x = READ_COORD();	// names ours
	float y = READ_COORD();
	float z = READ_COORD();

	g_PlayerExtraInfo[cl].radar_pos = Vector( x, y, z );

	return 1;
}

void CHudHealth :: GetAllPlayersInfo( void )
{
	for ( int i = 1; i < MAX_PLAYERS; i++ )
	{
		GetPlayerInfo( i, &g_PlayerInfoList[i] );

		if ( g_PlayerInfoList[i].thisplayer )
			m_iPlayerNum = i;  // !!!HACK: this should be initialized elsewhere... maybe gotten from the engine
	}
}

// Message handler for ScoreAttrib message
// accepts two values:
//		byte: client number
//		byte: flags (1: dead, 2: carries the bomb, 4: the VIP)
int CHudHealth :: MsgFunc_ScoreAttrib( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	short cl = READ_BYTE();
	short flags = READ_BYTE();	// name ours

	if ( cl > 0 && cl <= MAX_PLAYERS )
	{
		g_PlayerExtraInfo[cl].dead = flags & 1;
		g_PlayerExtraInfo[cl].has_c4 = ( flags & 2 ) ? 1 : 0;
		g_PlayerExtraInfo[cl].is_vip = ( flags & 4 ) ? 1 : 0;
	}

	return 1;
}
