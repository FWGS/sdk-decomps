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
#include "soundent.h"
#include "customentity.h"
#include "beams.h"

#ifndef CLIENT_DLL
#define BOLT_AIR_VELOCITY	2000
#define BOLT_WATER_VELOCITY	1000

LINK_ENTITY_TO_CLASS( crossbow_bolt, CCrossbowBolt );

CCrossbowBolt *CCrossbowBolt::BoltCreate( void )
{
	// Create a new entity with CCrossbowBolt private data
	CCrossbowBolt *pBolt = GetClassPtr( (CCrossbowBolt *)NULL );
	pBolt->pev->classname = MAKE_STRING("crossbow_bolt");
	pBolt->Spawn();

	return pBolt;
}

const char *CCrossbowBolt::GetKillVerb( BOOL fGibbed, int &iHitGroup )
{
	if ( pev->frags == 1 )
	{
		iHitGroup = HITGROUP_GENERIC;
		return DamageVerb( DMG_BLAST, fGibbed );
	}

	if ( pev->speed == 0 )
	{
		// a trap, not the dart itself
		iHitGroup = HITGROUP_GENERIC;
		if ( pev->frags == 3 )
			return DamageVerb( DMG_SHOCK, fGibbed );
		if ( pev->frags == 4 )
			return DamageVerb( 0, fGibbed );
	}

	return "impaled";
}

void CCrossbowBolt::Spawn( )
{
	Precache( );
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5;

	SET_MODEL(ENT(pev), "models/crossbow_bolt.mdl");

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	SetTouch( BoltTouch );
	SetThink( BubbleThink );
	pev->nextthink = gpGlobals->time + 0.2;
}


