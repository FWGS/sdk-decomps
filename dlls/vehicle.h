//=========================================================
// Gunman's drivable tank, as far as the SDK files need it.
// Provisional: completed when the vehicle code is reconstructed.
//=========================================================
#ifndef VEHICLE_H
#define VEHICLE_H

class CVehicleTank : public CBaseMonster
{
public:
	void EjectPlayer( void );

	CBasePlayer	*m_pPlayer;		// the driver
};

#endif // VEHICLE_H
