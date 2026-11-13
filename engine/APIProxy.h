//========= Counter-Strike 1.4: the function tables between the engine and the client library ============

// Names, order and (where this file is short enough to keep them) line numbers are those of Counter-Strike 1.6's
// engine/APIProxy.h as its Linux client's debug information has them; the text is ours.

#ifndef APIPROXY_H
#define APIPROXY_H

















// the client's exported functions (none is exported any more: the engine asks F() for them)

typedef int (*INITIALIZE_FUNC)( struct cl_enginefuncs_s *, int );
typedef void (*HUD_INIT_FUNC)( void );
typedef int (*HUD_VIDINIT_FUNC)( void );
typedef int (*HUD_REDRAW_FUNC)( float, int );
typedef int (*HUD_UPDATECLIENTDATA_FUNC)( struct client_data_s *, float );
typedef void (*HUD_RESET_FUNC)( void );
typedef void (*HUD_CLIENTMOVE_FUNC)( struct playermove_s *, qboolean );
typedef void (*HUD_CLIENTMOVEINIT_FUNC)( struct playermove_s * );
typedef char (*HUD_TEXTURETYPE_FUNC)( char * );
typedef void (*HUD_IN_ACTIVATEMOUSE_FUNC)( void );
typedef void (*HUD_IN_DEACTIVATEMOUSE_FUNC)( void );
typedef void (*HUD_IN_MOUSEEVENT_FUNC)( int );
typedef void (*HUD_IN_CLEARSTATES_FUNC)( void );
typedef void (*HUD_IN_ACCUMULATE_FUNC)( void );
typedef void (*HUD_CL_CREATEMOVE_FUNC)( float, struct usercmd_s *, int );
typedef int (*HUD_CL_ISTHIRDPERSON_FUNC)( void );
typedef void (*HUD_CL_GETCAMERAOFFSETS_FUNC)( float * );
typedef struct kbutton_s *(*HUD_KB_FIND_FUNC)( const char * );
typedef void (*HUD_CAMTHINK_FUNC)( void );
typedef void (*HUD_CALCREF_FUNC)( struct ref_params_s * );
typedef int (*HUD_ADDENTITY_FUNC)( int, struct cl_entity_s *, const char * );
typedef void (*HUD_CREATEENTITIES_FUNC)( void );
typedef void (*HUD_DRAWNORMALTRIS_FUNC)( void );
typedef void (*HUD_DRAWTRANSTRIS_FUNC)( void );
typedef void (*HUD_STUDIOEVENT_FUNC)( const struct mstudioevent_s *, const struct cl_entity_s * );
typedef void (*HUD_POSTRUNCMD_FUNC)( struct local_state_s *, struct local_state_s *, struct usercmd_s *, int, double, unsigned int );
typedef void (*HUD_SHUTDOWN_FUNC)( void );
typedef void (*HUD_TXFERLOCALOVERRIDES_FUNC)( struct entity_state_s *, const struct clientdata_s * );
typedef void (*HUD_PROCESSPLAYERSTATE_FUNC)( struct entity_state_s *, const struct entity_state_s * );
typedef void (*HUD_TXFERPREDICTIONDATA_FUNC)( struct entity_state_s *, const struct entity_state_s *, struct clientdata_s *, const struct clientdata_s *, struct weapon_data_s *, const struct weapon_data_s * );
typedef void (*HUD_DEMOREAD_FUNC)( int, unsigned char * );
typedef int (*HUD_CONNECTIONLESS_FUNC)( const struct netadr_s *, const char *, char *, int * );
typedef int (*HUD_GETHULLBOUNDS_FUNC)( int, float *, float * );
typedef void (*HUD_FRAME_FUNC)( double );
typedef int (*HUD_KEY_EVENT_FUNC)( int, int, const char * );
typedef void (*HUD_TEMPENTUPDATE_FUNC)( double, double, double, struct tempent_s **, struct tempent_s **, int (*)(struct cl_entity_s *), void (*)(struct tempent_s *, float) );
typedef struct cl_entity_s *(*HUD_GETUSERENTITY_FUNC)( int );
typedef void (*HUD_VOICESTATUS_FUNC)( int, qboolean );
typedef void (*HUD_DIRECTORMESSAGE_FUNC)( int, void * );
typedef int (*HUD_STUDIO_INTERFACE_FUNC)( int, struct r_studio_interface_s **, struct engine_studio_api_s * );







