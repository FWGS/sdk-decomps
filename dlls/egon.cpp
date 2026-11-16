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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "effects.h"
#include "customentity.h"
#include "gamerules.h"

#define	EGON_PRIMARY_VOLUME		450
#define EGON_BEAM_SPRITE		"sprites/xbeam1.spr"
#define EGON_FLARE_SPRITE		"sprites/XSpark1.spr"
#define EGON_SOUND_OFF			"weapons/egon_off1.wav"
#define EGON_SOUND_RUN			"weapons/egon_run3.wav"
#define EGON_SOUND_STARTUP		"weapons/egon_windup2.wav"
#define EGON_AMP_SPRITE			"sprites/amp.spr"			// names ours from here
#define EGON_ION_SPRITE			"sprites/ionstream3.spr"
#define EGON_ION_FLARE_SPRITE	"sprites/thunderball.spr"

#define EGON_SWITCH_NARROW_TIME			0.75			// Time it takes to switch fire modes
#define EGON_SWITCH_WIDE_TIME			1.5

enum egon_e {
	EGON_IDLE1 = 0,
	EGON_FIDGET1,
	EGON_ALTFIREON,
	EGON_ALTFIRECYCLE,
	EGON_ALTFIREOFF,
	EGON_FIRE1,
	EGON_FIRE2,
	EGON_FIRE3,
	EGON_FIRE4,
	EGON_DRAW,
	EGON_HOLSTER
};

// the ion discharge cannon's beam ends in front of the player: x along v_right,
// z along v_forward; the main beam first, then the pairs (name ours)
static Vector s_vecIonBeamOfs[5] =
{
	Vector( 0, 0, 16 ),
	Vector( 8, 0, 12 ),
	Vector( -8, 0, 12 ),
	Vector( 16, 0, 8 ),
	Vector( -16, 0, 8 ),
};

LINK_ENTITY_TO_CLASS( weapon_egon, CEgon );

void CEgon::Spawn( )
{
	Precache( );
	m_iId = WEAPON_EGON;
	SET_MODEL(ENT(pev), "models/w_egon.mdl");

	m_iDefaultAmmo = EGON_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CEgon::Precache( void )
{
	PRECACHE_MODEL("models/w_egon.mdl");
	PRECACHE_MODEL("models/v_egon.mdl");
	PRECACHE_MODEL("models/p_egon.mdl");

	PRECACHE_MODEL("models/w_9mmclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND( EGON_SOUND_OFF );
	PRECACHE_SOUND( EGON_SOUND_RUN );
	PRECACHE_SOUND( EGON_SOUND_STARTUP );

	PRECACHE_MODEL( EGON_BEAM_SPRITE );
	PRECACHE_MODEL( EGON_FLARE_SPRITE );
	PRECACHE_MODEL( EGON_AMP_SPRITE );
	PRECACHE_MODEL( EGON_ION_SPRITE );
	PRECACHE_MODEL( EGON_ION_FLARE_SPRITE );

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usEgonFire = PRECACHE_EVENT ( 1, "events/egon_fire.sc" );
	m_usEgonStop = PRECACHE_EVENT ( 1, "events/egon_stop.sc" );
}


BOOL CEgon::Deploy( void )
{
	m_deployed = FALSE;
	m_fireState = FIRE_OFF;
	return DefaultDeploy( "models/v_egon.mdl", "models/p_egon.mdl", EGON_DRAW, "egon" );
}

int CEgon::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}



void CEgon::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( EGON_HOLSTER );

    EndAttack();
}

int CEgon::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "uranium";
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 5;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_EGON;
	p->iFlags = 0;
	p->iWeight = EGON_WEIGHT;

	return 1;
}

#define EGON_PULSE_INTERVAL			0.1
#define EGON_DISCHARGE_INTERVAL		0.1

float CEgon::GetPulseInterval( void )
{
	return EGON_PULSE_INTERVAL;
}

float CEgon::GetDischargeInterval( void )
{
	return EGON_DISCHARGE_INTERVAL;
}

BOOL CEgon::HasAmmo( void )
{
	if ( m_pPlayer->ammo_uranium <= 0 )
		return FALSE;

	return TRUE;
}

void CEgon::UseAmmo( int count )
{
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= count )
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= count;
	else
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = 0;
}

