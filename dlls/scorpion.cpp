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

// Wanted: the scorpions (weapon_scorpion) and the scorpion they throw
// (monster_scorpion), written from the snark. A scorpion runs at the
// nearest enemy it sees, stings and dies after 15 s, without the snark's
// explosion. The file name is ours: retail's code sits between
// schedule.obj and scripted.obj.

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

// sequences of models/w_scorpion.mdl (the snark's names)
enum w_squeak_e {
	WSQUEAK_IDLE1 = 0,
	WSQUEAK_FIDGET,
	WSQUEAK_JUMP,
	WSQUEAK_RUN,
};

// sequences of models/v_scorpion.mdl (the snark's names)
enum squeak_e {
	SQUEAK_IDLE1 = 0,
	SQUEAK_FIDGETFIT,
	SQUEAK_FIDGETNIP,
	SQUEAK_DOWN,
	SQUEAK_UP,
	SQUEAK_THROW
};

class CScorpion : public CGrenade
{
	void Spawn( void );
	void Precache( void );
	int  Classify( void );
	void EXPORT SuperBounceTouch( CBaseEntity *pOther );
	void EXPORT HuntThink( void );
	int  BloodColor( void ) { return BLOOD_COLOR_RED; }
	void Killed( entvars_t *pevAttacker, int iGib );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	static float m_flNextBounceSoundTime;

	// CBaseEntity *m_pTarget;
	float m_flDie;
	Vector m_vecTarget;
	float m_flNextHunt;
	float m_flNextHit;
	Vector m_posPrev;
	EHANDLE m_hOwner;
	int  m_iMyClass;
};

float CScorpion::m_flNextBounceSoundTime = 0;

LINK_ENTITY_TO_CLASS( monster_scorpion, CScorpion );
TYPEDESCRIPTION	CScorpion::m_SaveData[] =
{
	DEFINE_FIELD( CScorpion, m_flDie, FIELD_TIME ),
	DEFINE_FIELD( CScorpion, m_vecTarget, FIELD_VECTOR ),
	DEFINE_FIELD( CScorpion, m_flNextHunt, FIELD_TIME ),
	DEFINE_FIELD( CScorpion, m_flNextHit, FIELD_TIME ),
	DEFINE_FIELD( CScorpion, m_posPrev, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CScorpion, m_hOwner, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CScorpion, CGrenade );

#define SQUEEK_DETONATE_DELAY	15.0

int CScorpion :: Classify ( void )
{
	if (m_iMyClass != 0)
		return m_iMyClass; // protect against recursion

	if (m_hEnemy != NULL)
	{
		m_iMyClass = CLASS_INSECT; // no one cares about it
		switch( m_hEnemy->Classify( ) )
		{
			case CLASS_PLAYER:
			case CLASS_HUMAN_PASSIVE:
			case CLASS_HUMAN_MILITARY:
				m_iMyClass = 0;
				return CLASS_ALIEN_MILITARY; // barney's get mad, grunts get mad at it
		}
		m_iMyClass = 0;
	}

	return CLASS_ALIEN_BIOWEAPON;
}

void CScorpion :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_SLIDEBOX;

	SET_MODEL(ENT(pev), "models/w_scorpion.mdl");
	UTIL_SetSize(pev, Vector( -5, -5, 0), Vector(5, 5, 10));
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( SuperBounceTouch );
	SetThink( HuntThink );
	pev->nextthink = gpGlobals->time + 0.1;
	m_flNextHunt = gpGlobals->time + 2.0;

	pev->flags |= FL_MONSTER;
	pev->takedamage		= DAMAGE_AIM;
	pev->health			= gSkillData.scorpionHealth;
	pev->gravity		= 0.5;
	pev->friction		= 0.6;

	m_flDie = gpGlobals->time + SQUEEK_DETONATE_DELAY;

	m_flFieldOfView = 0; // 180 degrees

	if ( pev->owner )
		m_hOwner = Instance( pev->owner );

	m_flNextBounceSoundTime = gpGlobals->time;// reset each time a snark is spawned.

	pev->sequence = WSQUEAK_RUN;
	ResetSequenceInfo( );
}

