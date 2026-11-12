/***
*
*	Counter-Strike Beta 5.0 (leak), c4.cpp: the C4 as a weapon (weapon_c4).
*
*	Beta 4.0 planted the bomb with the plant_c4 command (CBasePlayer::PlantC4);
*	the leak makes it an item of the grenade slot that a terrorist arms by
*	holding the trigger for three seconds inside a bomb target, with a progress
*	bar; CGrenade::ShootSatchelCharge (ggrenade.cpp) is the planted bomb as
*	before. File, class and method names are Beta 5.0's (Linux symbols); the
*	members, the animation names and the C4_ macros are ours.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

extern int gmsgBarTime;
extern void Broadcast( const char *sentence );

enum c4_e {
	C4_IDLE1 = 0,
	C4_DRAW,
	C4_DROP,
	C4_ARM,
};

class CC4 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	BOOL Deploy( void );
	void Holster( void );
	void WeaponIdle( void );
	float GetMaxSpeed( void ) { return 260; }	// Linux symbol

	bool m_fArming;			// 0x84 the trigger is held: the three seconds run (names ours)
	bool m_fPlantAnim;		// 0x85 the last 0.75 seconds' animation has started
	float m_flArmedTime;	// 0x88 when the bomb is planted
};
LINK_ENTITY_TO_CLASS( weapon_c4, CC4 );


// the backpack lies on the ground; no Precache, no classname of its own
void CC4::Spawn( )
{
	SET_MODEL( ENT(pev), "models/w_backpack.mdl" );

	pev->frame = 0;
	pev->body = 3;
	pev->sequence = 0;
	pev->framerate = 0;

	m_iId = WEAPON_C4;
	m_iDefaultAmmo = C4_DEFAULT_GIVE;

	FallInit();// get ready to fall down.

	SetThink( FallThink );
	pev->nextthink = gpGlobals->time + 0.1;
}


void CC4::Precache( void )
{
	PRECACHE_MODEL("models/v_c4.mdl");
	PRECACHE_MODEL("models/p_c4.mdl");
	PRECACHE_MODEL("models/w_backpack.mdl");

	PRECACHE_SOUND("weapons/c4_click.wav");
}

int CC4::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "C4";
	p->iMaxAmmo1 = C4_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_C4;
	p->iWeight = C4_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}


BOOL CC4::Deploy( )
{
	pev->body = 0;
	m_fArming = false;
	m_flArmedTime = 0;
	return DefaultDeploy( "models/v_c4.mdl", "models/p_c4.mdl", C4_DRAW, "trip" );
}

void CC4::Holster( )
{
	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;
	m_fArming = false;	// a holstered bomb is not being armed any more

	if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		// no more bombs
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_C4);
		SetThink( DestroyItem );
		pev->nextthink = gpGlobals->time + 0.1;
	}

	SendWeaponAnim( C4_DRAW );
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

//
// Holding the trigger inside a bomb target (the player's zone signals, bit 2)
// starts three seconds of arming with a progress bar; 0.75 seconds before the
// end the plant animation plays; leaving the target cancels it. Then the bomb
// goes down at the player's feet, 15 units ahead and 10 down from his origin
// as far as nothing is in the way.
//
void CC4::PrimaryAttack( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return;

	if (!m_fArming)
	{
		if (m_pPlayer->m_signals.GetState() & 2)
		{
			m_fArming = true;
			m_flArmedTime = gpGlobals->time + 3.0;
			m_fPlantAnim = false;
			SendWeaponAnim( C4_ARM );

			MESSAGE_BEGIN( MSG_ONE, gmsgBarTime, NULL, m_pPlayer->pev );
				WRITE_SHORT( 3 );
			MESSAGE_END();
		}
		else
		{
			ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "C4 must be activated at a Bomb Target." );
			m_flNextPrimaryAttack = gpGlobals->time + 1.0;
			return;
		}
	}
	else
	{
		if (gpGlobals->time < m_flArmedTime)
		{
			if (gpGlobals->time >= m_flArmedTime - 0.75 && !m_fPlantAnim)
			{
				m_fPlantAnim = true;
				SendWeaponAnim( C4_DROP );
				m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
			}
			else if (!(m_pPlayer->m_signals.GetState() & 2))
			{
				ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "Arming Sequence Canceled.\nC4 can only be placed at a Bomb Target." );
				m_flNextPrimaryAttack = gpGlobals->time + 1.5;
				m_fArming = false;

				MESSAGE_BEGIN( MSG_ONE, gmsgBarTime, NULL, m_pPlayer->pev );
					WRITE_SHORT( 0 );
				MESSAGE_END();

				if (m_fPlantAnim == true)
					SendWeaponAnim( C4_DRAW );
				else
					SendWeaponAnim( C4_IDLE1 );
				return;
			}
		}
		else if (m_fArming == true)
		{
			m_fArming = false;
			m_flArmedTime = 0;

			if (m_pPlayer->m_signals.GetState() & 2)
			{
				Vector vecEnd;
				TraceResult tr;

				Broadcast( "BOMBPL" );
				m_pPlayer->m_fHasC4 = false;

				vecEnd = UTIL_GetAimVector( m_pPlayer->edict(), 0 );
				vecEnd = vecEnd.Normalize();
				vecEnd = vecEnd * 15;
				vecEnd.z = -10;
				vecEnd = vecEnd + m_pPlayer->pev->origin;

				UTIL_TraceLine( vecEnd, m_pPlayer->pev->origin, ignore_monsters, ENT(pev), &tr );

				if (tr.flFraction == 1.0)
					CGrenade::ShootSatchelCharge( m_pPlayer->pev, vecEnd, Vector( 0, 0, 0 ) );
				else
					CGrenade::ShootSatchelCharge( m_pPlayer->pev, tr.vecEndPos, Vector( 0, 0, 0 ) );

				UTIL_ClientPrintAll( HUD_PRINTCENTER, "The bomb has been planted!" );
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/c4_plant.wav", 1.0, ATTN_NORM );

				m_pPlayer->pev->body = 0;
				m_pPlayer->SetBombIcon( FALSE );

				m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

				if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
				{
					RetireWeapon();
					return;
				}
			}
			else
			{
				ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "C4 must be activated at a Bomb Target." );
				m_flNextPrimaryAttack = gpGlobals->time + 1.0;
				return;
			}
		}
	}

	m_flNextPrimaryAttack = gpGlobals->time + 0.3;
	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT( 10, 15 );
}

// the trigger let go: the arming stops (WRITE_BYTE where the bar takes a short, sic)
void CC4::WeaponIdle( void )
{
	if (m_fArming == true)
	{
		m_fArming = false;
		m_flNextPrimaryAttack = gpGlobals->time + 1.0;

		MESSAGE_BEGIN( MSG_ONE, gmsgBarTime, NULL, m_pPlayer->pev );
			WRITE_BYTE( 0 );	// sic
		MESSAGE_END();

		if (m_fPlantAnim == true)
			SendWeaponAnim( C4_DRAW );
		else
			SendWeaponAnim( C4_IDLE1 );
	}

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		SendWeaponAnim( C4_DRAW );
		SendWeaponAnim( C4_IDLE1 );
	}
	else
	{
		RetireWeapon();
	}
}