void CCrossbowBolt::Precache( )
{
	PRECACHE_MODEL ("models/crossbow_bolt.mdl");
	PRECACHE_SOUND("weapons/xbow_hitbod1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
	PRECACHE_SOUND("weapons/xbow_fly1.wav");
	PRECACHE_SOUND("weapons/xbow_hit1.wav");
	PRECACHE_SOUND("fvox/beep.wav");
}


int	CCrossbowBolt :: Classify ( void )
{
	return	CLASS_NONE;
}

// a stuck laser net dart strings laser beams to the other stuck laser net darts in range
void CCrossbowBolt::LaserNet( void )
{
	float flRange = 250;

	CBaseEntity *pOther = UTIL_FindEntityByClassname( NULL, STRING( pev->classname ) );
	while ( pOther )
	{
		if ( pOther != this && pOther->pev->frags == 4 && pOther->pev->speed == 0 && pOther->pev->nextthink - gpGlobals->time >= 1 )
		{
			float flDist = ( pOther->pev->origin - pev->origin ).Length();
			if ( flDist < flRange )
			{
				TraceResult tr;
				UTIL_TraceLine( pev->origin, pOther->pev->origin, ignore_monsters, ignore_glass, ENT( pev ), &tr );
				if ( tr.flFraction >= 1.0 || tr.pHit == pOther->edict() )
				{
					CBeamCannon *pBeam = CBeamCannon::CannonCreate( MAKE_STRING( g_pModelNameLaser ), 0, 0 );
					pBeam->pev->origin = pev->origin;
					pBeam->m_firePosition = pOther->pev->origin;
					pBeam->SetWidth( 20 );
					pBeam->SetColor( 255, 10, 10 );
					pBeam->SetBrightness( 255 );
					pBeam->SetScrollRate( 35 );
					pBeam->pev->owner = pev->owner;
					pBeam->pev->spawnflags |= 0x402;
					pBeam->pev->dmg = 10;
					pBeam->m_damageType = DMG_ENERGYBEAM;
					pBeam->TurnOn();
					pBeam->m_expireTime = pOther->pev->nextthink;

					// each beam shrinks the range for the next
					if ( flDist < flRange * 0.5 )
						flRange = flRange - flDist;
					else
						flRange = flRange * 0.5;
				}
			}
		}
		pOther = UTIL_FindEntityByClassname( pOther, STRING( pev->classname ) );
	}

	CSprite *pSprite = CSprite::SpriteCreate( "sprites/glow01.spr", pev->origin, FALSE );
	pSprite->SetTransparency( kRenderTransAdd, 255, 12, 12, 255, kRenderFxNone );
	pSprite->SetScale( 0.2 );
	pSprite->SetThink( SUB_Remove );
	pev->nextthink = gpGlobals->time + 40;
	pSprite->pev->nextthink = pev->nextthink;
	SetThink( SUB_StartFadeOut );
}

void CCrossbowBolt::BoltTouch( CBaseEntity *pOther )
{
	if ( pev->frags == 1 )
	{
		// explosive darts go off on contact
		ExplodeThink();
		return;
	}

	SetTouch( NULL );
	SetThink( NULL );

	if (pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace( );
		entvars_t	*pevOwner;

		pevOwner = VARS( pev->owner );

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage( );

		float flDamage = gSkillData.plrDmgCrossbowMonster;
		if ( pOther->IsPlayer() )
			flDamage = gSkillData.plrDmgCrossbowClient;
		if ( pev->frags >= 2 )
			flDamage *= 0.5;

		pOther->TraceAttack(pevOwner, flDamage, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB ); 

		ApplyMultiDamage( pev, pevOwner );

		pev->velocity = Vector( 0, 0, 0 );
		// play body "thwack" sound
		switch( RANDOM_LONG(0,1) )
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM); break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/xbow_hitbod2.wav", 1, ATTN_NORM); break;
		}

		Killed( pev, GIB_NEVER );
	}
	else
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/xbow_hit1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,7));

		SetThink( SUB_StartFadeOut );
		pev->nextthink = gpGlobals->time;// this will get changed below if the bolt is allowed to stick in what it hit.

		if ( FBitSet( pOther->pev->flags, FL_WORLDBRUSH ) || FClassnameIs( pOther->pev, "worldspawn" ) )
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize( );
			UTIL_SetOrigin( pev, pev->origin - vecDir * 12 );
			pev->angles = UTIL_VecToAngles( vecDir );
			pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->angles.z = RANDOM_LONG(0,360);
			pev->nextthink = gpGlobals->time + 10.0;
			pev->speed = 0;

			if ( pev->frags == 3 )
			{
				// electrical: zaps around for ten seconds, a beam back along its path
				TraceResult tr = UTIL_GetGlobalTrace( );
				pev->movedir = UTIL_VecToAngles( tr.vecPlaneNormal );
				pev->dmgtime = pev->nextthink;
				pev->nextthink = gpGlobals->time + 0.1;
				SetThink( ZapThink );
				CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin, 256, 10 );

				UTIL_TraceLine( pev->origin, pev->origin - vecDir * 256, ignore_monsters, ENT( pev ), &tr );
				CBeam *pBeam = CBeam::BeamCreate( "sprites/lgtning.spr", 80 );
				pBeam->PointEntInit( tr.vecEndPos, entindex() );
				pBeam->SetBrightness( 255 );
				pBeam->SetColor( 0, 128, 255 );
				pBeam->SetNoise( 32 );
				pBeam->SetFlags( BEAM_FSHADEIN );
				pBeam->SetThink( SUB_Remove );
				pBeam->pev->nextthink = pev->dmgtime;
			}
			else if ( pev->frags == 2 )
			{
				// flare: lights up for twenty seconds
				pev->dmgtime = pev->nextthink + 10;
				pev->nextthink = gpGlobals->time + 0.1;
				SetThink( LightThink );

				MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY, pev->origin );
					WRITE_BYTE( TE_DLIGHT );
					WRITE_COORD( pev->origin.x );	// X
					WRITE_COORD( pev->origin.y );	// Y
					WRITE_COORD( pev->origin.z );	// Z
					WRITE_BYTE( 64 );		// radius * 0.1
					WRITE_BYTE( 255 );		// r
					WRITE_BYTE( 192 );		// g
					WRITE_BYTE( 64 );		// b
					WRITE_BYTE( 200 );		// time * 10
					WRITE_BYTE( 2 );		// decay * 0.1
				MESSAGE_END( );
			}
			else if ( pev->frags == 4 )
			{
				LaserNet();
			}
		}

		pev->avelocity = g_vecZero;
		pev->velocity = g_vecZero;

		if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		{
			UTIL_Sparks( pev->origin );
		}
	}
}

