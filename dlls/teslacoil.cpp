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
// teslacoil.cpp - Scientist Slaughterhouse's tesla coil:
// chain_lightning and weapon_teslacoil (file name ours)
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "effects.h"
#include "customentity.h"
#include "gamerules.h"
#include "soundent.h"


#define TESLACOIL_DEFAULT_GIVE	100	// names ours
#define TESLA_PRIMARY_VOLUME	450

enum teslacoil_e {	// v_gauss.mdl's sequences
	TESLA_IDLE = 0,
	TESLA_IDLE2,
	TESLA_FIDGET,
	TESLA_SPINUP,
	TESLA_SPIN,
	TESLA_FIRE,
	TESLA_FIRE2,
	TESLA_HOLSTER,
	TESLA_DRAW
};


#ifndef CLIENT_DLL

//=========================================================
// chain_lightning: the tesla coil's secondary attack. It
// jumps from victim to victim (the best visible enemy of
// its owner's, or a random direction), every half second,
// five times, and draws its last two links every tenth of
// a second, hurting what they cross.
//=========================================================
class CChainLightning : public CBaseMonster
{
public:
	void Spawn( void );
	int  Classify( void ) { return CLASS_NONE; }
	BOOL CanBeRepulsed( bool fForce ) { return FALSE; }
	const char *GetKillDesc( void ) { return "chain lightning"; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return DamageVerb( DMG_SHOCK, fGibbed ); }
	int IRelationship( CBaseEntity *pTarget );

	void EXPORT LightningThink( void );
	void Zap( const Vector &vecSrc, const Vector &vecDest );	// name ours
};

LINK_ENTITY_TO_CLASS( chain_lightning, CChainLightning );


void CChainLightning::Spawn( void )
{
	Precache( );
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	m_flFieldOfView = -1;	// sees all around

	// the chain's links: where it was, and whom it hit
	m_vecOldEnemy[0] = pev->origin;
	m_hOldEnemy[0].Set( pev->owner );

	SetThink( LightningThink );
	pev->health = 5;	// links left
	pev->nextthink = gpGlobals->time + 0.1;
	pev->dmgtime = gpGlobals->time + 0.5;
	m_hEnemy = NULL;
}


int CChainLightning::IRelationship( CBaseEntity *pTarget )
{
	// never the same victim twice in a row
	if ( pTarget == m_hEnemy )
		return R_NO;

	for ( int i = 0; i < MAX_OLD_ENEMIES; i++ )
	{
		if ( pTarget == m_hOldEnemy[i] )
			return R_NO;
	}

	if ( !pTarget->Classify() )
		return R_NO;

	// the shooter's friends and foes
	if ( m_hActivator )
	{
		CBaseMonster *pMonster = m_hActivator->MyMonsterPointer();
		if ( pMonster )
			return pMonster->IRelationship( pTarget );
	}

	return R_DL;
}


