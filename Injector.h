#pragma once

#include <string>
#include <windows.h>

class Injector
{
public:
    static Injector& Get();

    bool Initialize();
    void Execute(const std::string& code);

private:
    Injector() = default;

    using PTR_PyRun_SimpleStringFlags = int (*)(const char*, void*);
    using PTR_PyEval_InitThreads = void (*)();

    enum PyGILState_STATE { PyGILState_LOCKED, PyGILState_UNLOCKED };
    using PTR_PyGILState_Ensure = PyGILState_STATE (*)();
    using PTR_PyGILState_Release = void (*)(PyGILState_STATE);

    HMODULE m_python = nullptr;
    PTR_PyRun_SimpleStringFlags m_run = nullptr;
    PTR_PyEval_InitThreads m_initThreads = nullptr;
    PTR_PyGILState_Ensure m_gilEnsure = nullptr;
    PTR_PyGILState_Release m_gilRelease = nullptr;
    bool m_ready = false;
};
