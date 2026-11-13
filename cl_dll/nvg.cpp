//
// nvg.cpp
//
// implementation of CHudNVG class (name ours): the night vision overlay
// and the sniper scope
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

void EV_HLDM_CreateDLight( float *origin, float radius, int r, int g, int b, float life );	// name ours: ev_hldm.cpp

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

CHudNVG :: ~CHudNVG()
{
	delete m_prcScope[0][0];
	delete m_prcScope[0][1];
	delete m_prcScope[0][2];
	delete m_prcScope[0][3];
	delete m_prcScope[0][4];
	delete m_prcScope[0][5];
	delete m_prcScope[0][6];
	delete m_prcScope[0][7];
	delete m_prcScope[1][0];
	delete m_prcScope[1][1];
	delete m_prcScope[1][2];
	delete m_prcScope[1][3];
	delete m_prcScope[1][4];
	delete m_prcScope[1][5];
	delete m_prcScope[1][6];
	delete m_prcScope[1][7];
	delete m_prcScope[2][0];
	delete m_prcScope[2][1];
	delete m_prcScope[2][2];
	delete m_prcScope[2][3];
	delete m_prcScope[2][4];
	delete m_prcScope[2][5];
	delete m_prcScope[2][6];
	delete m_prcScope[2][7];
	delete m_prcHorizontal;
	delete m_prcVertical;
}

