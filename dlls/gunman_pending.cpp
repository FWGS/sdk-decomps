//=========================================================
// Data and globals that SDK files use but that retail defines in Gunman's
// own source files. They move to those files once they are reconstructed.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"weapons.h"
#include	"player.h"

// Gib models, by the iGibModel and iPrecacheGib fields of gMonsterTypes.
gibmodel_t gGibModels[] =
{
	{ "models/germangibs.mdl", 0, 0, 0, 0 },
	{ "models/hgibs.mdl", 0, 0, 0, 0 },
	{ "models/agibs.mdl", 1, 1, 1, 1 },
	{ "models/rgibs.mdl", 1, 1, 1, 1 },
	{ "models/raptorgibs.mdl", 1, 1, 1, 1 },
	{ "models/ouranogibs.mdl", 1, 1, 2, 1 },
	{ "models/pentagibs.mdl", 0, 1, 2, 1 },
	{ "models/scorpgibs.mdl", 3, 1, 1, 0 },
	{ "models/rustbitgibs.mdl", 1, 2, 1, 2 },
	{ "models/rustbotgibs.mdl", 1, 1, 2, 2 },
	{ "models/gunnergibs.mdl", 1, 2, 2, 0 },
	{ "models/Beakgib.mdl", 1, 0, 0, 0 },
	{ "models/critterparts.mdl", 1, 2, 1, 4 },
	{ "models/tubegibs.mdl", 2, 2, 0, 0 },
	{ "models/xenomegibs.mdl", 1, 1, 2, 1 },
	{ "sprites/nullsprite.spr", 0, 0, 0, 0 },
};

// Per-species data, by CBaseMonster::MonsterType().
monstertype_t gMonsterTypes[] =
{
	{ 15, 0, -1, 15, -1, -1, 431, 0, 0 },	// 0
	{ 1, 247, 76, 15, 5, 5, 431, 4, VIEW_FIELD_WIDE },	// 1
	{ 0, 156, 59, 15, 1, 1, 431, 4, VIEW_FIELD_WIDE },	// 2
	{ 1, 247, 76, 15, 5, 5, 431, 4, VIEW_FIELD_WIDE },	// 3
	{ 0, 156, 59, 15, 1, 1, 431, 4, VIEW_FIELD_WIDE },	// 4
	{ 15, 195, 67, 15, -1, -1, 386, 4, VIEW_FIELD_WIDE },	// 5
	{ 15, 247, 76, 15, -1, -1, 386, 4, VIEW_FIELD_WIDE },	// 6
	{ 15, 247, -1, 5, 5, 5, 386, 4, VIEW_FIELD_WIDE },	// 7
	{ 15, 247, 76, 4, 5, 5, 386, 4, VIEW_FIELD_WIDE },	// 8
	{ 15, 247, -1, 15, 5, 5, 386, 4, VIEW_FIELD_WIDE },	// 9
	{ 15, 200, 51, 15, -1, -1, 386, 4, VIEW_FIELD_WIDE },	// 10
	{ 15, 200, 51, 7, -1, -1, 386, 4, VIEW_FIELD_WIDE },	// 11
	{ 3, 156, 59, 8, 1, 1, 392, 4, VIEW_FIELD_WIDE },	// 12
	{ 3, 156, 59, 9, 1, 1, 392, 4, VIEW_FIELD_WIDE },	// 13
	{ 3, 156, 59, 15, 1, 1, 392, 4, VIEW_FIELD_WIDE },	// 14
	{ 2, 195, 60, 11, 2, 2, 395, 4, VIEW_FIELD_WIDE },	// 15
	{ 15, 100, -1, 15, 6, 6, 395, 4, VIEW_FIELD_WIDE },	// 16
	{ 2, 100, 58, 12, 3, 3, 395, 4, VIEW_FIELD_WIDE },	// 17
	{ 2, 50, 57, 13, 4, 4, 395, 4, VIEW_FIELD_WIDE },	// 18
	{ 2, 100, 58, 14, 3, 3, 395, 4, VIEW_FIELD_WIDE },	// 19
	{ 1, 247, 76, 1, 5, 5, 431, 4, VIEW_FIELD_WIDE },	// 20
	{ 15, 195, 74, 15, -1, -1, 386, 4, VIEW_FIELD_WIDE },	// 21
	{ 15, 156, -1, 15, -1, -1, 431, 4, VIEW_FIELD_WIDE },	// 22
	{ 1, 247, 76, 15, 5, 5, 431, 4, VIEW_FIELD_WIDE },	// 23
	{ 15, 100, -1, 15, 6, 6, 395, 4, VIEW_FIELD_WIDE },	// 24
	{ 1, 247, 76, 15, 5, 5, 431, 4, VIEW_FIELD_WIDE },	// 25
	{ 0, 156, 59, 15, 1, 1, 431, 4, VIEW_FIELD_WIDE },	// 26
};

