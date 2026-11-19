/***
*
*	Scientist Slaughterhouse: the tesla coil monster.
*	The file name and the names marked "ours" are ours; class, field and
*	entity names come from the binary (save tables, exports).
*
****/
/*

===== teslamonster.cpp ========================================================

  monster_teslacoil (CTeslaMonster): a tower that charges up while it has
  no target, then fires three lightning bolts of a third of its charge
  each at the enemy it sees. Shock damage overcharges it.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "effects.h"
#include "explode.h"


#define SF_TESLA_AUTOSTART		32		// "Autostart" (names ours)
#define SF_TESLA_STARTINACTIVE	64		// "Start Inactive"
#define SF_TESLA_INVULNERABLE	128		// "Invulnerable"
#define SF_TESLA_ALLY			256		// not in the fgd: the coil is on the player's side

#define TESLA_RANGE				1024
#define TESLA_MAX_CHARGE		150

// where the charging sparks start, from the top of the coil (names ours)
const float TESLA_SPARK_Z = -25.5;
static Vector g_vecSparkRight( 17, 0, TESLA_SPARK_Z );
static Vector g_vecSparkLeft( -17, 0, TESLA_SPARK_Z );

class CTeslaMonster : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	int  Classify( void );
	int  BloodColor( void ) { return DONT_BLEED; }
	void GibMonster( void ) {}
	BOOL CanBeGrenaded( int fLauncher ) { return TRUE; }
	BOOL AllowsGrenades( void ) { return TRUE; }
	BOOL CanBeRepulsed( bool fForce ) { return FALSE; }
	BOOL CanBeTeleported( void ) { return FALSE; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return fGibbed ? "gibbed" : "electrocuted"; }
	const char *GetKillDesc( void ) { return "a tesla coil"; }
	int  GetEntityTriggerState( void ) { return m_on; }
	int  GetBattlePower( void );
	int  TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void Killed( entvars_t *pevAttacker, int iGib );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void EXPORT PopupThink( void );
	void EXPORT TurretUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT SearchThink( void );
	void EXPORT ChargeThink( void );
	void EXPORT FireThink( void );

	void AutoSearch( void );			// names ours
	Vector RandomSparkOffset( void );
	void UpdateGlow( void );
	static void Spark( const Vector &vecEnd, const Vector &vecSrc, const Vector &vecColor );

	float	m_flCurrHeight;		// the rings' bone controller
	float	m_flGoalHeight;
	BOOL	m_on;
	int		m_charge;			// ChargeThink's steps
	float	m_flLastSight;
	BOOL	m_iAutoStart;
	float	m_flPointOffset;	// the eyes' height with the rings down
	CSprite	*m_pGlow;			// name ours
};

LINK_ENTITY_TO_CLASS( monster_teslacoil, CTeslaMonster );

TYPEDESCRIPTION	CTeslaMonster::m_SaveData[] = 
{
	DEFINE_FIELD( CTeslaMonster, m_on, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTeslaMonster, m_iAutoStart, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTeslaMonster, m_flLastSight, FIELD_TIME ),
	DEFINE_FIELD( CTeslaMonster, m_flCurrHeight, FIELD_FLOAT ),
	DEFINE_FIELD( CTeslaMonster, m_flGoalHeight, FIELD_FLOAT ),
	DEFINE_FIELD( CTeslaMonster, m_charge, FIELD_INTEGER ),
	DEFINE_FIELD( CTeslaMonster, m_flPointOffset, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CTeslaMonster, CBaseMonster );


void CTeslaMonster::Spawn( void )
{
	Precache( );
	SET_MODEL( ENT(pev), "models/tesla.mdl" );

	pev->movetype		= MOVETYPE_FLY;
	pev->solid			= SOLID_SLIDEBOX;
	pev->takedamage		= DAMAGE_AIM;
	if ( pev->spawnflags & SF_TESLA_INVULNERABLE )
		pev->takedamage	= DAMAGE_YES;	// no autoaim; TakeDamage ignores the damage
	m_flFieldOfView		= VIEW_FIELD_FULL;
	SetBits( pev->flags, FL_MONSTER );
	pev->max_health = pev->health = 50;

	SetThink( SearchThink );
	SetUse( TurretUse );
	pev->nextthink = gpGlobals->time + 0.3;

	if ( m_flHeight < -32 )
		m_flHeight = -32;
	else if ( m_flHeight > 128 )
		m_flHeight = 128;

	SetBoneController( 0, 0 );
	GetAttachment( 0, pev->vuser1, pev->view_ofs );
	pev->view_ofs = pev->vuser1 - pev->origin;

	m_flPointOffset = pev->view_ofs.z;
	m_on = TRUE;

	if ( pev->spawnflags & ( SF_TESLA_AUTOSTART | SF_TESLA_STARTINACTIVE ) )
	{
		// down
		pev->sequence = 1;
		pev->frame = 255;
		UTIL_SetSize( pev, Vector( -16, -16, -53 ), Vector( 16, 16, 13 ) );
		m_on = FALSE;

		if ( HasSpawnFlag( SF_TESLA_STARTINACTIVE ) )
			pev->nextthink = 0;
		else
			m_iAutoStart = TRUE;
	}
	else
	{
		UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, m_flHeight + 69 ) );
		m_flCurrHeight = m_flGoalHeight = m_flHeight;
		SetBoneController( 0, m_flHeight );
	}

	pev->dmg = TESLA_MAX_CHARGE;
}

void CTeslaMonster::Precache( void )
{
	PRECACHE_MODEL( "models/tesla.mdl" );
	PRECACHE_MODEL( "sprites/glow04.spr" );
	PRECACHE_SOUND( "ambience/particle_suck1.wav" );
	PRECACHE_SOUND( "weapons/shock_discharge.wav" );
}

void CTeslaMonster::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "height" ) )
	{
		m_flHeight = atof( pkvd->szValue );
		if ( m_flHeight > 0 )
			m_flHeight *= 0.5;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

//=========================================================
// GetBattlePower - the target ID's rating of this monster
//=========================================================
int CTeslaMonster::GetBattlePower( void )
{
	float flCharge = TESLA_MAX_CHARGE;
	if ( pev->dmg >= TESLA_MAX_CHARGE )
		flCharge = pev->dmg;
	flCharge *= 0.625;
	return flCharge * ( pev->max_health * 0.1 );
}

//=========================================================
// PopupThink - the rings go up (framerate 1) or down (-1)
//=========================================================
void CTeslaMonster::PopupThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;
	StudioFrameAdvance( );

	if ( m_fSequenceFinished )
	{
		if ( pev->framerate < 0 )
		{
			pev->maxs.z = m_flHeight + 69;
			pev->mins.z = 0;
			m_flGoalHeight = m_flHeight;
			UTIL_SetSize( pev, pev->mins, pev->maxs );
			pev->sequence = 0;
			ResetSequenceInfo( );
			m_on = TRUE;
			m_flLastSight = gpGlobals->time;
			SetThink( SearchThink );
			return;
		}

		pev->maxs.z = 13;
		pev->mins.z = -53;
		UTIL_SetSize( pev, pev->mins, pev->maxs );
		pev->framerate = 0;
		m_on = FALSE;
		if ( !m_iAutoStart )
			pev->nextthink = 0;
		SetThink( SearchThink );
	}
}

void CTeslaMonster::TurretUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	bool fOn = m_on || m_iAutoStart;

	if ( !ShouldToggle( useType, fOn ) )
		return;

	if ( fOn )
	{
		m_hEnemy = NULL;
		m_iAutoStart = FALSE;
		m_on = FALSE;
		m_flGoalHeight = 0;
		return;
	}

	pev->nextthink = gpGlobals->time + 0.1;

	if ( pev->spawnflags & SF_TESLA_AUTOSTART )
	{
		m_iAutoStart = TRUE;
		return;
	}

	pev->sequence = 1;
	ResetSequenceInfo( );
	pev->frame = 255;
	pev->framerate = -1;
	SetThink( PopupThink );
	pev->nextthink = gpGlobals->time + 0.1;
}

int CTeslaMonster::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( pevAttacker == pev )
		return 0;

	if ( bitsDamageType & DMG_SHOCK )
	{
		// shock damage overcharges the coil: retail stores the sum, then
		// 700 twice, so any shock sets the charge to 700
		pev->dmg += flDamage;
		if ( pev->dmg > 700 );	// sic
			pev->dmg = 700;
		pev->dmg = 700;
		return 0;
	}

	if ( pev->spawnflags & SF_TESLA_INVULNERABLE )
		return 0;

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CTeslaMonster::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( m_pGlow )
		m_pGlow->Killed( pevAttacker, iGib );

	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DEAD;

	CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
	if ( pOwner )
		pOwner->DeathNotice( pev );

	// sic: three floats from pev->impacttime on, not the origin: the
	// explosion goes off at the map's origin
	Vector vecPos( &pev->impacttime );
	ExplosionCreate( vecPos, pev->angles, edict(), 65, TRUE );
	UTIL_Remove( this );
}

//=========================================================
// AutoSearch - an autostart coil that's down looks for
// an enemy and comes up when it sees one
//=========================================================
void CTeslaMonster::AutoSearch( void )
{
	Look( TESLA_RANGE );
	m_hEnemy = BestVisibleEnemy( );

	if ( m_hEnemy != NULL )
	{
		pev->sequence = 1;
		ResetSequenceInfo( );
		pev->frame = 255;
		pev->framerate = -1;
		SetThink( PopupThink );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CTeslaMonster::SearchThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ( pev->dmg < TESLA_MAX_CHARGE )
		pev->dmg += 2;

	if ( m_flCurrHeight < m_flGoalHeight )
	{
		m_flCurrHeight += 3;
		if ( m_flCurrHeight > m_flGoalHeight )
			m_flCurrHeight = m_flGoalHeight;
		SetBoneController( 0, m_flCurrHeight );
		pev->view_ofs.z = m_flCurrHeight * 2 + m_flPointOffset;
		return;
	}

	if ( m_flCurrHeight > m_flGoalHeight )
	{
		m_flCurrHeight -= 3;
		if ( m_flCurrHeight < m_flGoalHeight )
			m_flCurrHeight = m_flGoalHeight;
		SetBoneController( 0, m_flCurrHeight );
		pev->view_ofs.z = m_flCurrHeight * 2 + m_flPointOffset;
		return;
	}

	if ( !m_on )
	{
		if ( pev->sequence == 0 )
		{
			// the rings are down: retract the tower
			pev->sequence = 1;
			ResetSequenceInfo( );
			pev->frame = 0;
			SetThink( PopupThink );
			pev->nextthink = gpGlobals->time + 0.1;
			return;
		}

		if ( m_iAutoStart )
			AutoSearch( );
		return;
	}

	Look( TESLA_RANGE );
	m_hEnemy = BestVisibleEnemy( );

	if ( m_hEnemy != NULL )
	{
		m_flLastSight = gpGlobals->time;

		if ( pev->dmg >= TESLA_MAX_CHARGE )
		{
			// charge up: three shots of a third of the charge each
			pev->dmg_save = pev->dmg / 3;
			SetThink( ChargeThink );
			m_charge = 0;
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "ambience/particle_suck1.wav", 1, ATTN_NORM );

			Vector vecSrc = pev->origin + pev->view_ofs;

			CSprite *pSprite = CSprite::SpriteCreate( g_pModelNameLaser, vecSrc + g_vecSparkRight, FALSE );
			pSprite->pev->rendermode = kRenderTransAdd;
			pSprite->pev->movetype = MOVETYPE_NOCLIP;
			pSprite->pev->velocity = g_vecSparkRight * -0.45;
			pSprite->pev->velocity.x += 2;
			pSprite->SetThink( SUB_Remove );
			pSprite->pev->nextthink = gpGlobals->time + 2;
			int iStart = pSprite->entindex( );

			pSprite = CSprite::SpriteCreate( g_pModelNameLaser, vecSrc + g_vecSparkLeft, FALSE );
			pSprite->pev->rendermode = kRenderTransAdd;
			pSprite->pev->movetype = MOVETYPE_NOCLIP;
			pSprite->pev->velocity = g_vecSparkLeft * -0.45;
			pSprite->pev->velocity.x -= 2;
			pSprite->SetThink( SUB_Remove );
			pSprite->pev->nextthink = gpGlobals->time + 2;

			// a ring between the two sparks
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
				WRITE_BYTE( TE_BEAMRING );
				WRITE_SHORT( iStart );
				WRITE_SHORT( pSprite->entindex( ) );
				WRITE_SHORT( g_sModelIndexLightning );
				WRITE_BYTE( 0 );	// framestart
				WRITE_BYTE( 10 );	// framerate
				WRITE_BYTE( 20 );	// life
				WRITE_BYTE( 60 );	// width
				WRITE_BYTE( 128 );	// noise
				if ( pev->dmg_save >= 100 )
				{
					WRITE_BYTE( 255 );
					WRITE_BYTE( 255 );
					WRITE_BYTE( 128 );
				}
				else
				{
					WRITE_BYTE( 0 );
					WRITE_BYTE( 128 );
					WRITE_BYTE( 255 );
				}
				WRITE_BYTE( 255 );	// brightness
				WRITE_BYTE( 30 );	// speed
			MESSAGE_END( );

			UpdateGlow( );
		}
	}
	else if ( m_iAutoStart && pev->sequence == 0 && m_flLastSight + 20 < gpGlobals->time )
	{
		// nothing seen for 20 seconds: lower the rings, then retract
		m_on = FALSE;
		m_flGoalHeight = 0;
	}
}

void CTeslaMonster::ChargeThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ( m_charge >= 20 )
	{
		if ( pev->dmg_save >= 100 )
			pev->skin = 2;
		else
			pev->skin = 1;

		if ( m_pGlow )
		{
			m_pGlow->SetScale( 0.5 );
			m_pGlow->SetBrightness( 255 );
		}

		SetThink( FireThink );
		return;
	}

	if ( m_charge % 5 )
	{
		Vector vecSrc = pev->origin + pev->view_ofs;

		for ( int i = 0; i < 4; i++ )
		{
			int iColor = RANDOM_LONG( 0, 192 );
			Vector vecColor;

			if ( pev->dmg_save >= 100 )
				vecColor = Vector( 255, 255, iColor );
			else
				vecColor = Vector( RANDOM_LONG( 0, iColor ), iColor, 255 );

			Spark( vecSrc + RandomSparkOffset( ), vecSrc, vecColor );
		}
	}

	m_charge++;

	if ( m_pGlow )
	{
		m_pGlow->SetScale( m_charge * 0.025 );
		m_pGlow->SetBrightness( m_charge * 12.0f );
	}
}

//=========================================================
// Spark - a short random lightning bolt
//=========================================================
void CTeslaMonster::Spark( const Vector &vecEnd, const Vector &vecSrc, const Vector &vecColor )
{
	int iNoise = RANDOM_LONG( 0, 192 );	// sic: unused

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
		WRITE_BYTE( TE_BEAMPOINTS );
		WRITE_COORD( vecEnd.x );
		WRITE_COORD( vecEnd.y );
		WRITE_COORD( vecEnd.z );
		WRITE_COORD( vecSrc.x );
		WRITE_COORD( vecSrc.y );
		WRITE_COORD( vecSrc.z );
		WRITE_SHORT( g_sModelIndexLightning );
		WRITE_BYTE( 0 );	// framestart
		WRITE_BYTE( 10 );	// framerate
		WRITE_BYTE( RANDOM_LONG( 4, 6 ) );	// life
		WRITE_BYTE( 20 );	// width
		WRITE_BYTE( 128 );	// noise
		WRITE_BYTE( vecColor.x );
		WRITE_BYTE( vecColor.y );
		WRITE_BYTE( vecColor.z );
		WRITE_BYTE( 255 );	// brightness
		WRITE_BYTE( 30 );	// speed
	MESSAGE_END( );
}

void CTeslaMonster::FireThink( void )
{
	if ( m_hEnemy == NULL || !m_hEnemy->IsAlive( ) || FVisible( m_hEnemy ) )	// sic: keeps an enemy it can't see
	{
		Look( TESLA_RANGE );
		m_hEnemy = BestVisibleEnemy( );
	}

	pev->nextthink = gpGlobals->time + 0.5;
	m_flLastSight = gpGlobals->time;

	if ( m_hEnemy != NULL && pev->frags < 3 && pev->dmg >= pev->dmg_save )
	{
		TraceResult tr;
		Vector vecSrc( pev->origin + pev->view_ofs );
		Vector vecTarget = m_hEnemy->BodyTarget( vecSrc );

		UTIL_TraceLine( vecSrc, vecTarget, dont_ignore_monsters, ENT(pev), &tr );

		if ( RANDOM_LONG( 0, 1 ) )
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/electro4.wav", 1, ATTN_NORM );
		else
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/electro5.wav", 1, ATTN_NORM );
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/shock_discharge.wav", 1, ATTN_NORM );

		if ( tr.pHit )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
			if ( pEntity->pev->takedamage )
			{
				ClearMultiDamage( );
				pEntity->TraceAttack( pev, pev->dmg_save, ( tr.vecEndPos - vecSrc ).Normalize( ), &tr, DMG_SHOCK );
				ApplyMultiDamage( pev, pev );
			}
		}

		for ( int i = 0; i < 4; i++ )
		{
			CBeam *pBeam = CBeam::BeamCreate( "sprites/lgtning.spr", 100 );
			pBeam->PointsInit( tr.vecEndPos, vecSrc );
			pBeam->SetBrightness( 255 );
			pBeam->SetThink( SUB_Remove );
			pBeam->pev->nextthink = gpGlobals->time + 0.5;

			switch ( i )
			{
			case 0:
				if ( pev->dmg_save >= 100 )
					pBeam->SetColor( 255, 255, 128 );
				else
					pBeam->SetColor( 0, 128, 255 );
				pBeam->SetNoise( 48 );
				break;
			case 1:
				if ( pev->dmg_save >= 100 )
					pBeam->SetColor( 255, 255, 128 );
				else
					pBeam->SetColor( 128, 128, 255 );
				pBeam->SetNoise( 96 );
				pBeam->SetWidth( 50 );
				break;
			case 2:
				pBeam->SetColor( 255, 255, 255 );
				pBeam->SetNoise( 64 );
				pBeam->SetWidth( 30 );
				break;
			case 3:
				if ( pev->dmg_save >= 100 )
					pBeam->SetColor( 255, 255, 0 );
				else
					pBeam->SetColor( 0, 0, 255 );
				pBeam->SetNoise( 128 );
				pBeam->SetWidth( 30 );
				break;
			}
		}

		if ( pev->dmg_save >= 100 )
		{
			// an overcharged bolt explodes where it hits
			CGrenade *pGrenade = CGrenade::ShootExplosion( pev, tr.vecEndPos, 0, Vector( 0, 0, -1 ) );
			pGrenade->pev->dmg = pev->dmg_save * 0.5;
			pGrenade->m_iszKillDesc = MAKE_STRING( "an explosion" );
			pGrenade->pev->spawnflags |= 0x100;

			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
				WRITE_BYTE( TE_DLIGHT );
				WRITE_COORD( vecSrc.x );
				WRITE_COORD( vecSrc.y );
				WRITE_COORD( vecSrc.z );
				WRITE_BYTE( 50 );	// radius
				WRITE_BYTE( 255 );
				WRITE_BYTE( 255 );
				WRITE_BYTE( 64 );
				WRITE_BYTE( 10 );	// life
				WRITE_BYTE( 2 );	// decay
		}
		else
		{
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
				WRITE_BYTE( TE_DLIGHT );
				WRITE_COORD( vecSrc.x );
				WRITE_COORD( vecSrc.y );
				WRITE_COORD( vecSrc.z );
				WRITE_BYTE( 50 );	// radius
				WRITE_BYTE( 64 );
				WRITE_BYTE( 64 );
				WRITE_BYTE( 255 );
				WRITE_BYTE( 10 );	// life
				WRITE_BYTE( 2 );	// decay
		}
		MESSAGE_END( );

		pev->frags++;
		pev->dmg -= pev->dmg_save;
		return;
	}

	// done: back to charging
	if ( m_pGlow )
		m_pGlow->SetBrightness( 0 );
	pev->skin = 0;
	pev->dmg_save = 0;
	pev->frags = 0;
	SetThink( SearchThink );
}

//=========================================================
// RandomSparkOffset - a point on the ring around the top
//=========================================================
Vector CTeslaMonster::RandomSparkOffset( void )
{
	Vector vecForward, vecRight, vecUp;
	Vector vecAngles( 0, RANDOM_FLOAT( 0, 360 ), 0 );

	g_engfuncs.pfnAngleVectors( vecAngles, vecForward, vecRight, vecUp );

	vecForward = vecForward * 15;
	float z = vecForward.z;
	vecForward.z = z + TESLA_SPARK_Z;
	return vecForward;
}

int CTeslaMonster::Classify( void )
{
	if ( !m_on && !m_iAutoStart )
		return CLASS_NONE;

	return ( pev->spawnflags & SF_TESLA_ALLY ) ? CLASS_PLAYER_ALLY : CLASS_MACHINE;
}

//=========================================================
// UpdateGlow - the glow at the top, coloured by the charge
//=========================================================
void CTeslaMonster::UpdateGlow( void )
{
	if ( !m_pGlow )
		m_pGlow = CSprite::SpriteCreate( "sprites/glow04.spr", pev->origin, FALSE );

	m_pGlow->pev->spawnflags |= SF_SPRITE_TEMPORARY;

	if ( pev->dmg_save >= 100 )
		m_pGlow->SetTransparency( kRenderGlow, 255, 255, 128, 0, kRenderFxNoDissipation );
	else
		m_pGlow->SetTransparency( kRenderGlow, 128, 192, 255, 0, kRenderFxNoDissipation );

	m_pGlow->SetScale( 0.1 );
	m_pGlow->SetAttachment( edict( ), 1 );
}
