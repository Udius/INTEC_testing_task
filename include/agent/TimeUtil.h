#pragma once
#include "agent/Platform.h"

#include <ctime>
#include <string>

namespace agent {

// Локальное время в формате "YYYY-MM-DD HH:MM:SS" (поле time в спецификации API).
inline std::string LocalTimeNow() {
    const std::time_t now = std::time(nullptr);
    std::tm tm{};
#ifdef AGENT_PLATFORM_WINDOWS
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
    return buffer;
}

} // namespace agent