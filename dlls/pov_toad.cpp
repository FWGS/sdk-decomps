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
// POV: the toad. A thrown toad (monster_toad) hunts like
// the snark and pops; the player's weapon (weapon_toad) is
// the snark's. A toad nest lying in the world jumps to the
// player when he comes near and gives itself to him. Made
// from POV's squeakgrenade.cpp; file name and the names
// retail doesn't give are ours.
//=========================================================
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

extern DLL_GLOBAL BOOL g_fLevelLoaded;			// POV
extern int gEvilImpulse101;

enum w_toad_e {	// names ours
	WTOAD_IDLE1 = 0,
	WTOAD_FIDGET,
	WTOAD_JUMP,
	WTOAD_RUN,
};

enum toad_e {	// names ours
	TOAD_IDLE1 = 0,
	TOAD_FIDGETFIT,
	TOAD_FIDGETLOOK,
	TOAD_FIDGETNIP,
	TOAD_DOWN,
	TOAD_UP,
	TOAD_THROW
};

#define TOAD_DEFAULT_GIVE	1	// names ours
#define TOAD_MAX_CARRY		15
#define TOAD_WEIGHT			8

#define SF_TOAD_IDLE		1	// POV, name ours: sits and croaks until the flag is cleared
#define SF_TOAD_SILENT		2	// POV, name ours: no idle croaks

class CToadGrenade : public CGrenade
{
	void Spawn( void );
	void Precache( void );
	int  Classify( void );
	int  IRelationship( CBaseEntity *pTarget );
	void StartIdle( void );	// POV, name ours
	void StartHunt( void );	// POV, name ours
	void EXPORT SuperBounceTouch( CBaseEntity *pOther );
	void EXPORT IdleThink( void );
	void EXPORT HuntThink( void );
	int  BloodColor( void ) { return BLOOD_COLOR_YELLOW; }
	void Killed( entvars_t *pevAttacker, int iGib );
	void GibMonster( void );

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
	float m_flNextIdleSound;	// POV, name ours
};

float CToadGrenade::m_flNextBounceSoundTime = 0;

LINK_ENTITY_TO_CLASS( monster_toad, CToadGrenade );
TYPEDESCRIPTION	CToadGrenade::m_SaveData[] = 
{
	DEFINE_FIELD( CToadGrenade, m_flDie, FIELD_TIME ),
	DEFINE_FIELD( CToadGrenade, m_vecTarget, FIELD_VECTOR ),
	DEFINE_FIELD( CToadGrenade, m_flNextHunt, FIELD_TIME ),
	DEFINE_FIELD( CToadGrenade, m_flNextHit, FIELD_TIME ),
	DEFINE_FIELD( CToadGrenade, m_posPrev, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CToadGrenade, m_hOwner, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CToadGrenade, CGrenade );

#define TOAD_DETONATE_DELAY	15.0

int CToadGrenade :: Classify ( void )
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
			case 14:	// POV: the special forces' class
				m_iMyClass = 0;
				return CLASS_ALIEN_MILITARY; // barney's get mad, grunts get mad at it
		}
		m_iMyClass = 0;
	}

	return CLASS_ALIEN_BIOWEAPON;
}

// POV: snarks go for the player
int CToadGrenade::IRelationship( CBaseEntity *pTarget )
{
	if ( pTarget->IsPlayer() )
		return R_DL;

	return CBaseMonster::IRelationship( pTarget );
}

void CToadGrenade :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/w_toad.mdl");
	UTIL_SetSize(pev, Vector( -4, -4, 0), Vector(4, 4, 8));
	UTIL_SetOrigin( pev, pev->origin );

	pev->flags |= FL_MONSTER;
	pev->takedamage		= DAMAGE_AIM;
	pev->health			= gSkillData.toadHealth;
	pev->gravity		= 0.5;
	pev->friction		= 0.5;

	pev->dmg = gSkillData.toadDmgPop;

	m_flFieldOfView = 0; // 180 degrees

	if ( pev->owner )
		m_hOwner = Instance( pev->owner );

	if ( pev->spawnflags & SF_TOAD_IDLE )
		StartIdle( );
	else
		StartHunt( );
}

void CToadGrenade::Precache( void )
{
	PRECACHE_MODEL("models/w_toad.mdl");
	PRECACHE_SOUND("toad/toad_blast1.wav");
	PRECACHE_SOUND("common/bodysplat.wav");
	PRECACHE_SOUND("toad/toad_die1.wav");
	PRECACHE_SOUND("toad/toad_hunt1.wav");
	PRECACHE_SOUND("toad/toad_hunt2.wav");
	PRECACHE_SOUND("toad/toad_hunt3.wav");
	PRECACHE_SOUND("toad/toad_deploy1.wav");
}

