//
// nvg.cpp
//
// implementation of CHudNVG class (name ours): the night vision overlay
// and the sniper scope
//

#include "hud.h"
#include "util.h"
#include "parsemsg.h"

DECLARE_MESSAGE( m_NVG, NVGToggle );
DECLARE_COMMAND( m_NVG, NVGAdjustUp );
DECLARE_COMMAND( m_NVG, NVGAdjustDown );

int CHudNVG :: Init( void )
{
	m_iOn = 0;
	m_iAlpha = 110;

	HOOK_MESSAGE( NVGToggle );
	HOOK_COMMAND( "+nvgadjust", NVGAdjustUp );
	HOOK_COMMAND( "-nvgadjust", NVGAdjustDown );

	m_iFlags |= HUD_ACTIVE;

	gHUD.AddHudElem( this );

	return 1;
}

void CHudNVG :: Reset( void )
{
	m_iOn = 0;
	m_iAlpha = 110;
}

int CHudNVG :: VidInit( void )
{
	int tl, t, tr, l, r, bl, b, br;		// names ours

	tl = gHUD.GetSpriteIndex( "top_left" );
	t = gHUD.GetSpriteIndex( "top" );
	tr = gHUD.GetSpriteIndex( "top_right" );
	l = gHUD.GetSpriteIndex( "left" );
	r = gHUD.GetSpriteIndex( "right" );
	bl = gHUD.GetSpriteIndex( "bottom_left" );
	b = gHUD.GetSpriteIndex( "bottom" );
	br = gHUD.GetSpriteIndex( "bottom_right" );

	m_prcScope[0][0] = &gHUD.GetSpriteRect( tl );
	m_prcScope[0][1] = &gHUD.GetSpriteRect( t );
	m_prcScope[0][2] = &gHUD.GetSpriteRect( tr );
	m_prcScope[0][3] = &gHUD.GetSpriteRect( l );
	m_prcScope[0][4] = &gHUD.GetSpriteRect( r );
	m_prcScope[0][5] = &gHUD.GetSpriteRect( bl );
	m_prcScope[0][6] = &gHUD.GetSpriteRect( b );
	m_prcScope[0][7] = &gHUD.GetSpriteRect( br );

	m_hScope[0] = gHUD.GetSprite( tl );
	m_hScope[1] = gHUD.GetSprite( t );
	m_hScope[2] = gHUD.GetSprite( tr );
	m_hScope[3] = gHUD.GetSprite( l );
	m_hScope[4] = gHUD.GetSprite( r );
	m_hScope[5] = gHUD.GetSprite( bl );
	m_hScope[6] = gHUD.GetSprite( b );
	m_hScope[7] = gHUD.GetSprite( br );

	int tl2, t2, tr2, l2, r2, bl2, b2, br2;

	tl2 = gHUD.GetSpriteIndex( "top_left2" );
	t2 = gHUD.GetSpriteIndex( "top2" );
	tr2 = gHUD.GetSpriteIndex( "top_right2" );
	l2 = gHUD.GetSpriteIndex( "left2" );
	r2 = gHUD.GetSpriteIndex( "right2" );
	bl2 = gHUD.GetSpriteIndex( "bottom_left2" );
	b2 = gHUD.GetSpriteIndex( "bottom2" );
	br2 = gHUD.GetSpriteIndex( "bottom_right2" );

	m_prcScope[1][0] = &gHUD.GetSpriteRect( tl2 );
	m_prcScope[1][1] = &gHUD.GetSpriteRect( t2 );
	m_prcScope[1][2] = &gHUD.GetSpriteRect( tr2 );
	m_prcScope[1][3] = &gHUD.GetSpriteRect( l2 );
	m_prcScope[1][4] = &gHUD.GetSpriteRect( r2 );
	m_prcScope[1][5] = &gHUD.GetSpriteRect( bl2 );
	m_prcScope[1][6] = &gHUD.GetSpriteRect( b2 );
	m_prcScope[1][7] = &gHUD.GetSpriteRect( br2 );

	m_hScope2[0] = gHUD.GetSprite( tl2 );
	m_hScope2[1] = gHUD.GetSprite( t2 );
	m_hScope2[2] = gHUD.GetSprite( tr2 );
	m_hScope2[3] = gHUD.GetSprite( l2 );
	m_hScope2[4] = gHUD.GetSprite( r2 );
	m_hScope2[5] = gHUD.GetSprite( bl2 );
	m_hScope2[6] = gHUD.GetSprite( b2 );
	m_hScope2[7] = gHUD.GetSprite( br2 );

	int tl3, t3, tr3, l3, r3, bl3, b3, br3;

	tl3 = gHUD.GetSpriteIndex( "top_left3" );
	t3 = gHUD.GetSpriteIndex( "top3" );
	tr3 = gHUD.GetSpriteIndex( "top_right3" );
	l3 = gHUD.GetSpriteIndex( "left3" );
	r3 = gHUD.GetSpriteIndex( "right3" );
	bl3 = gHUD.GetSpriteIndex( "bottom_left3" );
	b3 = gHUD.GetSpriteIndex( "bottom3" );
	br3 = gHUD.GetSpriteIndex( "bottom_right3" );

	m_prcScope[2][0] = &gHUD.GetSpriteRect( tl3 );
	m_prcScope[2][1] = &gHUD.GetSpriteRect( t3 );
	m_prcScope[2][2] = &gHUD.GetSpriteRect( tr3 );
	m_prcScope[2][3] = &gHUD.GetSpriteRect( l3 );
	m_prcScope[2][4] = &gHUD.GetSpriteRect( r3 );
	m_prcScope[2][5] = &gHUD.GetSpriteRect( bl3 );
	m_prcScope[2][6] = &gHUD.GetSpriteRect( b3 );
	m_prcScope[2][7] = &gHUD.GetSpriteRect( br3 );

	m_hScope3[0] = gHUD.GetSprite( tl3 );
	m_hScope3[1] = gHUD.GetSprite( t3 );
	m_hScope3[2] = gHUD.GetSprite( tr3 );
	m_hScope3[3] = gHUD.GetSprite( l3 );
	m_hScope3[4] = gHUD.GetSprite( r3 );
	m_hScope3[5] = gHUD.GetSprite( bl3 );
	m_hScope3[6] = gHUD.GetSprite( b3 );
	m_hScope3[7] = gHUD.GetSprite( br3 );

	int h = gHUD.GetSpriteIndex( "horizontal" );	// names ours
	int v = gHUD.GetSpriteIndex( "vertical" );

	m_prcHorizontal = &gHUD.GetSpriteRect( h );
	m_prcVertical = &gHUD.GetSpriteRect( v );

	m_hHorizontal = gHUD.GetSprite( h );
	m_hVertical = gHUD.GetSprite( v );

	return 1;
}