void CChainLightning::LightningThink( void )
{
	TraceResult tr;

	pev->nextthink = gpGlobals->time + 0.1;

	if ( pev->dmgtime != 0 && pev->dmgtime < gpGlobals->time )
	{
		if ( pev->health > 0 )
		{
			// the next link
			m_vecOldEnemy[1] = m_vecOldEnemy[0];
			m_vecOldEnemy[0] = pev->origin;
			m_hOldEnemy[1] = m_hOldEnemy[0];
			m_hOldEnemy[0] = m_hEnemy;

			Look( 512 );
			m_hEnemy = BestVisibleEnemy( );

			if ( m_hEnemy )
			{
				UTIL_SetOrigin( pev, m_hEnemy->BodyTarget( pev->origin ) );
			}
			else
			{
				// nobody to jump to: strike somewhere at random
				Vector vecDir = Vector( RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ) ) * 4096;
				UTIL_TraceLine( pev->origin, pev->origin + vecDir, dont_ignore_monsters, m_hOldEnemy[0].Get(), &tr );

				if ( ( tr.vecEndPos - pev->origin ).Length() < 8 )
				{
					// right against a wall: off it
					UTIL_TraceLine( tr.vecEndPos, tr.vecEndPos + tr.vecPlaneNormal * 1024, dont_ignore_monsters, m_hOldEnemy[0].Get(), &tr );
				}

				UTIL_SetOrigin( pev, tr.vecEndPos );
			}

			if ( RANDOM_LONG( 0, 1 ) )
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/electro4.wav", RANDOM_FLOAT( 0.7, 0.8 ), ATTN_NORM );
			else
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/electro5.wav", RANDOM_FLOAT( 0.7, 0.8 ), ATTN_NORM );
			EMIT_SOUND( ENT(pev), CHAN_ITEM, "weapons/electro6.wav", RANDOM_FLOAT( 0.7, 0.8 ), ATTN_NORM );

			pev->dmgtime = gpGlobals->time + 0.5;
			pev->health -= 1;
		}
		else if ( pev->health == 0 )
		{
			// the last link lingers half a second more
			pev->dmgtime = gpGlobals->time + 0.5;
			pev->health -= 1;
		}
		else
		{
			SetThink( SUB_Remove );
			return;
		}
	}

	if ( pev->health == 5 )
	{
		UTIL_TraceLine( m_vecOldEnemy[0], pev->origin, dont_ignore_monsters, pev->owner, &tr );
		if ( tr.pHit )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
			if ( pEntity->pev->takedamage )
				pEntity->TakeDamage( pev, VARS( pev->owner ), pev->dmg, DMG_SHOCK | DMG_NEVERGIB );
		}

		Zap( m_vecOldEnemy[0], pev->origin );
	}
	else if ( pev->health == 4 )
	{
		UTIL_TraceLine( m_vecOldEnemy[1], m_vecOldEnemy[0], dont_ignore_monsters, pev->owner, &tr );
		if ( tr.pHit )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
			if ( pEntity->pev->takedamage )
				pEntity->TakeDamage( pev, VARS( pev->owner ), pev->dmg, DMG_SHOCK | DMG_NEVERGIB );
		}

		UTIL_TraceLine( m_vecOldEnemy[0], pev->origin, dont_ignore_monsters, m_hOldEnemy[0].Get(), &tr );
		if ( tr.pHit )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
			if ( pEntity->pev->takedamage )
				pEntity->TakeDamage( pev, VARS( pev->owner ), pev->dmg, DMG_SHOCK | DMG_NEVERGIB );
		}

		Zap( m_vecOldEnemy[1], m_vecOldEnemy[0] );
		Zap( m_vecOldEnemy[0], pev->origin );
	}
	else if ( pev->health >= 0 )
	{
		UTIL_TraceLine( m_vecOldEnemy[1], m_vecOldEnemy[0], dont_ignore_monsters, m_hOldEnemy[1].Get(), &tr );
		if ( tr.pHit )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
			if ( pEntity->pev->takedamage )
				pEntity->TakeDamage( pev, VARS( pev->owner ), pev->dmg, DMG_SHOCK | DMG_NEVERGIB );
		}

		UTIL_TraceLine( m_vecOldEnemy[0], pev->origin, dont_ignore_monsters, m_hOldEnemy[0].Get(), &tr );
		if ( tr.pHit )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
			if ( pEntity->pev->takedamage )
				pEntity->TakeDamage( pev, VARS( pev->owner ), pev->dmg, DMG_SHOCK | DMG_NEVERGIB );
		}

		Zap( m_vecOldEnemy[1], m_vecOldEnemy[0] );
		Zap( m_vecOldEnemy[0], pev->origin );
	}
	else if ( pev->health == -1 )
	{
		UTIL_TraceLine( m_vecOldEnemy[0], pev->origin, dont_ignore_monsters, m_hOldEnemy[0].Get(), &tr );
		if ( tr.pHit )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
			if ( pEntity->pev->takedamage )
				pEntity->TakeDamage( pev, VARS( pev->owner ), pev->dmg, DMG_SHOCK | DMG_NEVERGIB );
		}

		Zap( m_vecOldEnemy[0], pev->origin );
	}

	float flVolume = RANDOM_FLOAT( 0.5, 1.0 );
	switch ( RANDOM_LONG( 0, 5 ) )
	{
		case 0: EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/spark1.wav", flVolume, ATTN_NORM);	break;
		case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/spark2.wav", flVolume, ATTN_NORM);	break;
		case 2: EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/spark3.wav", flVolume, ATTN_NORM);	break;
		case 3: EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/spark4.wav", flVolume, ATTN_NORM);	break;
		case 4: EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/spark5.wav", flVolume, ATTN_NORM);	break;
		case 5: EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/spark6.wav", flVolume, ATTN_NORM);	break;
	}
}