void CToadGrenade::StartIdle( void )
{
	SetThink( IdleThink );
	m_flNextIdleSound = 0;
	SetTouch( NULL );
	pev->nextthink = gpGlobals->time + 0.1;

	pev->sequence = WTOAD_IDLE1;
	ResetSequenceInfo( );
}

void CToadGrenade::StartHunt( void )
{
	SetTouch( SuperBounceTouch );
	SetThink( HuntThink );
	pev->nextthink = gpGlobals->time + 0.1;
	m_flNextHunt = gpGlobals->time;

	m_flDie = gpGlobals->time + TOAD_DETONATE_DELAY;

	m_flNextBounceSoundTime = gpGlobals->time;// reset each time a snark is spawned.

	pev->sequence = WTOAD_RUN;
	ResetSequenceInfo( );
}


void CToadGrenade :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->model = iStringNull;// make invisible
	SetTouch( NULL );

	// since toad grenades never leave a body behind, clear out their takedamage now.
	// Toads do a bit of radius damage when they pop, and that radius damage will
	// continue to call this function unless we acknowledge the Toad's death now. (sjb)
	pev->takedamage = DAMAGE_NO;

	// play toad blast
	EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, "toad/toad_blast1.wav", 0.8, 0.5, 0, PITCH_NORM);	

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, SMALL_EXPLOSION_VOLUME, 3.0 );

	UTIL_BloodDrips( pev->origin, g_vecZero, BloodColor(), 80 );

	if (m_hOwner != NULL)
		RadiusDamage ( pev, m_hOwner->pev, pev->dmg, CLASS_NONE, DMG_BLAST );
	else
		RadiusDamage ( pev, pev, pev->dmg, CLASS_NONE, DMG_BLAST );

	// reset owner so death message happens
	if (m_hOwner != NULL)
		pev->owner = m_hOwner->edict();

	CBaseMonster :: Killed( pevAttacker, GIB_ALWAYS );

	SetThink( NULL );
	UTIL_Remove( this );
}

void CToadGrenade :: GibMonster( void )
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "common/bodysplat.wav", 0.75, ATTN_NORM, 0, 200);		
}



// POV: an idle snark looks around and squeaks now and then until it's
// told to hunt
void CToadGrenade::IdleThink( void )
{
	Look( 512 );

	if ( !(pev->spawnflags & SF_TOAD_IDLE) )
	{
		StartHunt( );
		return;
	}

	if ( gpGlobals->time > m_flNextIdleSound && !(pev->spawnflags & SF_TOAD_SILENT) )
	{
		switch ( RANDOM_LONG( 0, 2 ) )
		{
		case 0:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "toad/toad_hunt1.wav", RANDOM_FLOAT( 0.5, 0.85 ), ATTN_NORM );	break;
		case 1:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "toad/toad_hunt2.wav", RANDOM_FLOAT( 0.5, 0.85 ), ATTN_NORM );	break;
		default: EMIT_SOUND( ENT(pev), CHAN_VOICE, "toad/toad_hunt3.wav", RANDOM_FLOAT( 0.5, 0.85 ), ATTN_NORM );	break;
		}
		m_flNextIdleSound = gpGlobals->time + RANDOM_FLOAT( 3, 4 );
	}

	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;
}


void CToadGrenade::HuntThink( void )
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

	// explode when ready
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
	else if (pev->movetype = MOVETYPE_FLY)
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

	// squeek if it's about time blow up
	if ((m_flDie - gpGlobals->time <= 0.5) && (m_flDie - gpGlobals->time >= 0.3))
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "toad/toad_die1.wav", 1, ATTN_NORM, 0, 100 + RANDOM_LONG(0,0x3F));
		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 256, 0.25 );
	}

	// higher pitch as squeeker gets closer to detonation time
	float flpitch = 155.0 - 60.0 * ((m_flDie - gpGlobals->time) / TOAD_DETONATE_DELAY);
	if (flpitch < 80)
		flpitch = 80;

	if (m_hEnemy != NULL)
	{
		if (FVisible( m_hEnemy ))
		{
			vecDir = m_hEnemy->EyePosition() - pev->origin;
			m_vecTarget = vecDir.Normalize( );
		}

		float flVel = pev->velocity.Length();
		float flAdj = 50.0 / (flVel + 10.0);

		if (flAdj > 1.2)
			flAdj = 1.2;
		
		// ALERT( at_console, "think : enemy\n");

		// ALERT( at_console, "%.0f %.2f %.2f %.2f\n", flVel, m_vecTarget.x, m_vecTarget.y, m_vecTarget.z );

		pev->velocity = pev->velocity * flAdj + m_vecTarget * 300;
	}

	if (pev->flags & FL_ONGROUND)
	{
		pev->avelocity = Vector( 0, 0, 0 );
	}
	else
	{
		if (pev->avelocity == Vector( 0, 0, 0))
		{
			pev->avelocity.x = RANDOM_FLOAT( -100, 100 );
			pev->avelocity.z = RANDOM_FLOAT( -100, 100 );
		}
	}

	if ((pev->origin - m_posPrev).Length() < 1.0)
	{
		pev->velocity.x = RANDOM_FLOAT( -100, 100 );
		pev->velocity.y = RANDOM_FLOAT( -100, 100 );
	}
	m_posPrev = pev->origin;

	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->angles.z = 0;
	pev->angles.x = 0;
}


