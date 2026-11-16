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
#if !defined( OEM_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"




enum rpg_e {
	RPG_IDLE = 0,
	RPG_FIDGET,
	RPG_RELOAD,		// to reload
	RPG_FIRE2,		// to empty
	RPG_HOLSTER1,	// loaded
	RPG_DRAW1,		// loaded
	RPG_HOLSTER2,	// unloaded
	RPG_DRAW_UL,	// unloaded
	RPG_IDLE_UL,	// unloaded idle
	RPG_FIDGET_UL,	// unloaded fidget
};

#ifndef CLIENT_DLL

LINK_ENTITY_TO_CLASS( laser_spot, CLaserSpot );

//=========================================================
//=========================================================
CLaserSpot *CLaserSpot::CreateSpot( void )
{
	CLaserSpot *pSpot = GetClassPtr( (CLaserSpot *)NULL );
	pSpot->Spawn();

	pSpot->pev->classname = MAKE_STRING("laser_spot");

	return pSpot;
}

//=========================================================
//=========================================================
void CLaserSpot::Spawn( void )
{
	Precache( );
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	pev->rendermode = kRenderGlow;
	pev->renderfx = kRenderFxNoDissipation;
	pev->renderamt = 255;
	pev->rendercolor = Vector( 255, 0, 0 );

	SET_MODEL(ENT(pev), "sprites/laserdot.spr");
	UTIL_SetOrigin( pev, pev->origin );
};

//=========================================================
// Suspend- make the laser sight invisible. 
//=========================================================
void CLaserSpot::Suspend( float flSuspendTime )
{
	pev->effects |= EF_NODRAW;
	
	SetThink( Revive );
	pev->nextthink = gpGlobals->time + flSuspendTime;
}

//=========================================================
// Revive - bring a suspended laser sight back.
//=========================================================
void CLaserSpot::Revive( void )
{
	pev->effects &= ~EF_NODRAW;

	SetThink( NULL );
}

void CLaserSpot::Precache( void )
{
	PRECACHE_MODEL("sprites/laserdot.spr");
};

LINK_ENTITY_TO_CLASS( rpg_rocket, CRpgRocket );

//=========================================================
//=========================================================
CRpgRocket *CRpgRocket::CreateRpgRocket( Vector vecOrigin, Vector vecAngles, edict_t *pOwner, CRpg *pLauncher, const Vector &vecColor )
{
	CRpgRocket *pRocket = GetClassPtr( (CRpgRocket *)NULL );

	UTIL_SetOrigin( pRocket->pev, vecOrigin );
	pRocket->pev->angles = vecAngles;
	pRocket->pev->angles.x = -vecAngles.x;
	pRocket->pev->rendercolor = vecColor;
	if ( pLauncher && pLauncher->m_mode == 3 )
	{
		// remote controlled: the player looks through it
		pRocket->pev->renderamt = 0;
		pRocket->pev->rendermode = kRenderTransAdd;
	}
	pRocket->Spawn();
	pRocket->m_pLauncher = pLauncher;// remember what RPG fired me. 
	pRocket->m_pLauncher->m_cActiveRockets++;// register this missile as active for the launcher
	pRocket->pev->owner = pOwner;

	return pRocket;
}

//=========================================================
//=========================================================
void CRpgRocket :: Spawn( void )
{
	CRocket::Spawn( );

	pev->classname = MAKE_STRING("rpg_rocket");

	SetTouch( RpgTouch );

	pev->dmg = gSkillData.plrDmgRPG;
}

void CRpgRocket :: IgniteTrail( const Vector &vecColor )
{
	pev->effects |= EF_LIGHT;

	// make rocket sound
	EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5 );

	// rocket trail, longer behind the remote controlled rocket
	BeamFollow( g_sModelIndexSmokePuff, pev->rendermode == kRenderTransAdd ? 160 : 40, 5, vecColor, 255 );
}

//=========================================================
//=========================================================
void CRpgRocket :: RpgTouch ( CBaseEntity *pOther )
{
	if ( m_pLauncher )
	{
		// my launcher is still around, tell it I'm dead.
		m_pLauncher->m_cActiveRockets--;
	}

	if ( pev->owner && pev->rendermode == kRenderTransAdd )
	{
		// the player's own view back
		SET_VIEW( pev->owner, pev->owner );
		pev->rendermode = kRenderNormal;
	}

	RocketTouch( pOther );
}

