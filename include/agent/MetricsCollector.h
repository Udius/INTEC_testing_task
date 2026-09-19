#pragma once
#include "agent/Metric.h"
#include <chrono>
#include <memory>

namespace agent {

// ШП - фабричный метод

// Платформо-независимый интерфейс сборщика метрик.
// Реализации: Windows (Win32 API) — src/CollectorWindows.cpp,
// Linux (X11) — src/CollectorLinux.cpp; файл выбирается в CMake по платформе.
class MetricsCollector {
public:
    virtual ~MetricsCollector() = default;

    // Один опрос: окно в фокусе + факт активности пользователя за интервал сбора.
    virtual MetricRecord Collect() = 0;
};

// Фабрика: возвращает реализацию для текущей платформы
// (collect_interval нужен для оценки факта ввода за интервал сбора).
// Обе реализации работают без X11-сессии (поля окна остаются пустыми),
// поэтому nullptr не возвращается; проверка в main — страховка на будущее.
std::unique_ptr<MetricsCollector> CreateCollector(std::chrono::seconds collect_interval);

} // namespace agent