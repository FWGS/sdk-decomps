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
// sniper.cpp - Scientist Slaughterhouse's sniper rifle and the
// anti-missile box: weapon_sniper, ammo_sniperclip, monster_sniper_box,
// anti_missile, laser_rocket (server only; file name ours)
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "explode.h"

#define SNIPER_DEFAULT_GIVE		10		// names ours from here
#define SNIPER_MAX_CLIP			5
#define AMMO_SNIPERCLIP_GIVE	5
#define SNIPERBOX_ROCKETS		5		// rockets a box costs

const float SNIPER_DAMAGE_RATE = 25;	// name ours: the aimed shot's damage per second of aiming

// v_m40a1.mdl's sequences
enum sniper_e {
	SNIPER_DRAW = 0,
	SNIPER_SLOWIDLE,
	SNIPER_FIRE,
	SNIPER_FIRELASTROUND,
	SNIPER_RELOAD1,
	SNIPER_RELOAD2,
	SNIPER_RELOAD3,
	SNIPER_SLOWIDLE2,
	SNIPER_HOLSTER
};

// the sniper box's types (pev->skin; names ours)
#define SNIPERBOX_ANTIMISSILE	0
#define SNIPERBOX_ANTIGRENADE	1
#define SNIPERBOX_MINIROCKET	2

class CSniperRifle : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 3; }
	BOOL ShouldWeaponIdle( void ) { return m_pSpot != NULL; }
	const char *GetKillDesc( void ) { return "a sniper rifle"; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return "shot"; }
	const char *GetHitGroupDesc( int iHitGroup ) { return HitGroupIn( iHitGroup ); }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void Reload( void );
	void WeaponIdle( void );
	void WeaponSpecial( void );

	int m_fInZoom;			// names ours
	CLaserSpot *m_pSpot;	// up while the shot is aimed
	EHANDLE m_hBox;			// the sniper box being placed
};

// a rocket that flies at m_hEnemy: the box's mini-rockets, and the rockets it fires at the
// rifle's laser spot (pev->frags 1); pev->owner is the box, pev->enemy the player who built it
class CLaserMissile : public CRocket
{
public:
	void Spawn( void );
	void Killed( entvars_t *pevAttacker, int iGib );
	void Steer( void );

	void EXPORT LMTouch( CBaseEntity *pOther );
};

// the anti-missile box's missile: flies at m_hEnemy and destroys it
class CAntiMissile : public CRocket
{
public:
	void Spawn( void );
	void Precache( void );
	const char *GetKillDesc( void ) { return "an anti-warhead missile"; }

	void EXPORT AMThink( void );
	void EXPORT AMTouch( CBaseEntity *pOther );
};

// the sniper box: a player builds it with the rifle (Install: pev->enemy is the builder),
// or a map places it (pev->weapons the type, -1 random)
class CAntiMissileBox : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int ObjectCaps( void )
	{
		if ( pev->enemy && ( pev->enemy->v.flags & FL_CLIENT ) )
			return FCAP_DONT_SAVE;
		return CBaseMonster::ObjectCaps();
	}
	void Install( CBaseMonster *pBuilder )
	{
		if ( pBuilder )
			pev->enemy = pBuilder->edict();
		pev->movetype = MOVETYPE_TOSS;
		SetTouch( StopTouch );
		pev->rendermode = kRenderTransAdd;
		pev->renderamt = 255;
		pev->nextthink = 0;
	}
	const char *GetHudName( void )
	{
		switch ( pev->skin )
		{
		case SNIPERBOX_ANTIGRENADE:	return "Anti-grenade box";
		case SNIPERBOX_MINIROCKET:	return "Mini-rocket box";
		default:					return "Anti-missile box";
		}
	}
	int Classify( void );
	const char *GetKillDesc( void );
	void Killed( entvars_t *pevAttacker, int iGib );
	bool IsTargeted( CBaseEntity *pTarget );		// names ours

	void EXPORT StopTouch( CBaseEntity *pOther );
	void EXPORT GuardThink( void );

	void Guard( char **pszTargets, int iCount );	// names ours
	void RocketThink( void );

	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	int IRelationship( CBaseEntity *pTarget );
	void DeathNotice( entvars_t *pevChild );
};

