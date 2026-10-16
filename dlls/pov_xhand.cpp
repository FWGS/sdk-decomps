/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
// POV: weapon_xhand, the alien slave's hands. The primary
// attack is a claw swing (the crowbar's trace), the
// secondary charges the hands' beams (the gauss's charge
// timing, the alien slave's arm beams) and zaps; part of
// the damage done heals the player. The charge is shown in
// pev->armorvalue. File name and the names retail doesn't
// give are ours.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "effects.h"
#include "decals.h"
#include "gamerules.h"


extern DLL_GLOBAL float g_flFadeEndTime;

#define XHAND_MAX_BEAMS	16

enum xhand_e {
	XHAND_IDLE1 = 0,
	XHAND_IDLE2,
	XHAND_IDLE3,
	XHAND_ATTACK1,
	XHAND_ATTACK1MISS,
	XHAND_ATTACK2,
	XHAND_ATTACK2MISS,
	XHAND_ATTACK3,
	XHAND_ATTACK3MISS,
	XHAND_SHOOT,
	XHAND_CHARGING,
	XHAND_HOLSTER,
	XHAND_DRAW
};

class CXHand : public CBasePlayerWeapon
{
public:
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	void Holster( void );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );

	float GetFullChargeTime( void );
	BOOL IsFullyCharged( void );
	BOOL IsOverCharged( void );
	void ZapFire( void );
	void PowerDown( void );
	void ClearAndPowerDown( void );
	void Fizzle( void );
	void UpdateCharge( void );
	void ResetCharge( void );
	void ZapHeal( CBaseEntity *pEntity, float flDamage );
	void ArmBeams( void );
	void ArmBeam( int side, int iBeam );
	void UpdateBeams( void );
	void BeamGlow( BOOL fCharge );
	void ClearBeams( void );
	void HitSound( CBaseEntity *pEntity );
	void MissSound( void );

	int m_fInAttack;
	float m_flStartCharge;
	CBeam *m_pBeam[XHAND_MAX_BEAMS];
	int m_iLightRadius;
	float m_flNextLight;
	int m_iBeams;	// unused
};
LINK_ENTITY_TO_CLASS( weapon_xhand, CXHand );


TYPEDESCRIPTION	CXHand::m_SaveData[] = 
{
	DEFINE_FIELD( CXHand, m_fInAttack, FIELD_INTEGER ),
	DEFINE_FIELD( CXHand, m_flStartCharge, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CXHand, CBasePlayerWeapon );


void FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity )
{
	int			i, j, k;
	float		distance;
	float		*minmaxs[2] = {mins, maxs};
	TraceResult tmpTrace;
	Vector		vecHullEnd = tr.vecEndPos;
	Vector		vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc)*2);
	UTIL_TraceLine( vecSrc, vecHullEnd, dont_ignore_monsters, pEntity, &tmpTrace );
	if ( tmpTrace.flFraction < 1.0 )
	{
		tr = tmpTrace;
		return;
	}

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			for ( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, pEntity, &tmpTrace );
				if ( tmpTrace.flFraction < 1.0 )
				{
					float thisDistance = (tmpTrace.vecEndPos - vecSrc).Length();
					if ( thisDistance < distance )
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}


float CXHand::GetFullChargeTime( void )
{
	if ( g_pGameRules->IsMultiplayer() )
	{
		return 1.5;
	}

	return gSkillData.plrChargeTime;
}


void CXHand::Spawn( )
{
	Precache( );
	m_iId = WEAPON_XHAND;
	SET_MODEL(ENT(pev), "models/v_xenhands.mdl");

	FallInit();// get ready to fall down.

	g_flFadeEndTime = gpGlobals->time - 1.0;
}


void CXHand::Precache( void )
{
	PRECACHE_MODEL("models/v_xenhands.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("debris/zap4.wav");
	PRECACHE_SOUND("weapons/electro4.wav");
	PRECACHE_SOUND("player/pl_fire1.wav");
	PRECACHE_SOUND("player/pl_nofire1.wav");

	PRECACHE_SOUND("player/pl_clawmiss1.wav");
	PRECACHE_SOUND("player/pl_clawmiss2.wav");
	PRECACHE_SOUND("player/pl_clawstrike1.wav");
	PRECACHE_SOUND("player/pl_clawstrike2.wav");
	PRECACHE_SOUND("player/pl_clawstrike3.wav");
	PRECACHE_SOUND("player/pl_clawstrike4.wav");
	PRECACHE_SOUND("player/pl_clawstrike5.wav");

	PRECACHE_MODEL("sprites/lgtning.spr");
}

int CXHand::AddToPlayer( CBasePlayer *pPlayer )
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

int CXHand::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 0;
	p->iId = WEAPON_XHAND;
	p->iFlags = ITEM_FLAG_SELECTONEMPTY | ITEM_FLAG_NOAUTORELOAD | ITEM_FLAG_NOAUTOSWITCHEMPTY;
	p->iWeight = CROWBAR_WEIGHT;
	return 1;
}