// the dart's coloured trail, by type
void CCrossbowBolt::BoltTrail( void )
{
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() );	// entity
		WRITE_SHORT( g_sModelIndexLaser );	// model
		WRITE_BYTE( 4 ); // life
		WRITE_BYTE( 2 );  // width
		switch ( (int)pev->frags )
		{
		case 1:	// explosive
			WRITE_BYTE( 255 );	// r, g, b
			WRITE_BYTE( 128 );
			WRITE_BYTE( 32 );
			break;
		case 2:	// flare
			WRITE_BYTE( 255 );
			WRITE_BYTE( 225 );
			WRITE_BYTE( 64 );
			break;
		case 3:	// electrical
			WRITE_BYTE( 4 );
			WRITE_BYTE( 170 );
			WRITE_BYTE( 255 );
			break;
		case 4:	// laser net
			WRITE_BYTE( 255 );
			WRITE_BYTE( 12 );
			WRITE_BYTE( 12 );
			break;
		default:
			WRITE_BYTE( 255 );
			WRITE_BYTE( 255 );
			WRITE_BYTE( 255 );
			break;
		}
		WRITE_BYTE( 128 );	// brightness
	MESSAGE_END();
}

void CCrossbowBolt::BubbleThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->waterlevel == 0)
		return;

	UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1, pev->origin, 1 );
}

void CCrossbowBolt::ZapThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;
	UTIL_Sparks( pev->origin );

	::RadiusDamage( pev->origin, pev, VARS( pev->owner ), 5, 256, CLASS_NONE, DMG_SHOCK | DMG_NEVERGIB );

	Vector vecAngles( RANDOM_FLOAT( -90, 90 ), RANDOM_FLOAT( -90, 90 ), 0 );
	UTIL_MakeVectors( vecAngles - pev->movedir );

	TraceResult tr;
	Vector vecEnd = pev->origin + gpGlobals->v_forward * 256;
	UTIL_TraceLine( pev->origin, vecEnd, ignore_monsters, ENT( pev ), &tr );

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMPOINTS );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( tr.vecEndPos.x );
		WRITE_COORD( tr.vecEndPos.y );
		WRITE_COORD( tr.vecEndPos.z );
		WRITE_SHORT( g_sModelIndexLightning );
		WRITE_BYTE( 0 ); // framestart
		WRITE_BYTE( RANDOM_LONG( 8, 12 ) ); // framerate
		WRITE_BYTE( 3 ); // life
		switch ( RANDOM_LONG( 0, 3 ) )
		{
		case 0:
			WRITE_BYTE( 80 );  // width
			WRITE_BYTE( 20 );   // noise
			WRITE_BYTE( 0 );   // r, g, b
			WRITE_BYTE( 128 );
			WRITE_BYTE( 255 );
			break;
		case 1:
			WRITE_BYTE( 40 );
			WRITE_BYTE( 32 );
			WRITE_BYTE( 0 );
			WRITE_BYTE( 128 );
			WRITE_BYTE( 255 );
			break;
		case 2:
			WRITE_BYTE( 20 );
			WRITE_BYTE( 15 );
			WRITE_BYTE( 255 );
			WRITE_BYTE( 255 );
			WRITE_BYTE( 255 );
			break;
		case 3:
			WRITE_BYTE( 20 );
			WRITE_BYTE( 48 );
			WRITE_BYTE( 0 );
			WRITE_BYTE( 0 );
			WRITE_BYTE( 255 );
			break;
		}
		WRITE_BYTE( 100 );	// brightness
		WRITE_BYTE( 35 );		// speed
	MESSAGE_END();

	if ( pev->dmgtime < gpGlobals->time )
		SetThink( SUB_StartFadeOut );
}

void CCrossbowBolt::LightThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;
	UTIL_Sparks( pev->origin );

	if ( pev->dmgtime < gpGlobals->time )
		SetThink( SUB_StartFadeOut );
}

