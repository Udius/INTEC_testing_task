#pragma once
// Платформенная защита: поддерживаются только Windows (Win32 API) и Linux (X11).
#if !defined(_WIN32) && !defined(__linux__)
#  error "Поддерживаются только Windows и Linux"
#endif

#if defined(_WIN32)
#  define AGENT_PLATFORM_WINDOWS 1
#elif defined(__linux__)
#  define AGENT_PLATFORM_LINUX 1
#endif

// Настройка консоли под UTF-8 (Windows: консоль в OEM-кодировке, а строки
// программы компилируются в UTF-8; Linux — консоль уже в UTF-8).
void SetupConsoleUtf8();
