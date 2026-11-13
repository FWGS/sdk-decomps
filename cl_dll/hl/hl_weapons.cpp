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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

#include "usercmd.h"
#include "entity_state.h"
#include "demo_api.h"
#include "pm_defs.h"
#include "event_api.h"
#include "r_efx.h"

#include "cl_entity.h"

#include "../hud_iface.h"
#include "../com_weapons.h"
#include "../demo.h"

extern globalvars_t *gpGlobals;

struct cl_entity_s *GetViewEntity( void );

// Pool of client side entities/entvars_t
static entvars_t	ev[ 32 ];
static int			num_ents = 0;

// Counter-Strike 1.1: set from iuser3 bit 0 with m_bCanShoot (name and place Counter-Strike 1.6's, line 49)
int g_iFreezeTimeOver;

// The entity we'll use to represent the local client
static CBasePlayer	player;

// Local version of game .dll global variables ( time, etc. )
static globalvars_t	Globals; 

static CBasePlayerWeapon *g_pWpns[ 32 ];

// Counter-Strike: SDK 2.1 has these four above Game_GetSequence; the order of the dynamic
// initialisers puts the two vectors here, where Counter-Strike 1.6 has them (lines 66, 67)
static int g_rseq = 0, g_gaitseq = 0;
static vec3_t g_clorg, g_clang;

// Counter-Strike's weapon placeholder entities (names ours)
CUSP	g_USP;
CELITE	g_ELITE;
CUMP45	g_UMP45;
CFiveSeven	g_FIVESEVEN;
CMP5N	g_MP5N;
CSG552	g_SG552;
CAK47	g_AK47;
CAUG	g_AUG;
CAWP	g_AWP;
CC4	g_C4;
CDEAGLE	g_DEAGLE;
CFlashbang	g_Flashbang;
CSmokeGrenade	g_SmokeGrenade;
CG3SG1	g_G3SG1;
CSG550	g_SG550;
CGLOCK18	g_GLOCK18;
CHEGrenade	g_HEGrenade;
CKnife	g_Knife;
CM249	g_M249;
CM3	g_M3;
CM4A1	g_M4A1;
CMAC10	g_MAC10;
CP228	g_P228;
CP90	g_P90;
CSCOUT	g_SCOUT;
CTMP	g_TMP;
CXM1014	g_XM1014;

/*
======================
AlertMessage

Print debug messages to console
======================
*/
void AlertMessage( ALERT_TYPE atype, char *szFmt, ... )
{
	va_list		argptr;
	static char	string[1024];
	
	va_start (argptr, szFmt);
	vsprintf (string, szFmt,argptr);
	va_end (argptr);

	gEngfuncs.Con_Printf( "cl:  " );
	gEngfuncs.Con_Printf( string );
}

/*
=====================
HUD_PrepEntity

Links the raw entity to an entvars_s holder.  If a player is passed in as the owner, then
we set up the m_pPlayer field.
=====================
*/
void HUD_PrepEntity( CBaseEntity *pEntity, CBasePlayer *pWeaponOwner )
{
	memset( &ev[ num_ents ], 0, sizeof( entvars_t ) );
	pEntity->pev = &ev[ num_ents++ ];

	pEntity->Precache();
	pEntity->Spawn();

	if ( pWeaponOwner )
	{
		ItemInfo info;
		
		((CBasePlayerWeapon *)pEntity)->m_pPlayer = pWeaponOwner;
		
		((CBasePlayerWeapon *)pEntity)->GetItemInfo( &info );

		g_pWpns[ info.iId ] = (CBasePlayerWeapon *)pEntity;
	}
}

