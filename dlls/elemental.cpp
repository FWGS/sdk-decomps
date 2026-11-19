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
//=========================================================
// elemental.cpp - the water elemental, its water blasts
// and the geysers of its spray attack (file name ours)
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"effects.h"
#include	"weapons.h"
#include	"decals.h"
#include	"soundent.h"
#include	"game.h"

extern DLL_GLOBAL Vector		g_vecAttackDir;

//=========================================================
// CWaterBlast - the elemental's shot: a splash sprite that
// leaves a trail of droplets
//=========================================================
class CWaterBlast : public CMagicMissile
{
public:
	void Spawn( void );
	void Precache( void );
	const char *GetKillDesc( void ) { return "a water blast"; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return "shot"; }
	const char *GetHitGroupDesc( int iHitGroup ) { return HitGroupIn( iHitGroup ); }
	void Touch( CBaseEntity *pOther );

	void EXPORT WaterThink( void );
};

LINK_ENTITY_TO_CLASS( water_blast, CWaterBlast );

void CWaterBlast::Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;

	SET_MODEL( ENT(pev), "sprites/watersplat.spr" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
	pev->framerate = 10;

	SetThink( WaterThink );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CWaterBlast::Precache( void )
{
	PRECACHE_MODEL( "sprites/watersplat.spr" );
	PRECACHE_SOUND( "bullchicken/bc_spithit1.wav" );
	PRECACHE_SOUND( "bullchicken/bc_spithit2.wav" );
}

void CWaterBlast::WaterThink( void )
{
	AnimateThink();

	Vector vecOrigin = pev->origin;
	vecOrigin.x += RANDOM_FLOAT( -10, 10 );
	vecOrigin.y += RANDOM_FLOAT( -10, 10 );
	vecOrigin.z += RANDOM_FLOAT( -10, 10 );

	CSprite *pSprite = CSprite::SpriteCreate( STRING( pev->model ), vecOrigin, TRUE );
	pSprite->pev->scale = RANDOM_FLOAT( 0.5, 1.0 );
	pSprite->pev->velocity = pev->velocity * 0.75;
	pSprite->pev->velocity.x += RANDOM_FLOAT( -25, 25 );
	pSprite->pev->velocity.y += RANDOM_FLOAT( -25, 25 );
	pSprite->pev->velocity.z += RANDOM_FLOAT( -25, 25 );

	int iColor = RANDOM_LONG( 128, 255 );
	pSprite->SetTransparency( kRenderTransAdd, iColor, iColor, 255, 255, kRenderFxNone );
	pSprite->AnimateAndDie( 5 );
}

void CWaterBlast::Touch( CBaseEntity *pOther )
{
	TraceResult tr;

	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit1.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit2.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		break;
	}

	tr = UTIL_GetGlobalTrace();

	if ( !pOther->pev->takedamage )
	{
		// make a splat on the wall
		UTIL_DecalTrace( &tr, DECAL_WATER1 + RANDOM_LONG( 0, 1 ) );

		// make some droplets
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos );
			WRITE_BYTE( TE_SPRAY );
			WRITE_COORD( tr.vecEndPos.x );	// pos
			WRITE_COORD( tr.vecEndPos.y );
			WRITE_COORD( tr.vecEndPos.z );
			WRITE_COORD( tr.vecPlaneNormal.x );	// dir
			WRITE_COORD( tr.vecPlaneNormal.y );
			WRITE_COORD( tr.vecPlaneNormal.z );
			WRITE_SHORT( pev->modelindex );	// model
			WRITE_BYTE( 5 );			// count
			WRITE_BYTE( 60 );			// speed
			WRITE_BYTE( 80 );			// noise ( client will divide by 100 )
			WRITE_BYTE( kRenderTransAdd );	// rendermode
		MESSAGE_END();
	}
	else
	{
		ClearMultiDamage();
		pOther->TraceAttack( VARS( pev->owner ), 10, pev->velocity.Normalize(), &tr, DMG_NEVERGIB | 0x8000000 );
		ApplyMultiDamage( pev, VARS( pev->owner ) );
	}

	SetThink( SUB_Remove );
	pev->nextthink = gpGlobals->time;
}


