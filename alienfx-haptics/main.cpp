#include <windows.h>
#include "Graphics.h"
#include "DFT_gosu.h"
//#include "ConfigHandler.h"
#include "FXHelper.h"
#include "WSAudioIn.h"

Graphics* Graphika;
DFT_gosu* dftG;
FXHelper* FXproc;

const int NUMPTS = 2048;// 44100 / 15;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{

	int* freq;
	
	ConfigHaptics conf;

	FXproc = new FXHelper(&conf);
	freq = new int[conf.numbars];
	ZeroMemory(freq, conf.numbars * sizeof(int));
	Graphika = new Graphics(hInstance ,nCmdShow, freq, &conf, FXproc);

	dftG = new DFT_gosu(NUMPTS, conf.numbars, conf.res , freq);

	WSAudioIn wsa(NUMPTS, conf.inpType, Graphika, FXproc, dftG);
	wsa.startSampling();

	Graphika->start();
	wsa.stopSampling();

	FXproc->FadeToBlack();

	delete FXproc;
	delete dftG;

	delete[] freq;
	delete Graphika;

	return 1;
}
