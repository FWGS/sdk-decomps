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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "customentity.h"

enum satchel_e {
	SATCHEL_IDLE1 = 0,
	SATCHEL_FIDGET1,
	SATCHEL_DRAW,
	SATCHEL_DROP
};

enum satchel_radio_e {
	SATCHEL_RADIO_IDLE1 = 0,
	SATCHEL_RADIO_FIDGET1,
	SATCHEL_RADIO_DRAW,
	SATCHEL_RADIO_FIRE,
	SATCHEL_RADIO_HOLSTER
};



#ifndef CLIENT_DLL
class CSatchelCharge : public CGrenade
{
	void Spawn( void );
	void Precache( void );
	void BounceSound( void );

	void EXPORT SatchelSlide( CBaseEntity *pOther );
	void EXPORT SatchelThink( void );

public:
	void Deactivate( void );
	void StartSatellite( void );	// name ours
	void EXPORT SatelliteThink( void );
};
LINK_ENTITY_TO_CLASS( monster_satchel, CSatchelCharge );

//=========================================================
// Deactivate - do whatever it is we do to an orphaned 
// satchel when we don't want it in the world anymore.
//=========================================================
void CSatchelCharge::Deactivate( void )
{
	pev->solid = SOLID_NOT;
	UTIL_Remove( this );
}


void CSatchelCharge :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/w_satchel.mdl");
	//UTIL_SetSize(pev, Vector( -16, -16, -4), Vector(16, 16, 32));	// Old box -- size of headcrab monsters/players get blocked by this
	UTIL_SetSize(pev, Vector( -4, -4, -4), Vector(4, 4, 4));	// Uses point-sized, and can be stepped over
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( SatchelSlide );
	SetUse( DetonateUse );
	SetThink( SatchelThink );
	pev->nextthink = gpGlobals->time + 0.1;
	pev->dmgtime = gpGlobals->time + 5;	// a satellite rain charge calls the strike after 5 seconds

	pev->gravity = 0.5;
	pev->friction = 0.8;

	pev->dmg = gSkillData.plrDmgSatchel;
	// ResetSequenceInfo( );
	pev->sequence = 1;

	m_iszKillDesc = MAKE_STRING( "a remote activated demolition unit" );
}


void CSatchelCharge::SatchelSlide( CBaseEntity *pOther )
{
	entvars_t	*pevOther = pOther->pev;

	// don't hit the guy that launched this grenade
	if ( pOther->edict() == pev->owner )
		return;

	// pev->avelocity = Vector (300, 300, 300);
	pev->gravity = 1;// normal gravity now

	// HACKHACK - On ground isn't always set, so look for ground underneath
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin - Vector(0,0,10), ignore_monsters, edict(), &tr );

	if ( tr.flFraction < 1.0 )
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.95;
		pev->avelocity = pev->avelocity * 0.9;
		// play sliding sound, volume based on velocity
	}
	if ( !(pev->flags & FL_ONGROUND) && pev->velocity.Length2D() > 10 )
	{
		BounceSound();
	}
	StudioFrameAdvance( );
}


void CSatchelCharge :: SatchelThink( void )
{
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	// satellite rain: pev->weapons is the number of strikes left
	if ( pev->weapons && pev->dmgtime <= gpGlobals->time )
	{
		StartSatellite();
		return;
	}

	if (pev->waterlevel == 3)
	{
		pev->movetype = MOVETYPE_FLY;
		pev->velocity = pev->velocity * 0.8;
		pev->avelocity = pev->avelocity * 0.9;
		pev->velocity.z += 8;
	}
	else if (pev->waterlevel == 0)
	{
		pev->movetype = MOVETYPE_BOUNCE;
	}
	else
	{
		pev->velocity.z -= 8;
	}	
}

void CSatchelCharge :: Precache( void )
{
	PRECACHE_MODEL("models/grenade.mdl");
	PRECACHE_SOUND("weapons/g_bounce1.wav");
	PRECACHE_SOUND("weapons/g_bounce2.wav");
	PRECACHE_SOUND("weapons/g_bounce3.wav");
	PRECACHE_MODEL("sprites/vslash.spr");
	PRECACHE_SOUND("weapons/mortarhit.wav");
}

void CSatchelCharge :: BounceSound( void )
{
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/g_bounce1.wav", 1, ATTN_NORM);	break;
	case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/g_bounce2.wav", 1, ATTN_NORM);	break;
	case 2:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/g_bounce3.wav", 1, ATTN_NORM);	break;
	}
}