void CScorpion::Precache( void )
{
	PRECACHE_MODEL("models/w_scorpion.mdl");
	PRECACHE_SOUND("scorpion/scorp_die1.wav");
	PRECACHE_SOUND("scorpion/scorp_hunt1.wav");
	PRECACHE_SOUND("scorpion/scorp_hunt2.wav");
	PRECACHE_SOUND("scorpion/scorp_hunt3.wav");
	PRECACHE_SOUND("scorpion/scorp_deploy1.wav");
}


// Wanted: no blast and no blood; the scorpion just dies and fades
void CScorpion :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->model = iStringNull;// make invisible
	SetThink( SUB_Remove );
	SetTouch( NULL );
	pev->nextthink = gpGlobals->time + 0.1;

	// since squeak grenades never leave a body behind, clear out their takedamage now.
	pev->takedamage = DAMAGE_NO;

	EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, "scorpion/scorp_die1.wav", 1, 0.5, 0, PITCH_NORM);

	// reset owner so death message happens
	if (m_hOwner != NULL)
		pev->owner = m_hOwner->edict();

	FadeMonster( );

	CBaseMonster :: Killed( pevAttacker, GIB_NEVER );
}


void CScorpion::HuntThink( void )
{
	// ALERT( at_console, "think\n" );

	if (!IsInWorld())
	{
		SetTouch( NULL );
		UTIL_Remove( this );
		return;
	}

	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	// die when ready
	if (gpGlobals->time >= m_flDie)
	{
		g_vecAttackDir = pev->velocity.Normalize( );
		pev->health = -1;
		Killed( pev, 0 );
		return;
	}

	// float
	if (pev->waterlevel != 0)
	{
		if (pev->movetype == MOVETYPE_BOUNCE)
		{
			pev->movetype = MOVETYPE_FLY;
		}
		pev->velocity = pev->velocity * 0.9;
		pev->velocity.z += 8.0;
	}
	else if (pev->movetype = MOVETYPE_FLY)	// sic (the snark's)
	{
		pev->movetype = MOVETYPE_BOUNCE;
	}

	// return if not time to hunt
	if (m_flNextHunt > gpGlobals->time)
		return;

	m_flNextHunt = gpGlobals->time + 2.0;

	CBaseEntity *pOther = NULL;
	Vector vecDir;
	TraceResult tr;

	Vector vecFlat = pev->velocity;
	vecFlat.z = 0;
	vecFlat = vecFlat.Normalize( );

	UTIL_MakeVectors( pev->angles );

	if (m_hEnemy == NULL || !m_hEnemy->IsAlive())
	{
		// find target, bounce a bit towards it.
		Look( 512 );
		m_hEnemy = BestVisibleEnemy( );
	}

	// Wanted: turn back when stuck
	if ((pev->origin - m_posPrev).Length() < 1.0)
	{
		pev->velocity = -pev->velocity;
	}
	m_posPrev = pev->origin;

	if (m_hEnemy != NULL)
	{
		if (FVisible( m_hEnemy ))
		{
			// Wanted: head for it along the ground
			vecDir = m_hEnemy->EyePosition() - pev->origin;
			m_vecTarget = Vector( vecDir.x, vecDir.y, 0 ).Normalize( );
		}

		// Wanted: run at 270 units/s, keeping the vertical speed
		m_vecTarget = m_vecTarget.Normalize( ) * 270;
		pev->velocity = Vector( m_vecTarget.x, m_vecTarget.y, pev->velocity.z );

		// Wanted: sting it when a 24x24x12 box around us touches it
		Vector vecMins = pev->mins;
		Vector vecMaxs = pev->maxs;
		UTIL_SetSize( pev, Vector( -12, -12, 0 ), Vector( 12, 12, 12 ) );
		if ( Intersects( m_hEnemy ) )
		{
			TraceResult tr;
			UTIL_TraceModel( m_hEnemy->pev->origin, m_hEnemy->pev->origin, 1, m_hEnemy->edict(), &tr );
			gpGlobals->trace_ent = m_hEnemy->edict();
			gpGlobals->trace_endpos = tr.vecEndPos;
			gpGlobals->trace_hitgroup = tr.iHitgroup;
			SuperBounceTouch( m_hEnemy );
		}
		UTIL_SetSize( pev, vecMins, vecMaxs );
	}

	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->angles.z = 0;
	pev->angles.x = 0;
}


