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
#include "effects.h"
#include "gamerules.h"

#define	TRIPMINE_PRIMARY_VOLUME		450



enum tripmine_e {
	TRIPMINE_IDLE1 = 0,
	TRIPMINE_IDLE2,
	TRIPMINE_ARM1,
	TRIPMINE_ARM2,
	TRIPMINE_FIDGET,
	TRIPMINE_HOLSTER,
	TRIPMINE_DRAW,
	TRIPMINE_WORLD,
	TRIPMINE_GROUND,
};


#ifndef CLIENT_DLL

class CTripmineGrenade : public CGrenade
{
public:
	void Spawn( void );
	void Precache( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	void EXPORT WarningThink( void );
	void EXPORT PowerupThink( void );
	void EXPORT BeamBreakThink( void );
	void EXPORT DelayDeathThink( void );
	void Killed( entvars_t *pevAttacker, int iGib );

	virtual void MakeBeam( void );
	virtual void KillBeam( void );

	BOOL CanBeTeleported( void ) { return FALSE; }
	BOOL CanBeRepulsed( bool fForce ) { return FALSE; }
	const char *GetKillDesc( void ) { return "a laser-activated anti-personnel mine"; }

	float		m_flPowerUp;
	Vector		m_vecDir;
	Vector		m_vecEnd;
	float		m_flBeamLength;

	EHANDLE		m_hOwner;
	CBeam		*m_pBeam;
	Vector		m_posOwner;
	Vector		m_angleOwner;
	edict_t		*m_pRealOwner;// tracelines don't hit PEV->OWNER, which means a player couldn't detonate his own trip mine, so we store the owner here.
};

LINK_ENTITY_TO_CLASS( monster_tripmine, CTripmineGrenade );

TYPEDESCRIPTION	CTripmineGrenade::m_SaveData[] =
{
	DEFINE_FIELD( CTripmineGrenade, m_flPowerUp, FIELD_TIME ),
	DEFINE_FIELD( CTripmineGrenade, m_vecDir, FIELD_VECTOR ),
	DEFINE_FIELD( CTripmineGrenade, m_vecEnd, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CTripmineGrenade, m_flBeamLength, FIELD_FLOAT ),
	DEFINE_FIELD( CTripmineGrenade, m_hOwner, FIELD_EHANDLE ),
	DEFINE_FIELD( CTripmineGrenade, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CTripmineGrenade, m_posOwner, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CTripmineGrenade, m_angleOwner, FIELD_VECTOR ),
	DEFINE_FIELD( CTripmineGrenade, m_pRealOwner, FIELD_EDICT ),
};

IMPLEMENT_SAVERESTORE(CTripmineGrenade,CGrenade);


void CTripmineGrenade :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;

	SET_MODEL(ENT(pev), "models/v_tripmine.mdl");
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = TRIPMINE_WORLD;
	ResetSequenceInfo( );
	pev->framerate = 0;

	UTIL_SetSize(pev, Vector( -8, -8, -8), Vector(8, 8, 8));
	UTIL_SetOrigin( pev, pev->origin );

	if (pev->spawnflags & 1)
	{
		// power up quickly
		m_flPowerUp = gpGlobals->time + 1.0;
	}
	else
	{
		// power up in 2.5 seconds
		m_flPowerUp = gpGlobals->time + 2.5;
	}

	SetThink( PowerupThink );
	pev->nextthink = gpGlobals->time + 0.2;

	pev->takedamage = DAMAGE_YES;
	pev->dmg = gSkillData.plrDmgTripmine;
	pev->health = 1; // don't let die normally

	if (pev->owner != NULL)
	{
		// play deploy sound
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/mine_deploy.wav", 1.0, ATTN_NORM );
		EMIT_SOUND( ENT(pev), CHAN_BODY, "weapons/mine_charge.wav", 0.2, ATTN_NORM ); // chargeup

		m_pRealOwner = pev->owner;// see CTripmineGrenade for why.
	}

	UTIL_MakeAimVectors( pev->angles );

	m_vecDir = gpGlobals->v_forward;
	m_vecEnd = pev->origin + m_vecDir * 2048;
}


