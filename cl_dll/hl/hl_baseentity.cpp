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

/*
==========================
This file contains "stubs" of class member implementations so that we can predict certain
 weapons client side.  From time to time you might find that you need to implement part of the
 these functions.  If so, cut it from here, paste it in hl_weapons.cpp or somewhere else and
 add in the functionality you need.
==========================
*/
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"nodes.h"
#include	"soundent.h"
#include	"effects.h"
#include	"skill.h"
#include	"gamerules.h"
#include	"dmlrocket.h"
#include	"chemgrenade.h"

// Globals used by game logic
const Vector g_vecZero = Vector( 0, 0, 0 );
int gmsgWeapPickup = 0;
enginefuncs_t g_engfuncs;
globalvars_t  *gpGlobals;

ItemInfo CBasePlayerItem::ItemInfoArray[MAX_WEAPONS];

// Gunman's weapons use these. Retail's .bss has gmsgTempX and gSkillData
// among the uninitialised globals and the rest, zero-initialised, after
// gmsgWeapPickup in this order
skilldata_t gSkillData;
int g_iSkillLevel = 1;
int gmsgSliderX = 0;
int gmsgTempX;
int gmsgZoom = 0;
int gmsgDmlCross = 0;
int gEvilImpulse101 = 0;
CGameRules *g_pGameRules = NULL;

// sic: the stubs' order is ours. Retail's code shows the order only of the
// stubs that leave a copy of their own, and that order is kept; the others
// are placed so that LINK's identical-copy sort keeps retail's copies (found
// by search, a fit like the placeholders below). The section comments stayed
// in place and no longer describe the stubs under them
struct cl_enginefuncs_s;
extern struct cl_enginefuncs_s gEngfuncs;
// sic: placeholders, ours: functions retail was linked with and dropped
// (nothing calls them); their text is unknown. They give LINK's
// identical-copy sort the (size, relocations) keys with which it keeps
// retail's copies (see Unknown_W1_1 in ev_hldm.cpp): a fit, not evidence.
void InitInput( void );

// key (16, 0)
void Unknown_top_hl_baseentity_1( void )
{
}

// key (16, 0)
void Unknown_top_hl_baseentity_2( void )
{
}


int	CBaseEntity :: Intersects( CBaseEntity *pOther ) { return 0; }
void CBaseEntity :: MakeDormant( void ) { }

// CBaseEntity Stubs
void CBasePlayer :: Precache( void ) { }
void CBaseMonster :: GibMonster( void ) { }
void CBaseMonster::SentenceStop( void ) { }
void CBaseMonster::ReportAIState( void ) { }
void CBaseMonster :: MonsterInit ( void ) { }
void CBaseMonster :: MonsterThink ( void ) { }
void CGrenade::Spawn( void ) { }
void CGrenade::BounceSound( void ) { }
void UTIL_SetOrigin( entvars_t *, const Vector &org ) { }
int CBaseEntity :: IsDormant( void ) { return 0; }
int CBaseEntity::ShouldToggle( USE_TYPE useType, BOOL currentState ) { return 0; }
int CBaseDelay::Restore( class CRestore & ) { return 1; }
void UTIL_PrecacheOther( const char *szClassname ) { }
int CBaseDelay::Save( class CSave & ) { return 1; }

// CBaseDelay Stubs
void CBaseEntity::SetObjectCollisionBox( void ) { }
int CBaseAnimating::Restore( class CRestore & ) { return 1; }
void EMIT_SOUND_DYN(edict_t *entity, int channel, const char *sample, float volume, float attenuation, int flags, int pitch) { }

// CBaseAnimating Stubs
BOOL CBasePlayerWeapon :: IsUseable( void ) { return TRUE; }
int CBaseAnimating::Save( class CSave & ) { return 1; }

// DEBUG Stubs
void CBasePlayer::AddPoints( int score, BOOL bAllowNegativeScore ) { } 
void CSprite::Expand( float scaleSpeed, float fadeSpeed ) { }

