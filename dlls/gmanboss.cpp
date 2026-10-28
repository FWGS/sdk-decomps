/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// gmanboss.cpp - T2DEF's final boss, the G-Man turned
// monster, after zombie.cpp (file name ours)
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"soundent.h"


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	GMANBOSS_AE_ATTACK_RIGHT		0x01
#define	GMANBOSS_AE_ATTACK_LEFT			0x02
#define	GMANBOSS_AE_ATTACK_BOTH			0x03

#define GMANBOSS_FLINCH_DELAY			2		// at most one flinch every n secs

class CGmanBoss : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int IgnoreConditions ( void );
	void RunAI( void );
	void RunTask( Task_t *pTask );
	CBaseEntity *CheckTraceHullAttack( float flDist, int iDamage, int iDmgType );
	void EXPORT BloodVomitThink( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	float m_flNextFlinch;
	bool m_lostArm;
	float m_flDeathTime;

	void PainSound( void );
	void AlertSound( void );
	void AttackSound( void );
	void DeathSound( void );

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
	static const char *pAttackSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack2 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
};

LINK_ENTITY_TO_CLASS( monster_gman_boss, CGmanBoss );

TYPEDESCRIPTION	CGmanBoss::m_SaveData[] =
{
	DEFINE_FIELD( CGmanBoss, m_lostArm, FIELD_BOOLEAN ),
	DEFINE_FIELD( CGmanBoss, m_flDeathTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CGmanBoss, CBaseMonster );

const char *CGmanBoss::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CGmanBoss::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CGmanBoss::pAttackSounds[] =
{
	"boss/gb_attack1.wav",
	"boss/gb_attack2.wav",
	"boss/gb_attack3.wav",
};

const char *CGmanBoss::pPainSounds[] =
{
	"boss/gb_pain1.wav",
	"boss/gb_pain2.wav",
	"boss/gb_pain3.wav",
};

const char *CGmanBoss::pDeathSounds[] =
{
	"boss/gb_die1.wav",
	"boss/gb_die2.wav",
	"boss/gb_die3.wav",
};

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CGmanBoss :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGmanBoss :: SetYawSpeed ( void )
{
	int ys;

	ys = 120;

#if 0
	switch ( m_Activity )
	{
	}
#endif

	pev->yaw_speed = ys;
}

int CGmanBoss :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Take 60% damage from bullets
	if ( bitsDamageType & DMG_BULLET )
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce( flDamage );
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.6;
	}

	// HACK HACK -- until we fix this.
	if ( IsAlive() )
	{
		PainSound();

		// lose the right arm at half health
		if ( pev->health < gSkillData.gmanbossHealth * 0.5 && !m_lostArm )
		{
			CGib::SpawnRandomGibs( pev, 1, 0, "models/gmanarmgib.mdl" );
			m_lostArm = TRUE;
			SetBodygroup( 1, 1 );
		}
	}
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CGmanBoss :: PainSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CGmanBoss :: AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CGmanBoss :: AttackSound( void )
{
	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

void CGmanBoss :: DeathSound( void )
{
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pDeathSounds[ RANDOM_LONG(0,ARRAYSIZE(pDeathSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGmanBoss :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case GMANBOSS_AE_ATTACK_RIGHT:
		{
			// no right arm, no right slash
			if ( m_lostArm )
			{
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				break;
			}

			// do stuff for this event.
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombieDmgOneSlash, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
				}
				// Play a random attack hit sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else // Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
				AttackSound();
		}
		break;

		case GMANBOSS_AE_ATTACK_LEFT:
		{
			// do stuff for this event.
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombieDmgOneSlash, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = 18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
				}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
				AttackSound();
		}
		break;

		case GMANBOSS_AE_ATTACK_BOTH:
		{
			// one arm hits like one slash
			int iDamage = gSkillData.zombieDmgBothSlash;
			if ( m_lostArm )
				iDamage = gSkillData.zombieDmgOneSlash;

			// do stuff for this event.
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, iDamage, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
				}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
				AttackSound();
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CGmanBoss :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/gmanboss.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= gSkillData.gmanbossHealth;
	pev->view_ofs		= Vector ( 0, 0, 96 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	m_lostArm			= FALSE;
	m_flDeathTime		= 0;

	SetBodygroup( 1, 0 );

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGmanBoss :: Precache()
{
	int i;

	PRECACHE_MODEL("models/gmanboss.mdl");
	PRECACHE_MODEL("models/gmanarmgib.mdl");

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pDeathSounds ); i++ )
		PRECACHE_SOUND((char *)pDeathSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CGmanBoss::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK1))
	{
#if 0
		if (pev->health < 20)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
		else
#endif
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + GMANBOSS_FLINCH_DELAY;
	}

	return iIgnore;

}

//=========================================================
// RunAI - the stump of the lost arm bleeds
//=========================================================
void CGmanBoss :: RunAI( void )
{
	CBaseMonster::RunAI();

	if ( m_lostArm )
	{
		if ( RANDOM_LONG( 0, 9 ) < 2 )
		{
			Vector vecBlood, vecAng; // names ours

			GetAttachment( 0, vecBlood, vecAng );
			UTIL_BloodStream( vecBlood, Vector( 0, 0, -1 ), 70, 25 );

			TraceResult tr;
			UTIL_TraceLine( vecBlood, vecBlood + Vector( 0, 0, -256 ), ignore_monsters, NULL, &tr );
			if ( tr.flFraction != 1.0 )
				UTIL_BloodDecalTrace( &tr, BLOOD_COLOR_RED );
		}
	}
}

//=========================================================
// CheckTraceHullAttack - CBaseMonster's, but the boss
// doesn't hit himself and hits Joe softer
//=========================================================
CBaseEntity* CGmanBoss :: CheckTraceHullAttack( float flDist, int iDamage, int iDmgType )
{
	TraceResult tr;

	if (IsPlayer())
		UTIL_MakeVectors( pev->angles );
	else
		UTIL_MakeAimVectors( pev->angles );

	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * flDist );

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );

	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		if ( FClassnameIs( pEntity->pev, "monster_gman_boss" ) )
			return NULL;

		if ( FClassnameIs( pEntity->pev, "monster_joe" ) )
			iDamage = iDamage * 0.7;

		if ( iDamage > 0 )
		{
			pEntity->TakeDamage( pev, pev, iDamage, iDmgType );
		}

		return pEntity;
	}

	return NULL;
}

