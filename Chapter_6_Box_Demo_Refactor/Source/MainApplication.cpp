/*  ==================================================
	Direct3D 12 Initialization Application Entry-Point
	================================================== */

#include "MyApp.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
	MyApp MainApplication(hInstance);

	if (!MainApplication.Init())
		return 0;

	return MainApplication.Run();
}