void CEgon::Attack( void )
{
	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		
		if ( m_fireState != FIRE_OFF || m_pBeam )
		{
			EndAttack();
		}
		else
		{
			PlayEmptySound( );
		}
		return;
	}

	Vector vecAiming;
	Vector vecSrc;

	// pev->weapons is the firing mode: 0 gluon gun, 1 antimatter pulverizer, 2 ion discharge cannon
	if ( pev->weapons != 2 )	// the ion cannon aims its own beams
	{
		UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
		vecAiming = gpGlobals->v_forward;
		vecSrc	 = m_pPlayer->GetGunPosition( );
	}

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	switch( m_fireState )
	{
		case FIRE_OFF:
		{
			if ( !HasAmmo() )
			{
				m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.25;
				PlayEmptySound( );
				return;
			}

			m_flAmmoUseTime = gpGlobals->time;// start using ammo ASAP.

			PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usEgonFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, m_fireState, pev->weapons, 1, 0 );
						
			m_shakeTime = 0;

			m_pPlayer->m_iWeaponVolume = EGON_PRIMARY_VOLUME;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
			pev->fuser1	= UTIL_WeaponTimeBase() + 2;

			pev->dmgtime = gpGlobals->time + GetPulseInterval();
			m_fireState = FIRE_CHARGE;
		}
		break;

		case FIRE_CHARGE:
		{
			if ( pev->weapons == 2 )
				IonCannon();
			else
				Fire( vecSrc, vecAiming );
			m_pPlayer->m_iWeaponVolume = EGON_PRIMARY_VOLUME;
		
			if ( pev->fuser1 <= UTIL_WeaponTimeBase() )
			{
				PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usEgonFire, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, m_fireState, pev->weapons, 0, 0 );
				pev->fuser1 = 1000;
			}

			if ( !HasAmmo() )
			{
				EndAttack();
				m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
			}

		}
		break;
	}
}

void CEgon::PrimaryAttack( void )
{
	Attack();
}

// the ion discharge cannon: 1, 3 or 5 beams fanned out 15 degrees apart that lock on
// to what they hit and follow it while it stays in front of the player
void CEgon::IonCannon( void )
{
#ifndef CLIENT_DLL
	TraceResult tr;

	UpdateEffect( g_vecZero, g_vecZero, 0 );

	CBeam *pBeam = m_pBeam;
	float flSpread = 15;

	for ( int i = 0; i < m_iBeamCount * 2 + 1; i++ )
	{
		Vector vecSrc = pBeam->GetEndPos();
		Vector vecAngles = m_pPlayer->pev->v_angle;

		if ( i % 2 )
			vecAngles.y -= min( i, 2 ) * flSpread;
		else if ( i )
			vecAngles.y += (i * flSpread) / 2;

		UTIL_MakeVectors( vecAngles );
		Vector vecDir = gpGlobals->v_forward * 2048;

		if ( m_hIonTargets[i] )
		{
			Vector vecTarget = m_hIonTargets[i]->pev->origin + m_vecIonTargetOfs[i];
			Vector vecToTarget = (vecTarget - vecSrc).Normalize();

			if ( DotProduct( vecToTarget, gpGlobals->v_forward ) < 0 )
				m_hIonTargets[i] = NULL;	// behind the player: let go
			else
				vecDir = vecToTarget * 2048;
		}

		UTIL_TraceLine( vecSrc, vecSrc + vecDir, dont_ignore_monsters, m_pPlayer->edict(), &tr );

		if ( tr.fAllSolid )
			continue;

		pBeam->SetStartPos( tr.vecEndPos );
		UTIL_SetOrigin( m_pIonSprites[i*2+1]->pev, tr.vecEndPos );

		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		if ( pEntity == NULL )
			continue;

		if ( pev->dmgtime < gpGlobals->time )
		{
			m_pIonSprites[i*2]->pev->frame = RANDOM_LONG( 0, m_pIonSprites[i*2]->Frames() );
			m_pIonSprites[i*2+1]->pev->frame = RANDOM_LONG( 0, m_pIonSprites[i*2+1]->Frames() );

			if ( pEntity->pev->takedamage )
			{
				m_hIonTargets[i] = pEntity;
				m_vecIonTargetOfs[i] = tr.vecEndPos - pEntity->pev->origin;

				ClearMultiDamage();
				pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgEgonNarrow, vecDir.Normalize(), &tr, DMG_SHOCK | DMG_NEVERGIB );
				ApplyMultiDamage( pev, m_pPlayer->pev );
			}
			else
			{
				m_hIonTargets[i] = NULL;
			}
		}

		// sic: the next beam only when this one hit an entity
		if ( i < 4 )
			pBeam = m_pIonBeams[i];
	}

	if ( pev->dmgtime < gpGlobals->time )
		pev->dmgtime = gpGlobals->time + GetPulseInterval();

	// 3 ammo a second per beam pair
	if ( gpGlobals->time >= m_flAmmoUseTime )
	{
		UseAmmo( 1 );
		m_flAmmoUseTime = gpGlobals->time + 1 / ((m_iBeamCount + 1.0) * 3);
	}
