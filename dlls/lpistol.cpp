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
/*

  The laser pistol (weapon_laserhandgun, the glock's models with a glow
  shell) fires laser pulses from a 24-shot clip of uranium; its secondary
  fire empties a full clip into one special_laser, a "forceweb" pulse that
  grabs what it hits; +attack3 switches the emitter between red, green and
  blue, which picks what the forceweb does. ammo_laserclip gives 20
  uranium.

*/

#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

// the glock's view model sequences (names ours)
enum lpistol_e {
	LPISTOL_IDLE1 = 0,
	LPISTOL_IDLE2,
	LPISTOL_IDLE3,
	LPISTOL_SHOOT,
	LPISTOL_SHOOT_EMPTY,
	LPISTOL_RELOAD,
	LPISTOL_RELOAD_NOT_EMPTY,
	LPISTOL_DRAW,
	LPISTOL_HOLSTER,
	LPISTOL_ADD_SILENCER
};

#define LASERPISTOL_MAX_CLIP		24	// names ours
#define LASERPISTOL_DEFAULT_GIVE	20
#define AMMO_LASERCLIP_GIVE			20

#ifndef CLIENT_DLL

// special_laser: the secondary fire's pulse. pev->weapons is the emitter's color
// (0 red, 1 green, 2 blue), which picks what it does to a monster it hits: red
// carries it off and throws it at the next wall, green spins it up into the air
// and gibs it when it falls, blue drills it into the ground.
class CLaserSpecial : public CLaserPulse
{
public:
	void Spawn( void );
	const char *GetKillDesc( void ) { return "a forceweb pulse"; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup )
	{
		iHitGroup = 0;
		switch ( pev->weapons )
		{
		case 1:
			return "imploded";
		case 2:
			return "drilled";
		default:
			return "smashed into a wall";
		}
	}

	// name ours
	static CLaserSpecial *Shoot( Vector vecOrigin, edict_t *pOwner, const Vector &vecColor, int iTrailLife, float flScale );

	void EXPORT SpecialTouch( CBaseEntity *pOther );
	void EXPORT FlyThink( void );
	void EXPORT PinballThink( void );
	void EXPORT PinballTouch( CBaseEntity *pOther );
	void EXPORT SpinThink( void );
	void EXPORT DrillThink( void );
};

LINK_ENTITY_TO_CLASS( special_laser, CLaserSpecial );

CLaserSpecial *CLaserSpecial::Shoot( Vector vecOrigin, edict_t *pOwner, const Vector &vecColor, int iTrailLife, float flScale )
{
	CLaserSpecial *pLaser = GetClassPtr( (CLaserSpecial *)NULL );

	UTIL_SetOrigin( pLaser->pev, vecOrigin );
	pLaser->pev->iuser1 = iTrailLife;
	pLaser->pev->owner = pOwner;
	pLaser->pev->rendercolor = vecColor;
	pLaser->pev->scale = flScale;
	pLaser->Spawn( );
	pLaser->pev->classname = MAKE_STRING( "special_laser" );

	if ( vecColor.y == 255 )
		pLaser->pev->weapons = 1;
	else if ( vecColor.z == 255 )
		pLaser->pev->weapons = 2;

	return pLaser;
}

void CLaserSpecial::Spawn( void )
{
	CLaserPulse::Spawn( );

	SetTouch( SpecialTouch );
	SetThink( FlyThink );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CLaserSpecial::SpecialTouch( CBaseEntity *pOther )
{
	if ( pOther->pev->takedamage && !pOther->Shielded( ) && pOther->CanBeRepulsed( false ) )
	{
		m_hEnemy = pOther;

		pOther->pev->rendercolor = pev->rendercolor;
		pOther->pev->renderamt = 64;
		pOther->pev->renderfx = kRenderFxGlowShell;

		pev->velocity = g_vecZero;
		pev->effects = EF_NODRAW;
		pev->nextthink = gpGlobals->time + 0.1;

		switch ( pev->weapons )
		{
		default:
			SetTouch( PinballTouch );
			SetThink( PinballThink );
			pev->velocity = Vector( RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ) ).Normalize( ) * 1000;
			pOther->pev->movetype = MOVETYPE_NOCLIP;
			pOther->pev->solid = SOLID_NOT;
			pOther->pev->nextthink = 0;
			pOther->SetThink( NULL );
			pOther->SetTouch( NULL );
			pev->movetype = MOVETYPE_BOUNCEMISSILE;
			pev->friction = 0;
			UTIL_SetSize( pev, pOther->pev->mins, pOther->pev->maxs );
			return;
		case 2:
			SetThink( DrillThink );
			pOther->pev->velocity = Vector( 0, 0, -50 );
			pOther->pev->avelocity = Vector( 0, 2000, 0 );
			pOther->pev->movetype = MOVETYPE_NOCLIP;
			EMIT_SOUND( ENT( pOther->pev ), CHAN_VOICE, "ambience/screammachine.wav", 1, 0.8 );
			return;
		case 1:
			SetThink( SpinThink );
			pOther->pev->velocity = Vector( 0, 0, 500 );
			pOther->pev->avelocity = Vector( RANDOM_FLOAT( -500, 500 ), 2000, 0 );
			pOther->pev->movetype = MOVETYPE_TOSS;
			EMIT_SOUND( ENT( pOther->pev ), CHAN_VOICE, "ambience/screammachine.wav", 1, 0.8 );
			return;
		}
	}

	LaserTouch( pOther );
}

