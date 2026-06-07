#include "Console.h"

#include <fstream>

Console& Console::Get()
{
    static Console instance;
    return instance;
}

std::string Console::FindLatestLog() const
{
    char modulePath[MAX_PATH] = {0};
    GetModuleFileNameA(nullptr, modulePath, MAX_PATH);

    std::string dir(modulePath);
    const size_t slash = dir.find_last_of("\\/");
    if (slash == std::string::npos)
        return std::string();
    dir.erase(slash + 1);

    const std::string pattern = dir + "*.log";

    WIN32_FIND_DATAA find = {0};
    HANDLE handle = FindFirstFileA(pattern.c_str(), &find);
    if (handle == INVALID_HANDLE_VALUE)
        return std::string();

    std::string best;
    FILETIME bestTime = {0};
    do
    {
        if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
        if (CompareFileTime(&find.ftLastWriteTime, &bestTime) > 0)
        {
            bestTime = find.ftLastWriteTime;
            best = dir + find.cFileName;
        }
    } while (FindNextFileA(handle, &find));

    FindClose(handle);
    return best;
}

void Console::Poll(std::string& out)
{
    const std::string latest = FindLatestLog();
    if (latest.empty())
        return;

    if (latest != m_path)
    {
        m_path = latest;
        m_offset = 0;
    }

    std::ifstream in(m_path.c_str(), std::ios::binary);
    if (!in)
        return;

    in.seekg(0, std::ios::end);
    const long long size = static_cast<long long>(in.tellg());

    if (size < m_offset)
        m_offset = 0;

    if (size <= m_offset)
        return;

    in.seekg(m_offset, std::ios::beg);
    const std::streamsize toRead =
        static_cast<std::streamsize>(size - m_offset);

    std::string chunk;
    chunk.resize(static_cast<size_t>(toRead));
    in.read(&chunk[0], toRead);
    chunk.resize(static_cast<size_t>(in.gcount()));

    m_offset += static_cast<long long>(chunk.size());
    out += chunk;
}
