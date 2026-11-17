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
//=========================================================
// vortex.cpp - Scientist Slaughterhouse's vortex launcher: vortex_blast,
// vortex_missile, bh_rocket, weapon_vortexlauncher, ammo_vortexclip
// (file name ours)
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "effects.h"
#include "hornet.h"
#include "soundent.h"
#include "gamerules.h"


// the vortex launcher uses the RPG's view model; names ours
enum vortexlauncher_e {
	VORTEX_IDLE = 0,
	VORTEX_FIDGET,
	VORTEX_RELOAD,		// to reload
	VORTEX_FIRE2,		// to empty
	VORTEX_HOLSTER1,	// loaded
	VORTEX_DRAW1,		// loaded
	VORTEX_HOLSTER2,	// unloaded
	VORTEX_DRAW_UL,		// unloaded
	VORTEX_IDLE_UL,		// unloaded idle
	VORTEX_FIDGET_UL,	// unloaded fidget
};

#ifndef CLIENT_DLL

//=========================================================
// vortex_blast: the vortex missile's explosion. A sprite
// (sprites/c-tele1.spr) that implodes, then ends in a beam
// cylinder and a grenade explosion of pev->dmg (250).
//=========================================================
class CVortexBlast : public CGrenade
{
public:
	void Spawn( void );
	void Precache( void );

	const char *GetKillDesc( void ) { return "a high-energy detonation"; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return fGibbed ? "annihilated" : "killed"; }
	BOOL CanBeRepulsed( bool fForce ) { return FALSE; }
	int HeatSignature( void ) { return -2; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void EXPORT VortexThink( void );

	float m_lastTime;
	float m_maxFrame;
};
LINK_ENTITY_TO_CLASS( vortex_blast, CVortexBlast );

TYPEDESCRIPTION	CVortexBlast::m_SaveData[] =
{
	DEFINE_FIELD( CVortexBlast, m_lastTime, FIELD_TIME ),
	DEFINE_FIELD( CVortexBlast, m_maxFrame, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CVortexBlast, CGrenade );

void CVortexBlast::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;
	pev->takedamage = DAMAGE_NO;

	Precache( );
	SET_MODEL( ENT(pev), "sprites/c-tele1.spr" );
	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;

	m_lastTime = gpGlobals->time;
	SetThink( VortexThink );
	pev->nextthink = 0.1;	// sic: not gpGlobals->time + 0.1, it thinks at once
}

void CVortexBlast::Precache( void )
{
	PRECACHE_MODEL( "sprites/c-tele1.spr" );
}

void CVortexBlast::VortexThink( void )
{
	if ( pev->frame == 0 )
	{
		CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin, pev->dmg * 5, 3.0 );

		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_IMPLOSION );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_BYTE( 255 );	// radius
			WRITE_BYTE( RANDOM_LONG( 160, 255 ) );	// count
			WRITE_BYTE( 12 );	// life
		MESSAGE_END();
	}

	pev->nextthink = gpGlobals->time + 0.1;

	// animate at 20 frames a second
	pev->frame += ( gpGlobals->time - m_lastTime ) * 20;
	m_lastTime = gpGlobals->time;

	if ( pev->frame >= m_maxFrame )
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_BEAMCYLINDER );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z + pev->dmg * 5 ); // reach damage radius over .2 seconds
			WRITE_SHORT( g_sModelIndexLaser );
			WRITE_BYTE( 0 ); // startframe
			WRITE_BYTE( 0 ); // framerate
			WRITE_BYTE( 4 ); // life
			WRITE_BYTE( 32 );  // width
			WRITE_BYTE( 0 );   // noise
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 192 );   // r, g, b
			WRITE_BYTE( 128 );   // r, g, b
			WRITE_BYTE( 128 ); // brightness
			WRITE_BYTE( 0 );		// speed
		MESSAGE_END();

		TraceResult tr;

		UTIL_TraceLine( pev->origin, pev->origin + pev->movedir, ignore_monsters, ENT(pev), &tr );
		Explode( &tr, DMG_BLAST );
		UTIL_Remove( this );
	}
}