void CTripmineGrenade :: Precache( void )
{
	PRECACHE_MODEL("models/v_tripmine.mdl");
	PRECACHE_SOUND("weapons/mine_deploy.wav");
	PRECACHE_SOUND("weapons/mine_activate.wav");
	PRECACHE_SOUND("weapons/mine_charge.wav");
}


void CTripmineGrenade :: WarningThink( void  )
{
	// play warning sound
	// EMIT_SOUND( ENT(pev), CHAN_VOICE, "buttons/Blip2.wav", 1.0, ATTN_NORM );

	// set to power up
	SetThink( PowerupThink );
	pev->nextthink = gpGlobals->time + 1.0;
}


void CTripmineGrenade :: PowerupThink( void  )
{
	TraceResult tr;

	if (m_hOwner == NULL)
	{
		// find an owner
		edict_t *oldowner = pev->owner;
		pev->owner = NULL;
		UTIL_TraceLine( pev->origin + m_vecDir * 8, pev->origin - m_vecDir * 32, dont_ignore_monsters, ENT( pev ), &tr );
		if (tr.fStartSolid || (oldowner && tr.pHit == oldowner))
		{
			pev->owner = oldowner;
			m_flPowerUp += 0.1;
			pev->nextthink = gpGlobals->time + 0.1;
			return;
		}
		if (tr.flFraction < 1.0)
		{
			pev->owner = tr.pHit;
			m_hOwner = CBaseEntity::Instance( pev->owner );
			m_posOwner = m_hOwner->pev->origin;
			m_angleOwner = m_hOwner->pev->angles;
		}
		else
		{
			STOP_SOUND( ENT(pev), CHAN_VOICE, "weapons/mine_deploy.wav" );
			STOP_SOUND( ENT(pev), CHAN_BODY, "weapons/mine_charge.wav" );
			SetThink( SUB_Remove );
			pev->nextthink = gpGlobals->time + 0.1;
			ALERT( at_console, "WARNING:Tripmine at %.0f, %.0f, %.0f removed\n", pev->origin.x, pev->origin.y, pev->origin.z );
			KillBeam();
			return;
		}
	}
	else if (m_posOwner != m_hOwner->pev->origin || m_angleOwner != m_hOwner->pev->angles)
	{
		// disable
		STOP_SOUND( ENT(pev), CHAN_VOICE, "weapons/mine_deploy.wav" );
		STOP_SOUND( ENT(pev), CHAN_BODY, "weapons/mine_charge.wav" );
		CBaseEntity *pMine = Create( "weapon_tripmine", pev->origin + m_vecDir * 24, pev->angles );
		pMine->pev->spawnflags |= SF_NORESPAWN;
		((CBasePlayerWeapon *)pMine)->m_iDefaultAmmo = 1;	// one mine back, not a full give

		SetThink( SUB_Remove );
		KillBeam();
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}
	// ALERT( at_console, "%d %.0f %.0f %0.f\n", pev->owner, m_pOwner->pev->origin.x, m_pOwner->pev->origin.y, m_pOwner->pev->origin.z );

	pev->nextthink = gpGlobals->time + 0.1;

	if (gpGlobals->time > m_flPowerUp)
	{
		// make solid
		pev->solid = SOLID_BBOX;
		UTIL_SetOrigin( pev, pev->origin );

		MakeBeam( );

		// play enabled sound
        EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "weapons/mine_activate.wav", 0.5, ATTN_NORM, 1.0, 75 );
	}
}