//=========================================================
// CWaterSpray - a geyser of the spray attack: sprays for
// ten seconds and throws up whatever stands in it
//=========================================================
class CWaterSpray : public CMagicMissile
{
public:
	void Spawn( void );
	const char *GetKillDesc( void ) { return "Hydro Pump"; }
	void Think( void );
	void Touch( CBaseEntity *pOther );

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	float	m_flEndTime;
};

LINK_ENTITY_TO_CLASS( water_spray, CWaterSpray );

TYPEDESCRIPTION	CWaterSpray::m_SaveData[] =
{
	DEFINE_FIELD( CWaterSpray, m_flEndTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CWaterSpray, CMagicMissile );

void CWaterSpray::Spawn( void )
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;

	SET_MODEL( ENT(pev), "sprites/watersplat.spr" );
	pev->rendercolor = Vector( 128, 192, 255 );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 72 ) );

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
	pev->framerate = 10;
	pev->nextthink = gpGlobals->time + 0.1;

	m_flEndTime = gpGlobals->time + 10;
}

void CWaterSpray::Think( void )
{
	if ( m_flEndTime < gpGlobals->time )
	{
		EMIT_SOUND( ENT(pev), CHAN_BODY, "common/null.wav", 1, ATTN_NORM );
		UTIL_Remove( this );
		return;
	}

	AnimateThink();

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SPRAY );
		WRITE_COORD( pev->origin.x );	// pos
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( 0 );	// dir
		WRITE_COORD( 0 );
		WRITE_COORD( 1 );
		WRITE_SHORT( pev->modelindex );	// model
		WRITE_BYTE( 5 );			// count
		WRITE_BYTE( 200 );			// speed
		WRITE_BYTE( 30 );			// noise ( client will divide by 100 )
		WRITE_BYTE( kRenderTransAdd );	// rendermode
	MESSAGE_END();

	CBaseEntity *pList[16];
	Vector vecMins = pev->origin + pev->mins;
	Vector vecMaxs = pev->origin + pev->maxs;
	int count = UTIL_EntitiesInBox( pList, 16, vecMins, vecMaxs, FL_CLIENT | FL_MONSTER );
	if ( count )
	{
		for ( int i = 0; i < count; i++ )
			Touch( pList[i] );
	}
}

void CWaterSpray::Touch( CBaseEntity *pOther )
{
	if ( pOther->pev->takedamage )
	{
		entvars_t *pevOwner = pev;
		if ( pev->owner )
			pevOwner = VARS( pev->owner );

		pOther->TakeDamage( pev, pevOwner, 3, DMG_NEVERGIB | 0x8000000 );

		if ( pOther->CanBeRepulsed( false ) )
			pOther->pev->velocity = pOther->pev->velocity + Vector( 0, 0, 64 );
	}
}


//=========================================================
// monster_water - the water elemental (class name ours)
//=========================================================

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	WATER_AE_SHOOT1		1	// names ours
#define	WATER_AE_SHOOT2		2
#define	WATER_AE_SPRAY		3
#define	WATER_AE_GLOW		4

class CWaterElemental : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	const char *GetHudName( void ) { return "Water Elemental"; }
	const char *GetKillDesc( void ) { return "a water elemental"; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return DamageVerb( DMG_CLUB, fGibbed ); }
	const char *GetHitGroupDesc( int iHitGroup ) { return HitGroupIn( iHitGroup ); }
	void SetYawSpeed( void );
	int  Classify( void );
	int  GetBattlePower( void );
	int  TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void MonsterThink( void );
	BOOL FCanCheckAttacks( void );
	BOOL CheckRangeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack2( float flDot, float flDist );
	void Killed( entvars_t *pevAttacker, int iGib );
	Schedule_t *GetScheduleOfType( int Type );
	void GibMonster( void );

	void SpawnSpray( const Vector &vecPos, int iDelay );	// name ours

	CUSTOM_SCHEDULES;

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
	static const char *pAttackSounds[];

	BOOL	m_fCanSpray;		// names ours: CheckRangeAttack2's last answer
	float	m_flNextSpray;		// until then CheckRangeAttack2 repeats that answer
	BOOL	m_fGlowing;			// the rain effect and the glow shell are on
	unsigned short m_usRain;	// events/rain.sc
};