/*
// FIXME:  In order to predict animations client side, you'll need to work with the following code.
//  It's not quite working, but it should be of use if you want
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pmodel - 
//			*label - 
// Output : int
//-----------------------------------------------------------------------------
int LookupSequence( void *pmodel, const char *label )
{
	studiohdr_t *pstudiohdr;
	
	pstudiohdr = (studiohdr_t *)pmodel;
	if (! pstudiohdr)
		return 0;

	mstudioseqdesc_t	*pseqdesc;

	pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex);

	for (int i = 0; i < pstudiohdr->numseq; i++)
	{
		if (stricmp( pseqdesc[i].label, label ) == 0)
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *label - 
// Output : int CBaseAnimating :: LookupSequence
//-----------------------------------------------------------------------------
int CBaseAnimating :: LookupSequence ( const char *label )
{
	cl_entity_t *current;

	current = gEngfuncs.GetLocalPlayer();
	if ( !current || !current->model )
		return 0;

	return ::LookupSequence( (studiohdr_t *)IEngineStudio.Mod_Extradata( current->model ), label );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pmodel - 
//			*pev - 
//			*pflFrameRate - 
//			*pflGroundSpeed - 
//-----------------------------------------------------------------------------
void GetSequenceInfo( void *pmodel, entvars_t *pev, float *pflFrameRate, float *pflGroundSpeed )
{
	studiohdr_t *pstudiohdr;
	
	pstudiohdr = (studiohdr_t *)pmodel;
	if (! pstudiohdr)
		return;

	mstudioseqdesc_t	*pseqdesc;

	if (pev->sequence >= pstudiohdr->numseq)
	{
		*pflFrameRate = 0.0;
		*pflGroundSpeed = 0.0;
		return;
	}

	pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex) + (int)pev->sequence;

	if (pseqdesc->numframes > 1)
	{
		*pflFrameRate = 256 * pseqdesc->fps / (pseqdesc->numframes - 1);
		*pflGroundSpeed = sqrt( pseqdesc->linearmovement[0]*pseqdesc->linearmovement[0]+ pseqdesc->linearmovement[1]*pseqdesc->linearmovement[1]+ pseqdesc->linearmovement[2]*pseqdesc->linearmovement[2] );
		*pflGroundSpeed = *pflGroundSpeed * pseqdesc->fps / (pseqdesc->numframes - 1);
	}
	else
	{
		*pflFrameRate = 256.0;
		*pflGroundSpeed = 0.0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pmodel - 
//			*pev - 
// Output : int
//-----------------------------------------------------------------------------
int GetSequenceFlags( void *pmodel, entvars_t *pev )
{
	studiohdr_t *pstudiohdr;
	
	pstudiohdr = (studiohdr_t *)pmodel;
	if ( !pstudiohdr || pev->sequence >= pstudiohdr->numseq )
		return 0;

	mstudioseqdesc_t	*pseqdesc;
	pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex) + (int)pev->sequence;

	return pseqdesc->flags;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
//-----------------------------------------------------------------------------
void CBaseAnimating :: ResetSequenceInfo ( )
{
	cl_entity_t *current;

	current = gEngfuncs.GetLocalPlayer();
	if ( !current || !current->model )
		return;

	void *pmodel = (studiohdr_t *)IEngineStudio.Mod_Extradata( current->model );

	GetSequenceInfo( pmodel, pev, &m_flFrameRate, &m_flGroundSpeed );
	m_fSequenceLoops = ((GetSequenceFlags() & STUDIO_LOOPING) != 0);
	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0;
	m_fSequenceFinished = FALSE;
	m_flLastEventCheck = gpGlobals->time;
}
*/
/*
=====================
CBaseEntity :: Killed

If weapons code "kills" an entity, just set its effects to EF_NODRAW
=====================
*/
void CBaseEntity :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->effects |= EF_NODRAW;
}

/*
=====================
CBasePlayerWeapon :: DefaultReload
=====================
*/
BOOL CBasePlayerWeapon :: DefaultReload( int iClipSize, int iAnim, float fDelay )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return FALSE;

	int j = min(iClipSize - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);	

	if (j == 0)
		return FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + fDelay;

	m_fInReload = TRUE;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + fDelay;
	return TRUE;
}

/*
=====================
CBasePlayerWeapon :: CanDeploy
=====================
*/
BOOL CBasePlayerWeapon :: CanDeploy( void ) 
{
	BOOL bHasAmmo = 0;

	if ( !pszAmmo1() )
	{
		// this weapon doesn't use ammo, can always deploy.
		return TRUE;
	}

	if ( pszAmmo1() )
	{
		bHasAmmo |= (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != 0);
	}
	if ( pszAmmo2() )
	{
		bHasAmmo |= (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] != 0);
	}
	if (m_iClip > 0)
	{
		bHasAmmo |= 1;
	}
	if (!bHasAmmo)
	{
		return FALSE;
	}

	return TRUE;
}

/*
=====================
CBasePlayerWeapon :: DefaultDeploy

=====================
*/
BOOL CBasePlayerWeapon :: DefaultDeploy( char *szViewModel, char *szWeaponModel, int iAnim, char *szAnimExt, int skiplocal )
{
	if ( !CanDeploy() )
		return FALSE;

	gEngfuncs.CL_LoadModel( szViewModel, &m_pPlayer->pev->viewmodel );
	
	SendWeaponAnim( iAnim );

	m_pPlayer->m_flNextAttack = 0.5;
	m_flTimeWeaponIdle = 1.5;
	return TRUE;
}

/*
=====================
CBasePlayerWeapon :: PlayEmptySound

=====================
*/
BOOL CBasePlayerWeapon :: PlayEmptySound( void )
{
	if (m_iPlayEmptySound)
	{
		HUD_PlaySound( "weapons/357_cock1.wav", 0.8 );
		m_iPlayEmptySound = 0;
		return 0;
	}
	return 0;
}

/*
=====================
CBasePlayerWeapon :: ResetEmptySound

=====================
*/
void CBasePlayerWeapon :: ResetEmptySound( void )
{
	m_iPlayEmptySound = 1;
}

/*
=====================
CBasePlayerWeapon::Holster

Put away weapon
=====================
*/
void CBasePlayerWeapon::Holster( int skiplocal /* = 0 */ )
{ 
	m_fInReload = FALSE; // cancel any reload in progress.
	m_pPlayer->pev->viewmodel = 0; 
}

