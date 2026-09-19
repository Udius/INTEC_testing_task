#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace agent {

// Одна метрика активности (одна запись в массиве payload).
struct MetricRecord {
    std::string time;          // локальное время "YYYY-MM-DD HH:MM:SS"
    std::string process_name;  // имя исполняемого файла окна в фокусе
    std::string window_title;  // заголовок окна в фокусе
    bool user_active = false;  // был ли ввод (мышь/клавиатура) за интервал сбора
};

} // namespace agent