// Прототип консольного агента мониторинга активности (ТЗ: C++20, Win32/X11).
#include "agent/Platform.h"
#include "agent/Config.h"
#include "agent/Metric.h"
#include "agent/MetricsCollector.h"
#include "agent/MetricsQueue.h"
#include "agent/Sender.h"
#include "agent/SignalHandler.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#ifdef AGENT_PLATFORM_WINDOWS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#endif

namespace agent {
namespace {

// Имя компьютера — идентификатор агента (agent_id из спецификации API).
std::string ComputerName() {
#ifdef AGENT_PLATFORM_WINDOWS
    wchar_t buffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameW(buffer, &size)) {
        std::string out;
        out.reserve(size);
        for (DWORD i = 0; i < size; ++i) {
            out += static_cast<char>(buffer[i]);
        }
        return out;
    }
#endif
    return "agent";
}

} // namespace
} // namespace agent

// Изменение кодировки консоли на UTF-8.
void SetupConsoleUtf8() {
#ifdef AGENT_PLATFORM_WINDOWS
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#else
    // Linux/macOS: консоль уже в UTF-8.
#endif
}

int main() {
    using namespace agent;
    using namespace std::chrono_literals;

    SetupConsoleUtf8();

    auto& stop_flag = InstallSignalHandler();

    Config config;
    config.agent_id = ComputerName();

    auto collector = CreateCollector(config.collect_interval);
    if (!collector) {
        std::cerr << "[agent] сборщик метрик для текущей платформы не реализован" << std::endl;
        return 1;
    }

    MetricsQueue queue(config.max_buffer);
    Sender sender(queue, config, stop_flag);

    std::cout << "[agent] запуск: id=" << config.agent_id
              << ", сбор " << config.collect_interval.count() << "с"
              << ", отправка " << config.flush_interval.count() << "с"
              << " или " << config.batch_size << " записей"
              << ", endpoint " << config.endpoint << std::endl;

    std::thread sender_thread([&sender] { sender.Run(); });

    // Поток сбора: каждые collect_interval — опрос ОС и push в буфер.
    auto next_collect = std::chrono::steady_clock::now();
    while (!stop_flag.load(std::memory_order_relaxed)) {
        if (std::chrono::steady_clock::now() >= next_collect) {
            queue.Push(collector->Collect());
            next_collect += config.collect_interval;
        }
        std::this_thread::sleep_for(100ms);
    }

    // Graceful shutdown: дождаться Sender'а и сбросить остаток буфера.
    std::cout << "[agent] остановка по сигналу..." << std::endl;
    queue.NotifyAll();
    sender_thread.join();
    sender.FlushOnShutdown();
    std::cout << "[agent] завершено" << std::endl;
    return 0;
}