// four noisy bolts from vecSrc to vecDest, as the tesla coil's own beams
void CChainLightning::Zap( const Vector &vecSrc, const Vector &vecDest )
{
	for ( int i = 0; i < 4; i++ )
	{
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMPOINTS );
			WRITE_COORD( vecSrc.x );
			WRITE_COORD( vecSrc.y );
			WRITE_COORD( vecSrc.z );
			WRITE_COORD( vecDest.x );
			WRITE_COORD( vecDest.y );
			WRITE_COORD( vecDest.z );
			WRITE_SHORT( g_sModelIndexLightning );
			WRITE_BYTE( 0 ); // framestart
			WRITE_BYTE( 10 ); // framerate
			WRITE_BYTE( 2 ); // life
			switch ( i )
			{
			case 0:
				WRITE_BYTE( 80 );  // width
				WRITE_BYTE( 20 );   // noise
				WRITE_BYTE( 0 );   // r, g, b
				WRITE_BYTE( 128 );   // r, g, b
				WRITE_BYTE( 255 );   // r, g, b
				break;
			case 1:
				WRITE_BYTE( 40 );  // width
				WRITE_BYTE( 32 );   // noise
				WRITE_BYTE( 0 );   // r, g, b
				WRITE_BYTE( 128 );   // r, g, b
				WRITE_BYTE( 255 );   // r, g, b
				break;
			case 2:
				WRITE_BYTE( 20 );  // width
				WRITE_BYTE( 15 );   // noise
				WRITE_BYTE( 255 );   // r, g, b
				WRITE_BYTE( 255 );   // r, g, b
				WRITE_BYTE( 255 );   // r, g, b
				break;
			case 3:
				WRITE_BYTE( 20 );  // width
				WRITE_BYTE( 48 );   // noise
				WRITE_BYTE( 0 );   // r, g, b
				WRITE_BYTE( 0 );   // r, g, b
				WRITE_BYTE( 255 );   // r, g, b
				break;
			}
			WRITE_BYTE( 100 );	// brightness
			WRITE_BYTE( 35 );		// speed
		MESSAGE_END();
	}
}


LINK_ENTITY_TO_CLASS( weapon_teslacoil, CTeslaCoil );

