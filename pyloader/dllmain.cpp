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
T GetFunctionPtr(HMODULE module, const char* name) {
  T func = (T)GetProcAddress(module, name);
  if (NULL == func) {
    std::cout << "An error occured! Could not find " << name << "!"
              << std::endl;
  }

  return func;
}

#define GET_FUNC_PTR(module, name) \
  name = GetFunctionPtr<PTR_##name>(module, #name)

//---------------------------------------------------------
// Typedefs

typedef int (*PTR_PyRun_SimpleStringFlags)(const char* command, void* flags);
PTR_PyRun_SimpleStringFlags PyRun_SimpleStringFlags = NULL;

typedef void* (*PTR_PyRun_String)(const char* str,
                                  int start,
                                  void* globals,
                                  void* locals);
PTR_PyRun_String PyRun_String = NULL;

// Thread stuff
typedef void (*PTR_PyEval_InitThreads)();
PTR_PyEval_InitThreads PyEval_InitThreads = NULL;

// Global Interpreter Lock (GIL)
enum PyGILState_STATE { PyGILState_LOCKED, PyGILState_UNLOCKED };

typedef enum PyGILState_STATE (*PTR_PyGILState_Ensure)(void);
PTR_PyGILState_Ensure PyGILState_Ensure = NULL;

typedef void (*PTR_PyGILState_Release)(enum PyGILState_STATE);
PTR_PyGILState_Release PyGILState_Release = NULL;

//#define Py_file_input 257
//#define Py_eval_input 258

HMODULE g_PythonDLL = NULL;
LPDWORD g_ThreadID 	= NULL;

#define DLL_EXPORT extern "C" __declspec(dllexport)

DLL_EXPORT bool executeCode(const char *code) {
  if (!(GET_FUNC_PTR(g_PythonDLL, PyRun_SimpleStringFlags))) {
    return FALSE;
  }

  if (!(GET_FUNC_PTR(g_PythonDLL, PyRun_String))) {
    return FALSE;
  }

  // GIL
  if (!(GET_FUNC_PTR(g_PythonDLL, PyGILState_Ensure))) {
    return FALSE;
  }

  if (!(GET_FUNC_PTR(g_PythonDLL, PyGILState_Release))) {
    return FALSE;
  }

  PyGILState_STATE state = PyGILState_Ensure();
  PyRun_SimpleStringFlags(code, NULL);
  PyGILState_Release(state);

  return TRUE;
}

const char *hax = R"(
from threading import Thread
import socket

def InjServer():
    HOST = '0.0.0.0'
    PORT = 1337

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind((HOST, PORT))
    s.listen(10)

    while True:
        conn, addr = s.accept()
        data = conn.recv(4096)

        if not data:
            break

        try:
            exec(data, globals())
        except:
            import traceback
            traceback.print_exc()

        conn.close()

def hax():
    exec(open('C:/scripts/hax.py').read(), globals())

base.accept('f1', hax)
Thread(target = InjServer).start()
)";

DWORD WINAPI LoaderMain(LPVOID lpParam) {
  if (!(GET_FUNC_PTR(g_PythonDLL, PyRun_SimpleStringFlags))) {
    return FALSE;
  }

  if (!(GET_FUNC_PTR(g_PythonDLL, PyRun_String))) {
    return FALSE;
  }

  // GIL
  if (!(GET_FUNC_PTR(g_PythonDLL, PyGILState_Ensure))) {
    return FALSE;
  }

  if (!(GET_FUNC_PTR(g_PythonDLL, PyGILState_Release))) {
    return FALSE;
  }

  PyGILState_STATE state = PyGILState_Ensure();
  std::cout << "Setting up injector...\n";
  PyRun_SimpleStringFlags(hax, NULL);
  PyGILState_Release(state);
  std::cout << "Done!\n";
  FreeLibraryAndExitThread((HMODULE)lpParam, 0);

  return TRUE;
}

BOOL APIENTRY DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved) {
  switch (dwReason) {
    case DLL_PROCESS_ATTACH: {
      if (!(g_PythonDLL = GetModuleHandleA("python24.dll"))) {
        MessageBoxA(NULL, "Could not load python24.dll!", "Error",
                    MB_ICONERROR);
        return FALSE;
      }

      PyEval_InitThreads = (PTR_PyEval_InitThreads)GetProcAddress(
          g_PythonDLL, "PyEval_InitThreads");
      if (!PyEval_InitThreads) {
        MessageBoxA(NULL,
                    "An error occured! Could not find PyEval_InitThreads!",
                    "Error", MB_ICONERROR);
        return FALSE;
      }

      // This must be called in main thread!
      PyEval_InitThreads();

      CreateThread(NULL, 0, LoaderMain, 0, 0, g_ThreadID);
      break;
    }
    case DLL_PROCESS_DETACH: {
      break;
    }
  }

  return TRUE;
}