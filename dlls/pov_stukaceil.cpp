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
// POV: the stukabat hanging from the ceiling
// (weapon_stukabat_onceiling). It fidgets until the player
// touches it, which gives him a stukabat, or shoots it:
// then, by its "stktype", it either attacks (a stkb_grenade
// drops from it) or falls down and becomes a stukabat
// lying on the ground. From the barnacle. File name and the
// names retail doesn't give are ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"player.h"
#include	"soundent.h"

#define SF_STKB_FELL	0x80000000	// POV, name ours: set on the stukabat a fallen bat leaves

class CStkbatCeil : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	int  Classify( void );
	int  IRelationship( CBaseEntity *pTarget );
	int  TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void EXPORT StkbatCeilTouch( CBaseEntity *pOther );
	void EXPORT StkbatCeilThink( void );
	void EXPORT StkbatFall( void );
	void EXPORT StkbatFallTouch( CBaseEntity *pOther );

	void StartFall( void );			// names ours
	void Attack( void );
	void PlaySound( int iSound );
	void StopSound( void );
	void FireSound( void );
	void DieSound( void );
	void LandSound( void );

	static const char *pIdleSounds[];	// names ours
	static const char *pFireSounds[];
	static const char *pDieSounds[];
	static const char *pLandSounds[];

	int m_iSoundPlayed;
	int m_iSequence;	// name ours: the current base sequence (idle, then falling)
	int m_iStkType;
};

LINK_ENTITY_TO_CLASS( weapon_stukabat_onceiling, CStkbatCeil );

TYPEDESCRIPTION	CStkbatCeil::m_SaveData[] =
{
	DEFINE_FIELD( CStkbatCeil, m_iStkType, FIELD_INTEGER ),
	DEFINE_FIELD( CStkbatCeil, m_iSoundPlayed, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CStkbatCeil, CBaseMonster );

const char *CStkbatCeil::pIdleSounds[] =
{
	"stukabat/stkb_idle1.wav",
	"stukabat/stkb_idletpick1.wav",
};

const char *CStkbatCeil::pFireSounds[] =
{
	"stukabat/stkb_fire1.wav",
	"stukabat/stkb_fire2.wav",
};

const char *CStkbatCeil::pDieSounds[] =
{
	"stukabat/stkb_die1.wav",
};

const char *CStkbatCeil::pLandSounds[] =
{
	"stukabat/stkb_idletpick2.wav",
};


void CStkbatCeil :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "stktype"))
	{
		if ( atoi( pkvd->szValue ) >= 0 && atoi( pkvd->szValue ) <= 4 )
			m_iStkType = atoi( pkvd->szValue );
		else
			m_iStkType = 0;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CStkbatCeil :: Classify ( void )
{
	return	CLASS_ALIEN_MONSTER;
}

// nobody cares about it
int CStkbatCeil :: IRelationship( CBaseEntity *pTarget )
{
	return R_NO;
}

//=========================================================
// Spawn
//=========================================================
void CStkbatCeil :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/stukabat.mdl");
	UTIL_SetSize( pev, Vector(-16, -16, -32), Vector(16, 16, 0) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_NONE;
	pev->takedamage		= DAMAGE_AIM;
	m_bloodColor		= BLOOD_COLOR_YELLOW;
	pev->effects		= EF_INVLIGHT; // take light from the ceiling
	pev->health			= 1000;
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	InitBoneControllers();

	SetActivity ( ACT_IDLE );

	pev->sequence = m_iSequence = LookupSequence( "Subtle_fidget" );
	ResetSequenceInfo( );

	SetTouch ( StkbatCeilTouch );
	SetThink ( StkbatCeilThink );
	pev->nextthink = gpGlobals->time + 0.1;

	UTIL_SetOrigin ( pev, pev->origin - Vector( 0, 0, 36 ) );
}

// any shot brings it down
int CStkbatCeil :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, 1, bitsDamageType );
}

// the player picks it off the ceiling
void CStkbatCeil :: StkbatCeilTouch( CBaseEntity *pOther )
{
	if ( pOther->IsPlayer() )
	{
		StopSound( );

		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_FindEntityByClassname( NULL, "player" );
		if ( pPlayer )
			pPlayer->GiveNamedItem( "weapon_stukabat" );
		else
			ALERT( at_console, "CStkbatCeil: ERROR - couldn't find player!!\n" );

		SetTouch( NULL );
		SetThink( NULL );
		UTIL_Remove( this );
	}
}

