// Резервный сброс неотправленных метрик в backup.json (формат — массив payload).
#include "agent/BackupWriter.h"

#include <nlohmann/json.hpp>

#include <fstream>

namespace agent {

bool WriteBackup(const std::string& path, const std::vector<MetricRecord>& records) {
    nlohmann::json body = nlohmann::json::array();
    for (const auto& record : records) {
        body.push_back({
            {"time", record.time},
            {"process_name", record.process_name},
            {"window_title", record.window_title},
            {"user_active", record.user_active},
        });
    }

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    out << body.dump(2);
    return out.good();
}

} // namespace agent
