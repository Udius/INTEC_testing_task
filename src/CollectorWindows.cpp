// Сборщик метрик активности: Windows (Win32 API).
#include "agent/MetricsCollector.h"
#include "agent/Platform.h"
#include "agent/TimeUtil.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <chrono>
#include <string>

namespace agent {
namespace {

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                                         nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                        out.data(), size, nullptr, nullptr);
    return out;
}

class WindowsCollector final : public MetricsCollector {
public:
    explicit WindowsCollector(std::chrono::seconds collect_interval)
        : active_window_ms_(std::chrono::duration_cast<std::chrono::milliseconds>(collect_interval).count()) {}

    MetricRecord Collect() override {
        MetricRecord record;
        record.time = LocalTimeNow();

        if (const HWND hwnd = GetForegroundWindow()) {
            record.process_name = FocusedProcessName(hwnd);
            record.window_title = FocusedWindowTitle(hwnd);
        }

        // Факт физической активности за интервал сбора (без кейлоггера).
        record.user_active = static_cast<long long>(IdleTimeMs()) < active_window_ms_;
        return record;
    }

private:
    static std::string FocusedProcessName(HWND hwnd) {
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid == 0) {
            return {};
        }
        HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!process) {
            return {};
        }
        wchar_t path[MAX_PATH];
        DWORD size = MAX_PATH;
        std::string name;
        if (QueryFullProcessImageNameW(process, 0, path, &size)) {
            std::wstring full(path, size);
            const auto slash = full.find_last_of(L"\\/");
            name = WideToUtf8(slash == std::wstring::npos ? full : full.substr(slash + 1));
        }
        CloseHandle(process);
        return name;
    }

    static std::string FocusedWindowTitle(HWND hwnd) {
        const int length = GetWindowTextLengthW(hwnd);
        if (length <= 0) {
            return {};
        }
        std::wstring title(static_cast<std::size_t>(length) + 1, L'\0');
        GetWindowTextW(hwnd, title.data(), length + 1);
        title.resize(static_cast<std::size_t>(length));
        return WideToUtf8(title);
    }

    // Миллисекунды с последнего ввода мыши/клавиатуры.
    static DWORD IdleTimeMs() {
        LASTINPUTINFO info{};
        info.cbSize = sizeof(info);
        if (!GetLastInputInfo(&info)) {
            return 0;
        }
        return GetTickCount() - info.dwTime;
    }

    long long active_window_ms_ = 5000;
};

} // namespace

std::unique_ptr<MetricsCollector> CreateCollector(std::chrono::seconds collect_interval) {
    return std::make_unique<WindowsCollector>(collect_interval);
}

} // namespace agent