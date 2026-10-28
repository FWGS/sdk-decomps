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
// friendly.cpp - T2DEF's Macrobrachius (Half-Life's unused
// friendly.mdl): claws and lobbed loogies (file name ours)
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"game.h"
#include	"decals.h"


//=========================================================
// CFriendlyLoogie - the loogie, after apache.cpp's
// CApacheHVR and ggrenade.cpp's CGrenade
//=========================================================
class CFriendlyLoogie : public CBaseMonster
{
	void Spawn( void );
	void Precache( void );
	void EXPORT IgniteThink( void );
	void EXPORT AccelerateThink( void );
	void EXPORT LoogieTouch( CBaseEntity *pOther );
	void Explode( TraceResult *pTrace, int bitsDamageType );

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int m_iTrail;
	Vector m_vecForward;
	int m_iRing; // name ours
};
LINK_ENTITY_TO_CLASS( friendlyloogie, CFriendlyLoogie );

TYPEDESCRIPTION	CFriendlyLoogie::m_SaveData[] =
{
	DEFINE_FIELD( CFriendlyLoogie, m_vecForward, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CFriendlyLoogie, CBaseMonster );

void CFriendlyLoogie :: Spawn( void )
{
	Precache( );

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/friendlyloogie.mdl");
	UTIL_SetSize(pev, Vector( -4, -4, 0), Vector(4, 4, 8));
	UTIL_SetOrigin( this, pev->origin );

	SetThink( IgniteThink );
	SetTouch( LoogieTouch );

	UTIL_MakeAimVectors( pev->angles );
	m_vecForward = gpGlobals->v_forward;
	pev->gravity = 1.0;

	SetNextThink( 0.1 );

	pev->dmg = 15;

	pev->avelocity.x = RANDOM_FLOAT( -500, 500 );
	pev->avelocity.y = RANDOM_FLOAT( -500, 500 );
	pev->avelocity.z = RANDOM_FLOAT( -500, 500 );
}

void CFriendlyLoogie :: Precache( void )
{
	PRECACHE_MODEL("models/friendlyloogie.mdl");
	m_iTrail = PRECACHE_MODEL("sprites/smoke.spr");
	PRECACHE_SOUND ("friendly/loogie_fire.wav");
	PRECACHE_SOUND ("friendly/loogie_impact.wav");
	m_iRing = PRECACHE_MODEL("sprites/white.spr");
}

void CFriendlyLoogie :: IgniteThink( void  )
{
	// make loogie sound
	EMIT_SOUND( ENT(pev), CHAN_VOICE, "friendly/loogie_fire.wav", 1, 0.5 );

	// loogie trail
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );

		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT(entindex());	// entity
		WRITE_SHORT(m_iTrail );	// model
		WRITE_BYTE( 1 ); // life
		WRITE_BYTE( 3 );  // width
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 0 );   // r, g, b
		WRITE_BYTE( 255 );	// brightness

	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	// set to accelerate
	SetThink( AccelerateThink );
	SetNextThink( 0.1 );
}

void CFriendlyLoogie :: AccelerateThink( void  )
{
	// check world boundaries
	if (pev->origin.x < -4096 || pev->origin.x > 4096 || pev->origin.y < -4096 || pev->origin.y > 4096 || pev->origin.z < -4096 || pev->origin.z > 4096)
	{
		UTIL_Remove( this );
		return;
	}

	// drip
	if ( RANDOM_LONG( 0, 15 ) < 3 )
		UTIL_BloodDrips( pev->origin, g_vecZero, BLOOD_COLOR_YELLOW, 80 );

	SetNextThink( 0.1 );
}

void CFriendlyLoogie :: LoogieTouch( CBaseEntity *pOther )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT(pev), &tr );

	Explode( &tr, DMG_BLAST );
}

