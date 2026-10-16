/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
// POV: alien panther, made from the alien slave (islave.cpp)
// without the squad. File name and the names retail doesn't
// give are ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"squadmonster.h"
#include	"schedule.h"
#include	"effects.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"decals.h"

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		PANTHER_AE_CLAW		( 1 )
#define		PANTHER_AE_CLAWRAKE	( 2 )
#define		PANTHER_AE_ZAP_POWERUP	( 3 )
#define		PANTHER_AE_ZAP_SHOOT		( 4 )
#define		PANTHER_AE_ZAP_DONE		( 5 )

#define		PANTHER_MAX_BEAMS	8

class CPanther : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int	 ISoundMask( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin - Vector( 80, 80, 0 );
		pev->absmax = pev->origin + Vector( 80, 80, 96 );
	}

	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }

	void DeathSound( void );
	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );

	void Killed( entvars_t *pevAttacker, int iGib );

    void StartTask ( Task_t *pTask );
	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType ( int Type );
	CUSTOM_SCHEDULES;

	int	Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	void ClearBeams( );
	void ArmBeam( int side );
	void WackBeam( int side, CBaseEntity *pEntity );
	void ZapBeam( int side );
	void BeamGlow( void );

	CBeam *m_pBeam[PANTHER_MAX_BEAMS];

	int m_iBeams;
	float m_flNextAttack;

	int	m_voicePitch;

	EHANDLE m_hDead;

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];
};
LINK_ENTITY_TO_CLASS( monster_alien_panther, CPanther );


TYPEDESCRIPTION	CPanther::m_SaveData[] =
{
	DEFINE_ARRAY( CPanther, m_pBeam, FIELD_CLASSPTR, PANTHER_MAX_BEAMS ),
	DEFINE_FIELD( CPanther, m_iBeams, FIELD_INTEGER ),
	DEFINE_FIELD( CPanther, m_flNextAttack, FIELD_TIME ),

	DEFINE_FIELD( CPanther, m_voicePitch, FIELD_INTEGER ),

	DEFINE_FIELD( CPanther, m_hDead, FIELD_EHANDLE ),

};

IMPLEMENT_SAVERESTORE( CPanther, CBaseMonster );




const char *CPanther::pIdleSounds[] =
{
	"panther/p_idle1.wav",
	"panther/p_idle2.wav",
	"panther/p_idle3.wav",
};

const char *CPanther::pAlertSounds[] =
{
	"panther/p_alert1.wav",
	"panther/p_alert2.wav",
	"panther/p_alert3.wav",
};

const char *CPanther::pAttackHitSounds[] =
{
	"panther/pclaw_strike1.wav",
	"panther/pclaw_strike2.wav",
	"panther/pclaw_strike3.wav",
};

const char *CPanther::pAttackMissSounds[] =
{
	"panther/pclaw_miss1.wav",
	"panther/pclaw_miss2.wav",
};

const char *CPanther::pPainSounds[] =
{
	"panther/p_pain1.wav",
	"panther/p_pain2.wav",
};

const char *CPanther::pDeathSounds[] =
{
	"panther/p_die1.wav",
	"panther/p_die2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CPanther :: Classify ( void )
{
	return	CLASS_ALIEN_MILITARY;
}


//=========================================================
// ALertSound - scream
//=========================================================
void CPanther :: AlertSound( void )
{
	if ( m_hEnemy != NULL )
	{
		EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
	}
}

//=========================================================
// IdleSound
//=========================================================
void CPanther :: IdleSound( void )
{
	if (RANDOM_LONG( 0, 2 ) == 0)
	{
		EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
	}

	int side = RANDOM_LONG( 0, 1 ) * 2 - 1;

	ClearBeams( );
	ArmBeam( side );

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + gpGlobals->v_right * 2 * side;
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(vecSrc.x);	// X
		WRITE_COORD(vecSrc.y);	// Y
		WRITE_COORD(vecSrc.z);	// Z
		WRITE_BYTE( 8 );		// radius * 0.1
		WRITE_BYTE( 192 );		// r
		WRITE_BYTE( 64 );		// g
		WRITE_BYTE( 8 );		// b
		WRITE_BYTE( 10 );		// time * 10
		WRITE_BYTE( 0 );		// decay * 0.1
	MESSAGE_END( );

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "panther/p_zap1.wav", 1, ATTN_NORM, 0, 100 );
}

//=========================================================
// PainSound
//=========================================================
void CPanther :: PainSound( void )
{
	if (RANDOM_LONG( 0, 2 ) == 0)
	{
		EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
	}
}