// the box type's name in the rifle's messages (name ours)
inline const char *SniperBoxName( int iType )
{
	switch ( iType )
	{
	case SNIPERBOX_ANTIGRENADE:	return "Anti-grenade";
	case SNIPERBOX_MINIROCKET:	return "Minirocket";
	default:					return "Anti-missile";
	}
}

LINK_ENTITY_TO_CLASS( weapon_sniper, CSniperRifle );

void CSniperRifle::Spawn( )
{
	Precache( );
	m_iId = WEAPON_SNIPER;
	SET_MODEL(ENT(pev), "models/w_m40a1.mdl");

	m_iDefaultAmmo = SNIPER_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CSniperRifle::Precache( void )
{
	PRECACHE_MODEL("models/v_m40a1.mdl");
	PRECACHE_MODEL("models/w_m40a1.mdl");
	PRECACHE_MODEL("models/p_m40a1.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/sniper_fire.wav");
	PRECACHE_SOUND("weapons/sniper_bolt1.wav");
	PRECACHE_SOUND("weapons/sniper_bolt2.wav");
	PRECACHE_SOUND("weapons/sniper_zoom.wav");
	PRECACHE_SOUND("weapons/sniper_reload_first_seq.wav");
	PRECACHE_SOUND("weapons/sniper_reload_second_seq.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");
	PRECACHE_SOUND ("weapons/building.wav");

	UTIL_PrecacheOther( "monster_sniper_box" );
}


int CSniperRifle::AddToPlayer( CBasePlayer *pPlayer )
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


int CSniperRifle::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "m40";
	p->iMaxAmmo1 = M40_MAX_CARRY;
	p->pszAmmo2 = "rockets";
	p->iMaxAmmo2 = ROCKET_MAX_CARRY;
	p->iMaxClip = SNIPER_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SNIPER;
	p->iWeight = SNIPER_WEIGHT;

	return 1;
}


BOOL CSniperRifle::Deploy( void )
{
	return DefaultDeploy( "models/v_m40a1.mdl", "models/p_m40a1.mdl", SNIPER_DRAW, "shotgun" );
}


void CSniperRifle::Holster( int skiplocal /* = 0 */ )
{
	if ( m_fInZoom || m_hBox != NULL )
		SecondaryAttack();

	if ( m_pSpot )
	{
		UTIL_Remove( m_pSpot );
		m_pSpot = NULL;
	}

	CBasePlayerWeapon::Holster( skiplocal );
}


// with a box placed: finish it; otherwise hold to aim (the laser spot), the shot goes when the
// button is let go (WeaponIdle)
void CSniperRifle::PrimaryAttack( void )
{
	if ( m_hBox )
	{
		if ( m_pPlayer->m_afButtonPressed & IN_ATTACK )
		{
			pev->weapons = m_hBox->pev->skin;	// the next box is of this type
			m_hBox->pev->rendermode = kRenderNormal;
			m_hBox->pev->nextthink = gpGlobals->time + 1;
			m_hBox = NULL;
			ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "Construction complete" );
			m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] -= SNIPERBOX_ROCKETS;
		}
	}
	else if ( pev->frags == 0 )	// not working the bolt
	{
		// don't fire underwater
		if ( m_pPlayer->pev->waterlevel == 3 )
		{
			PlayEmptySound( );
			m_flNextPrimaryAttack = gpGlobals->time + 0.15;
			return;
		}

		if ( m_iClip <= 0 )
		{
			PlayEmptySound( );
			return;
		}

		if ( !m_pSpot )
		{
			pev->dmgtime = gpGlobals->time;	// aiming starts
			m_pSpot = CLaserSpot::CreateSpot();
		}

		Vector vecSrc = m_pPlayer->GetGunPosition( );
		Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
		TraceResult tr;

		UTIL_TraceLine( vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr );
		UTIL_SetOrigin( m_pSpot->pev, tr.vecEndPos );
	}
}