void CToadGrenade::SuperBounceTouch( CBaseEntity *pOther )
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
	flpitch = 155.0 - 60.0 * ((m_flDie - gpGlobals->time) / TOAD_DETONATE_DELAY);

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
				pOther->TraceAttack(pev, gSkillData.toadDmgBite, gpGlobals->v_forward, &tr, DMG_SLASH ); 
				if (m_hOwner != NULL)
					ApplyMultiDamage( pev, m_hOwner->pev );
				else
					ApplyMultiDamage( pev, pev );

				pev->dmg += gSkillData.toadDmgPop; // add more explosion damage
				// m_flDie += 2.0; // add more life

				// make bite sound
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "toad/toad_deploy1.wav", 1.0, ATTN_NORM, 0, (int)flpitch);
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
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "toad/toad_hunt1.wav", 1, ATTN_NORM, 0, (int)flpitch);		
		else if (flRndSound <= 0.66)
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "toad/toad_hunt2.wav", 1, ATTN_NORM, 0, (int)flpitch);
		else 
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "toad/toad_hunt3.wav", 1, ATTN_NORM, 0, (int)flpitch);
		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 256, 0.25 );
	}
	else
	{
		// skittering sound
		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 100, 0.1 );
	}

	m_flNextBounceSoundTime = gpGlobals->time + 0.5;// half second.
}




class CToad : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	void Holster( void );
	void WeaponIdle( void );

	void EXPORT ToadIdle( void );
	void IdleSound( BOOL fForce );	// name ours

	void StartVModelSound( int iSound );
	void EXPORT PlayVModelSound( void );
	void StopVModelSound( void );	// name ours

	static const char *pVModelSounds[];	// name ours

	int m_iSoundPlayed;
	int m_fJustThrown;
	int m_fJumping;			// names ours: jumping to the player
	float m_flNextIdleSound;
	float m_flNextTrace;
	Vector m_vecGoal;
};
LINK_ENTITY_TO_CLASS( weapon_toad, CToad );

TYPEDESCRIPTION	CToad::m_SaveData[] = 
{
	DEFINE_FIELD( CToad, m_iSoundPlayed, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CToad, CBasePlayerWeapon );

// POV: what the view model's toads say while they fidget
const char *CToad::pVModelSounds[] = 
{
	"toad/toad_hunt2.wav",
	"toad/toad_idle1.wav",
};


void CToad::Spawn( )
{
	Precache( );
	m_iId = WEAPON_TOAD;
	SET_MODEL(ENT(pev), "models/toad_nest.mdl");

	FallInit();//get ready to fall down.

	m_iDefaultAmmo = TOAD_DEFAULT_GIVE;
		
	pev->sequence = 1;
	pev->animtime = gpGlobals->time;
	pev->framerate = 0.6;
	pev->view_ofs = Vector( 0, 0, 6 );

	SetThink( ToadIdle );
	pev->nextthink = gpGlobals->time + 0.2;

	m_fJumping = 0;
	m_flNextIdleSound = 0;
	m_flNextTrace = 0;
}


void CToad::Precache( void )
{
	PRECACHE_MODEL("models/toad_nest.mdl");
	PRECACHE_MODEL("models/w_toad.mdl");
	PRECACHE_MODEL("models/v_toad.mdl");
	PRECACHE_MODEL("models/p_toad.mdl");
	PRECACHE_SOUND("toad/toad_hunt2.wav");
	PRECACHE_SOUND("toad/toad_hunt3.wav");
	UTIL_PrecacheOther("monster_toad");

	PRECACHE_SOUND_ARRAY( pVModelSounds );
}


int CToad::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Toads";
	p->iMaxAmmo1 = TOAD_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_TOAD;
	p->iWeight = TOAD_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}



