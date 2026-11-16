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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"


#define	HANDGRENADE_PRIMARY_VOLUME		450

enum handgrenade_e {
	HANDGRENADE_IDLE = 0,
	HANDGRENADE_FIDGET,
	HANDGRENADE_PINPULL,
	HANDGRENADE_THROW1,	// toss
	HANDGRENADE_THROW2,	// medium
	HANDGRENADE_THROW3,	// hard
	HANDGRENADE_HOLSTER,
	HANDGRENADE_DRAW
};


#ifndef CLIENT_DLL

// repulsor_grenade, the hand grenade's shockwave mode (CRepulsorGrenade: the export's
// class name): three seconds after the throw it pushes everything that can be repulsed
// and is in sight within 192 units away at 1000 units/s plus 200 up, with a green
// shockwave ring, and is gone
class CRepulsorGrenade : public CGrenade
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT Repulse( void );

	int m_iSpriteTexture;	// name ours
};

LINK_ENTITY_TO_CLASS( repulsor_grenade, CRepulsorGrenade );

void CRepulsorGrenade::Spawn( void )
{
	Precache( );
	m_fRegisteredSound = TRUE;

	SetTouch( BounceTouch );
	SetThink( Repulse );
	pev->nextthink = gpGlobals->time + 3;

	pev->sequence = RANDOM_LONG( 3, 6 );
	pev->framerate = 1.0;
	pev->gravity = 0.5;
	pev->friction = 0.8;

	SET_MODEL(ENT(pev), "models/w_grenade.mdl");
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
}

void CRepulsorGrenade::Precache( void )
{
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/shockwave.spr" );
	PRECACHE_SOUND( "weapons/concgren_blast1.wav" );
}

void CRepulsorGrenade::Repulse( void )
{
	CBaseEntity *pEntity = NULL;

	while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 192 ) ) != NULL )
	{
		if ( pEntity != this && pEntity->CanBeRepulsed( false ) )
		{
			TraceResult tr;
			Vector vecSpot = pEntity->Center();

			UTIL_TraceLine( pev->origin, vecSpot, ignore_monsters, ENT(pev), &tr );
			if ( tr.flFraction >= 1 || tr.pHit == pEntity->edict() )
			{
				pEntity->pev->velocity = pEntity->pev->velocity + ( vecSpot - pev->origin ).Normalize() * 1000 + Vector( 0, 0, 200 );
			}
		}
	}

	EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/concgren_blast1.wav", 1, ATTN_NORM );

	// shockwave
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z + 768 ); // reach damage radius over .25 seconds
		WRITE_SHORT( m_iSpriteTexture );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 2 ); // life
		WRITE_BYTE( 64 );  // width
		WRITE_BYTE( 0 );   // noise
		WRITE_BYTE( 128 );   // r
		WRITE_BYTE( 255 );   // g
		WRITE_BYTE( 32 );   // b
		WRITE_BYTE( 255 ); //brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();

	UTIL_Remove( this );
}

#endif


LINK_ENTITY_TO_CLASS( weapon_handgrenade, CHandGrenade );


void CHandGrenade::Spawn( )
{
	Precache( );
	m_iId = WEAPON_HANDGRENADE;
	SET_MODEL(ENT(pev), "models/w_grenade.mdl");

#ifndef CLIENT_DLL
	pev->dmg = gSkillData.plrDmgHandGrenade;
#endif

	m_iDefaultAmmo = HANDGRENADE_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CHandGrenade::Precache( void )
{
	PRECACHE_MODEL("models/w_grenade.mdl");
	PRECACHE_MODEL("models/v_grenade.mdl");
	PRECACHE_MODEL("models/p_grenade.mdl");
	PRECACHE_MODEL("models/mirv_grenade.mdl");
	PRECACHE_MODEL("models/bomblet.mdl");
	UTIL_PrecacheOther( "repulsor_grenade" );
}

int CHandGrenade::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Hand Grenade";
	p->iMaxAmmo1 = HANDGRENADE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_HANDGRENADE;
	p->iWeight = HANDGRENADE_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}


BOOL CHandGrenade::Deploy( )
{
	m_flReleaseThrow = -1;
	return DefaultDeploy( "models/v_grenade.mdl", "models/p_grenade.mdl", HANDGRENADE_DRAW, "crowbar" );
}

BOOL CHandGrenade::CanHolster( void )
{
	// can only holster hand grenades when not primed!
	return ( m_flStartThrow == 0 );
}

void CHandGrenade::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
	{
		SendWeaponAnim( HANDGRENADE_HOLSTER );
	}
	else
	{
		// no more grenades!
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_HANDGRENADE);
		SetThink( DestroyItem );
		pev->nextthink = gpGlobals->time + 0.1;
	}

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

void CHandGrenade::PrimaryAttack()
{
	if ( !m_flStartThrow && m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] > 0 )
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0;

		SendWeaponAnim( HANDGRENADE_PINPULL );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
		pev->iuser1 = 0;	// the mode's first grenade
	}
}

void CHandGrenade::SecondaryAttack()
{
	if ( !m_flStartThrow && m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] > 0 )
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0;

		SendWeaponAnim( HANDGRENADE_PINPULL );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
		pev->iuser1 = 1;	// the mode's second grenade
	}
}