BOOL CXHand::Deploy( )
{
	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT( 3, 6 );
	return DefaultDeploy( "models/v_xenhands.mdl", "models/v_xenhands.mdl", XHAND_DRAW, "xhand" );
}

void CXHand::Holster( void )
{
	ClearBeams( );
	DefaultHolster( XHAND_HOLSTER, 0.9, 0.9 );
	m_fInAttack = 0;

	EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM, 0, RANDOM_LONG( 98, 102 ) );
}


void CXHand::PrimaryAttack( void )
{
	if ( m_pPlayer->m_fDidHolster )
	{
		WeaponIdle( );
		return;
	}

	if ( m_flNextPrimaryAttack > gpGlobals->time )
		return;

	ClearBeams( );
	if ( m_fInAttack )
		Fizzle( );

	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

	if ( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if ( tr.flFraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() )
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}

	if ( tr.flFraction >= 1.0 )
	{
		// miss
		switch( RANDOM_LONG(0,1) )
		{
		case 0:
			SendWeaponAnim( XHAND_ATTACK1MISS ); break;
		case 1:
			SendWeaponAnim( XHAND_ATTACK2MISS ); break;
		}
		MissSound( );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_flNextPrimaryAttack = gpGlobals->time + 0.5;
		m_flNextSecondaryAttack = gpGlobals->time + 0.5;
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_LONG( 3, 6 );
	}
	else
	{
		// hit
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		switch( RANDOM_LONG(0,1) )
		{
		case 0:
			SendWeaponAnim( XHAND_ATTACK1 ); break;
		case 1:
			SendWeaponAnim( XHAND_ATTACK2 ); break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		ClearMultiDamage( );
		pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgMelee, gpGlobals->v_forward, &tr, DMG_SLASH | DMG_NEVERGIB ); 
		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

		if ( pEntity )
			HitSound( pEntity );

		m_flNextPrimaryAttack = gpGlobals->time + 0.5;
		m_flNextSecondaryAttack = gpGlobals->time + 0.5;
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_LONG( 3, 6 );
	}
}


void CXHand::SecondaryAttack( void )
{
	if ( m_pPlayer->m_fDidHolster )
	{
		WeaponIdle( );
		return;
	}

	if ( !m_fInAttack && m_flNextSecondaryAttack > gpGlobals->time )
		return;

	if ( UTIL_IsUnderwater( m_pPlayer ) )
	{
		if ( m_fInAttack )
			ClearAndPowerDown( );
		else
			EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "player/pl_nofire1.wav", 0.8, ATTN_NORM, 0, RANDOM_LONG( 98, 102 ) );

		m_flNextPrimaryAttack = gpGlobals->time + 1.0;
		m_flNextSecondaryAttack = gpGlobals->time + 1.0;
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_LONG( 3, 6 );
		return;
	}

	switch ( m_fInAttack )
	{
	case 0:
		m_fInAttack = 1;
		SendWeaponAnim( XHAND_CHARGING );
		ArmBeams( );
		m_flStartCharge = gpGlobals->time;
		m_flNextPrimaryAttack = gpGlobals->time + 0.5;
		m_flTimeWeaponIdle = gpGlobals->time + 0.1;
		break;
	case 1:
		if ( IsFullyCharged( ) )
			m_fInAttack = 2;
		else
			UpdateCharge( );
		m_flNextPrimaryAttack = gpGlobals->time + 0.5;
		m_flTimeWeaponIdle = gpGlobals->time + 0.1;
		break;
	case 2:
		if ( IsOverCharged( ) )
		{
			Fizzle( );
			ClearBeams( );
		}
		BeamGlow( FALSE );
		m_flNextPrimaryAttack = gpGlobals->time + 0.5;
		m_flTimeWeaponIdle = gpGlobals->time + 0.1;
		break;
	}
}