/*
=====================
CBaseEntity::FireBullets3

Counter-Strike 1.3: the bullet's deviation, from the seed the server shares with the client when
there is an attacker (the server's function computes the same two numbers before it traces)
=====================
*/
Vector CBaseEntity::FireBullets3( Vector vecSrc, Vector vecDirShooting, float vecSpread, float flDistance, int iPenetration, int iBulletType, int iDamage, float flRangeModifier, entvars_t *pevAttacker, bool bPistol, int shared_rand )
{
	float x, y, z;

	if ( !pevAttacker )
	{
		do {
			x = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
			y = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
			z = x*x+y*y;
		} while (z > 1);
	}
	else
	{
		x = UTIL_SharedRandomFloat( shared_rand, -0.5, 0.5 ) + UTIL_SharedRandomFloat( shared_rand + 1, -0.5, 0.5 );
		y = UTIL_SharedRandomFloat( shared_rand + 2, -0.5, 0.5 ) + UTIL_SharedRandomFloat( shared_rand + 3, -0.5, 0.5 );
	}

	return Vector( x * vecSpread, y * vecSpread, 0.0 );
}

/*
=====================
CBasePlayerWeapon::SendWeaponAnim

Animate weapon model
=====================
*/
void CBasePlayerWeapon::SendWeaponAnim( int iAnim, int skiplocal )
{
	if ( !skiplocal )
		return;

	m_pPlayer->pev->weaponanim = iAnim;
	
	int body = 0;

	HUD_SendWeaponAnim( iAnim, body, 0 );
}

/*
=====================
CBasePlayerWeapon::FireRemaining

Counter-Strike: the next shot of the Glock 18's burst
=====================
*/
void CBasePlayerWeapon::FireRemaining( void )
{
	m_iClip--;

	if (m_iClip < 0)
	{
		m_iClip = 0;
		m_iBurstShotsFired = 3;
		m_flNextBurstShot = 0;
		return;
	}

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecSrc = m_pPlayer->GetGunPosition( );
	Vector vecAiming;
	vecAiming = gpGlobals->v_forward;

	Vector vecDir;
	vecDir = m_pPlayer->FireBullets3( vecSrc, vecAiming, 0.05, 8192, 1, BULLET_PLAYER_9MM, 18, 0.9, m_pPlayer->pev, true, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireGlock18, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, (int)(m_pPlayer->pev->punchangle.x * 10000000), (int)(m_pPlayer->pev->punchangle.y * 10000000), (m_iClip == 0), m_pPlayer->m_bLeftHanded != false );

	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_iBurstShotsFired++;

	if (m_iBurstShotsFired != 3)
		m_flNextBurstShot = gpGlobals->time + 0.1f;
	else
		m_flNextBurstShot = 0;
}

/*
=====================
CBasePlayerWeapon::ItemPostFrame

Handles weapon firing, reloading, etc.
=====================
*/
void CBasePlayerWeapon::ItemPostFrame( void )
{
	if ((m_fInReload) && (m_pPlayer->m_flNextAttack <= 0.0))
	{
#if 0 // FIXME, need ammo on client to make this work right
		// complete the reload. 
		int j = min( iMaxClip() - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);	

		// Add them to the clip
		m_iClip += j;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= j;
#else	
		m_iClip += 10;
#endif
		m_fInReload = FALSE;
	}

	if ( m_flNextBurstShot != 0.0 )
		FireRemaining();

	if ( m_flNextPrimaryAttack <= UTIL_WeaponTimeBase() )
	{
		if ( m_pPlayer->m_bResumeZoom == true )
		{
			m_pPlayer->m_iFOV = m_pPlayer->m_iLastZoom;

			if ( m_pPlayer->m_iFOV == m_pPlayer->m_iLastZoom )
			{
				cl_entity_t *ent = GetViewEntity();		// name ours

				ent->baseline.effects |= EF_NODRAW;
				ent->curstate.effects |= EF_NODRAW;
				m_pPlayer->m_bResumeZoom = false;
			}
		}
	}

	if ((m_pPlayer->pev->button & IN_ATTACK2) && (m_flNextSecondaryAttack <= 0.0))
	{
		if ( pszAmmo2() && !m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] )
		{
			m_fFireOnEmpty = TRUE;
		}

		SecondaryAttack();
		m_pPlayer->pev->button &= ~IN_ATTACK2;
	}
	else if ((m_pPlayer->pev->button & IN_ATTACK) && (m_flNextPrimaryAttack <= 0.0))
	{
		if ( (m_iClip == 0 && pszAmmo1()) || (iMaxClip() == -1 && !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] ) )
		{
			m_fFireOnEmpty = TRUE;
		}

		if ( m_pPlayer->m_bCanShoot == true )
			PrimaryAttack();
	}
	else if ( m_pPlayer->pev->button & IN_RELOAD && iMaxClip() != WEAPON_NOCLIP && !m_fInReload ) 
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
	}
	else if ( !(m_pPlayer->pev->button & (IN_ATTACK|IN_ATTACK2) ) )
	{
		// no fire buttons down

		if ( m_bDelayFire == true )
		{
			m_bDelayFire = false;

			if ( m_iShotsFired < 16 )
				m_iShotsFired += 4;
		}

		if ( m_iId == WEAPON_USP || m_iId == WEAPON_GLOCK18 || m_iId == WEAPON_P228 || m_iId == WEAPON_DEAGLE || m_iId == WEAPON_ELITE || m_iId == WEAPON_FIVESEVEN )
			m_iShotsFired = 0;

		m_fFireOnEmpty = FALSE;

		// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
		if ( m_iClip == 0 && !(iFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack < 0.0 )
		{
			Reload();
			return;
		}

		WeaponIdle( );
		return;
	}
	
	// catch all
	if ( ShouldWeaponIdle() )
	{
		WeaponIdle();
	}
}

