/***
*
*	Counter-Strike Beta 7.1, vehicle.cpp: the drivable vehicle (func_vehicle,
*	CFuncVehicle, declared in trains.h) and the volume its driver stands in
*	(func_vehiclecontrols, CFuncVehicleControls). Written from the SDK's
*	CFuncTrackTrain (plats.cpp) and retail's code; function names and their
*	order are Beta 7.1's Linux symbols, member and local names Counter-Strike
*	1.6's where its debug information has them. Since Beta 7.1 the six corner
*	points, the surface normal and the direction are members and Next's parts
*	are CheckTurning, CollisionDetection and TerrainFollowing, as in 1.6
*	(Beta 7.0 had them as locals of one function).
*
*	The vehicle does not follow its path_track after Find has put it on the
*	first one. The driver's Use( USE_SET ) values below 10 are the throttle
*	(forward, brake and reverse by a table of steps), 20 and 30 steer. Next
*	thinks every 0.1 s: CheckTurning keeps the wheel off walls and limits the
*	speed in a turn, wheel and throttle fall back, TerrainFollowing takes the
*	ground's normal from a trace below, CollisionDetection slides along or
*	bounces off walls ahead, and the vehicle turns towards the direction the
*	ground and the wheel give or falls when there is no ground.
*
*	Retail's oddities kept: KeyValue tests "width" without an else after
*	"length" (a length key ends as not handled); Spawn sets m_acceleration to
*	5 whatever the map said; the turn-time chain's first test is always true;
*	the second UTIL_VecToAngles' result is unused; m_flLastNormalZ is only
*	written; the save table is the train's twelve fields (none of the
*	vehicle's own members is saved); Blocked keeps Beta 7.0's lines behind a
*	return; of the six wall traces only the front left one stops at monsters
*	and only the back left one looks whether it hit another vehicle (and only
*	prints that).
*
*	CollisionDetection and Next each carry a stand-in (marked sic) for text
*	that makes no code but that VC6's inline expansion shows was there.
*
****/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"	// Counter-Strike 1.0: the image shows a header more than Beta 7.1 had (Vector::operator== stands ahead of FixupAngles2 in vehicle.obj: retail's object with this include, 163 bytes off without); which header and where is Counter-Strike 1.6's list
#include "trains.h"
#include "saverestore.h"

// The SDK's Fix and FixupAngles under new names. They stand first in the file:
// the Linux library has them ahead of Save and Restore, and retail's
// image has them after Next (their first user) but before the out-of-line
// Vector::operator==, the order VC6 gives only when their text is this early.
static float Fix2( float angle )
{
	while ( angle < 0 )
		angle += 360;
	while ( angle > 360 )
		angle -= 360;

	return angle;
}


static void FixupAngles2( Vector &v )
{
	v.x = Fix2( v.x );
	v.y = Fix2( v.y );
	v.z = Fix2( v.z );
}


TYPEDESCRIPTION	CFuncVehicle::m_SaveData[] = 
{
	DEFINE_FIELD( CFuncVehicle, m_ppath, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncVehicle, m_length, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncVehicle, m_height, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncVehicle, m_speed, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncVehicle, m_dir, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncVehicle, m_startSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncVehicle, m_controlMins, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncVehicle, m_controlMaxs, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncVehicle, m_sounds, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncVehicle, m_flVolume, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncVehicle, m_flBank, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncVehicle, m_oldSpeed, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CFuncVehicle, CBaseEntity );
LINK_ENTITY_TO_CLASS( func_vehicle, CFuncVehicle );

void CFuncVehicle :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "length"))
	{
		m_length = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	if (FStrEq(pkvd->szKeyName, "width"))	// no else: retail's
	{
		m_width = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "height"))
	{
		m_height = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "startspeed"))
	{
		m_startSpeed = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "sounds"))
	{
		m_sounds = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "volume"))
	{
		m_flVolume = (float) (atoi(pkvd->szValue));
		m_flVolume *= 0.1;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "bank"))
	{
		m_flBank = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "acceleration"))
	{
		m_acceleration = atoi(pkvd->szValue);

		if ( m_acceleration < 1 )
			m_acceleration = 1;
		else if ( m_acceleration > 10 )
			m_acceleration = 10;

		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}


void CFuncVehicle :: NextThink( float thinkTime, BOOL alwaysThink )
{
	if ( alwaysThink )
		pev->flags |= FL_ALWAYSTHINK;
	else
		pev->flags &= ~FL_ALWAYSTHINK;

	pev->nextthink = thinkTime;
}


