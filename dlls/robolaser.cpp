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
// T2DEF: monster_robo_laser, the M450 mining robot with a
// beam laser. Built from the apache's gun and death, the
// egon's beams and the gargantua's eye glow.
// (file name, function names and #define names ours)
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "effects.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define ROBO_AE_STEP		2
#define ROBO_AE_STOMP		3

class CRoboLaser : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void RunAI( void );
	int  ISoundMask( void );
	void AlertSound( void );
	void DeathSound( void );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Killed( entvars_t *pevAttacker, int iGib );
	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );
	Schedule_t *GetScheduleOfType( int Type );
	void MonsterInit( void );

	BOOL CheckMeleeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack2 ( float flDot, float flDist ) { return FALSE; }

	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin + Vector( -256, -256, 0 );
		pev->absmax = pev->origin + Vector( 256, 256, 256 );
	}

	void FireLaser( void );
	void UpdateEffect( const Vector &startPoint, const Vector &endPoint, float timeBlend );
	void CreateEffect( void );
	void DestroyEffect( void );
	void EndAttack( void );
	void DeathExplosion( void );
	void ShakeGun( void );
	void LaserGlowOff( void );
	void LaserGlowUpdate( void );
	void PlayFireSound( int iSound );

	int	Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES;

	Vector m_angGun;
	float m_flOutOfRangeTime;
	float m_flInRangeTime;
	CBeam *m_pBeam;
	CBeam *m_pNoise;

	int m_iSpriteTexture;
	int m_iExplode;
	int m_iBodyGibs;

	float m_flGoalSpeed; // never used

	int m_iStartRunSound;
	float m_flAwakeTime;
	int m_iLastFireSoundPlayed;

	CSprite *m_pLaserGlow;
	float m_LaserGlowBrightness;
};
LINK_ENTITY_TO_CLASS( monster_robo_laser, CRoboLaser );

TYPEDESCRIPTION	CRoboLaser::m_SaveData[] =
{
	DEFINE_FIELD( CRoboLaser, m_angGun, FIELD_VECTOR ),
	DEFINE_FIELD( CRoboLaser, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CRoboLaser, m_pNoise, FIELD_CLASSPTR ),
	DEFINE_FIELD( CRoboLaser, m_flOutOfRangeTime, FIELD_TIME ),
	DEFINE_FIELD( CRoboLaser, m_flInRangeTime, FIELD_TIME ),
	DEFINE_FIELD( CRoboLaser, m_pLaserGlow, FIELD_CLASSPTR ),
	DEFINE_FIELD( CRoboLaser, m_LaserGlowBrightness, FIELD_FLOAT ),
	DEFINE_FIELD( CRoboLaser, m_iStartRunSound, FIELD_INTEGER ),
	DEFINE_FIELD( CRoboLaser, m_flAwakeTime, FIELD_TIME ),
	DEFINE_FIELD( CRoboLaser, m_iLastFireSoundPlayed, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CRoboLaser, CBaseMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Task_t	tlRoboWakeAngry[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
	{ TASK_SOUND_WAKE,			(float)0		},
	{ TASK_WAIT,				(float)2		},
	{ TASK_FACE_IDEAL,			(float)0		},
};

Schedule_t	slRoboWakeAngry[] =
{
	{
		tlRoboWakeAngry,
		ARRAYSIZE ( tlRoboWakeAngry ),
		0,
		0,
		"RoboLaser Wake Angry"
	}
};

Task_t	tlRoboChaseEnemyFailed[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_WAIT,					(float)2					},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)2					},
};

Schedule_t	slRoboChaseEnemyFailed[] =
{
	{
		tlRoboChaseEnemyFailed,
		ARRAYSIZE ( tlRoboChaseEnemyFailed ),
		bits_COND_NEW_ENEMY			|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		0,
		"tlRoboChaseEnemyFailed"
	},
};

DEFINE_CUSTOM_SCHEDULES( CRoboLaser )
{
	slRoboWakeAngry,
	slRoboChaseEnemyFailed,
};

IMPLEMENT_CUSTOM_SCHEDULES( CRoboLaser, CBaseMonster );

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CRoboLaser :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_FACTION_A;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CRoboLaser :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 90;
		break;
	default:
		ys = 35;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// AlertSound
//=========================================================
void CRoboLaser :: AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "robolaser/robo_startup.wav", 1.0, 0.3, 0, pitch );

	m_iStartRunSound = TRUE;
	m_flAwakeTime = gpGlobals->time;
}