void CScorpion::SuperBounceTouch( CBaseEntity *pOther )
{
	float	flpitch;

	TraceResult tr = UTIL_GetGlobalTrace( );

	// don't hit the guy that launched this grenade
	if ( pev->owner && pOther->edict() == pev->owner )
		return;

	// at least until we've bounced once
	pev->owner = NULL;

	pev->angles.x = 0;
	pev->angles.z = 0;

	// avoid bouncing too much
	if (m_flNextHit > gpGlobals->time)
		return;

	// higher pitch as squeeker gets closer to detonation time
	flpitch = 155.0 - 60.0 * ((m_flDie - gpGlobals->time) / SQUEEK_DETONATE_DELAY);

	if ( pOther->pev->takedamage && m_flNextAttack < gpGlobals->time )
	{
		// attack!

		// make sure it's me who has touched them
		if (tr.pHit == pOther->edict())
		{
			// and it's not another squeakgrenade
			if (tr.pHit->v.modelindex != pev->modelindex)
			{
				// ALERT( at_console, "hit enemy\n");
				ClearMultiDamage( );
				pOther->TraceAttack(pev, gSkillData.scorpionDmgSting, gpGlobals->v_forward, &tr, DMG_POISON );
				if (m_hOwner != NULL)
					ApplyMultiDamage( pev, m_hOwner->pev );
				else
					ApplyMultiDamage( pev, pev );

				// make bite sound
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "scorpion/scorp_deploy1.wav", 1.0, ATTN_NORM, 0, (int)flpitch);
				m_flNextAttack = gpGlobals->time + 0.5;
			}
		}
		else
		{
			// ALERT( at_console, "been hit\n");
		}
	}

	m_flNextHit = gpGlobals->time + 0.1;
	m_flNextHunt = gpGlobals->time;

	if ( g_pGameRules->IsMultiplayer() )
	{
		// in multiplayer, we limit how often snarks can make their bounce sounds to prevent overflows.
		if ( gpGlobals->time < m_flNextBounceSoundTime )
		{
			// too soon!
			return;
		}
	}

	if (!(pev->flags & FL_ONGROUND))
	{
		// play bounce sound
		float flRndSound = RANDOM_FLOAT ( 0 , 1 );

		if ( flRndSound <= 0.33 )
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scorpion/scorp_hunt1.wav", 1, ATTN_NORM, 0, (int)flpitch);
		else if (flRndSound <= 0.66)
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scorpion/scorp_hunt2.wav", 1, ATTN_NORM, 0, (int)flpitch);
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scorpion/scorp_hunt3.wav", 1, ATTN_NORM, 0, (int)flpitch);
		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 256, 0.25 );
	}
	else
	{
		// skittering sound
		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 100, 0.1 );
	}

	m_flNextBounceSoundTime = gpGlobals->time + 0.5;// half second.
}


class CScorp : public CBasePlayerWeapon	// name ours (the port's)
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 5; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );
	void WeaponIdle( void );
	int m_fJustThrown;
};
LINK_ENTITY_TO_CLASS( weapon_scorpion, CScorp );


void CScorp::Spawn( )
{
	Precache( );
	m_iId = WEAPON_SCORPION;
	SET_MODEL(ENT(pev), "models/w_scorpnest.mdl");

	FallInit();//get ready to fall down.

	m_iDefaultAmmo = 5;

	pev->sequence = 1;
	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0;
}