// sic: a stand-in, name ours.  hl_weapons.obj builds its own precompiled header and emits the
// headers' inline functions by the intermediate code before each of its functions.  Retail had
// more of it before CBasePlayer::Killed than the text here gives.  With CBasePlayer's four
// Vector members (player.h: the implicit constructor of this file's `player` carries their
// intermediate code) this unused inline gives retail's order with 1 to 12 empty statements in
// Beta 6.5 (the server job's measurement), 0 to 12 in Beta 6.6, 1 to 9 in Beta 7.0 and 2 to 19 in
// Beta 7.1 (full builds).  With int arrays in the Vectors' place they took 14 to 40, 26 to 40,
// 20 to 36 and 29 or more.  Counter-Strike 1.0 takes 6 to 8 (full
// builds; 5 and 9 do not), with extdll.h's declared names at 8,040.
// Nothing calls it.
inline void b65_hlweapons_unknown( void )
{
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
}

// Counter-Strike: UTIL_MakeVectors stands in this file, not among hl_baseentity.cpp's stubs (in
// Valve's 1.6 build too, there after CBasePlayer::Spawn).  It is empty in this version: the
// weapons' calls end in the one "ret" the linker keeps.  The linker's kept copies show a function
// of this size here and one fewer there, and the order of this object's functions wants its
// intermediate code before CBasePlayer::Killed.
void UTIL_MakeVectors( const Vector &vecAngles )
{
}

/*
=====================
CBasePlayer::SelectItem

  Switch weapons
=====================
*/
void CBasePlayer::SelectItem(const char *pstr)
{
	if (!pstr)
		return;

	CBasePlayerItem *pItem = NULL;

	if (!pItem)
		return;

	
	if (pItem == m_pActiveItem)
		return;

	if (m_pActiveItem)
		m_pActiveItem->Holster( );
	
	m_pLastItem = m_pActiveItem;
	m_pActiveItem = pItem;

	if (m_pActiveItem)
	{
		m_pActiveItem->Deploy( );
	}
}

/*
=====================
CBasePlayer::SelectLastItem

=====================
*/
void CBasePlayer::SelectLastItem(void)
{
	if (!m_pLastItem)
	{
		return;
	}

	if ( m_pActiveItem && !m_pActiveItem->CanHolster() )
	{
		return;
	}

	if (m_pActiveItem)
		m_pActiveItem->Holster( );
	
	CBasePlayerItem *pTemp = m_pActiveItem;
	m_pActiveItem = m_pLastItem;
	m_pLastItem = pTemp;
	m_pActiveItem->Deploy( );
}

/*
=====================
CBasePlayer::Killed

=====================
*/
void CBasePlayer::Killed( entvars_t *pevAttacker, int iGib )
{
	// Holster weapon immediately, to allow it to cleanup
	if (m_pActiveItem)
		m_pActiveItem->Holster( );
}

/*
=====================
CBasePlayer::Spawn

=====================
*/
void CBasePlayer::Spawn( void )
{
	if (m_pActiveItem)
		m_pActiveItem->Deploy( );
}

/*
=====================
UTIL_TraceLine

Don't actually trace, but act like the trace didn't hit anything.
=====================
*/
void UTIL_TraceLine( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr )
{
	memset( ptr, 0, sizeof( *ptr ) );
	ptr->flFraction = 1.0;
}

/*
=====================
UTIL_ParticleBox

For debugging, draw a box around a player made out of particles
=====================
*/
void UTIL_ParticleBox( CBasePlayer *player, float *mins, float *maxs, float life, unsigned char r, unsigned char g, unsigned char b )
{
	int i;
	vec3_t mmin, mmax;

	for ( i = 0; i < 3; i++ )
	{
		mmin[ i ] = player->pev->origin[ i ] + mins[ i ];
		mmax[ i ] = player->pev->origin[ i ] + maxs[ i ];
	}

	gEngfuncs.pEfxAPI->R_ParticleBox( (float *)&mmin, (float *)&mmax, 5.0, 0, 255, 0 );
}

