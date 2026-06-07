#include "Overlay.h"

#include "Console.h"
#include "D3D9Hook.h"
#include "Injector.h"
#include "Scripts.h"

#include <string>

#include <d3d9.h>
#include <GL/gl.h>

#ifndef GL_VERTEX_PROGRAM_ARB
#define GL_VERTEX_PROGRAM_ARB 0x8620
#endif
#ifndef GL_FRAGMENT_PROGRAM_ARB
#define GL_FRAGMENT_PROGRAM_ARB 0x8804
#endif
#ifndef GL_VERTEX_PROGRAM_NV
#define GL_VERTEX_PROGRAM_NV 0x8620
#endif
#ifndef GL_FRAGMENT_PROGRAM_NV
#define GL_FRAGMENT_PROGRAM_NV 0x8870
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif
#ifndef GL_ARRAY_BUFFER_BINDING
#define GL_ARRAY_BUFFER_BINDING 0x8894
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER_BINDING
#define GL_ELEMENT_ARRAY_BUFFER_BINDING 0x8895
#endif

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_opengl2.h"
#include "imgui/backends/imgui_impl_dx9.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "MinHook.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#pragma comment(lib, "opengl32.lib")

namespace
{
    using PTR_SwapBuffers = BOOL(WINAPI*)(HDC);
    using PTR_EndScene = HRESULT(APIENTRY*)(IDirect3DDevice9*);

    PTR_SwapBuffers oWglSwapBuffers = nullptr;
    PTR_SwapBuffers oGdiSwapBuffers = nullptr;
    PTR_EndScene oEndScene = nullptr;

    enum class GLSource { Unclaimed, Wgl, Gdi };
    GLSource g_glSource = GLSource::Unclaimed;

    char g_codeBuffer[1 << 16] = {0};
    std::string g_log = "[*] Welcome to the Team Rocket Injector!\n";

    BOOL WINAPI hkWglSwapBuffers(HDC hdc)
    {
        if (g_glSource == GLSource::Unclaimed)
            g_glSource = GLSource::Wgl;
        if (g_glSource == GLSource::Wgl)
            Overlay::Get().OnPresentGL(hdc);
        return oWglSwapBuffers(hdc);
    }

    BOOL WINAPI hkGdiSwapBuffers(HDC hdc)
    {
        if (g_glSource == GLSource::Unclaimed)
            g_glSource = GLSource::Gdi;
        if (g_glSource == GLSource::Gdi)
            Overlay::Get().OnPresentGL(hdc);
        return oGdiSwapBuffers(hdc);
    }

    HRESULT APIENTRY hkEndScene(IDirect3DDevice9* device)
    {
        Overlay::Get().OnPresentD3D9(device);
        return oEndScene(device);
    }

