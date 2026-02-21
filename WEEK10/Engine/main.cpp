#include "pch.h"
#include "Core/Public/ClientApp.h"

extern "C" {
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

extern "C" {
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    FClientApp Client;

#if !WITH_EDITOR
    if (lpCmdLine && strlen(lpCmdLine) > 0)
    {
        Client.SetScenePath(lpCmdLine);
    }
#else
    UNREFERENCED_PARAMETER(lpCmdLine);
#endif

    return Client.Run(hInstance, nShowCmd);
}