void CFuncVehicle :: Blocked( CBaseEntity *pOther )
{
	entvars_t	*pevOther = pOther->pev;

	// Blocker is on-ground on the train
	if ( FBitSet( pevOther->flags, FL_ONGROUND ) && VARS(pevOther->groundentity) == pev )
	{
		// Counter-Strike Beta 7.1: a rider takes the vehicle's velocity. Beta 7.0's lines stay
		// behind the return: retail's inlining of Blocked is that of the function with
		// them (without them the Vector operators come out as for a function 40 to 56
		// size units shorter), and Counter-Strike 1.6 still has deltaSpeed.
		pevOther->velocity = pev->velocity;
		return;

		float deltaSpeed = fabs(pev->speed);
		if ( deltaSpeed > 50 )
			deltaSpeed = 50;
		if ( !pevOther->velocity.z )
			pevOther->velocity.z += deltaSpeed;
		return;
	}
	else
		pevOther->velocity = (pevOther->origin - pev->origin ).Normalize() * pev->dmg;

	pevOther->velocity.z += 300;
	pev->velocity = pev->velocity * 0.85;

	ALERT( at_console, "TRAIN(%s): Blocked by %s (dmg:%.2f)\n", STRING(pev->targetname), STRING(pOther->pev->classname), pev->dmg );

	UTIL_MakeVectors( pev->angles );

	Vector vFrontLeft, vFrontRight, vBackLeft, vBackRight;
	Vector forward = (gpGlobals->v_forward * -1) * (m_length / 2);
	Vector right = (gpGlobals->v_right * -1) * (m_width / 2);

	vFrontLeft = pev->origin + forward - right;
	vFrontRight = pev->origin + forward + right;
	vBackLeft = pev->origin - forward - right;
	vBackRight = pev->origin - forward + right;

	Vector vOrigin = pOther->pev->origin;

	float minx, miny, minz;
	float maxx, maxy, maxz;

	minx = min( vFrontLeft.x, vBackRight.x );
	maxx = max( vFrontLeft.x, vBackRight.x );
	miny = min( vFrontLeft.y, vBackRight.y );
	maxy = max( vFrontLeft.y, vBackRight.y );
	minz = pev->origin.z;
	maxz = pev->origin.z + ( 2 * abs( pev->mins.z - pev->maxs.z ) );

	if ( vOrigin.x < minx || vOrigin.x > maxx || vOrigin.y < miny || vOrigin.y > maxy || vOrigin.z < minz || vOrigin.z > maxz )
		pOther->TakeDamage( pev, pev, 150, DMG_CRUSH );
}


void CFuncVehicle :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( useType != USE_SET )
	{
		if ( !ShouldToggle( useType, (pev->speed != 0) ) )
			return;

		if ( pev->speed == 0 )
		{
			pev->speed = m_speed * m_dir;
			
			Next();
		}
		else
		{
			pev->speed = 0;
			pev->velocity = g_vecZero;
			pev->avelocity = g_vecZero;
			StopSound();
			SetThink( NULL );
		}
	}
	else
	{
		float delta = value;

		// below 10: the throttle (1 forward, -1 back); 20 and 30: the wheel
		if ( delta < 10 )
		{
			if ( delta < 0 && pev->speed > 145 )
				StopSound();

			float flSpeedRatio = delta;

			// delta becomes the speed as a part of the top speed; a step is added
			// by the speed's range (the constants are retail's doubles: quotients
			// such as 0.012 / 10, which is not 0.0012)
			if ( delta > 0 )
			{
				delta = pev->speed / m_speed;

				if ( pev->speed < 0 )			flSpeedRatio = m_acceleration * 0.0005 + delta + 0.005;
				else if ( pev->speed < 10 )		flSpeedRatio = m_acceleration * 0.0006 + delta + (0.015 / 7);
				else if ( pev->speed < 20 )		flSpeedRatio = m_acceleration * 0.0007 + delta + (0.02 / 6);
				else if ( pev->speed < 30 )		flSpeedRatio = m_acceleration * 0.0007 + delta + (0.025 / 6);
				else if ( pev->speed < 45 )		flSpeedRatio = m_acceleration * 0.0007 + delta + (0.02 / 5);
				else if ( pev->speed < 60 )		flSpeedRatio = m_acceleration * 0.0008 + delta + (0.019 / 5);
				else if ( pev->speed < 80 )		flSpeedRatio = m_acceleration * 0.0008 + delta + (0.018 / 4);
				else if ( pev->speed < 100 )	flSpeedRatio = m_acceleration * 0.0009 + delta + (0.017 / 4);
				else if ( pev->speed < 150 )	flSpeedRatio = m_acceleration * 0.0008 + delta + (0.016 / 6);
				else if ( pev->speed < 225 )	flSpeedRatio = m_acceleration * 0.0007 + delta + (0.016 / 7);
				else if ( pev->speed < 300 )	flSpeedRatio = m_acceleration * 0.0006 + delta + (0.015 / 8);
				else if ( pev->speed < 400 )	flSpeedRatio = m_acceleration * 0.0005 + delta + (0.013 / 9);
				else if ( pev->speed < 550 )	flSpeedRatio = m_acceleration * 0.0005 + delta + (0.012 / 10);
				else if ( pev->speed < 800 )	flSpeedRatio = m_acceleration * 0.0005 + delta + (0.011 / 12);
			}
			else if ( delta < 0 )
			{
				delta = pev->speed / m_speed;

				// braking, then reversing down to -0.35 (retail's compares)
				if ( delta > 0 )								flSpeedRatio = delta - 0.0125;
				else if ( delta > -0.05 )						flSpeedRatio = delta - 0.0075;
				else if ( delta <= 0.05 && delta > -0.1 )		flSpeedRatio = delta - 0.01;
				else if ( delta <= 0.15 && delta > -0.15 )		flSpeedRatio = delta - 0.0125;
				else if ( delta <= 0.15 && delta > -0.22 )		flSpeedRatio = delta - 0.01375;
				else if ( delta <= 0.22 && delta > -0.3 )		flSpeedRatio = delta - 0.0175;
				else if ( delta <= 0.3 )						flSpeedRatio = delta - 0.0125;
			}

			if ( flSpeedRatio > 1 )
				flSpeedRatio = 1;
			else if ( flSpeedRatio < -0.35 )
				flSpeedRatio = -0.35;

			pev->speed = m_speed * flSpeedRatio;
			Next();
			m_flAcceleratorDecay = gpGlobals->time + 0.25;
		}
		else
		{
			if ( m_flCanTurnNow < gpGlobals->time )
			{
				if ( delta == 20 )
				{
					m_iTurnAngle++;
					m_flSteeringWheelDecay = gpGlobals->time + 0.075;

					if ( m_iTurnAngle > 8 )
						m_iTurnAngle = 8;
				}
				else if ( delta == 30 )
				{
					m_iTurnAngle--;
					m_flSteeringWheelDecay = gpGlobals->time + 0.075;

					if ( m_iTurnAngle < -8 )
						m_iTurnAngle = -8;
				}

				m_flCanTurnNow = gpGlobals->time + 0.05;
			}
		}
	}
}