BOOL CToad::Deploy( )
{
	if ( !gEvilImpulse101 )
	{
		// play hunt sound
		float flRndSound = RANDOM_FLOAT ( 0 , 1 );

		if ( flRndSound <= 0.5 )
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "toad/toad_hunt2.wav", 1, ATTN_NORM, 0, 100);
		else 
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "toad/toad_hunt3.wav", 1, ATTN_NORM, 0, 100);

		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	}

	m_flTimeWeaponIdle = gpGlobals->time + 1.1;

	return DefaultDeploy( "models/v_toad.mdl", "models/p_toad.mdl", TOAD_UP, "toad" );
}


void CToad::Holster( void )
{
	if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_TOAD);
		SetThink( DestroyItem );
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}
	
	StopVModelSound( );
	DefaultHolster( TOAD_DOWN, 1.5, 1.5 );
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}


void CToad::PrimaryAttack()
{
	if ( m_pPlayer->m_fDidHolster )
	{
		// POV: still holstering
		WeaponIdle( );
		return;
	}

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		StopVModelSound( );

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
			SendWeaponAnim( TOAD_THROW );

			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

			CBaseEntity *pToad = CBaseEntity::Create( "monster_toad", tr.vecEndPos, m_pPlayer->pev->v_angle, m_pPlayer->edict() );

			pToad->pev->velocity = gpGlobals->v_forward * 200 + m_pPlayer->pev->velocity;

			// play hunt sound
			float flRndSound = RANDOM_FLOAT ( 0 , 1 );

			if ( flRndSound <= 0.5 )
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "toad/toad_hunt2.wav", 1, ATTN_NORM, 0, 105);
			else 
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "toad/toad_hunt3.wav", 1, ATTN_NORM, 0, 105);

			m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

			m_fJustThrown = 1;

			m_flNextPrimaryAttack = gpGlobals->time + 0.3;
			m_flTimeWeaponIdle = gpGlobals->time + 1.1;
		}
	}
	else if ( m_flNextPrimaryAttack < gpGlobals->time )
	{
		// POV: click when empty
		ResetEmptySound( );
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->time + 0.5;
	}
}


void CToad::SecondaryAttack( void )
{
	if ( m_pPlayer->m_fDidHolster )
		WeaponIdle( );
}


void CToad::WeaponIdle( void )
{
	if ( g_fLevelLoaded == TRUE )
	{
		StopVModelSound( );
		return;
	}

	if ( m_pPlayer->m_fDidHolster )
	{
		// POV: the holster animation is over, draw the weapon again
		if ( m_flTimeWeaponIdle <= gpGlobals->time )
		{
			m_pPlayer->m_fDidHolster = FALSE;
			Deploy( );
			UpdateItemInfo( );
		}
		return;
	}

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

		SendWeaponAnim( TOAD_UP );
		m_flTimeWeaponIdle = gpGlobals->time + 1.1;
		return;
	}

	if ( m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] )
	{
		m_iSoundPlayed = -1;

		int iAnim;
		switch ( RANDOM_LONG( 0, 5 ) )
		{
		case 0:
			iAnim = TOAD_FIDGETFIT;
			m_flTimeWeaponIdle = gpGlobals->time + 2.2;
			StartVModelSound( 0 );
			break;
		case 1:
			iAnim = TOAD_FIDGETLOOK;
			m_flTimeWeaponIdle = gpGlobals->time + 2.18;
			break;
		case 2:
			iAnim = TOAD_FIDGETNIP;
			m_flTimeWeaponIdle = gpGlobals->time + 2.9;
			StartVModelSound( 1 );
			break;
		default:
			iAnim = TOAD_IDLE1;
			m_flTimeWeaponIdle = gpGlobals->time + 2.2;
			break;
		}
		SendWeaponAnim( iAnim );
	}
}


