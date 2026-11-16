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
// Hornets
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"hornet.h"
#include	"gamerules.h"


int iHornetTrail;
int iHornetPuff;

LINK_ENTITY_TO_CLASS( hornet, CHornet );

//=========================================================
// Save/Restore
//=========================================================
TYPEDESCRIPTION	CHornet::m_SaveData[] = 
{
	DEFINE_FIELD( CHornet, m_flStopAttack, FIELD_TIME ),
	DEFINE_FIELD( CHornet, m_iHornetType, FIELD_INTEGER ),
	DEFINE_FIELD( CHornet, m_flFlySpeed, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CHornet, CBaseMonster );

//=========================================================
// don't let hornets gib, ever.
//=========================================================
int CHornet :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// filter these bits a little.
	bitsDamageType &= ~ ( DMG_ALWAYSGIB );
	bitsDamageType |= DMG_NEVERGIB;

	return CBaseMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
//=========================================================
void CHornet :: Spawn( void )
{
	Precache();

	pev->movetype	= MOVETYPE_FLY;
	pev->solid		= SOLID_BBOX;
	pev->takedamage = DAMAGE_YES;
	pev->flags		|= FL_MONSTER;
	pev->health		= 1;// weak!
	
	if ( g_pGameRules->IsMultiplayer() )
	{
		// hornets don't live as long in multiplayer
		m_flStopAttack = gpGlobals->time + 3.5;
	}
	else
	{
		m_flStopAttack	= gpGlobals->time + 15.0;
	}

	m_flFieldOfView = 0.9; // +- 25 degrees

	// the type is the shooter's to set (pev->weapons); StartHornet takes it up
	m_iHornetType = HORNET_TYPE_NORMAL;
	pev->weapons = HORNET_TYPE_NORMAL;

	SET_MODEL(ENT( pev ), "models/hornet.mdl");
	UTIL_SetSize( pev, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ) );

	SetThink( StartHornet );

	edict_t *pSoundEnt = pev->owner;
	if ( !pSoundEnt )
		pSoundEnt = edict();

	switch (RANDOM_LONG(0,2))
	{
	case 0:	EMIT_SOUND( pSoundEnt, CHAN_WEAPON, "agrunt/ag_fire1.wav", 1, ATTN_NORM);	break;
	case 1:	EMIT_SOUND( pSoundEnt, CHAN_WEAPON, "agrunt/ag_fire2.wav", 1, ATTN_NORM);	break;
	case 2:	EMIT_SOUND( pSoundEnt, CHAN_WEAPON, "agrunt/ag_fire3.wav", 1, ATTN_NORM);	break;
	}

	if ( !FNullEnt(pev->owner) && (pev->owner->v.flags & FL_CLIENT) )
	{
		pev->dmg = gSkillData.plrDmgHornet;
	}
	else
	{
		// no real owner, or owner isn't a client. 
		pev->dmg = gSkillData.monDmgHornet;
	}
	
	pev->nextthink = gpGlobals->time + 0.1;
	ResetSequenceInfo( );
}


void CHornet :: Precache()
{
	PRECACHE_MODEL("models/hornet.mdl");

	PRECACHE_SOUND( "agrunt/ag_fire1.wav" );
	PRECACHE_SOUND( "agrunt/ag_fire2.wav" );
	PRECACHE_SOUND( "agrunt/ag_fire3.wav" );

	PRECACHE_SOUND( "hornet/ag_buzz1.wav" );
	PRECACHE_SOUND( "hornet/ag_buzz2.wav" );
	PRECACHE_SOUND( "hornet/ag_buzz3.wav" );

	PRECACHE_SOUND( "hornet/ag_hornethit1.wav" );
	PRECACHE_SOUND( "hornet/ag_hornethit2.wav" );
	PRECACHE_SOUND( "hornet/ag_hornethit3.wav" );

	iHornetPuff = PRECACHE_MODEL( "sprites/muz1.spr" );
	iHornetTrail = PRECACHE_MODEL("sprites/laserbeam.spr");
}	

