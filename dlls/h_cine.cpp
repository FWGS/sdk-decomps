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
/*

===== h_cine.cpp ========================================================

  The Halflife hard coded "scripted sequence".

  I'm pretty sure all this code is obsolete

*/

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"decals.h"


class CLegacyCineMonster : public CBaseMonster
{
public:
	void CineSpawn( char *szModel );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT CineThink( void );
	void Pain( void );
	void Die( void );
};

// Wanted replaced the scientist and barney dummies with its own characters
// (class names ours); no map uses any of them and their models don't ship.
class CCineWtowna : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine-twnwestA.mdl"); }
};

class CCineTiedColonel : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine-colonel_tied.mdl"); }
};

class CCineWtownb : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine-twnwestB.mdl"); }
};

class CCineTownmex : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine-townmex.mdl"); }
};

class CCineMasala : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine-masala.mdl"); }
};

class CCineNagatow : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine-nagatow.mdl"); }
};

class CCineCrispen : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine-crispen.mdl"); }
};

class CCineDave : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine-dave.mdl"); }
};

class CCinePanther : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine-panther.mdl"); }
};

class CCineHoss : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine-hoss.mdl"); }
};

class CCineAnnie : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine-annie.mdl"); }
};

class CCine2Wtowna : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine2-twnwestA.mdl"); }
};

class CCine2TiedColonel : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine2-colonel_tied.mdl"); }
};

class CCine2Wtownb : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine2-twn2west.mdl"); }
};

class CCine2Townmex : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine2-townmex.mdl"); }
};

class CCine2Masala : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine2-masala.mdl"); }
};

class CCine2Nagatow : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine2-nagatow.mdl"); }
};

class CCine2Crispen : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine2-crispen.mdl"); }
};

class CCine2Dave : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine2-dave.mdl"); }
};

class CCine2HeavyWeapons : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine2_hvyweapons.mdl"); }
};

class CCine2Slave : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine2_slave.mdl"); }
};

class CCine3Wtowna : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine3-twnwestA.mdl"); }
};

class CCine3TiedColonel : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine3-colonel_tied.mdl"); }
};

class CCine3Wtownb : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine3-twn2west.mdl"); }
};

class CCine3Townmex : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine3-townmex.mdl"); }
};

class CCine3Masala : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine3-masala.mdl"); }
};

class CCine3Nagatow : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine3-nagatow.mdl"); }
};

class CCine3Crispen : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine3-crispen.mdl"); }
};

class CCine3Dave : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine3-dave.mdl"); }
};

class CCine3Hoss : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine3-hoss.mdl"); }
};

class CCine3Annie : public CLegacyCineMonster
{
public:
	void Spawn( void ) { CineSpawn("models/cine3-annie.mdl"); }
};

//
// ********** Scientist SPAWN **********
//

LINK_ENTITY_TO_CLASS( monster_cine_wtowna, CCineWtowna );
LINK_ENTITY_TO_CLASS( monster_cine_tiedcolonel, CCineTiedColonel );
LINK_ENTITY_TO_CLASS( monster_cine_wtownb, CCineWtownb );
LINK_ENTITY_TO_CLASS( monster_cine_townmex, CCineTownmex );
LINK_ENTITY_TO_CLASS( monster_cine_masala, CCineMasala );
LINK_ENTITY_TO_CLASS( monster_cine_nagatow, CCineNagatow );
LINK_ENTITY_TO_CLASS( monster_cine_crispen, CCineCrispen );
LINK_ENTITY_TO_CLASS( monster_cine_dave, CCineDave );
LINK_ENTITY_TO_CLASS( monster_cine_panther, CCinePanther );
LINK_ENTITY_TO_CLASS( monster_cine_hoss, CCineHoss );
LINK_ENTITY_TO_CLASS( monster_cine_annie, CCineAnnie );
LINK_ENTITY_TO_CLASS( monster_cine2_wtowna, CCine2Wtowna );
LINK_ENTITY_TO_CLASS( monster_cine2_tiedcolonel, CCine2TiedColonel );
LINK_ENTITY_TO_CLASS( monster_cine2_wtownb, CCine2Wtownb );
LINK_ENTITY_TO_CLASS( monster_cine2_townmex, CCine2Townmex );
LINK_ENTITY_TO_CLASS( monster_cine2_masala, CCine2Masala );
LINK_ENTITY_TO_CLASS( monster_cine2_nagatow, CCine2Nagatow );
LINK_ENTITY_TO_CLASS( monster_cine2_crispen, CCine2Crispen );
LINK_ENTITY_TO_CLASS( monster_cine2_dave, CCine2Dave );
LINK_ENTITY_TO_CLASS( monster_cine2_hvyweapons, CCine2HeavyWeapons );
LINK_ENTITY_TO_CLASS( monster_cine2_slave, CCine2Slave );
LINK_ENTITY_TO_CLASS( monster_cine3_wtowna, CCine3Wtowna );
LINK_ENTITY_TO_CLASS( monster_cine3_tiedcolonel, CCine3TiedColonel );
LINK_ENTITY_TO_CLASS( monster_cine3_wtownb, CCine3Wtownb );
LINK_ENTITY_TO_CLASS( monster_cine3_townmex, CCine3Townmex );
LINK_ENTITY_TO_CLASS( monster_cine3_masala, CCine3Masala );
LINK_ENTITY_TO_CLASS( monster_cine3_nagatow, CCine3Nagatow );
LINK_ENTITY_TO_CLASS( monster_cine3_crispen, CCine3Crispen );
LINK_ENTITY_TO_CLASS( monster_cine3_dave, CCine3Dave );
LINK_ENTITY_TO_CLASS( monster_cine3_hoss, CCine3Hoss );
LINK_ENTITY_TO_CLASS( monster_cine3_annie, CCine3Annie );

