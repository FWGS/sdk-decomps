/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
/*

===== lights.cpp ========================================================

  spawn and think functions for editor-placed lights

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"

//LRC
char* GetStdLightStyle (int iStyle)
{
	switch (iStyle)
	{

	// 0 normal
	case 0: return "m";

	// 1 FLICKER (first variety)
	case 1: return "mmnmmommommnonmmonqnmmo";

	// 2 SLOW STRONG PULSE
	case 2: return "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba";

	// 3 CANDLE (first variety)
	case 3: "mmmmmaaaaammmmmaaaaaabcdefgabcdefg";

	// 4 FAST STROBE
	case 4: return "mamamamamama";
	
	// 5 GENTLE PULSE 1
	case 5: return "jklmnopqrstuvwxyzyxwvutsrqponmlkj";
	
	// 6 FLICKER (second variety)
	case 6: return "nmonqnmomnmomomno";
	
	// 7 CANDLE (second variety)
	case 7: return "mmmaaaabcdefgmmmmaaaammmaamm";
	
	// 8 CANDLE (third variety)
	case 8: return "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa";
	
	// 9 SLOW STROBE (fourth variety)
	case 9: return "aaaaaaaazzzzzzzz";
	
	// 10 FLUORESCENT FLICKER
	case 10: return "mmamammmmammamamaaamammma";

	// 11 SLOW PULSE NOT FADE TO BLACK
	case 11: return "abcdefghijklmnopqrrqponmlkjihgfedcba";
	
	// 12 UNDERWATER LIGHT MUTATION
	// this light only distorts the lightmap - no contribution
	// is made to the brightness of affected surfaces
	case 12: return "mmnnmmnnnmmnn";

	// 13 OFF (LRC)
	case 13: return "a";

	// 14 SLOW FADE IN (LRC)
	case 14: return "aabbccddeeffgghhiijjkkllmmmmmmmmmmmmmm";

	// 15 MED FADE IN (LRC)
	case 15: return "abcdefghijklmmmmmmmmmmmmmmmmmmmmmmmmmm";

	// 16 FAST FADE IN (LRC)
	case 16: return "acegikmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm";

	// 17 SLOW FADE OUT (LRC)
	case 17: return "llkkjjiihhggffeeddccbbaaaaaaaaaaaaaaaa";

	// 18 MED FADE OUT (LRC)
	case 18: return "lkjihgfedcbaaaaaaaaaaaaaaaaaaaaaaaaaaa";

	// 19 FAST FADE OUT (LRC)
	case 19: return "kigecaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

	default: return "m";
	}
}


class CLight : public CPointEntity
{
public:
	virtual void	KeyValue( KeyValueData* pkvd ); 
	virtual void	Spawn( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	Think( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	virtual STATE	GetState(void) { return m_iState; }; //LRC
	
	static	TYPEDESCRIPTION m_SaveData[];

	void	SetCorrectStyle(void);

	int		m_iStyle; // number: this light's index in the style table
					  // made public so trigger_lightstyle works

private:
	STATE	m_iState; // current state
	int		m_iOnStyle; // style to use while on
	int		m_iOffStyle; // style to use while off
	int		m_iTurnOnStyle; // style to use while turning on
	int		m_iTurnOffStyle; // style to use while turning off
	int		m_iTurnOnTime; // time taken to turn on
	int		m_iTurnOffTime; // time taken to turn off
	int		m_iszPattern; // custom style to use while on
};
LINK_ENTITY_TO_CLASS( light, CLight );

TYPEDESCRIPTION	CLight::m_SaveData[] = 
{
	DEFINE_FIELD( CLight, m_iState, FIELD_INTEGER ),
	DEFINE_FIELD( CLight, m_iStyle, FIELD_INTEGER ),
	DEFINE_FIELD( CLight, m_iszPattern, FIELD_STRING ),
	DEFINE_FIELD( CLight, m_iOnStyle, FIELD_INTEGER ),
	DEFINE_FIELD( CLight, m_iOffStyle, FIELD_INTEGER ),
	DEFINE_FIELD( CLight, m_iTurnOnStyle, FIELD_INTEGER ),
	DEFINE_FIELD( CLight, m_iTurnOffStyle, FIELD_INTEGER ),
	DEFINE_FIELD( CLight, m_iTurnOnTime, FIELD_INTEGER ),
	DEFINE_FIELD( CLight, m_iTurnOffTime, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CLight, CPointEntity );


//
// Cache user-entity-field values until spawn is called.
//
void CLight :: KeyValue( KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "style"))
	{
		m_iStyle = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iOnStyle"))
	{
		m_iOnStyle = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iOffStyle"))
	{
		m_iOffStyle = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iTurnOnStyle"))
	{
		m_iTurnOnStyle = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iTurnOffStyle"))
	{
		m_iTurnOffStyle = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iTurnOnTime"))
	{
		m_iTurnOnTime = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iTurnOffTime"))
	{
		m_iTurnOffTime = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pitch"))
	{
		pev->angles.x = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pattern"))
	{
		m_iszPattern = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "firetarget"))
	{
		pev->target = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CPointEntity::KeyValue( pkvd );
	}
}

// regardless of what's been set by trigger_lightstyle ents, set the style I think I need
void CLight :: SetCorrectStyle ( void )
{
	if (m_iStyle >= 32)
	{
		switch (m_iState)
		{
		case STATE_ON:
			if (m_iszPattern) // custom styles have priority over standard ones
				LIGHT_STYLE(m_iStyle, (char *)STRING( m_iszPattern ));
			else if (m_iOnStyle)
				LIGHT_STYLE(m_iStyle, GetStdLightStyle(m_iOnStyle));
			else
				LIGHT_STYLE(m_iStyle, "m");
			break;
		case STATE_OFF:
			if (m_iOffStyle)
				LIGHT_STYLE(m_iStyle, GetStdLightStyle(m_iOffStyle));
			else
				LIGHT_STYLE(m_iStyle, "a");
			break;
		case STATE_TURN_ON:
			if (m_iTurnOnStyle)
				LIGHT_STYLE(m_iStyle, GetStdLightStyle(m_iTurnOnStyle));
			else
				LIGHT_STYLE(m_iStyle, "a");
			break;
		case STATE_TURN_OFF:
			if (m_iTurnOffStyle)
				LIGHT_STYLE(m_iStyle, GetStdLightStyle(m_iTurnOffStyle));
			else
				LIGHT_STYLE(m_iStyle, "m");
			break;
		}
	}
}

void CLight :: Think( void )
{
	switch (GetState())
	{
	case STATE_TURN_ON:
		m_iState = STATE_ON;
		FireTargets(STRING(pev->target),this,this,USE_ON,0);
		break;
	case STATE_TURN_OFF:
		m_iState = STATE_OFF;
		FireTargets(STRING(pev->target),this,this,USE_OFF,0);
		break;
	}
	SetCorrectStyle();
}

/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) LIGHT_START_OFF
Non-displayed light.
Default light value is 300
Default style is 0
If targeted, it will toggle between on or off.
*/

