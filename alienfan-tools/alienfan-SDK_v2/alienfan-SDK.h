#pragma once

#include <string>
#include <vector>
#include <wbemidl.h>
#include <wtypes.h>

using namespace std;

#define byte BYTE

#define DEV_FLAG_AWCC		1
#define DEV_FLAG_CONTROL	4
#define DEV_FLAG_GMODE		8

namespace AlienFan_SDK {

	struct ALIENFAN_SEN_INFO {
		union {
			struct {
				byte index;
				byte type;
			};
			WORD sid; // LOBYTE - index, HIBYTE - type: 0 = ESIF, 1 = AWCC, 2 - Disk, 3 - KRDT, 4 = OHM
		};
		string name;
		BSTR instance; // for ESIF/OHM/SSD sensors
	};

	//struct ALIENFAN_COMMAND {
	//	byte com;
	//	byte sub;
	//};

	struct ALIENFAN_CONTROL {
		byte getPowerID[2];
		byte getFanRPM[2];
		byte getFanPercent[2];
		byte getFanBoost[2];
		byte setFanBoost[2];
		byte getTemp[2];
		byte getPower[2];
		byte setPower[2];
		byte getGMode[2];
		byte setGMode[2];
		byte getSysID[2];
	};

	union ALIENFAN_INTERFACE {
		struct {
			byte sub,
			     arg1,
			     arg2,
			     reserved;
		};
		DWORD args;
	};

	class Control {
	private:
		VARIANT m_instancePath;
		byte devFlags = 0;
		DWORD systemID = 0;
		//byte sysType = 0;
		int Percent(int, int);

	public:
		//VARIANT m_instancePath;
		IWbemServices* m_WbemServices = NULL, * m_OHMService = NULL, * m_DiskService = NULL;
		IWbemClassObject* m_InParamaters = NULL;
		Control();
		~Control();

		// Probe hardware, sensors, fans, power modes and fill structures.
		// Result: true - compatible hardware found, false - not found.
		bool Probe();

		// Get RPM for the fan index fanID at fans[]
		// Result: fan RPM
		int GetFanRPM(int fanID);

		// Get fan RPMs as a percent of RPM
		// Result: percent of the fan speed
		int GetFanPercent(int fanID);

		// Get boost value for the fan index fanID at fans[]. If force, raw value returned, otherwise cooked by boost
		// Result: Error or raw value if forced, otherwise cooked by boost.
		int GetFanBoost(int fanID, bool force = false);

		// Set boost value for the fan index fanID at fans[]. If force, raw value set, otherwise cooked by boost.
		// Result: value or error
		int SetFanBoost(int fanID, byte value, bool force = false);

		// Get temperature value for the sensor index TanID at sensors[]
		// Result: temperature value or error
		int GetTempValue(int TempID);

		// Unlock manual fan operations. The same as SetPower(0)
		// Result: raw value set or error
		int Unlock();

		// Set system power profile to defined power code (NOT level)
		// Result: raw value set or error
		int SetPower(byte level);

		// Get current system power value index at powers[]
		// Result: power value index in powers[] or error
		int GetPower();

		// Set system GPU limit level (0 - no limit, 3 - min. limit)
		// Result: success or error
		//int SetGPU(int power);

		// Toggle G-mode on some systems
		int SetGMode(bool state);

		// Check G-mode state
		int GetGMode();

		// Return current device capability
		inline byte GetDeviceFlags() { return devFlags; };

		// Return current device ID
		inline DWORD GetSystemID() { return systemID; };

		// Call custom Alienware method trough WMI
		int CallWMIMethod(const byte* com, byte arg1 = 0, byte arg2 = 0);

		// Arrays of sensors, fans, max. boosts and power values detected at Probe()
		vector<ALIENFAN_SEN_INFO> sensors;
		vector<byte> fans;
		vector<byte> boosts;
		vector<WORD> maxrpm;
		vector<byte> powers;

		IWbemClassObject* m_AWCCGetObj = NULL;
	};

	//class Lights {
	//private:
	//	bool activated = false;
	//public:
	//	Lights(Control *ac);

	//	// Resets light subsystem
	//	bool Reset();

	//	// Prepare for operations
	//	bool Prepare();

	//	// Update lights state (end operation)
	//	bool Update();

	//	// Set color of lights mask defined by id to RGB
	//	bool SetColor(byte id, byte r, byte g, byte b);

	//	// Set light system mode (brightness, ???)
	//	bool SetMode(byte mode, bool onoff);

	//	// Return color subsystem availability
	//	bool IsActivated();
	//};
}