// zoom; with a box placed: cancel it
void CSniperRifle::SecondaryAttack( void )
{
	if ( m_hBox )
	{
		if ( m_pPlayer->m_afButtonPressed & IN_ATTACK2 )
		{
			UTIL_Remove( m_hBox );
			m_hBox = NULL;
			ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "Construction canceled" );
		}
		return;
	}

	if ( m_pPlayer->pev->fov != 0 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
		m_fInZoom = 0;
	}
	else
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 20;
		m_fInZoom = 1;
	}

	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/sniper_zoom.wav", 1, ATTN_NORM );
	m_flNextSecondaryAttack = gpGlobals->time + 1.0;
}


void CSniperRifle::Reload( void )
{
	if ( m_fInZoom || m_hBox != NULL )
		SecondaryAttack();

	if ( DefaultReload( SNIPER_MAX_CLIP, SNIPER_RELOAD1, 2.35 ) )
		pev->frags = 1;	// work the bolt when the reload is done (WeaponIdle)
}


// fires the aimed shot: 25 damage a second of aiming, 0.5 to 4 seconds
void CSniperRifle::WeaponIdle( void )
{
	ResetEmptySound( );

	// the reload's bolt
	if ( pev->frags != 0 && !m_fInReload )
	{
		SendWeaponAnim( SNIPER_RELOAD2 );
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->time + 1.85;
		m_flTimeWeaponIdle = gpGlobals->time + 5.0;
		pev->frags = 0;
	}

	if ( m_hBox != NULL && ( m_pPlayer->pev->button & ( IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT ) ) )
		SecondaryAttack();

	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_pSpot && !( m_pPlayer->pev->button & IN_ATTACK ) )
	{
		UTIL_Remove( m_pSpot );
		m_pSpot = NULL;

		float flAimTime = gpGlobals->time - pev->dmgtime;
		if ( flAimTime < 0.5 )
			flAimTime = 0.5;
		else if ( flAimTime > 4 )
			flAimTime = 4;

		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

		m_iClip--;

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		Vector vecSrc = m_pPlayer->GetGunPosition( );
		m_pPlayer->FireBullets( 1, vecSrc, vecAiming, g_vecZero, 8192, BULLET_PLAYER_SNIPER, 0, flAimTime * SNIPER_DAMAGE_RATE );

		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/sniper_fire.wav", 1, ATTN_NORM );

		if ( m_iClip <= 0 )
		{
			SendWeaponAnim( SNIPER_FIRELASTROUND );

			if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
				// HEV suit - indicate out of ammo condition
				m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
		}
		else
			SendWeaponAnim( SNIPER_FIRE );

		m_flNextPrimaryAttack = gpGlobals->time + 1.75;
		m_flTimeWeaponIdle = gpGlobals->time + 5.0;
	}

	if ( m_flTimeWeaponIdle < gpGlobals->time )
	{
		SendWeaponAnim( RANDOM_FLOAT( 0, 1 ) > 0.5 ? SNIPER_SLOWIDLE : SNIPER_SLOWIDLE2 );
		m_flTimeWeaponIdle = gpGlobals->time + 100.0 / 23.0;
	}
}


