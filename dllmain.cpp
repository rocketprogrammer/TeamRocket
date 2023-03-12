#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iterator>
#include <streambuf>
#include <iomanip>

template <typename T>
T GetFunctionPtr(HMODULE module, const char *name)
{
  T func = (T)GetProcAddress(module, name);
  if (NULL == func)
  {
    std::cout << "An error occured! Could not find " << name << "!"
              << std::endl;
  }

  return func;
}

#define GET_FUNC_PTR(module, name) \
  name = GetFunctionPtr<PTR_##name>(module, #name)

//---------------------------------------------------------
// Typedefs

typedef int (*PTR_PyRun_SimpleStringFlags)(const char *command, void *flags);
PTR_PyRun_SimpleStringFlags PyRun_SimpleStringFlags = NULL;

typedef void *(*PTR_PyRun_String)(const char *str,
                                  int start,
                                  void *globals,
                                  void *locals);
PTR_PyRun_String PyRun_String = NULL;

// Thread stuff
typedef void (*PTR_PyEval_InitThreads)();
PTR_PyEval_InitThreads PyEval_InitThreads = NULL;

// Global Interpreter Lock (GIL)
enum PyGILState_STATE
{
  PyGILState_LOCKED,
  PyGILState_UNLOCKED
};

typedef enum PyGILState_STATE (*PTR_PyGILState_Ensure)(void);
PTR_PyGILState_Ensure PyGILState_Ensure = NULL;

typedef void (*PTR_PyGILState_Release)(enum PyGILState_STATE);
PTR_PyGILState_Release PyGILState_Release = NULL;

// #define Py_file_input 257
// #define Py_eval_input 258

HMODULE g_PythonDLL = NULL;

HINSTANCE MainHInstance;
HANDLE CurrentProcess;

#define DLL_EXPORT extern "C" __declspec(dllexport)

DLL_EXPORT bool executeCode(const char *code)
{
  if (!(GET_FUNC_PTR(g_PythonDLL, PyRun_SimpleStringFlags)))
  {
    return FALSE;
  }

  if (!(GET_FUNC_PTR(g_PythonDLL, PyRun_String)))
  {
    return FALSE;
  }

  // GIL
  if (!(GET_FUNC_PTR(g_PythonDLL, PyGILState_Ensure)))
  {
    return FALSE;
  }

  if (!(GET_FUNC_PTR(g_PythonDLL, PyGILState_Release)))
  {
    return FALSE;
  }

  PyGILState_STATE state = PyGILState_Ensure();
  PyRun_SimpleStringFlags(code, NULL);
  PyGILState_Release(state);

  return TRUE;
}

bool Initialize()
{
  if (!(GET_FUNC_PTR(g_PythonDLL, PyRun_SimpleStringFlags)))
  {
    return FALSE;
  }

  if (!(GET_FUNC_PTR(g_PythonDLL, PyRun_String)))
  {
    return FALSE;
  }

  // GIL
  if (!(GET_FUNC_PTR(g_PythonDLL, PyGILState_Ensure)))
  {
    return FALSE;
  }

  if (!(GET_FUNC_PTR(g_PythonDLL, PyGILState_Release)))
  {
    return FALSE;
  }
};

void InjectCode(char *payload)
{
  PyGILState_STATE state = PyGILState_Ensure();

  PyRun_SimpleStringFlags(payload, NULL);
  PyGILState_Release(state);
};

LRESULT CreateInjectWindows(HWND hWndParent, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  HMODULE ModuleHandle;
  HWND InjectTextWindowhWnd;
  HWND SubmitCodehWnd;

  // These are the ids for our sub menus.
  int SubmitCodeButtonId = 1;
  int InjectTextWindowId = 2;

  char String[65536];

  switch (Msg)
  {
  case 1:
    // Get our module handle.
    ModuleHandle = GetModuleHandleA(0);

    // Create our sumbit injection code button.
    SubmitCodehWnd = CreateWindowExA(0, "Button", "Submit Code", 0x50000001, 0, 250, 500, 250, hWndParent, (HMENU)SubmitCodeButtonId, ModuleHandle, 0);

    // Create our text window so users can input code to inject,
    InjectTextWindowhWnd = CreateWindowExA(0, "Edit", "# Welcome to the Team Rocket Python Injector! #", 0x50300004, 0, 0, 500, 250, hWndParent, (HMENU)InjectTextWindowId, ModuleHandle, 0);

    // Show both of the windows we created.
    ShowWindow(SubmitCodehWnd, 1);
    ShowWindow(InjectTextWindowhWnd, 1);

  case 273:
    if (wParam != 1)
    {
      return 0;
    }

    // Get our code to compile and inject from our text window.
    GetDlgItemTextA(hWndParent, InjectTextWindowId, String, 100000);

    // Perform our python injection with our code to inject.
    InjectCode(String);

    // Show a message box containing the code we injected.
    MessageBoxA(0, String, nullptr, 0);

    return 0;

  case 2:
    PostQuitMessage(0);

  case 16:
    return 0;

  default:
    return DefWindowProcA(hWndParent, Msg, wParam, lParam);
  }

  return DefWindowProcA(hWndParent, Msg, wParam, lParam);
}

BOOL CreateInjectClass()
{
  WNDCLASSEXA InjectClass;

  InjectClass.cbSize = sizeof(WNDCLASSEX);
  InjectClass.style = CS_DBLCLKS;
  InjectClass.lpfnWndProc = (WNDPROC)CreateInjectWindows;
  InjectClass.cbClsExtra = 0;
  InjectClass.cbWndExtra = 0;
  InjectClass.hInstance = (HINSTANCE)MainHInstance;
  InjectClass.hIcon = LoadIconA(0, (LPCSTR)0x7F00);
  InjectClass.hCursor = LoadCursorA(0, (LPCSTR)0x7F00);
  InjectClass.hbrBackground = (HBRUSH)COLOR_WINDOWFRAME;
  InjectClass.lpszMenuName = 0;
  InjectClass.lpszClassName = "InjectClass";
  InjectClass.hIconSm = LoadIconA(0, (LPCSTR)0x7F00);
  return RegisterClassExA(&InjectClass) != 0;
}

DWORD StartInjector(LPVOID lpThreadParameter)
{
  HWND Window;
  struct tagMSG Msg;

  Initialize();
  CreateInjectClass();

  Window = CreateWindowExA(0, "InjectClass", "Python Injector", 0xCF0000u, 0x80000000, 0x80000000, 516, 538, 0, 0, 0, 0);
  ShowWindow(Window, 1);

  while (GetMessageA(&Msg, 0, 0, 0))
  {
    TranslateMessage(&Msg);
    DispatchMessageA(&Msg);
  }

  return 1;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
  switch (ul_reason_for_call)
  {
  case DLL_PROCESS_ATTACH:
  {
    if (!(g_PythonDLL = GetModuleHandleA("python24.dll")))
    {
      MessageBoxA(NULL, "Could not load python24.dll!", "Error",
                  MB_ICONERROR);
      return FALSE;
    }

    PyEval_InitThreads = (PTR_PyEval_InitThreads)GetProcAddress(
        g_PythonDLL, "PyEval_InitThreads");
    if (!PyEval_InitThreads)
    {
      MessageBoxA(NULL,
                  "An error occured! Could not find PyEval_InitThreads!",
                  "Error", MB_ICONERROR);
      return FALSE;
    }

    // This must be called in main thread!
    PyEval_InitThreads();

    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)StartInjector, 0, 0, 0);
    break;
  }
  case DLL_PROCESS_DETACH:
  {
    break;
  }
  }

  return TRUE;
}
