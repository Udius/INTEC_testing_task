#pragma once
#include "agent/Metric.h"
#include <chrono>
#include <memory>

namespace agent {

// ШП - фабричный метод

// Платформо-независимый интерфейс сборщика метрик.
// Реализации: Windows (Win32 API), Linux (X11) — пока заглушка.
class MetricsCollector {
public:
    virtual ~MetricsCollector() = default;

    // Один опрос: окно в фокусе + факт активности пользователя за интервал сбора.
    virtual MetricRecord Collect() = 0;
};

// Фабрика: возвращает реализацию для текущей платформы
// (collect_interval нужен для оценки факта ввода за интервал сбора).
// nullptr — реализация для платформы еще не готова (Linux/X11).
std::unique_ptr<MetricsCollector> CreateCollector(std::chrono::seconds collect_interval);

} // namespace agent