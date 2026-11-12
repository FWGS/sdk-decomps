/***
*
*	Counter-Strike Beta 4.0, shield.cpp: the shield (weapon_shield).
*
*	knife.cpp's text cut down: the knife's class with its swing members
*	(retail's CShield is the knife's 196 bytes, a TraceResult at 0x8c), the
*	knife's view and world models and a shield for the player model, no
*	attack. No code of Beta 4.0 names weapon_shield (W_Precache does not
*	precache it, nothing gives or sells it). File, class and method
*	names are Beta 4.0's (Linux symbols); the animation names, SHIELD_
*	macros and member names are ours.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"


class CShield : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	BOOL Deploy( void );
	void Holster( void );
	int m_iSwing;
	TraceResult m_trHit;
};
LINK_ENTITY_TO_CLASS( weapon_shield, CShield );



enum shield_e {
	SHIELD_IDLE = 0,
	SHIELD_ATTACK1,
	SHIELD_ATTACK2,
	SHIELD_DRAW
};


void CShield::Spawn( )
{
	Precache( );
	m_iId = WEAPON_SHIELD;
	SET_MODEL(ENT(pev), "models/w_knife.mdl");	// sic: the knife's
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CShield::Precache( void )
{
	PRECACHE_MODEL("models/v_knife.mdl");
	PRECACHE_MODEL("models/w_knife.mdl");
	PRECACHE_MODEL("models/p_shield.mdl");
}

int CShield::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 2;
	p->iId = WEAPON_SHIELD;
	p->iWeight = SHIELD_WEIGHT;
	return 1;
}



BOOL CShield::Deploy( )
{
	g_engfuncs.pfnSetClientMaxspeed( ENT(m_pPlayer->pev), 200 );
	return DefaultDeploy( "models/v_knife.mdl", "models/p_shield.mdl", SHIELD_DRAW, "shield" );
}

void CShield::Holster( )
{
	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;
}


void CShield::PrimaryAttack()
{
}
