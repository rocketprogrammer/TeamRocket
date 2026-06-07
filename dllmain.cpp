#include <windows.h>

#include "Injector.h"
#include "Overlay.h"

static DWORD WINAPI InitThread(LPVOID)
{
    Overlay::Get().Attach();

    for (int i = 0; i < 600; ++i)
    {
        if (Injector::Get().Initialize())
            break;
        Sleep(500);
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
    UNREFERENCED_PARAMETER(reserved);

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(module);
        CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
        break;

    case DLL_PROCESS_DETACH:
        Overlay::Get().Detach();
        break;
    }

    return TRUE;
}