/*
=====================
UTIL_ParticleBoxes

For debugging, draw boxes for other collidable players
=====================
*/
void UTIL_ParticleBoxes( void )
{
	int idx;
	physent_t *pe;
	cl_entity_t *player;
	vec3_t mins, maxs;
	
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	player = gEngfuncs.GetLocalPlayer();
	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( player->index - 1 );	

	for ( idx = 1; idx < 100; idx++ )
	{
		pe = gEngfuncs.pEventAPI->EV_GetPhysent( idx );
		if ( !pe )
			break;

		if ( pe->info >= 1 && pe->info <= gEngfuncs.GetMaxClients() )
		{
			mins = pe->origin + pe->mins;
			maxs = pe->origin + pe->maxs;

			gEngfuncs.pEfxAPI->R_ParticleBox( (float *)&mins, (float *)&maxs, 0, 0, 255, 2.0 );
		}
	}

	gEngfuncs.pEventAPI->EV_PopPMStates();
}

/*
=====================
UTIL_ParticleLine

For debugging, draw a line made out of particles
=====================
*/
void UTIL_ParticleLine( CBasePlayer *player, float *start, float *end, float life, unsigned char r, unsigned char g, unsigned char b )
{
	gEngfuncs.pEfxAPI->R_ParticleLine( start, end, r, g, b, life );
}

/*
=====================
CBasePlayerWeapon::PrintState

For debugging, print out state variables to log file
=====================
*/
void CBasePlayerWeapon::PrintState( void )
{
	COM_Log( "c:\\hl.log", "%.4f ", gpGlobals->time );
	COM_Log( "c:\\hl.log", "%.4f ", m_pPlayer->m_flNextAttack );
	COM_Log( "c:\\hl.log", "%.4f ", m_flNextPrimaryAttack );
	COM_Log( "c:\\hl.log", "%.4f ", m_flTimeWeaponIdle - gpGlobals->time);
	COM_Log( "c:\\hl.log", "%i ", m_iClip );
}

/*
=====================
HUD_InitClientWeapons

Set up weapons, player and functions needed to run weapons code client-side.
=====================
*/
void HUD_InitClientWeapons( void )
{
	static int initialized = 0;
	if ( initialized )
		return;

	initialized = 1;

	// Set up pointer ( dummy object )
	gpGlobals = &Globals;

	// Fill in current time ( probably not needed )
	gpGlobals->time = gEngfuncs.GetClientTime();

	// Fake functions
	g_engfuncs.pfnPrecacheModel		= stub_PrecacheModel;
	g_engfuncs.pfnPrecacheSound		= stub_PrecacheSound;
	g_engfuncs.pfnPrecacheEvent		= stub_PrecacheEvent;
	g_engfuncs.pfnNameForFunction	= stub_NameForFunction;
	g_engfuncs.pfnSetModel			= stub_SetModel;
	g_engfuncs.pfnSetClientMaxspeed = HUD_SetMaxSpeed;

	// Handled locally
	g_engfuncs.pfnPlaybackEvent		= HUD_PlaybackEvent;
	g_engfuncs.pfnAlertMessage		= AlertMessage;

	// Pass through to engine
	g_engfuncs.pfnPrecacheEvent		= gEngfuncs.pfnPrecacheEvent;
	g_engfuncs.pfnRandomFloat		= gEngfuncs.pfnRandomFloat;
	g_engfuncs.pfnRandomLong		= gEngfuncs.pfnRandomLong;

	// Allocate a slot for the local player
	HUD_PrepEntity( &player		, NULL );

	// Allocate slot(s) for each weapon that we are going to be predicting
	HUD_PrepEntity( &g_USP	, &player );
	HUD_PrepEntity( &g_ELITE	, &player );
	HUD_PrepEntity( &g_UMP45	, &player );
	HUD_PrepEntity( &g_FIVESEVEN	, &player );
	HUD_PrepEntity( &g_MP5N	, &player );
	HUD_PrepEntity( &g_SG552	, &player );
	HUD_PrepEntity( &g_AK47	, &player );
	HUD_PrepEntity( &g_AUG	, &player );
	HUD_PrepEntity( &g_AWP	, &player );
	HUD_PrepEntity( &g_C4	, &player );
	HUD_PrepEntity( &g_DEAGLE	, &player );
	HUD_PrepEntity( &g_Flashbang	, &player );
	HUD_PrepEntity( &g_SmokeGrenade	, &player );
	HUD_PrepEntity( &g_G3SG1	, &player );
	HUD_PrepEntity( &g_SG550	, &player );
	HUD_PrepEntity( &g_GLOCK18	, &player );
	HUD_PrepEntity( &g_HEGrenade	, &player );
	HUD_PrepEntity( &g_Knife	, &player );
	HUD_PrepEntity( &g_M249	, &player );
	HUD_PrepEntity( &g_M3	, &player );
	HUD_PrepEntity( &g_M4A1	, &player );
	HUD_PrepEntity( &g_MAC10	, &player );
	HUD_PrepEntity( &g_P228	, &player );
	HUD_PrepEntity( &g_P90	, &player );
	HUD_PrepEntity( &g_SCOUT	, &player );
	HUD_PrepEntity( &g_TMP	, &player );
	HUD_PrepEntity( &g_XM1014	, &player );
}

