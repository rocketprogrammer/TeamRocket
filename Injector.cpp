#include "Injector.h"

Injector& Injector::Get()
{
    static Injector instance;
    return instance;
}

bool Injector::Initialize()
{
    if (m_ready)
        return true;

    m_python = GetModuleHandleA("python24.dll");
    if (!m_python)
        return false;

    m_run = reinterpret_cast<PTR_PyRun_SimpleStringFlags>(
        GetProcAddress(m_python, "PyRun_SimpleStringFlags"));
    m_initThreads = reinterpret_cast<PTR_PyEval_InitThreads>(
        GetProcAddress(m_python, "PyEval_InitThreads"));
    m_gilEnsure = reinterpret_cast<PTR_PyGILState_Ensure>(
        GetProcAddress(m_python, "PyGILState_Ensure"));
    m_gilRelease = reinterpret_cast<PTR_PyGILState_Release>(
        GetProcAddress(m_python, "PyGILState_Release"));

    if (!m_run || !m_initThreads || !m_gilEnsure || !m_gilRelease)
        return false;

    m_initThreads();
    m_ready = true;
    return true;
}

void Injector::Execute(const std::string& code)
{
    if (!m_ready)
        return;

    const PyGILState_STATE state = m_gilEnsure();
    m_run(code.c_str(), nullptr);
    m_gilRelease(state);
}
