/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
// monster_renesaur: the renesaur's head, a scenery monster
// that can't be hurt. Its animation events play its bites,
// roars and growls; whatever comes within 200 units of its
// jaw is bitten, more often and harder on harder skills.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define RENESAUR_AE_BITE1			50
#define RENESAUR_AE_BITE2			51
#define RENESAUR_AE_BITE3			52
#define RENESAUR_AE_BITEDOOR		53
#define RENESAUR_AE_BITEHAND		54
#define RENESAUR_AE_DOWN1			55
#define RENESAUR_AE_DOWN2			56
#define RENESAUR_AE_DOWN3			57
#define RENESAUR_AE_BITEDEBRIS2		58
#define RENESAUR_AE_BITEDEBRIS1		59
#define RENESAUR_AE_ROAR1			60
#define RENESAUR_AE_ROAR2			61
#define RENESAUR_AE_ROAR3			62

#define RENESAUR_JAW_BONE			18
#define RENESAUR_BITE_RADIUS		200

class CRenesaur : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin - Vector( 1000, 1000, 0 );
		pev->absmax = pev->origin + Vector( 1000, 1000, 1000 );
	}
	int  Classify( void );
	void SetYawSpeed( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void RunAI( void );
	int  ISoundMask( void );

	int	Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	float	m_flNextBiteTime;
};

LINK_ENTITY_TO_CLASS( monster_renesaur, CRenesaur );

TYPEDESCRIPTION	CRenesaur::m_SaveData[] =
{
	DEFINE_FIELD( CRenesaur, m_flNextBiteTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CRenesaur, CBaseMonster );

int CRenesaur::Classify( void )
{
	return CLASS_NONE;
}

void CRenesaur::SetYawSpeed( void )
{
	pev->yaw_speed = 0;
}

//=========================================================
// HandleAnimEvent - the head's sounds
//=========================================================
void CRenesaur::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch ( pEvent->event )
	{
	case RENESAUR_AE_BITE1:
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "renesaur/ren_bite1.wav", 1, ATTN_NORM, 0, 90 );
		break;
	case RENESAUR_AE_BITE2:
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "renesaur/ren_bite2.wav", 1, ATTN_NORM, 0, 90 );
		break;
	case RENESAUR_AE_BITE3:
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "renesaur/ren_bite3.wav", 1, ATTN_NORM, 0, 90 );
		break;
	case RENESAUR_AE_BITEDOOR:
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "renesaur/ren_bitedoor.wav", 1, ATTN_NORM, 0, 90 );
		break;
	case RENESAUR_AE_BITEHAND:
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "renesaur/ren_bitehand.wav", 1, ATTN_NORM, 0, 90 );
		break;
	case RENESAUR_AE_DOWN1:
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "renesaur/ren_down1.wav", 1, ATTN_NORM, 0, 90 );
		break;
	case RENESAUR_AE_DOWN2:
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "renesaur/ren_down2.wav", 1, ATTN_NORM, 0, 90 );
		break;
	case RENESAUR_AE_DOWN3:
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "renesaur/ren_down3.wav", 1, ATTN_NORM, 0, 90 );
		break;
	case RENESAUR_AE_BITEDEBRIS2:
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "renesaur/ren_bitedebris2.wav", 1, ATTN_NORM, 0, 90 );
		break;
	case RENESAUR_AE_BITEDEBRIS1:
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "renesaur/ren_bitedebris1.wav", 1, ATTN_NORM, 0, 90 );
		break;
	case RENESAUR_AE_ROAR1:
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "renesaur/ren_roar1.wav", 1, ATTN_NORM, 0, 90 );
		break;
	case RENESAUR_AE_ROAR2:
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "renesaur/ren_roar2.wav", 1, ATTN_NORM, 0, 90 );
		break;
	case RENESAUR_AE_ROAR3:
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "renesaur/ren_roar3.wav", 1, ATTN_NORM, 0, 90 );
		break;
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// RunAI - bite whatever is near the jaw
//=========================================================
void CRenesaur::RunAI( void )
{
	Vector vecJaw, vecAngles;
	CBaseEntity *pEntity;

	CBaseMonster::RunAI();

	GetBonePosition( RENESAUR_JAW_BONE, vecJaw, vecAngles );

	if ( m_flNextBiteTime < gpGlobals->time )
	{
		pEntity = NULL;

		switch ( g_iSkillLevel )
		{
		case SKILL_EASY:
			while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, vecJaw, RENESAUR_BITE_RADIUS ) ) != NULL )
			{
				if ( pEntity->pev->takedamage != DAMAGE_NO )
					pEntity->TakeDamage( pev, pev, RANDOM_FLOAT( 10, 20 ), DMG_CRUSH );

				if ( pEntity->IsPlayer() )
					m_flNextBiteTime = gpGlobals->time + 0.7;
			}
			break;
		case SKILL_MEDIUM:
			while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, vecJaw, RENESAUR_BITE_RADIUS ) ) != NULL )
			{
				if ( pEntity->pev->takedamage != DAMAGE_NO )
					pEntity->TakeDamage( pev, pev, RANDOM_FLOAT( 10, 20 ), DMG_CRUSH );

				if ( pEntity->IsPlayer() )
					m_flNextBiteTime = gpGlobals->time + 0.3;
			}
			break;
		case SKILL_HARD:
			while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, vecJaw, RENESAUR_BITE_RADIUS ) ) != NULL )
			{
				if ( pEntity->pev->takedamage != DAMAGE_NO )
					pEntity->TakeDamage( pev, pev, RANDOM_FLOAT( 20, 40 ), DMG_CRUSH );

				if ( pEntity->IsPlayer() )
					m_flNextBiteTime = gpGlobals->time + 0.1;
			}
			break;
		}
	}
}

int CRenesaur::ISoundMask( void )
{
	return 0;	// it hears nothing
}

void CRenesaur::Spawn( void )
{
	Precache();

	SET_MODEL( ENT(pev), "models/renesaurhead.mdl" );

	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_NONE;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= 8;
	m_flFieldOfView		= 0;
	m_MonsterState		= MONSTERSTATE_IDLE;

	MonsterInit();

	m_flNextBiteTime	= gpGlobals->time;
	pev->solid			= SOLID_NOT;
	pev->takedamage		= DAMAGE_NO;
}

void CRenesaur::Precache( void )
{
	PRECACHE_MODEL( "models/renesaurhead.mdl" );

	PRECACHE_SOUND( "renesaur/ren_down3.wav" );
	PRECACHE_SOUND( "renesaur/ren_down2.wav" );
	PRECACHE_SOUND( "renesaur/ren_down1.wav" );
	PRECACHE_SOUND( "renesaur/ren_bitehand.wav" );
	PRECACHE_SOUND( "renesaur/ren_bitedoor.wav" );
	PRECACHE_SOUND( "renesaur/ren_bitedebris2.wav" );
	PRECACHE_SOUND( "renesaur/ren_bitedebris1.wav" );
	PRECACHE_SOUND( "renesaur/ren_bite3.wav" );
	PRECACHE_SOUND( "renesaur/ren_bite2.wav" );
	PRECACHE_SOUND( "renesaur/ren_bite1.wav" );
	PRECACHE_SOUND( "renesaur/ren_roar1.wav" );
	PRECACHE_SOUND( "renesaur/ren_roar2.wav" );
	PRECACHE_SOUND( "renesaur/ren_roar3.wav" );
}