//=========================================================
// DeathSound
//=========================================================
void CRoboLaser :: DeathSound( void )
{
	EMIT_SOUND( ENT(pev), CHAN_BODY, "robolaser/robo_die.wav", 1.0, 0.3 );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CRoboLaser :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case ROBO_AE_STEP:
			EMIT_SOUND( edict(), CHAN_BODY, pEvent->options, 0.7, 0.3 );
			break;

		case ROBO_AE_STOMP:
			UTIL_ScreenShake( pev->origin, 4.0, 3.0, 0.6, 1500.0 );
			EMIT_SOUND( edict(), CHAN_BODY, pEvent->options, 0.8, 0.3 );
			break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// ISoundMask
//=========================================================
int CRoboLaser :: ISoundMask ( void )
{
	return CBaseMonster::ISoundMask();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CRoboLaser :: Precache()
{
	PRECACHE_SOUND("ambience/loader_hydra1.wav");
	PRECACHE_SOUND("ambience/loader_step1.wav");
	PRECACHE_SOUND("weapons/explode4.wav");
	PRECACHE_SOUND("robolaser/robo_startup.wav");
	PRECACHE_SOUND("robolaser/robo_run.wav");
	PRECACHE_SOUND("robolaser/robo_fire.wav");
	PRECACHE_SOUND("robolaser/robo_firewinddown.wav");
	PRECACHE_SOUND("robolaser/robo_firewindup.wav");
	PRECACHE_SOUND("robolaser/robo_die.wav");

	PRECACHE_MODEL("sprites/laserbeam.spr");
	PRECACHE_MODEL("sprites/zbeam2.spr");
	PRECACHE_MODEL("models/robolaser.mdl");
	PRECACHE_MODEL("models/robolaser_largegibs.mdl");
	PRECACHE_MODEL("sprites/gargeye1.spr");

	m_iExplode = PRECACHE_MODEL( "sprites/fexplo.spr" );
	m_iBodyGibs = PRECACHE_MODEL( "models/robolaser_smallgibs.mdl" );
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/white.spr" );

	m_iLastFireSoundPlayed = 0;
}

//=========================================================
// Spawn
//=========================================================
void CRoboLaser :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/robolaser.mdl");
	UTIL_SetSize( pev, Vector( -128, -128, 0 ), Vector( 128, 128, 192 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= DONT_BLEED;
	pev->health			= gSkillData.m450Health;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= -0.7;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	m_flInRangeTime = gpGlobals->time;
	m_flOutOfRangeTime = gpGlobals->time;
	m_iStartRunSound = FALSE;
	m_flAwakeTime = 0;
	PlayFireSound( 0 );

	MonsterInit();

	m_pLaserGlow = CSprite::SpriteCreate( "sprites/gargeye1.spr", pev->origin, FALSE );
	m_pLaserGlow->SetTransparency( kRenderGlow, 255, 255, 255, 0, kRenderFxNoDissipation );
	m_pLaserGlow->SetAttachment( edict(), 1 );
	m_pLaserGlow->SetScale( 2.0 );
	LaserGlowOff();
}

//=========================================================
// RunAI
//=========================================================
void CRoboLaser :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if ( m_iStartRunSound && m_flAwakeTime + 2 < gpGlobals->time )
	{
		EMIT_SOUND( ENT(pev), CHAN_STATIC, "robolaser/robo_run.wav", 0.7, 0.3 );
		m_iStartRunSound = FALSE;
	}

	if ( pev->deadflag == DEAD_NO )
		FireLaser();
}

//=========================================================
// FireLaser - aims the gun at the enemy, fires the beam
// once it has been on target long enough
//=========================================================
void CRoboLaser :: FireLaser( void )
{
	if ( m_hEnemy == NULL )
	{
		EndAttack();
		m_flOutOfRangeTime = gpGlobals->time;
		return;
	}

	if ( !FVisible( m_hEnemy ) )
	{
		EndAttack();
		m_flOutOfRangeTime = gpGlobals->time;
		PlayFireSound( 3 );
		return;
	}

	UTIL_MakeAimVectors( pev->angles );

	Vector posGun, angGun;
	GetAttachment( 1, posGun, angGun );

	Vector vecEnemy = m_hEnemy->Center( );
	Vector vecTarget = (vecEnemy - posGun).Normalize( );

	Vector vecOut;

	vecOut.x = DotProduct( gpGlobals->v_forward, vecTarget );
	vecOut.y = -DotProduct( gpGlobals->v_right, vecTarget );
	vecOut.z = DotProduct( gpGlobals->v_up, vecTarget );

	Vector angles = UTIL_VecToAngles (vecOut);

	angles.x = -angles.x;
	if (angles.y > 180)
		angles.y = angles.y - 360;
	if (angles.y < -180)
		angles.y = angles.y + 360;
	if (angles.x > 180)
		angles.x = angles.x - 360;
	if (angles.x < -180)
		angles.x = angles.x + 360;

	if (angles.x > m_angGun.x)
		m_angGun.x = min( angles.x, m_angGun.x + 5.0 );
	if (angles.x < m_angGun.x)
		m_angGun.x = max( angles.x, m_angGun.x - 5.0 );
	if (angles.y > m_angGun.y)
		m_angGun.y = min( angles.y, m_angGun.y + 5.0 );
	if (angles.y < m_angGun.y)
		m_angGun.y = max( angles.y, m_angGun.y - 5.0 );

	m_angGun.y = SetBoneController( 0, m_angGun.y );
	m_angGun.x = SetBoneController( 1, m_angGun.x );

	Vector posBarrel, angBarrel;
	GetAttachment( 0, posBarrel, angBarrel );
	Vector vecGun = (posBarrel - posGun).Normalize( );

	if ( DotProduct( vecGun, vecTarget ) < 0.9 || (m_hEnemy->pev->origin - pev->origin).Length() > 1200 )
	{
		m_flOutOfRangeTime = gpGlobals->time;
		PlayFireSound( 3 );
	}
	else
	{
		m_flInRangeTime = gpGlobals->time;
		PlayFireSound( 1 );
	}

	m_LaserGlowBrightness = (m_flInRangeTime - m_flOutOfRangeTime) * 255 / 2.0;
	if ( m_LaserGlowBrightness > 255 )
		m_LaserGlowBrightness = 255;
	if ( m_LaserGlowBrightness < 0 )
		m_LaserGlowBrightness = 0;
	LaserGlowUpdate();

	if ( DotProduct( vecGun, vecTarget ) > 0.985 && m_flInRangeTime - m_flOutOfRangeTime > 2.0 )
	{
		TraceResult tr;
		UTIL_TraceLine( posGun, posGun + vecTarget * 8192, dont_ignore_monsters, ENT( pev ), &tr );

		if ( !tr.fAllSolid )
		{
			UpdateEffect( posGun, tr.vecEndPos, 1 );
			PlayFireSound( 2 );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( pEntity != NULL )
			{
				float timedist;

				if ( pev->dmgtime < gpGlobals->time )
				{
					ClearMultiDamage();
					if (pEntity->pev->takedamage)
					{
						pEntity->TraceAttack( pev, gSkillData.plrDmgEgonNarrow, vecTarget, &tr, DMG_ENERGYBEAM );
					}
					ApplyMultiDamage(pev, pev);

					pev->dmgtime = gpGlobals->time + 0.1;
				}

				timedist = ( pev->dmgtime - gpGlobals->time ) / 0.1;

				if ( timedist < 0 )
					timedist = 0;
				else if ( timedist > 1 )
					timedist = 1;
				timedist = 1-timedist;
			}
		}
	}
	else
	{
		EndAttack();
		if ( m_iLastFireSoundPlayed == 2 )
			PlayFireSound( 3 );
	}
}

void CRoboLaser :: UpdateEffect( const Vector &startPoint, const Vector &endPoint, float timeBlend )
{
	if ( !m_pBeam )
	{
		CreateEffect();
	}

	float flPulse = fabs(sin(gpGlobals->time*0.1));

	m_pBeam->SetStartPos( endPoint );

	m_pNoise->SetColor( 60 + (25*flPulse), 120 + (30*flPulse), 64 + 80*flPulse );
	m_pNoise->SetStartPos( endPoint );
}

void CRoboLaser :: CreateEffect( void )
{
	DestroyEffect();

	m_pBeam = CBeam::BeamCreate( "sprites/laserbeam.spr", 40 );
	m_pBeam->PointEntInit( pev->origin, entindex() );
	m_pBeam->SetEndAttachment( 1 );
	m_pBeam->SetBrightness( 240 );
	m_pBeam->SetColor( 255, 0, 0 );
	m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;	// Flag these to be destroyed on save/restore or level transition
	m_pBeam->pev->owner = edict();

	m_pNoise = CBeam::BeamCreate( "sprites/zbeam2.spr", 85 );
	m_pNoise->PointEntInit( pev->origin, entindex() );
	m_pNoise->SetScrollRate( 25 );
	m_pNoise->SetBrightness( 96 );
	m_pNoise->SetEndAttachment( 1 );
	m_pNoise->pev->spawnflags |= SF_BEAM_TEMPORARY;
	m_pNoise->pev->owner = edict();

	m_pBeam->SetScrollRate( 110 );
	m_pBeam->SetNoise( 5 );
	m_pNoise->SetColor( 80, 120, 255 );
	m_pNoise->SetNoise( 2 );
}

void CRoboLaser :: DestroyEffect( void )
{
	if ( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
	if ( m_pNoise )
	{
		UTIL_Remove( m_pNoise );
		m_pNoise = NULL;
	}
}

void CRoboLaser :: EndAttack( void )
{
	DestroyEffect();
}

//=========================================================
// TraceAttack
//=========================================================
void CRoboLaser :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( ptr->iHitgroup == 0 && (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB | DMG_ENERGYBEAM)) )
	{
		// hit armor
		if ( RANDOM_LONG( 0, 10 ) < 1 )
			UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 1, 2) );
		else if ( RANDOM_LONG( 0, 10 ) < 1 )
			UTIL_Sparks( ptr->vecEndPos );

		flDamage = 0.1;// don't hurt the monster much, but allow bits_COND_LIGHT_DAMAGE to be generated
	}
	else if ( ptr->iHitgroup == 1 )
	{
		SpawnBlood( ptr->vecEndPos, BLOOD_COLOR_RED, flDamage );
		TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
		flDamage *= 1.7;
	}

	AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
}