TYPEDESCRIPTION	CTeslaCoil::m_SaveData[] = 
{
	DEFINE_ARRAY( CTeslaCoil, m_pBeams, FIELD_CLASSPTR, 4 ),
	DEFINE_FIELD( CTeslaCoil, m_shootTime, FIELD_INTEGER ),
	DEFINE_FIELD( CTeslaCoil, m_fireState, FIELD_INTEGER ),
	DEFINE_FIELD( CTeslaCoil, m_flAmmoUseTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CTeslaCoil, CBasePlayerWeapon );

#endif	// CLIENT_DLL (the client's tesla coil has no factory either)


void CTeslaCoil::Spawn( )
{
	Precache( );
	m_iId = WEAPON_TESLACOIL;
	SET_MODEL(ENT(pev), "models/w_gauss.mdl");

	m_iDefaultAmmo = TESLACOIL_DEFAULT_GIVE;

	// a blue glow shell
	pev->rendercolor = Vector( 0, 128, 255 );
	pev->renderfx = kRenderFxGlowShell;

	FallInit();// get ready to fall down.
}


void CTeslaCoil::Precache( void )
{
	PRECACHE_MODEL("models/w_gauss.mdl");
	PRECACHE_MODEL("models/v_gauss.mdl");
	PRECACHE_MODEL("models/p_gauss.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("buttons/spark1.wav");
	PRECACHE_SOUND("buttons/spark2.wav");
	PRECACHE_SOUND("buttons/spark3.wav");
	PRECACHE_SOUND("buttons/spark4.wav");
	PRECACHE_SOUND("buttons/spark5.wav");
	PRECACHE_SOUND("buttons/spark6.wav");
	PRECACHE_SOUND("weapons/electro4.wav");
	PRECACHE_SOUND("weapons/electro5.wav");
	PRECACHE_SOUND("weapons/electro6.wav");
	PRECACHE_SOUND("weapons/357_cock1.wav");

	UTIL_PrecacheOther( "chain_lightning" );

	m_usTeslaFire = PRECACHE_EVENT( 1, "events/teslafire.sc" );
	m_usTeslaStop = PRECACHE_EVENT( 1, "events/teslastop.sc" );
	m_usTeslaSpecial = PRECACHE_EVENT( 1, "events/teslaspecial.sc" );
}


int CTeslaCoil::AddToPlayer( CBasePlayer *pPlayer )
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


int CTeslaCoil::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "uranium";
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 5;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_TESLACOIL;
	p->iFlags = 0;
	p->iWeight = TESLACOIL_WEIGHT;

	return 1;
}


BOOL CTeslaCoil::Deploy( void )
{
	return DefaultDeploy( "models/v_gauss.mdl", "models/p_gauss.mdl", TESLA_DRAW, "gauss" );
}


void CTeslaCoil::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( TESLA_HOLSTER );

	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM );
}


void CTeslaCoil::PrimaryAttack( void )
{
	if ( pev->weapons )
	{
		// "Attraction" mode
		CallLightning( );
		return;
	}

	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		if ( m_fireState == FIRE_CHARGE )
			EndAttack();
		PlayEmptySound( );
		return;
	}

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.25;

	switch( m_fireState )
	{
		case FIRE_OFF:
		{
			if ( !HasAmmo() )
			{
				m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.25;
				PlayEmptySound( );
				return;
			}

			m_flAmmoUseTime = gpGlobals->time;// start using ammo ASAP.
			m_shootTime = TRUE;	// play the fire event with the first bolt

			m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
			m_pPlayer->m_iWeaponVolume = TESLA_PRIMARY_VOLUME;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
			pev->dmgtime = gpGlobals->time + 0.1;
			m_fireState = FIRE_CHARGE;
		}
		break;

		case FIRE_CHARGE:
		{
			Fire( m_pPlayer->GetGunPosition( ), m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
			m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
			m_pPlayer->m_iWeaponVolume = TESLA_PRIMARY_VOLUME;

			if ( m_shootTime )
			{
				PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usTeslaFire, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0 );
				m_shootTime = FALSE;
			}

			if ( !HasAmmo() )
			{
				EndAttack();
				m_fireState = FIRE_OFF;
				m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
			}
		}
		break;
	}
}


// +attack3: switches between "Projection" (the bolts) and "Attraction" (CallLightning)
void CTeslaCoil::WeaponSpecial( void )
{
	pev->weapons = !pev->weapons;
	m_flNextWeaponSpecial = UTIL_WeaponTimeBase() + 1;
#ifndef CLIENT_DLL
	ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "%s mode selected", pev->weapons ? "Attraction" : "Projection" );
#endif
}