// UTIL_* Stubs
int CBasePlayerWeapon::SecondaryAmmoIndex( void ) {	return -1; }
CChemGrenade *CChemGrenade::ShootSplatter( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, chemicalmixture_t mixture ) { return NULL; }
Vector CBasePlayer :: BodyTarget( const Vector &posSrc ) { return g_vecZero; }
CChemGrenade *CChemGrenade::ShootBouncy( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, chemicalmixture_t mixture ) { return NULL; }
float CBaseMonster::ChangeYaw ( int yawSpeed ) { return 0; }
CChemGrenade *CChemGrenade::ShootSticky( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, chemicalmixture_t mixture ) { return NULL; }
Schedule_t *CBaseMonster :: GetSchedule ( void ) { return NULL; }
CBaseEntity *UTIL_FindEntityByClassname( CBaseEntity *pStartEntity, const char *szName ) { return NULL; }
void CBasePlayer::GiveNamedItem( const char *pszName ) { }
void CBasePlayer :: UpdateClientData( void ) { }
void CBaseMonster :: StartTask ( Task_t *pTask ) { }

// CBaseToggle Stubs
void CBaseMonster :: RunTask ( Task_t *pTask ) { }
void CBaseMonster :: MonsterInitDead( BOOL fKeepAnimation ) { }
void CBaseMonster :: KeyValue( KeyValueData *pkvd ) { }

// CGrenade Stubs
void CBaseMonster :: HandleAnimEvent( MonsterEvent_t *pEvent ) { }
int CBasePlayer :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType ) { return 0; }
CDmlRocket *CDmlRocket::CreateDmlRocket( const Vector &vecOrigin, const Vector &vecAngles, edict_t *pentOwner, int iFlightpath, int iPayload, int iDetonate ) { return NULL; }
void CBaseMonster :: BarnacleVictimBitten ( entvars_t *pevBarnacle ) { }
void CBasePlayer::SetAnimation( PLAYER_ANIM playerAnim ) { }

void UTIL_SetSize( entvars_t *pev, const Vector &vecMin, const Vector &vecMax ) { }
edict_t *DBG_EntOfVars( const entvars_t *pev ) { return NULL; }

void DBG_AssertFunction(BOOL fExpr,	const char*	szExpr,	const char*	szFile,	int szLine,	const char*	szMessage) { }
void UTIL_ScreenFade( CBaseEntity *pEntity, const Vector &color, float fadeTime, float fadeHold, int alpha, int flags ) { }
void UTIL_MakeAimVectors( const Vector &vecAngles ) { }
void UTIL_BloodDrips( const Vector &origin, const Vector &direction, int color, int amount ) { }
void UTIL_GunshotDecalTrace( TraceResult *pTrace, int decalNumber ) { }
void UTIL_DecalTrace( TraceResult *pTrace, int decalNumber ) { }
int CBasePlayer :: Illumination( void ) { return 0; }
BOOL UTIL_IsValidEntity( edict_t *pent ) { return TRUE; }
CBeam *CBeam::BeamCreate( const char *pSpriteName, int width ) { return NULL; }
void CBaseMonster :: Move ( float flInterval ) { }
void UTIL_Remove( CBaseEntity *pEntity ) { }
BOOL CBaseMonster :: FBecomeProne ( void ) { return TRUE; }
BOOL CBaseMonster :: ShouldFadeOnDeath( void ) { return FALSE; }
BOOL CBaseMonster :: HasAlienGibs( void ) { return FALSE; }