//=========================================================
// StartSatellite - the charge's target designator: a laser
// to the sky; with sky above, the charge moves up there,
// hidden, and starts the orbital strike.
//=========================================================
void CSatchelCharge::StartSatellite( void )
{
	TraceResult tr;
	float flLife = 1.4;
	Vector vecEnd = pev->origin;

	vecEnd.z = 4000;
	UTIL_TraceLine( pev->origin, vecEnd, ignore_monsters, ENT( pev ), &tr );

	UTIL_BeamRing( TE_BEAMTORUS, g_sModelIndexLaser, pev->origin + Vector( 0, 0, 8 ), 128, 1, flLife, pev->rendercolor, 255 );

	CBeam *pBeam = CBeam::BeamCreate( g_pModelNameLaser, 40 );
	pBeam->PointsInit( pev->origin, tr.vecEndPos );
	pBeam->pev->rendercolor = pev->rendercolor;
	pBeam->SetBrightness( 255 );
	pBeam->SetThink( SUB_Remove );
	pBeam->pev->nextthink = gpGlobals->time + flLife;

	if ( UTIL_PointContents( tr.vecEndPos ) == CONTENTS_SKY )
	{
		UTIL_SetOrigin( pev, tr.vecEndPos );
		SetThink( SatelliteThink );
		pev->nextthink = gpGlobals->time + 5;

		pev->effects = EF_NODRAW;
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
		pev->velocity = g_vecZero;
		pev->movetype = MOVETYPE_NONE;

		pBeam->pev->angles.z = 4000;	// the beam's end goes to the sky
		pBeam->SetFlags( BEAM_FSHADEOUT );
		return;
	}

	CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
	if ( pOwner && pOwner->IsNetClient() )
		UTIL_SayText( "Target Designator Error: No sky above location!", pOwner );

	UTIL_Remove( this );
}

// name ours: the point flFraction of the way from vecSrc to vecEnd (an inline helper: retail calls its
// operators out of line, which needs them in a nested inline list)
inline Vector TracePoint( const Vector &vecSrc, const Vector &vecEnd, float flFraction )
{
	return vecSrc + flFraction * ( vecEnd - vecSrc );
}

//=========================================================
// SatelliteThink - one strike of the orbital plasma cannon
// within 512 units of the charge, every 0.1 to 2 seconds,
// until pev->weapons runs out
//=========================================================
void CSatchelCharge::SatelliteThink( void )
{
	TraceResult tr;

	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.1, 2 );

	UTIL_MakeVectors( Vector( 0, RANDOM_FLOAT( 0, 360 ), 0 ) );
	Vector vecSky;
	vecSky = pev->origin + gpGlobals->v_forward * RANDOM_FLOAT( 0, 512 );

	if ( UTIL_PointContents( vecSky ) != CONTENTS_SKY )
	{
		UTIL_TraceLine( pev->origin, vecSky, ignore_monsters, ENT( pev ), &tr );
		if ( tr.flFraction < 1 )
			vecSky = TracePoint( pev->origin, tr.vecEndPos, tr.flFraction );
	}

	vecSky.z = 4000;
	Vector vecGround = vecSky;
	vecGround.z = -4000;
	UTIL_TraceLine( vecSky, vecGround, ignore_monsters, ENT( pev ), &tr );

	if ( tr.fAllSolid || tr.flFraction >= 1 || UTIL_PointContents( tr.vecEndPos ) == CONTENTS_SKY )
	{
		// nothing to hit, try again
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	vecGround = tr.vecEndPos;

	CGrenade *pBlast = CGrenade::ShootExplosion( VARS( pev->owner ), vecGround, 0.1, Vector( 0, 0, -1 ) );
	pBlast->pev->effects = 0;
	pBlast->pev->rendermode = kRenderTransAdd;
	pBlast->pev->renderamt = 0;
	pBlast->pev->spawnflags = 32;
	pBlast->pev->rendercolor = Vector( 255, 100, 16 );
	pBlast->pev->dmg = 150;
	pBlast->m_iszKillDesc = MAKE_STRING( "Orbital Plasma Cannon" );

	CBeam *pBeam = CBeam::BeamCreate( "sprites/vslash.spr", 160 );
	pBeam->PointsInit( vecSky, vecGround );
	pBeam->SetFlags( BEAM_FSHADEIN );
	pBeam->SetColor( 255, 255, 255 );
	pBeam->SetBrightness( 255 );
	pBeam->SetScrollRate( 35 );
	pBeam->SetThink( SUB_Remove );
	pBeam->pev->nextthink = gpGlobals->time + 1;

	pev->weapons--;
	if ( pev->weapons < 1 )
		SetThink( SUB_Remove );
}