// gib and gore spray sprites, precached by the species that use them
int g_iGibHuman;
int g_iGoreHuman;
int g_iGibBot;
int g_iGoreBot;
int g_iGibBirth;
int g_iGoreBeak;
int g_iGibBeak;
int g_iGoreBirth;
int g_iGoreTube;
int g_iGibTube;
int g_iGoreXeno;
int g_iGibXeno;


//=========================================================
// provisional, for gunner.cpp to link: the rust robots' base
// and the gunner's mortar belong to the rust flier job
// (rust.h, rust.cpp). Remove this block when merging it.
//=========================================================
#include	"squadmonster.h"

class CBaseRust : public CSquadMonster
{
public:
	void Precache( void );
	void GibMonster( void );
	void CallForHelp( char *szClassname, float flDist, EHANDLE hEnemy, Vector &vecLocation );

	void SpeakSentence( void );
	void RustInit( void );
	void JustSpoke( void );
	BOOL FOkToSpeak( void );

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int		m_iVoicePitch;
	int		m_iSentence;
	int		m_iSentenceVolume;
	int		m_iModelIndexBotDeath;
	float	m_flSpawnTime;
};

class CGunnerMortar : public CBaseEntity
{
public:
	static CGunnerMortar *MortarCreate( void );
};

TYPEDESCRIPTION	CBaseRust::m_SaveData[] =
{
	DEFINE_FIELD( CBaseRust, m_iVoicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseRust, m_iSentence, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseRust, m_iModelIndexBotDeath, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CBaseRust, CSquadMonster );

void CBaseRust :: RustInit( void )
{
	m_flSpawnTime = gpGlobals->time;
}

void CBaseRust :: Precache( void )
{
	m_iModelIndexBotDeath = PRECACHE_MODEL( "sprites/botdeath.spr" );
	g_iGoreBot = PRECACHE_MODEL( "sprites/gorebot.spr" );
	g_iGibBot = PRECACHE_MODEL( "sprites/gibbot.spr" );

	CBaseMonster::Precache( );
}

void CBaseRust :: GibMonster( void )
{
	CBaseMonster::GibMonster( );
}

void CBaseRust :: CallForHelp( char *szClassname, float flDist, EHANDLE hEnemy, Vector &vecLocation )
{
	CBaseEntity *pEntity = NULL;

	while ((pEntity = UTIL_FindEntityByClassname( pEntity, szClassname )) != NULL)
	{
		float d = (pev->origin - pEntity->pev->origin).Length();
		if (d < flDist)
		{
			CBaseMonster *pMonster = pEntity->MyMonsterPointer( );
			if (pMonster)
			{
				pMonster->m_hEnemy = hEnemy;
				pMonster->m_afMemory |= bits_MEMORY_PROVOKED;
			}
		}
	}
}

// nothing calls the gunner's ShootMortar; the real one makes a gunner_mortar
CGunnerMortar *CGunnerMortar :: MortarCreate( void )
{
	return NULL;
}