int CHudNVG :: MsgFunc_NVGToggle( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iOn = READ_BYTE();

	return 1;
}

void CHudNVG :: UserCmd_NVGAdjustUp( void )
{
	m_iAlpha += 10;
	if ( m_iAlpha > 220 )
		m_iAlpha = 220;
}

void CHudNVG :: UserCmd_NVGAdjustDown( void )
{
	m_iAlpha -= 10;
	if ( m_iAlpha < 30 )
		m_iAlpha = 30;
}

int CHudNVG :: Draw( float flTime )
{
	if ( m_iOn )
		FillRGBA( 0, 0, ScreenWidth, ScreenHeight, 50, 225, 50, m_iAlpha );

	// the sniper scope, zoomed in and from 640 wide
	if ( gHUD.m_iFOV < 45 && ScreenWidth >= 640 )
	{
		int cx = ScreenWidth / 2;	// names ours
		int cy = ScreenHeight / 2;
		int left, right, top, bottom;

		if ( ScreenWidth >= 1024 )
		{
			SPR_Set( m_hScope3[0], 255, 255, 255 );
			SPR_DrawHoles( 0, cx - 384, cy - 384, m_prcScope[2][0] );
			SPR_Set( m_hScope3[1], 255, 255, 255 );
			SPR_DrawHoles( 0, cx - 128, cy - 384, m_prcScope[2][1] );
			SPR_Set( m_hScope3[2], 255, 255, 255 );
			SPR_DrawHoles( 0, cx + 128, cy - 384, m_prcScope[2][2] );
			SPR_Set( m_hScope3[3], 255, 255, 255 );
			SPR_DrawHoles( 0, cx - 384, cy - 128, m_prcScope[2][3] );
			SPR_Set( m_hScope3[4], 255, 255, 255 );
			SPR_DrawHoles( 0, cx + 128, cy - 128, m_prcScope[2][4] );
			SPR_Set( m_hScope3[5], 255, 255, 255 );
			SPR_DrawHoles( 0, cx - 384, cy + 128, m_prcScope[2][5] );
			SPR_Set( m_hScope3[6], 255, 255, 255 );
			SPR_DrawHoles( 0, cx - 128, cy + 128, m_prcScope[2][6] );
			SPR_Set( m_hScope3[7], 255, 255, 255 );
			SPR_DrawHoles( 0, cx + 128, cy + 128, m_prcScope[2][7] );
		}
		else if ( ScreenWidth >= 800 )
		{
			SPR_Set( m_hScope2[0], 255, 255, 255 );
			SPR_DrawHoles( 0, cx - 384, cy - 300, m_prcScope[1][0] );
			SPR_Set( m_hScope2[1], 255, 255, 255 );
			SPR_DrawHoles( 0, cx - 128, cy - 300, m_prcScope[1][1] );
			SPR_Set( m_hScope2[2], 255, 255, 255 );
			SPR_DrawHoles( 0, cx + 128, cy - 300, m_prcScope[1][2] );
			SPR_Set( m_hScope2[3], 255, 255, 255 );
			SPR_DrawHoles( 0, cx - 384, cy - 128, m_prcScope[1][3] );
			SPR_Set( m_hScope2[4], 255, 255, 255 );
			SPR_DrawHoles( 0, cx + 128, cy - 128, m_prcScope[1][4] );
			SPR_Set( m_hScope2[5], 255, 255, 255 );
			SPR_DrawHoles( 0, cx - 384, cy + 128, m_prcScope[1][5] );
			SPR_Set( m_hScope2[6], 255, 255, 255 );
			SPR_DrawHoles( 0, cx - 128, cy + 128, m_prcScope[1][6] );
			SPR_Set( m_hScope2[7], 255, 255, 255 );
			SPR_DrawHoles( 0, cx + 128, cy + 128, m_prcScope[1][7] );
		}
		else
		{
			SPR_Set( m_hScope[0], 255, 255, 255 );
			SPR_DrawHoles( 0, cx - 320, cy - 240, m_prcScope[0][0] );
			SPR_Set( m_hScope[1], 255, 255, 255 );
			SPR_DrawHoles( 0, cx - 128, cy - 240, m_prcScope[0][1] );
			SPR_Set( m_hScope[2], 255, 255, 255 );
			SPR_DrawHoles( 0, cx + 128, cy - 240, m_prcScope[0][2] );
			SPR_Set( m_hScope[3], 255, 255, 255 );
			SPR_DrawHoles( 0, cx - 320, cy - 128, m_prcScope[0][3] );
			SPR_Set( m_hScope[4], 255, 255, 255 );
			SPR_DrawHoles( 0, cx + 128, cy - 128, m_prcScope[0][4] );
			SPR_Set( m_hScope[5], 255, 255, 255 );
			SPR_DrawHoles( 0, cx - 320, cy + 128, m_prcScope[0][5] );
			SPR_Set( m_hScope[6], 255, 255, 255 );
			SPR_DrawHoles( 0, cx - 128, cy + 128, m_prcScope[0][6] );
			SPR_Set( m_hScope[7], 255, 255, 255 );
			SPR_DrawHoles( 0, cx + 128, cy + 128, m_prcScope[0][7] );
		}

		// the black bands around the scope (the 1024 case is never reached)
		if ( ScreenWidth >= 800 )
		{
			left = cx - 384;
			right = cx + 384;
			top = cy - 300;
			bottom = cy + 300;
		}
		else if ( ScreenWidth >= 1024 )
		{
			left = cx - 384;
			right = cx + 384;
			top = cy - 384;
			bottom = cy + 384;
		}

		if ( ScreenWidth > 640 )
		{
			int nx = ScreenWidth / 256 + 1;		// names ours
			int ny = ScreenHeight / 256 + 1;
			int x, k;
			int y = top - 32;

			if ( ScreenHeight > 768 )
			{
				for ( ; y > 0; y -= 32 )
				{
					for ( k = 0; k < nx; k++ )
					{
						SPR_Set( m_hHorizontal, 255, 255, 255 );
						SPR_Draw( 0, k * 256, y, m_prcHorizontal );
					}
				}

				for ( k = 0; k < nx; k++ )
				{
					SPR_Set( m_hHorizontal, 255, 255, 255 );
					SPR_Draw( 0, k * 256, 0, m_prcHorizontal );
				}

				for ( y = bottom; y < ScreenHeight; y += 32 )
				{
					for ( k = 0; k < nx; k++ )
					{
						SPR_Set( m_hHorizontal, 255, 255, 255 );
						SPR_Draw( 0, k * 256, y, m_prcHorizontal );
					}
				}
			}

			for ( x = left - 40; x > 0; x -= 40 )
			{
				for ( k = 0; k < ny; k++ )
				{
					SPR_Set( m_hVertical, 255, 255, 255 );
					SPR_Draw( 0, x, k * 256, m_prcVertical );
				}
			}

			for ( k = 0; k < ny; k++ )
			{
				SPR_Set( m_hVertical, 255, 255, 255 );
				SPR_Draw( 0, 0, k * 256, m_prcVertical );
			}

			for ( x = right; x < ScreenWidth; x += 40 )
			{
				for ( k = 0; k < ny; k++ )
				{
					SPR_Set( m_hVertical, 255, 255, 255 );
					SPR_Draw( 0, x, k * 256, m_prcVertical );
				}
			}
		}
	}

	return 1;
}