//=========================================================
// vortex_missile: the vortex launcher's primary missile, a
// hornet that seeks what it sees (Look 512), at most three
// on one target, and turns into a vortex_blast where it hits
//=========================================================
class CVortexRocket : public CHornet
{
public:
	void Spawn( void );
	void Precache( void );
	void Killed( entvars_t *pevAttacker, int iGib ) { CBaseEntity::Killed( pevAttacker, iGib ); }
	int  Classify( void ) { return CLASS_NONE; }
	int  HeatSignature( void ) { return 2; }
	int  IRelationship( CBaseEntity *pTarget );
	short CountSeekers( CBaseEntity *pTarget );	// name ours

	static CVortexRocket *CreateVortexRocket( Vector vecOrigin, Vector vecAngles, edict_t *pOwner );	// name ours

	void EXPORT VortexTouch( CBaseEntity *pOther );
	void EXPORT SeekThink( void );

	int m_iTrail;	// name ours
};
LINK_ENTITY_TO_CLASS( vortex_missile, CVortexRocket );

void CVortexRocket::Precache( void )
{
	PRECACHE_MODEL( "models/rpgrocket.mdl" );
	m_iTrail = PRECACHE_MODEL( "sprites/LRed.spr" );
	PRECACHE_SOUND( "weapons/rocket1.wav" );
	UTIL_PrecacheOther( "vortex_blast" );
}

CVortexRocket *CVortexRocket::CreateVortexRocket( Vector vecOrigin, Vector vecAngles, edict_t *pOwner )
{
	// sic: no classname
	CVortexRocket *pRocket = GetClassPtr( (CVortexRocket *)NULL );

	pRocket->pev->velocity = gpGlobals->v_forward * 200;
	UTIL_SetOrigin( pRocket->pev, vecOrigin );
	pRocket->pev->angles = vecAngles;
	pRocket->Spawn();
	pRocket->pev->owner = pOwner;

	return pRocket;
}

//=========================================================
// IRelationship - hates everything but the player's side;
// one level less for each missile already after the target
//=========================================================
int CVortexRocket::IRelationship( CBaseEntity *pTarget )
{
	int iEnemy[20] =
	{
		R_NO,	// CLASS_NONE
		R_HT,	// CLASS_MACHINE
		R_AL,	// CLASS_PLAYER
		R_HT,	// CLASS_HUMAN_PASSIVE
		R_HT,	// CLASS_HUMAN_MILITARY
		R_HT,	// CLASS_ALIEN_MILITARY
		R_HT,	// CLASS_ALIEN_PASSIVE
		R_HT,	// CLASS_ALIEN_MONSTER
		R_HT,	// CLASS_ALIEN_PREY
		R_HT,	// CLASS_ALIEN_PREDATOR
		R_NO,	// CLASS_INSECT
		R_DL,	// CLASS_PLAYER_ALLY
		R_NO,	// CLASS_PLAYER_BIOWEAPON
		R_DL,	// CLASS_ALIEN_BIOWEAPON
		R_AL,	// 14
		R_HT,	// 15
		R_HT,	// 16
		R_HT,	// 17
		R_HT,	// 18
		R_DL,	// 19
	};

	short iRelation = iEnemy[ pTarget->Classify() ] - CountSeekers( pTarget );

	if ( iRelation < 0 )
		return R_NO;

	return iRelation;
}

//=========================================================
// CountSeekers - how many entities of this class are after
// pTarget (three at most)
//=========================================================
short CVortexRocket::CountSeekers( CBaseEntity *pTarget )
{
	short iCount = 0;
	CBaseEntity *pEntity = NULL;

	while ( ( pEntity = UTIL_FindEntityByClassname( pEntity, STRING( pev->classname ) ) ) != NULL )
	{
		CBaseMonster *pMonster = pEntity->MyMonsterPointer();

		if ( pMonster && pMonster->m_hEnemy == pTarget )
		{
			iCount++;
			if ( iCount >= 3 )
				break;
		}
	}

	return iCount;
}