// the client's functions, as F() hands them to the engine
typedef struct cldll_func_s
{
	INITIALIZE_FUNC	pInitFunc;
	HUD_INIT_FUNC	pHudInitFunc;
	HUD_VIDINIT_FUNC	pHudVidInitFunc;
	HUD_REDRAW_FUNC	pHudRedrawFunc;
	HUD_UPDATECLIENTDATA_FUNC	pHudUpdateClientDataFunc;
	HUD_RESET_FUNC	pHudResetFunc;
	HUD_CLIENTMOVE_FUNC	pClientMove;
	HUD_CLIENTMOVEINIT_FUNC	pClientMoveInit;
	HUD_TEXTURETYPE_FUNC	pClientTextureType;
	HUD_IN_ACTIVATEMOUSE_FUNC	pIN_ActivateMouse;
	HUD_IN_DEACTIVATEMOUSE_FUNC	pIN_DeactivateMouse;
	HUD_IN_MOUSEEVENT_FUNC	pIN_MouseEvent;
	HUD_IN_CLEARSTATES_FUNC	pIN_ClearStates;
	HUD_IN_ACCUMULATE_FUNC	pIN_Accumulate;
	HUD_CL_CREATEMOVE_FUNC	pCL_CreateMove;
	HUD_CL_ISTHIRDPERSON_FUNC	pCL_IsThirdPerson;
	HUD_CL_GETCAMERAOFFSETS_FUNC	pCL_GetCameraOffsets;
	HUD_KB_FIND_FUNC	pFindKey;
	HUD_CAMTHINK_FUNC	pCamThink;
	HUD_CALCREF_FUNC	pCalcRefdef;
	HUD_ADDENTITY_FUNC	pAddEntity;
	HUD_CREATEENTITIES_FUNC	pCreateEntities;
	HUD_DRAWNORMALTRIS_FUNC	pDrawNormalTriangles;
	HUD_DRAWTRANSTRIS_FUNC	pDrawTransparentTriangles;
	HUD_STUDIOEVENT_FUNC	pStudioEvent;
	HUD_POSTRUNCMD_FUNC	pPostRunCmd;
	HUD_SHUTDOWN_FUNC	pShutdown;
	HUD_TXFERLOCALOVERRIDES_FUNC	pTxferLocalOverrides;
	HUD_PROCESSPLAYERSTATE_FUNC	pProcessPlayerState;
	HUD_TXFERPREDICTIONDATA_FUNC	pTxferPredictionData;
	HUD_DEMOREAD_FUNC	pReadDemoBuffer;
	HUD_CONNECTIONLESS_FUNC	pConnectionlessPacket;
	HUD_GETHULLBOUNDS_FUNC	pGetHullBounds;
	HUD_FRAME_FUNC	pHudFrame;
	HUD_KEY_EVENT_FUNC	pKeyEvent;
	HUD_TEMPENTUPDATE_FUNC	pTempEntUpdate;
	HUD_GETUSERENTITY_FUNC	pGetUserEntity;
	HUD_VOICESTATUS_FUNC	pVoiceStatus;
	HUD_DIRECTORMESSAGE_FUNC	pDirectorMessage;
	HUD_STUDIO_INTERFACE_FUNC	pStudioInterface;
} cldll_func_t;



// the same list as the engine's entry points: each takes the addresses of the function's arguments