void CLaserSpecial::FlyThink( void )
{
	pev->scale = RANDOM_FLOAT( 0.25, 0.5 );
	pev->nextthink = gpGlobals->time + 0.1;
}

// red: the victim rides along with the pulse, which bounces around until it hits something
void CLaserSpecial::PinballThink( void )
{
	if ( m_hEnemy == NULL )
	{
		UTIL_Remove( this );
		return;
	}

	m_hEnemy->pev->origin = pev->origin;
	m_hEnemy->pev->velocity = pev->velocity;
	pev->nextthink = gpGlobals->time + 0.01;
}

void CLaserSpecial::PinballTouch( CBaseEntity *pOther )
{
	if ( m_hEnemy != NULL )
	{
		if ( UTIL_PointContents( pev->origin ) == CONTENTS_SKY )
		{
			m_hEnemy->TakeDamage( pev, VARS( pev->owner ), m_hEnemy->pev->max_health > 200 ? m_hEnemy->pev->max_health : 200, DMG_ENERGYBEAM | DMG_ALWAYSGIB );
		}
		else
		{
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/shock_discharge.wav", 1, 0.3 );

			if ( pOther->pev->takedamage )
			{
				// thrown at another monster: it takes the victim's full health in damage
				pOther->TakeDamage( m_hEnemy->pev, VARS( pev->owner ), m_hEnemy->pev->max_health, DMG_ENERGYBEAM | DMG_ALWAYSGIB );
				m_hEnemy->TakeDamage( pev, VARS( pev->owner ), m_hEnemy->pev->max_health > 200 ? m_hEnemy->pev->max_health : 200, DMG_ENERGYBEAM | DMG_ALWAYSGIB );
			}
			else if ( m_hEnemy->pev->health > 1 )
			{
				// against a wall: one point per bounce, the pulse keeps going
				m_hEnemy->TakeDamage( pev, VARS( pev->owner ), 1, DMG_ENERGYBEAM );
				return;
			}
			else
			{
				m_hEnemy->TakeDamage( pev, VARS( pev->owner ), 200, DMG_ENERGYBEAM | DMG_ALWAYSGIB );
			}
		}
	}

	UTIL_Remove( this );
}

// green: gibs the victim once it stops rising
void CLaserSpecial::SpinThink( void )
{
	if ( m_hEnemy == NULL )
	{
		UTIL_Remove( this );
		return;
	}

	pev->nextthink = gpGlobals->time + 0.1;

	if ( m_hEnemy->pev->velocity.z <= 0 )
	{
		m_hEnemy->TakeDamage( pev, VARS( pev->owner ), m_hEnemy->pev->health + 200, DMG_ENERGYBEAM | DMG_ALWAYSGIB );
		SetThink( SUB_Remove );
	}
}

// blue: the victim bleeds while it sinks, and dies (fading out) when its top is in the ground
void CLaserSpecial::DrillThink( void )
{
	if ( m_hEnemy == NULL )
	{
		UTIL_Remove( this );
		return;
	}

	pev->nextthink = gpGlobals->time + 0.1;

	Vector vecTop = m_hEnemy->pev->origin + Vector( 0, 0, m_hEnemy->pev->maxs.z );

	if ( UTIL_PointContents( vecTop ) == CONTENTS_SOLID )
	{
		m_hEnemy->TakeDamage( pev, VARS( pev->owner ), m_hEnemy->pev->health, DMG_ENERGYBEAM | DMG_FADE );
		SetThink( SUB_Remove );
		return;
	}

	TraceResult tr;
	Vector vecBottom = m_hEnemy->pev->origin + Vector( 0, 0, m_hEnemy->pev->mins.z );
	UTIL_TraceLine( vecTop, vecBottom, ignore_monsters, ENT( pev ), &tr );
	SpawnBlood( tr.vecEndPos, m_hEnemy->BloodColor( ), 100 );
}

LINK_ENTITY_TO_CLASS( weapon_laserhandgun, CLaserPistol );

#endif