void CCrossbowBolt::ExplodeThink( void )
{
	int iContents = UTIL_PointContents ( pev->origin );
	int iScale;
	
	pev->dmg = 40;
	iScale = 10;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION);		
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		if (iContents != CONTENTS_WATER)
		{
			WRITE_SHORT( g_sModelIndexFireball );
		}
		else
		{
			WRITE_SHORT( g_sModelIndexWExplosion );
		}
		WRITE_BYTE( iScale  ); // scale * 10
		WRITE_BYTE( 15  ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	entvars_t *pevOwner;

	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	::RadiusDamage( pev->origin, pev, pevOwner, pev->dmg, 128, CLASS_NONE, DMG_BLAST | DMG_ALWAYSGIB );

	UTIL_Remove(this);
}
#endif

enum crossbow_e {
	CROSSBOW_IDLE1 = 0,	// full
	CROSSBOW_IDLE2,		// empty
	CROSSBOW_FIDGET1,	// full
	CROSSBOW_FIDGET2,	// empty
	CROSSBOW_FIRE1,		// full
	CROSSBOW_FIRE2,		// reload
	CROSSBOW_FIRE3,		// empty
	CROSSBOW_RELOAD,	// from empty
	CROSSBOW_DRAW1,		// full
	CROSSBOW_DRAW2,		// empty
	CROSSBOW_HOLSTER1,	// full
	CROSSBOW_HOLSTER2,	// empty
};

LINK_ENTITY_TO_CLASS( weapon_crossbow, CCrossbow );

void CCrossbow::Spawn( )
{
	Precache( );
	m_iId = WEAPON_CROSSBOW;
	SET_MODEL(ENT(pev), "models/w_crossbow.mdl");

	m_iDefaultAmmo = CROSSBOW_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

int CCrossbow::AddToPlayer( CBasePlayer *pPlayer )
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

void CCrossbow::Precache( void )
{
	PRECACHE_MODEL("models/w_crossbow.mdl");
	PRECACHE_MODEL("models/v_crossbow.mdl");
	PRECACHE_MODEL("models/p_crossbow.mdl");

	PRECACHE_SOUND("weapons/xbow_fire1.wav");
	PRECACHE_SOUND("weapons/xbow_reload1.wav");

	UTIL_PrecacheOther( "crossbow_bolt" );

	m_usCrossbow = PRECACHE_EVENT( 1, "events/crossbow1.sc" );
	m_usCrossbow2 = PRECACHE_EVENT( 1, "events/crossbow2.sc" );
}


int CCrossbow::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "bolts";
	p->iMaxAmmo1 = BOLT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = CROSSBOW_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 2;
	p->iId = WEAPON_CROSSBOW;
	p->iFlags = 0;
	p->iWeight = CROSSBOW_WEIGHT;
	return 1;
}


BOOL CCrossbow::Deploy( )
{
	if (m_iClip)
		return DefaultDeploy( "models/v_crossbow.mdl", "models/p_crossbow.mdl", CROSSBOW_DRAW1, "bow" );
	return DefaultDeploy( "models/v_crossbow.mdl", "models/p_crossbow.mdl", CROSSBOW_DRAW2, "bow" );
}

void CCrossbow::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if ( m_fInZoom )
	{
		SecondaryAttack( );
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	if (m_iClip)
		SendWeaponAnim( CROSSBOW_HOLSTER1 );
	else
		SendWeaponAnim( CROSSBOW_HOLSTER2 );
}

void CCrossbow::PrimaryAttack( void )
{
	if (m_iClip == 0)
	{
		PlayEmptySound( );
		return;
	}

	// fired with the secondary button held: keep firing from WeaponIdle
	if ( m_pPlayer->pev->button & IN_ATTACK2 )
		pev->frags = 1;

	FireBolt();

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.75;

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75;

	if ( pev->frags )
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
	else if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75;
}

