#include "agent/MetricsQueue.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <limits>

namespace agent {

MetricsQueue::MetricsQueue(std::size_t max_size)
    : max_size_(max_size == 0 ? 1 : max_size) {}

void MetricsQueue::PushLocked(const MetricRecord& record) {
    records_.push_back(record);
    // Буфер ограничен сверху: старейшие записи отбрасываются, ОЗУ не раздувается.
    while (records_.size() > max_size_) {
        records_.pop_front();
    }
}

void MetricsQueue::Push(const MetricRecord& record) {
    {
        std::lock_guard lock(mutex_);
        PushLocked(record);
    }
    cv_.notify_all();
}

std::vector<MetricRecord> MetricsQueue::Take(std::size_t n) {
    std::lock_guard lock(mutex_);
    n = std::min(n, records_.size());
    const auto first = records_.begin();
    const auto last = records_.begin() + static_cast<std::ptrdiff_t>(n);
    std::vector<MetricRecord> out(first, last);
    records_.erase(first, last);
    return out;
}

std::vector<MetricRecord> MetricsQueue::TakeAll() {
    return Take(std::numeric_limits<std::size_t>::max());
}

void MetricsQueue::Return(std::vector<MetricRecord> records) {
    std::lock_guard lock(mutex_);
    // Неотправленный пакет возвращается в начало (порядок сохраняется).
    records_.insert(records_.begin(), records.begin(), records.end());
    while (records_.size() > max_size_) {
        records_.pop_front();
    }
}

bool MetricsQueue::WaitForData(std::atomic<bool>& stop_flag, std::chrono::milliseconds timeout) {
    std::unique_lock lock(mutex_);
    return cv_.wait_for(lock, timeout, [&] {
        return !records_.empty() || stop_flag.load(std::memory_order_relaxed);
    });
}

void MetricsQueue::WaitOrStop(std::atomic<bool>& stop_flag, std::chrono::milliseconds timeout) {
    std::unique_lock lock(mutex_);
    cv_.wait_for(lock, timeout, [&] {
        return stop_flag.load(std::memory_order_relaxed);
    });
}

void MetricsQueue::NotifyAll() {
    cv_.notify_all();
}

std::size_t MetricsQueue::Size() const {
    std::lock_guard lock(mutex_);
    return records_.size();
}

bool MetricsQueue::Empty() const {
    std::lock_guard lock(mutex_);
    return records_.empty();
}

} // namespace agent
