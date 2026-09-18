#pragma once
#include <chrono>
#include <cstddef>
#include <string>

namespace agent {

// Настройки агента (значения по умолчанию — из ТЗ).
struct Config {
    std::chrono::seconds collect_interval{5};       // период сбора метрик
    std::chrono::seconds flush_interval{30};        // период отправки пакета
    std::size_t batch_size = 10;                    // отправка при достижении N записей
    std::size_t max_buffer = 100;                   // ограничение буфера неотправленных записей
    std::string endpoint = "http://127.0.0.1:8080"; // 127.0.0.1 вместо localhost: на Windows localhost -> ::1
    std::string backup_path = "backup.json";        // файл сброса при завершении
    std::string agent_id;                           // пусто => имя компьютера при запуске
};

} // namespace agent