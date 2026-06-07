#pragma once

#include <string>
#include <windows.h>

class Console
{
public:
    static Console& Get();

    void Poll(std::string& out);

private:
    Console() = default;

    std::string FindLatestLog() const;

    std::string m_path;
    long long m_offset = 0;
};