    LRESULT CALLBACK WndProcTrampoline(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
    {
        return Overlay::Get().OnWndProc(hwnd, msg, w, l);
    }

    void RunScript(const char* code)
    {
        Injector::Get().Execute(code);
    }
}

Overlay& Overlay::Get()
{
    static Overlay instance;
    return instance;
}

void Overlay::Attach()
{
    if (m_attached)
        return;

    if (MH_Initialize() != MH_OK)
        return;

    for (int i = 0; i < 600; ++i)
    {
        if (!m_glHooked)
        {
            HMODULE gl = GetModuleHandleA("opengl32.dll");
            void* wglSwap = gl ? reinterpret_cast<void*>(
                                     GetProcAddress(gl, "wglSwapBuffers"))
                               : nullptr;
            if (wglSwap &&
                MH_CreateHook(wglSwap, &hkWglSwapBuffers,
                              reinterpret_cast<void**>(&oWglSwapBuffers)) == MH_OK)
            {
                if (MH_EnableHook(wglSwap) == MH_OK)
                    m_glHooked = true;
            }

            HMODULE gdi = GetModuleHandleA("gdi32.dll");
            void* gdiSwap = gdi ? reinterpret_cast<void*>(
                                      GetProcAddress(gdi, "SwapBuffers"))
                                : nullptr;
            if (gdiSwap &&
                MH_CreateHook(gdiSwap, &hkGdiSwapBuffers,
                              reinterpret_cast<void**>(&oGdiSwapBuffers)) == MH_OK)
            {
                if (MH_EnableHook(gdiSwap) == MH_OK)
                    m_glHooked = true;
            }
        }

        if (!m_d3dHooked && GetModuleHandleA("d3d9.dll"))
        {
            void* endScene = GetD3D9EndSceneAddress();
            if (endScene &&
                MH_CreateHook(endScene, &hkEndScene,
                              reinterpret_cast<void**>(&oEndScene)) == MH_OK)
            {
                MH_EnableHook(endScene);
                m_d3dHooked = true;
            }
        }

        if (m_glHooked || m_d3dHooked)
            break;

        Sleep(500);
    }

    m_attached = true;
}

void Overlay::Detach()
{
    if (!m_attached)
        return;

    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    if (m_window && m_originalWndProc)
        SetWindowLongPtr(m_window, GWLP_WNDPROC,
                         reinterpret_cast<LONG_PTR>(m_originalWndProc));

    if (m_imguiReady)
    {
        if (m_backend == Backend::OpenGL)
            ImGui_ImplOpenGL2_Shutdown();
        else if (m_backend == Backend::D3D9)
            ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        m_imguiReady = false;
    }

    m_attached = false;
}

void Overlay::InitCommon(HWND hwnd)
{
    m_window = hwnd;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ApplyTheme();
    ImGui_ImplWin32_Init(hwnd);
    HookWindow(hwnd);

    m_imguiReady = true;
}

void Overlay::HookWindow(HWND hwnd)
{
    m_originalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(
        hwnd, GWLP_WNDPROC,
        reinterpret_cast<LONG_PTR>(WndProcTrampoline)));
}

void Overlay::ApplyTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.WindowPadding = ImVec2(12, 12);
    style.FramePadding = ImVec2(8, 5);
    style.ItemSpacing = ImVec2(8, 8);

    ImVec4* c = style.Colors;
    const ImVec4 bg = ImVec4(0.12f, 0.12f, 0.12f, 0.98f);
    const ImVec4 panel = ImVec4(0.18f, 0.18f, 0.19f, 1.00f);
    const ImVec4 accent = ImVec4(0.84f, 0.20f, 0.20f, 1.00f);
    const ImVec4 accentHover = ImVec4(0.94f, 0.28f, 0.28f, 1.00f);

    c[ImGuiCol_WindowBg] = bg;
    c[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    c[ImGuiCol_PopupBg] = panel;
    c[ImGuiCol_Border] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    c[ImGuiCol_FrameBg] = panel;
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.24f, 0.26f, 1.00f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    c[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    c[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    c[ImGuiCol_Button] = ImVec4(0.24f, 0.24f, 0.25f, 1.00f);
    c[ImGuiCol_ButtonHovered] = accentHover;
    c[ImGuiCol_ButtonActive] = accent;
    c[ImGuiCol_Header] = accent;
    c[ImGuiCol_HeaderHovered] = accentHover;
    c[ImGuiCol_HeaderActive] = accent;
    c[ImGuiCol_CheckMark] = accent;
    c[ImGuiCol_SliderGrab] = accent;
    c[ImGuiCol_Text] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
}

void Overlay::OnPresentGL(HDC hdc)
{
    if (m_backend == Backend::D3D9)
        return;

    if (!m_imguiReady)
    {
        HWND hwnd = WindowFromDC(hdc);
        if (hwnd)
            hwnd = GetAncestor(hwnd, GA_ROOT);
        if (!hwnd)
            return;
        m_backend = Backend::OpenGL;
        InitCommon(hwnd);
    }

    RECT rect = {0};
    if (!GetClientRect(m_window, &rect) ||
        rect.right <= rect.left || rect.bottom <= rect.top)
        return;

    if (!m_backendReady)
    {
        ImGui_ImplOpenGL2_Init();
        m_backendReady = true;
    }

    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::GetIO().DisplaySize = ImVec2(
        static_cast<float>(rect.right - rect.left),
        static_cast<float>(rect.bottom - rect.top));
    ImGui::GetIO().DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    ImGui::NewFrame();
    Render();
    ImGui::Render();

    typedef void(APIENTRY * PFNGLBINDBUFFER)(GLenum, GLuint);
    static PFNGLBINDBUFFER pglBindBuffer =
        reinterpret_cast<PFNGLBINDBUFFER>(wglGetProcAddress("glBindBuffer"));

    GLint prevArrayBuffer = 0;
    GLint prevElementBuffer = 0;
    if (pglBindBuffer)
    {
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevArrayBuffer);
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &prevElementBuffer);
    }

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

    if (pglBindBuffer)
    {
        pglBindBuffer(GL_ARRAY_BUFFER, 0);
        pglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CLIP_PLANE0);
    glDisable(GL_VERTEX_PROGRAM_ARB);
    glDisable(GL_FRAGMENT_PROGRAM_ARB);
    glDisable(GL_VERTEX_PROGRAM_NV);
    glDisable(GL_FRAGMENT_PROGRAM_NV);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glDepthMask(GL_FALSE);

    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    glPopClientAttrib();
    glPopAttrib();

    if (pglBindBuffer)
    {
        pglBindBuffer(GL_ARRAY_BUFFER, prevArrayBuffer);
        pglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prevElementBuffer);
    }
}