void CVortexRocket::Spawn( void )
{
	Precache( );

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_NO;
	pev->effects = EF_LIGHT;
	pev->rendercolor = Vector( 255, 0, 128 );
	pev->renderfx = kRenderFxGlowShell;

	m_flFieldOfView = 0.9;
	m_flFlySpeed = 1000;

	SET_MODEL( ENT(pev), "models/rpgrocket.mdl" );
	UTIL_SetSize( pev, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ) );

	UTIL_MakeVectors( pev->angles );
	m_vecEnemyLKP = pev->origin + gpGlobals->v_forward * 512;

	// rocket trail
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() );	// entity
		WRITE_SHORT( m_iTrail );	// model
		WRITE_BYTE( 40 ); // life
		WRITE_BYTE( 5 );  // width
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );	// brightness
	MESSAGE_END();

	EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5 );

	SetTouch( VortexTouch );
	SetThink( SeekThink );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CVortexRocket::VortexTouch( CBaseEntity *pOther )
{
	STOP_SOUND( ENT(pev), CHAN_VOICE, "weapons/rocket1.wav" );

	if ( pOther->edict() == pev->owner )
	{
		pev->solid = SOLID_NOT;
		return;
	}

	CBaseEntity *pBlast = CBaseEntity::Create( "vortex_blast", pev->origin, pev->angles, pev->owner );
	pBlast->pev->dmg = pev->dmg;
	pBlast->pev->movedir = pev->velocity.Normalize() * 32;
	pBlast->pev->scale = pev->dmg / 250;

	UTIL_Remove( this );
}

void CVortexRocket::SeekThink( void )
{
	Vector	vecDirToEnemy;

	if ( m_hEnemy == NULL )
	{
		Look( 512 );
		m_hEnemy = BestVisibleEnemy( );
	}

	if ( m_hEnemy != NULL && FVisible( m_hEnemy ) )
	{
		m_vecEnemyLKP = m_hEnemy->BodyTarget( pev->origin );
	}
	else
	{
		m_vecEnemyLKP = m_vecEnemyLKP + pev->velocity * m_flFlySpeed * 0.1;
		m_hEnemy = NULL;
	}

	vecDirToEnemy = ( m_vecEnemyLKP - pev->origin ).Normalize();

	// the new speed: 80% of the old one, plus a fifth of the top speed (at
	// least 100) while below 80% of it; a fifth of the old velocity stays
	float flSpeed = pev->velocity.Length();
	if ( flSpeed < m_flFlySpeed * 0.8 )
	{
		float flAccel = m_flFlySpeed * 0.2;
		flSpeed = flSpeed * 0.8 + max( 100, flAccel );
		flSpeed = min( flSpeed, m_flFlySpeed );
		pev->velocity = pev->velocity * 0.2 + vecDirToEnemy * flSpeed;
	}
	else
	{
		flSpeed = flSpeed * 0.8;
		pev->velocity = pev->velocity * 0.2 + vecDirToEnemy * flSpeed;
	}

	float flMaxSpeed = m_flFlySpeed;
	if ( pev->velocity.Length() > flMaxSpeed )
		pev->velocity = pev->velocity.Normalize() * flMaxSpeed;

	pev->nextthink = gpGlobals->time + 0.1;
	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->solid = SOLID_BBOX;
}

//=========================================================
// bh_rocket: the vortex launcher's secondary missile, flies
// straight (750 units/s) and opens a black_hole where it
// hits or when the launcher's secondary attack fires again
//=========================================================
class CBlackHoleRocket : public CBaseEntity	// name ours
{
public:
	void Spawn( void );
	void Precache( void );
	void Touch( CBaseEntity *pOther );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int m_iTrail;	// name ours
};
LINK_ENTITY_TO_CLASS( bh_rocket, CBlackHoleRocket );

void CBlackHoleRocket::Spawn( void )
{
	Precache( );

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_YES;
	pev->health = 10;
	pev->effects = EF_LIGHT;
	pev->rendercolor = Vector( 255, 0, 128 );
	pev->renderfx = kRenderFxGlowShell;

	SET_MODEL( ENT(pev), "models/rpgrocket.mdl" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );

	UTIL_MakeVectors( pev->angles );
	pev->velocity = gpGlobals->v_forward * 750;

	// rocket trail
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() );	// entity
		WRITE_SHORT( m_iTrail );	// model
		WRITE_BYTE( 40 ); // life
		WRITE_BYTE( 5 );  // width
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );	// brightness
	MESSAGE_END();

	EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5 );
}