/*
=====================
HUD_WeaponsPostThink

Run Weapon firing code on client
=====================
*/
void HUD_WeaponsPostThink( local_state_s *from, local_state_s *to, usercmd_t *cmd, double time, unsigned int random_seed )
{
	int i;
	int buttonsChanged;
	CBasePlayerWeapon *pWeapon = NULL;
	CBasePlayerWeapon *pCurrent;
	weapon_data_t *pfrom, *pto;
	static int lasthealth;

	HUD_InitClientWeapons();	

	// Get current clock
	gpGlobals->time = time;

	// Fill in data based on selected weapon
	// FIXME, make this a method in each weapon?  where you pass in an entity_state_t *?
	switch ( from->client.m_iId )
	{
	case WEAPON_USP: pWeapon = &g_USP; break;
	case WEAPON_ELITE: pWeapon = &g_ELITE; break;
	case WEAPON_FIVESEVEN: pWeapon = &g_FIVESEVEN; break;
	case WEAPON_UMP45: pWeapon = &g_UMP45; break;
	case WEAPON_MP5N: pWeapon = &g_MP5N; break;
	case WEAPON_SG552: pWeapon = &g_SG552; break;
	case WEAPON_AK47: pWeapon = &g_AK47; break;
	case WEAPON_AUG: pWeapon = &g_AUG; break;
	case WEAPON_AWP: pWeapon = &g_AWP; break;
	case WEAPON_C4: pWeapon = &g_C4; break;
	case WEAPON_DEAGLE: pWeapon = &g_DEAGLE; break;
	case WEAPON_FLASHBANG: pWeapon = &g_Flashbang; break;
	case WEAPON_SMOKEGRENADE: pWeapon = &g_SmokeGrenade; break;
	case WEAPON_G3SG1: pWeapon = &g_G3SG1; break;
	case WEAPON_SG550: pWeapon = &g_SG550; break;
	case WEAPON_GLOCK18: pWeapon = &g_GLOCK18; break;
	case WEAPON_HEGRENADE: pWeapon = &g_HEGrenade; break;
	case WEAPON_KNIFE: pWeapon = &g_Knife; break;
	case WEAPON_M249: pWeapon = &g_M249; break;
	case WEAPON_M3: pWeapon = &g_M3; break;
	case WEAPON_M4A1: pWeapon = &g_M4A1; break;
	case WEAPON_MAC10: pWeapon = &g_MAC10; break;
	case WEAPON_P228: pWeapon = &g_P228; break;
	case WEAPON_P90: pWeapon = &g_P90; break;
	case WEAPON_SCOUT: pWeapon = &g_SCOUT; break;
	case WEAPON_TMP: pWeapon = &g_TMP; break;
	case WEAPON_XM1014: pWeapon = &g_XM1014; break;
	}

	// We are not predicting the current weapon, just bow out here.
	if ( !pWeapon )
		return;

	for ( i = 0; i < 32; i++ )
	{
		pCurrent = g_pWpns[ i ];
		if ( !pCurrent )
		{
			continue;
		}

		pfrom = &from->weapondata[ i ];
		
		pCurrent->m_fInReload			= pfrom->m_fInReload;
		pCurrent->m_fInSpecialReload	= pfrom->m_fInSpecialReload;
		pCurrent->m_iClip				= pfrom->m_iClip;
		pCurrent->m_flNextPrimaryAttack	= pfrom->m_flNextPrimaryAttack;
		pCurrent->m_flNextSecondaryAttack = pfrom->m_flNextSecondaryAttack;
		pCurrent->m_flNextReload		= pfrom->m_flNextReload;
		pCurrent->m_flTimeWeaponIdle	= pfrom->m_flTimeWeaponIdle;
		pCurrent->m_iShotsFired			= pfrom->m_fInZoom;
		pCurrent->m_iWeaponState		= pfrom->m_iWeaponState;
		pCurrent->m_flLastFire			= pfrom->m_fAimedDamage;
		pCurrent->m_iPrimaryAmmoType	= (int)from->client.vuser4[ 0 ];
		player.m_rgAmmo[ pCurrent->m_iPrimaryAmmoType ] = (int)from->client.vuser4[ 1 ];
	}

	// For random weapon events, use this seed to seed random # generator
	player.random_seed = random_seed;

	// Get old buttons from previous state.
	player.m_afButtonLast = from->playerstate.oldbuttons;

	// Which buttsons chave changed
	buttonsChanged = (player.m_afButtonLast ^ cmd->buttons);	// These buttons have changed this frame
	
	// Debounced button codes for pressed/released
	// The changed ones still down are "pressed"
	player.m_afButtonPressed =  buttonsChanged & cmd->buttons;	
	// The ones not down are "released"
	player.m_afButtonReleased = buttonsChanged & (~cmd->buttons);

	// Set player variables that weapons code might check/alter
	player.pev->button = cmd->buttons;

	player.pev->velocity = from->client.velocity;
	player.pev->flags = from->client.flags;

	player.pev->deadflag = from->client.deadflag;
	player.pev->waterlevel = from->client.waterlevel;
	player.pev->maxspeed    = from->client.maxspeed;
	player.pev->punchangle = from->client.punchangle;
	player.pev->fov = from->client.fov;
	player.pev->weaponanim = from->client.weaponanim;
	player.pev->viewmodel = from->client.viewmodel;
	player.m_flNextAttack = from->client.m_flNextAttack;
	int iUser3 = from->client.iuser3;	// name ours
	if ( iUser3 & 1 )
	{
		player.m_bCanShoot = true;
		g_iFreezeTimeOver = 1;
	}
	else
	{
		player.m_bCanShoot = false;
		g_iFreezeTimeOver = 0;
	}
	if ( iUser3 & 2 )
		player.m_bLeftHanded = true;
	else
		player.m_bLeftHanded = false;

	// Ammo counts for the client's weapons
	player.ammo_buckshot	= from->client.ammo_shells;
	player.ammo_9mm			= from->client.ammo_nails;
	player.ammo_556nato		= from->client.ammo_cells;
	player.ammo_556natobox	= from->client.ammo_rockets;
	player.ammo_762nato		= (int)from->client.vuser2[ 0 ];
	player.ammo_45acp		= (int)from->client.vuser2[ 1 ];
	player.ammo_50ae		= (int)from->client.vuser2[ 2 ];
	player.ammo_338mag		= (int)from->client.vuser3[ 0 ];
	player.ammo_57mm		= (int)from->client.vuser3[ 1 ];
	player.ammo_357sig		= (int)from->client.vuser3[ 2 ];

	// Point to current weapon object
	if ( from->client.m_iId )
	{
		player.m_pActiveItem = g_pWpns[ from->client.m_iId ];
	}

	// Store pointer to our destination entity_state_t so we can get our origin, etc. from it
	//  for setting up events on the client
	g_finalstate = to;

	// Don't go firing anything if we have died.
	// Or if we don't have a weapon model deployed
	if ( ( player.pev->deadflag != ( DEAD_DISCARDBODY + 1 ) ) && !CL_IsDead() && player.pev->viewmodel )
	{
		if ( player.m_flNextAttack <= 0 )
		{
			pWeapon->ItemPostFrame();
		}
	}

	// If we are running events/etc. go ahead and see if we
	//  managed to die between last frame and this one
	// If so, run the appropriate player killed or spawn function
	if ( g_runfuncs )
	{
		if ( to->client.health <= 0 && lasthealth > 0 )
		{
			player.Killed( NULL, 0 );
		}
		else if ( to->client.health > 0 && lasthealth <= 0 )
		{
			player.Spawn();
		}

		lasthealth = to->client.health;
	}

	// Assume that we are not going to switch weapons
	to->client.m_iId					= from->client.m_iId;

	// Now see if we issued a changeweapon command ( and we're not dead )
	if ( cmd->weaponselect && ( player.pev->deadflag != ( DEAD_DISCARDBODY + 1 ) ) )
	{
		// Switched to a different weapon?
		if ( from->weapondata[ cmd->weaponselect ].m_iId == cmd->weaponselect )
		{
			CBasePlayerWeapon *pNew = g_pWpns[ cmd->weaponselect ];
			if ( pNew && ( pNew != pWeapon ) )
			{
				// Put away old weapon
				if (player.m_pActiveItem)
					player.m_pActiveItem->Holster( );
				
				player.m_pLastItem = player.m_pActiveItem;
				player.m_pActiveItem = pNew;

				// Deploy new weapon
				if (player.m_pActiveItem)
				{
					player.m_pActiveItem->Deploy( );
				}

				// Update weapon id so we can predict things correctly.
				to->client.m_iId = cmd->weaponselect;
			}
		}
	}

	// Copy in results of predcition code
	to->client.viewmodel				= player.pev->viewmodel;
	to->client.fov						= player.pev->fov;
	to->client.weaponanim				= player.pev->weaponanim;
	to->client.m_flNextAttack			= player.m_flNextAttack;
	to->client.maxspeed					= player.pev->maxspeed;
	to->client.punchangle				= player.pev->punchangle;

	to->client.ammo_shells				= player.ammo_buckshot;
	to->client.ammo_nails				= player.ammo_9mm;
	to->client.ammo_cells				= player.ammo_556nato;
	to->client.ammo_rockets				= player.ammo_556natobox;
	to->client.vuser2[ 0 ]				= player.ammo_762nato;
	to->client.vuser2[ 1 ]				= player.ammo_45acp;
	to->client.vuser2[ 2 ]				= player.ammo_50ae;
	to->client.vuser3[ 0 ]				= player.ammo_338mag;
	to->client.vuser3[ 1 ]				= player.ammo_57mm;
	to->client.vuser3[ 2 ]				= player.ammo_357sig;

	to->client.iuser3					= iUser3;

	// Make sure that weapon animation matches what the game .dll is telling us
	//  over the wire ( fixes some animation glitches )
	if ( g_runfuncs && ( HUD_GetWeaponAnim() != to->client.weaponanim ) )
	{
		int body = 2;
		// Force a fixed anim down to viewmodel
		HUD_SendWeaponAnim( to->client.weaponanim, body, 1 );
	}

	for ( i = 0; i < 32; i++ )
	{
		pCurrent = g_pWpns[ i ];

		pto = &to->weapondata[ i ];

		if ( !pCurrent )
		{
			memset( pto, 0, sizeof( weapon_data_t ) );
			continue;
		}
	
		pto->m_fInReload				= pCurrent->m_fInReload;
		pto->m_fInSpecialReload			= pCurrent->m_fInSpecialReload;
		pto->m_iClip					= pCurrent->m_iClip; 
		pto->m_flNextPrimaryAttack		= pCurrent->m_flNextPrimaryAttack;
		pto->m_flNextSecondaryAttack	= pCurrent->m_flNextSecondaryAttack;
		pto->m_flNextReload				= pCurrent->m_flNextReload;
		pto->m_flTimeWeaponIdle			= pCurrent->m_flTimeWeaponIdle;
		pto->m_iWeaponState				= pCurrent->m_iWeaponState;
		pto->m_fInZoom					= pCurrent->m_iShotsFired;
		pto->m_fAimedDamage				= pCurrent->m_flLastFire;
		to->client.vuser4[ 0 ]			= pCurrent->m_iPrimaryAmmoType;
		to->client.vuser4[ 1 ]			= player.m_rgAmmo[ pCurrent->m_iPrimaryAmmoType ];

		// Decrement weapon counters, server does this at same time ( during post think, after doing everything else )
		pto->m_flNextReload				-= cmd->msec / 1000.0;
		pto->m_fNextAimBonus			-= cmd->msec / 1000.0;
		pto->m_flNextPrimaryAttack		-= cmd->msec / 1000.0;
		pto->m_flNextSecondaryAttack	-= cmd->msec / 1000.0;
		pto->m_flTimeWeaponIdle			-= cmd->msec / 1000.0;

		if ( pto->m_flPumpTime != -9999 )
		{
			pto->m_flPumpTime -= cmd->msec / 1000.0;
			if ( pto->m_flPumpTime < -0.001 )
				pto->m_flPumpTime = -0.001;
		}

		if ( pto->m_fNextAimBonus < -1.0 )
		{
			pto->m_fNextAimBonus = -1.0;
		}

		if ( pto->m_flNextPrimaryAttack < -1.0 )
		{
			pto->m_flNextPrimaryAttack = -1.0;
		}

		if ( pto->m_flNextSecondaryAttack < -0.001 )
		{
			pto->m_flNextSecondaryAttack = -0.001;
		}

		if ( pto->m_flTimeWeaponIdle < -0.001 )
		{
			pto->m_flTimeWeaponIdle = -0.001;
		}

		if ( pto->m_flNextReload < -0.001 )
		{
			pto->m_flNextReload = -0.001;
		}
	}

	// m_flNextAttack is now part of the weapons, but is part of the player instead
	to->client.m_flNextAttack -= cmd->msec / 1000.0;
	if ( to->client.m_flNextAttack < -0.001 )
	{
		to->client.m_flNextAttack = -0.001;
	}

	// Wipe it so we can't use it after this frame
	g_finalstate = NULL;
}

