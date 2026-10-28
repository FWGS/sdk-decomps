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
// rat - environmental monster
// T2DEF: the mutant rat, a biting monster with glowing eyes
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"effects.h"
#include	"soundent.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	RAT_AE_BITE		0x01 // name ours

class CRat : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void GibMonster( void );
	void PrescheduleThink( void );
	void RunTask( Task_t *pTask );

	void EyeOff( void ); // names ours
	void EyeUpdate( void );
	void EXPORT FadeEyesThink( void );

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void DeathSound( void );

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	static const char *pBiteSounds[];
	static const char *pDeathSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];

	CSprite *m_pLeftEyeGlow;
	CSprite *m_pRightEyeGlow;
	int m_eyeBrightness;
	int m_saveBrightness;
};
LINK_ENTITY_TO_CLASS( monster_rat, CRat );

const char *CRat::pBiteSounds[] =
{
	"rat/ratbite1.wav",
	"rat/ratbite2.wav",
	"rat/ratbite3.wav",
};

const char *CRat::pDeathSounds[] =
{
	"rat/ratdie1.wav",
	"rat/ratdie2.wav",
	"rat/ratdie3.wav",
	"rat/ratdie4.wav",
	"rat/ratdie5.wav",
};

const char *CRat::pIdleSounds[] =
{
	"rat/ratidle1.wav",
	"rat/ratidle2.wav",
	"rat/ratidle3.wav",
	"rat/ratidle4.wav",
};

const char *CRat::pAlertSounds[] =
{
	"rat/ratalert1.wav",
	"rat/ratalert2.wav",
	"rat/ratalert3.wav",
	"rat/ratalert4.wav",
};

const char *CRat::pPainSounds[] =
{
	"rat/ratpain1.wav",
	"rat/ratpain2.wav",
};