LINK_ENTITY_TO_CLASS( monster_water, CWaterElemental );

const char *CWaterElemental::pAttackHitSounds[] =
{
	"bullchicken/bc_spithit1.wav",
	"bullchicken/bc_spithit2.wav",
};

const char *CWaterElemental::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CWaterElemental::pAttackSounds[] =
{
	"bullchicken/bc_attack2.wav",
	"bullchicken/bc_attack3.wav",
};

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

// spray attack: stand still, face the enemy, raise the geysers
Task_t	tlWaterPower[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK2,		(float)0		},
};

Schedule_t	slWaterPower[] =
{
	{
		tlWaterPower,
		ARRAYSIZE ( tlWaterPower ),
		0,
		0,
		"WaterPower"
	},
};

DEFINE_CUSTOM_SCHEDULES( CWaterElemental )
{
	slWaterPower,
};

IMPLEMENT_CUSTOM_SCHEDULES( CWaterElemental, CBaseMonster );

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CWaterElemental :: Classify ( void )
{
	return	19;	// a class the mod added
}

int CWaterElemental :: GetBattlePower ( void )
{
	return ( pev->max_health * 0.115 ) * 55;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CWaterElemental :: SetYawSpeed ( void )
{
	int ys;

	ys = 120;

	pev->yaw_speed = ys;
}

//=========================================================
// TakeDamage - poison, gas, acid and water don't hurt it,
// bullets only a little
//=========================================================
int CWaterElemental :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( bitsDamageType & ( DMG_NERVEGAS | DMG_POISON | DMG_ACID | 0x8000000 ) )
		return 0;

	if ( bitsDamageType & DMG_BULLET )
		flDamage *= 0.3;

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CWaterElemental :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case WATER_AE_SHOOT1:
		case WATER_AE_SHOOT2:
		{
			Vector vecGunPos, vecDir;

			GetAttachment( pEvent->event - 1, vecGunPos, vecDir );	// vecDir: the gun's angles, unused; it gets the aim below
			UTIL_MakeVectors( pev->angles );

			Vector vecSrc = Center();
			vecSrc.z = vecGunPos.z;
			vecSrc = vecSrc + gpGlobals->v_forward * 16;

			vecDir = ShootAtEnemy( vecSrc );

			CBaseEntity *pBlast = CBaseEntity::Create( "water_blast", vecGunPos, g_vecZero, edict() );

			// get circular gaussian spread
			float x, y;
			do {
				x = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
				y = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
			} while ( x*x+y*y > 1 );

			pBlast->pev->velocity = ( vecDir + x * 0.1 * gpGlobals->v_right + y * 0.1 * gpGlobals->v_up ) * 800;

			EMIT_SOUND( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG( 0, ARRAYSIZE( pAttackSounds ) - 1 ) ], 1.0, ATTN_NORM );
			CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 600, 0.3 );
		}
		break;

		case WATER_AE_SPRAY:
		{
			// five geysers in a row across the enemy's last known position, the outer ones later
			Vector vecSide, vecSrc, vecEnd;	// sic: vecSrc and vecEnd unused; with two (or three) such constructor calls the shot's last sum and product stay out of line, as retail

			Vector vecDir = pev->origin - m_vecEnemyLKP;
			Vector vecUp = Vector( 0, 0, 1 );	// a copy-initialised local, not a temporary in the call: the Normalize below then keeps retail's x87 operand order
			vecSide = CrossProduct( vecDir.Normalize(), vecUp );

			SpawnSpray( m_vecEnemyLKP, 0 );
			SpawnSpray( m_vecEnemyLKP + vecSide * 40, 1 );
			SpawnSpray( m_vecEnemyLKP + vecSide * 80, 2 );
			SpawnSpray( m_vecEnemyLKP - vecSide * 40, 1 );
			SpawnSpray( m_vecEnemyLKP - vecSide * 80, 2 );

			m_flNextSpray = gpGlobals->time + 6;
			m_fCanSpray = FALSE;
			pev->renderfx = kRenderFxNone;
		}
		break;

		case WATER_AE_GLOW:
		{
			if ( m_fGlowing )
			{
				PLAYBACK_EVENT_FULL( FEV_RELIABLE | FEV_GLOBAL, edict(), m_usRain, 0.0, pev->origin, pev->angles, 0.0, 0.0, 0, 0, 1, 0 );	// without the casts (two inline conversions): the inlining above is retail's with one of the two calls spelled so
				m_fGlowing = FALSE;
				STOP_SOUND( ENT(pev), CHAN_BODY, "ambience/sparks.wav" );
			}
			else
			{
				PLAYBACK_EVENT_FULL( FEV_GLOBAL, edict(), m_usRain, 0.0, (float *)&pev->origin, (float *)&pev->angles, 1.0, 0.1, 32, 1000, 0, 1 );
				m_fGlowing = TRUE;
				pev->renderfx = kRenderFxGlowShell;
				pev->rendercolor = g_vecZero;
				pev->renderamt = 32;
				EMIT_SOUND( ENT(pev), CHAN_BODY, "ambience/sparks.wav", 1, 0.3 );
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
void CWaterElemental :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/welem.mdl");
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 72 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= DONT_BLEED;
	pev->health			= 100;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CWaterElemental :: Precache()
{
	int i;

	PRECACHE_MODEL("models/welem.mdl");

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	m_usRain = PRECACHE_EVENT( 1, "events/rain.sc" );

	UTIL_PrecacheOther( "water_blast" );	// sic: not water_spray

	PRECACHE_SOUND( "ambience/waterrun.wav" );
	PRECACHE_SOUND( "ambience/sparks.wav" );
}

//=========================================================
// MonsterThink - the glow shell brightens, the skin
// ripples, and in water it heals
//=========================================================
void CWaterElemental :: MonsterThink( void )
{
	if ( m_fGlowing && pev->rendercolor.z < 255 )
		pev->rendercolor.z += 5;

	pev->skin = RANDOM_LONG( 0, 2 );

	if ( pev->waterlevel > 0 && pev->health < 100 )
		pev->health += 0.1;

	CBaseMonster::MonsterThink();
}

//=========================================================
// SpawnSpray - a geyser on the ground under vecPos, iDelay
// times 0.4 seconds late; none in the sky
//=========================================================
void CWaterElemental :: SpawnSpray( const Vector &vecPos, int iDelay )
{
	if ( UTIL_PointContents( vecPos ) == CONTENTS_SKY )
		return;

	TraceResult tr;
	UTIL_TraceLine( vecPos, vecPos - Vector( 0, 0, 80 ), ignore_monsters, ENT(pev), &tr );

	if ( tr.flFraction < 1.0 && UTIL_PointContents( tr.vecEndPos ) != CONTENTS_SKY )
	{
		CBaseEntity *pSpray = CBaseEntity::Create( "water_spray", tr.vecEndPos, g_vecZero, edict() );

		if ( iDelay )
			pSpray->pev->nextthink += iDelay * 0.4;
		else
			EMIT_SOUND( ENT(pSpray->pev), CHAN_BODY, "ambience/waterrun.wav", 1, ATTN_NORM );
	}
}

BOOL CWaterElemental :: FCanCheckAttacks ( void )
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
		return TRUE;

	return FALSE;
}

//=========================================================
// CheckRangeAttack1 - the water blast
//=========================================================
BOOL CWaterElemental :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 768 && flDot >= 0.5 )
	{
		if ( m_hEnemy->IsPlayer() || flDist > 64 )
		{
			TraceResult tr;
			Vector vecSrc = GetGunPosition();

			UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget( vecSrc ), ignore_monsters, ENT(pev), &tr );

			if ( tr.flFraction == 1.0 )
				return TRUE;
		}
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - the spray attack, standing still,
// on an enemy on or near the ground
//=========================================================
BOOL CWaterElemental :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if ( m_flGroundSpeed != 0 )
	{
		m_fCanSpray = FALSE;
		return FALSE;
	}

	if ( gpGlobals->time < m_flNextSpray )
		return m_fCanSpray;

	if ( HasConditions( bits_COND_SEE_ENEMY | bits_COND_ENEMY_OCCLUDED ) && flDist < 2048 && flDot >= 0.5 )
	{
		if ( !FBitSet( m_hEnemy->pev->flags, FL_ONGROUND ) )
		{
			TraceResult tr;
			UTIL_TraceLine( m_hEnemy->pev->origin, m_hEnemy->pev->origin - Vector( 0, 0, 80 ), ignore_monsters, ENT(pev), &tr );

			if ( tr.flFraction >= 1 )
			{
				m_fCanSpray = FALSE;
				return FALSE;
			}
		}

		m_fCanSpray = TRUE;
		return TRUE;
	}

	m_fCanSpray = FALSE;
	return FALSE;
}

