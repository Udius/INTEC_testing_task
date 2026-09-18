#pragma once
#include <atomic>

namespace agent {

// Устанавливает обработчики SIGINT/SIGTERM (Windows: Ctrl+C через
// SetConsoleCtrlHandler). Обработчик только выставляет флаг —
// потоки сами корректно завершаются и делают сброс в backup.json.
// Возвращает ссылку на флаг остановки для главных компонентов.
std::atomic<bool>& InstallSignalHandler();

} // namespace agent