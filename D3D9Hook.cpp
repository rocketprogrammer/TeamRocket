#include "D3D9Hook.h"

#include <d3d9.h>
#include <windows.h>

#pragma comment(lib, "d3d9.lib")

namespace
{
    LRESULT CALLBACK DummyWndProc(HWND h, UINT m, WPARAM w, LPARAM l)
    {
        return DefWindowProc(h, m, w, l);
    }
}

void* GetD3D9EndSceneAddress()
{
    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DummyWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "TRDummyD3D";
    if (!RegisterClassExA(&wc))
        return nullptr;

    HWND hwnd = CreateWindowA(wc.lpszClassName, "", WS_OVERLAPPEDWINDOW,
                              0, 0, 1, 1, nullptr, nullptr, wc.hInstance,
                              nullptr);
    if (!hwnd)
    {
        UnregisterClassA(wc.lpszClassName, wc.hInstance);
        return nullptr;
    }

    void* endScene = nullptr;
    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (d3d)
    {
        D3DPRESENT_PARAMETERS pp = {0};
        pp.Windowed = TRUE;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.hDeviceWindow = hwnd;

        IDirect3DDevice9* device = nullptr;
        HRESULT hr = d3d->CreateDevice(
            D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
            &pp, &device);

        if (FAILED(hr) || !device)
        {
            pp.Windowed = FALSE;
            hr = d3d->CreateDevice(
                D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                D3DCREATE_SOFTWARE_VERTEXPROCESSING |
                    D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
                &pp, &device);
        }

        if (SUCCEEDED(hr) && device)
        {
            void** vtable = *reinterpret_cast<void***>(device);
            endScene = vtable[42];
            device->Release();
        }
        d3d->Release();
    }

    DestroyWindow(hwnd);
    UnregisterClassA(wc.lpszClassName, wc.hInstance);
    return endScene;
}