// A toad nest lying in the world jumps to the player once he is near
// and gives itself to him.
void CToad::ToadIdle( void )
{
	CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( 1 );

	if ( !pPlayer )
	{
		pev->nextthink = gpGlobals->time + 1.5;
		return;
	}

	if ( !pPlayer->FVisible( this ) )
	{
		if ( pev->sequence != 1 )
		{
			pev->sequence = 1;
			pev->framerate = 0.6;
			ResetSequenceInfo( );
		}

		if ( RANDOM_LONG( 0, 10 ) == 0 )
			IdleSound( TRUE );

		pev->nextthink = gpGlobals->time + 0.2;
		return;
	}

	if ( (pPlayer->Center() - Center()).Length() < 50 )
	{
		if ( !m_fJumping )
		{
			pev->nextthink = gpGlobals->time + 0.1;
			return;
		}

		for ( int i = 0; i < m_iDefaultAmmo; i++ )
			pPlayer->GiveNamedItem( "weapon_toad" );

		SetThink( NULL );
		UTIL_Remove( this );
		return;
	}

	if ( (pPlayer->Center() - Center()).Length() < 256 )
	{
		m_fJumping = 1;
		pev->movetype = MOVETYPE_STEP;
		StudioFrameAdvance( );

		if ( pev->sequence == 1 )
		{
			pev->sequence = 0;
			pev->frame = 0;
			ResetSequenceInfo( );
			pev->framerate = 3.0;
			IdleSound( TRUE );
		}
		else
		{
			Vector vecFlat;

			if ( gpGlobals->time > m_flNextTrace )
			{
				vecFlat = (Center() - pPlayer->Center()).Normalize();
				vecFlat.z = 0;
				m_vecGoal = pev->origin + vecFlat * 128;

				TraceResult tr;
				tr.flFraction = 1.0;
				UTIL_TraceLine( EyePosition(), EyePosition() + vecFlat * 68, ignore_monsters, ENT(pev), &tr );

				if ( tr.flFraction < 1.0 )
				{
					if ( CBaseEntity::Instance( tr.pHit )->Classify() )
						tr.vecPlaneNormal = (EyePosition() - tr.vecEndPos).Normalize();

					UTIL_MakeVectorsPrivate( Vector( 0, UTIL_AngleMod( UTIL_VecToAngles( tr.vecPlaneNormal ).y + 195 ), 0 ), vecFlat, NULL, NULL );
					m_vecGoal = pev->origin - vecFlat * 128;
				}

				m_flNextTrace = gpGlobals->time + 0.5;
			}

			vecFlat = m_vecGoal - Center();
			vecFlat.z = 0;
			pev->angles = UTIL_VecToAngles( vecFlat );
			UTIL_MoveToOrigin( ENT(pev), vecFlat * 256, 200 / m_flGroundSpeed, MOVE_STRAFE );

			IdleSound( FALSE );
		}
		pev->nextthink = gpGlobals->time + 0.2;
		return;
	}
	else
	{
		if ( pev->sequence != 1 )
		{
			pev->sequence = 1;
			pev->framerate = 0.6;
			ResetSequenceInfo( );
		}

		if ( RANDOM_LONG( 0, 10 ) == 0 )
			IdleSound( TRUE );
	}

	pev->nextthink = gpGlobals->time + 0.2;
}

void CToad::IdleSound( BOOL fForce )
{
	if ( gpGlobals->time > m_flNextIdleSound || fForce )
	{
		switch ( RANDOM_LONG( 0, 2 ) )
		{
		case 0:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "toad/toad_hunt1.wav", RANDOM_FLOAT( 0.5, 0.85 ), ATTN_NORM );	break;
		case 1:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "toad/toad_hunt2.wav", RANDOM_FLOAT( 0.5, 0.85 ), ATTN_NORM );	break;
		default: EMIT_SOUND( ENT(pev), CHAN_VOICE, "toad/toad_hunt3.wav", RANDOM_FLOAT( 0.5, 0.85 ), ATTN_NORM );	break;
		}
		m_flNextIdleSound = gpGlobals->time + RANDOM_FLOAT( 3, 4 );
	}
}


// POV: the fidget animations' croaks, played by think
void CToad::StartVModelSound( int iSound )
{
	m_iSoundPlayed = iSound;

	switch ( iSound )
	{
	case 0:
		pev->nextthink = gpGlobals->time + 0.2;
		break;
	case 1:
		pev->nextthink = gpGlobals->time + 0.95;
		break;
	default:
		ALERT( at_console, "StartVModelSound: no %d sound to play!!\n", iSound );
		m_iSoundPlayed = -1;
		break;
	}

	if ( m_iSoundPlayed != -1 )
		SetThink( PlayVModelSound );
}

void CToad::PlayVModelSound( void )
{
	EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_VOICE, pVModelSounds[m_iSoundPlayed], 0.3, ATTN_IDLE, 0, PITCH_NORM );
	SetThink( NULL );
	pev->nextthink = gpGlobals->time;
}

void CToad::StopVModelSound( void )
{
	SetThink( NULL );
	pev->nextthink = gpGlobals->time;

	if ( m_iSoundPlayed != -1 )
		EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_VOICE, pVModelSounds[m_iSoundPlayed], 0.3, ATTN_IDLE, SND_STOP, PITCH_NORM );
}

#endif
