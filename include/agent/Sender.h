#pragma once
#include "agent/Config.h"
#include "agent/Metric.h"
#include "agent/MetricsQueue.h"
#include <atomic>
#include <string>
#include <vector>

namespace agent {

// Сетевой модуль: раз в flush_interval (или при batch_size записей)
// упаковывает метрики в JSON Package и отправляет POST на endpoint.
// При недоступности сервера записи возвращаются в буфер (retry),
// не отправленные после завершения работы сбрасываются в backup.json.
class Sender {
public:
    Sender(MetricsQueue& queue, const Config& config, std::atomic<bool>& stop_flag);

    // Цикл отправки (запускается в отдельном потоке).
    void Run();

    // Принудительная отправка всего буфера; остаток — в backup-файл.
    // Вызывается при graceful shutdown.
    void FlushOnShutdown();

private:
    // Один POST пакета. true — сервер принял (2xx).
    bool SendBatch(const std::vector<MetricRecord>& records);

    // Собрать JSON-тело запроса из записей.
    std::string BuildJsonBody(const std::vector<MetricRecord>& records) const;

    MetricsQueue& queue_;
    const Config& config_;
    std::atomic<bool>& stop_flag_;
};

} // namespace agent