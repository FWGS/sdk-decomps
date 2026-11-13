/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
//
//  cdll_int.c
//
// this implementation handles the linking of the engine to the DLL
//

#include "hud.h"
#include "cl_util.h"
#include "netadr.h"
#include "vgui_schememanager.h"

extern "C"
{
#include "pm_shared.h"
}

#include <string.h>
#include "hud_servers.h"
#include "vgui_int.h"
#include "interface.h"
#include "ITrackerUser.h"

#define DLLEXPORT		// Counter-Strike 1.4: nothing is exported but F (cdll_int.cpp)


cl_enginefunc_t gEngfuncs;
CHud gHUD;
TeamFortressViewport *gViewPort = NULL;

HINTERFACEMODULE g_hTrackerModule = NULL;
ITrackerUser *g_pTrackerUser = NULL;

void InitInput (void);
void EV_HookEvents( void );
void IN_Commands( void );

/*
========================== 
    Initialize

Called when the DLL is first loaded.
==========================
*/
extern "C" 
{
int		DLLEXPORT Initialize( cl_enginefunc_t *pEnginefuncs, int iVersion );
int		DLLEXPORT HUD_VidInit( void );
void	DLLEXPORT HUD_Init( void );
int		DLLEXPORT HUD_Redraw( float flTime, int intermission );
int		DLLEXPORT HUD_UpdateClientData( client_data_t *cdata, float flTime );
void	DLLEXPORT HUD_Reset ( void );
void	DLLEXPORT HUD_PlayerMove( struct playermove_s *ppmove, int server );
void	DLLEXPORT HUD_PlayerMoveInit( struct playermove_s *ppmove );
char	DLLEXPORT HUD_PlayerMoveTexture( char *name );
int		DLLEXPORT HUD_ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size );
int		DLLEXPORT HUD_GetHullBounds( int hullnumber, float *mins, float *maxs );
void	DLLEXPORT HUD_Frame( double time );
void	DLLEXPORT HUD_VoiceStatus(int entindex, qboolean bTalking);
void	DLLEXPORT HUD_DirectorMessage( int iSize, void *pbuf );

// Counter-Strike 1.4: the other files' functions that F hands to the engine
void	IN_ActivateMouse( void );
void	IN_DeactivateMouse( void );
void	IN_MouseEvent( int mstate );
void	IN_ClearStates( void );
void	IN_Accumulate( void );
void	CL_CreateMove( float frametime, struct usercmd_s *cmd, int active );
int		CL_IsThirdPerson( void );
void	CL_CameraOffset( float *ofs );
struct kbutton_s *KB_Find( const char *name );
void	CAM_Think( void );
void	V_CalcRefdef( struct ref_params_s *pparams );
int		HUD_AddEntity( int type, struct cl_entity_s *ent, const char *modelname );
void	HUD_CreateEntities( void );
void	HUD_DrawNormalTriangles( void );
void	HUD_DrawTransparentTriangles( void );
void	HUD_StudioEvent( const struct mstudioevent_s *event, const struct cl_entity_s *entity );
void	HUD_PostRunCmd( struct local_state_s *from, struct local_state_s *to, struct usercmd_s *cmd, int runfuncs, double time, unsigned int random_seed );
void	HUD_Shutdown( void );
void	HUD_TxferLocalOverrides( struct entity_state_s *state, const struct clientdata_s *client );
void	HUD_ProcessPlayerState( struct entity_state_s *dst, const struct entity_state_s *src );
void	HUD_TxferPredictionData( struct entity_state_s *ps, const struct entity_state_s *pps, struct clientdata_s *pcd, const struct clientdata_s *ppcd, struct weapon_data_s *wd, const struct weapon_data_s *pwd );
void	Demo_ReadBuffer( int size, unsigned char *buffer );
int		HUD_Key_Event( int down, int keynum, const char *pszCurrentBinding );
void	HUD_TempEntUpdate( double frametime, double client_time, double cl_gravity, struct tempent_s **ppTempEntFree, struct tempent_s **ppTempEntActive, int ( *Callback_AddVisibleEntity )( struct cl_entity_s *pEntity ), void ( *Callback_TempEntPlaySound )( struct tempent_s *pTemp, float damp ) );
struct cl_entity_s *HUD_GetUserEntity( int index );
int		HUD_GetStudioModelInterface( int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio );

// Counter-Strike 1.4: the file functions of steam_file.cpp (names ours)
int		Steam_GetLocalCopy( const char *pszName );
int		Steam_FileExists( const char *pszName );
}