//=========================================================
// hornets will never get mad at each other, no matter who the owner is.
//=========================================================
int CHornet::IRelationship ( CBaseEntity *pTarget )
{
	if ( pTarget->pev->modelindex == pev->modelindex )
	{
		return R_NO;
	}

	return CBaseMonster :: IRelationship( pTarget );
}

//=========================================================
// ID's Hornet as their owner
//=========================================================
int CHornet::Classify ( void )
{

	if ( pev->owner && pev->owner->v.flags & FL_CLIENT)
	{
		return CLASS_PLAYER_BIOWEAPON;
	}

	return	CLASS_ALIEN_BIOWEAPON;
}

//=========================================================
// StartHornet - starts a hornet out by its type
//=========================================================
void CHornet :: StartHornet ( void )
{
	m_iHornetType = pev->weapons;

	IgniteTrail();

	switch ( m_iHornetType )
	{
	case HORNET_TYPE_DART:
		// flies straight
		SetTouch( DartTouch );
		SetThink( FlyThink );
		m_flFlySpeed = 1200;
		pev->movedir = pev->velocity.Normalize();
		pev->velocity = pev->movedir * 1200;
		pev->enemy = pev->owner;
		break;
	case HORNET_TYPE_HEADSHOT:
		SetTouch( HeadshotTouch );
		SetThink( TrackTarget );
		m_flFlySpeed = 600;
		break;
	case HORNET_TYPE_BOUNCE:
		SetTouch( BounceTouch );
		SetThink( FlyThink );
		pev->movetype = MOVETYPE_BOUNCEMISSILE;
		pev->velocity = pev->velocity.Normalize() * 1000;
		m_flFlySpeed = 1000;
		break;
	case HORNET_TYPE_FIRE:
		SetTouch( TrackTouch );
		SetThink( TrackTarget );
		m_flFlySpeed = 700;
		break;
	case HORNET_TYPE_ICE:
		SetTouch( TrackTouch );
		SetThink( TrackTarget );
		m_flFlySpeed = 900;
		break;
	default:
		SetTouch( TrackTouch );
		SetThink( TrackTarget );
		m_flFlySpeed = 800;
		break;
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

void CHornet::IgniteTrail( void )
{
	// trail
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE(  TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() );	// entity
		WRITE_SHORT( iHornetTrail );	// model
		WRITE_BYTE( 10 ); // life
		WRITE_BYTE( 2 );  // width
		
		switch ( m_iHornetType )
		{
		case HORNET_TYPE_NORMAL:
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 0 );   // r, g, b
			WRITE_BYTE( 0 );   // r, g, b
			break;
		case HORNET_TYPE_DART:
			WRITE_BYTE( 0 );   // r, g, b
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 0 );   // r, g, b
			break;
		case HORNET_TYPE_HEADSHOT:
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 0 );   // r, g, b
			break;
		case HORNET_TYPE_BOUNCE:
			WRITE_BYTE( 0 );   // r, g, b
			WRITE_BYTE( 0 );   // r, g, b
			WRITE_BYTE( 255 );   // r, g, b
			break;
		case HORNET_TYPE_FIRE:
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 128 );   // r, g, b
			WRITE_BYTE( 0 );   // r, g, b
			break;
		case HORNET_TYPE_ICE:
			WRITE_BYTE( 128 );   // r, g, b
			WRITE_BYTE( 128 );   // r, g, b
			WRITE_BYTE( 255 );   // r, g, b
			break;
		}

		WRITE_BYTE( 128 );	// brightness

	MESSAGE_END();
}