void CLight :: Spawn( void )
{
	if (FStringNull(pev->targetname))
	{       // inert light
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	if (FBitSet(pev->spawnflags,SF_LIGHT_START_OFF))
		m_iState = STATE_OFF;
	else
		m_iState = STATE_ON;
	SetCorrectStyle();
}


void CLight :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (m_iStyle >= 32)
	{
		if ( !ShouldToggle( useType ) )
			return;

		switch (GetState())
		{
		case STATE_ON:
		case STATE_TURN_ON:
			if (m_iTurnOffTime)
			{
				m_iState = STATE_TURN_OFF;
				SetNextThink( m_iTurnOffTime );
			}
			else
				m_iState = STATE_OFF;
			break;
		case STATE_OFF:
		case STATE_TURN_OFF:
			if (m_iTurnOnTime)
			{
				m_iState = STATE_TURN_ON;
				SetNextThink( m_iTurnOnTime );
			}
			else
				m_iState = STATE_ON;
			break;
		}
		SetCorrectStyle();
	}
}

//
// shut up spawn functions for new spotlights
//
LINK_ENTITY_TO_CLASS( light_spot, CLight );


class CEnvLight : public CLight
{
public:
	void	KeyValue( KeyValueData* pkvd ); 
	void	Spawn( void );
};

LINK_ENTITY_TO_CLASS( light_environment, CEnvLight );

void CEnvLight::KeyValue( KeyValueData* pkvd )
{
	if (FStrEq(pkvd->szKeyName, "_light"))
	{
		int r, g, b, v, j;
		char szColor[64];
		j = sscanf( pkvd->szValue, "%d %d %d %d\n", &r, &g, &b, &v );
		if (j == 1)
		{
			g = b = r;
		}
		else if (j == 4)
		{
			r = r * (v / 255.0);
			g = g * (v / 255.0);
			b = b * (v / 255.0);
		}

		// simulate qrad direct, ambient,and gamma adjustments, as well as engine scaling
		r = pow( r / 114.0, 0.6 ) * 264;
		g = pow( g / 114.0, 0.6 ) * 264;
		b = pow( b / 114.0, 0.6 ) * 264;

		pkvd->fHandled = TRUE;
		sprintf( szColor, "%d", r );
		CVAR_SET_STRING( "sv_skycolor_r", szColor );
		sprintf( szColor, "%d", g );
		CVAR_SET_STRING( "sv_skycolor_g", szColor );
		sprintf( szColor, "%d", b );
		CVAR_SET_STRING( "sv_skycolor_b", szColor );
	}
	else
	{
		CLight::KeyValue( pkvd );
	}
}