TYPEDESCRIPTION	CRat::m_SaveData[] =
{
	DEFINE_FIELD( CRat, m_pLeftEyeGlow, FIELD_CLASSPTR ),
	DEFINE_FIELD( CRat, m_pRightEyeGlow, FIELD_CLASSPTR ),
	DEFINE_FIELD( CRat, m_eyeBrightness, FIELD_INTEGER ),
	DEFINE_FIELD( CRat, m_saveBrightness, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CRat, CBaseMonster );

//=========================================================
// FadeEyesThink - the dead rat's eyes go out
//=========================================================
void CRat :: FadeEyesThink( void )
{
	if ( m_pLeftEyeGlow && m_pRightEyeGlow )
	{
		float flBrightness = m_pLeftEyeGlow->pev->renderamt;
		flBrightness -= 2;

		if ( flBrightness <= 0.0 )
		{
			UTIL_Remove( m_pLeftEyeGlow );
			m_pLeftEyeGlow = NULL;
			UTIL_Remove( m_pRightEyeGlow );
			m_pRightEyeGlow = NULL;
			SetThink( NULL );
		}
		else
		{
			m_pLeftEyeGlow->pev->renderamt = flBrightness;
			m_pRightEyeGlow->pev->renderamt = flBrightness;
		}
	}

	SetNextThink( 0.1 );
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CRat :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CRat :: SetYawSpeed ( void )
{
	pev->yaw_speed = 140;
}

//=========================================================
// Spawn
//=========================================================
void CRat :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/mutantrat.mdl");
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 24 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->health			= gSkillData.ratHealth;
	pev->view_ofs		= Vector ( 0, 0, 6 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE;
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();

	m_pLeftEyeGlow = CSprite::SpriteCreate( "sprites/gargeye1.spr", pev->origin, FALSE );
	m_pLeftEyeGlow->SetTransparency( kRenderGlow, 255, 255, 255, 0, kRenderFxNoDissipation );
	m_pLeftEyeGlow->SetAttachment( edict(), 1 );
	m_pLeftEyeGlow->SetScale( 0.2 );

	m_pRightEyeGlow = CSprite::SpriteCreate( "sprites/gargeye1.spr", pev->origin, FALSE );
	m_pRightEyeGlow->SetTransparency( kRenderGlow, 255, 255, 255, 0, kRenderFxNoDissipation );
	m_pRightEyeGlow->SetAttachment( edict(), 2 );
	m_pRightEyeGlow->SetScale( 0.2 );

	m_eyeBrightness = 200;
	m_saveBrightness = 200;
	EyeOff();

	SetNextThink( 1 );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CRat :: Precache()
{
	int i;

	PRECACHE_MODEL("models/mutantrat.mdl");
	PRECACHE_MODEL("sprites/gargeye1.spr");

	PRECACHE_SOUND("rat/ratstep1.wav");
	PRECACHE_SOUND("rat/ratstep2.wav");

	for ( i = 0; i < ARRAYSIZE( pBiteSounds ); i++ )
		PRECACHE_SOUND((char *)pBiteSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pDeathSounds ); i++ )
		PRECACHE_SOUND((char *)pDeathSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CRat :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case RAT_AE_BITE:
		{
			CBaseEntity *pHurt;
			TraceResult tr;

			UTIL_MakeAimVectors( pev->angles );

			Vector vecStart = pev->origin;
			vecStart.z += 32;
			Vector vecEnd = vecStart + (gpGlobals->v_forward * 64 );

			UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );

			if ( tr.pHit )
			{
				pHurt = CBaseEntity::Instance( tr.pHit );

				if ( pHurt )
				{
					pHurt->TakeDamage( pev, pev, gSkillData.leechDmgBite, DMG_SLASH );

					if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
						pHurt->pev->punchangle.z = -5;
						pHurt->pev->punchangle.x = 5;
					}

					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pBiteSounds[ RANDOM_LONG(0,ARRAYSIZE(pBiteSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// GibMonster - the eyes go with the rat
//=========================================================
void CRat :: GibMonster( void )
{
	if ( m_pLeftEyeGlow && m_pRightEyeGlow )
	{
		UTIL_Remove( m_pLeftEyeGlow );
		m_pLeftEyeGlow = NULL;
		UTIL_Remove( m_pRightEyeGlow );
		m_pRightEyeGlow = NULL;
	}

	CBaseMonster::GibMonster();
}

//=========================================================
// PrescheduleThink - squeak in combat, blink, and let the
// eyes glow brighter in the dark
//=========================================================
void CRat :: PrescheduleThink( void )
{
	if ( m_MonsterState == MONSTERSTATE_COMBAT && RANDOM_FLOAT( 0, 7 ) < 0.12 )
	{
		AlertSound();
	}

	if ( m_eyeBrightness && m_MonsterState != MONSTERSTATE_DEAD && RANDOM_FLOAT( 0, 8 ) < 0.15 )
	{
		EyeOff();
	}
	else
	{
		m_eyeBrightness = m_saveBrightness;

		if ( Illumination() < 32 )
			m_eyeBrightness += 12;
		else
			m_eyeBrightness -= 12;

		if ( m_eyeBrightness < 0 )
			m_eyeBrightness = 0;
		if ( m_eyeBrightness > 200 )
			m_eyeBrightness = 200;

		m_saveBrightness = m_eyeBrightness;
	}

	EyeUpdate();
}

void CRat :: EyeOff( void )
{
	m_saveBrightness = m_eyeBrightness;
	m_eyeBrightness = 0;
}

void CRat :: EyeUpdate( void )
{
	if ( m_pLeftEyeGlow )
	{
		m_pLeftEyeGlow->pev->renderamt = m_eyeBrightness;
		if ( m_pLeftEyeGlow->pev->renderamt == 0 )
			m_pLeftEyeGlow->pev->effects |= EF_NODRAW;
		else
			m_pLeftEyeGlow->pev->effects &= ~EF_NODRAW;

		Vector vecOrigin, vecAngles;
		GetAttachment( 0, vecOrigin, vecAngles );
		UTIL_SetOrigin( m_pLeftEyeGlow, vecOrigin );
	}

	if ( m_pRightEyeGlow )
	{
		m_pRightEyeGlow->pev->renderamt = m_eyeBrightness;
		if ( m_pRightEyeGlow->pev->renderamt == 0 )
			m_pRightEyeGlow->pev->effects |= EF_NODRAW;
		else
			m_pRightEyeGlow->pev->effects &= ~EF_NODRAW;

		Vector vecOrigin, vecAngles;
		GetAttachment( 1, vecOrigin, vecAngles );
		UTIL_SetOrigin( m_pRightEyeGlow, vecOrigin );
	}
}

//=========================================================
// RunTask - schedule.cpp's TASK_DIE, then fade the eyes
//=========================================================
void CRat :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_DIE:
		{
			if ( m_fSequenceFinished && pev->frame >= 255 )
			{
				pev->deadflag = DEAD_DEAD;

				SetThink ( FadeEyesThink );
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

void CRat :: PainSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	if (RANDOM_LONG(0,5) < 4)
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CRat :: AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CRat :: IdleSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_IDLE, 0, pitch );
}

void CRat :: DeathSound( void )
{
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pDeathSounds[ RANDOM_LONG(0,ARRAYSIZE(pDeathSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