void CBlackHoleRocket::Precache( void )
{
	PRECACHE_MODEL( "models/rpgrocket.mdl" );
	m_iTrail = PRECACHE_MODEL( "sprites/LRed.spr" );
	PRECACHE_SOUND( "weapons/rocket1.wav" );
	UTIL_PrecacheOther( "black_hole" );
}

void CBlackHoleRocket::Touch( CBaseEntity *pOther )
{
	STOP_SOUND( ENT(pev), CHAN_VOICE, "weapons/rocket1.wav" );
	CBaseEntity::Create( "black_hole", pev->origin, g_vecZero, pev->owner );
	UTIL_Remove( this );
}

void CBlackHoleRocket::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	Touch( pActivator );
}

#endif

LINK_ENTITY_TO_CLASS( weapon_vortexlauncher, CVortexLauncher );

void CVortexLauncher::Reload( void )
{
	if ( m_iClip == 1 )
	{
		// don't bother with any of this if don't need to reload.
		return;
	}

	if ( DefaultReload( RPG_MAX_CLIP, VORTEX_RELOAD, 2 ) )
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT( 10, 15 );
}

void CVortexLauncher::Spawn( void )
{
	Precache( );
	m_iId = WEAPON_VORTEXLAUNCHER;

	SET_MODEL( ENT(pev), "models/w_vortexlauncher.mdl" );
	pev->skin = 1;

	m_iDefaultAmmo = RPG_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CVortexLauncher::Precache( void )
{
	PRECACHE_MODEL( "models/w_vortexlauncher.mdl" );
	PRECACHE_MODEL( "models/v_rpg.mdl" );
	PRECACHE_MODEL( "models/p_vortexlauncher.mdl" );

	PRECACHE_SOUND( "items/9mmclip1.wav" );

	UTIL_PrecacheOther( "vortex_missile" );
	UTIL_PrecacheOther( "bh_rocket" );

	PRECACHE_SOUND( "weapons/rocketfire1.wav" );

	m_usRpg = PRECACHE_EVENT( 1, "events/rpg.sc" );

#ifndef CLIENT_DLL
	// precached again while held (a restored game): put the glow back
	if ( m_pPlayer && m_pPlayer->m_pActiveItem == this && !m_pSprite )
		MakeSprite();
#endif
}

int CVortexLauncher::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "vortex_m";
	p->iMaxAmmo1 = VORTEX_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = RPG_MAX_CLIP;
	p->iSlot = 6;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_VORTEXLAUNCHER;
	p->iFlags = 0;
	p->iWeight = VORTEXLAUNCHER_WEIGHT;

	return 1;
}

int CVortexLauncher::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CVortexLauncher::Deploy( void )
{
#ifndef CLIENT_DLL
	if ( !m_pSprite )
		MakeSprite();
#endif

	if ( m_iClip == 0 )
	{
		return DefaultDeploy( "models/v_rpg.mdl", "models/p_vortexlauncher.mdl", VORTEX_DRAW_UL, "rpg" );
	}

	return DefaultDeploy( "models/v_rpg.mdl", "models/p_vortexlauncher.mdl", VORTEX_DRAW1, "rpg" );
}

void CVortexLauncher::Holster( int skiplocal /* = 0 */ )
{
#ifndef CLIENT_DLL
	if ( m_pSprite )
	{
		UTIL_Remove( m_pSprite );
		m_pSprite = NULL;
	}
#endif

	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	SendWeaponAnim( VORTEX_HOLSTER1 );
}

//=========================================================
// PrimaryAttack - a vortex missile (vortex_missile, 250
// damage)
//=========================================================
void CVortexLauncher::PrimaryAttack( void )
{
	if ( m_iClip )
	{
		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		int flags;
#if defined( CLIENT_WEAPONS )
		flags = FEV_NOTHOST;
#else
		flags = 0;
#endif

		PLAYBACK_EVENT( flags, m_pPlayer->edict(), m_usRpg );

#ifndef CLIENT_DLL
		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		Vector vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -8;

		CVortexRocket *pRocket = CVortexRocket::CreateVortexRocket( vecSrc, m_pPlayer->pev->v_angle, m_pPlayer->edict() );
		pRocket->pev->dmg = 250;
#endif

		m_iClip--;

		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
	}
	else
	{
		PlayEmptySound( );
	}
}