// the chain lightning
void CTeslaCoil::SecondaryAttack( void )
{
	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound( );
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 20 )
	{
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usTeslaSpecial, 0, (float *)&m_pPlayer->pev->origin, (float *)&m_pPlayer->pev->v_angle, 0.0, 0.0, 0, 0, 0, 0 );

#ifndef CLIENT_DLL
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_pPlayer->m_iWeaponVolume = TESLA_PRIMARY_VOLUME;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector anglesAim, vecSrc;

	anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors( anglesAim );
	Vector vecAiming = gpGlobals->v_forward;
	vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_up * -8 + gpGlobals->v_right * 8;

	CBaseMonster *pLightning = CBaseEntity::Create( "chain_lightning", vecSrc, anglesAim, m_pPlayer->edict() )->MyMonsterPointer();
	pLightning->m_hActivator = m_pPlayer;

	// it starts where the player aims
	TraceResult tr;
	UTIL_TraceLine( vecSrc, vecSrc + vecAiming * 4096, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );
	pLightning->pev->origin = tr.vecEndPos;
	pLightning->pev->dmg = 5;

	if ( tr.pHit )
	{
		CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
		if ( pLightning->IRelationship( pHit ) )
			pLightning->m_hEnemy = pHit;
	}
#endif

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 20;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1;
}


void CTeslaCoil::Fire( const Vector &vecOrigSrc, const Vector &vecAngles )
{
#ifndef CLIENT_DLL
	UTIL_MakeVectors( vecAngles );
	Vector vecDir = gpGlobals->v_forward;
	edict_t		*pentIgnore;
	TraceResult tr;

	pentIgnore = m_pPlayer->edict();

	UTIL_TraceLine( vecOrigSrc, vecOrigSrc + vecDir * 8192, dont_ignore_monsters, pentIgnore, &tr );

	if ( tr.fAllSolid )
		return;

	if ( !tr.pHit )
		return;

	CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

	UTIL_Sparks( tr.vecEndPos );

	float flVolume = RANDOM_FLOAT( 0.5, 1.0 );
	switch ( RANDOM_LONG( 0, 5 ) )
	{
		case 0: EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_VOICE, "buttons/spark1.wav", flVolume, ATTN_NORM, 0, PITCH_NORM);	break;
		case 1: EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_VOICE, "buttons/spark2.wav", flVolume, ATTN_NORM, 0, PITCH_NORM);	break;
		case 2: EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_VOICE, "buttons/spark3.wav", flVolume, ATTN_NORM, 0, PITCH_NORM);	break;
		case 3: EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_VOICE, "buttons/spark4.wav", flVolume, ATTN_NORM, 0, PITCH_NORM);	break;
		case 4: EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_VOICE, "buttons/spark5.wav", flVolume, ATTN_NORM, 0, PITCH_NORM);	break;
		case 5: EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_VOICE, "buttons/spark6.wav", flVolume, ATTN_NORM, 0, PITCH_NORM);	break;
	}

	if ( pEntity->pev->takedamage )
	{
		ClearMultiDamage( );
		pEntity->TraceAttack( m_pPlayer->pev, 2, vecDir, &tr, DMG_SHOCK | DMG_NEVERGIB );
		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
	}

	if ( !tr.fAllSolid )
		AddEffect( tr.vecEndPos );

	if ( gpGlobals->time >= m_flAmmoUseTime )
	{
		UseAmmo( 1 );
		m_flAmmoUseTime = gpGlobals->time + 0.666;
	}
#endif
}