// For storing predicted sequence and gaitsequence and origin/angles data
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *seq - 
//			*gaitseq - 
//-----------------------------------------------------------------------------
void Game_GetSequence( int *seq, int *gaitseq )
{
	*seq = g_rseq;
	*gaitseq = g_gaitseq;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : seq - 
//			gaitseq - 
//-----------------------------------------------------------------------------
void Game_SetSequence( int seq, int gaitseq )
{
	g_rseq = seq;
	g_gaitseq = gaitseq;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : o - 
//			a - 
//-----------------------------------------------------------------------------
void Game_SetOrientation( vec3_t o, vec3_t a )
{
	g_clorg = o;
	g_clang = a;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *o - 
//			*a - 
//-----------------------------------------------------------------------------
void Game_GetOrientation( float *o, float *a )
{
	int i;
	
	for ( i = 0; i < 3; i++ )
	{
		o[ i ] = g_clorg[ i ];
		a[ i ] = g_clang[ i ];
	}
}

/*
=====================
HUD_PostRunCmd

Client calls this during prediction, after it has moved the player and updated any info changed into to->
time is the current client clock based on prediction
cmd is the command that caused the movement, etc
runfuncs is 1 if this is the first time we've predicted this command.  If so, sounds and effects should play, otherwise, they should
be ignored
=====================
*/
void _DLLEXPORT HUD_PostRunCmd( struct local_state_s *from, struct local_state_s *to, struct usercmd_s *cmd, int runfuncs, double time, unsigned int random_seed )
{
	g_runfuncs = runfuncs;

	HUD_WeaponsPostThink( from, to, cmd, time, random_seed );

	to->client.fov = g_lastFOV;

	// Store of final sequence, etc. for client side animation
	if ( g_runfuncs )
	{
		Game_SetSequence( to->playerstate.sequence, to->playerstate.gaitsequence );
		Game_SetOrientation( to->playerstate.origin, cmd->viewangles );
	}

	// All games can use FOV state
	g_lastFOV = to->client.fov;
}