//=========================================================
// Hornet is flying, gently tracking target
//=========================================================
void CHornet :: TrackTarget ( void )
{
	Vector	vecFlightDir;
	Vector	vecDirToEnemy;
	float	flDelta;

	StudioFrameAdvance( );

	if (gpGlobals->time > m_flStopAttack)
	{
		Expire();
		return;
	}

	// UNDONE: The player pointer should come back after returning from another level
	if ( m_hEnemy == NULL )
	{// enemy is dead.
		Look( 512 );
		m_hEnemy = BestVisibleEnemy( );
	}
	
	if ( m_hEnemy != NULL && FVisible( m_hEnemy ))
	{
		// the headshot hornet goes for a scientist's eyes
		if ( m_iHornetType == HORNET_TYPE_HEADSHOT && m_hEnemy->Classify() == CLASS_HUMAN_PASSIVE )
			m_vecEnemyLKP = m_hEnemy->EyePosition( );
		else
			m_vecEnemyLKP = m_hEnemy->BodyTarget( pev->origin );
	}
	else
	{
		m_vecEnemyLKP = m_vecEnemyLKP + pev->velocity * m_flFlySpeed * 0.1;
	}

	vecDirToEnemy = ( m_vecEnemyLKP - pev->origin ).Normalize();

	if (pev->velocity.Length() < 0.1)
		vecFlightDir = vecDirToEnemy;
	else 
		vecFlightDir = pev->velocity.Normalize();

	// measure how far the turn is, the wider the turn, the slow we'll go this time.
	flDelta = DotProduct ( vecFlightDir, vecDirToEnemy );
	
	if ( flDelta < 0.5 )
	{// hafta turn wide again. play sound
		switch (RANDOM_LONG(0,2))
		{
		case 0:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hornet/ag_buzz1.wav", HORNET_BUZZ_VOLUME, ATTN_NORM);	break;
		case 1:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hornet/ag_buzz2.wav", HORNET_BUZZ_VOLUME, ATTN_NORM);	break;
		case 2:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hornet/ag_buzz3.wav", HORNET_BUZZ_VOLUME, ATTN_NORM);	break;
		}
	}

	if ( flDelta <= 0 && m_iHornetType == HORNET_TYPE_HEADSHOT )
	{// no flying backwards, but we don't want to invert this, cause we'd go fast when we have to turn REAL far.
		flDelta = 0.25;
	}

	pev->velocity = ( vecFlightDir + vecDirToEnemy).Normalize();

	if ( pev->owner && !(pev->owner->v.flags & FL_CLIENT) && m_iHornetType != HORNET_TYPE_HEADSHOT )
	{
		// random pattern only applies to hornets fired by monsters, not players. 

		pev->velocity.x += RANDOM_FLOAT ( -0.10, 0.10 );// scramble the flight dir a bit.
		pev->velocity.y += RANDOM_FLOAT ( -0.10, 0.10 );
		pev->velocity.z += RANDOM_FLOAT ( -0.10, 0.10 );
	}
	
	switch ( m_iHornetType )
	{
		case HORNET_TYPE_HEADSHOT:
			pev->velocity = pev->velocity * ( m_flFlySpeed * flDelta );// scale the dir by the ( speed * width of turn )
			pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.1, 0.3 );
			break;
		default:
			pev->velocity = pev->velocity * m_flFlySpeed;// do not have to slow down to turn.
			pev->nextthink = gpGlobals->time + 0.1;// fixed think time
			break;
	}

	pev->angles = UTIL_VecToAngles (pev->velocity);

	pev->solid = SOLID_BBOX;

	// if hornet is close to the enemy, jet in a straight line for a half second.
	// (only in the single player game)
	if ( m_hEnemy != NULL && !g_pGameRules->IsMultiplayer() )
	{
		if ( flDelta >= 0.4 && ( pev->origin - m_vecEnemyLKP ).Length() <= 300 )
		{
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_SPRITE );
				WRITE_COORD( pev->origin.x);	// pos
				WRITE_COORD( pev->origin.y);
				WRITE_COORD( pev->origin.z);
				WRITE_SHORT( iHornetPuff );		// model
				// WRITE_BYTE( 0 );				// life * 10
				WRITE_BYTE( 2 );				// size * 10
				WRITE_BYTE( 128 );			// brightness
			MESSAGE_END();

			switch (RANDOM_LONG(0,2))
			{
			case 0:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hornet/ag_buzz1.wav", HORNET_BUZZ_VOLUME, ATTN_NORM);	break;
			case 1:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hornet/ag_buzz2.wav", HORNET_BUZZ_VOLUME, ATTN_NORM);	break;
			case 2:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hornet/ag_buzz3.wav", HORNET_BUZZ_VOLUME, ATTN_NORM);	break;
			}
			pev->velocity = pev->velocity * 2;
			pev->nextthink = gpGlobals->time + 1.0;
			// don't attack again
			m_flStopAttack = gpGlobals->time;
		}
	}
}