#define VEHICLE_STARTPITCH	60
#define VEHICLE_MAXPITCH	200
#define VEHICLE_MAXSPEED	1500	// approx max speed for sound pitch calculation (Beta 7.0: 1000)

void CFuncVehicle :: StopSound( void )
{
	// if sound playing, stop it
	if (m_soundPlaying && pev->noise)
	{
		unsigned short us_encode;
		unsigned short us_sound  = ( ( unsigned short )( m_sounds ) & 0x0007 ) << 12;

		us_encode = us_sound;

		PLAYBACK_EVENT_FULL( FEV_RELIABLE | FEV_UPDATE, edict(), m_usAdjustPitch, 0.0, 
			(float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, us_encode, 0, 1, 0 );
	}

	m_soundPlaying = 0;
}

// update pitch based on speed, start sound if not playing
// NOTE: when train goes through transition, m_soundPlaying should go to 0, 
// which will cause the looped sound to restart.

void CFuncVehicle :: UpdateSound( void )
{
	float flpitch;
	
	if (!pev->noise)
		return;

	flpitch = VEHICLE_STARTPITCH + (abs(pev->speed) * (VEHICLE_MAXPITCH - VEHICLE_STARTPITCH) / VEHICLE_MAXSPEED);

	if ( flpitch > 200 )
		flpitch = 200;

	if (!m_soundPlaying)
	{
		// play startup sound for train
		if ( m_sounds < 5 )
			EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, "plats/vehicle_brake1.wav", m_flVolume, ATTN_NORM, 0, 100);
		EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noise), m_flVolume, ATTN_NORM, 0, (int) flpitch);
		m_soundPlaying = 1;
	} 
	else
	{
		// volume 0.0 - 1.0 - 6 bits
		// m_sounds 3 bits
		// flpitch = 6 bits
		// 15 bits total

		unsigned short us_encode;
		unsigned short us_sound  = ( ( unsigned short )( m_sounds ) & 0x0007 ) << 12;
		unsigned short us_pitch  = ( ( unsigned short )( flpitch / 10.0 ) & 0x003f ) << 6;
		unsigned short us_volume = ( ( unsigned short )( m_flVolume * 40.0 ) & 0x003f );

		us_encode = us_sound | us_pitch | us_volume;

		PLAYBACK_EVENT_FULL( FEV_UPDATE, edict(), m_usAdjustPitch, 0.0, 
			(float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, us_encode, 0, 0, 0 );
	}
}