void Overlay::OnPresentD3D9(IDirect3DDevice9* device)
{
    if (m_backend == Backend::OpenGL)
        return;

    if (!m_imguiReady)
    {
        D3DDEVICE_CREATION_PARAMETERS params = {0};
        if (FAILED(device->GetCreationParameters(&params)) ||
            !params.hFocusWindow)
            return;
        m_backend = Backend::D3D9;
        InitCommon(params.hFocusWindow);
    }

    if (!m_backendReady)
    {
        if (!ImGui_ImplDX9_Init(device))
            return;
        m_backendReady = true;
    }

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    Render();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

void Overlay::Render()
{
    if (!m_visible)
        return;

    ImGui::SetNextWindowSize(ImVec2(560, 560), ImGuiCond_FirstUseEver);
    ImGui::Begin("Python Injector", &m_visible, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Scripts"))
        {
            for (int i = 0; i < kCategoryCount; ++i)
            {
                const ScriptCategory& cat = kCategories[i];
                if (ImGui::BeginMenu(cat.name))
                {
                    for (int j = 0; j < cat.count; ++j)
                    {
                        if (ImGui::MenuItem(cat.scripts[j].name))
                            RunScript(cat.scripts[j].code);
                    }
                    ImGui::EndMenu();
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Misc"))
        {
            ImGui::MenuItem("We love PyRun_SimpleStringFlags!", nullptr, false,
                            false);
            ImGui::MenuItem("Written by Rocket.", nullptr, false, false);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::TextDisabled("Code to inject");
    ImGui::InputTextMultiline(
        "##code", g_codeBuffer, sizeof(g_codeBuffer),
        ImVec2(-1.0f, 280.0f), ImGuiInputTextFlags_AllowTabInput);

    if (ImGui::Button("Execute Script", ImVec2(-1.0f, 38.0f)))
        RunScript(g_codeBuffer);

    if (ImGui::Button("Clear Code", ImVec2(-1.0f, 0)))
        g_codeBuffer[0] = '\0';

    Console::Get().Poll(g_log);

    ImGui::TextDisabled("Game Console");
    ImGui::BeginChild("##log", ImVec2(-1.0f, -1.0f), true);
    ImGui::TextUnformatted(g_log.c_str());
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();

    ImGui::End();
}

LRESULT Overlay::OnWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_KEYDOWN && wparam == VK_INSERT)
    {
        ToggleVisible();
        return 0;
    }

    if (m_visible)
    {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
            return 1;

        const ImGuiIO& io = ImGui::GetIO();

        const bool keyUp = (msg == WM_KEYUP || msg == WM_SYSKEYUP);
        const bool mouseUp =
            (msg == WM_LBUTTONUP || msg == WM_RBUTTONUP ||
             msg == WM_MBUTTONUP || msg == WM_XBUTTONUP);

        const bool keyboardMsg =
            (msg >= WM_KEYFIRST && msg <= WM_KEYLAST);
        const bool mouseMsg =
            (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST);

        if (io.WantCaptureKeyboard && keyboardMsg && !keyUp)
            return 0;
        if (io.WantCaptureMouse && mouseMsg && !mouseUp)
            return 0;
    }

    return CallWindowProc(m_originalWndProc, hwnd, msg, wparam, lparam);
}