BOOL CXHand::IsFullyCharged( void )
{
	return (int)m_pPlayer->pev->armorvalue >= 100;
}


BOOL CXHand::IsOverCharged( void )
{
	if ( gpGlobals->time < m_flStartCharge + gSkillData.plrOldTime )
		return FALSE;
	return TRUE;
}


void CXHand::ZapFire( void )
{
	if ( !IsFullyCharged( ) )
		PowerDown( );

	float flDamage = gSkillData.plrDmgZapBeam;
	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( 0 );
	Vector vecSrc = m_pPlayer->GetGunPosition( );
	Vector vecDest = vecSrc + vecAiming * 8192;

	UTIL_TraceLine( vecSrc, vecDest, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr );

	if ( tr.fAllSolid )
		return;

	CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
	if ( pEntity == NULL )
		return;

	if ( pEntity->pev->takedamage )
	{
		float flHealth = pEntity->pev->health;

		if ( FClassnameIs( pEntity->pev, "func_breakable" ) )
		{
			if ( flDamage < pEntity->pev->max_health / 2 + 1 )
				flDamage = pEntity->pev->max_health / 2 + 1;
		}

		ClearMultiDamage( );
		pEntity->TraceAttack( m_pPlayer->pev, flDamage, vecAiming, &tr, DMG_SHOCK | DMG_NEVERGIB );
		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

		float flDone;
		if ( ((CBaseMonster *)pEntity)->m_IdealMonsterState != MONSTERSTATE_DEAD )
			flDone = flHealth - pEntity->pev->health;
		else
			flDone = flHealth;

		ZapHeal( pEntity, flDone );
	}

	if ( pEntity->Classify() == CLASS_NONE )
		UTIL_DecalTrace( &tr, RANDOM_LONG( DECAL_SMALLSCORCH1, DECAL_SMALLSCORCH3 ) );

	vecSrc = tr.vecEndPos + vecAiming;

	CSoundEnt::InsertSound( bits_SOUND_COMBAT, m_pPlayer->pev->origin, 600, 1.5 );
	EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "player/pl_fire1.wav", 1, ATTN_NORM, 0, RANDOM_FLOAT( 98, 102 ) );

	SendWeaponAnim( XHAND_SHOOT );

	CBeam *pBeam1 = CBeam::BeamCreate( "sprites/lgtning.spr", 50 );
	CBeam *pBeam2 = CBeam::BeamCreate( "sprites/lgtning.spr", 50 );

	if ( pBeam1 )
	{
		pBeam1->PointEntInit( tr.vecEndPos, m_pPlayer->entindex( ) );
		pBeam1->SetEndAttachment( 1 );
		pBeam1->SetColor( 180, 255, 96 );
		pBeam1->SetBrightness( 255 );
		pBeam1->SetNoise( 20 );
		pBeam1->LiveForTime( 0.5 );
		pBeam1->pev->spawnflags |= SF_BEAM_TEMPORARY;
	}

	if ( pBeam2 )
	{
		pBeam2->PointEntInit( tr.vecEndPos, m_pPlayer->entindex( ) );
		pBeam2->SetEndAttachment( 2 );
		pBeam2->SetColor( 180, 255, 96 );
		pBeam2->SetBrightness( 255 );
		pBeam2->SetNoise( 20 );
		pBeam2->LiveForTime( 0.5 );
		pBeam2->pev->spawnflags |= SF_BEAM_TEMPORARY;
	}

	UTIL_ScreenShake( m_pPlayer->pev->origin, 2.0, 1.0, 0.5, 0 );

	if ( g_flFadeEndTime < gpGlobals->time )
		UTIL_ScreenFade( m_pPlayer, Vector( 96, 128, 16 ), 0.2, 0.1, 128, 0 );

	m_fInAttack = 0;
	ResetCharge( );

	m_flNextPrimaryAttack = gpGlobals->time + 1.0;
	m_flNextSecondaryAttack = gpGlobals->time + 1.0;
	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_LONG( 3, 6 );
}


void CXHand::PowerDown( void )
{
	EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/electro4.wav", 1.0, ATTN_NORM, 0, RANDOM_LONG( 98, 102 ) );

	m_fInAttack = 0;
	ResetCharge( );

	SendWeaponAnim( XHAND_IDLE3 );

	m_flNextPrimaryAttack = gpGlobals->time + 0.5;
	m_flNextSecondaryAttack = gpGlobals->time + 0.5;
	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_LONG( 3, 6 );
}