void CFuncVehicle :: CheckTurning( void )
{
	float maxspeed;
	TraceResult tr;

	bool bTurnIntoWall = false;

	// don't steer into a wall
	if ( m_iTurnAngle < 0 )
	{
		if ( pev->speed > 0 )
		{
			UTIL_TraceLine( m_vFrontRight, m_vFrontRight - (gpGlobals->v_right * 16), ignore_monsters, dont_ignore_glass, ENT(pev), &tr );

			if ( tr.flFraction != 1.0 )
				m_iTurnAngle = 1;
		}
		else if ( pev->speed < 0 )
		{
			UTIL_TraceLine( m_vBackLeft, m_vBackLeft + (gpGlobals->v_right * 16), ignore_monsters, dont_ignore_glass, ENT(pev), &tr );

			if ( tr.flFraction != 1.0 )
				m_iTurnAngle = 1;
		}
	}
	else if ( m_iTurnAngle > 0 )
	{
		if ( pev->speed > 0 )
		{
			UTIL_TraceLine( m_vFrontLeft, m_vFrontLeft + (gpGlobals->v_right * 16), ignore_monsters, dont_ignore_glass, ENT(pev), &tr );

			if ( tr.flFraction != 1.0 )
				m_iTurnAngle = -1;
		}
		else if ( pev->speed < 0 )
		{
			UTIL_TraceLine( m_vBackRight, m_vBackRight - (gpGlobals->v_right * 16), ignore_monsters, dont_ignore_glass, ENT(pev), &tr );

			if ( tr.flFraction != 1.0 )
				m_iTurnAngle = -1;
		}
	}

	// slow down in a turn
	if ( pev->speed > 0 )
	{
		int iTurnAngle = abs( m_iTurnAngle );

		if ( iTurnAngle > 4 )
		{
			if ( m_flTurnStartTime == -1 )
			{
				m_flTurnStartTime = gpGlobals->time;
				maxspeed = m_speed;
			}
			else
			{
				float flTurnTime = gpGlobals->time - m_flTurnStartTime;

				// retail's order: the first test is always true
				if ( flTurnTime >= 0.0 )
					maxspeed = m_speed * 0.98;
				else if ( flTurnTime > 0.3 )
					maxspeed = m_speed * 0.95;
				else if ( flTurnTime > 0.6 )
					maxspeed = m_speed * 0.9;
				else if ( flTurnTime > 0.8 )
					maxspeed = m_speed * 0.8;
				else if ( flTurnTime > 1 )
					maxspeed = m_speed * 0.7;
				else if ( flTurnTime > 1.2 )
					maxspeed = m_speed * 0.5;
			}
		}
		else if ( iTurnAngle > 2 )
		{
			maxspeed = m_speed * 0.9;
			m_flTurnStartTime = -1;
		}
		else
		{
			maxspeed = m_speed;
			m_flTurnStartTime = -1;
		}

		if ( maxspeed < pev->speed )
			pev->speed -= m_speed / 10;
	}
}