int CHudNVG :: VidInit( void )
{
	int tl, t, tr, l, r, bl, b, br;		// names ours

	m_prcScope[0][0] = new wrect_t;
	m_prcScope[0][1] = new wrect_t;
	m_prcScope[0][2] = new wrect_t;
	m_prcScope[0][3] = new wrect_t;
	m_prcScope[0][4] = new wrect_t;
	m_prcScope[0][5] = new wrect_t;
	m_prcScope[0][6] = new wrect_t;
	m_prcScope[0][7] = new wrect_t;
	m_prcScope[1][0] = new wrect_t;
	m_prcScope[1][1] = new wrect_t;
	m_prcScope[1][2] = new wrect_t;
	m_prcScope[1][3] = new wrect_t;
	m_prcScope[1][4] = new wrect_t;
	m_prcScope[1][5] = new wrect_t;
	m_prcScope[1][6] = new wrect_t;
	m_prcScope[1][7] = new wrect_t;
	m_prcScope[2][0] = new wrect_t;
	m_prcScope[2][1] = new wrect_t;
	m_prcScope[2][2] = new wrect_t;
	m_prcScope[2][3] = new wrect_t;
	m_prcScope[2][4] = new wrect_t;
	m_prcScope[2][5] = new wrect_t;
	m_prcScope[2][6] = new wrect_t;
	m_prcScope[2][7] = new wrect_t;
	m_prcHorizontal = new wrect_t;
	m_prcVertical = new wrect_t;

	tl = gHUD.GetSpriteIndex( "top_left" );
	t = gHUD.GetSpriteIndex( "top" );
	tr = gHUD.GetSpriteIndex( "top_right" );
	l = gHUD.GetSpriteIndex( "left" );
	r = gHUD.GetSpriteIndex( "right" );
	bl = gHUD.GetSpriteIndex( "bottom_left" );
	b = gHUD.GetSpriteIndex( "bottom" );
	br = gHUD.GetSpriteIndex( "bottom_right" );

	m_prcScope[0][0]->left = 0;
	m_prcScope[0][0]->top = 0;
	m_prcScope[0][0]->right = 192;
	m_prcScope[0][0]->bottom = 112;
	m_prcScope[0][1]->left = 0;
	m_prcScope[0][1]->top = 0;
	m_prcScope[0][1]->right = 256;
	m_prcScope[0][1]->bottom = 112;
	m_prcScope[0][2]->left = 0;
	m_prcScope[0][2]->top = 0;
	m_prcScope[0][2]->right = 192;
	m_prcScope[0][2]->bottom = 112;
	m_prcScope[0][3]->left = 0;
	m_prcScope[0][3]->top = 0;
	m_prcScope[0][3]->right = 192;
	m_prcScope[0][3]->bottom = 256;
	m_prcScope[0][4]->left = 0;
	m_prcScope[0][4]->top = 0;
	m_prcScope[0][4]->right = 192;
	m_prcScope[0][4]->bottom = 256;
	m_prcScope[0][5]->left = 0;
	m_prcScope[0][5]->top = 0;
	m_prcScope[0][5]->right = 192;
	m_prcScope[0][5]->bottom = 112;
	m_prcScope[0][6]->left = 0;
	m_prcScope[0][6]->top = 0;
	m_prcScope[0][6]->right = 256;
	m_prcScope[0][6]->bottom = 112;
	m_prcScope[0][7]->left = 0;
	m_prcScope[0][7]->top = 0;
	m_prcScope[0][7]->right = 192;
	m_prcScope[0][7]->bottom = 112;

	m_hScope[0] = SPR_Load( "sprites/top_left.spr" );
	m_hScope[1] = SPR_Load( "sprites/top.spr" );
	m_hScope[2] = SPR_Load( "sprites/top_right.spr" );
	m_hScope[3] = SPR_Load( "sprites/left.spr" );
	m_hScope[4] = SPR_Load( "sprites/right.spr" );
	m_hScope[5] = SPR_Load( "sprites/bottom_left.spr" );
	m_hScope[6] = SPR_Load( "sprites/bottom.spr" );
	m_hScope[7] = SPR_Load( "sprites/bottom_right.spr" );

	int tl2, t2, tr2, l2, r2, bl2, b2, br2;

	tl2 = gHUD.GetSpriteIndex( "top_left2" );
	t2 = gHUD.GetSpriteIndex( "top2" );
	tr2 = gHUD.GetSpriteIndex( "top_right2" );
	l2 = gHUD.GetSpriteIndex( "left2" );
	r2 = gHUD.GetSpriteIndex( "right2" );
	bl2 = gHUD.GetSpriteIndex( "bottom_left2" );
	b2 = gHUD.GetSpriteIndex( "bottom2" );
	br2 = gHUD.GetSpriteIndex( "bottom_right2" );

	m_prcScope[1][0]->left = 0;
	m_prcScope[1][0]->top = 0;
	m_prcScope[1][0]->right = 256;
	m_prcScope[1][0]->bottom = 172;
	m_prcScope[1][1]->left = 0;
	m_prcScope[1][1]->top = 0;
	m_prcScope[1][1]->right = 256;
	m_prcScope[1][1]->bottom = 172;
	m_prcScope[1][2]->left = 0;
	m_prcScope[1][2]->top = 0;
	m_prcScope[1][2]->right = 256;
	m_prcScope[1][2]->bottom = 172;
	m_prcScope[1][3]->left = 0;
	m_prcScope[1][3]->top = 0;
	m_prcScope[1][3]->right = 256;
	m_prcScope[1][3]->bottom = 256;
	m_prcScope[1][4]->left = 0;
	m_prcScope[1][4]->top = 0;
	m_prcScope[1][4]->right = 256;
	m_prcScope[1][4]->bottom = 256;
	m_prcScope[1][5]->left = 0;
	m_prcScope[1][5]->top = 0;
	m_prcScope[1][5]->right = 256;
	m_prcScope[1][5]->bottom = 172;
	m_prcScope[1][6]->left = 0;
	m_prcScope[1][6]->top = 0;
	m_prcScope[1][6]->right = 256;
	m_prcScope[1][6]->bottom = 172;
	m_prcScope[1][7]->left = 0;
	m_prcScope[1][7]->top = 0;
	m_prcScope[1][7]->right = 256;
	m_prcScope[1][7]->bottom = 172;

	m_hScope2[0] = SPR_Load( "sprites/top_left2.spr" );
	m_hScope2[1] = SPR_Load( "sprites/top2.spr" );
	m_hScope2[2] = SPR_Load( "sprites/top_right2.spr" );
	m_hScope2[3] = SPR_Load( "sprites/left2.spr" );
	m_hScope2[4] = SPR_Load( "sprites/right2.spr" );
	m_hScope2[5] = SPR_Load( "sprites/bottom_left2.spr" );
	m_hScope2[6] = SPR_Load( "sprites/bottom2.spr" );
	m_hScope2[7] = SPR_Load( "sprites/bottom_right2.spr" );

	int tl3, t3, tr3, l3, r3, bl3, b3, br3;

	tl3 = gHUD.GetSpriteIndex( "top_left3" );
	t3 = gHUD.GetSpriteIndex( "top3" );
	tr3 = gHUD.GetSpriteIndex( "top_right3" );
	l3 = gHUD.GetSpriteIndex( "left3" );
	r3 = gHUD.GetSpriteIndex( "right3" );
	bl3 = gHUD.GetSpriteIndex( "bottom_left3" );
	b3 = gHUD.GetSpriteIndex( "bottom3" );
	br3 = gHUD.GetSpriteIndex( "bottom_right3" );

	m_prcScope[2][0]->left = 0;
	m_prcScope[2][0]->top = 0;
	m_prcScope[2][0]->right = 256;
	m_prcScope[2][0]->bottom = 256;
	m_prcScope[2][1]->left = 0;
	m_prcScope[2][1]->top = 0;
	m_prcScope[2][1]->right = 256;
	m_prcScope[2][1]->bottom = 256;
	m_prcScope[2][2]->left = 0;
	m_prcScope[2][2]->top = 0;
	m_prcScope[2][2]->right = 256;
	m_prcScope[2][2]->bottom = 256;
	m_prcScope[2][3]->left = 0;
	m_prcScope[2][3]->top = 0;
	m_prcScope[2][3]->right = 256;
	m_prcScope[2][3]->bottom = 256;
	m_prcScope[2][4]->left = 0;
	m_prcScope[2][4]->top = 0;
	m_prcScope[2][4]->right = 256;
	m_prcScope[2][4]->bottom = 256;
	m_prcScope[2][5]->left = 0;
	m_prcScope[2][5]->top = 0;
	m_prcScope[2][5]->right = 256;
	m_prcScope[2][5]->bottom = 256;
	m_prcScope[2][6]->left = 0;
	m_prcScope[2][6]->top = 0;
	m_prcScope[2][6]->right = 256;
	m_prcScope[2][6]->bottom = 256;
	m_prcScope[2][7]->left = 0;
	m_prcScope[2][7]->top = 0;
	m_prcScope[2][7]->right = 256;
	m_prcScope[2][7]->bottom = 256;

	m_hScope3[0] = SPR_Load( "sprites/top_left3.spr" );
	m_hScope3[1] = SPR_Load( "sprites/top3.spr" );
	m_hScope3[2] = SPR_Load( "sprites/top_right3.spr" );
	m_hScope3[3] = SPR_Load( "sprites/left3.spr" );
	m_hScope3[4] = SPR_Load( "sprites/right3.spr" );
	m_hScope3[5] = SPR_Load( "sprites/bottom_left3.spr" );
	m_hScope3[6] = SPR_Load( "sprites/bottom3.spr" );
	m_hScope3[7] = SPR_Load( "sprites/bottom_right3.spr" );

	int h = gHUD.GetSpriteIndex( "horizontal" );	// names ours
	int v = gHUD.GetSpriteIndex( "vertical" );

	m_prcHorizontal->left = 0;
	m_prcHorizontal->top = 0;
	m_prcHorizontal->right = 256;
	m_prcHorizontal->bottom = 32;
	m_prcVertical->left = 0;
	m_prcVertical->top = 0;
	m_prcVertical->right = 40;
	m_prcVertical->bottom = 256;

	m_hHorizontal = SPR_Load( "sprites/horizontal.spr" );
	m_hVertical = SPR_Load( "sprites/vertical.spr" );

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
	{
		FillRGBA( 0, 0, ScreenWidth, ScreenHeight, 50, 225, 50, m_iAlpha );
		int iRadius = gEngfuncs.pfnRandomLong( 0, 50 ) + 750;	// name ours
		EV_HLDM_CreateDLight( gHUD.m_vecOrigin, iRadius, 1, 20, 1, 0.1 );
	}

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

		// the black bands around the scope
		if ( ScreenWidth >= 1024 )
		{
			left = cx - 384;
			right = cx + 384;
			top = cy - 384;
			bottom = cy + 384;
		}
		else if ( ScreenWidth >= 800 )
		{
			left = cx - 384;
			right = cx + 384;
			top = cy - 300;
			bottom = cy + 300;
		}

		if ( ScreenWidth > 640 )
		{
			int nx = ScreenWidth / 256 + 1;		// names ours
			int ny = ScreenHeight / 256 + 1;
			int x, k;
			int y = top - 32;

			if ( ScreenWidth > 1024 )
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