// Picks the grenade mode (pev->weapons): 0 standard (hand grenade; secondary
// incendiary), 1 cluster (cluster bomb; secondary MIRV), 2 shockwave (repulsor
// grenade; secondary EMP)
void CHandGrenade::WeaponSpecial( void )
{
	if ( !m_flStartThrow )
	{
		m_flNextWeaponSpecial = UTIL_WeaponTimeBase() + 1.0;

		pev->weapons++;
		if ( pev->weapons > 2 )
			pev->weapons = 0;

#ifndef CLIENT_DLL
		const char *pszMode;
		switch ( pev->weapons )
		{
		case 1:	pszMode = "Cluster";	break;
		case 2:	pszMode = "Shockwave";	break;
		default:	pszMode = "Standard";	break;
		}

		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "%s mode selected", pszMode );
#endif
	}
}


void CHandGrenade::WeaponIdle( void )
{
	if ( m_flReleaseThrow == 0 && m_flStartThrow )
		 m_flReleaseThrow = gpGlobals->time;

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if ( m_flStartThrow )
	{
		CBaseEntity *pThrown = NULL;	// sic: never used (retail's x87 operand order in vecSrc needs it)
		Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		if ( angThrow.x < 0 )
			angThrow.x = -10 + angThrow.x * ( ( 90 - 10 ) / 90.0 );
		else
			angThrow.x = -10 + angThrow.x * ( ( 90 + 10 ) / 90.0 );

		float flVel = ( 90 - angThrow.x ) * 4;
		if ( flVel > 500 )
			flVel = 500;

		UTIL_MakeVectors( angThrow );

		Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16;

		Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity;

		// alway explode 3 seconds after the pin was pulled
		float time = m_flStartThrow - gpGlobals->time + 3.0;
		if (time < 0)
			time = 0;

#ifndef CLIENT_DLL
		if ( pev->weapons == 2 )
		{
			if ( pev->iuser1 )
			{
				// EMP: an energy sphere of shock damage
				CGrenade *pGrenade = CGrenade::ShootTimed( m_pPlayer->pev, vecSrc, vecThrow, time );
				pGrenade->pev->rendercolor = Vector( 128, 192, 255 );
				pGrenade->pev->spawnflags |= 16;
				pGrenade->m_bitsDamageInflict = DMG_SHOCK | DMG_NEVERGIB;
				pGrenade->m_iszKillDesc = MAKE_STRING( "an EMP hand grenade" );
			}
			else
			{
				CBaseEntity *pRepulsor = CBaseEntity::Create( "repulsor_grenade", vecSrc, UTIL_VecToAngles( vecThrow ), m_pPlayer->edict() );
				pRepulsor->pev->velocity = vecThrow;
				pRepulsor->pev->nextthink = gpGlobals->time + time;
			}
		}
		else if ( pev->weapons )
		{
			CClusterBomb *pBomb;
			if ( pev->iuser1 )
			{
				pBomb = CClusterBomb::ShootMirv( m_pPlayer->edict(), vecSrc, vecThrow, time );
				pBomb->pev->frags = 40;
			}
			else
			{
				pBomb = CClusterBomb::ShootTimed( m_pPlayer->edict(), vecSrc, vecThrow, time );
				pBomb->pev->frags = 6;
				pBomb->pev->spawnflags |= 1;
			}
			pBomb->m_iszKillDesc = MAKE_STRING( "a cluster bomb" );
		}
		else
		{
			CGrenade *pGrenade = CGrenade::ShootTimed( m_pPlayer->pev, vecSrc, vecThrow, time );
			if ( pev->iuser1 )
			{
				// incendiary, leaves flames
				pGrenade->pev->spawnflags |= 3;
				pGrenade->m_iszKillDesc = MAKE_STRING( "an incendiary hand grenade" );
			}
			else
				pGrenade->m_iszKillDesc = MAKE_STRING( "a hand grenade" );
		}
#endif

		if ( flVel < 500 )
		{
			SendWeaponAnim( HANDGRENADE_THROW1 );
		}
		else if ( flVel < 1000 )
		{
			SendWeaponAnim( HANDGRENADE_THROW2 );
		}
		else
		{
			SendWeaponAnim( HANDGRENADE_THROW3 );
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_flReleaseThrow = 0;
		m_flStartThrow = 0;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;

		m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ]--;

		if ( !m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
		{
			// just threw last grenade
			// set attack times in the future, and weapon idle in the future so we can see the whole throw
			// animation, weapon idle will automatically retire the weapon for us.
			m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;// ensure that the animation can finish playing
		}
		return;
	}
	else if ( m_flReleaseThrow > 0 )
	{
		// we've finished the throw, restart.
		m_flStartThrow = 0;

		if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
		{
			SendWeaponAnim( HANDGRENADE_DRAW );
		}
		else
		{
			RetireWeapon();
			return;
		}

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		m_flReleaseThrow = -1;
		return;
	}

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if (flRand <= 0.75)
		{
			iAnim = HANDGRENADE_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );// how long till we do this again.
		}
		else 
		{
			iAnim = HANDGRENADE_FIDGET;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 75.0 / 30.0;
		}

		SendWeaponAnim( iAnim );
	}
}