BOOL CBaseMonster :: FCanActiveIdle ( void ) { return FALSE; }
CBaseEntity *CBaseMonster :: BestVisibleEnemy ( void ) { return NULL; }
int CBaseMonster :: IgnoreConditions ( void ) { return 0; }
int CBaseMonster :: VisionFlags ( void ) { return 0; }
void CBasePlayer :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType) { }
void CBasePlayerAmmo::KeyValue( KeyValueData *pkvd ) { }
int CBaseEntity::Restore( CRestore &restore ) { return 1; }
void CBasePlayer :: BarnacleVictimBitten ( entvars_t *pevBarnacle ) { }
float CBaseMonster :: FLSoundVolume ( CSound *pSound ) { return 0.0; }
void CBaseToggle :: KeyValue( struct KeyValueData_s * ) { }
int CBaseEntity :: TakeHealth( float flHealth, int bitsDamageType ) { return 1; }
int CBaseEntity :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType ) { return 1; }
int	CBaseEntity :: DamageDecal( int bitsDamageType ) { return -1; }
Vector UTIL_VecToAngles( const Vector &vec ) { return g_vecZero; }
int CBaseMonster :: CanPlaySequence( BOOL fDisregardMonsterState, int interruptLevel ) { return FALSE; }
int CBasePlayer::AddPlayerItem( CBasePlayerItem *pItem ) { return FALSE; }
int CBaseMonster::Restore( class CRestore & ) { return 1; }
int CBasePlayerWeapon::Save( class CSave & ) { return 1; }
int CBasePlayerAmmo::Restore( class CRestore & ) { return 1; }
Schedule_t* CBaseMonster :: GetScheduleOfType ( int Type ) { return NULL; }
void CBasePlayer :: TraceBleed( float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType, Vector vecPos ) { }
void CBaseMonster :: TraceBleed( float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType, Vector vecPos ) { }
BOOL CBaseEntity :: FVisible ( const Vector &vecOrigin ) { return FALSE; }
int CBasePlayer::Restore( CRestore &restore ) { return 0; }
int CBasePlayer::RemovePlayerItem( CBasePlayerItem *pItem ) { return FALSE; }
int CBasePlayer::Save( CSave &save ) { return 0; }
Schedule_t *CBaseMonster::ScheduleFromName( const char *pName ) { return NULL;}
BOOL CBaseMonster :: FInViewCone ( Vector *pOrigin ) { return FALSE; }
int CBaseMonster::IRelationship ( CBaseEntity *pTarget ) { return 0; }
BOOL CBaseMonster :: FVisible ( CBaseEntity *pEntity ) { return FALSE; }
BOOL CBaseMonster :: FInViewCone ( CBaseEntity *pEntity ) { return FALSE; }
BOOL CBaseMonster :: FValidateHintType ( short sHint ) { return FALSE; }
int CBaseMonster :: CheckEnemy ( CBaseEntity *pEnemy ) { return 0; }
float UTIL_WaterLevel( const Vector &position, float minz, float maxz ) { return 0; }
BOOL CBaseMonster:: ShouldAdvanceRoute( float flWaypointDist ) { return FALSE; }
BOOL CBaseEntity :: FVisible ( CBaseEntity *pEntity ) { return FALSE; }
BOOL CBaseMonster :: FVisible ( const Vector &vecOrigin ) { return FALSE; }
int CBasePlayerWeapon::AddDuplicate( CBasePlayerItem *pOriginal ) { return 0; }
int CBasePlayerWeapon::AddToPlayer( CBasePlayer *pPlayer ) { return FALSE; }
int CBasePlayerWeapon::UpdateClientData( CBasePlayer *pPlayer ) { return 0; }
void CBaseMonster::PlaySentence( const char *pszSentence, float duration, float volume, float attenuation ) { }
int CBasePlayer :: TakeHealth( float flHealth, int bitsDamageType ) { return 0; }
void CBaseMonster :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType) { }
void CBaseMonster :: Look ( int iDistance ) { }
void CBaseMonster :: SetActivity ( Activity NewActivity ) { }
TraceResult UTIL_GetGlobalTrace( void ) { TraceResult tr; return tr; }
int CBasePlayerWeapon::ExtractAmmo( CBasePlayerWeapon *pWeapon ) { return 0; }
int CBasePlayerWeapon::ExtractClipAmmo( CBasePlayerWeapon *pWeapon ) { return 0; }	
BOOL CBasePlayer::HasNamedPlayerItem( const char *pszItemName ) { return FALSE; }
CBaseEntity * EHANDLE :: operator = (CBaseEntity *pEntity) { return NULL; }
BOOL CBaseEntity :: IsInWorld( void ) { return TRUE; }
void CBasePlayerAmmo :: DefaultTouch( CBaseEntity *pOther ) { }
int CBaseToggle::Restore( class CRestore & ) { return 1; }
int CBaseToggle::Save( class CSave & ) { return 1; }
void CGrenade::BounceTouch( CBaseEntity *pOther ) { }
int CBaseMonster::Save( class CSave & ) { return 1; }
int CBasePlayerWeapon::Restore( class CRestore & ) { return 1; }
CBaseEntity * CBaseEntity::Create( char *szName, const Vector &vecOrigin, const Vector &vecAngles, edict_t *pentOwner ) { return NULL; }
int CBasePlayerAmmo::Save( class CSave & ) { return 1; }
float CBaseMonster :: OpenDoorAndWait( entvars_t *pevDoor ) { return 0.0; }
void CBaseMonster :: CallForHelp( char *szClassname, float flDist, EHANDLE hEnemy, Vector &vecLocation ) { }
BOOL UTIL_PercentChance( int iPercent ) { return FALSE; }
BOOL CBaseMonster :: CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
CSprite *CSprite::SpriteCreate( const char *pSpriteName, const Vector &origin, BOOL animate ) { return NULL; }
BOOL CBaseMonster :: CheckMeleeAttack1 ( float flDot, float flDist ) { return FALSE; }
BOOL CBaseMonster :: CheckMeleeAttack2 ( float flDot, float flDist ) { return FALSE; }
CSound* CBaseMonster :: PBestScent ( void ) { return NULL; } 
CSound* CBaseMonster :: PBestSound ( void ) { return NULL; }
BOOL CBaseMonster :: HasRobotGibs( void ) { return FALSE; }
int  CBasePlayer::Classify ( void ) { return 0; }
CBaseEntity* CBasePlayerItem::Respawn( void ) { return NULL; }
int CBaseMonster :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType ) { return 0; }
BOOL CBaseMonster :: BuildNearestRoute ( Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist ) { return FALSE; }
void CBasePlayer::SetSuitUpdate(char *name, int fgroup, int iNoRepeatTime) { }
void CBaseMonster :: CheckAttacks ( CBaseEntity *pTarget, float flDist ) { }
void CGrenade::Killed( entvars_t *, int ) { }
void CBaseEntity::SUB_Remove( void ) { }
void CBaseMonster :: AttackHitSound ( CBaseMonster *pVictim ) { }
void CBasePlayerItem::DefaultTouch( CBaseEntity *pOther ) { }
void CBasePlayerItem::AttachToPlayer ( CBasePlayer *pPlayer ) { }
BOOL UTIL_GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon ) { return TRUE; }
int UTIL_EntitiesInBox( CBaseEntity **pList, int listMax, const Vector &mins, const Vector &maxs, int flagMask ) { return 0; }
int UTIL_PointContents( const Vector &vec ) { return 0; }
float	CBaseMonster::FlYawDiff ( void ) { return 0.0; }
int CBaseMonster :: ISoundMask ( void ) { return 0; }
void UTIL_LogPrintf(char *,...) { }
void UTIL_ClientPrintAll( int,char const *,char const *,char const *,char const *,char const *) { }
BOOL CBaseMonster :: FCanCheckAttacks ( void ) { return FALSE; }
void ClientPrint( entvars_t *client, int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 ) { }
BOOL CBaseMonster :: HasHumanGibs( void ) { return FALSE; }
Vector CBaseMonster :: ShootAtEnemy( const Vector &shootOrigin ) { return g_vecZero; }
CBaseEntity * EHANDLE :: operator -> () { return NULL; }
CDmlRocket *CDmlRocket::CreateSpiralRocket( const Vector &vecOrigin, const Vector &vecAngles, edict_t *pentOwner, int iFlightpath, int iPayload, int iDetonate ) { return NULL; }
void CBeam::PointEntInit( const Vector &start, int endIndex ) { }
void CBaseMonster :: Killed( entvars_t *pevAttacker, int iGib ) {}
void CBasePlayer::AddPointsToTeam( int score, BOOL bAllowNegativeScore ) { } 
BOOL CBasePlayer :: FBecomeProne ( void ) { return TRUE; }
void UTIL_MakeVectors( const Vector &vecAngles ) { }
BOOL CBaseMonster :: FTriangulate ( const Vector &vecStart , const Vector &vecEnd, float flDist, CBaseEntity *pTargetEnt, Vector *pApex ) { return FALSE; }
BOOL CBaseMonster :: FindCover ( Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist ) { return FALSE; }
void UTIL_TraceHull( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, int hullNumber, edict_t *pentIgnore, TraceResult *ptr ) { }
void CBaseMonster :: BarnacleVictimReleased ( void ) { }
void CBaseMonster :: MonsterInitThink ( void ) { }
void CBaseMonster :: StartMonster ( void ) { }
void CBaseAnimating :: ResetSequenceInfo ( ) { }
void CBaseMonster::CorpseFallThink( void ) { }
Vector CBasePlayer :: AutoaimDeflection( Vector &vecSrc, float flDist, float flDelta  ) { return g_vecZero; }
void CBaseMonster::FadeMonster( void ) { }
void CBaseMonster :: GibSound ( void ) { }
void CBaseMonster::BecomeDead( void ) {}
void CBaseMonster :: RunAI ( void ) {}
void CBasePlayer :: DeathSound( void ) { }
void CBasePlayer::PlayerDeathThink(void) { }
void CBasePlayer::Jump() { }
float CBaseAnimating :: StudioFrameAdvance ( float flInterval ) { return 0.0; }
void CBasePlayer::Duck( ) { }
void CBasePlayer::PreThink(void) { }
void CBasePlayer::PostThink() { }
void CBasePlayer::ImpulseCommands( ) { }
void CBasePlayer :: BarnacleVictimReleased ( void ) { }
void ClearMultiDamage(void) { }
void ApplyMultiDamage(entvars_t *pevInflictor, entvars_t *pevAttacker ) { }
void EjectBrass ( const Vector &vecOrigin, const Vector &vecVelocity, float rotation, int model, int soundtype ) { }
void CBasePlayerItem :: SetObjectCollisionBox( void ) { }
void CBasePlayerItem :: FallInit( void ) { }
void CBasePlayerItem::FallThink ( void ) { }
void CBasePlayerItem::Materialize( void ) { }
void CBasePlayerItem::AttemptToMaterialize( void ) { }
void CBasePlayerItem::DestroyItem( void ) { }
void CBasePlayerItem::Drop( void ) { }
void CBasePlayerItem::Kill( void ) { }
void CBasePlayerAmmo::Spawn( void ) { }
void CBasePlayerAmmo::Materialize( void ) { }
void CBasePlayerWeapon::RetireWeapon( void ) { }