typedef void (*DST_INITIALIZE_FUNC)( struct cl_enginefuncs_s **, int * );
typedef void (*DST_HUD_INIT_FUNC)( void );
typedef void (*DST_HUD_VIDINIT_FUNC)( void );
typedef void (*DST_HUD_REDRAW_FUNC)( float *, int * );
typedef void (*DST_HUD_UPDATECLIENTDATA_FUNC)( struct client_data_s **, float * );
typedef void (*DST_HUD_RESET_FUNC)( void );
typedef void (*DST_HUD_CLIENTMOVE_FUNC)( struct playermove_s **, qboolean * );
typedef void (*DST_HUD_CLIENTMOVEINIT_FUNC)( struct playermove_s ** );
typedef void (*DST_HUD_TEXTURETYPE_FUNC)( char ** );
typedef void (*DST_HUD_IN_ACTIVATEMOUSE_FUNC)( void );
typedef void (*DST_HUD_IN_DEACTIVATEMOUSE_FUNC)( void );
typedef void (*DST_HUD_IN_MOUSEEVENT_FUNC)( int * );
typedef void (*DST_HUD_IN_CLEARSTATES_FUNC)( void );
typedef void (*DST_HUD_IN_ACCUMULATE_FUNC)( void );
typedef void (*DST_HUD_CL_CREATEMOVE_FUNC)( float *, struct usercmd_s **, int * );
typedef void (*DST_HUD_CL_ISTHIRDPERSON_FUNC)( void );
typedef void (*DST_HUD_CL_GETCAMERAOFFSETS_FUNC)( float ** );
typedef void (*DST_HUD_KB_FIND_FUNC)( const char ** );
typedef void (*DST_HUD_CAMTHINK_FUNC)( void );
typedef void (*DST_HUD_CALCREF_FUNC)( struct ref_params_s ** );
typedef void (*DST_HUD_ADDENTITY_FUNC)( int *, struct cl_entity_s **, const char ** );
typedef void (*DST_HUD_CREATEENTITIES_FUNC)( void );
typedef void (*DST_HUD_DRAWNORMALTRIS_FUNC)( void );
typedef void (*DST_HUD_DRAWTRANSTRIS_FUNC)( void );
typedef void (*DST_HUD_STUDIOEVENT_FUNC)( const struct mstudioevent_s **, const struct cl_entity_s ** );
typedef void (*DST_HUD_POSTRUNCMD_FUNC)( struct local_state_s **, struct local_state_s **, struct usercmd_s **, int *, double *, unsigned int * );
typedef void (*DST_HUD_SHUTDOWN_FUNC)( void );
typedef void (*DST_HUD_TXFERLOCALOVERRIDES_FUNC)( struct entity_state_s **, const struct clientdata_s ** );
typedef void (*DST_HUD_PROCESSPLAYERSTATE_FUNC)( struct entity_state_s **, const struct entity_state_s ** );
typedef void (*DST_HUD_TXFERPREDICTIONDATA_FUNC)( struct entity_state_s **, const struct entity_state_s **, struct clientdata_s **, const struct clientdata_s **, struct weapon_data_s **, const struct weapon_data_s ** );
typedef void (*DST_HUD_DEMOREAD_FUNC)( int *, unsigned char ** );
typedef void (*DST_HUD_CONNECTIONLESS_FUNC)( const struct netadr_s **, const char **, char **, int ** );
typedef void (*DST_HUD_GETHULLBOUNDS_FUNC)( int *, float **, float ** );
typedef void (*DST_HUD_FRAME_FUNC)( double * );
typedef void (*DST_HUD_KEY_EVENT_FUNC)( int *, int *, const char ** );
typedef void (*DST_HUD_TEMPENTUPDATE_FUNC)( double *, double *, double *, struct tempent_s ***, struct tempent_s ***, int (**)(struct cl_entity_s *), void (**)(struct tempent_s *, float) );
typedef void (*DST_HUD_GETUSERENTITY_FUNC)( int * );
typedef void (*DST_HUD_VOICESTATUS_FUNC)( int *, qboolean * );
typedef void (*DST_HUD_DIRECTORMESSAGE_FUNC)( int *, void ** );
typedef void (*DST_HUD_STUDIO_INTERFACE_FUNC)( int *, struct r_studio_interface_s ***, struct engine_studio_api_s ** );



