#pragma once
#include "ConfigAmbient.h"
#include "FXHelper.h"

class CaptureHelper
{
public:
	CaptureHelper(HWND dlg, ConfigAmbient* conf, FXHelper* fhh);
	~CaptureHelper();
	void SetCaptureScreen(int mode);
	void Start();
	void Stop();
	void Restart();
	bool isDirty = false;
private:
	HANDLE dwHandle = NULL;
};