void CFuncVehicle :: CollisionDetection( void )
{
	TraceResult tr;
	bool bHitSomething = false;

	// sic: a stand-in for text of CollisionDetection that makes no code. Retail
	// expands the Vector operators here as a function 164 to 171 units larger
	// than the text below gets expanded (VC6 gives a function twice its own
	// size to spend on inline expansion and counts what the optimizer later
	// removes). A dead store counts 4: 41 and 42 of them give retail's bytes,
	// 40 and 43 do not (bHitSomething, a local Counter-Strike 1.6 has, counts
	// as one more: without it 42 and 43 fit). What stood here is not known;
	// Blocked shows the kind, old lines left behind a return.
	int iStandIn;
	iStandIn = 0; iStandIn = 0; iStandIn = 0; iStandIn = 0; iStandIn = 0; iStandIn = 0;
	iStandIn = 0; iStandIn = 0; iStandIn = 0; iStandIn = 0; iStandIn = 0; iStandIn = 0;
	iStandIn = 0; iStandIn = 0; iStandIn = 0; iStandIn = 0; iStandIn = 0; iStandIn = 0;
	iStandIn = 0; iStandIn = 0; iStandIn = 0; iStandIn = 0; iStandIn = 0; iStandIn = 0;
	iStandIn = 0; iStandIn = 0; iStandIn = 0; iStandIn = 0; iStandIn = 0; iStandIn = 0;
	iStandIn = 0; iStandIn = 0; iStandIn = 0; iStandIn = 0; iStandIn = 0; iStandIn = 0;
	iStandIn = 0; iStandIn = 0; iStandIn = 0; iStandIn = 0; iStandIn = 0; iStandIn = 0;

	if ( pev->speed < 0 )
	{
		UTIL_TraceLine( m_vBackLeft, m_vBackLeft + (gpGlobals->v_forward * 16), ignore_monsters, dont_ignore_glass, ENT(pev), &tr );

		if ( tr.flFraction != 1.0 )
		{
			if ( DotProduct( tr.vecPlaneNormal, gpGlobals->v_forward * -1 ) < 0.7 && tr.vecPlaneNormal.z < 0.1 )
			{
				m_vSurfaceNormal = tr.vecPlaneNormal;
				m_vSurfaceNormal.z = 0;
				pev->speed *= 0.99;
			}
			else if ( tr.vecPlaneNormal.z < 0.65 || tr.fStartSolid )
				pev->speed *= -1;
			else
				m_vSurfaceNormal = tr.vecPlaneNormal;

			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );

			if ( pHit && pHit->Classify() == CLASS_VEHICLE )
				ALERT( at_console, "I hit another vehicle\n" );
		}
		else
		{
			UTIL_TraceLine( m_vBackRight, m_vBackRight + (gpGlobals->v_forward * 16), ignore_monsters, dont_ignore_glass, ENT(pev), &tr );

			if ( tr.flFraction != 1.0 )
			{
				if ( DotProduct( tr.vecPlaneNormal, gpGlobals->v_forward * -1 ) < 0.7 && tr.vecPlaneNormal.z < 0.1 )
				{
					m_vSurfaceNormal = tr.vecPlaneNormal;
					m_vSurfaceNormal.z = 0;
					pev->speed *= 0.99;
				}
				else if ( tr.vecPlaneNormal.z < 0.65 || tr.fStartSolid )
					pev->speed *= -1;
				else
					m_vSurfaceNormal = tr.vecPlaneNormal;
			}
			else
			{
				UTIL_TraceLine( m_vBack, m_vBack + (gpGlobals->v_forward * 16), ignore_monsters, dont_ignore_glass, ENT(pev), &tr );

				if ( tr.flFraction != 1.0 )
				{
					if ( DotProduct( tr.vecPlaneNormal, gpGlobals->v_forward * -1 ) < 0.7 && tr.vecPlaneNormal.z < 0.1 )
					{
						m_vSurfaceNormal = tr.vecPlaneNormal;
						m_vSurfaceNormal.z = 0;
						pev->speed *= 0.99;
					}
					else if ( tr.vecPlaneNormal.z < 0.65 || tr.fStartSolid )
						pev->speed *= -1;
					else
						m_vSurfaceNormal = tr.vecPlaneNormal;
				}
			}
		}
	}
	else if ( pev->speed > 0 )
	{
		UTIL_TraceLine( m_vFrontLeft, m_vFrontLeft - (gpGlobals->v_forward * 16), dont_ignore_monsters, dont_ignore_glass, ENT(pev), &tr );

		if ( tr.flFraction != 1.0 )
		{
			if ( DotProduct( tr.vecPlaneNormal, gpGlobals->v_forward * -1 ) > -0.7 && tr.vecPlaneNormal.z < 0.1 )
			{
				m_vSurfaceNormal = tr.vecPlaneNormal;
				m_vSurfaceNormal.z = 0;
				pev->speed *= 0.99;
			}
			else if ( tr.vecPlaneNormal.z < 0.65 || tr.fStartSolid )
				pev->speed *= -1;
			else
				m_vSurfaceNormal = tr.vecPlaneNormal;
		}
		else
		{
			UTIL_TraceLine( m_vFrontRight, m_vFrontRight - (gpGlobals->v_forward * 16), ignore_monsters, dont_ignore_glass, ENT(pev), &tr );

			if ( tr.flFraction != 1.0 )
			{
				if ( DotProduct( tr.vecPlaneNormal, gpGlobals->v_forward * -1 ) > -0.7 && tr.vecPlaneNormal.z < 0.1 )
				{
					m_vSurfaceNormal = tr.vecPlaneNormal;
					m_vSurfaceNormal.z = 0;
					pev->speed *= 0.99;
				}
				else if ( tr.vecPlaneNormal.z < 0.65 || tr.fStartSolid )
					pev->speed *= -1;
				else
					m_vSurfaceNormal = tr.vecPlaneNormal;
			}
			else
			{
				UTIL_TraceLine( m_vFront, m_vFront - (gpGlobals->v_forward * 16), ignore_monsters, dont_ignore_glass, ENT(pev), &tr );

				if ( tr.flFraction != 1.0 )
				{
					if ( DotProduct( tr.vecPlaneNormal, gpGlobals->v_forward * -1 ) > -0.7 && tr.vecPlaneNormal.z < 0.1 )
					{
						m_vSurfaceNormal = tr.vecPlaneNormal;
						m_vSurfaceNormal.z = 0;
						pev->speed *= 0.99;
					}
					else if ( tr.vecPlaneNormal.z < 0.65 || tr.fStartSolid )
						pev->speed *= -1;
					else
						m_vSurfaceNormal = tr.vecPlaneNormal;
				}
			}
		}
	}
}


void CFuncVehicle :: TerrainFollowing( void )
{
	TraceResult tr;

	// the ground under the vehicle
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, (m_height + 48) * -1 ), ignore_monsters, dont_ignore_glass, ENT(pev), &tr );

	if ( tr.flFraction != 1.0 )
	{
		m_vSurfaceNormal = tr.vecPlaneNormal;
	}
	else if ( tr.fInWater )
	{
		m_vSurfaceNormal = Vector( 0, 0, 1 );
	}
}