void CSoundEnt :: InsertSound ( int iType, const Vector &vecOrigin, int iVolume, float flDuration ) { }
void RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType ) { }
Vector CBaseMonster :: GetGunPosition( void ) { return g_vecZero; }
void CBaseMonster :: Precache( void ) { }
EHANDLE :: operator int ( ) { }	// sic: no return value (retail's is a bare ret)
float	CBaseMonster::VecToYaw ( Vector vecDir ) { return 0.0; }
EHANDLE :: operator CBaseEntity *() { }	// sic: no return value either (retail calls a bare ret)
void CSprite::AnimateUntilDead( void ) { }
void CBeam::RelinkBeam( void ) { }
void CGrenade::Explode( Vector, Vector ) { }
void CGrenade::Explode( TraceResult *, int ) { }
CBaseEntity* CBaseMonster :: CheckTraceHullAttack( float flDist, int iDamage, int iDmgType ) { return NULL; }
void CBaseMonster :: Eat ( float flFullDuration ) { }
BOOL CBaseMonster :: FShouldEat ( void ) { return TRUE; }
void CBaseMonster :: Listen ( void ) { }
void CBaseMonster :: RouteClear ( void ) { }
void CBaseMonster :: RouteNew ( void ) { }
BOOL CBaseMonster :: FRouteClear ( void ) { return FALSE; }
BOOL CBaseMonster :: FRefreshRoute ( void ) { return 0; }
BOOL CBaseMonster::MoveToEnemy( Activity movementAct, float waitTime ) { return FALSE; }
BOOL CBaseMonster::MoveToLocation( Activity movementAct, float waitTime, const Vector &goal ) { return FALSE; }
BOOL CBaseMonster::MoveToTarget( Activity movementAct, float waitTime ) { return FALSE; }
BOOL CBaseMonster::MoveToNode( Activity movementAct, float waitTime, const Vector &goal ) { return FALSE; }
void CBaseMonster :: RouteSimplify( CBaseEntity *pTargetEnt ) { }
void CBaseMonster :: PushEnemy( CBaseEntity *pEnemy, Vector &vecLastKnownPos ) { }
BOOL CBaseMonster :: PopEnemy( ) { return FALSE; }
void CBaseMonster :: SetSequenceByName ( char *szSequence ) { }
void CBaseMonster :: AdvanceRoute ( float distance ) { }
int CBaseMonster :: RouteClassify( int iMoveFlag ) { return 0; }
BOOL CBaseMonster :: BuildRoute ( const Vector &vecGoal, int iMoveFlag, CBaseEntity *pTarget ) { return FALSE; }
void CBaseMonster :: InsertWaypoint ( Vector vecLocation, int afMoveFlags ) { }
void CBaseMonster :: MovementComplete( void ) { }
int CBaseMonster::TaskIsRunning( void ) { return 0; }
void CBaseMonster :: MakeIdealYaw( Vector vecTarget ) { }
int CBaseAnimating :: LookupActivity ( int activity ) { return 0; }
int CBaseAnimating :: LookupActivityHeaviest ( int activity ) { return 0; }
void CBaseMonster :: SetEyePosition ( void ) { }
int CBaseAnimating :: LookupSequence ( const char *label ) { return 0; }
BOOL CBaseAnimating :: GetSequenceFlags( ) { return FALSE; }
float CBaseAnimating :: SetBoneController ( int iController, float flValue ) { return 0.0; }
void CBaseAnimating :: DispatchAnimEvents ( float flInterval ) { }
void CBaseAnimating :: InitBoneControllers ( void ) { }
void CBaseAnimating :: GetBonePosition ( int iBone, Vector &origin, Vector &angles ) { }
void CBaseAnimating :: GetAttachment ( int iAttachment, Vector &origin, Vector &angles ) { }
int CBaseAnimating :: FindTransition( int iEndingSequence, int iGoalSequence, int *piDir ) { return -1; }
void CBaseAnimating :: GetAutomovement( Vector &origin, Vector &angles, float flInterval ) { }
void CBaseAnimating :: SetBodygroup( int iGroup, int iValue ) { }
int CBaseAnimating :: GetBodygroup( int iGroup ) { return 0; }
void CBaseMonster :: MakeDamageBloodDecal ( int cCount, float flNoise, TraceResult *ptr, const Vector &vecDir ) { }
BOOL CBaseMonster :: FGetNodeRoute ( Vector vecDest ) { return TRUE; }
int CBaseMonster :: FindHintNode ( void ) { return NO_NODE; }
BOOL CBaseMonster :: FCheckAITrigger ( void ) { return FALSE; }
BOOL CBaseMonster :: FindLateralCover ( const Vector &vecThreat, const Vector &vecViewOffset ) { return FALSE; }
Vector CBasePlayer :: GetAutoaimVector( float flDelta ) { return g_vecZero; }
BOOL CBaseMonster :: FacingIdeal( void ) { return FALSE; }
BOOL CBaseMonster :: BBoxFlat ( void ) { return TRUE; }
BOOL CBaseMonster :: GetEnemy ( void ) { return FALSE; }
CBaseEntity* CBaseMonster :: DropItem ( char *pszItemName, const Vector &vecPos, const Vector &vecAng ) { return NULL; }
void CBaseMonster :: RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType ) { }
int TrainSpeed(int iSpeed, int iMax) { 	return 0; }
void CBasePlayer::RemoveAllItems( BOOL giveFists ) { }
void CBasePlayer::WaterMove() { }
BOOL CBasePlayer::IsOnLadder( void ) { return FALSE; }
void CBasePlayer::StartDeathCam( void ) { }
float CBaseAnimating :: SetBlending ( int iBlender, float flValue ) { return 0; }
void CBasePlayer::StartObserver( Vector vecPosition, Vector vecViewAngle ) { }