// +attack3: with the laser spot up, the player's idle boxes that see it fire a laser-guided
// rocket at it; with a box placed, the next box type; otherwise place a box (5 rockets)
void CSniperRifle::WeaponSpecial( void )
{
	if ( !( m_pPlayer->m_afButtonPressed & IN_ALT1 ) )
		return;

	if ( m_pSpot )
	{
		CBaseEntity *pBox = NULL;

		while ( ( pBox = UTIL_FindEntityByClassname( pBox, "monster_sniper_box" ) ) != NULL )
		{
			if ( pBox->pev->frags == 0 && pBox->pev->enemy == ENT( m_pPlayer->pev ) )
			{
				Vector vecSrc = pBox->pev->origin + pBox->pev->view_ofs;
				TraceResult tr;

				UTIL_TraceLine( vecSrc, m_pSpot->pev->origin, ignore_monsters, ENT( pBox->pev ), &tr );

				if ( ( tr.vecEndPos - m_pSpot->pev->origin ).Length() < 64 )
				{
					CRocket *pRocket = (CRocket *)CBaseEntity::Create( "laser_rocket", vecSrc, UTIL_VecToAngles( m_pSpot->pev->origin - vecSrc ), ENT( pBox->pev ) );
					pRocket->SetThink( CRocket::TargetThink );
					pRocket->pev->enemy = ENT( m_pPlayer->pev );
					pBox->pev->frags = 1;	// busy until the rocket is gone (DeathNotice)
					pRocket->pev->frags = 1;
					pRocket->m_hEnemy = m_pSpot;
					pRocket->pev->speed = 1600;
					pRocket->m_iszKillDesc = MAKE_STRING( "a laser-guided rocket" );
					return;
				}
			}
		}
		return;
	}

	if ( m_hBox )
	{
		m_hBox->pev->skin++;
		if ( m_hBox->pev->skin > SNIPERBOX_MINIROCKET )
			m_hBox->pev->skin = SNIPERBOX_ANTIMISSILE;

		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "Building a %s box", SniperBoxName( m_hBox->pev->skin ) );
		return;
	}

	if ( m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] >= SNIPERBOX_ROCKETS )
	{
		UTIL_MakeVectors( Vector( 0, m_pPlayer->pev->v_angle.y, 0 ) );

		Vector vecSrc;
		vecSrc = m_pPlayer->pev->origin;
		if ( m_pPlayer->pev->flags & FL_DUCKING )
			vecSrc = vecSrc - Vector( 0, 0, -18 );

		Vector vecEnd = vecSrc + gpGlobals->v_forward * 64;
		TraceResult tr;

		UTIL_TraceLine( vecSrc + gpGlobals->v_forward * 20, vecEnd, dont_ignore_monsters, NULL, &tr );

		if ( !tr.fAllSolid && !tr.fStartSolid && tr.flFraction > 0.25 )
		{
			m_hBox = CBaseEntity::Create( "monster_sniper_box", tr.vecEndPos - gpGlobals->v_forward * 16, g_vecZero, NULL );
			m_hBox->pev->skin = pev->weapons;
			const char *pszType;
			switch ( m_hBox->pev->skin )
			{
			case SNIPERBOX_ANTIGRENADE:	pszType = "Anti-grenade"; break;
			case SNIPERBOX_MINIROCKET:	pszType = "Minirocket"; break;
			default:					pszType = "Anti-missile"; break;
			}
			ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "Building a %s box", pszType );
			m_hBox->Install( m_pPlayer );
			EMIT_SOUND( m_hBox->edict(), CHAN_VOICE, "weapons/building.wav", 1, ATTN_NORM );

			m_flNextPrimaryAttack = gpGlobals->time + 0.3;
			m_flTimeWeaponIdle = gpGlobals->time + 1.0;
		}
		else
			ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "Not enough room to build here" );
	}
	else
		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "Building a sniper box requires 5 rockets" );
}


class CSniperAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_m40a1clip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_m40a1clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if (pOther->GiveAmmo( AMMO_SNIPERCLIP_GIVE, "m40", M40_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_sniperclip, CSniperAmmo );


// what the anti-missile and anti-grenade boxes shoot at
static char *s_pszMissiles[] = { "rocket", "hvr_rocket", "na_rocket", "vortex_missile" };	// names ours
static char *s_pszGrenades[] = { "grenade", "vortex_mine", "cshell", "cluster_bomb" };

LINK_ENTITY_TO_CLASS( monster_sniper_box, CAntiMissileBox );
LINK_ENTITY_TO_CLASS( anti_missile, CAntiMissile );

void CAntiMissileBox::Spawn( void )
{
	Precache( );
	SET_MODEL( ENT(pev), "models/w_battery.mdl" );
	UTIL_SetSize( pev, Vector( -12, -12, 0 ), Vector( 12, 12, 32 ) );
	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_FLY;
	pev->view_ofs = Vector( 0, 0, 16 );
	m_flFieldOfView = VIEW_FIELD_FULL;

	pev->skin = pev->weapons;
	if ( pev->weapons == -1 )
		pev->skin = RANDOM_LONG( SNIPERBOX_ANTIMISSILE, SNIPERBOX_MINIROCKET );

	m_bloodColor = DONT_BLEED;
	pev->health = 100;
	pev->takedamage = DAMAGE_AIM;
	if ( HasSpawnFlag( 128 ) )	// invulnerable
		pev->takedamage = DAMAGE_NO;
	pev->frags = 0;

	SetThink( GuardThink );
	pev->nextthink = gpGlobals->time + 1;
}


void CAntiMissileBox::Precache( void )
{
	PRECACHE_MODEL( "models/w_battery.mdl" );
	UTIL_PrecacheOther( "anti_missile" );
	UTIL_PrecacheOther( "laser_rocket" );
}


// only the mini-rocket box is a monster
int CAntiMissileBox::Classify( void )
{
	if ( pev->skin == SNIPERBOX_MINIROCKET )
	{
		if ( pev->enemy )
			return 20;	// a player's
		return HasSpawnFlag( 256 ) ? CLASS_PLAYER_ALLY : CLASS_MACHINE;
	}
	return CLASS_NONE;
}


// a player's box kills in his name
const char *CAntiMissileBox::GetKillDesc( void )
{
	if ( pev->enemy )
		return CBaseEntity::Instance( pev->enemy )->GetKillDesc();

	switch ( pev->skin )
	{
	case SNIPERBOX_ANTIGRENADE:	return "an anti-grenade box";
	case SNIPERBOX_MINIROCKET:	return "a mini-rocket box";
	default:					return "an anti-missile box";
	}
}


void CAntiMissileBox::Killed( entvars_t *pevAttacker, int iGib )
{
	pev->deadflag = DEAD_DEAD;
	pev->takedamage = DAMAGE_NO;
	pev->solid = SOLID_NOT;
	pev->flags &= ~FL_MONSTER;

	CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
	if ( pOwner )
	{
		if ( pOwner->IsPlayer() )
			ClientPrint( pOwner->pev, HUD_PRINTTALK, "Command Control: Your sniper box was destroyed\n" );
		pOwner->DeathNotice( pev );
	}

	ExplosionCreate( pev->origin, pev->angles, edict(), 75, TRUE, 0 );
	UTIL_Remove( this );
}


// is an anti-missile already after pTarget?
bool CAntiMissileBox::IsTargeted( CBaseEntity *pTarget )
{
	CBaseEntity *pMissile = NULL;

	while ( ( pMissile = UTIL_FindEntityByClassname( pMissile, "anti_missile" ) ) != NULL )
	{
		CBaseMonster *pMonster = pMissile->MyMonsterPointer();
		if ( pMonster && pMonster->m_hEnemy == pTarget )
			return true;
	}

	return false;
}


// a box being placed falls (Install) until it lands on the world
void CAntiMissileBox::StopTouch( CBaseEntity *pOther )
{
	if ( !pOther->entindex() )
	{
		pev->movetype = MOVETYPE_FLY;
		pev->velocity = g_vecZero;
	}
}


void CAntiMissileBox::GuardThink( void )
{
	if ( pev->skin == SNIPERBOX_ANTIMISSILE )
		Guard( s_pszMissiles, 4 );
	else if ( pev->skin == SNIPERBOX_ANTIGRENADE )
		Guard( s_pszGrenades, 4 );
	else
		RocketThink();
}


// fires an anti-missile at the first of pszTargets in sight within 1024 units, then waits a second
void CAntiMissileBox::Guard( char **pszTargets, int iCount )
{
	pev->nextthink = gpGlobals->time + 0.1;

	Vector vecSrc = pev->origin + pev->view_ofs;
	CBaseEntity *pTarget = NULL;

	while ( ( pTarget = UTIL_FindEntityInSphere( pTarget, pev->origin, 1024 ) ) != NULL )
	{
		// not the builder's
		if ( pev->enemy && pTarget->pev->owner == pev->enemy )
			continue;

		if ( pTarget->pev->solid == SOLID_NOT )
			continue;

		if ( IsTargeted( pTarget ) )
			continue;

		for ( int i = 0; i < iCount; i++ )
		{
			if ( !FClassnameIs( pTarget->pev, pszTargets[i] ) )
				continue;

			TraceResult tr;
			UTIL_TraceLine( vecSrc, pTarget->pev->origin, dont_ignore_monsters, ENT( pev ), &tr );

			Vector vecDir = pTarget->pev->origin - vecSrc;

			if ( ( tr.vecEndPos - pTarget->pev->origin ).Length() < 64 )
			{
				CAntiMissile *pMissile = (CAntiMissile *)CBaseEntity::Create( "anti_missile", vecSrc, UTIL_VecToAngles( vecDir ), ENT( pev ) );
				pMissile->m_hEnemy = pTarget;
				pMissile->pev->velocity = vecDir.Normalize() * 512;

				pev->nextthink = gpGlobals->time + 1;
				return;
			}
			break;
		}
	}
}


// the mini-rocket box: a rocket (75 damage) at the best visible enemy, then waits a second
void CAntiMissileBox::RocketThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	Look( 1024 );
	m_hEnemy = BestVisibleEnemy( );
	pev->flags |= FL_MONSTER;

	if ( m_hEnemy )
	{
		Vector vecSrc = pev->origin + pev->view_ofs;
		Vector vecDir = m_hEnemy->BodyTarget( vecSrc ) - vecSrc;

		CRocket *pRocket = (CRocket *)CBaseEntity::Create( "laser_rocket", vecSrc, UTIL_VecToAngles( vecDir ), ENT( pev ) );
		SET_MODEL( ENT( pRocket->pev ), "models/crossbow_bolt.mdl" );
		pRocket->SetThink( CRocket::TargetThink );
		pRocket->pev->enemy = pev->enemy;
		pRocket->m_hEnemy = m_hEnemy;
		pRocket->pev->speed = 2000;
		pRocket->pev->velocity = vecDir.Normalize() * 512;
		pRocket->pev->dmg = 75;
		pRocket->m_iszKillDesc = MAKE_STRING( "a mini-rocket pod" );

		pev->nextthink = gpGlobals->time + 1;
	}
}