LINK_ENTITY_TO_CLASS( weapon_satchel, CSatchel );
#endif


//=========================================================
// CALLED THROUGH the newly-touched weapon's instance. The existing player weapon is pOriginal
//=========================================================
int CSatchel::AddDuplicate( CBasePlayerItem *pOriginal )
{
	CSatchel *pSatchel;

#ifdef CLIENT_DLL
	if ( bIsMultiplayer() )
#else
	if ( g_pGameRules->IsMultiplayer() )
#endif
	{
		pSatchel = (CSatchel *)pOriginal;

		if ( pSatchel->m_chargeReady != 0 )
		{
			// player has some satchels deployed. Refuse to add more.
			return FALSE;
		}
	}

	return CBasePlayerWeapon::AddDuplicate ( pOriginal );
}

//=========================================================
//=========================================================
int CSatchel::AddToPlayer( CBasePlayer *pPlayer )
{
	int bResult = CBasePlayerItem::AddToPlayer( pPlayer );

	pPlayer->pev->weapons |= (1<<m_iId);
	m_chargeReady = 0;// this satchel charge weapon now forgets that any satchels are deployed by it.

	if ( bResult )
	{
		return AddWeapon( );
	}
	return FALSE;
}

void CSatchel::Spawn( )
{
	Precache( );
	m_iId = WEAPON_SATCHEL;
	SET_MODEL(ENT(pev), "models/w_satchel.mdl");

	m_iDefaultAmmo = SATCHEL_DEFAULT_GIVE;
		
	FallInit();// get ready to fall down.
}


void CSatchel::Precache( void )
{
	PRECACHE_MODEL("models/v_satchel.mdl");
	PRECACHE_MODEL("models/v_satchel_radio.mdl");
	PRECACHE_MODEL("models/w_satchel.mdl");
	PRECACHE_MODEL("models/p_satchel.mdl");
	PRECACHE_MODEL("models/p_satchel_radio.mdl");

	UTIL_PrecacheOther( "monster_satchel" );
}


int CSatchel::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Satchel Charge";
	p->iMaxAmmo1 = SATCHEL_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 1;
	p->iFlags = ITEM_FLAG_SELECTONEMPTY | ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->iId = m_iId = WEAPON_SATCHEL;
	p->iWeight = SATCHEL_WEIGHT;

	return 1;
}

//=========================================================
//=========================================================
BOOL CSatchel::IsUseable( void )
{
	if ( m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] > 0 ) 
	{
		// player is carrying some satchels
		return TRUE;
	}

	if ( m_chargeReady != 0 )
	{
		// player isn't carrying any satchels, but has some out
		return TRUE;
	}

	return FALSE;
}

BOOL CSatchel::CanDeploy( void )
{
	if ( m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] > 0 ) 
	{
		// player is carrying some satchels
		return TRUE;
	}

	if ( m_chargeReady != 0 )
	{
		// player isn't carrying any satchels, but has some out
		return TRUE;
	}

	return FALSE;
}

BOOL CSatchel::Deploy( )
{

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	if ( m_chargeReady )
		return DefaultDeploy( "models/v_satchel_radio.mdl", "models/p_satchel_radio.mdl", SATCHEL_RADIO_DRAW, "hive" );
	else
		return DefaultDeploy( "models/v_satchel.mdl", "models/p_satchel.mdl", SATCHEL_DRAW, "trip" );

	
	return TRUE;
}


void CSatchel::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	
	if ( m_chargeReady )
	{
		SendWeaponAnim( SATCHEL_RADIO_HOLSTER );
	}
	else
	{
		SendWeaponAnim( SATCHEL_DROP );
	}
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);

	if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] && !m_chargeReady )
	{
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_SATCHEL);
		SetThink( DestroyItem );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}



void CSatchel::PrimaryAttack()
{
	switch (m_chargeReady)
	{
	case 0:
		{
		Throw( );
		}
		break;
	case 1:
		{
		SendWeaponAnim( SATCHEL_RADIO_FIRE );

#ifndef CLIENT_DLL
		edict_t *pPlayer = m_pPlayer->edict( );

		CBaseEntity *pSatchel = NULL;

		while ((pSatchel = UTIL_FindEntityByClassname( pSatchel, "monster_satchel" )) != NULL)
		{
			if (pSatchel->pev->owner == pPlayer)
			{
				pSatchel->Use( m_pPlayer, m_pPlayer, USE_ON, 0 );
				m_chargeReady = 2;
			}
		}
#endif

		m_chargeReady = 2;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
		break;
		}

	case 2:
		// we're reloading, don't allow fire
		{
		}
		break;
	}
}