#endif
}

void CEgon::Fire( const Vector &vecOrigSrc, const Vector &vecDir )
{
#ifndef CLIENT_DLL
	Vector vecDest = vecOrigSrc + vecDir * 2048;
	edict_t		*pentIgnore;
	TraceResult tr;

	pentIgnore = m_pPlayer->edict();
	Vector tmpSrc = vecOrigSrc + gpGlobals->v_up * -8 + gpGlobals->v_right * 3;

	// ALERT( at_console, "." );
	
	UTIL_TraceLine( vecOrigSrc, vecDest, dont_ignore_monsters, pentIgnore, &tr );

	if (tr.fAllSolid)
		return;

	CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

	if (pEntity == NULL)
		return;

	switch ( pev->weapons )
	{
	case 1:	// antimatter pulverizer
		if ( pev->dmgtime < gpGlobals->time )
		{
			ClearMultiDamage();
			if (pEntity->pev->takedamage)
			{
				pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgEgonNarrow, vecDir, &tr, DMG_ENERGYBEAM );
			}
			ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);

			// every other ammo point leaves an explosion where the beam hits
			if ( !(m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] % 2) )
			{
				CGrenade *pBlast = CGrenade::ShootExplosion( m_pPlayer->pev, tr.vecEndPos, 0, Vector( 0, 0, -1 ) );
				pBlast->m_iszKillDesc = MAKE_STRING( GetKillDesc() );
				pBlast->pev->dmg = 75;
			}

			// 10 ammo a second
			if ( gpGlobals->time >= m_flAmmoUseTime )
			{
				UseAmmo( 1 );
				m_flAmmoUseTime = gpGlobals->time + 0.1;
			}

			pev->dmgtime = gpGlobals->time + GetPulseInterval();
		}
		break;

	default:	// gluon gun: the SDK's wide mode, radius damage in single play too
		if ( pev->dmgtime < gpGlobals->time )
		{
			// wide mode does damage to the ent, and radius damage
			ClearMultiDamage();
			if (pEntity->pev->takedamage)
			{
				pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgEgonWide, vecDir, &tr, DMG_ENERGYBEAM | DMG_ALWAYSGIB);
			}
			ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);

			::RadiusDamage( tr.vecEndPos, pev, m_pPlayer->pev, gSkillData.plrDmgEgonWide/4, 128, CLASS_NONE, DMG_ENERGYBEAM | DMG_BLAST | DMG_ALWAYSGIB );

			if ( !m_pPlayer->IsAlive() )
				return;

			// 5 ammo a second
			if ( gpGlobals->time >= m_flAmmoUseTime )
			{
				UseAmmo( 1 );
				m_flAmmoUseTime = gpGlobals->time + 0.2;
			}

			pev->dmgtime = gpGlobals->time + GetDischargeInterval();
			if ( m_shakeTime < gpGlobals->time )
			{
				UTIL_ScreenShake( tr.vecEndPos, 5.0, 150.0, 0.75, 250.0 );
				m_shakeTime = gpGlobals->time + 1.5;
			}
		}
		break;
	}

	UpdateEffect( tmpSrc, tr.vecEndPos, 0 );
#endif
}