//=========================================================
// RunTask - schedule.cpp's TASK_DIE, then vomit blood
//=========================================================
void CGmanBoss :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_DIE:
		{
			if ( m_fSequenceFinished && pev->frame >= 255 )
			{
				pev->deadflag = DEAD_DEAD;
				m_flDeathTime = gpGlobals->time;

				SetThink ( BloodVomitThink );
				StopAnimation();

				if ( !BBoxFlat() )
				{
					// a bit of a hack. If a corpses' bbox is positioned such that being left solid so that it can be attacked will
					// block the player on a slope or stairs, the corpse is made nonsolid.
//					pev->solid = SOLID_NOT;
					UTIL_SetSize ( pev, Vector ( -4, -4, 0 ), Vector ( 4, 4, 1 ) );
				}
				else // !!!HACKHACK - put monster in a thin, wide bounding box until we fix the solid type/bounding volume problem
					UTIL_SetSize ( pev, Vector ( pev->mins.x, pev->mins.y, pev->mins.z ), Vector ( pev->maxs.x, pev->maxs.y, pev->mins.z + 1 ) );

				if ( ShouldFadeOnDeath() )
				{
					// this monster was created by a monstermaker... fade the corpse out.
					SUB_StartFadeOut();
				}
				else
				{
					// body is gonna be around for a while, so have it stink for a bit.
					CSoundEnt::InsertSound ( bits_SOUND_CARCASS, pev->origin, 384, 30 );
				}
			}
			break;
		}
	default:
		{
			CBaseMonster :: RunTask(pTask);
			break;
		}
	}
}

//=========================================================
// BloodVomitThink - the dead boss bleeds from the mouth for
// five seconds
//=========================================================
void CGmanBoss :: BloodVomitThink( void )
{
	if ( m_flDeathTime + 5 < gpGlobals->time )
	{
		SetThink( NULL );
	}
	else if ( RANDOM_LONG( 0, 10 ) < 3 )
	{
		Vector vecOrigin, angles; // names ours

		GetAttachment( 1, vecOrigin, angles );

		float x = RANDOM_FLOAT( -0.5, 0.5 );
		float y = sqrt( 0.25 - x * x );
		float z = sin( M_PI / 3 );

		UTIL_BloodStream( vecOrigin, Vector( x, y, z ), 70, RANDOM_LONG( 100, 120 ) );
	}

	SetNextThink( 0.1 );
}

BOOL CGmanBoss :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	if ( flDist <= 96 && flDot >= 0.7 && m_hEnemy != NULL && FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) )
	{
		return TRUE;
	}
	return FALSE;
}

BOOL CGmanBoss :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	if ( flDist <= 96 && flDot >= 0.7 )
	{
		return TRUE;
	}
	return FALSE;
}