/*
================================
HUD_GetHullBounds

  Engine calls this to enumerate player collision hulls, for prediction.  Return 0 if the hullnumber doesn't exist.
================================
*/
int DLLEXPORT HUD_GetHullBounds( int hullnumber, float *mins, float *maxs )
{
	g_pcldstAddrs->pGetHullBounds( &hullnumber, &mins, &maxs );

	int iret = 0;

	switch ( hullnumber )
	{
	case 0:				// Normal player
		mins = Vector(-16, -16, -36);
		maxs = Vector(16, 16, 36);
		iret = 1;
		break;
	case 1:				// Crouched player
		mins = Vector(-16, -16, -18 );
		maxs = Vector(16, 16, 18 );
		iret = 1;
		break;
	case 2:				// Point based hull
		mins = Vector( 0, 0, 0 );
		maxs = Vector( 0, 0, 0 );
		iret = 1;
		break;
	}

	return iret;
}

/*
================================
HUD_ConnectionlessPacket

 Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
  size of the response_buffer, so you must zero it out if you choose not to respond.
================================
*/
int	DLLEXPORT HUD_ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size )
{
	g_pcldstAddrs->pConnectionlessPacket( &net_from, &args, &response_buffer, &response_buffer_size );

	// Parse stuff from args
	int max_buffer_size = *response_buffer_size;

	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	return 0;
}

void DLLEXPORT HUD_PlayerMoveInit( struct playermove_s *ppmove )
{
	g_pcldstAddrs->pClientMoveInit( &ppmove );

	PM_Init( ppmove );
}

char DLLEXPORT HUD_PlayerMoveTexture( char *name )
{
	g_pcldstAddrs->pClientTextureType( &name );

	return PM_FindTextureType( name );
}

void DLLEXPORT HUD_PlayerMove( struct playermove_s *ppmove, int server )
{
	g_pcldstAddrs->pClientMove( &ppmove, &server );

	PM_Move( ppmove, server );
}

int DLLEXPORT Initialize( cl_enginefunc_t *pEnginefuncs, int iVersion )
{
	gEngfuncs = *pEnginefuncs;

	g_pcldstAddrs->pInitFunc( &pEnginefuncs, &iVersion );

	if (iVersion != CLDLL_INTERFACE_VERSION)
		return 0;

	EV_HookEvents();
	// get tracker interface, if any
	char szDir[512];
	if (!gEngfuncs.COM_ExpandFilename("Bin/TrackerUI.dll", szDir, sizeof(szDir)))
	{
		g_pTrackerUser = NULL;
		g_hTrackerModule = NULL;
		return 1;
	}

	Steam_GetLocalCopy( szDir );
	g_hTrackerModule = Sys_LoadModule(szDir);
	CreateInterfaceFn trackerFactory = Sys_GetFactory(g_hTrackerModule);
	if (!trackerFactory)
	{
		g_pTrackerUser = NULL;
		g_hTrackerModule = NULL;
		return 1;
	}

	g_pTrackerUser = (ITrackerUser *)trackerFactory(TRACKERUSER_INTERFACE_VERSION, NULL);
	return 1;
}


/*
==========================
	HUD_VidInit

Called when the game initializes
and whenever the vid_mode is changed
so the HUD can reinitialize itself.
==========================
*/

int DLLEXPORT HUD_VidInit( void )
{
	g_pcldstAddrs->pHudVidInitFunc();

	gHUD.VidInit();

	VGui_Startup();

	return 1;
}

// Counter-Strike 1.4: the stock player models.  The server's "CheckModels" message has the client
// look for the six that 1.4 brought new versions of; cl_minmodels works only when all are there.
// (Counter-Strike 1.6: sPlayerModelFiles, here; g_bPlayerModelsFound and CheckPlayerModelFiles are
// names of ours.)
bool g_bPlayerModelsFound = true;	// name ours

char *sPlayerModelFiles[] =
{
	"models/player.mdl",
	"models/player/leet/leet.mdl",
	"models/player/gign/gign.mdl",
	"models/player/vip/vip.mdl",
	"models/player/gsg9/gsg9.mdl",
	"models/player/guerilla/guerilla.mdl",
	"models/player/arctic/arctic.mdl",
	"models/player/sas/sas.mdl",
	"models/player/terror/terror.mdl",
	"models/player/urban/urban.mdl",
};

void CheckPlayerModelFiles( void )	// name ours
{
	char szPath[ 64 ];

	g_bPlayerModelsFound = true;

	for ( int i = 4; i < 10; i++ )
	{
		_snprintf( szPath, sizeof( szPath ), "cstrike/%s", sPlayerModelFiles[ i ] );
		if ( !Steam_FileExists( szPath ) )
		{
			g_bPlayerModelsFound = false;
			return;
		}
	}
}

/*
==========================
	HUD_Init

Called whenever the client connects
to a server.  Reinitializes all 
the hud variables.
==========================
*/

void DLLEXPORT HUD_Init( void )
{
	g_pcldstAddrs->pHudInitFunc();

	InitInput();
	gHUD.Init();
	Scheme_Init();
}


/*
==========================
	HUD_Redraw

called every screen frame to
redraw the HUD.
===========================
*/