// a player's anti-missile and anti-grenade boxes take damage only from players
int CAntiMissileBox::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if ( pev->skin < SNIPERBOX_MINIROCKET && pev->enemy && ( !pevAttacker || !( pevAttacker->flags & FL_CLIENT ) ) )
		return 0;

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}


// the builder and what he owns are friends
int CAntiMissileBox::IRelationship( CBaseEntity *pTarget )
{
	if ( pTarget->edict() == pev->enemy || pTarget->pev->owner == pev->enemy )
		return R_AL;

	return CBaseMonster::IRelationship( pTarget );
}


// the laser-guided rocket is gone: ready again
void CAntiMissileBox::DeathNotice( entvars_t *pevChild )
{
	pev->frags = 0;
}


void CAntiMissile::Spawn( void )
{
	Precache( );
	pev->movetype = MOVETYPE_FLYMISSILE;
	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT(pev), "models/crossbow_bolt.mdl" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->effects |= EF_LIGHT;

	// make rocket sound
	EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5 );

	// rocket trail
	BeamFollow( g_sModelIndexSmokePuff, 40, 3, Vector( 255, 192, 32 ), 255 );

	SetThink( AMThink );
	SetTouch( AMTouch );
	pev->nextthink = gpGlobals->time + 0.1;

	pev->dmg = 75;
}