void CFriendlyLoogie :: Explode( TraceResult *pTrace, int bitsDamageType )
{
	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	pev->takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->flFraction != 1.0 )
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * (pev->dmg - 24) * 0.6);
	}

	int iContents = UTIL_PointContents ( pev->origin );

	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	RadiusDamage ( pev, pevOwner, pev->dmg, CLASS_NONE, bitsDamageType );

	if ( RANDOM_FLOAT( 0 , 1 ) < 0.5 )
	{
		UTIL_DecalTrace( pTrace, DECAL_YBLOOD1 );
	}
	else
	{
		UTIL_DecalTrace( pTrace, DECAL_YBLOOD3 );
	}

	// splash ring
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z);
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 200 ); // reach damage radius over .2 seconds
		WRITE_SHORT( m_iRing );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 2 ); // life
		WRITE_BYTE( 32 );  // width
		WRITE_BYTE( 0 );   // noise
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 0 );   // r, g, b
		WRITE_BYTE( 128 ); // brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();

	Vector vecUp = Vector( 0, 0, 1 );
	UTIL_BloodDrips( pev->origin, vecUp, BLOOD_COLOR_YELLOW, 200 );

	EMIT_SOUND( ENT(pev), CHAN_VOICE, "friendly/loogie_impact.wav", 1, ATTN_NORM );

	UTIL_Remove( this );
}


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	FRIENDLY_AE_ATTACK_RIGHT	0x01 // names ours
#define	FRIENDLY_AE_ATTACK_LEFT		0x02
#define	FRIENDLY_AE_SPIT			0x03
#define	FRIENDLY_AE_ATTACK_SOUND	0x04

#define FRIENDLY_FLINCH_DELAY		2		// at most one flinch every n secs

//=========================================================
// CFriendly - the Macrobrachius (class name ours), after
// zombie.cpp
//=========================================================
class CFriendly : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void MonsterInit( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int IgnoreConditions ( void );
	void PrescheduleThink( void );

	float m_flNextFlinch;
	float m_flNextSpitTime;
	BOOL m_fHighArc; // name ours

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void DeathSound( void );

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
	static const char *pDeathSounds[];

	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack2 ( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
};

LINK_ENTITY_TO_CLASS( monster_friendly, CFriendly );

const char *CFriendly::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CFriendly::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CFriendly::pAttackSounds[] =
{
	"macro/mb_attack1.wav",
	"macro/mb_attack2.wav",
};

const char *CFriendly::pIdleSounds[] =
{
	"macro/mb_idle1.wav",
	"macro/mb_idle2.wav",
};

const char *CFriendly::pAlertSounds[] =
{
	"macro/mb_alert1.wav",
	"macro/mb_alert2.wav",
	"macro/mb_alert3.wav",
};

const char *CFriendly::pPainSounds[] =
{
	"macro/mb_pain1.wav",
	"macro/mb_pain2.wav",
};

const char *CFriendly::pDeathSounds[] =
{
	"macro/mb_die1.wav",
};

void CFriendly :: MonsterInit( void )
{
	CBaseMonster::MonsterInit();

	m_flDistTooFar = 3200;
}

//=========================================================
// CheckRangeAttack1 - can a loogie launched at 1600 units
// per second reach the enemy?
//=========================================================
BOOL CFriendly :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( m_hEnemy == NULL || gpGlobals->time <= m_flNextSpitTime )
		return FALSE;

	if ( flDist > 256 )
	{
		float gravity = g_psv_gravity->value;
		if (gravity <= 1)
			gravity = 1;

		float h = m_hEnemy->pev->origin.z + m_hEnemy->pev->view_ofs.z - pev->origin.z - 47.0;
		float enemydist = ( m_hEnemy->pev->origin - pev->origin ).Length();

		float flMaxRange = powf( 1600, 4 ) / powf( gravity, 2 );
		flMaxRange = flMaxRange - 2 * powf( 1600, 2 ) * h / gravity;

		if ( flMaxRange < 0 )
			return FALSE;
		else
			flMaxRange = sqrt( flMaxRange );
		flMaxRange = flMaxRange * 0.95;

		if ( enemydist <= flMaxRange )
		{
			if ( IsMoving() )
			{
				// don't spit again for a long time, resume chasing enemy.
				m_flNextSpitTime = gpGlobals->time + 3;
			}
			else
			{
				// not moving, so spit again pretty soon.
				m_flNextSpitTime = gpGlobals->time + 0.5;
			}

			return TRUE;
		}
	}

	return FALSE;
}

BOOL CFriendly :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 100 && flDot >= 0.7 && !HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CFriendly :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CFriendly :: SetYawSpeed ( void )
{
	int ys;

	ys = 60;

	pev->yaw_speed = ys;
}

int CFriendly :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Take 30% damage from bullets
	if ( bitsDamageType == DMG_BULLET )
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce( flDamage );
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.3;
	}

	// HACK HACK -- until we fix this.
	if ( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CFriendly :: PainSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_IDLE, 0, pitch );
}

void CFriendly :: AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NONE, 0, pitch );
}

void CFriendly :: IdleSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	// Play a random idle sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_IDLE, 0, 100 + RANDOM_LONG(-5,5) );
}