void CEnvLight :: Spawn( void )
{
	char szVector[64];
	UTIL_MakeAimVectors( pev->angles );

	sprintf( szVector, "%f", gpGlobals->v_forward.x );
	CVAR_SET_STRING( "sv_skyvec_x", szVector );
	sprintf( szVector, "%f", gpGlobals->v_forward.y );
	CVAR_SET_STRING( "sv_skyvec_y", szVector );
	sprintf( szVector, "%f", gpGlobals->v_forward.z );
	CVAR_SET_STRING( "sv_skyvec_z", szVector );

	CLight::Spawn( );
}

//**********************************************************
//LRC- the CLightDynamic entity - works like the flashlight.
//**********************************************************

#define SF_LIGHTDYNAMIC_START_OFF	1
#define SF_LIGHTDYNAMIC_FLARE		2

class CLightDynamic : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void SetEffects( void );
	STATE GetState( void );
};

LINK_ENTITY_TO_CLASS( light_glow, CLightDynamic );

void CLightDynamic::Spawn( void )
{
	Precache( );

	SET_MODEL(ENT(pev), "sprites/null.spr");
	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_NONE;

	if (!(pev->spawnflags & SF_LIGHTDYNAMIC_START_OFF))
	{
		pev->health = 1;
		SetEffects();
	}
}

void CLightDynamic :: Precache( void )
{
	PRECACHE_MODEL("sprites/null.spr");
}

void CLightDynamic::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (ShouldToggle(useType, pev->health))
	{
		if (pev->health)
			pev->health = 0;
		else
			pev->health = 1;
		SetEffects();
	}
}

void CLightDynamic::SetEffects( void )
{
	if (pev->health)
	{
		if (pev->frags == 2)
			pev->effects |= EF_BRIGHTLIGHT;
		else if (pev->frags)
			pev->effects |= EF_DIMLIGHT;

		if (pev->spawnflags & SF_LIGHTDYNAMIC_FLARE)
			pev->effects |= EF_LIGHT;
	}
	else
	{
		pev->effects &= ~(EF_DIMLIGHT | EF_BRIGHTLIGHT | EF_LIGHT);
	}
}

STATE CLightDynamic::GetState( void )
{
	if (pev->health)
		return STATE_ON;
	else
		return STATE_OFF;
}

//**********************************************************
//LRC- the CTriggerLightstyle entity - changes the style of a light temporarily.
//**********************************************************

class CTriggerLightstyle : public CBaseEntity
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void EXPORT Think( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

private:
	int m_iStyle;
	int m_iszPattern;
	int m_iWait;
};

LINK_ENTITY_TO_CLASS( trigger_lightstyle, CTriggerLightstyle );

TYPEDESCRIPTION	CTriggerLightstyle::m_SaveData[] = 
{
	DEFINE_FIELD( CTriggerLightstyle, m_iStyle, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerLightstyle, m_iszPattern, FIELD_STRING ),
	DEFINE_FIELD( CTriggerLightstyle, m_iWait, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE(CTriggerLightstyle,CBaseEntity);

void CTriggerLightstyle::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "style"))
	{
		m_iStyle = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pattern"))
	{
		m_iszPattern = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iWait"))
	{
		m_iWait = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}


void CTriggerLightstyle::Spawn( void )
{
}

void CTriggerLightstyle::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pTarget = NULL;
	if ( !pev->target )
		return;

	//ALERT( at_console, "Lightstyle change for: (%s)\n", STRING(pev->target) );

	for (;;)
	{
		pTarget = UTIL_FindEntityByTargetname(pTarget,STRING(pev->target));
		if (FNullEnt(pTarget))
			break;

		if (!FClassnameIs(pTarget->pev, "light") && !FClassnameIs(pTarget->pev, "light_spot") && !FClassnameIs(pTarget->pev, "env_light"))
			continue;

		CLight *pLight = (CLight*)pTarget;

		if (pLight->m_iStyle < 32) // if it's using a global style, don't change it
			continue;

		if (m_iszPattern)
			LIGHT_STYLE(pLight->m_iStyle, (char *)STRING( m_iszPattern ));
		else if (m_iStyle)
			LIGHT_STYLE(pLight->m_iStyle, GetStdLightStyle(m_iStyle));

		if (m_iWait != -1)
		{
			SetNextThink( m_iWait );
		}
	}
}

void CTriggerLightstyle::Think( void )
{
	CBaseEntity *pTarget = NULL;
	if ( !pev->target )
		return;

	//reset all targets to their former styles
	for (;;)
	{
		pTarget = UTIL_FindEntityByTargetname(pTarget, STRING(pev->target));
		if (FNullEnt(pTarget))
			break;

		if (!FClassnameIs(pTarget->pev, "light") && !FClassnameIs(pTarget->pev, "light_spot") && !FClassnameIs(pTarget->pev, "env_light"))
			continue;

		CLight *pLight = (CLight*)pTarget;

		pLight->SetCorrectStyle();
	}
}