//=========================================================
// Killed
//=========================================================
void CRoboLaser :: Killed( entvars_t *pevAttacker, int iGib )
{
	EndAttack();
	STOP_SOUND( ENT(pev), CHAN_STATIC, "robolaser/robo_run.wav" );
	PlayFireSound( 0 );
	UTIL_Remove( m_pLaserGlow );
	m_pLaserGlow = NULL;
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}

//=========================================================
// StartTask
//=========================================================
void CRoboLaser :: StartTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_GET_PATH_TO_ENEMY:
		{
			CBaseEntity *pEnemy = m_hEnemy;

			if ( pEnemy == NULL )
			{
				TaskFail();
				return;
			}

			if (BuildNearestRoute( pEnemy->pev->origin, pEnemy->pev->view_ofs, 0, (pEnemy->pev->origin - pev->origin).Length() ))
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				ALERT ( at_aiconsole, "GetPathToEnemy failed!!\n" );
				TaskFail();
			}
			break;
		}
	case TASK_DIE:
		m_flWaitFinished = gpGlobals->time + 6.0;
		// FALL THROUGH
	default:
		CBaseMonster :: StartTask ( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CRoboLaser :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_DIE:
		if ( gpGlobals->time > m_flWaitFinished )
		{
			pev->renderamt = 0;
			pev->rendermode = kRenderTransTexture;
			StopAnimation();
			SetNextThink( 1 );
			SetThink( SUB_Remove );
			DeathExplosion();
			return;
		}
		else
		{
			ShakeGun();
		}
		break;

	default:
		CBaseMonster :: RunTask( pTask );
		break;
	}
}

