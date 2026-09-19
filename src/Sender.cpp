// Сетевой модуль: JSON-пакет (nlohmann/json) + POST (cpp-httplib).
#include "agent/Sender.h"
#include "agent/BackupWriter.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <iostream>

namespace agent {
namespace {

// Время таймаута при недоступном сервере
constexpr auto kPollStep = std::chrono::milliseconds(200);
// Пауза перед повторной попыткой после сбоя (retry backoff)
constexpr auto kRetryBackoff = std::chrono::seconds(5);

nlohmann::json RecordToJson(const MetricRecord& record) {
    return {
        {"time", record.time},
        {"process_name", record.process_name},
        {"window_title", record.window_title},
        {"user_active", record.user_active},
    };
}

std::int64_t UnixNow() {
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

} // namespace

Sender::Sender(MetricsQueue& queue, const Config& config, std::atomic<bool>& stop_flag)
    : queue_(queue), config_(config), stop_flag_(stop_flag) {}

std::string Sender::BuildJsonBody(const std::vector<MetricRecord>& records) const {
    nlohmann::json body;
    body["agent_id"] = config_.agent_id;
    body["timestamp"] = UnixNow();
    body["payload"] = nlohmann::json::array();
    for (const auto& record : records) {
        body["payload"].push_back(RecordToJson(record));
    }
    return body.dump();
}

bool Sender::SendBatch(const std::vector<MetricRecord>& records) {
    httplib::Client client(config_.endpoint);
    client.set_connection_timeout(2, 0);
    client.set_read_timeout(5, 0);

    const auto result = client.Post("/", BuildJsonBody(records), "application/json");
    if (!result) {
        std::cerr << "[sender] сервер недоступен, пакет остался в буфере." << std::endl;
        return false;
    }
    if (result->status < 200 || result->status >= 300) {
        std::cerr << "[sender] сервер ответил " << result->status
                  << ", пакет остался в буфере" << std::endl;
        return false;
    }
    std::cout << "[sender] отправлено записей: " << records.size() << std::endl;
    return true;
}

void Sender::Run() {
    const auto now = [] { return std::chrono::steady_clock::now(); };
    auto last_attempt = now();

    while (true) {
        queue_.WaitForData(stop_flag_, kPollStep);

        const bool stopping = stop_flag_.load(std::memory_order_relaxed);
        const bool time_up = now() - last_attempt >= config_.flush_interval;
        const bool batch_full = queue_.Size() >= config_.batch_size;

        if (stopping || time_up || batch_full) {
            auto batch = queue_.Take(config_.batch_size);
            if (!batch.empty()) {
                last_attempt = now();
                if (!SendBatch(batch)) {
                    queue_.Return(std::move(batch)); // retry при восстановлении сервера
                    std::cerr << "[sender] пакет вернулся в буфер, всего в буфере "
                                << queue_.Size() << " записей." << std::endl;

                    // Backoff: пауза до повторной попытки. WaitOrStop, а не WaitForData:
                    // буфер не пуст (в нём вернувшийся пакет), и ожидание "до появления
                    // данных" вернулось бы мгновенно — получился бы спам попытками.
                    if (!stopping) {
                        queue_.WaitOrStop(stop_flag_, kRetryBackoff);
                    }
                }
            }
        } else {
            // Условий для отправки нет, но буфер может быть не пуст — тогда
            // WaitForData в начале цикла вернётся мгновенно, и цикл закрутится
            // вхолостую. Выдерживаем шаг опроса и здесь.
            queue_.WaitOrStop(stop_flag_, kPollStep);
        }
        if (stopping) {
            break;
        }
    }
}

void Sender::FlushOnShutdown() {
    auto batch = queue_.TakeAll();
    if (batch.empty()) {
        return;
    }
    if (SendBatch(batch)) {
        return;
    }
    // Сервер не принял — резервный сброс, чтобы не потерять данные.
    if (WriteBackup(config_.backup_path, batch)) {
        std::cout << "[sender] буфер сохранён в " << config_.backup_path << std::endl;
    } else {
        std::cerr << "[sender] не удалось записать " << config_.backup_path << std::endl;
    }
}

} // namespace agent