//=========================================================
// DieSound
//=========================================================

void CPanther :: DeathSound( void )
{
	EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pDeathSounds[ RANDOM_LONG(0,ARRAYSIZE(pDeathSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards.
//=========================================================
int CPanther :: ISoundMask ( void)
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}


void CPanther::Killed( entvars_t *pevAttacker, int iGib )
{
	ClearBeams( );
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CPanther :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_WALK:
		ys = 50;
		break;
	case ACT_RUN:
		ys = 70;
		break;
	case ACT_IDLE:
		ys = 50;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CPanther :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	// ALERT( at_console, "event %d : %f\n", pEvent->event, pev->frame );
	switch( pEvent->event )
	{
		case PANTHER_AE_CLAW:
		{
			// SOUND HERE!
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.pantherDmgClaw, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
				}
				// Play a random attack hit sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
			}
			else
			{
				// Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
			}
		}
		break;

		case PANTHER_AE_CLAWRAKE:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.pantherDmgClawrake, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
				}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
			}
			else
			{
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
			}
		}
		break;

		case PANTHER_AE_ZAP_POWERUP:
		{
			UTIL_MakeAimVectors( pev->angles );

			if (m_iBeams == 0)
			{
				Vector vecSrc = pev->origin + gpGlobals->v_forward * 2;
				MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
					WRITE_BYTE(TE_DLIGHT);
					WRITE_COORD(vecSrc.x);	// X
					WRITE_COORD(vecSrc.y);	// Y
					WRITE_COORD(vecSrc.z);	// Z
					WRITE_BYTE( 12 );		// radius * 0.1
					WRITE_BYTE( 192 );		// r
					WRITE_BYTE( 64 );		// g
					WRITE_BYTE( 8 );		// b
					WRITE_BYTE( 20 / pev->framerate );		// time * 10
					WRITE_BYTE( 0 );		// decay * 0.1
				MESSAGE_END( );

			}
			if (m_hDead != NULL)
			{
				WackBeam( -1, m_hDead );
				WackBeam( 1, m_hDead );
			}
			else
			{
				ArmBeam( -1 );
				ArmBeam( 1 );
				BeamGlow( );
			}

			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "panther/p_zap2.wav", 1, ATTN_NORM, 0, 100 + m_iBeams * 10 );
			pev->skin = m_iBeams / 2;
		}
		break;

		case PANTHER_AE_ZAP_SHOOT:
		{
			ClearBeams( );

			if (m_hDead != NULL)
			{
				Vector vecDest = m_hDead->pev->origin + Vector( 0, 0, 38 );
				TraceResult trace;
				UTIL_TraceHull( vecDest, vecDest, dont_ignore_monsters, human_hull, m_hDead->edict(), &trace );

				if ( !trace.fStartSolid )
				{
					CBaseEntity *pNew = Create( "monster_panther", m_hDead->pev->origin, m_hDead->pev->angles );
					CBaseMonster *pNewMonster = pNew->MyMonsterPointer( );
					pNew->pev->spawnflags |= 1;
					WackBeam( -1, pNew );
					WackBeam( 1, pNew );
					UTIL_Remove( m_hDead );
					EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "panther/p_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 130, 160 ) );

					/*
					CBaseEntity *pEffect = Create( "test_effect", pNew->Center(), pev->angles );
					pEffect->Use( this, this, USE_ON, 1 );
					*/
					break;
				}
			}
			ClearMultiDamage();

			UTIL_MakeAimVectors( pev->angles );

			ZapBeam( -1 );
			ZapBeam( 1 );

			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "panther/p_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 130, 160 ) );
			// STOP_SOUND( ENT(pev), CHAN_WEAPON, "debris/zap4.wav" );
			ApplyMultiDamage(pev, pev);

			m_flNextAttack = gpGlobals->time + RANDOM_FLOAT( 0.5, 4.0 );
		}
		break;

		case PANTHER_AE_ZAP_DONE:
		{
			ClearBeams( );
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// CheckMeleeAttack1 - the alien grunt's
//=========================================================
BOOL CPanther :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( HasConditions ( bits_COND_SEE_ENEMY ) && flDist <= 100 && flDot >= 0.7 && m_hEnemy != NULL )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 - normal beam attack
//=========================================================
BOOL CPanther :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if (m_flNextAttack > gpGlobals->time)
	{
		return FALSE;
	}

	return CBaseMonster::CheckRangeAttack1( flDot, flDist );
}