void CRpgRocket :: Killed( entvars_t *pevAttacker, int iGib )
{
	if ( pev->owner && pev->rendermode == kRenderTransAdd )
	{
		SET_VIEW( pev->owner, pev->owner );
		pev->rendermode = kRenderNormal;
	}

	CRocket::Killed( pevAttacker, iGib );
}

// remote controlled: flies where its owner looks, until the owner presses +attack2
void CRpgRocket :: CameraThink( void )
{
	pev->nextthink = gpGlobals->time + 0.01;

	edict_t *pOwner = pev->owner;
	if ( !pOwner )
	{
		SetThink( FlyThink );
		pev->rendermode = kRenderNormal;
	}

	// sic: goes on with a NULL owner
	if ( pOwner->v.button & IN_ATTACK2 )
	{
		SetThink( FlyThink );
		SET_VIEW( pev->owner, pev->owner );
		pev->rendermode = kRenderNormal;
	}

	pev->angles = pOwner->v.v_angle;

	if ( pev->dmgtime <= gpGlobals->time )
	{
		pev->dmgtime = gpGlobals->time + 0.1;

		UTIL_MakeVectors( pev->angles );

		float flSpeed = pev->velocity.Length() + 256;
		if ( flSpeed >= pev->speed )
			flSpeed = pev->speed;
		pev->velocity = gpGlobals->v_forward * flSpeed;

		if (pev->waterlevel == 3)
		{
			// go slow underwater
			if (pev->velocity.Length() > 300)
			{
				pev->velocity = pev->velocity.Normalize() * 300;
			}
			UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1, pev->origin, 4 );
		}
	}
}
#endif

LINK_ENTITY_TO_CLASS( weapon_rpg, CRpg );

void CRpg::Reload( void )
{
	if ( m_iClip == 1 )
	{
		// don't bother with any of this if don't need to reload.
		return;
	}

	// because the RPG waits to autoreload when no missiles are active while  the LTD is on, the
	// weapons code is constantly calling into this function, but is often denied because 
	// a) missiles are in flight, but the LTD is on
	// or
	// b) player is totally out of ammo and has nothing to switch to, and should be allowed to
	//    shine the designator around
	//
	// Set the next attack time into the future so that WeaponIdle will get called more often
	// than reload, allowing the RPG LTD to be updated
	
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;

	if ( m_cActiveRockets && ( m_fSpotActive || m_mode == 3 ) )
	{
		// no reloading when there are active missiles tracking the designator (or flown by the player).
		// ward off future autoreload attempts by setting next attack time into the future for a bit. 
		return;
	}

#ifndef CLIENT_DLL
	if ( m_pSpot && m_fSpotActive )
	{
		m_pSpot->Suspend( 2.1 );
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2.1;
	}
#endif

	if ( DefaultReload( RPG_MAX_CLIP, RPG_RELOAD, 2 ) )
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	
}

void CRpg::Spawn( )
{
	Precache( );
	m_iId = WEAPON_RPG;

	SET_MODEL(ENT(pev), "models/w_rpg.mdl");
	m_fSpotActive = 1;
	m_mode = 1;		// laser-guided
	pev->skin = 0;

	m_iDefaultAmmo = RPG_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CRpg::Precache( void )
{
	PRECACHE_MODEL("models/w_rpg.mdl");
	PRECACHE_MODEL("models/v_rpg.mdl");
	PRECACHE_MODEL("models/p_rpg.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");

	UTIL_PrecacheOther( "laser_spot" );
	UTIL_PrecacheOther( "rpg_rocket" );

	PRECACHE_SOUND("weapons/rocketfire1.wav");
	PRECACHE_SOUND("weapons/glauncher.wav"); // alternative fire sound

	m_usRpg = PRECACHE_EVENT ( 1, "events/rpg.sc" );
}


int CRpg::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "rockets";
	p->iMaxAmmo1 = ROCKET_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = RPG_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_RPG;
	p->iFlags = 0;
	p->iWeight = RPG_WEIGHT;

	return 1;
}

