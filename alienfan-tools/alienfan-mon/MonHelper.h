#pragma once
#include "alienfan-SDK.h"
#include "ConfigFan.h"
#include "ThreadHelper.h"

class MonHelper {
public:
	ThreadHelper* monThread = NULL;
	short oldPower = -1, oldGmode = 0;
	vector<short> fanRpm, boostRaw, boostSets, fanSleep;
	map<WORD, short> senValues, maxTemps;
	vector<map<WORD, short>> senBoosts;

	MonHelper();
	~MonHelper();
	void Start();
	void Stop();
	void SetCurrentGmode(WORD newMode);
};