//=========================================================
// DeathExplosion - the apache's last blast
//=========================================================
void CRoboLaser :: DeathExplosion( void )
{
	CGib::SpawnRandomGibs( pev, 12, 1 );
	CGib::SpawnRandomGibs( pev, 4, 0, "models/robolaser_largegibs.mdl" );

	Vector vecSpot = pev->origin;

	// fireball
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
		WRITE_BYTE( TE_SPRITE );
		WRITE_COORD( vecSpot.x );
		WRITE_COORD( vecSpot.y );
		WRITE_COORD( vecSpot.z + 48 );
		WRITE_SHORT( m_iExplode );
		WRITE_BYTE( 15 ); // scale * 10
		WRITE_BYTE( 255 ); // brightness
	MESSAGE_END();

	// big smoke
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
		WRITE_BYTE( TE_SMOKE );
		WRITE_COORD( vecSpot.x );
		WRITE_COORD( vecSpot.y );
		WRITE_COORD( vecSpot.z + 48 );
		WRITE_SHORT( g_sModelIndexSmoke );
		WRITE_BYTE( 15 ); // scale * 10
		WRITE_BYTE( 5  ); // framerate
	MESSAGE_END();

	// blast circle
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z);
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 1000 ); // reach damage radius over .2 seconds
		WRITE_SHORT( m_iSpriteTexture );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 4 ); // life
		WRITE_BYTE( 32 );  // width
		WRITE_BYTE( 0 );   // noise
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 192 );   // r, g, b
		WRITE_BYTE( 128 ); // brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();

	EMIT_SOUND(ENT(pev), CHAN_STATIC, "weapons/explode4.wav", 1.0, 0.3);

	RadiusDamage( pev->origin, pev, pev, 150, CLASS_NONE, DMG_BLAST );

	if ( pev->flags & FL_ONGROUND )
	{
		CBaseEntity *pWreckage = Create( "cycler_wreckage", pev->origin, pev->angles );
		// SET_MODEL( ENT(pWreckage->pev), STRING(pev->model) );
		UTIL_SetSize( pWreckage->pev, Vector( -200, -200, -128 ), Vector( 200, 200, -32 ) );
		pWreckage->pev->frame = pev->frame;
		pWreckage->pev->sequence = pev->sequence;
		pWreckage->pev->framerate = 0;
		pWreckage->pev->dmgtime = gpGlobals->time + 5;
	}

	// gibs
	vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
		WRITE_BYTE( TE_BREAKMODEL);

		// position
		WRITE_COORD( vecSpot.x );
		WRITE_COORD( vecSpot.y );
		WRITE_COORD( vecSpot.z + 64);

		// size
		WRITE_COORD( 400 );
		WRITE_COORD( 400 );
		WRITE_COORD( 128 );

		// velocity
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );
		WRITE_COORD( 50 );

		// randomization
		WRITE_BYTE( 30 );

		// Model
		WRITE_SHORT( m_iBodyGibs );	//model id#

		// # of shards
		WRITE_BYTE( 80 );

		// duration
		WRITE_BYTE( 100 );

		// flags

		WRITE_BYTE( BREAK_METAL );
	MESSAGE_END();
}