void CTripmineGrenade :: KillBeam( void )
{
	if ( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
}


void CTripmineGrenade :: MakeBeam( void )
{
	TraceResult tr;

	// ALERT( at_console, "serverflags %f\n", gpGlobals->serverflags );

	UTIL_TraceLine( pev->origin, m_vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

	m_flBeamLength = tr.flFraction;

	// set to follow laser spot
	SetThink( BeamBreakThink );
	pev->nextthink = gpGlobals->time + 0.1;

	Vector vecTmpEnd = pev->origin + m_vecDir * 2048 * m_flBeamLength;

	m_pBeam = CBeam::BeamCreate( g_pModelNameLaser, 10 );
	m_pBeam->PointEntInit( vecTmpEnd, entindex() );
	m_pBeam->SetColor( 0, 214, 198 );
	m_pBeam->SetScrollRate( 255 );
	m_pBeam->SetBrightness( 64 );
}


void CTripmineGrenade :: BeamBreakThink( void  )
{
	BOOL bBlowup = 0;

	TraceResult tr;

	// HACKHACK Set simple box using this really nice global!
	gpGlobals->trace_flags = FTRACE_SIMPLEBOX;
	UTIL_TraceLine( pev->origin, m_vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

	// ALERT( at_console, "%f : %f\n", tr.flFraction, m_flBeamLength );

	// respawn detect.
	if ( !m_pBeam )
	{
		MakeBeam( );
		if ( tr.pHit )
			m_hOwner = CBaseEntity::Instance( tr.pHit );	// reset owner too
	}

	if (fabs( m_flBeamLength - tr.flFraction ) > 0.001)
	{
		bBlowup = 1;
	}
	else
	{
		if (m_hOwner == NULL)
			bBlowup = 1;
		else if (m_posOwner != m_hOwner->pev->origin)
			bBlowup = 1;
		else if (m_angleOwner != m_hOwner->pev->angles)
			bBlowup = 1;
	}

	if (bBlowup)
	{
		// a bit of a hack, but all CGrenade code passes pev->owner along to make sure the proper player gets credit for the kill
		// so we have to restore pev->owner from pRealOwner, because an entity's tracelines don't strike it's pev->owner which meant
		// that a player couldn't trigger his own tripmine. Now that the mine is exploding, it's safe the restore the owner so the
		// CGrenade code knows who the explosive really belongs to.
		pev->owner = m_pRealOwner;
		pev->health = 0;
		Killed( VARS( pev->owner ), GIB_NORMAL );
		return;
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

int CTripmineGrenade :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if (gpGlobals->time < m_flPowerUp && flDamage < pev->health)
	{
		// disable
		// Create( "weapon_tripmine", pev->origin + m_vecDir * 24, pev->angles );
		SetThink( SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.1;
		KillBeam();
		return FALSE;
	}
	return CGrenade::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CTripmineGrenade::Killed( entvars_t *pevAttacker, int iGib )
{
	pev->takedamage = DAMAGE_NO;

	if ( pevAttacker && ( pevAttacker->flags & FL_CLIENT ) )
	{
		// some client has destroyed this mine, he'll get credit for any kills
		pev->owner = ENT( pevAttacker );
	}

	SetThink( DelayDeathThink );
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.1, 0.3 );

	EMIT_SOUND( ENT(pev), CHAN_BODY, "common/null.wav", 0.5, ATTN_NORM ); // shut off chargeup
}


void CTripmineGrenade::DelayDeathThink( void )
{
	KillBeam();
	TraceResult tr;
	UTIL_TraceLine ( pev->origin + m_vecDir * 8, pev->origin - m_vecDir * 64,  dont_ignore_monsters, ENT(pev), & tr);

	Explode( &tr, DMG_BLAST );
}


//=========================================================
// monster_tripmine_turret: the trip mine's secondary attack. A mine whose
// red beam sweeps for enemies and fires laser pulses along it (48 shots),
// then blows up like a trip mine.
//=========================================================
class CTripmineTurret : public CTripmineGrenade
{
public:
	void Spawn( void );
	void Precache( void );
	int  Classify( void ) { return CLASS_NONE; }
	const char *GetKillDesc( void )
	{
		if ( m_pRealOwner )
		{
			CBaseEntity *pOwner = CBaseEntity::Instance( m_pRealOwner );
			if ( pOwner )
				return pOwner->GetKillDesc();
		}
		return STRING( m_iszKillDesc );
	}
	int IRelationship( CBaseEntity *pTarget );
	void MakeBeam( void );
	BOOL FInViewCone( CBaseEntity *pEntity );

	void EXPORT ActiveThink( void );
	void MoveTurret( void );					// name ours
	void Shoot( const Vector &vecDir );			// name ours

	// m_vecAngle1: the goal angles, m_vecAngle2: where the beam points now
	EHANDLE		m_hPulse;						// the last pulse fired; name ours
};

LINK_ENTITY_TO_CLASS( monster_tripmine_turret, CTripmineTurret );


void CTripmineTurret :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;

	SET_MODEL(ENT(pev), "models/v_tripmine.mdl");
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = TRIPMINE_WORLD;
	ResetSequenceInfo( );
	pev->framerate = 0;

	UTIL_SetSize(pev, Vector( -8, -8, -8), Vector(8, 8, 8));
	UTIL_SetOrigin( pev, pev->origin );

	if (pev->spawnflags & 1)
	{
		// power up quickly
		m_flPowerUp = gpGlobals->time + 1.0;
	}
	else
	{
		// power up in 2.5 seconds
		m_flPowerUp = gpGlobals->time + 2.5;
	}

	SetThink( PowerupThink );
	pev->nextthink = gpGlobals->time + 0.2;

	m_iszKillDesc = MAKE_STRING( "a laser-targeted pulse cannon" );

	pev->takedamage = DAMAGE_YES;
	pev->dmg = 50;
	pev->frags = 48;	// shots
	pev->health = 1; // don't let die normally

	if (pev->owner != NULL)
	{
		// play deploy sound
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/mine_deploy.wav", 1.0, ATTN_NORM );
		EMIT_SOUND( ENT(pev), CHAN_BODY, "weapons/mine_charge.wav", 0.2, ATTN_NORM ); // chargeup

		m_pRealOwner = pev->owner;// see CTripmineGrenade for why.
	}

	UTIL_MakeAimVectors( pev->angles );
	m_flFieldOfView = 0.7;

	m_vecDir = gpGlobals->v_forward;
	m_vecAngle2 = pev->angles;
	m_vecAngle1 = pev->angles;
	m_vecEnd = pev->origin + m_vecDir * 2048;
}


int CTripmineTurret :: IRelationship( CBaseEntity *pTarget )
{
	if ( !pTarget->Classify() )
		return R_NO;

	return pTarget->edict() != m_pRealOwner;
}


void CTripmineTurret :: Precache( void )
{
	PRECACHE_MODEL("models/v_tripmine.mdl");
	PRECACHE_SOUND("weapons/mine_deploy.wav");
	PRECACHE_SOUND("weapons/mine_activate.wav");
	PRECACHE_SOUND("weapons/mine_charge.wav");
}


void CTripmineTurret :: MakeBeam( void )
{
	TraceResult tr;

	UTIL_TraceLine( pev->origin, m_vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

	SetThink( ActiveThink );
	pev->nextthink = gpGlobals->time + 0.1;

	m_pBeam = CBeam::BeamCreate( g_pModelNameLaser, 10 );
	m_pBeam->PointEntInit( tr.vecEndPos, entindex() );
	m_pBeam->SetColor( 255, 16, 16 );
	m_pBeam->SetScrollRate( 35 );
	m_pBeam->SetBrightness( 128 );
}


BOOL CTripmineTurret :: FInViewCone( CBaseEntity *pEntity )
{
	Vector vecLOS = ( pEntity->pev->origin - pev->origin ).Normalize();

	float flDot = DotProduct( vecLOS, m_vecDir );

	if ( flDot > m_flFieldOfView )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


void CTripmineTurret :: ActiveThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ( m_hEnemy == NULL || !m_hEnemy->IsAlive() || !FVisible( m_hEnemy->BodyTarget( pev->origin ) ) )
	{
		Look( 2048 );
		m_hEnemy = BestVisibleEnemy();

		if ( m_hEnemy == NULL )
		{
			m_vecAngle1 = pev->angles;
			MoveTurret();
			return;
		}
	}

	Vector vecDirToEnemy = m_hEnemy->BodyTarget( pev->origin ) - pev->origin;
	m_vecAngle1 = UTIL_VecToAngles( vecDirToEnemy );
	MoveTurret();
}


void CTripmineTurret :: MoveTurret( void )
{
	TraceResult tr;

	if ( m_hPulse == NULL && m_vecAngle2.x != m_vecAngle1.x )
	{
		float flDir = m_vecAngle1.x > m_vecAngle2.x ? 1 : -1 ;
		float flDist = fabs( m_vecAngle1.x - m_vecAngle2.x );

		if ( flDist > 180 )
		{
			flDist = 360 - flDist;
			flDir = -flDir;
		}

		if ( flDist < 1.0 )
			m_vecAngle2.x = m_vecAngle1.x;
		else
			m_vecAngle2.x += flDir;
	}

	if ( m_hPulse == NULL && m_vecAngle2.y != m_vecAngle1.y )
	{
		float flDir = m_vecAngle1.y > m_vecAngle2.y ? 1 : -1 ;
		float flDist = fabs( m_vecAngle1.y - m_vecAngle2.y );

		if ( flDist > 180 )
		{
			flDist = 360 - flDist;
			flDir = -flDir;
		}

		if ( flDist < 1.0 )
			m_vecAngle2.y = m_vecAngle1.y;
		else
			m_vecAngle2.y += flDir;

		if ( m_vecAngle2.y < 0 )
			m_vecAngle2.y += 360;
		else if ( m_vecAngle2.y >= 360 )
			m_vecAngle2.y -= 360;
	}

	UTIL_MakeAimVectors( m_vecAngle2 );
	m_vecEnd = pev->origin + gpGlobals->v_forward * 2048;

	UTIL_TraceLine( pev->origin, m_vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
	m_pBeam->pev->origin = tr.vecEndPos;

	if ( tr.pHit && gpGlobals->time > m_flNextAttack )
	{
		if ( pev->frags > 0 )
		{
			if ( IRelationship( CBaseEntity::Instance( tr.pHit ) ) > R_NO )
				Shoot( gpGlobals->v_forward );
		}
		else if ( m_hPulse == NULL )
		{
			// out of shots: blow up like a trip mine
			pev->owner = m_pRealOwner;
			pev->health = 0;
			Killed( VARS( pev->owner ), GIB_NORMAL );
		}
	}
}


void CTripmineTurret :: Shoot( const Vector &vecDir )
{
	m_hPulse = CLaserPulse::Shoot( pev->origin, edict(), Vector( 255, 16, 16 ), 1, 0.25, 0 );
	m_hPulse->pev->velocity = vecDir * 2000;
	m_hPulse->pev->dmg = 20;
	m_hPulse->pev->spawnflags |= 4;

	if ( m_pRealOwner )
		m_hPulse->m_iszKillDesc = m_iszKillDesc;
	else
		m_hPulse->m_iszKillDesc = MAKE_STRING( "a laser blast" );

	if ( RANDOM_LONG( 0, 1 ) )
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/pl_gun1.wav", 1, ATTN_NORM );
	else
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/pl_gun2.wav", 1, ATTN_NORM );

	m_flNextAttack = gpGlobals->time + 0.2;
	pev->frags -= 1;
}
#endif

LINK_ENTITY_TO_CLASS( weapon_tripmine, CTripmine );

void CTripmine::Spawn( )
{
	Precache( );
	m_iId = WEAPON_TRIPMINE;
	SET_MODEL(ENT(pev), "models/v_tripmine.mdl");
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = TRIPMINE_GROUND;
	// ResetSequenceInfo( );
	pev->framerate = 0;

	FallInit();// get ready to fall down
	pev->solid = SOLID_TRIGGER;

	m_iDefaultAmmo = TRIPMINE_DEFAULT_GIVE;

#ifdef CLIENT_DLL
	if ( !bIsMultiplayer() )
#else
	if ( !g_pGameRules->IsMultiplayer() )
#endif
	{
		UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 28) );
	}
}

void CTripmine::Precache( void )
{
	PRECACHE_MODEL ("models/v_tripmine.mdl");
	PRECACHE_MODEL ("models/p_tripmine.mdl");
	UTIL_PrecacheOther( "monster_tripmine" );
	UTIL_PrecacheOther( "monster_tripmine_turret" );
}

int CTripmine::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Trip Mine";
	p->iMaxAmmo1 = TRIPMINE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_TRIPMINE;
	p->iWeight = TRIPMINE_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}

BOOL CTripmine::Deploy( )
{
	pev->body = 0;
	return DefaultDeploy( "models/v_tripmine.mdl", "models/p_tripmine.mdl", TRIPMINE_DRAW, "trip" );
}


void CTripmine::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		// out of mines
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_TRIPMINE);
		SetThink( DestroyItem );
		pev->nextthink = gpGlobals->time + 0.1;
	}

	SendWeaponAnim( TRIPMINE_HOLSTER );
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

void CTripmine::PrimaryAttack( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = gpGlobals->v_forward;

	TraceResult tr;

	UTIL_TraceLine( vecSrc, vecSrc + vecAiming * 128, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

	if (tr.flFraction < 1.0)
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		if ( pEntity && !(pEntity->pev->flags & FL_CONVEYOR) )
		{
			Vector angles = UTIL_VecToAngles( tr.vecPlaneNormal );

			CBaseEntity *pEnt = CBaseEntity::Create( "monster_tripmine", tr.vecEndPos + tr.vecPlaneNormal * 8, angles, m_pPlayer->edict() );

			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

			if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
			{
				SendWeaponAnim( TRIPMINE_DRAW );
			}
			else
			{
				// no more mines! 
				RetireWeapon();
				return;
			}
		}
		else
		{
			// ALERT( at_console, "no deploy\n" );
		}
	}
	else
	{

	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flNextWeaponSpecial = UTIL_WeaponTimeBase() + 0.3;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

// the turret mine
void CTripmine::SecondaryAttack( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = gpGlobals->v_forward;

	TraceResult tr;

	UTIL_TraceLine( vecSrc, vecSrc + vecAiming * 128, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

	if (tr.flFraction < 1.0)
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		if ( pEntity && !(pEntity->pev->flags & FL_CONVEYOR) )
		{
			Vector angles = UTIL_VecToAngles( tr.vecPlaneNormal );

			CBaseEntity *pEnt = CBaseEntity::Create( "monster_tripmine_turret", tr.vecEndPos + tr.vecPlaneNormal * 8, angles, m_pPlayer->edict() );

			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

			if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
			{
				SendWeaponAnim( TRIPMINE_DRAW );
			}
			else
			{
				// no more mines! 
				RetireWeapon();
				return;
			}
		}
		else
		{
			// ALERT( at_console, "no deploy\n" );
		}
	}
	else
	{

	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flNextWeaponSpecial = UTIL_WeaponTimeBase() + 0.3;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

// sic: no entity is called target_designator, so Create fails ("NULL Ent in Create!") but the mine is spent
void CTripmine::WeaponSpecial( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = gpGlobals->v_forward;

	TraceResult tr;

	UTIL_TraceLine( vecSrc, vecSrc + vecAiming * 128, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

	if (tr.flFraction < 1.0)
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		if ( pEntity && !(pEntity->pev->flags & FL_CONVEYOR) )
		{
			Vector angles = UTIL_VecToAngles( tr.vecPlaneNormal );

			CBaseEntity *pEnt = CBaseEntity::Create( "target_designator", tr.vecEndPos + tr.vecPlaneNormal * 8, angles, m_pPlayer->edict() );

			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

			if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
			{
				SendWeaponAnim( TRIPMINE_DRAW );
			}
			else
			{
				// no more mines! 
				RetireWeapon();
				return;
			}
		}
		else
		{
			// ALERT( at_console, "no deploy\n" );
		}
	}
	else
	{

	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flNextWeaponSpecial = UTIL_WeaponTimeBase() + 0.3;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CTripmine::WeaponIdle( void )
{
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
	{
		SendWeaponAnim( TRIPMINE_DRAW );
	}
	else
	{
		RetireWeapon();
		return;
	}

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
	if (flRand <= 0.25)
	{
		iAnim = TRIPMINE_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 90.0 / 30.0;
	}
	else if (flRand <= 0.75)
	{
		iAnim = TRIPMINE_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 30.0;
	}
	else
	{
		iAnim = TRIPMINE_FIDGET;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 100.0 / 30.0;
	}

	SendWeaponAnim( iAnim );
}