void CLaserPistol::Spawn( )
{
	Precache( );
	m_iId = WEAPON_LASERPISTOL;
	SET_MODEL( ENT( pev ), "models/w_9mmhandgun.mdl" );

	pev->body = 1;
	pev->rendercolor = Vector( 255, 0, 0 );
	pev->renderfx = kRenderFxGlowShell;

	m_iDefaultAmmo = LASERPISTOL_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CLaserPistol::Precache( void )
{
	PRECACHE_MODEL( "models/v_9mmhandgun.mdl" );
	PRECACHE_MODEL( "models/w_9mmhandgun.mdl" );
	PRECACHE_MODEL( "models/p_9mmhandgun.mdl" );

	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND( "items/9mmclip2.wav" );

	PRECACHE_SOUND( "weapons/pl_gun1.wav" );
	PRECACHE_SOUND( "weapons/pl_gun2.wav" );

	UTIL_PrecacheOther( "laser_pulse" );
	PRECACHE_SOUND( "ambience/screammachine.wav" );
	PRECACHE_SOUND( "weapons/shock_discharge.wav" );

	m_usLaserGun = PRECACHE_EVENT( 1, "events/lasergun.sc" );
}

int CLaserPistol::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "uranium";
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = LASERPISTOL_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_LASERPISTOL;
	p->iWeight = LASERPISTOL_WEIGHT;

	return 1;
}

BOOL CLaserPistol::Deploy( )
{
	return DefaultDeploy( "models/v_9mmhandgun.mdl", "models/p_9mmhandgun.mdl", LPISTOL_DRAW, "onehanded", 0 );
}

void CLaserPistol::PrimaryAttack( void )
{
	if ( m_iClip <= 0 )
	{
		if ( m_fFireOnEmpty )
		{
			PlayEmptySound( );
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase( ) + 0.2;
		}

		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict( ), m_usLaserGun, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#ifndef CLIENT_DLL
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	Vector vecSrc = m_pPlayer->GetGunPosition( ) - gpGlobals->v_up * 8 + gpGlobals->v_right * 2;
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	CLaserPulse *pPulse = CLaserPulse::Shoot( vecSrc, m_pPlayer->edict( ), pev->rendercolor, 2, 0.25, 0 );
	pPulse->pev->dmg = gSkillData.plrDmg9MM * 1.4;
	pPulse->pev->velocity = vecAiming * 2000;
	pPulse->pev->iuser2 = DMG_ENERGYBEAM | DMG_NEVERGIB;
#endif

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase( ) + 0.3;

	if ( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

// the forceweb: a full clip in one pulse
void CLaserPistol::SecondaryAttack( void )
{
	if ( m_iClip < LASERPISTOL_MAX_CLIP )
	{
		if ( m_fFireOnEmpty )
		{
			PlayEmptySound( );
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase( ) + 0.2;
		}

		return;
	}

	m_iClip = 0;

	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict( ), m_usLaserGun, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#ifndef CLIENT_DLL
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	Vector vecSrc = m_pPlayer->GetGunPosition( ) - gpGlobals->v_up * 8 + gpGlobals->v_right * 2;
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	CLaserSpecial *pLaser = CLaserSpecial::Shoot( vecSrc, m_pPlayer->edict( ), pev->rendercolor, 2, 0.25 );
	pLaser->pev->dmg = gSkillData.plrDmg9MM * 2.8;
	pLaser->pev->velocity = vecAiming * 1400;
	pLaser->pev->iuser2 = DMG_ENERGYBEAM | DMG_NEVERGIB;
#endif

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase( ) + 1;

	if ( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

// +attack3: unloads the clip and switches the emitter's color, red, green, blue
void CLaserPistol::WeaponSpecial( void )
{
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase( ) + 2;

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < iMaxAmmo1( ) )
	{
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] += m_iClip;
		if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > iMaxAmmo1( ) )
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = iMaxAmmo1( );
	}

	m_iClip = 0;

#ifndef CLIENT_DLL
	const char *pszColor = "Red";

	if ( pev->rendercolor.x != 0 )
	{
		pev->rendercolor = Vector( 0, 255, 0 );
		pszColor = "Green";
	}
	else if ( pev->rendercolor.y != 0 )
	{
		pev->rendercolor = Vector( 0, 0, 255 );
		pszColor = "Blue";
	}
	else
	{
		pev->rendercolor = Vector( 255, 0, 0 );
	}

	m_pPlayer->pev->rendercolor = pev->rendercolor;
	ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "%s emitter selected", pszColor );
#endif

	Reload( );
}

void CLaserPistol::Reload( void )
{
	if ( DefaultReload( LASERPISTOL_MAX_CLIP, LPISTOL_RELOAD, 1.5 ) )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
}

void CLaserPistol::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase( ) )
		return;

	// only idle if the slid isn't back
	if ( m_iClip != 0 )
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

		if ( flRand <= 0.3 + 0 * 0.75 )
		{
			iAnim = LPISTOL_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 49.0 / 16;
		}
		else if ( flRand <= 0.6 + 0 * 0.875 )
		{
			iAnim = LPISTOL_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 60.0 / 16.0;
		}
		else
		{
			iAnim = LPISTOL_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 40.0 / 16.0;
		}
		SendWeaponAnim( iAnim, 1 );
	}
}


class CLaserAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL( ENT( pev ), "models/w_9mmclip.mdl" );
		CBasePlayerAmmo::Spawn( );
		pev->rendercolor = Vector( 255, 0, 0 );
		pev->renderfx = kRenderFxGlowShell;
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_9mmclip.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if ( pOther->GiveAmmo( AMMO_LASERCLIP_GIVE, "uranium", URANIUM_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_laserclip, CLaserAmmo );

#endif
