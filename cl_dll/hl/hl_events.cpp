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
#include "../hud.h"
#include "../cl_util.h"
#include "event_api.h"

extern "C"
{
// HLDM
void EV_FireGlock1( struct event_args_s *args  );
void EV_FireGlock2( struct event_args_s *args  );
void EV_FireShotGunSingle( struct event_args_s *args  );
void EV_FireMP5( struct event_args_s *args  );
void EV_FirePython( struct event_args_s *args  );
void EV_FireGauss( struct event_args_s *args  );
void EV_SpinGauss( struct event_args_s *args  );
void EV_TrainPitchAdjust( struct event_args_s *args );

// Gunman
void EV_FireGWShotgun( struct event_args_s *args );
void EV_FireGWMinigun( struct event_args_s *args );
void EV_MinigunHeat( struct event_args_s *args );
void EV_ChemicalSpray( struct event_args_s *args );
void EV_Lightning( struct event_args_s *args );
void EV_RocketSmoke( struct event_args_s *args );
void EV_VolumeExplosion( struct event_args_s *args );
void EV_ClusterExplosion( struct event_args_s *args );
void EV_ChargedImpact( struct event_args_s *args );
void EV_SphereExplode( struct event_args_s *args );
void EV_RapidImpact( struct event_args_s *args );
void EV_TankMGuns( struct event_args_s *args );
void EV_MonsterShotgun( struct event_args_s *args );
void EV_MonsterMechagun( struct event_args_s *args );
void EV_SniperShot( struct event_args_s *args );
void EV_Cylinder( struct event_args_s *args );
void EV_MonsterSentry( struct event_args_s *args );
void EV_TurretBullets( struct event_args_s *args );
void EV_WaterRipple( struct event_args_s *args );
void EV_BatbotBeam( struct event_args_s *args );
void EV_ChopperFire( struct event_args_s *args );
void EV_BallLightning( struct event_args_s *args );
void EV_RustbitImpact( struct event_args_s *args );
void EV_KataImpact( struct event_args_s *args );
void EV_Cleanup( struct event_args_s *args );
void EV_SpurtBlood( struct event_args_s *args );
}

/*
======================
Game_HookEvents

Associate script file name with callback functions.  Callback's must be extern "C" so
 the engine doesn't get confused about name mangling stuff.  Note that the format is
 always the same.  Of course, a clever mod team could actually embed parameters, behavior
 into the actual .sc files and create a .sc file parser and hook their functionality through
 that.. i.e., a scripting system.

That was what we were going to do, but we ran out of time...oh well.
======================
*/
void Game_HookEvents( void )
{
	gEngfuncs.pfnHookEvent( "events/glock1.sc",						EV_FireGlock1 );
	gEngfuncs.pfnHookEvent( "events/glock2.sc",						EV_FireGlock2 );
	gEngfuncs.pfnHookEvent( "events/shotgun1.sc",					EV_FireShotGunSingle );
	gEngfuncs.pfnHookEvent( "events/mp5.sc",						EV_FireMP5 );
	gEngfuncs.pfnHookEvent( "events/python.sc",						EV_FirePython );
	gEngfuncs.pfnHookEvent( "events/gauss.sc",						EV_FireGauss );
	gEngfuncs.pfnHookEvent( "events/gaussspin.sc",					EV_SpinGauss );
	gEngfuncs.pfnHookEvent( "events/train.sc",						EV_TrainPitchAdjust );
	gEngfuncs.pfnHookEvent( "events/gw_shotgun.sc",					EV_FireGWShotgun );
	gEngfuncs.pfnHookEvent( "events/gw_minigun.sc",					EV_FireGWMinigun );
	gEngfuncs.pfnHookEvent( "events/gw_minigunHeat.sc",				EV_MinigunHeat );
	gEngfuncs.pfnHookEvent( "events/ChemicalSpray.sc",				EV_ChemicalSpray );
	gEngfuncs.pfnHookEvent( "events/lightning.sc",					EV_Lightning );
	gEngfuncs.pfnHookEvent( "events/RocketSmoke.sc",				EV_RocketSmoke );
	gEngfuncs.pfnHookEvent( "events/volumeexplosion.sc",			EV_VolumeExplosion );
	gEngfuncs.pfnHookEvent( "events/ClusterExplosion.sc",			EV_ClusterExplosion );
	gEngfuncs.pfnHookEvent( "events/ChargedImpact.sc",				EV_ChargedImpact );
	gEngfuncs.pfnHookEvent( "events/sphereexplode.sc",				EV_SphereExplode );
	gEngfuncs.pfnHookEvent( "events/rapidimpact.sc",				EV_RapidImpact );
	gEngfuncs.pfnHookEvent( "events/tankmguns.sc",					EV_TankMGuns );
	gEngfuncs.pfnHookEvent( "events/monstershotgun.sc",				EV_MonsterShotgun );
	gEngfuncs.pfnHookEvent( "events/monstermechagun.sc",			EV_MonsterMechagun );
	gEngfuncs.pfnHookEvent( "events/snipershot.sc",					EV_SniperShot );
	gEngfuncs.pfnHookEvent( "events/cylinder.sc",					EV_Cylinder );
	gEngfuncs.pfnHookEvent( "events/monstersentry.sc",				EV_MonsterSentry );
	gEngfuncs.pfnHookEvent( "events/turretbullets.sc",				EV_TurretBullets );
	gEngfuncs.pfnHookEvent( "events/waterripple.sc",				EV_WaterRipple );
	gEngfuncs.pfnHookEvent( "events/batbotbeam.sc",					EV_BatbotBeam );
	gEngfuncs.pfnHookEvent( "events/chopperfire.sc",				EV_ChopperFire );
	gEngfuncs.pfnHookEvent( "events/balllightning.sc",				EV_BallLightning );
	gEngfuncs.pfnHookEvent( "events/rustbitimpact.sc",				EV_RustbitImpact );
	gEngfuncs.pfnHookEvent( "events/kataimpact.sc",					EV_KataImpact );
	gEngfuncs.pfnHookEvent( "events/cleanup.sc",					EV_Cleanup );
	gEngfuncs.pfnHookEvent( "events/spurtblood.sc",					EV_SpurtBlood );
}

// sic: placeholders, ours: functions retail was linked with and dropped
// (nothing calls them); their text is unknown. They give LINK's
// identical-copy sort the (size, relocations) keys with which it keeps
// retail's copies (see Unknown_W1_1 in ev_hldm.cpp): a fit, not evidence.
struct cl_enginefuncs_s;
extern struct cl_enginefuncs_s gEngfuncs;
void InitInput( void );

// key (96, 12)
int Unknown_end_hl_events_1( int *p )
{
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	p[0] = 1;
	p[1] = 2;
	p[2] = 3;
	return 0;
}
