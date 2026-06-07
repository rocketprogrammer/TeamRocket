#include <windows.h>
#include <tlhelp32.h>

#include <string>

#include "Log.h"

static const char* kDefaultProcess = "Pirates_Online.exe";
static const char* kDefaultDll = "injector.dll";

static DWORD FindProcessId(const char* name)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32 entry = {0};
    entry.dwSize = sizeof(entry);

    DWORD pid = 0;
    if (Process32First(snapshot, &entry))
    {
        do
        {
            if (_stricmp(entry.szExeFile, name) == 0)
            {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return pid;
}

static bool Inject(DWORD pid, const std::string& dllPath)
{
    HANDLE process = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
            PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);
    if (!process)
    {
        tlog::error("OpenProcess failed (" + std::to_string(GetLastError()) +
                   "). Try running as admin.");
        return false;
    }

    bool ok = false;
    const SIZE_T size = dllPath.size() + 1;
    LPVOID remote = VirtualAllocEx(process, nullptr, size,
                                   MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote)
    {
        tlog::error("VirtualAllocEx failed (" +
                   std::to_string(GetLastError()) + ").");
    }
    else if (!WriteProcessMemory(process, remote, dllPath.c_str(), size,
                                 nullptr))
    {
        tlog::error("WriteProcessMemory failed (" +
                   std::to_string(GetLastError()) + ").");
    }
    else
    {
        LPVOID loadLib = (LPVOID)GetProcAddress(
            GetModuleHandleA("kernel32.dll"), "LoadLibraryA");

        HANDLE thread = CreateRemoteThread(
            process, nullptr, 0, (LPTHREAD_START_ROUTINE)loadLib,
            remote, 0, nullptr);
        if (!thread)
        {
            tlog::error("CreateRemoteThread failed (" +
                       std::to_string(GetLastError()) + ").");
        }
        else
        {
            WaitForSingleObject(thread, INFINITE);

            DWORD exitCode = 0;
            GetExitCodeThread(thread, &exitCode);
            if (exitCode == 0)
                tlog::error("LoadLibraryA returned NULL inside the target.");
            else
                ok = true;

            CloseHandle(thread);
        }
    }

    if (remote)
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
    CloseHandle(process);
    return ok;
}

static std::string ResolveDllPath(const char* arg)
{
    char full[MAX_PATH] = {0};
    if (GetFullPathNameA(arg, MAX_PATH, full, nullptr) == 0)
        return std::string(arg);
    return std::string(full);
}

int main(int argc, char** argv)
{
    const char* processName = (argc > 1) ? argv[1] : kDefaultProcess;
    const char* dllName = (argc > 2) ? argv[2] : kDefaultDll;

    const std::string dllPath = ResolveDllPath(dllName);

    if (GetFileAttributesA(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        tlog::fatal("DLL not found: " + dllPath);
        return 1;
    }

    tlog::info("Target:  " + std::string(processName));
    tlog::info("Payload: " + dllPath);

    DWORD pid = FindProcessId(processName);
    if (!pid)
    {
        tlog::fatal("Process not found: " + std::string(processName) +
                    ". Is the game running?");
        return 1;
    }

    tlog::info("PID:     " + std::to_string(pid));

    if (!Inject(pid, dllPath))
    {
        tlog::fatal("Injection failed.");
        return 1;
    }

    tlog::success("Injected successfully. Press Insert in-game to toggle.");
    return 0;
}
