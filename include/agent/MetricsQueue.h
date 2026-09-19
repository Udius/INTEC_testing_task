#pragma once
#include "agent/Metric.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>

namespace agent {

// Потокобезопасный буфер между потоком сбора и сетевым модулем.
// Push никогда не блокирует сборщик; при переполнении (max_size)
// oldest записи отбрасываются, чтобы не раздувать ОЗУ.
class MetricsQueue {
public:
    explicit MetricsQueue(std::size_t max_size);

    // Поместить запись (без блокировки на ожидание сети).
    void Push(const MetricRecord& record);

    // Забрать до n записей из головы буфера (неблокирующий).
    std::vector<MetricRecord> Take(std::size_t n);

    // Забрать все записи (для завершения / резервного сброса).
    std::vector<MetricRecord> TakeAll();

    // Вернуть записи в буфер (при сбое отправки), с учетом лимита.
    void Return(std::vector<MetricRecord> records);

    // Ждать появления данных или флага остановки (для цикла Sender'а).
    // true — есть данные; false — таймаут или остановка.
    bool WaitForData(std::atomic<bool>& stop_flag, std::chrono::milliseconds timeout);

    // Ждать только флага остановки или таймаута, игнорируя данные.
    // Для backoff после сбоя отправки: буфер не пуст, но слать нельзя.
    void WaitOrStop(std::atomic<bool>& stop_flag, std::chrono::milliseconds timeout);

    // Разбудить ожидающего (например, при выставлении флага остановки).
    void NotifyAll();

    std::size_t Size() const;
    bool Empty() const;

private:
    void PushLocked(const MetricRecord& record);

    mutable std::mutex mutex_;
    std::condition_variable cv_;        // пробуждение Sender'а: данные или стоп
    std::deque<MetricRecord> records_;
    std::size_t max_size_;
};

} // namespace agent