//=========================================================
// ShakeGun - the gun twitches while the robot dies
//=========================================================
void CRoboLaser :: ShakeGun( void )
{
	m_angGun.x = m_angGun.x + RANDOM_FLOAT( -2, 2 ) * 5.0;
	m_angGun.y = m_angGun.y + RANDOM_FLOAT( -2, 2 ) * 5.0;

	m_angGun.y = SetBoneController( 0, m_angGun.y );
	m_angGun.x = SetBoneController( 1, m_angGun.x );
}

void CRoboLaser :: LaserGlowOff( void )
{
	m_LaserGlowBrightness = 0;
}

void CRoboLaser :: LaserGlowUpdate( void )
{
	if ( m_pLaserGlow )
	{
		m_pLaserGlow->pev->renderamt = m_LaserGlowBrightness;
		if ( m_pLaserGlow->pev->renderamt == 0 )
			m_pLaserGlow->pev->effects |= EF_NODRAW;
		else
			m_pLaserGlow->pev->effects &= ~EF_NODRAW;

		Vector posGun, angGun;
		GetAttachment( 0, posGun, angGun );
		UTIL_SetOrigin( m_pLaserGlow, posGun );
	}
}

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t* CRoboLaser :: GetScheduleOfType ( int Type )
{
	switch	( Type )
	{
	case SCHED_WAKE_ANGRY:
		return &slRoboWakeAngry[ 0 ];
	case SCHED_CHASE_ENEMY_FAILED:
		return &slRoboChaseEnemyFailed[ 0 ];
	}

	return CBaseMonster :: GetScheduleOfType( Type );
}

//=========================================================
// PlayFireSound - 1 wind up, 2 fire, 3 wind down, 0 silence
//=========================================================
void CRoboLaser :: PlayFireSound( int iSound )
{
	if ( iSound != m_iLastFireSoundPlayed )
	{
		switch ( iSound )
		{
		case 1:
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "robolaser/robo_firewindup.wav", 1.0, 0.3 );
			m_iLastFireSoundPlayed = iSound;
			break;
		case 2:
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "robolaser/robo_fire.wav", 1.0, 0.3 );
			m_iLastFireSoundPlayed = iSound;
			break;
		case 3:
			if ( m_iLastFireSoundPlayed != 0 )
			{
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "robolaser/robo_firewinddown.wav", 1.0, 0.3 );
				m_iLastFireSoundPlayed = 0;
			}
			break;
		default:
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "common/null.wav", 1.0, 0.3 );
			m_iLastFireSoundPlayed = 0;
			break;
		}
	}
}

//=========================================================
// MonsterInit
//=========================================================
void CRoboLaser :: MonsterInit( void )
{
	CBaseMonster::MonsterInit();
	m_flDistTooFar = 3200;
}