//
// ********** Scientist SPAWN **********
//

void CLegacyCineMonster :: CineSpawn( char *szModel )
{
	PRECACHE_MODEL(szModel);
	SET_MODEL(ENT(pev), szModel);
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 64));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	pev->effects		= 0;
	pev->health			= 1;
	pev->yaw_speed		= 10;
	
	// ugly alpha hack, can't set ints from the bsp.	
	pev->sequence		= (int)pev->impulse;
	ResetSequenceInfo( );
	pev->framerate = 0.0;

	m_bloodColor = BLOOD_COLOR_RED;

	// if no targetname, start now
	if ( FStringNull(pev->targetname) )	
	{
		SetThink( CineThink );
		pev->nextthink += 1.0;
	}
}


//
// CineStart
//
void CLegacyCineMonster :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	pev->animtime = 0;	// reset the sequence
	SetThink( CineThink );
	pev->nextthink = gpGlobals->time;
}

//
// ********** Scientist DIE **********
//
void CLegacyCineMonster :: Die( void )
{
	SetThink( SUB_Remove );
}

//
// ********** Scientist PAIN **********
//
void CLegacyCineMonster :: Pain( void )
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pain3.wav", 1, ATTN_NORM);
}

void CLegacyCineMonster :: CineThink( void )
{
	// DBG_CheckMonsterData(pev);
	
	// Emit particles from origin (double check animator's placement of model)
	// THIS is a test feature
	//UTIL_ParticleEffect(pev->origin, g_vecZero, 255, 20);

	if (!pev->animtime)
		ResetSequenceInfo( );

	pev->nextthink = gpGlobals->time + 1.0;

	if (pev->spawnflags != 0 && m_fSequenceFinished)
	{
		Die();
		return;
	}

	StudioFrameAdvance ( );
}

//
// cine_blood
//
// e3/prealpha only. 
class CCineBlood : public CBaseEntity
{
public:
	void Spawn( void );
	void EXPORT BloodStart ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT BloodGush ( void );
};

LINK_ENTITY_TO_CLASS( cine_blood, CCineBlood );


void CCineBlood :: BloodGush ( void )
{
	Vector	vecSplatDir;
	TraceResult	tr;
	pev->nextthink = gpGlobals->time + 0.1;

	UTIL_MakeVectors(pev->angles);
	if ( pev->health-- < 0 )
		REMOVE_ENTITY(ENT(pev));
// CHANGE_METHOD ( ENT(pev), em_think, SUB_Remove );

	if ( RANDOM_FLOAT ( 0 , 1 ) < 0.7 )// larger chance of globs
	{
		UTIL_BloodDrips( pev->origin, UTIL_RandomBloodVector(), BLOOD_COLOR_RED, 10 );
	}
	else// slim chance of geyser
	{
		UTIL_BloodStream( pev->origin, UTIL_RandomBloodVector(), BLOOD_COLOR_RED, RANDOM_LONG(50, 150) );
	}

	if ( RANDOM_FLOAT ( 0, 1 ) < 0.75 )
	{// decals the floor with blood.
		vecSplatDir = Vector ( 0 , 0 , -1 );
		vecSplatDir = vecSplatDir + (RANDOM_FLOAT(-1,1) * 0.6 * gpGlobals->v_right) + (RANDOM_FLOAT(-1,1) * 0.6 * gpGlobals->v_forward);// randomize a bit
		UTIL_TraceLine( pev->origin + Vector ( 0, 0 , 64) , pev->origin + vecSplatDir * 256, ignore_monsters, ENT(pev), &tr);
		if ( tr.flFraction != 1.0 )
		{
			// Decal with a bloodsplat
			UTIL_BloodDecalTrace( &tr, BLOOD_COLOR_RED );
		}
	}
}

void CCineBlood :: BloodStart ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( BloodGush );
	pev->nextthink = gpGlobals->time;// now!
}

void CCineBlood :: Spawn ( void )
{
	pev->solid = SOLID_NOT;
	SetUse ( BloodStart );
	pev->health = 20;//hacked health to count iterations
}

