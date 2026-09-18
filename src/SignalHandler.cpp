// Перехват SIGINT/SIGTERM (Windows — Ctrl+C через SetConsoleCtrlHandler).
#include "agent/SignalHandler.h"
#include "agent/Platform.h"

#include <csignal>

#ifdef AGENT_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace agent {
namespace {

std::atomic<bool> g_stop_flag{false};

#ifdef AGENT_PLATFORM_WINDOWS

BOOL WINAPI ConsoleCtrlHandler(DWORD ctrl_type) {
    if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_BREAK_EVENT || ctrl_type == CTRL_CLOSE_EVENT) {
        g_stop_flag.store(true, std::memory_order_relaxed);
        return TRUE;
    }
    return FALSE;
}

#else

void OnSignal(int) {
    g_stop_flag.store(true, std::memory_order_relaxed);
}

#endif

} // namespace

std::atomic<bool>& InstallSignalHandler() {
#ifdef AGENT_PLATFORM_WINDOWS
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
#else
    std::signal(SIGINT, OnSignal);
    std::signal(SIGTERM, OnSignal);
#endif
    return g_stop_flag;
}

} // namespace agent
