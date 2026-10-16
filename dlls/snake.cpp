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
// Wanted: the rattlesnake (monster_snake). A copy of the SDK zombie
// (zombie.cpp, which Wanted doesn't link) with one bite attack, the
// snake's sounds and size, and a RunAI that bends the body along the
// ground through the model's bone controllers. The file name, CSnake
// and the names of the event and sound arrays are ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	SNAKE_AE_BITE		0x01

class CSnake : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void RunAI( void );
	void Killed( entvars_t *pevAttacker, int iGib );

	void PainSound( void );
	void IdleSound( void );
	void AttackSound( void );

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pPainSounds[];

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
};

LINK_ENTITY_TO_CLASS( monster_snake, CSnake );

const char *CSnake::pAttackSounds[] =
{
	"snake/snake_bite1.wav",
	"snake/snake_bite2.wav",
};

const char *CSnake::pIdleSounds[] =
{
	"snake/snake_idle1.wav",
	"snake/snake_idle2.wav",
};

const char *CSnake::pPainSounds[] =
{
	"snake/snake_pain1.wav",
	"snake/snake_pain2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CSnake :: Classify ( void )
{
	return	CLASS_SNAKE;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CSnake :: SetYawSpeed ( void )
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

int CSnake :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Take 50% damage from bullets
	if ( bitsDamageType == DMG_BULLET )
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce( flDamage );
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.5;
	}

	// HACK HACK -- until we fix this.
	if ( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CSnake :: PainSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CSnake :: IdleSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	// Play a random idle sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

void CSnake :: AttackSound( void )
{
	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CSnake :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case SNAKE_AE_BITE:
		{
			// do stuff for this event.
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.snakeDmgBite, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = 18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
				}
				// Play a random attack hit sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}

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
void CSnake :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/snake.mdl");
	InitBoneControllers( );
	UTIL_SetSize( pev, Vector( -15, -15, 0 ), Vector( 15, 15, 55 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= gSkillData.snakeHealth;
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// RunAI - lay the body along the ground: trace down under the
// middle, then down from points half the body length ahead and
// behind along the ground's normal, and bend the body (controller 2)
// and the head (controller 0) by the difference.
//=========================================================
void CSnake :: RunAI( void )
{
	TraceResult tr, tr2;
	Vector vecForward;
	Vector vecFront, vecBack;
	float flBend;

	UTIL_TraceLine( pev->origin + Vector( 0, 0, 16 ), pev->origin - Vector( 0, 0, 32 ), ignore_monsters, ENT( pev ), &tr );
	g_engfuncs.pfnAngleVectors( pev->angles, vecForward, NULL, NULL );
	// sic: four discarded lengths. They make no code, but VC6 counts
	// them against the function's inline budget, and only with them
	// does it call the back trace's Vector operators out of line, as
	// retail does (retail's real leftover is unknown).
	vecForward.Length();
	vecForward.Length();
	vecForward.Length();
	vecForward.Length();

	Vector vecMid = pev->origin + tr.vecPlaneNormal * 16;
	vecFront = vecMid + vecForward * 64 * 0.5;
	vecBack = vecMid - vecForward * 64 * 0.5;

	UTIL_TraceLine( vecFront, vecFront - tr.vecPlaneNormal * 32, ignore_monsters, ENT( pev ), &tr );
	// sic: the back trace goes along the normal the front trace found
	UTIL_TraceLine( vecBack, vecBack - tr.vecPlaneNormal * 32, ignore_monsters, ENT( pev ), &tr2 );

	float flDiff = ( tr.flFraction - tr2.flFraction ) * 29.0;
	flBend = max( -20, min( 20, flDiff ) );

	SetBoneController( 2, flBend );
	SetBoneController( 0, 0 );

	float flFront = 1 - ( Vector( 0, 0, 1 ) - tr.vecPlaneNormal ).Length();
	float flBack = 1 - ( Vector( 0, 0, 1 ) - tr2.vecPlaneNormal ).Length();

	if ( flFront > flBack )
		flBend *= -1;

	if ( fabs( flFront - flBack ) > 0.001 )
	{
		if ( tr.flFraction < tr2.flFraction )
			SetBoneController( 0, flBend * 1.5 );
		else if ( tr2.flFraction < tr.flFraction )
			SetBoneController( 0, flBend * 0.8 );
	}

	CBaseMonster::RunAI();
}

void CSnake :: Killed( entvars_t *pevAttacker, int iGib )
{
	InitBoneControllers( );
	CBaseMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSnake :: Precache()
{
	int i;

	PRECACHE_MODEL("models/snake.mdl");
	PRECACHE_MODEL("sprites/xbeam1.spr");	// sic: never used

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);
}