void CFuncVehicle :: Next( void )
{
	Vector vGravityVector = g_vecZero;

	UTIL_MakeVectors( pev->angles );

	Vector forward = (gpGlobals->v_forward * -1) * (m_length / 2);
	Vector right = (gpGlobals->v_right * -1) * (m_width / 2);
	Vector up = gpGlobals->v_up * 16;

	m_vFrontLeft = pev->origin + forward - right + up;
	m_vFrontRight = pev->origin + forward + right + up;
	m_vFront = pev->origin + forward + up;
	m_vBackLeft = pev->origin - forward - right + up;
	m_vBackRight = pev->origin - forward + right + up;
	m_vBack = pev->origin - forward + up;

	m_vSurfaceNormal = g_vecZero;

	CheckTurning();

	float time = 0.1;

	// sic: a stand-in for text of Next that makes no code. Two more locals than
	// the text below has give retail's x87 operand order in the two cross
	// products (one leaves 8 instructions in another order, three leave 6),
	// and 12 to 28 more units of size give its choice of expanded operators:
	// with the two initialised, 1 to 5 further dead stores fit, none and 6 do
	// not. What stood here is not known. Beta 7.0's larger stand-in, its
	// declared-then-assigned vGravityVector and its gotos are not needed here.
	int iStandIn0 = 0, iStandIn1 = 0;
	iStandIn0 = 0; iStandIn1 = 0; iStandIn0 = 0;

	if ( m_flSteeringWheelDecay < gpGlobals->time )
	{
		m_flSteeringWheelDecay = gpGlobals->time + 0.1;

		if ( m_iTurnAngle < 0 )
			m_iTurnAngle++;
		else if ( m_iTurnAngle > 0 )
			m_iTurnAngle--;
	}

	if ( m_flAcceleratorDecay < gpGlobals->time )
	{
		m_flAcceleratorDecay = gpGlobals->time + 0.1;

		if ( pev->speed < 0 )
		{
			pev->speed += 20;

			if ( pev->speed > 0 )
				pev->speed = 0;
		}
		else if ( pev->speed > 0 )
		{
			pev->speed -= 20;

			if ( pev->speed < 0 )
				pev->speed = 0;
		}
	}

	if ( pev->speed == 0 )
	{
		m_iTurnAngle = 0;
		pev->avelocity = g_vecZero;
		pev->velocity = g_vecZero;

		SetThink( Next );
		NextThink( pev->ltime + time, TRUE );
		return;
	}

	TerrainFollowing();
	CollisionDetection();

	if ( m_vSurfaceNormal != g_vecZero )
	{
		// on the ground: follow the surface
		Vector vTargetAngle, vAngle;
		float vx, vy;

		m_vVehicleDirection = CrossProduct( m_vSurfaceNormal, gpGlobals->v_forward );
		m_vVehicleDirection = CrossProduct( m_vSurfaceNormal, m_vVehicleDirection );

		vTargetAngle = UTIL_VecToAngles( m_vVehicleDirection );
		vTargetAngle.y += 180;

		vAngle = pev->angles;

		if ( m_iTurnAngle != 0 )
			vTargetAngle.y += m_iTurnAngle;

		// !!!  All of this crap has to be done to make the angles not wrap around, revisit this.
		FixupAngles2( vTargetAngle );
		FixupAngles2( vAngle );

		vx = UTIL_AngleDistance( vTargetAngle.x, vAngle.x );
		vy = UTIL_AngleDistance( vTargetAngle.y, vAngle.y );

		if ( vx > 10 )
			vx = 10;
		else if ( vx < -10 )
			vx = -10;

		if ( vy > 10 )
			vy = 10;
		else if ( vy < -10 )
			vy = -10;

		pev->avelocity.y = (int)(vy * 10);
		pev->avelocity.x = (int)(vx * 10);

		m_flLaunchTime = -1;
		m_flLastNormalZ = m_vSurfaceNormal.z;
	}
	else
	{
		// in the air
		if ( m_flLaunchTime == -1 )
		{
			m_flLaunchTime = gpGlobals->time;
			vGravityVector = Vector( 0, 0, 0 );
			pev->velocity = pev->velocity * 1.5;
		}
		else
		{
			float flAirTime = gpGlobals->time - m_flLaunchTime;

			vGravityVector = Vector( 0, 0, flAirTime * -35 );

			if ( vGravityVector.z < -400 )
				vGravityVector.z = -400;
		}

		m_vVehicleDirection = gpGlobals->v_forward * -1;
	}

	UTIL_VecToAngles( m_vVehicleDirection );	// result unused in retail

	if ( m_flUpdateSound < gpGlobals->time )
	{
		UpdateSound();
		m_flUpdateSound = gpGlobals->time + 1;
	}

	if ( m_vSurfaceNormal != g_vecZero )
	{
		pev->velocity = pev->speed * m_vVehicleDirection.Normalize();
	}
	else
	{
		pev->velocity = pev->velocity + vGravityVector;
	}

	SetThink( Next );
	NextThink( pev->ltime + time, TRUE );
}