int CRpg::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CRpg::Deploy( )
{
	if ( m_iClip == 0 )
	{
		return DefaultDeploy( "models/v_rpg.mdl", "models/p_rpg.mdl", RPG_DRAW_UL, "rpg" );
	}

	return DefaultDeploy( "models/v_rpg.mdl", "models/p_rpg.mdl", RPG_DRAW1, "rpg" );
}


BOOL CRpg::CanHolster( void )
{
	if ( ( m_fSpotActive || m_mode == 3 ) && m_cActiveRockets )
	{
		// can't put away while guiding a missile.
		return FALSE;
	}

	return TRUE;
}

void CRpg::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	
	SendWeaponAnim( RPG_HOLSTER1 );

#ifndef CLIENT_DLL
	if (m_pSpot)
	{
		m_pSpot->Killed( NULL, GIB_NEVER );
		m_pSpot = NULL;
	}
#endif

}



void CRpg::PrimaryAttack()
{
	if ( m_iClip )
	{
		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

#ifndef CLIENT_DLL
		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		// the trail's colour, by guidance and warhead
		Vector vecColor = Vector( 224, 224, 255 );
		switch ( m_mode )
		{
		case 1:
			if ( m_ammo == 1 || m_ammo == 2 )
				vecColor = Vector( 255, 128, 0 );
			break;
		case 2:
			vecColor = Vector( 255, 224, 64 );
			break;
		case 3:
			vecColor = Vector( 70, 140, 255 );
			break;
		default:
			if ( m_ammo == 1 || m_ammo == 2 )
				vecColor = Vector( 255, 64, 0 );
			break;
		}
		if ( m_ammo == 3 )
			vecColor = Vector( 128, 192, 255 );

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		Vector vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -8;
		
		CRpgRocket *pRocket = CRpgRocket::CreateRpgRocket( vecSrc, m_pPlayer->pev->v_angle, m_pPlayer->edict(), this, vecColor );

		// guidance
		switch ( m_mode )
		{
		case 1:
			pRocket->SetThink( CRocket::FollowThink );
			pRocket->pev->speed = 1500;
			pRocket->m_iszKillDesc = MAKE_STRING( "a laser-guided rocket" );
			break;
		case 2:
			pRocket->SetThink( CRocket::SeekThink );
			pRocket->pev->speed = 1200;
			pRocket->m_iszKillDesc = MAKE_STRING( "a heat-seeking missile" );
			break;
		case 3:
			pRocket->SetThink( CRpgRocket::CameraThink );
			pRocket->pev->speed = 512;
			pRocket->m_iszKillDesc = MAKE_STRING( "a remote controlled rocket" );
			SET_VIEW( m_pPlayer->edict(), pRocket->edict() );
			break;
		default:
			pev->speed = 2000;	// sic: the launcher's
			pRocket->m_iszKillDesc = MAKE_STRING( "a rocket propelled grenade" );
			break;
		}

		// warhead (ggrenade.cpp reads the spawnflags)
		switch ( m_ammo )
		{
		case 1:
			// incendiary
			pRocket->pev->spawnflags |= 3;
			if ( m_mode < 3 )
				pRocket->pev->speed -= 200;
			pRocket->pev->dmg = gSkillData.plrDmgRPG * 0.5;
			break;
		case 2:
			// plasma bomb
			pRocket->pev->spawnflags |= 9;
			if ( m_mode < 3 )
				pRocket->pev->speed -= 200;
			pRocket->pev->rendercolor = Vector( 255, 64, 0 );
			break;
		case 3:
			// EMP
			pRocket->pev->spawnflags |= 16;
			pRocket->m_bitsDamageInflict = DMG_SHOCK | DMG_NEVERGIB;
			pRocket->m_flDelay = 2;
			if ( m_mode < 3 )
				pRocket->pev->speed -= 400;
			pRocket->pev->rendercolor = Vector( 128, 192, 255 );
			break;
		}
#endif

		// firing RPG no longer turns on the designator. ALT fire is a toggle switch for the LTD.
		// Ken signed up for this as a global change (sjb)

		int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

		PLAYBACK_EVENT( flags, m_pPlayer->edict(), m_usRpg );

		m_iClip--; 
				
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
	}
	else
	{
		PlayEmptySound( );
	}
	UpdateSpot( );
}