void CXHand::ClearAndPowerDown( void )
{
	ClearBeams( );
	PowerDown( );
}


void CXHand::Fizzle( void )
{
	if ( g_flFadeEndTime < gpGlobals->time )
		UTIL_ScreenFade( m_pPlayer, Vector( 96, 128, 16 ), 0.3, 0.1, 64, 0 );

	PowerDown( );
}


void CXHand::UpdateCharge( void )
{
	m_pPlayer->pev->armorvalue = (int)( ( gpGlobals->time - m_flStartCharge ) / GetFullChargeTime( ) * 100 );
	UpdateBeams( );
}


void CXHand::ResetCharge( void )
{
	m_pPlayer->pev->armorvalue = 0;
}


void CXHand::ZapHeal( CBaseEntity *pEntity, float flDamage )
{
	if ( FClassnameIs( pEntity->pev, "monster_apache" ) )
		return;
	if ( FClassnameIs( pEntity->pev, "monster_gman" ) )
		return;
	if ( FClassnameIs( pEntity->pev, "monster_generic" ) )
		return;
	if ( FClassnameIs( pEntity->pev, "monster_loader_armed" ) )
		return;

	if ( pEntity->pev->deadflag != DEAD_NO )
		return;
	if ( pEntity->pev->health <= 0 )
		return;

	if ( pEntity->Classify() == CLASS_HUMAN_PASSIVE ||
		pEntity->Classify() == CLASS_HUMAN_MILITARY ||
		pEntity->Classify() == CLASS_HUMAN_SPFORCE ||
		pEntity->Classify() == CLASS_ALIEN_MILITARY ||
		pEntity->Classify() == CLASS_ALIEN_PASSIVE ||
		pEntity->Classify() == CLASS_ALIEN_MONSTER ||
		pEntity->Classify() == CLASS_ALIEN_PREY ||
		pEntity->Classify() == CLASS_ALIEN_PREDATOR ||
		pEntity->Classify() == CLASS_PLAYER_ALLY ||
		pEntity->Classify() == CLASS_BARNACLE )
	{
		if ( flDamage < 0 )
			flDamage = flDamage * -1;

		m_pPlayer->TakeHealth( (float)(gSkillData.plrZapHeal * flDamage) / 100, DMG_GENERIC );
	}
}


void CXHand::ArmBeams( void )
{
	int i;

	for ( i = 0; i < XHAND_MAX_BEAMS / 2; i++ )
		ArmBeam( 1, i );

	for ( i = XHAND_MAX_BEAMS / 2; i < XHAND_MAX_BEAMS; i++ )
		ArmBeam( 2, i );

	EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "debris/zap4.wav", 1, ATTN_NORM, 0, 100 );

	m_iLightRadius = 10;
	m_flNextLight = gpGlobals->time - 1.0;
	BeamGlow( TRUE );
}


void CXHand::ArmBeam( int side, int iBeam )
{
	TraceResult tr;

	Vector vecSrc = m_pPlayer->GetGunPosition( );
	Vector vecAim( RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, -0.4 ) );
	vecAim = vecAim.Normalize( );

	UTIL_TraceLine( vecSrc, vecSrc + vecAim * 1024, ignore_monsters, ENT( m_pPlayer->pev ), &tr );

	Vector vecDist = tr.vecEndPos - vecSrc;
	if ( vecDist.Length( ) < 102.4 )
		return;

	if ( tr.flFraction == 1.0 )
		return;

	DecalGunshot( &tr, BULLET_PLAYER_CROWBAR );

	m_pBeam[iBeam] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	if ( !m_pBeam[iBeam] )
		return;

	m_pBeam[iBeam]->PointEntInit( tr.vecEndPos, m_pPlayer->entindex( ) );
	m_pBeam[iBeam]->SetEndAttachment( side );
	m_pBeam[iBeam]->SetColor( 96, 128, 16 );
	m_pBeam[iBeam]->SetBrightness( 64 );
	m_pBeam[iBeam]->SetNoise( 80 );
	m_pBeam[iBeam]->pev->spawnflags |= SF_BEAM_TEMPORARY;
}


void CXHand::UpdateBeams( void )
{
	for ( int i = 0; i < XHAND_MAX_BEAMS; i++ )
	{
		if ( m_pBeam[i] )
			m_pBeam[i]->SetBrightness( (float)(m_pPlayer->pev->armorvalue * 191) / 100 );
	}

	BeamGlow( TRUE );
}