void CFuncVehicle::DeadEnd( void )
{
	// Fire the dead-end target if there is one
	CPathTrack *pTrack, *pNext;

	pTrack = m_ppath;

	ALERT( at_aiconsole, "TRAIN(%s): Dead end ", STRING(pev->targetname) );
	// Find the dead end path node
	// HACKHACK -- This is bugly, but the train can actually stop moving at a different node depending on it's speed
	// so we have to traverse the list to it's end.
	if ( pTrack )
	{
		if ( m_oldSpeed < 0 )
		{
			do
			{
				pNext = pTrack->ValidPath( pTrack->GetPrevious(), TRUE );
				if ( pNext )
					pTrack = pNext;
			} while ( pNext );
		}
		else
		{
			do
			{
				pNext = pTrack->ValidPath( pTrack->GetNext(), TRUE );
				if ( pNext )
					pTrack = pNext;
			} while ( pNext );
		}
	}

	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	if ( pTrack )
	{
		ALERT( at_aiconsole, "at %s\n", STRING(pTrack->pev->targetname) );
		if ( pTrack->pev->netname )
			FireTargets( STRING(pTrack->pev->netname), this, this, USE_TOGGLE, 0 );
	}
	else
		ALERT( at_aiconsole, "\n" );
}


void CFuncVehicle :: SetControls( entvars_t *pevControls )
{
	Vector offset = pevControls->origin - pev->oldorigin;

	m_controlMins = pevControls->mins + offset;
	m_controlMaxs = pevControls->maxs + offset;
}


BOOL CFuncVehicle :: OnControls( entvars_t *pevTest )
{
	Vector offset = pevTest->origin - pev->origin;

	if ( pev->spawnflags & SF_TRACKTRAIN_NOCONTROL )
		return FALSE;

	// Transform offset into local coordinates
	UTIL_MakeVectors( pev->angles );
	Vector local;
	local.x = DotProduct( offset, gpGlobals->v_forward );
	local.y = -DotProduct( offset, gpGlobals->v_right );
	local.z = DotProduct( offset, gpGlobals->v_up );

	if ( local.x >= m_controlMins.x && local.y >= m_controlMins.y && local.z >= m_controlMins.z &&
		 local.x <= m_controlMaxs.x && local.y <= m_controlMaxs.y && local.z <= m_controlMaxs.z )
		 return TRUE;

	return FALSE;
}


void CFuncVehicle :: Find( void )
{
	m_ppath = CPathTrack::Instance(FIND_ENTITY_BY_TARGETNAME( NULL, STRING(pev->target) ));
	if ( !m_ppath )
		return;

	entvars_t *pevTarget = m_ppath->pev;
	if ( !FClassnameIs( pevTarget, "path_track" ) )
	{
		ALERT( at_console, "func_track_train must be on a path of path_track\n" );
		m_ppath = NULL;
		return;
	}

	Vector nextPos = pevTarget->origin;
	nextPos.z += m_height;

	Vector look = nextPos;
	look.z -= m_height;
	m_ppath->LookAhead( &look, m_length, 0 );
	look.z += m_height;

	pev->angles = UTIL_VecToAngles( look - nextPos );
	// The train actually points west
	pev->angles.y += 180;

	if ( pev->spawnflags & SF_TRACKTRAIN_NOPITCH )
		pev->angles.x = 0;
    UTIL_SetOrigin( pev, nextPos );
	NextThink( pev->ltime + 0.1, FALSE );
	SetThink( Next );
	pev->speed = m_startSpeed;

	UpdateSound();
}


void CFuncVehicle :: NearestPath( void )
{
	CBaseEntity *pTrack = NULL;
	CBaseEntity *pNearest = NULL;
	float dist, closest;

	closest = 1024;

	while ((pTrack = UTIL_FindEntityInSphere( pTrack, pev->origin, 1024 )) != NULL)
	{
		// filter out non-tracks
		if ( !(pTrack->pev->flags & (FL_CLIENT|FL_MONSTER)) && FClassnameIs( pTrack->pev, "path_track" ) )
		{
			dist = (pev->origin - pTrack->pev->origin).Length();
			if ( dist < closest )
			{
				closest = dist;
				pNearest = pTrack;
			}
		}
	}

	if ( !pNearest )
	{
		ALERT( at_console, "Can't find a nearby track !!!\n" );
		SetThink(NULL);
		return;
	}

	ALERT( at_aiconsole, "TRAIN: %s, Nearest track is %s\n", STRING(pev->targetname), STRING(pNearest->pev->targetname) );
	// If I'm closer to the next path_track on this path, then it's my real path
	pTrack = ((CPathTrack *)pNearest)->GetNext();
	if ( pTrack )
	{
		if ( (pev->origin - pTrack->pev->origin).Length() < (pev->origin - pNearest->pev->origin).Length() )
			pNearest = pTrack;
	}

	m_ppath = (CPathTrack *)pNearest;

	if ( pev->speed != 0 )
	{
		NextThink( pev->ltime + 0.1, FALSE );
		SetThink( Next );
	}
}


