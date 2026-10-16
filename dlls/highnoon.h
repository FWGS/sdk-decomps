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
//
// highnoon.h: High Noon, Wanted's duel mode. The file and class names
// are ours.
//

#ifndef HIGHNOON_H
#define HIGHNOON_H

//=========================================================
// CHighNoon - duels: two players fight while the others wait.
// Class name ours.
//=========================================================
class CHighNoon : public CWantedMultiplay
{
public:
	virtual void Init( void );
	virtual void Think( void );
	virtual const char *GetGameDescription( void ) { return "Wanted! High Noon"; }
	virtual int GetGameType( void ) { return GAMETYPE_HIGHNOON; }
	virtual BOOL ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] );
	virtual void ClientDisconnected( edict_t *pClient );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual BOOL FPlayerCanRespawn( CBasePlayer *pPlayer );
	virtual edict_t *GetPlayerSpawnSpot( CBasePlayer *pPlayer );
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual float FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo );
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer ) { return GR_PLR_DROP_GUN_NO; }
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer ) { return GR_PLR_DROP_AMMO_NO; }

	int m_iUnknown14[12];	// +0x14 .. +0x40 (the High Noon file names them)
};

#endif // HIGHNOON_H
