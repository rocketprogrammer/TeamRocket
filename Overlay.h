#pragma once

#include <windows.h>

struct IDirect3DDevice9;

class Overlay
{
public:
    enum class Backend { None, OpenGL, D3D9 };

    static Overlay& Get();

    void Attach();
    void Detach();

    void OnPresentGL(HDC hdc);
    void OnPresentD3D9(IDirect3DDevice9* device);
    LRESULT OnWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    bool IsVisible() const { return m_visible; }
    void ToggleVisible() { m_visible = !m_visible; }

private:
    Overlay() = default;

    void InitCommon(HWND hwnd);
    void HookWindow(HWND hwnd);
    void ApplyTheme();
    void Render();

    bool m_attached = false;
    bool m_glHooked = false;
    bool m_d3dHooked = false;
    bool m_imguiReady = false;
    bool m_backendReady = false;
    Backend m_backend = Backend::None;

    bool m_visible = true;
    HWND m_window = nullptr;
    WNDPROC m_originalWndProc = nullptr;
};
