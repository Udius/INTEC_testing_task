#pragma once
#include "agent/Metric.h"
#include <string>
#include <vector>

namespace agent {

// Сброс неотправленных записей в JSON-файл (backup.json) при завершении.
// Формат — тот же Package, чтобы файл можно было отправить позже.
// Возвращает false при ошибке записи (например, нет прав на файл).
bool WriteBackup(const std::string& path, const std::vector<MetricRecord>& records);

} // namespace agent