//=========================================================
// StartTask
//=========================================================
void CPanther :: StartTask ( Task_t *pTask )
{
	ClearBeams( );

	CBaseMonster :: StartTask ( pTask );
}


//=========================================================
// Spawn
//=========================================================
void CPanther :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/panther.mdl");
	UTIL_SetSize(pev, Vector( -56, -56, 0 ), Vector( 56, 56, 72 ));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;
	pev->health			= gSkillData.pantherHealth;
	pev->view_ofs		= Vector ( 0, 0, 80 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_RANGE_ATTACK2 | bits_CAP_DOORS_GROUP;

	m_voicePitch		= RANDOM_LONG( 85, 110 );

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CPanther :: Precache()
{
	int i;

	PRECACHE_MODEL("models/panther.mdl");
	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_SOUND("panther/p_zap1.wav");
	PRECACHE_SOUND("panther/p_zap2.wav");
	PRECACHE_SOUND("panther/p_electro1.wav");
	PRECACHE_SOUND("panther/p_shoot1.wav");
	PRECACHE_SOUND("panther/p_pain2.wav");
	PRECACHE_SOUND("panther/p_headbite.wav");
	PRECACHE_SOUND("panther/pclaw_miss1.wav");

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pDeathSounds ); i++ )
		PRECACHE_SOUND((char *)pDeathSounds[i]);

	UTIL_PrecacheOther( "test_effect" );
}


//=========================================================
// TakeDamage - get provoked when injured
//=========================================================

int CPanther :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// don't slash one of your own
	if (!UTIL_StrIEq(STRING(pevAttacker->classname), "player") && (bitsDamageType & DMG_SLASH) && pevAttacker && IRelationship( Instance(pevAttacker) ) < R_DL)
		return 0;

	// answer the player's shots
	if ( m_MonsterState != MONSTERSTATE_PRONE && !(m_afMemory & bits_MEMORY_PROVOKED) )
	{
		switch ( UTIL_PlayerProvoke( this, pevAttacker ) )
		{
		case 1:
			PainSound( );
			break;
		case 2:
			PainSound( );
			Remember( bits_MEMORY_SUSPICIOUS );
			break;
		case 3:
			AlertSound( );
			Remember( bits_MEMORY_PROVOKED );
			break;
		}
	}

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}


void CPanther::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	// shocks hurt only from trigger_hurt and the player, the player's half
	if (bitsDamageType & DMG_SHOCK)
	{
		if ( !UTIL_StrIEq( STRING(pevAttacker->classname), "trigger_hurt" ) && !UTIL_StrIEq( STRING(pevAttacker->classname), "player" ) )
		{
			UTIL_Ricochet( ptr->vecEndPos, 2.0 );
			return;
		}

		if ( UTIL_StrIEq( STRING(pevAttacker->classname), "player" ) )
			flDamage /= 2;
	}

	// bullets and clubs ricochet off all but the head
	if ( !(bitsDamageType & DMG_SLASH) && ptr->iHitgroup != HITGROUP_HEAD && (bitsDamageType & (DMG_BULLET | DMG_CLUB)) )
	{
		UTIL_Ricochet( ptr->vecEndPos, 1.0 );
		flDamage = 0.01;
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// AI Schedules Specific to this monster
//=========================================================



// primary range attack
Task_t	tlPantherAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slPantherAttack1[] =
{
	{
		tlPantherAttack1,
		ARRAYSIZE ( tlPantherAttack1 ),
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAR_SOUND |
		bits_COND_HEAVY_DAMAGE,

		bits_SOUND_DANGER,
		"Panther Range Attack1"
	},
};


DEFINE_CUSTOM_SCHEDULES( CPanther )
{
	slPantherAttack1,
};

IMPLEMENT_CUSTOM_SCHEDULES( CPanther, CBaseMonster );


//=========================================================
//=========================================================
Schedule_t *CPanther :: GetSchedule( void )
{
	ClearBeams( );

	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );

		if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}

	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
// dead enemy
		if ( HasConditions( bits_COND_ENEMY_DEAD ) )
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster :: GetSchedule();
		}
		break;
	}
	return CBaseMonster::GetSchedule( );
}


Schedule_t *CPanther :: GetScheduleOfType ( int Type )
{
	switch	( Type )
	{
	case SCHED_FAIL:
		if (HasConditions( bits_COND_CAN_MELEE_ATTACK1 ))
		{
			return CBaseMonster :: GetScheduleOfType( SCHED_MELEE_ATTACK1 ); ;
		}
		break;
	case SCHED_RANGE_ATTACK1:
		return slPantherAttack1;
	case SCHED_RANGE_ATTACK2:
		return slPantherAttack1;
	}
	return CBaseMonster :: GetScheduleOfType( Type );
}