void CBasePlayer::PlayerUse ( void ) { }
void CBasePlayer :: PlayStepSound(int step, float fvol) { }	
void CBasePlayer :: UpdateStepSound( void ) { }
void CBasePlayer::CheckTimeBasedDamage()  { }
void CBasePlayer :: UpdateGeigerCounter( void ) { }
float TEXTURETYPE_PlaySound(TraceResult *ptr,  Vector vecSrc, Vector vecEnd, int iBulletType) { return 0.0; }
void CBasePlayer::CheckSuitUpdate() { }
void CBasePlayer :: UpdatePlayerSound ( void ) { }
void CBasePlayer::RenewItems(void) { }
void CBasePlayer::SelectNextItem( int iItem ) { }
BOOL CBasePlayer::HasWeapons( void ) { return FALSE; }
void CBasePlayer::SelectPrevItem( int iItem ) { }
CBaseEntity *FindEntityForward( CBaseEntity *pMe ) { return NULL; }
BOOL CBasePlayer :: FlashlightIsOn( void ) { return FALSE; }
Vector CBasePlayer :: GetGunPosition( void ) { return g_vecZero; }
void CBasePlayer :: FlashlightTurnOn( void ) { }
void CBasePlayer :: FlashlightTurnOff( void ) { }
void CBasePlayer :: ForceClientDllUpdate( void ) { }
void CBasePlayer::CheatImpulseCommands( int iImpulse ) { }
void CBasePlayer::ItemPreFrame() { }
void CBasePlayer::ItemPostFrame() { }
int CBasePlayer::AmmoInventory( int iAmmoIndex ) { return -1; }
int CBasePlayer::GetAmmoIndex(const char *psz) { return -1; }
void CBasePlayer::SendAmmoUpdate(void) { }
void CBaseEntity::FireBullets(ULONG cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq, int iDamage, entvars_t *pevAttacker ) { }
void CBaseMonster :: MonsterUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) { }
void CBaseMonster::PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener ) { }
void CBaseEntity :: TraceBleed( float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType, Vector vecPos ) { }
Activity CBaseMonster :: GetDeathActivity ( void ) { return ACT_DIE_HEADSHOT; }
MONSTERSTATE CBaseMonster :: GetIdealState ( void ) { return MONSTERSTATE_ALERT; }
int CBaseEntity::Save( CSave &save ) { return 1; }
void CBaseMonster :: RadiusDamage(entvars_t* pevInflictor, entvars_t*	pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType ) { }
BOOL CBaseMonster :: CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
void CBaseEntity::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType) { }
int CBaseMonster :: CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist ) { return 0; }
void CBaseMonster::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval ) { }
void CBaseDelay :: KeyValue( struct KeyValueData_s * ) { }
CBaseEntity *CBaseEntity::GetNextTarget( void ) { return NULL; }
const char *CBasePlayer::TeamID( void ) { return ""; }
void CBasePlayer :: EnableControl(BOOL fControl) { }
void CBasePlayer :: ResetAutoaim( ) { }
void CBasePlayer :: SetCustomDecalFrames( int nFrames ) { }
int CBasePlayer :: GetCustomDecalFrames( void ) { return -1; }
void CBasePlayer::DropPlayerItem ( char *pszItemName ) { }
BOOL CBasePlayer::HasPlayerItem( CBasePlayerItem *pCheckItem ) { return FALSE; }
BOOL CBasePlayer :: SwitchWeapon( CBasePlayerItem *pWeapon )  { return FALSE; }