// cycles the guidance (m_mode): unguided, laser-guided, heat-seeking, remote controlled
void CRpg::SecondaryAttack()
{
	if ( ( !m_cActiveRockets || ( !m_fSpotActive && m_mode != 3 ) ) && ( m_iClip || m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 ) )
	{
		m_mode++;
		if ( m_mode > 3 )
			m_mode = 0;

		if ( m_mode == 1 )
			m_fSpotActive = 1;
		else
			m_fSpotActive = 0;

#ifndef CLIENT_DLL
		if (!m_fSpotActive && m_pSpot)
		{
			m_pSpot->Killed( NULL, GIB_NORMAL );
			m_pSpot = NULL;
		}
#endif

		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2;

		// unload (sic: an empty launcher gives a rocket too)
		m_iClip = 0;
		if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < iMaxAmmo1() )
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]++;

#ifndef CLIENT_DLL
		const char *pszMode;
		switch ( m_mode )
		{
		case 1:
			pszMode = "Laser-guided";
			break;
		case 2:
			pszMode = "Heatseeking";
			break;
		case 3:
			pszMode = "Remote controlled";
			break;
		default:
			pszMode = "Unguided";
			break;
		}

		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "%s missile loaded", pszMode );
#endif

		Reload( );
	}
}


// +attack3: cycles the warhead (m_ammo): high explosive, incendiary, plasma bomb, EMP
void CRpg::WeaponSpecial( void )
{
	if ( ( !m_cActiveRockets || ( !m_fSpotActive && m_mode != 3 ) ) && ( m_iClip || m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 ) )
	{
		// unload (sic: an empty launcher gives a rocket too)
		m_iClip = 0;
		if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < iMaxAmmo1() )
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]++;

		m_ammo++;
		if ( m_ammo > 3 )
			m_ammo = 0;

		m_flNextWeaponSpecial = UTIL_WeaponTimeBase() + 1.0;

#ifndef CLIENT_DLL
		const char *pszWarhead;
		switch ( m_ammo )
		{
		case 1:
			pszWarhead = "Incendiary";
			break;
		case 2:
			pszWarhead = "Plasma Bomb";
			break;
		case 3:
			pszWarhead = "EMP";
			break;
		default:
			pszWarhead = "High Explosive";
			break;
		}

		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "%s warhead loaded", pszWarhead );
#endif

		Reload( );
	}
}


void CRpg::WeaponIdle( void )
{
	UpdateSpot( );

	ResetEmptySound( );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if (flRand <= 0.75 || m_fSpotActive)
		{
			if ( m_iClip == 0 )
				iAnim = RPG_IDLE_UL;
			else
				iAnim = RPG_IDLE;

			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 90.0 / 15.0;
		}
		else
		{
			if ( m_iClip == 0 )
				iAnim = RPG_FIDGET_UL;
			else
				iAnim = RPG_FIDGET;

			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0;
		}

		SendWeaponAnim( iAnim );
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1;	// sic: also after an idle animation
}



void CRpg::UpdateSpot( void )
{

#ifndef CLIENT_DLL
	if (m_fSpotActive)
	{
		if (!m_pSpot)
		{
			m_pSpot = CLaserSpot::CreateSpot();
		}

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		Vector vecSrc = m_pPlayer->GetGunPosition( );;
		Vector vecAiming = gpGlobals->v_forward;

		TraceResult tr;
		UTIL_TraceLine ( vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr );
		
		UTIL_SetOrigin( m_pSpot->pev, tr.vecEndPos );
	}
#endif

}


class CRpgAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_rpgammo.mdl");
		CBasePlayerAmmo::Spawn( );
		pev->body = RANDOM_FLOAT( 0, 1 );	// sic: almost always 0
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_rpgammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_RPGCLIP_GIVE, "rockets", ROCKET_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_rpgclip, CRpgAmmo );

#endif