void CXHand::BeamGlow( BOOL fCharge )
{
	Vector vecSrc;

	if ( m_flNextLight > gpGlobals->time )
		return;

	vecSrc = m_pPlayer->pev->origin;
	vecSrc = vecSrc + gpGlobals->v_forward * 16;

	if ( fCharge )
		m_iLightRadius = 10 - (int)( (float)(m_pPlayer->pev->armorvalue * 5) / -100 );

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
		WRITE_BYTE( TE_DLIGHT );
		WRITE_COORD( vecSrc.x );	// X
		WRITE_COORD( vecSrc.y );	// Y
		WRITE_COORD( vecSrc.z );	// Z
		WRITE_BYTE( m_iLightRadius );		// radius * 0.1
		WRITE_BYTE( 180 );		// r
		WRITE_BYTE( 255 );		// g
		WRITE_BYTE( 96 );		// b
		WRITE_BYTE( 1 );		// time * 10
		WRITE_BYTE( 1 );		// decay * 0.1
	MESSAGE_END( );

	m_flNextLight = gpGlobals->time + 0.05;
}


void CXHand::ClearBeams( void )
{
	for (int i = 0; i < XHAND_MAX_BEAMS; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}

	STOP_SOUND( ENT(pev), CHAN_WEAPON, "debris/zap4.wav" );
}


void CXHand::WeaponIdle( void )
{
	if ( m_pPlayer->m_fDidHolster )
	{
		if ( m_flTimeWeaponIdle < gpGlobals->time )
		{
			m_pPlayer->m_fDidHolster = FALSE;
			Deploy( );
			UpdateItemInfo( );
		}
		return;
	}

	m_pPlayer->GetAutoaimVector( 0 );

	if ( m_flTimeWeaponIdle > gpGlobals->time )
		return;

	int iAnim;

	switch ( m_fInAttack )
	{
	case 0:
		switch ( RANDOM_LONG( 0, 2 ) )
		{
		case 0:
			iAnim = XHAND_IDLE1;
			break;
		case 1:
			iAnim = XHAND_IDLE2;
			break;
		case 2:
			iAnim = XHAND_IDLE3;
			break;
		}
		SendWeaponAnim( iAnim );
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT( 3, 6 );
		break;
	case 1:
		ClearBeams( );
		PowerDown( );
		m_fInAttack = 0;
		break;
	case 2:
		ClearBeams( );
		ZapFire( );
		m_fInAttack = 0;
		break;
	}
}


void CXHand::HitSound( CBaseEntity *pEntity )
{
	if ( pEntity->Classify() == CLASS_NONE )
	{
		MissSound( );
	}
	else if ( pEntity->Classify() != CLASS_MACHINE && !FClassnameIs( pEntity->pev, "monster_apache" ) && !FClassnameIs( pEntity->pev, "monster_loader_armed" ) )
	{
		// flesh
		switch( RANDOM_LONG(0,2) )
		{
		case 0:
			EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "player/pl_clawstrike1.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 98, 102 ) ); break;
		case 1:
			EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "player/pl_clawstrike2.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 98, 102 ) ); break;
		case 2:
			EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "player/pl_clawstrike3.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 98, 102 ) ); break;
		}
	}
	else
	{
		// metal
		switch( RANDOM_LONG(0,1) )
		{
		case 0:
			EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "player/pl_clawstrike4.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 98, 102 ) ); break;
		case 1:
			EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "player/pl_clawstrike5.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 98, 102 ) ); break;
		}
	}

	CSoundEnt::InsertSound( bits_SOUND_COMBAT, m_pPlayer->pev->origin, 600, 1.0 );
}


void CXHand::MissSound( void )
{
	if ( RANDOM_LONG( 0, 1 ) )
		EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "player/pl_clawmiss1.wav", RANDOM_FLOAT( 0.8, 1.0 ), ATTN_NORM, 0, RANDOM_LONG( 98, 102 ) );
	else
		EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "player/pl_clawmiss2.wav", RANDOM_FLOAT( 0.8, 1.0 ), ATTN_NORM, 0, RANDOM_LONG( 98, 102 ) );

	CSoundEnt::InsertSound( bits_SOUND_COMBAT, m_pPlayer->pev->origin, 200, 0.5 );
}