//=========================================================
// ArmBeam - small beam from arm to nearby geometry
//=========================================================

void CPanther :: ArmBeam( int side )
{
	TraceResult tr;
	float flDist = 1.0;

	if (m_iBeams >= PANTHER_MAX_BEAMS)
		return;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;

	for (int i = 0; i < 3; i++)
	{
		Vector vecAim = gpGlobals->v_right * side * RANDOM_FLOAT( 0, 1 ) + gpGlobals->v_up * RANDOM_FLOAT( -1, 1 );
		TraceResult tr1;
		UTIL_TraceLine ( vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT( pev ), &tr1);
		if (flDist > tr1.flFraction)
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if ( flDist == 1.0 )
		return;

	DecalGunshot( &tr, BULLET_PLAYER_CROWBAR );

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex( ) );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	m_pBeam[m_iBeams]->SetColor( 192, 64, 8 );
	m_pBeam[m_iBeams]->SetBrightness( 64 );
	m_pBeam[m_iBeams]->SetNoise( 80 );
	m_iBeams++;
}


//=========================================================
// BeamGlow - brighten all beams
//=========================================================
void CPanther :: BeamGlow( )
{
	int b = m_iBeams * 32;
	if (b > 255)
		b = 255;

	for (int i = 0; i < m_iBeams; i++)
	{
		if (m_pBeam[i]->GetBrightness() != 255)
		{
			m_pBeam[i]->SetBrightness( b );
		}
	}
}


//=========================================================
// WackBeam - regenerate dead colleagues
//=========================================================
void CPanther :: WackBeam( int side, CBaseEntity *pEntity )
{
	Vector vecDest;
	float flDist = 1.0;

	if (m_iBeams >= PANTHER_MAX_BEAMS)
		return;

	if (pEntity == NULL)
		return;

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( pEntity->Center(), entindex( ) );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	m_pBeam[m_iBeams]->SetColor( 192, 64, 8 );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 80 );
	m_iBeams++;
}

//=========================================================
// ZapBeam - heavy damage directly forward
//=========================================================
void CPanther :: ZapBeam( int side )
{
	Vector vecSrc, vecAim;
	TraceResult tr;
	CBaseEntity *pEntity;

	if (m_iBeams >= PANTHER_MAX_BEAMS)
		return;

	vecSrc = pev->origin + gpGlobals->v_up * 36;
	vecAim = ShootAtEnemy( vecSrc );
	float deflection = 0.01;
	vecAim = vecAim + side * gpGlobals->v_right * RANDOM_FLOAT( 0, deflection ) + gpGlobals->v_up * RANDOM_FLOAT( -deflection, deflection );
	UTIL_TraceLine ( vecSrc, vecSrc + vecAim * 1024, dont_ignore_monsters, ENT( pev ), &tr);

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 50 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex( ) );
	m_pBeam[m_iBeams]->SetEndAttachment( 3 );
	m_pBeam[m_iBeams]->SetColor( 192, 64, 8 );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 20 );
	m_iBeams++;

	pEntity = CBaseEntity::Instance(tr.pHit);
	if (pEntity != NULL)
	{
		if (pEntity->pev->takedamage)
		{
			if ( pEntity->IsPlayer() )
				pEntity->TraceAttack( pev, gSkillData.pantherDmgZap / 2, vecAim, &tr, DMG_ENERGYBEAM | DMG_NEVERGIB );
			else
				pEntity->TraceAttack( pev, gSkillData.pantherDmgZap / 2, vecAim, &tr, DMG_SHOCK | DMG_NEVERGIB );
		}
		else if (pEntity->Classify() == CLASS_NONE)
		{
			UTIL_DecalTrace( &tr, RANDOM_LONG( DECAL_SMALLSCORCH1, DECAL_SMALLSCORCH3 ) );
		}
	}
	UTIL_EmitAmbientSound( ENT(pev), tr.vecEndPos, "panther/p_electro1.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );
}


//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CPanther :: ClearBeams( )
{
	for (int i = 0; i < PANTHER_MAX_BEAMS; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
	m_iBeams = 0;
	pev->skin = 0;

	STOP_SOUND( ENT(pev), CHAN_WEAPON, "panther/p_zap2.wav" );
}