//=========================================================
// SecondaryAttack - a black hole rocket (bh_rocket); while
// one flies, opens its black hole
//=========================================================
void CVortexLauncher::SecondaryAttack( void )
{
	if ( m_hRocket )
	{
#ifndef CLIENT_DLL
		m_hRocket->Use( m_pPlayer, this, USE_TOGGLE, 0 );
#endif
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.5;
		return;
	}

	if ( m_iClip )
	{
		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		int flags;
#if defined( CLIENT_WEAPONS )
		flags = FEV_NOTHOST;
#else
		flags = 0;
#endif

		PLAYBACK_EVENT( flags, m_pPlayer->edict(), m_usRpg );

#ifndef CLIENT_DLL
		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		Vector vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -8;

		m_hRocket = CBaseEntity::Create( "bh_rocket", vecSrc, m_pPlayer->pev->v_angle, m_pPlayer->edict() );
#endif

		m_iClip--;

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.5;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
	}
	else
	{
		PlayEmptySound( );
	}
}

//=========================================================
// MakeSprite - the launcher's blue glow, on the player's
// first attachment (server only)
//=========================================================
void CVortexLauncher::MakeSprite( void )
{
#ifndef CLIENT_DLL
	m_pSprite = CSprite::SpriteCreate( "sprites/blueflare1.spr", pev->origin, FALSE );
	m_pSprite->SetAttachment( m_pPlayer->edict(), 1 );
	m_pSprite->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
	m_pSprite->SetScale( 0.35 );
	m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;
#endif
}

void CVortexLauncher::WeaponIdle( void )
{
	ResetEmptySound( );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if ( flRand <= 0.75 )
		{
			if ( m_iClip == 0 )
				iAnim = VORTEX_IDLE_UL;
			else
				iAnim = VORTEX_IDLE;

			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 90.0 / 15.0;
		}
		else
		{
			if ( m_iClip == 0 )
				iAnim = VORTEX_FIDGET_UL;
			else
				iAnim = VORTEX_FIDGET;

			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0;
		}

		SendWeaponAnim( iAnim );
	}
	else
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1;
	}
}

#ifndef CLIENT_DLL

extern int gEvilImpulse101;

//=========================================================
// ammo_vortexclip: five vortex missiles, with a blue glow
// that goes with the ammo and comes back 20 seconds after
// it respawns
//=========================================================
class CVortexAmmo : public CBasePlayerAmmo	// name ours
{
	void Spawn( void );
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_rpgammo.mdl" );
		PRECACHE_MODEL( "sprites/blueflare1.spr" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	CBaseEntity *Respawn( void )
	{
		if ( !m_pSprite )
		{
			MakeSprite();

			CSprite *pSprite = m_pSprite;
			pSprite->TurnOff();
			pSprite->SetThink( &CSprite::TurnOnThink );
			pSprite->pev->nextthink = gpGlobals->time + 20;
		}

		return CBasePlayerAmmo::Respawn();
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if ( pOther->GiveAmmo( 5, "vortex_m", VORTEX_MAX_CARRY ) != -1 )
		{
			if ( m_pSprite )
			{
				UTIL_Remove( m_pSprite );
				m_pSprite = NULL;
			}

			EMIT_SOUND( ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
	void MakeSprite( void );	// name ours

	CSprite *m_pSprite;	// name ours
};
LINK_ENTITY_TO_CLASS( ammo_vortexclip, CVortexAmmo );

void CVortexAmmo::Spawn( void )
{
	Precache( );
	SET_MODEL( ENT(pev), "models/w_rpgammo.mdl" );
	CBasePlayerAmmo::Spawn( );

	pev->skin = 1;

	if ( gEvilImpulse101 )
	{
		pev->sequence = 1;
	}
	else
	{
		pev->sequence = 0;
		MakeSprite();
	}
}

void CVortexAmmo::MakeSprite( void )
{
	m_pSprite = CSprite::SpriteCreate( "sprites/blueflare1.spr", pev->origin, FALSE );
	m_pSprite->SetAttachment( edict(), 1 );
	m_pSprite->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNone );
	m_pSprite->SetScale( 0.5 );
	m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;
}

#endif