void CTeslaCoil::WeaponIdle( void )
{
	ResetEmptySound( );

	if ( m_fireState == FIRE_CHARGE )
		EndAttack();

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = RANDOM_FLOAT(0, 1);
	if (flRand <= 0.5)
	{
		iAnim = TESLA_IDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
	else if (flRand <= 0.75)
	{
		iAnim = TESLA_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
	else
	{
		iAnim = TESLA_FIDGET;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3;
	}

	SendWeaponAnim( iAnim );
}


void CTeslaCoil::EndAttack( void )
{
	m_fireState = FIRE_OFF;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;

	PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, m_pPlayer->edict(), m_usTeslaStop, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0 );

#ifndef CLIENT_DLL
	for ( int i = 0; i < 4; i++ )
		RemoveBeam( i );
#endif
}


// the four beams from the gun to endPoint: made the first time, then moved
void CTeslaCoil::AddEffect( const Vector &endPoint )
{
	for ( int i = 0; i < 4; i++ )
	{
		if ( m_pBeams[i] )
		{
			m_pBeams[i]->SetStartPos( endPoint );
		}
		else
		{
			m_pBeams[i] = CBeam::BeamCreate( "sprites/lgtning.spr", 80 );
			m_pBeams[i]->PointEntInit( endPoint, m_pPlayer->entindex() );
			m_pBeams[i]->SetEndAttachment( 1 );
			m_pBeams[i]->SetBrightness( 100 );
			m_pBeams[i]->pev->flags |= FL_SKIPLOCALHOST;
			m_pBeams[i]->pev->owner = m_pPlayer->edict();

			switch ( i )
			{
			case 0:
				m_pBeams[i]->SetColor( 0, 128, 255 );
				m_pBeams[i]->SetNoise( 20 );
				break;
			case 1:
				m_pBeams[i]->SetColor( 0, 128, 255 );
				m_pBeams[i]->SetNoise( 32 );
				m_pBeams[i]->SetWidth( 40 );
				break;
			case 2:
				m_pBeams[i]->SetColor( 255, 255, 255 );
				m_pBeams[i]->SetNoise( 15 );
				m_pBeams[i]->SetWidth( 20 );
				break;
			case 3:
				m_pBeams[i]->SetColor( 0, 0, 255 );
				m_pBeams[i]->SetNoise( 48 );
				m_pBeams[i]->SetWidth( 20 );
				break;
			}
		}
	}
}


void CTeslaCoil::RemoveBeam( int iBeam )
{
	if ( m_pBeams[iBeam] )
		UTIL_Remove( m_pBeams[iBeam] );
	m_pBeams[iBeam] = NULL;
}


// "Attraction" mode's primary attack: a lightning strike from above on what the player aims at
void CTeslaCoil::CallLightning( void )
{
	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound( );
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 5 )
	{
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

#ifndef CLIENT_DLL
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_pPlayer->m_iWeaponVolume = TESLA_PRIMARY_VOLUME;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector anglesAim( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecSrc( m_pPlayer->GetGunPosition( ) );
	UTIL_MakeVectors( anglesAim );
	Vector vecAiming( gpGlobals->v_forward );

	TraceResult tr;
	UTIL_TraceLine( vecSrc, vecSrc + vecAiming * 4096, dont_ignore_monsters, m_pPlayer->edict(), &tr );

	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		if ( pEntity->pev->takedamage )
		{
			ClearMultiDamage( );
			pEntity->TraceAttack( m_pPlayer->pev, 50, Vector( 0, 0, -1 ), &tr, DMG_SHOCK );
			ApplyMultiDamage( pev, m_pPlayer->pev );
		}
	}

	CSoundEnt::InsertSound( bits_SOUND_COMBAT, tr.vecEndPos, 512, 3.0 );
#endif

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usTeslaSpecial, 0, (float *)&m_pPlayer->pev->origin, (float *)&m_pPlayer->pev->v_angle, 0.0, 0.0, 1, 0, 0, 0 );

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 5;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1;
}


BOOL CTeslaCoil::HasAmmo( void )
{
	if ( m_pPlayer->ammo_uranium <= 0 )
		return FALSE;

	return TRUE;
}


void CTeslaCoil::UseAmmo( int count )
{
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= count )
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= count;
	else
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = 0;
}
