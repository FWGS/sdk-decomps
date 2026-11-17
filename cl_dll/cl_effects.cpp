//
// cl_effects.cpp
//
// Purpose: helpers shared by the events and the client effects: a temp entity
//          that grows and fades away, and the test whether an entity came in
//          the last packet. Names ours but EV_IsInPVS.
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "r_efx.h"

// names ours: the gluon gun's flare at the end of its beam, and the tesla
// coil's lightning beams (four used); both follow the view in UpdateBeams
TEMPENTITY *g_pEgonFlare;
BEAM *g_pTeslaBeams[5];

static void EV_FadeGrowCallback( TEMPENTITY *ent, float frametime, float currenttime );

//=========================================================
// lets a temp entity grow by flScaleSpeed per second and
// fade at its curstate.fuser2 per second until it is gone;
// flUnused is never read (sic)
//=========================================================
void EV_FadeGrowTempEnt( TEMPENTITY *pTemp, float flUnused, float flScaleSpeed )
{
	if ( pTemp && flScaleSpeed > 0 )
	{
		pTemp->callback = EV_FadeGrowCallback;
		pTemp->entity.curstate.fuser1 = flScaleSpeed;
	}
}

static void EV_FadeGrowCallback( TEMPENTITY *ent, float frametime, float currenttime )
{
	if ( !ent )
		return;

	ent->entity.curstate.scale += frametime * ent->entity.curstate.fuser1;
	ent->entity.curstate.renderamt -= frametime * ent->entity.curstate.fuser2;

	if ( ent->entity.curstate.renderamt <= 0 )
		ent->die = currenttime;
}

//=========================================================
// true if the entity came in the last packet (it is in the
// local player's PVS)
//=========================================================
bool EV_IsInPVS( cl_entity_t *pEntity )
{
	if ( pEntity->curstate.messagenum < gEngfuncs.GetLocalPlayer()->curstate.messagenum )
		return false;

	return true;
}