// Gunman's weapons call these
void AddMultiDamage( entvars_t *pevInflictor, CBaseEntity *pEntity, float flDamage, int bitsDamageType) { }
void SpawnBlood(Vector vecSpot, int bloodColor, float flDamage) { }
int DamageDecal( CBaseEntity *pEntity, int bitsDamageType ) { return 0; }
void DecalGunshot( TraceResult *pTrace, int iBulletType ) { }
void AddAmmoNameToAmmoRegistry( const char *szAmmoname ) { }
int CBasePlayerItem::Restore( class CRestore & ) { return 1; }
int CBasePlayerItem::Save( class CSave & ) { return 1; }
void CBasePlayerItem :: CheckRespawn ( void ) { }
int CBasePlayerItem::AddToPlayer( CBasePlayer *pPlayer ) { return TRUE; }
void CBasePlayerItem::Holster( int skiplocal ) { }
BOOL CBasePlayerWeapon :: AddPrimaryAmmo( int iCount, char *szName, int iMaxClip, int iMaxCarry ) { return TRUE; }
int CBasePlayer :: GiveAmmo( int iCount, char *szName, int iMax ) { return 0; }
int CBasePlayerWeapon::PrimaryAmmoIndex( void ) { return -1; }
int CBaseMonster :: TakeHealth (float flHealth, int bitsDamageType) { return 0; }
BOOL CBasePlayerWeapon :: AddSecondaryAmmo( int iCount, char *szName, int iMax ) { return TRUE; }
CBaseEntity* CBasePlayerAmmo::Respawn( void ) { return this; }

// sic: placeholders, ours: functions retail was linked with and dropped
// (nothing calls them); their text is unknown. They give LINK's
// identical-copy sort the (size, relocations) keys with which it keeps
// retail's copies (see Unknown_W1_1 in ev_hldm.cpp): a fit, not evidence.
void InitInput( void );

// key (16, 3)
void Unknown_end_hl_baseentity_1( void )
{
	InitInput();
	InitInput();
	InitInput();
}

// key (32, 2)
int Unknown_end_hl_baseentity_2( int *p )
{
	InitInput();
	InitInput();
	p[0] = 1;
	return 0;
}

// key (16, 1)
void Unknown_end_hl_baseentity_3( void )
{
	InitInput();
}

// key (128, 9)
int Unknown_end_hl_baseentity_4( int *p )
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
	p[0] = 1;
	p[1] = 2;
	p[2] = 3;
	p[3] = 4;
	p[4] = 5;
	p[5] = 6;
	p[6] = 7;
	p[7] = 8;
	p[8] = 9;
	return 0;
}