//=========================================================
// Tracking Hornet hit something
//=========================================================
void CHornet :: TrackTouch ( CBaseEntity *pOther )
{
	if ( pOther->edict() == pev->owner || pOther->pev->modelindex == pev->modelindex )
	{// bumped into the guy that shot it.
		pev->solid = SOLID_NOT;
		return;
	}

	if ( IRelationship( pOther ) <= R_NO )
	{
		// hit something we don't want to hurt, so turn around.

		pev->velocity = pev->velocity.Normalize();

		pev->velocity.x *= -1;
		pev->velocity.y *= -1;

		pev->origin = pev->origin + pev->velocity * 4; // bounce the hornet off a bit.
		pev->velocity = pev->velocity * m_flFlySpeed;

		return;
	}

	DieTouch( pOther );
}

//=========================================================
// the headshot hornet sticks to a scientist's head and blows it off
//=========================================================
void CHornet :: HeadshotTouch ( CBaseEntity *pOther )
{
	if ( pOther->edict() == pev->owner || pOther->pev->modelindex == pev->modelindex )
	{// bumped into the guy that shot it.
		pev->solid = SOLID_NOT;
		return;
	}

	if ( IRelationship( pOther ) <= R_NO )
	{
		// hit something we don't want to hurt, so turn around.

		pev->velocity = pev->velocity.Normalize();

		pev->velocity.x *= -1;
		pev->velocity.y *= -1;

		pev->origin = pev->origin + pev->velocity * 4; // bounce the hornet off a bit.
		pev->velocity = pev->velocity * m_flFlySpeed;

		return;
	}

	if ( FBitSet( pOther->pev->flags, FL_NOTARGET ) || !FClassnameIs( pOther->pev, "monster_scientist" ) )
	{
		DieTouch( pOther );
		return;
	}

	SetThink( HeadBlastThink );
	pev->origin = pOther->EyePosition( );
	pev->nextthink = gpGlobals->time + 2;
	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	pev->body = 0;
	pev->aiment = pOther->edict();
	pev->movetype = MOVETYPE_FOLLOW;
	m_hEnemy = pOther;
	pOther->pev->flags |= FL_NOTARGET;	// one hornet per head
	pev->effects = EF_NODRAW;
	SetTouch( NULL );

	switch (RANDOM_LONG(0,2))
	{
		case 0:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hornet/ag_hornethit1.wav", 1, ATTN_NORM);	break;
		case 1:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hornet/ag_hornethit2.wav", 1, ATTN_NORM);	break;
		case 2:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hornet/ag_hornethit3.wav", 1, ATTN_NORM);	break;
	}
}

//=========================================================
// the dart goes through what it hits
//=========================================================
void CHornet::DartTouch( CBaseEntity *pOther )
{
	if ( pOther->edict() == pev->owner || pOther->pev->modelindex == pev->modelindex )
	{// bumped into the guy that shot it.
		pev->solid = SOLID_NOT;
		return;
	}

	if ( pOther->IsBSPModel() || pOther->Shielded() )
	{
		DieTouch( pOther );
		return;
	}

	switch (RANDOM_LONG(0,2))
	{// buzz when you plug someone
		case 0:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hornet/ag_hornethit1.wav", 1, ATTN_NORM);	break;
		case 1:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hornet/ag_hornethit2.wav", 1, ATTN_NORM);	break;
		case 2:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hornet/ag_hornethit3.wav", 1, ATTN_NORM);	break;
	}

	pOther->TakeDamage( pev, VARS( pev->owner ), pev->dmg, DMG_BULLET );

	pev->solid = SOLID_NOT;

	// find the other side of what it hit
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + pev->movedir * 256, dont_ignore_monsters, ENT( pOther->pev ), &tr );
	Vector vecEnd = tr.vecEndPos;
	UTIL_TraceLine( vecEnd, pev->origin, dont_ignore_monsters, ENT( pev ), &tr );
	if ( tr.flFraction == 0.0 )
	{
		UTIL_Remove( this );
		return;
	}

	pev->origin = tr.vecEndPos + pev->movedir * 8;
	pev->velocity = pev->movedir * m_flFlySpeed;
}

