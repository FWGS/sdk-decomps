/***
*
*	Scientist Slaughterhouse: the volcano of level 4.
*	The file name and the names marked "ours" are ours; class, field and
*	entity names come from the binary (save tables, exports).
*
****/
/*

===== volcano.cpp ========================================================

  volcano_boulder (CBoulder) and env_volcano (CVolcano)

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"


//
// volcano_boulder: a bouncing rock that explodes as a grenade (100 damage, 150
// for the big body and the small glowing one) when it lands slowly or on a coin
// toss, or after 10 seconds. A quarter of them burn (a fire sprite every 0.1
// seconds), a quarter trail a beam, the rest fly plain.
//
class CBoulder : public CGrenade
{
public:
	void	Spawn( void );
	void	Precache( void );
	const char *GetKillDesc( void ) { return "a volcano boulder"; }

	void EXPORT BurnThink( void );
	void EXPORT BoulderTouch( CBaseEntity *pOther );

	int		m_iTrail;	// name ours: the trail model, g_sModelIndexFireExplosion when burning, 0 for none
};
LINK_ENTITY_TO_CLASS( volcano_boulder, CBoulder );


void CBoulder::Spawn( void )
{
	Precache( );

	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	pev->friction = 0.5;

	SET_MODEL( ENT( pev ), "models/vboulder.mdl" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	pev->body = RANDOM_LONG( 0, 2 );
	pev->skin = RANDOM_LONG( 0, 1 );

	pev->dmg = 100;
	if ( pev->body == 0 || ( pev->body == 1 && pev->skin == 1 ) )
		pev->dmg = 150;

	if ( pev->skin == 1 && RANDOM_FLOAT( 0, 1 ) > 0.75 )
	{
		// a glowing rock
		pev->renderfx = kRenderFxGlowShell;
		pev->rendercolor = Vector( 255, 144, 0 );
		pev->renderamt = 0;
		pev->spawnflags |= 1;
	}

	if ( m_iTrail == g_sModelIndexFireExplosion )
	{
		SetThink( &CBoulder::BurnThink );
		pev->nextthink = gpGlobals->time + 0.1;
		pev->dmgtime = gpGlobals->time + 10;
	}
	else if ( m_iTrail )
	{
		SetThink( &CGrenade::Detonate );
		pev->nextthink = gpGlobals->time + 10;

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() );	// entity
			WRITE_SHORT( m_iTrail );	// model
			WRITE_BYTE( 15 ); // life
			WRITE_BYTE( 5 );  // width
			WRITE_BYTE( 225 );   // r
			WRITE_BYTE( 255 );   // g
			WRITE_BYTE( 225 );   // b
			WRITE_BYTE( 200 );	// brightness
		MESSAGE_END();
	}
	else
	{
		SetThink( &CGrenade::Detonate );
		pev->nextthink = gpGlobals->time + 10;
	}

	SetTouch( &CBoulder::BoulderTouch );

	pev->avelocity.x = RANDOM_FLOAT( 0, 100 );
	pev->avelocity.y = RANDOM_FLOAT( 0, 100 );
}


void CBoulder::Precache( void )
{
	PRECACHE_MODEL( "models/vboulder.mdl" );
	m_iTrail = PRECACHE_MODEL( "sprites/VSlash.spr" );

	float flRand = RANDOM_FLOAT( 0, 1 );
	if ( flRand > 0.75 )
		m_iTrail = g_sModelIndexFireExplosion;
	else if ( flRand < 0.5 )
		m_iTrail = 0;
}


void CBoulder::BurnThink( void )
{
	if ( m_iTrail == g_sModelIndexFireExplosion && pev->dmgtime >= gpGlobals->time )
	{
		pev->nextthink = gpGlobals->time + 0.1;

		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SPRITE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( m_iTrail );
			WRITE_BYTE( 5 ); // scale * 10
			WRITE_BYTE( 255 ); // brightness
		MESSAGE_END();
	}
	else
		Detonate( );
}


void CBoulder::BoulderTouch( CBaseEntity *pOther )
{
	pev->velocity = pev->velocity * 0.5;

	if ( RANDOM_FLOAT( 0, 1 ) > 0.5 || pev->velocity.Length() < 32 )
		ExplodeTouch( pOther );
}


//
// env_volcano: shoots boulders and fireballs along its angles, every "delay"
// seconds while on, "m_flVelocity" fast, scattered by "m_flVariance" along
// each axis. "m_iRocks" limits a run (0: unlimited); a used run ends with the
// entity removed, or rearmed when "Repeatable" is set.
//
#define SF_VOLCANO_REPEATABLE	1	// name ours

class CVolcano : public CBaseDelay
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	KeyValue( KeyValueData *pkvd );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int		GetOnState( void ) { if ( pev->nextthink > 0 ) return 1; return 0; }

	void EXPORT ShootThink( void );

	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int		m_iRocks;			// rocks left in this run
	int		m_iRockCapacity;	// "m_iRocks"
	int		m_iRockMaterial;	// saved but never set or read: "weapons" picks the projectile
	float	m_flRockVelocity;	// "m_flVelocity"
	float	m_flVariance;		// "m_flVariance"
};
LINK_ENTITY_TO_CLASS( env_volcano, CVolcano );

TYPEDESCRIPTION	CVolcano::m_SaveData[] =
{
	DEFINE_FIELD( CVolcano, m_iRocks, FIELD_INTEGER ),
	DEFINE_FIELD( CVolcano, m_iRockCapacity, FIELD_INTEGER ),
	DEFINE_FIELD( CVolcano, m_iRockMaterial, FIELD_INTEGER ),
	DEFINE_FIELD( CVolcano, m_flRockVelocity, FIELD_FLOAT ),
	DEFINE_FIELD( CVolcano, m_flVariance, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CVolcano, CBaseDelay );


void CVolcano::Precache( void )
{
	UTIL_PrecacheOther( "volcano_boulder" );
	UTIL_PrecacheOther( "fireball" );
}


void CVolcano::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "m_iRocks" ) )
	{
		m_iRocks = m_iRockCapacity = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_flVelocity" ) )
	{
		m_flRockVelocity = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_flVariance" ) )
	{
		m_flVariance = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}


void CVolcano::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( m_iRockCapacity > 0 )
	{
		// a limited volcano always starts a run
		SetThink( &CVolcano::ShootThink );
		pev->nextthink = gpGlobals->time;
	}
	else if ( pev->nextthink != 0 )
	{
		pev->nextthink = 0;
	}
	else
	{
		SetThink( &CVolcano::ShootThink );
		pev->nextthink = gpGlobals->time;
	}
}


void CVolcano::Spawn( void )
{
	Precache( );

	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	if ( m_flDelay == 0 )
		m_flDelay = 0.1;

	SetMovedir( pev );
}


void CVolcano::ShootThink( void )
{
	pev->nextthink = gpGlobals->time + m_flDelay;

	Vector vecDir;
	vecDir = pev->movedir;
	vecDir = vecDir + gpGlobals->v_right * RANDOM_FLOAT( -1, 1 ) * m_flVariance;
	vecDir = vecDir + gpGlobals->v_forward * RANDOM_FLOAT( -1, 1 ) * m_flVariance;
	vecDir = vecDir + gpGlobals->v_up * RANDOM_FLOAT( -1, 1 ) * m_flVariance;
	vecDir = vecDir.Normalize();

	float flRand = RANDOM_FLOAT( 0, 1 );

	// "weapons": 0 mixed (one in five a fireball), 1 fireballs only, 2 rocks only
	if ( pev->weapons == 1 || ( pev->weapons == 0 && flRand > 0.8 ) )
	{
		CBaseEntity *pFire = CBaseEntity::Create( "fireball", pev->origin, g_vecZero, NULL );
		pFire->pev->movetype = MOVETYPE_TOSS;
		pFire->pev->gravity = 1;
		pFire->pev->velocity = vecDir * m_flRockVelocity;
		pFire->pev->vuser2 = pFire->pev->velocity;
	}
	else
	{
		CBaseEntity *pRock = CBaseEntity::Create( "volcano_boulder", pev->origin, g_vecZero, NULL );
		pRock->pev->velocity = vecDir * m_flRockVelocity;
		pRock->pev->avelocity.x = RANDOM_FLOAT( 0, 100 );
		pRock->pev->avelocity.y = RANDOM_FLOAT( 0, 150 );
	}

	if ( m_iRockCapacity )
	{
		if ( --m_iRocks <= 0 )
		{
			if ( pev->spawnflags & SF_VOLCANO_REPEATABLE )
			{
				m_iRocks = m_iRockCapacity;
				SetThink( NULL );
				pev->nextthink = 0;
			}
			else
			{
				SetThink( &CBaseEntity::SUB_Remove );
				pev->nextthink = gpGlobals->time;
			}
		}
	}
}