void CEgon::UpdateEffect( const Vector &startPoint, const Vector &endPoint, float timeBlend )
{
#ifndef CLIENT_DLL
	if ( !m_pBeam )
	{
		CreateEffect();
	}

	if ( pev->weapons == 2 )
	{
		// the ion beams end in front of the player, flares on their ends
		UTIL_MakeVectors( m_pPlayer->pev->angles );

		m_pBeam->SetEndPos( m_pPlayer->pev->origin + Vector( 0, 0, 10 ) + gpGlobals->v_forward * s_vecIonBeamOfs[0].z );
		UTIL_SetOrigin( m_pIonSprites[0]->pev, m_pBeam->GetEndPos() );

		if ( m_iBeamCount )
		{
			for ( int i = 0; i < m_iBeamCount * 2; i++ )
			{
				m_pIonBeams[i]->SetEndPos( m_pPlayer->pev->origin + Vector( 0, 0, 10 ) + gpGlobals->v_forward * s_vecIonBeamOfs[i+1].z + gpGlobals->v_right * s_vecIonBeamOfs[i+1].x );
				UTIL_SetOrigin( m_pIonSprites[i*2+2]->pev, m_pIonBeams[i]->GetEndPos() );
			}
		}
	}
	else
	{
		m_pBeam->SetStartPos( endPoint );

		if ( pev->weapons == 0 )
		{
			m_pBeam->pev->rendercolor.z = 144 + 80*fabs(sin(gpGlobals->time*10));

			UTIL_SetOrigin( m_pSprite->pev, endPoint );
			m_pSprite->pev->frame += 8 * gpGlobals->frametime;
			if ( m_pSprite->pev->frame > m_pSprite->Frames() )
				m_pSprite->pev->frame = 0;

			m_pNoise->SetStartPos( endPoint );
		}
	}
#endif
}

void CEgon::CreateEffect( void )
{

#ifndef CLIENT_DLL
	DestroyEffect();

	if ( pev->weapons == 1 )
	{
		// antimatter pulverizer: one white beam
		m_pBeam = CBeam::BeamCreate( EGON_AMP_SPRITE, 40 );
		m_pBeam->PointEntInit( pev->origin, m_pPlayer->entindex() );
		m_pBeam->SetScrollRate( 24 );
		m_pBeam->SetBrightness( 255 );
		m_pBeam->SetColor( 255, 255, 255 );
		m_pBeam->SetEndAttachment( 1 );
		m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;	// Flag these to be destroyed on save/restore or level transition
		m_pBeam->pev->flags |= FL_SKIPLOCALHOST;
		m_pBeam->pev->owner = m_pPlayer->edict();
	}
	else if ( pev->weapons == 2 )
	{
		// ion discharge cannon: 1 + 2 * m_iBeamCount beams, a flare on each end
		int i;

		m_pBeam = CBeam::BeamCreate( EGON_ION_SPRITE, 100 );
		m_pBeam->PointsInit( pev->origin, pev->origin );
		m_pBeam->SetScrollRate( 24 );
		m_pBeam->SetBrightness( 255 );
		m_pBeam->SetColor( 255, 255, 255 );
		m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;

		if ( m_iBeamCount )
		{
			for ( i = 0; i < m_iBeamCount * 2; i++ )
			{
				m_pIonBeams[i] = CBeam::BeamCreate( EGON_ION_SPRITE, 80 );
				m_pIonBeams[i]->PointsInit( pev->origin, pev->origin );
				m_pIonBeams[i]->SetScrollRate( 24 );
				m_pIonBeams[i]->SetBrightness( 255 );
				m_pIonBeams[i]->SetColor( 255, 255, 255 );
				m_pIonBeams[i]->pev->spawnflags |= SF_BEAM_TEMPORARY;
			}
		}

		for ( i = 0; i < m_iBeamCount * 4 + 2; i++ )
		{
			m_pIonSprites[i] = CSprite::SpriteCreate( EGON_ION_FLARE_SPRITE, pev->origin, FALSE );
			m_pIonSprites[i]->pev->scale = (i % 2) ? 0.5 : 0.2;
			m_pIonSprites[i]->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
			m_pIonSprites[i]->pev->spawnflags |= SF_SPRITE_TEMPORARY;
		}
	}
	else
	{
		// gluon gun: the SDK's wide beam
		m_pBeam = CBeam::BeamCreate( EGON_BEAM_SPRITE, 40 );
		m_pBeam->PointEntInit( pev->origin, m_pPlayer->entindex() );
		m_pBeam->SetFlags( BEAM_FSINE );
		m_pBeam->SetEndAttachment( 1 );
		m_pBeam->SetBrightness( 100 );
		m_pBeam->SetColor( 80, 90, 144 );
		m_pBeam->SetScrollRate( 50 );
		m_pBeam->SetNoise( 20 );
		m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;	// Flag these to be destroyed on save/restore or level transition
		m_pBeam->pev->flags |= FL_SKIPLOCALHOST;
		m_pBeam->pev->owner = m_pPlayer->edict();

		m_pNoise = CBeam::BeamCreate( EGON_BEAM_SPRITE, 55 );
		m_pNoise->PointEntInit( pev->origin, m_pPlayer->entindex() );
		m_pNoise->SetScrollRate( 25 );
		m_pNoise->SetBrightness( 100 );
		m_pNoise->SetColor( 80, 90, 255 );
		m_pNoise->SetNoise( 8 );
		m_pNoise->SetEndAttachment( 1 );
		m_pNoise->pev->spawnflags |= SF_BEAM_TEMPORARY;
		m_pNoise->pev->flags |= FL_SKIPLOCALHOST;
		m_pNoise->pev->owner = m_pPlayer->edict();

		m_pSprite = CSprite::SpriteCreate( EGON_FLARE_SPRITE, pev->origin, FALSE );
		m_pSprite->pev->scale = 1.0;
		m_pSprite->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
		m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;
		m_pSprite->pev->flags |= FL_SKIPLOCALHOST;
		m_pSprite->pev->owner = m_pPlayer->edict();
	}
#endif

}


