/***
*
*	Scientist Slaughterhouse: the cluster bomb.
*	The file name and the names marked "ours" are ours; class, entity and
*	exported member names come from the binary.
*
****/
/*

===== cluster_bomb.cpp ========================================================

  cluster_bomb (CClusterBomb, declared in weapons.h): the hand grenade's
  cluster bomb. It flies and bounces like a timed grenade; when it goes off
  it explodes and throws pev->frags bomblets of the kind pev->weapons
  picks: 0 timed grenades, 1 contact grenades, 2 hybrid grenades (contact
  on what takes damage), 3 the MIRV's shells, flung upwards.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "soundent.h"

LINK_ENTITY_TO_CLASS( cluster_bomb, CClusterBomb );

void CClusterBomb::ClusterDetonate( void )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	if ( pev->frags < 1 )
	{
		Detonate( );
		return;
	}

	vecSpot = pev->origin + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -40 ),  ignore_monsters, ENT(pev), & tr);

	pev->movedir = Vector( 0, 0, 1 );

	edict_t *pOwner = pev->owner;
	Explode( &tr, DMG_BLAST );
	pev->owner = pOwner;	// Explode forgets the owner; the bomblets need it

	ClusterExplode( &tr );
	SetThink( SUB_Remove );
}

void CClusterBomb::ClusterExplodeTouch( CBaseEntity *pOther )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	if ( pev->frags < 1 )
	{
		ExplodeTouch( pOther );
		return;
	}

	pev->movedir = pev->velocity.Normalize();

	vecSpot = pev->origin - pev->movedir * 32;
	UTIL_TraceLine( vecSpot, vecSpot + pev->movedir * 64, ignore_monsters, ENT(pev), &tr );

	edict_t *pOwner = pev->owner;
	Explode( &tr, DMG_BLAST );
	pev->owner = pOwner;

	ClusterExplode( &tr );
	SetThink( SUB_Remove );
}

void CClusterBomb::ClusterBounceTouch( CBaseEntity *pOther )
{
	// don't hit the guy that launched this grenade
	if ( pOther->edict() == pev->owner )
		return;

	// only do damage if we're moving fairly fast
	if (m_flNextAttack < gpGlobals->time && pev->velocity.Length() > 100)
	{
		entvars_t *pevOwner = VARS( pev->owner );
		if (pevOwner)
		{
			TraceResult tr = UTIL_GetGlobalTrace( );
			ClearMultiDamage( );
			pOther->TraceAttack(pevOwner, 1, gpGlobals->v_forward, &tr, DMG_CLUB ); 
			ApplyMultiDamage( pev, pevOwner);
		}
		m_flNextAttack = gpGlobals->time + 1.0; // debounce
	}

	Vector vecTestVelocity;
	// pev->avelocity = Vector (300, 300, 300);

	// this is my heuristic for modulating the grenade velocity because grenades dropped purely vertical
	// or thrown very far tend to slow down too quickly for me to always catch just by testing velocity. 
	// trimming the Z velocity a bit seems to help quite a bit.
	vecTestVelocity = pev->velocity; 
	vecTestVelocity.z *= 0.45;

	if ( !m_fRegisteredSound && vecTestVelocity.Length() <= 60 )
	{
		// grenade is moving really slow. It's probably very close to where it will ultimately stop moving. 
		// go ahead and emit the danger sound.
		CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin, 512, 1 );
		m_fRegisteredSound = TRUE;
	}

	if (pev->flags & FL_ONGROUND)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.8;
	}
	else
	{
		// play bounce sound
		BounceSound();
	}
}

void CClusterBomb::ClusterHybridThink( void )
{
	DangerSoundThink( );

	if ( pev->dmgtime <= gpGlobals->time )
		ClusterDetonate( );
}

void CClusterBomb::ClusterHybridTouch( CBaseEntity *pOther )
{
	if ( pOther->pev->takedamage )
		ClusterExplodeTouch( pOther );
	else
		ClusterBounceTouch( pOther );
}

void CClusterBomb::Spawn( void )
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->classname = MAKE_STRING( "cluster_bomb" );
	
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/grenade.mdl");
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));

	pev->dmg = 100;
	m_fRegisteredSound = FALSE;
}

void CClusterBomb::ClusterExplode( TraceResult *pTrace )
{
	entvars_t *pevOwner = NULL;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );

	Vector vecDir;
	CGrenade *pBomblet;

	for ( int i = 0; i < pev->frags; i++ )
	{
		switch ( pev->weapons )
		{
		case 1:
			UTIL_MakeVectors( UTIL_VecToAngles( -pev->movedir ) );
			vecDir = ClusterSpread( Vector( 1, 1, 1 ) );
			pBomblet = CGrenade::ShootContact( pevOwner, pev->origin + vecDir * 8, vecDir * RANDOM_FLOAT( 150, 300 ) );
			break;
		case 2:
			UTIL_MakeVectors( UTIL_VecToAngles( -pev->movedir ) );
			vecDir = ClusterSpread( Vector( 1, 1, 1 ) );
			pBomblet = CGrenade::ShootHybrid( pevOwner, pev->origin + vecDir * 8, vecDir * RANDOM_FLOAT( 150, 300 ), RANDOM_FLOAT( 2.5, 3.5 ) );
			break;
		case 3:
			// MIRV: shells flung upwards, solid after half a second
			vecDir = Vector( RANDOM_FLOAT( -0.25, 0.25 ), RANDOM_FLOAT( -0.25, 0.25 ), 1.5 );
			pBomblet = CGrenade::ShootContact( pevOwner, pev->origin + vecDir * 8, vecDir * RANDOM_FLOAT( 300, 400 ) );
			SET_MODEL( ENT( pBomblet->pev ), "models/shotgunshell.mdl" );
			pBomblet->pev->dmg = 25;
			pBomblet->pev->solid = SOLID_NOT;
			pBomblet->SetThink( MakeSolid );
			pBomblet->pev->nextthink = gpGlobals->time + 0.5;
			pBomblet->pev->spawnflags |= 512;
			break;
		default:
			UTIL_MakeVectors( UTIL_VecToAngles( -pev->movedir ) );
			vecDir = ClusterSpread( Vector( 1, 1, 1 ) );
			pBomblet = CGrenade::ShootTimed( pevOwner, pev->origin + vecDir * 8, vecDir * RANDOM_FLOAT( 150, 300 ), RANDOM_FLOAT( 2.5, 3.5 ) );
			break;
		}

		if ( pev->weapons < 3 )
			SET_MODEL( ENT( pBomblet->pev ), "models/bomblet.mdl" );
		UTIL_SetSize( pBomblet->pev, g_vecZero, g_vecZero );

		// spawnflag 1: smoke trails
		if ( pev->spawnflags & 1 )
			pBomblet->BeamFollow( g_sModelIndexSmokePuff, 15, 3, Vector( 224, 224, 255 ), 255 );

		pBomblet->m_iszKillDesc = m_iszKillDesc;
	}
}

Vector CClusterBomb::ClusterSpread( const Vector &vecSpread )
{
	float x = RANDOM_FLOAT( -1, 1 );
	float y = RANDOM_FLOAT( -1, 1 );
	float z = RANDOM_FLOAT( 0.5, 1 );

	return gpGlobals->v_forward * ( z * vecSpread.z ) + gpGlobals->v_right * ( x * vecSpread.x ) + gpGlobals->v_up * ( y * vecSpread.y );
}

void CClusterBomb::ClusterPreDetonate( void )
{
	CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin, 512, 1 );

	SetThink( ClusterDetonate );
	pev->nextthink = gpGlobals->time + 1;
}

CClusterBomb *CClusterBomb::ShootTimed( edict_t *pOwner, const Vector &vecStart, const Vector &vecVelocity, float time )
{
	CClusterBomb *pBomb = GetClassPtr( (CClusterBomb *)NULL );
	pBomb->Spawn();
	UTIL_SetOrigin( pBomb->pev, vecStart );
	pBomb->pev->velocity = vecVelocity;
	pBomb->pev->angles = g_vecZero;
	pBomb->pev->owner = pOwner;

	pBomb->SetTouch( ClusterBounceTouch );	// Bounce if touched

	pBomb->pev->dmgtime = gpGlobals->time + time;
	if ( time <= 1 )
	{
		pBomb->SetThink( ClusterDetonate );
		pBomb->pev->nextthink = gpGlobals->time + time;
	}
	else
	{
		// one second early, for the danger sound
		pBomb->SetThink( ClusterPreDetonate );
		pBomb->pev->nextthink = gpGlobals->time + time - 1;
	}

	pBomb->pev->avelocity.y = -75;

	pBomb->pev->gravity = 0.5;
	pBomb->pev->friction = 0.8;

	SET_MODEL( ENT( pBomb->pev ), "models/mirv_grenade.mdl" );
	pBomb->pev->dmg = gSkillData.plrDmgHandGrenade;
	pBomb->pev->weapons = 0;

	return pBomb;
}

CClusterBomb *CClusterBomb::ShootMirv( edict_t *pOwner, const Vector &vecStart, const Vector &vecVelocity, float time )
{
	CClusterBomb *pBomb = ShootTimed( pOwner, vecStart, vecVelocity, time );
	pBomb->pev->weapons = 3;
	return pBomb;
}
