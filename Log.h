#pragma once

#include <windows.h>

#include <cstdio>
#include <ctime>
#include <string>

namespace tlog
{
    namespace detail
    {
        inline const char* RESET = "\x1b[0m";
        inline const char* GRAY = "\x1b[90m";
        inline const char* GREEN = "\x1b[32m";
        inline const char* LIGHTMAGENTA = "\x1b[95m";
        inline const char* LIGHTYELLOW = "\x1b[93m";
        inline const char* LIGHTRED = "\x1b[91m";
        inline const char* RED = "\x1b[31m";

        inline void EnableAnsi()
        {
            static bool done = false;
            if (done)
                return;
            done = true;

            HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD mode = 0;
            if (GetConsoleMode(out, &mode))
                SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }

        inline std::string Timestamp()
        {
            std::time_t now = std::time(nullptr);
            std::tm tm = {};
            localtime_s(&tm, &now);
            char buf[16] = {0};
            std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
            return std::string(GRAY) + buf + RESET;
        }

        inline std::string Highlight(const std::string& text)
        {
            std::string out;
            for (size_t i = 0; i < text.size(); ++i)
            {
                if (text[i] == '[')
                {
                    const size_t close = text.find(']', i);
                    if (close != std::string::npos)
                    {
                        out += GRAY;
                        out += '[';
                        out += RESET;
                        out += text.substr(i + 1, close - i - 1);
                        out += GRAY;
                        out += ']';
                        out += RESET;
                        i = close;
                        continue;
                    }
                }
                out += text[i];
            }
            return out;
        }

        inline void Emit(const char* tagColor, const char* tag,
                         const std::string& text, const char* sep)
        {
            EnableAnsi();
            std::printf("%s %s%s%s %s%s%s%s\n",
                        Timestamp().c_str(),
                        tagColor, tag, RESET,
                        GRAY, sep, RESET,
                        Highlight(text).c_str());
        }
    }

    inline void success(const std::string& text, const char* sep = " ")
    {
        detail::Emit(detail::GREEN, "YES", text, sep);
    }

    inline void info(const std::string& text, const char* sep = " ")
    {
        detail::Emit(detail::LIGHTMAGENTA, "INF", text, sep);
    }

    inline void debug(const std::string& text, const char* sep = " ")
    {
        detail::Emit(detail::LIGHTYELLOW, "DBG", text, sep);
    }

    inline void error(const std::string& text, const char* sep = " ")
    {
        detail::Emit(detail::LIGHTRED, "ERR", text, sep);
    }

    inline void fatal(const std::string& text, const char* sep = " ")
    {
        detail::Emit(detail::RED, "FTL", text, sep);
    }
}