void CFuncVehicle::OverrideReset( void )
{
	NextThink( pev->ltime + 0.1, FALSE );
	SetThink( NearestPath );
}


CFuncVehicle *CFuncVehicle::Instance( edict_t *pent )
{ 
	if ( FClassnameIs( pent, "func_vehicle" ) )
		return (CFuncVehicle *)GET_PRIVATE(pent);
	return NULL;
}


int CFuncVehicle :: Classify( void )
{
	return CLASS_VEHICLE;
}


void CFuncVehicle :: Spawn( void )
{
	if ( pev->speed == 0 )
		m_speed = 165;
	else
		m_speed = pev->speed;

	if ( !m_sounds )
		m_sounds = 3;

	ALERT( at_console, "M_speed = %f\n", m_speed );
	
	pev->speed = 0;
	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	pev->impulse = m_speed;

	m_acceleration = 5;		// whatever the "acceleration" key said
	m_dir = 1;
	m_flTurnStartTime = -1;

	if ( FStringNull(pev->target) )
		ALERT( at_console, "Vehicle with no target" );

	if ( pev->spawnflags & SF_TRACKTRAIN_PASSABLE )
		pev->solid = SOLID_NOT;
	else
		pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;
	
	SET_MODEL( ENT(pev), STRING(pev->model) );

	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( pev, pev->origin );

	// Cache off placed origin for train controls
	pev->oldorigin = pev->origin;

	m_controlMins = pev->mins;
	m_controlMaxs = pev->maxs;
	m_controlMaxs.z += 72;
// start trains on the next frame, to make sure their targets have had
// a chance to spawn/activate
	NextThink( pev->ltime + 0.1, FALSE );
	SetThink( Find );
	Precache();
}


void CFuncVehicle :: Restart( void )
{
	ALERT( at_console, "M_speed = %f\n", m_speed );

	pev->speed = 0;
	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	pev->impulse = m_speed;

	m_flTurnStartTime = -1;
	m_flUpdateSound = -1;
	m_dir = 1;
	m_pDriver = NULL;

	if ( FStringNull(pev->target) )
		ALERT( at_console, "Vehicle with no target" );

	UTIL_SetOrigin( pev, pev->oldorigin );

	STOP_SOUND( ENT(pev), CHAN_STATIC, (char*)STRING(pev->noise) );

	NextThink( pev->ltime + 0.1, FALSE );
	SetThink( Find );
}


void CFuncVehicle :: Precache( void )
{
	if (m_flVolume == 0.0)
		m_flVolume = 1.0;

	switch (m_sounds)
	{
	case 1: PRECACHE_SOUND("plats/vehicle1.wav"); pev->noise = MAKE_STRING("plats/vehicle1.wav");break;
	case 2: PRECACHE_SOUND("plats/vehicle2.wav"); pev->noise = MAKE_STRING("plats/vehicle2.wav");break;
	case 3: PRECACHE_SOUND("plats/vehicle3.wav"); pev->noise = MAKE_STRING("plats/vehicle3.wav");break; 
	case 4: PRECACHE_SOUND("plats/vehicle4.wav"); pev->noise = MAKE_STRING("plats/vehicle4.wav");break;
	case 5: PRECACHE_SOUND("plats/vehicle6.wav"); pev->noise = MAKE_STRING("plats/vehicle6.wav");break;
	case 6: PRECACHE_SOUND("plats/vehicle7.wav"); pev->noise = MAKE_STRING("plats/vehicle7.wav");break;
	default: PRECACHE_SOUND("plats/vehicle7.wav"); pev->noise = MAKE_STRING("plats/vehicle7.wav");break;
	}

	PRECACHE_SOUND("plats/vehicle_brake1.wav");
	PRECACHE_SOUND("plats/vehicle_start1.wav");

	m_usAdjustPitch = PRECACHE_EVENT( 1, "events/vehicle.sc" );
}

// This class defines the volume of space that the player must stand in to control the vehicle
class CFuncVehicleControls : public CBaseEntity
{
public:
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void Spawn( void );
	void EXPORT Find( void );
};
LINK_ENTITY_TO_CLASS( func_vehiclecontrols, CFuncVehicleControls );


void CFuncVehicleControls :: Find( void )
{
	edict_t *pTarget = NULL;

	do 
	{
		pTarget = FIND_ENTITY_BY_TARGETNAME( pTarget, STRING(pev->target) );
	} while ( !FNullEnt(pTarget) && !FClassnameIs(pTarget, "func_vehicle") );

	if ( FNullEnt( pTarget ) )
	{
		ALERT( at_console, "No vehicle %s\n", STRING(pev->target) );
		return;
	}

	CFuncVehicle *pvehicle = CFuncVehicle::Instance(pTarget);
	pvehicle->SetControls( pev );
	UTIL_Remove( this );
}


void CFuncVehicleControls :: Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL( ENT(pev), STRING(pev->model) );

	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( pev, pev->origin );
	
	SetThink( Find );
	pev->nextthink = gpGlobals->time;
}