void CAntiMissile::Precache( void )
{
	PRECACHE_MODEL( "models/crossbow_bolt.mdl" );
	PRECACHE_SOUND( "weapons/rocket1.wav" );
}


// leads the target; within 32 units both go off, the target with half its damage
void CAntiMissile::AMThink( void )
{
	if ( m_hEnemy == NULL )
	{
		Killed( pev, GIB_NEVER );
		return;
	}

	Vector vecDir = m_hEnemy->pev->origin - pev->origin;
	float flDist = vecDir.Length();

	if ( flDist < 32 )
	{
		m_hEnemy->pev->dmg *= 0.5;
		m_hEnemy->SetTouch( NULL );
		m_hEnemy->pev->spawnflags &= ~( 2 | 8 | 16 );
		m_hEnemy->Killed( pev, GIB_NEVER );
		Killed( pev, GIB_NEVER );
		return;
	}

	// lead the target
	vecDir = m_hEnemy->pev->origin + m_hEnemy->pev->velocity * ( flDist * 0.0005 ) - pev->origin;
	pev->velocity = pev->velocity * 0.2 + vecDir.Normalize() * 1024;

	if ( pev->velocity.Length() > 2000 )
		pev->velocity = pev->velocity.Normalize() * 2000;

	pev->nextthink = gpGlobals->time + 0.05;
}


void CAntiMissile::AMTouch( CBaseEntity *pOther )
{
	if ( pOther == m_hEnemy )
	{
		m_hEnemy->pev->dmg *= 0.5;
		m_hEnemy->SetTouch( NULL );
		m_hEnemy->pev->spawnflags &= ~( 2 | 8 | 16 );
		m_hEnemy->Killed( pev, GIB_NEVER );
		Detonate( );
	}
	else
		RocketTouch( pOther );
}


LINK_ENTITY_TO_CLASS( laser_rocket, CLaserMissile );

void CLaserMissile::Spawn( void )
{
	pev->rendercolor = Vector( 255, 192, 32 );
	pev->angles.x = -pev->angles.x;
	CRocket::Spawn( );
	SetTouch( LMTouch );
	pev->classname = MAKE_STRING( "laser_rocket" );
}


// tells the box a laser-guided rocket is gone; the kill goes to the box's builder
void CLaserMissile::LMTouch( CBaseEntity *pOther )
{
	if ( pev->owner && pev->frags != 0 )
	{
		CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
		if ( pOwner )
			pOwner->DeathNotice( pev );
	}

	if ( pev->enemy )
		pev->owner = pev->enemy;

	RocketTouch( pOther );
}


// at m_vecEnemyLKP (TargetThink); a laser-guided one turns harder
void CLaserMissile::Steer( void )
{
	if ( m_hEnemy == NULL )
	{
		Killed( pev, GIB_NEVER );
		return;
	}

	if ( pev->frags != 0 )
	{
		pev->velocity = pev->velocity * 0.85 + ( m_vecEnemyLKP - pev->origin ).Normalize() * 128;
	}
	else
	{
		float flSpeed = pev->velocity.Length() * 0.4;
		if ( flSpeed < 256 )
			flSpeed = 256;

		pev->velocity = pev->velocity * 0.8 + ( m_vecEnemyLKP - pev->origin ).Normalize() * flSpeed;
	}

	if ( pev->velocity.Length() > pev->speed )
		pev->velocity = pev->velocity.Normalize() * pev->speed;

	if ( pev->waterlevel == 3 )
		UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1, pev->origin, 4 );

	pev->angles = UTIL_VecToAngles( pev->velocity );
}


void CLaserMissile::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( pev->owner && pev->frags != 0 )
	{
		CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
		if ( pOwner )
			pOwner->DeathNotice( pev );
	}

	if ( pev->enemy )
		pev->owner = pev->enemy;

	CRocket::Killed( pevAttacker, iGib );
}