void CEgon::DestroyEffect( void )
{

#ifndef CLIENT_DLL
	int i;

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
	if ( m_pSprite )
	{
		m_pSprite->Expand( 10, 500 );
		m_pSprite = NULL;
	}
	for ( i = 0; i < 4; i++ )
	{
		if ( m_pIonBeams[i] )
		{
			UTIL_Remove( m_pIonBeams[i] );
			m_pIonBeams[i] = NULL;
		}
	}
	for ( i = 0; i < 10; i++ )
	{
		if ( m_pIonSprites[i] )
		{
			UTIL_Remove( m_pIonSprites[i] );
			m_pIonSprites[i] = NULL;
		}
	}
#endif

}


// the ion cannon: cycle 1, 3 and 5 beams
void CEgon::SecondaryAttack( void )
{
	if ( pev->weapons == 2 )
	{
		if ( ++m_iBeamCount > 2 )
			m_iBeamCount = 0;

		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;

		if ( m_fireState != FIRE_OFF )
			CreateEffect();
	}
}

// +attack3: cycle the firing modes, not while firing
void CEgon::WeaponSpecial( void )
{
	if ( m_fireState == FIRE_OFF )
	{
		pev->weapons++;
		if ( pev->weapons > 2 )
			pev->weapons = 0;

		m_flNextWeaponSpecial = UTIL_WeaponTimeBase() + 1.0;

#ifndef CLIENT_DLL
		const char *pszMode;

		switch ( pev->weapons )
		{
		case 1:		pszMode = "Antimatter Pulverizer"; break;
		case 2:		pszMode = "Ion Discharge Cannon"; break;
		default:	pszMode = "Gluon Gun"; break;
		}

		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "%s mode selected", pszMode );
#endif
	}
}



void CEgon::WeaponIdle( void )
{
	ResetEmptySound( );

	if ( m_flTimeWeaponIdle > gpGlobals->time )
		return;

	if ( m_fireState != FIRE_OFF )
		 EndAttack();
	
	int iAnim;

	float flRand = RANDOM_FLOAT(0,1);

	if ( flRand <= 0.5 )
	{
		iAnim = EGON_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
	else 
	{
		iAnim = EGON_FIDGET1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3;
	}

	SendWeaponAnim( iAnim );
	m_deployed = TRUE;
}



void CEgon::EndAttack( void )
{
	bool bMakeNoise = false;
		
	if ( m_fireState != FIRE_OFF ) //Checking the button just in case!.
		 bMakeNoise = true;

	PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, m_pPlayer->edict(), m_usEgonStop, 0, (float *)&m_pPlayer->pev->origin, (float *)&m_pPlayer->pev->angles, 0.0, 0.0, bMakeNoise, 0, 0, 0 );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;

	m_fireState = FIRE_OFF;

#ifndef CLIENT_DLL
	for ( int i = 0; i < 5; i++ )
		m_hIonTargets[i] = NULL;

	DestroyEffect();
#endif
}



#ifndef CLIENT_DLL
class CEgonAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_chainammo.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_chainammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( 250, "uranium", URANIUM_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_egonclip, CEgonAmmo );
#endif

#endif