void CSatchel::SecondaryAttack( void )
{
	if ( m_chargeReady != 2 )
	{
		Throw( );
	}
}


//=========================================================
// WeaponSpecial - "+attack3" switches between satchel
// charges and satellite rain (pev->weapons)
//=========================================================
void CSatchel::WeaponSpecial( void )
{
	if ( m_pPlayer->m_afButtonPressed & IN_ALT1 )
	{
		pev->weapons = !pev->weapons;
#ifndef CLIENT_DLL
		char *pszMode;
		switch ( pev->weapons )
		{
		case 1:
			pszMode = "Satellite Rain";
			break;
		default:
			pszMode = "Satchel Charge";
			break;
		}
		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "%s selected", pszMode );
#endif
	}
}


void CSatchel::Throw( void )
{
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		Vector vecSrc = m_pPlayer->pev->origin;

		Vector vecThrow = gpGlobals->v_forward * 274 + m_pPlayer->pev->velocity;

#ifndef CLIENT_DLL
		CBaseEntity *pSatchel = Create( "monster_satchel", vecSrc, Vector( 0, 0, 0), m_pPlayer->edict() );
		pSatchel->pev->velocity = vecThrow;
		pSatchel->pev->avelocity.y = 400;
#endif

		if ( pev->weapons )
		{
#ifndef CLIENT_DLL
			// satellite rain: a glowing target designator, 14 strikes, no remote
			pSatchel->SetUse( NULL );
			pSatchel->pev->rendercolor = Vector( 255, 100, 16 );
			pSatchel->pev->renderfx = kRenderFxGlowShell;
			pSatchel->pev->renderamt = 32;
			pSatchel->pev->weapons = 14;
#endif
		}
		else
		{
#ifndef CLIENT_DLL
			m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_satchel_radio.mdl");
			m_pPlayer->pev->weaponmodel = MAKE_STRING("models/p_satchel_radio.mdl");
#else
			LoadVModel ( "models/v_satchel_radio.mdl", m_pPlayer );
#endif

			SendWeaponAnim( SATCHEL_RADIO_DRAW );

			m_chargeReady = 1;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
	}
}


void CSatchel::WeaponIdle( void )
{
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	switch( m_chargeReady )
	{
	case 0:
		SendWeaponAnim( SATCHEL_FIDGET1 );
		// use tripmine animations
		strcpy( m_pPlayer->m_szAnimExtention, "trip" );
		break;
	case 1:
		SendWeaponAnim( SATCHEL_RADIO_FIDGET1 );
		// use hivehand animations
		strcpy( m_pPlayer->m_szAnimExtention, "hive" );
		break;
	case 2:
		if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			m_chargeReady = 0;
			RetireWeapon();
			return;
		}

#ifndef CLIENT_DLL
		m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_satchel.mdl");
		m_pPlayer->pev->weaponmodel = MAKE_STRING("models/p_satchel.mdl");
#else
		LoadVModel ( "models/v_satchel.mdl", m_pPlayer );
#endif

		SendWeaponAnim( SATCHEL_DRAW );

		// use tripmine animations
		strcpy( m_pPlayer->m_szAnimExtention, "trip" );

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_chargeReady = 0;
		break;
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );// how long till we do this again.
}

//=========================================================
// DeactivateSatchels - removes all satchels owned by
// the provided player. Should only be used upon death.
//
// Made this global on purpose.
//=========================================================
void DeactivateSatchels( CBasePlayer *pOwner )
{
#ifndef CLIENT_DLL
	edict_t *pFind; 

	pFind = FIND_ENTITY_BY_CLASSNAME( NULL, "monster_satchel" );

	while ( !FNullEnt( pFind ) )
	{
		CBaseEntity *pEnt = CBaseEntity::Instance( pFind );
		CSatchelCharge *pSatchel = (CSatchelCharge *)pEnt;

		if ( pSatchel )
		{
			// the owner's satchel charges go off; satellite rain keeps going
			if ( pSatchel->pev->owner == pOwner->edict() && !pSatchel->pev->weapons )
			{
				pSatchel->Detonate();
			}
		}

		pFind = FIND_ENTITY_BY_CLASSNAME( pFind, "monster_satchel" );
	}
#endif
}

#endif