void CHornet::BounceTouch( CBaseEntity *pOther )
{
	if ( pOther->edict() == pev->owner || pOther->pev->modelindex == pev->modelindex )
	{// bumped into the guy that shot it.
		pev->solid = SOLID_NOT;
		return;
	}

	if ( pOther->pev->takedamage )
		DieTouch( pOther );
}

void CHornet::DieTouch ( CBaseEntity *pOther )
{
	if ( pOther && pOther->pev->takedamage )
	{// do the damage

		switch (RANDOM_LONG(0,2))
		{// buzz when you plug someone
			case 0:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hornet/ag_hornethit1.wav", 1, ATTN_NORM);	break;
			case 1:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hornet/ag_hornethit2.wav", 1, ATTN_NORM);	break;
			case 2:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hornet/ag_hornethit3.wav", 1, ATTN_NORM);	break;
		}

		int bitsDamage = DMG_BULLET;
		if ( m_iHornetType == HORNET_TYPE_FIRE )
			bitsDamage = DMG_SLOWBURN | DMG_NEVERGIB;
		else if ( m_iHornetType == HORNET_TYPE_ICE )
			bitsDamage = DMG_FREEZE | DMG_NEVERGIB;
			
		pOther->TakeDamage( pev, VARS( pev->owner ), pev->dmg, bitsDamage );
	}

	pev->modelindex = 0;// so will disappear for the 0.1 secs we wait until NEXTTHINK gets rid
	pev->solid = SOLID_NOT;

	SetThink ( SUB_Remove );
	pev->nextthink = gpGlobals->time + 1;// stick around long enough for the sound to finish!
}

//=========================================================
// two seconds on a scientist's head
//=========================================================
void CHornet::HeadBlastThink( void )
{
	if ( m_hEnemy == NULL || m_hEnemy->pev->body >= 4 )
	{
		UTIL_Remove( this );
		return;
	}

	m_hEnemy->pev->body += 4;	// headless

	Vector vecOrigin, vecAngles;
	GET_ATTACHMENT( m_hEnemy->edict(), 0, vecOrigin, vecAngles );
	SpawnBlood( vecOrigin, BLOOD_COLOR_RED, 250 );

	entvars_t *pevOwner = VARS( pev->owner );
	m_hEnemy->TakeDamage( pev, pevOwner, m_hEnemy->pev->health, DMG_NEVERGIB );
	if ( m_hEnemy->pev->deadflag != DEAD_DEAD )
		PLAYBACK_EVENT_FULL( 0, m_hEnemy->edict(), g_iBloodSprayEvent, 0.0, pev->origin, pev->angles, 0.0, 0.0, 1, 0, 0, 0 );

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );
		WRITE_COORD( vecOrigin.x );
		WRITE_COORD( vecOrigin.y );
		WRITE_COORD( vecOrigin.z );
		if ( UTIL_PointContents( vecOrigin ) != CONTENTS_WATER )
		{
			WRITE_SHORT( g_sModelIndexFireball );
		}
		else
		{
			WRITE_SHORT( g_sModelIndexWExplosion );
		}
		WRITE_BYTE( 10 ); // scale * 10
		WRITE_BYTE( 15 ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NOPARTICLES );
	MESSAGE_END();

	UTIL_Remove( this );
}

//=========================================================
// the dart and the bouncer fly without tracking
//=========================================================
void CHornet::FlyThink( void )
{
	pev->angles = UTIL_VecToAngles( pev->velocity );

	if (gpGlobals->time > m_flStopAttack)
	{
		Expire();
		return;
	}

	if ( m_iHornetType == HORNET_TYPE_DART )
		pev->velocity = pev->movedir * m_flFlySpeed;

	pev->solid = SOLID_BBOX;
	pev->nextthink = gpGlobals->time + 0.1;
}

// out of time: drop to the ground, gone two seconds later
void CHornet::Expire( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->owner = NULL;
	SetTouch( DeadTouch );
	SetThink( SUB_Remove );
	pev->nextthink = gpGlobals->time + 2;
}

void CHornet::DeadTouch( CBaseEntity *pOther )
{
	SUB_Remove();
}