void CWaterElemental :: Killed( entvars_t *pevAttacker, int iGib )
{
	if ( m_fGlowing )
	{
		PLAYBACK_EVENT_FULL( FEV_RELIABLE | FEV_GLOBAL, edict(), m_usRain, 0.0, (float *)&pev->origin, (float *)&pev->angles, 0.0, 0.0, 0, 0, 1, 0 );
		m_fGlowing = FALSE;
		STOP_SOUND( ENT(pev), CHAN_BODY, "ambience/sparks.wav" );
		iGib = GIB_ALWAYS;
	}

	pev->renderfx = kRenderFxNone;
	CBaseMonster::Killed( pevAttacker, iGib );
}

Schedule_t *CWaterElemental :: GetScheduleOfType ( int Type )
{
	if ( Type == SCHED_RANGE_ATTACK2 )
		return &slWaterPower[ 0 ];

	return CBaseMonster :: GetScheduleOfType( Type );
}

//=========================================================
// GibMonster - bursts into water blasts
//=========================================================
void CWaterElemental :: GibMonster( void )
{
	EMIT_SOUND( ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit1.wav", 1, ATTN_NORM );

	SpawnBlood( Center(), 241, 500 );	// 241: a palette colour of its own

	int cGibs = ssh_monstergibs.value;

	for ( int i = 0; i < cGibs; i++ )
	{
		CBaseEntity *pGib = CBaseEntity::Create( "water_blast", pev->origin, g_vecZero, edict() );

		pGib->pev->movetype = MOVETYPE_TOSS;

		// spawn the gib somewhere in the monster's bounding volume
		pGib->pev->origin.x = pev->absmin.x + pev->size.x * (RANDOM_FLOAT ( 0 , 1 ) );
		pGib->pev->origin.y = pev->absmin.y + pev->size.y * (RANDOM_FLOAT ( 0 , 1 ) );
		pGib->pev->origin.z = pev->absmin.z + pev->size.z * (RANDOM_FLOAT ( 0 , 1 ) ) + 1;	// absmin.z is in the floor because the engine subtracts 1 to enlarge the box

		// make the gib fly away from the attack vector
		pGib->pev->velocity = g_vecAttackDir * -1;

		// mix in some noise
		pGib->pev->velocity.x += RANDOM_FLOAT ( -0.25, 0.25 );
		pGib->pev->velocity.y += RANDOM_FLOAT ( -0.25, 0.25 );
		pGib->pev->velocity.z += RANDOM_FLOAT ( -0.25, 0.25 );

		pGib->pev->velocity = pGib->pev->velocity * RANDOM_FLOAT ( 500, 700 );
	}

	SetThink( SUB_Remove );
	pev->nextthink = gpGlobals->time;
}