typedef struct cldll_func_dst_s
{

	DST_INITIALIZE_FUNC	pInitFunc;
	DST_HUD_INIT_FUNC	pHudInitFunc;
	DST_HUD_VIDINIT_FUNC	pHudVidInitFunc;
	DST_HUD_REDRAW_FUNC	pHudRedrawFunc;
	DST_HUD_UPDATECLIENTDATA_FUNC	pHudUpdateClientDataFunc;
	DST_HUD_RESET_FUNC	pHudResetFunc;
	DST_HUD_CLIENTMOVE_FUNC	pClientMove;
	DST_HUD_CLIENTMOVEINIT_FUNC	pClientMoveInit;
	DST_HUD_TEXTURETYPE_FUNC	pClientTextureType;
	DST_HUD_IN_ACTIVATEMOUSE_FUNC	pIN_ActivateMouse;
	DST_HUD_IN_DEACTIVATEMOUSE_FUNC	pIN_DeactivateMouse;
	DST_HUD_IN_MOUSEEVENT_FUNC	pIN_MouseEvent;
	DST_HUD_IN_CLEARSTATES_FUNC	pIN_ClearStates;
	DST_HUD_IN_ACCUMULATE_FUNC	pIN_Accumulate;
	DST_HUD_CL_CREATEMOVE_FUNC	pCL_CreateMove;
	DST_HUD_CL_ISTHIRDPERSON_FUNC	pCL_IsThirdPerson;
	DST_HUD_CL_GETCAMERAOFFSETS_FUNC	pCL_GetCameraOffsets;
	DST_HUD_KB_FIND_FUNC	pFindKey;
	DST_HUD_CAMTHINK_FUNC	pCamThink;
	DST_HUD_CALCREF_FUNC	pCalcRefdef;
	DST_HUD_ADDENTITY_FUNC	pAddEntity;
	DST_HUD_CREATEENTITIES_FUNC	pCreateEntities;
	DST_HUD_DRAWNORMALTRIS_FUNC	pDrawNormalTriangles;
	DST_HUD_DRAWTRANSTRIS_FUNC	pDrawTransparentTriangles;
	DST_HUD_STUDIOEVENT_FUNC	pStudioEvent;
	DST_HUD_POSTRUNCMD_FUNC	pPostRunCmd;
	DST_HUD_SHUTDOWN_FUNC	pShutdown;
	DST_HUD_TXFERLOCALOVERRIDES_FUNC	pTxferLocalOverrides;
	DST_HUD_PROCESSPLAYERSTATE_FUNC	pProcessPlayerState;
	DST_HUD_TXFERPREDICTIONDATA_FUNC	pTxferPredictionData;
	DST_HUD_DEMOREAD_FUNC	pReadDemoBuffer;
	DST_HUD_CONNECTIONLESS_FUNC	pConnectionlessPacket;
	DST_HUD_GETHULLBOUNDS_FUNC	pGetHullBounds;
	DST_HUD_FRAME_FUNC	pHudFrame;
	DST_HUD_KEY_EVENT_FUNC	pKeyEvent;
	DST_HUD_TEMPENTUPDATE_FUNC	pTempEntUpdate;
	DST_HUD_GETUSERENTITY_FUNC	pGetUserEntity;
	DST_HUD_VOICESTATUS_FUNC	pVoiceStatus;
	DST_HUD_DIRECTORMESSAGE_FUNC	pDirectorMessage;
	DST_HUD_STUDIO_INTERFACE_FUNC	pStudioInterface;
} cldll_func_dst_t;

// What the engine hands to F() beside the table.  Counter-Strike 1.4's F() copies 27 words of it
// and no function of the client reads them: the first 17 members have Counter-Strike 1.6's names
// (engine/APIProxy.h, modfuncs_s), the count of the unused words behind them is the image's.
typedef struct modfuncs_s
{
	void	(*m_pfnLoadMod)( char *pchModule );
	void	(*m_pfnCloseMod)( void );
	int		(*m_pfnNCall)( int ijump, int cnArg, ... );

	void	(*m_pfnGetClDstAddrs)( cldll_func_dst_t *pcldstAddrs );
	void	(*m_pfnGetEngDstAddrs)( void *pengdstAddrs );

	void	(*m_pfnModuleLoaded)( void );

	void	(*m_pfnProcessOutgoingNet)( struct netchan_s *pchan, struct sizebuf_s *psizebuf );
	qboolean	(*m_pfnProcessIncomingNet)( struct netchan_s *pchan, struct sizebuf_s *psizebuf );

	void	(*m_pfnTextureLoad)( char *pszName, int dxWidth, int dyHeight, char *pbData );
	void	(*m_pfnModelLoad)( struct model_s *pmodel, void *pvBuf );

	void	(*m_pfnFrameBegin)( void );
	void	(*m_pfnFrameRender1)( void );
	void	(*m_pfnFrameRender2)( void );

	void	(*m_pfnSetModSHelpers)( void *pmodshelpers );
	void	(*m_pfnSetModCHelpers)( void *pmodchelpers );
	void	(*m_pfnSetEngData)( void *pengdata );

	int		m_nVersion;

	int		m_nVoid0;
	int		m_nVoid1;
	int		m_nVoid2;
	int		m_nVoid3;
	int		m_nVoid4;
	int		m_nVoid5;
	int		m_nVoid6;
	int		m_nVoid7;
	int		m_nVoid8;
	int		m_nVoid9;
} modfuncs_t;

// the client's copy of the engine's entry points (cl_dll/cdll_int.cpp)
extern cldll_func_dst_t *g_pcldstAddrs;

#endif // APIPROXY_H