void CCrossbow::FireBolt()
{
	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

	m_iClip--;

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usCrossbow, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType], 0, 0 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#ifndef CLIENT_DLL
	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors( anglesAim );
	
	anglesAim.x		= -anglesAim.x;
	Vector vecSrc	 = m_pPlayer->GetGunPosition( ) - gpGlobals->v_up * 2;
	Vector vecDir;

	if ( pev->frags )
	{
		// rapid fire scatters the darts
		vecDir = UTIL_SpreadVector( Vector( 0.3473, 0.3473, 0.3473 ) ).Normalize();
		anglesAim = UTIL_VecToAngles( vecDir );
	}
	else
		vecDir = gpGlobals->v_forward;

	CCrossbowBolt *pBolt = CCrossbowBolt::BoltCreate();
	pBolt->pev->origin = vecSrc;
	pBolt->pev->angles = anglesAim;
	pBolt->pev->owner = m_pPlayer->edict();
	pBolt->pev->frags = pev->weapons;
	pBolt->BoltTrail();

	if (m_pPlayer->pev->waterlevel == 3)
	{
		pBolt->pev->velocity = vecDir * BOLT_WATER_VELOCITY;
		pBolt->pev->speed = BOLT_WATER_VELOCITY;
	}
	else
	{
		pBolt->pev->velocity = vecDir * BOLT_AIR_VELOCITY;
		pBolt->pev->speed = BOLT_AIR_VELOCITY;
	}
	pBolt->pev->avelocity.z = 10;
#endif

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
}


void CCrossbow::SecondaryAttack()
{
	if ( m_pPlayer->pev->fov != 0 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
		m_fInZoom = 0;
	}
	else
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 20;
		m_fInZoom = 1;
	}
	
	pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
}

// +attack3: unloads the clip and loads the next dart type
void CCrossbow::WeaponSpecial( void )
{
	if ( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 1 )
		return;

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < iMaxAmmo1() )
	{
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] += m_iClip;
		if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > iMaxAmmo1() )
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = iMaxAmmo1();
	}
	m_iClip = 0;

	pev->weapons++;
	if ( pev->weapons > 4 )
		pev->weapons = 0;

	m_flNextWeaponSpecial = UTIL_WeaponTimeBase() + 1.0;

#ifndef CLIENT_DLL
	const char *pszType;
	switch ( pev->weapons )
	{
	case 1:	pszType = "Explosive"; break;
	case 2:	pszType = "Flare"; break;
	case 3:	pszType = "Electrical"; break;
	case 4:	pszType = "Laser net"; break;
	default:	pszType = "Standard"; break;
	}
	ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "%s darts loaded", pszType );
#endif

	Reload();
}


void CCrossbow::Reload( void )
{
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 1 )
		return;

	if ( m_fInZoom )
	{
		SecondaryAttack();
	}

	if ( DefaultReload( 5, CROSSBOW_RELOAD, 4.5 ) )
	{
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/xbow_reload1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0,0xF));
	}
}


void CCrossbow::WeaponIdle( void )
{
	m_pPlayer->GetAutoaimVector( AUTOAIM_2DEGREES );  // get the autoaim vector but ignore it;  used for autoaim crosshair in DM

	ResetEmptySound( );
	
	if ( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
	{
		if ( pev->frags )
		{
			// rapid fire
			FireBolt();
			if ( m_iClip <= 0 )
			{
				pev->frags = 0;
				m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75;
			}
			else
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
			return;
		}

		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if (flRand <= 0.75)
		{
			if (m_iClip)
			{
				SendWeaponAnim( CROSSBOW_IDLE1 );
			}
			else
			{
				SendWeaponAnim( CROSSBOW_IDLE2 );
			}
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		}
		else
		{
			if (m_iClip)
			{
				SendWeaponAnim( CROSSBOW_FIDGET1 );
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 90.0 / 30.0;
			}
			else
			{
				SendWeaponAnim( CROSSBOW_FIDGET2 );
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 80.0 / 30.0;
			}
		}
	}
}



class CCrossbowAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_crossbow_clip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_crossbow_clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_CROSSBOWCLIP_GIVE, "bolts", BOLT_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_crossbow, CCrossbowAmmo );



#endif