void CScorp::Precache( void )
{
	PRECACHE_MODEL("models/w_scorpnest.mdl");
	PRECACHE_MODEL("models/v_scorpion.mdl");
	PRECACHE_MODEL("models/p_scorpion.mdl");
	PRECACHE_SOUND("scorpion/scorp_hunt2.wav");
	PRECACHE_SOUND("scorpion/scorp_hunt3.wav");
	UTIL_PrecacheOther("monster_scorpion");
}


int CScorp::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "scorpions";
	p->iMaxAmmo1 = 20;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_SCORPION;
	p->iWeight = 5;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}



BOOL CScorp::Deploy( )
{
	// play hunt sound
	float flRndSound = RANDOM_FLOAT ( 0 , 1 );

	if ( flRndSound <= 0.5 )
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scorpion/scorp_hunt2.wav", 1, ATTN_NORM, 0, 100);
	else
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scorpion/scorp_hunt3.wav", 1, ATTN_NORM, 0, 100);

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

	return DefaultDeploy( "models/v_scorpion.mdl", "models/p_scorpion.mdl", SQUEAK_UP, "scorpion" );
}


void CScorp::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;

	if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_SCORPION);
		SetThink( DestroyItem );
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	SendWeaponAnim( SQUEAK_DOWN );
}


void CScorp::PrimaryAttack()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		TraceResult tr;
		Vector trace_origin;

		// HACK HACK:  Ugly hacks to handle change in origin based on new physics code for players
		// Move origin up if crouched and start trace a bit outside of body ( 20 units instead of 16 )
		trace_origin = m_pPlayer->pev->origin;
		if ( m_pPlayer->pev->flags & FL_DUCKING )
		{
			trace_origin = trace_origin - ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );
		}

		// find place to toss monster
		UTIL_TraceLine( trace_origin + gpGlobals->v_forward * 20, trace_origin + gpGlobals->v_forward * 64, dont_ignore_monsters, NULL, &tr );

		if (tr.fAllSolid == 0 && tr.fStartSolid == 0 && tr.flFraction > 0.25)
		{
			SendWeaponAnim( SQUEAK_THROW );

			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

			CBaseEntity *pSqueak = CBaseEntity::Create( "monster_scorpion", tr.vecEndPos, m_pPlayer->pev->v_angle, m_pPlayer->edict() );

			pSqueak->pev->velocity = gpGlobals->v_forward * 200 + m_pPlayer->pev->velocity;

			// play hunt sound
			float flRndSound = RANDOM_FLOAT ( 0 , 1 );

			if ( flRndSound <= 0.5 )
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scorpion/scorp_hunt2.wav", 1, ATTN_NORM, 0, 105);
			else
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scorpion/scorp_hunt3.wav", 1, ATTN_NORM, 0, 105);

			m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

			m_fJustThrown = 1;

			m_flNextPrimaryAttack = gpGlobals->time + 0.3;
			m_flTimeWeaponIdle = gpGlobals->time + 1.0;
		}
	}
}


void CScorp::SecondaryAttack( void )
{

}


void CScorp::WeaponIdle( void )
{
	m_pPlayer->m_iFOV = 0;

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if (m_fJustThrown)
	{
		m_fJustThrown = 0;

		if ( !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] )
		{
			RetireWeapon();
			return;
		}

		SendWeaponAnim( SQUEAK_UP );
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
		return;
	}

	int iAnim;
	float flRand = RANDOM_FLOAT(0, 1);
	if (flRand <= 0.75)
	{
		iAnim = SQUEAK_IDLE1;
		m_flTimeWeaponIdle = gpGlobals->time + 30.0 / 16 * (2);
	}
	else if (flRand <= 0.875)
	{
		iAnim = SQUEAK_FIDGETFIT;
		m_flTimeWeaponIdle = gpGlobals->time + 70.0 / 16.0;
	}
	else
	{
		iAnim = SQUEAK_FIDGETNIP;
		m_flTimeWeaponIdle = gpGlobals->time + 80.0 / 16.0;
	}
	SendWeaponAnim( iAnim );
}