void CFriendly :: AttackSound( void )
{
	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_IDLE, 0, 100 + RANDOM_LONG(-5,5) );
}

void CFriendly :: DeathSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pDeathSounds[ RANDOM_LONG(0,ARRAYSIZE(pDeathSounds)-1) ], 1.0, ATTN_IDLE, 0, pitch );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CFriendly :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case FRIENDLY_AE_ATTACK_SOUND:
			AttackSound();
		break;

		case FRIENDLY_AE_ATTACK_RIGHT:
		{
			// do stuff for this event.
			CBaseEntity *pHurt = CheckTraceHullAttack( 100, gSkillData.friendlyDmgOneSlash, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -25;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 300;
				}
				// Play a random attack hit sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else // Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
		}
		break;

		case FRIENDLY_AE_ATTACK_LEFT:
		{
			// do stuff for this event.
			CBaseEntity *pHurt = CheckTraceHullAttack( 100, gSkillData.friendlyDmgBothSlash, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->punchangle.z = -25;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 300;
				}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
		}
		break;

		case FRIENDLY_AE_SPIT:
		{
			if ( m_hEnemy != NULL )
			{
				// lob the loogie at 1600 units per second (names ours)
				float gravity = g_psv_gravity->value;
				if (gravity <= 1)
					gravity = 1;

				float h = m_hEnemy->pev->origin.z + m_hEnemy->pev->view_ofs.z - pev->origin.z - 47.0;
				float deltaX = m_hEnemy->pev->origin.x - pev->origin.x;
				float deltaY = m_hEnemy->pev->origin.y - pev->origin.y;

				float fB = 2560000.0 - gravity * h;
				float fA = gravity * gravity / 4;
				float fC = powf( deltaX, 2 ) + powf( deltaY, 2 ) + powf( h, 2 );

				float disc = fB * fB - 4 * fC * fA;
				if ( disc > 0 )
					disc = sqrt( disc );
				else
				{
					ALERT( at_console, "loogie failed! negative operand!\n" );
					break;
				}

				int iSign = 1;
				if ( RANDOM_LONG( 0, 5 ) < 2 || sqrt( deltaX * deltaX + deltaY * deltaY ) < 512 || m_fHighArc )
				{
					iSign = -1;
					m_fHighArc = FALSE;
				}
				else
				{
					m_fHighArc = TRUE;
				}

				float hangTime = ( iSign * disc + fB ) / ( 2 * fA );
				if ( hangTime > 0 )
					hangTime = sqrt( hangTime );
				else
				{
					ALERT( at_console, "loogie failed! negative hang time!\n" );
					break;
				}

				Vector vecOrigin;
				UTIL_MakeVectors( pev->angles );
				vecOrigin = -gpGlobals->v_right + gpGlobals->v_forward * 32 + gpGlobals->v_up * 47;
				vecOrigin = pev->origin + vecOrigin;

				CBaseEntity *pLoogie = CBaseEntity::Create( "friendlyloogie", vecOrigin, pev->angles, edict() );
				if ( pLoogie )
				{
					pLoogie->pev->velocity.x = deltaX / hangTime;
					pLoogie->pev->velocity.y = deltaY / hangTime;
					pLoogie->pev->velocity.z = h / hangTime + gravity * hangTime / 2;
					pLoogie->pev->angles = UTIL_VecToAngles( pLoogie->pev->velocity );
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
// Spawn
//=========================================================
void CFriendly :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/friendly.mdl");
	UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 64 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->health			= gSkillData.friendlyHealth;
	pev->view_ofs		= Vector ( 0, 0, 28 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextSpitTime	= gpGlobals->time;
	m_fHighArc			= FALSE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CFriendly :: Precache()
{
	int i;

	PRECACHE_MODEL("models/friendly.mdl");

	UTIL_PrecacheOther( "friendlyloogie" );

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pDeathSounds ); i++ )
		PRECACHE_SOUND((char *)pDeathSounds[i]);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CFriendly::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK1))
	{
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + FRIENDLY_FLINCH_DELAY;
	}

	return iIgnore;

}

//=========================================================
// PrescheduleThink
//=========================================================
void CFriendly :: PrescheduleThink ( void )
{
	// growl a little bit in combat state
	if ( m_MonsterState == MONSTERSTATE_COMBAT && RANDOM_FLOAT( 0, 5 ) < 0.1 )
	{
		AlertSound();
	}
}