void CStkbatCeil :: StkbatCeilThink( void )
{
	StudioFrameAdvance( );

	if ( pev->health != 1000 )
	{
		// it was shot
		BOOL fAttack;

		switch ( m_iStkType )
		{
		case 1:		// attacks two times out of three
			fAttack = RANDOM_LONG( 0, 2 );
			if ( fAttack != 0 )
				fAttack = TRUE;
			break;
		case 2:		// attacks one time out of three
			fAttack = RANDOM_LONG( 0, 2 );
			if ( fAttack != 1 )
				fAttack = FALSE;
			break;
		case 3:		// always falls
			fAttack = FALSE;
			break;
		case 4:		// always attacks
			fAttack = TRUE;
			break;
		default:
			fAttack = RANDOM_LONG( 0, 1 );
			break;
		}

		StopSound( );

		if ( !fAttack )
			StartFall( );
		else
			Attack( );
		return;
	}

	if ( m_fSequenceFinished )
	{
		if ( pev->sequence != m_iSequence )
		{
			pev->sequence = m_iSequence;
		}
		else if ( RANDOM_LONG( 0, 3 ) == 0 )
		{
			if ( RANDOM_LONG( 1, 2 ) == 1 )
			{
				PlaySound( 0 );
				pev->sequence = m_iSequence + 1;
			}
			else
			{
				PlaySound( 1 );
				pev->sequence = m_iSequence + 2;
			}
		}
		ResetSequenceInfo( );
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

void CStkbatCeil :: StartFall( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->gravity = 0.3;
	pev->takedamage = DAMAGE_NO;

	pev->sequence = m_iSequence = LookupSequence( "fall_cycler" );
	ResetSequenceInfo( );

	SetThink( StkbatFall );
	SetTouch( StkbatFallTouch );

	DieSound( );

	pev->nextthink = gpGlobals->time + 0.1;
}

void CStkbatCeil :: StkbatFall( void )
{
	StudioFrameAdvance( );

	pev->avelocity = pev->avelocity * 1.02;

	pev->nextthink = gpGlobals->time + 0.2;
}

void CStkbatCeil :: StkbatFallTouch( CBaseEntity *pOther )
{
	if ( pOther->IsPlayer() )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_FindEntityByClassname( NULL, "player" );
		if ( pPlayer )
			pPlayer->GiveNamedItem( "weapon_stukabat" );
		else
			ALERT( at_console, "CStkbatCeil: ERROR - couldn't find player!!\n" );

		SetTouch( NULL );
		SetThink( NULL );
		UTIL_Remove( this );
		return;
	}

	if ( pOther->pev->solid != SOLID_BSP )
		return;

	// landed: leave a stukabat lying on the ground
	LandSound( );

	edict_t *pent = CREATE_NAMED_ENTITY( MAKE_STRING( "weapon_stukabat" ) );
	if ( FNullEnt( pent ) )
	{
		ALERT( at_console, "CStkbatCeil: couldn't create weapon_stukabat!\n" );
		return;
	}

	pent->v.origin = pev->origin;
	pent->v.targetname = ALLOC_STRING( "STKB122363" );
	pent->v.angles = pev->angles;
	pent->v.spawnflags |= SF_STKB_FELL;
	DispatchSpawn( pent->v.pContainingEntity );
	pent->v.owner = ENT( pev );

	SetTouch( NULL );
	SetThink( NULL );
	UTIL_Remove( this );
}

// a bat drops from it and goes for the enemy
void CStkbatCeil :: Attack( void )
{
	edict_t *pent = CREATE_NAMED_ENTITY( MAKE_STRING( "stkb_grenade" ) );
	if ( FNullEnt( pent ) )
	{
		ALERT( at_console, "CStkbatCeil: couldn't create stkb_grenade!\n" );
		return;
	}

	pent->v.origin = pev->origin - Vector( 0, 0, 36 );
	pent->v.angles = pev->angles - Vector( 0, 0, 15 );
	DispatchSpawn( pent->v.pContainingEntity );
	pent->v.owner = ENT( pev );

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, pFireSounds[ RANDOM_LONG(0,ARRAYSIZE(pFireSounds)-1) ], 0.9, ATTN_NORM, 0, PITCH_NORM );

	SetThink( NULL );
	UTIL_Remove( this );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CStkbatCeil :: Precache()
{
	PRECACHE_MODEL("models/stukabat.mdl");
	UTIL_PrecacheOther( "stkb_grenade" );

	PRECACHE_SOUND_ARRAY( pFireSounds );
	PRECACHE_SOUND_ARRAY( pDieSounds );
	PRECACHE_SOUND_ARRAY( pLandSounds );
	PRECACHE_SOUND_ARRAY( pIdleSounds );

	m_iSequence = LookupSequence( "Subtle_fidget" );
}

void CStkbatCeil :: PlaySound( int iSound )
{
	StopSound( );

	m_iSoundPlayed = iSound;

	if ( iSound < 0 || iSound > 1 )
	{
		ALERT( at_console, "StartVModelSound: no %d sound to play!!\n", iSound );
		m_iSoundPlayed = -1;
	}

	if ( m_iSoundPlayed != -1 )
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pIdleSounds[m_iSoundPlayed], 0.6, ATTN_IDLE, 0, PITCH_NORM );
}

void CStkbatCeil :: StopSound( void )
{
	if ( m_iSoundPlayed != -1 )
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pIdleSounds[m_iSoundPlayed], 0.6, ATTN_IDLE, SND_STOP, PITCH_NORM );
}

void CStkbatCeil :: FireSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, pFireSounds[ RANDOM_LONG(0,ARRAYSIZE(pFireSounds)-1) ], 0.6, ATTN_IDLE, 0, RANDOM_LONG(98,100) );
	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 600, 3.0 );
}

void CStkbatCeil :: DieSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, pDieSounds[ RANDOM_LONG(0,ARRAYSIZE(pDieSounds)-1) ], 0.6, ATTN_IDLE, 0, RANDOM_LONG(98,100) );
	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 600, 3.0 );
}

void CStkbatCeil :: LandSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, pLandSounds[ RANDOM_LONG(0,ARRAYSIZE(pLandSounds)-1) ], 0.6, ATTN_IDLE, 0, RANDOM_LONG(98,100) );
	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 600, 3.0 );
}