int DLLEXPORT HUD_Redraw( float time, int intermission )
{
	g_pcldstAddrs->pHudRedrawFunc( &time, &intermission );

	gHUD.Redraw( time, intermission );

	return 1;
}


/*
==========================
	HUD_UpdateClientData

called every time shared client
dll/engine data gets changed,
and gives the cdll a chance
to modify the data.

returns 1 if anything has been changed, 0 otherwise.
==========================
*/

int DLLEXPORT HUD_UpdateClientData(client_data_t *pcldata, float flTime )
{
	g_pcldstAddrs->pHudUpdateClientDataFunc( &pcldata, &flTime );

	IN_Commands();

	return gHUD.UpdateClientData(pcldata, flTime );
}

/*
==========================
	HUD_Reset

Called at start and end of demos to restore to "non"HUD state.
==========================
*/

void DLLEXPORT HUD_Reset( void )
{
	g_pcldstAddrs->pHudResetFunc();

	gHUD.VidInit();
}

/*
==========================
HUD_Frame

Called by engine every frame that client .dll is loaded
==========================
*/

void DLLEXPORT HUD_Frame( double time )
{
	g_pcldstAddrs->pHudFrame( &time );

	ServersThink( time );

	GetClientVoiceMgr()->Frame(time);
}


/*
==========================
HUD_VoiceStatus

Called when a player starts or stops talking.
==========================
*/

void DLLEXPORT HUD_VoiceStatus(int entindex, qboolean bTalking)
{
	g_pcldstAddrs->pVoiceStatus( &entindex, &bTalking );

	if ( entindex >= 0 && entindex < gEngfuncs.GetMaxClients() )
	{
		if ( bTalking )
		{
			g_PlayerExtraInfo[ entindex ].flash_next = gHUD.m_flTime;
			g_PlayerExtraInfo[ entindex ].flash_on = 1;
			g_PlayerExtraInfo[ entindex ].flash_left = 99999;
		}
		else
		{
			g_PlayerExtraInfo[ entindex ].flash_next = -1;
			g_PlayerExtraInfo[ entindex ].flash_on = 0;
			g_PlayerExtraInfo[ entindex ].flash_left = 0;
		}
	}

	GetClientVoiceMgr()->UpdateSpeakerStatus(entindex, bTalking);
}

/*
==========================
HUD_DirectorEvent

Called when a director event message was received
==========================
*/

void DLLEXPORT HUD_DirectorMessage( int iSize, void *pbuf )
{
	g_pcldstAddrs->pDirectorMessage( &iSize, &pbuf );

	 gHUD.m_Spectator.DirectorMessage( iSize, pbuf );
}

/*
==========================
F

Counter-Strike 1.4: the library's one export.  The engine passes a table whose first entry
points at its module functions and whose third at the entry points every function below calls
first; it gets the client's functions back in the same table.
(Counter-Strike 1.6: g_modfuncs, g_pcldstAddrs, F( void *pv ), pcldll_func, cldll_func.)
==========================
*/
modfuncs_t g_modfuncs;
cldll_func_dst_t *g_pcldstAddrs;

extern "C" void __declspec( dllexport ) F( void *pv )
{
	cldll_func_t *pcldll_func = (cldll_func_t *)pv;

	g_modfuncs = *(modfuncs_t *)pcldll_func->pInitFunc;
	g_pcldstAddrs = (cldll_func_dst_t *)pcldll_func->pHudVidInitFunc;

	cldll_func_t cldll_func = 
	{
	Initialize,
	HUD_Init,
	HUD_VidInit,
	HUD_Redraw,
	HUD_UpdateClientData,
	HUD_Reset,
	HUD_PlayerMove,
	HUD_PlayerMoveInit,
	HUD_PlayerMoveTexture,
	IN_ActivateMouse,
	IN_DeactivateMouse,
	IN_MouseEvent,
	IN_ClearStates,
	IN_Accumulate,
	CL_CreateMove,
	CL_IsThirdPerson,
	CL_CameraOffset,
	KB_Find,
	CAM_Think,
	V_CalcRefdef,
	HUD_AddEntity,
	HUD_CreateEntities,
	HUD_DrawNormalTriangles,
	HUD_DrawTransparentTriangles,
	HUD_StudioEvent,
	HUD_PostRunCmd,
	HUD_Shutdown,
	HUD_TxferLocalOverrides,
	HUD_ProcessPlayerState,
	HUD_TxferPredictionData,
	Demo_ReadBuffer,
	HUD_ConnectionlessPacket,
	HUD_GetHullBounds,
	HUD_Frame,
	HUD_Key_Event,
	HUD_TempEntUpdate,
	HUD_GetUserEntity,
	HUD_VoiceStatus,
	HUD_DirectorMessage,
	HUD_GetStudioModelInterface,
	};

	*pcldll_func = cldll_func;
}


