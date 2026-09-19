// Сборщик метрик активности: Linux (X11).
// Пишется под Linux-окружение: на Windows не компилируется и не линкуется —
// CMake выбирает этот файл только для не-Windows сборок (см. CMakeLists.txt).
#include "agent/MetricsCollector.h"
#include "agent/Platform.h"
#include "agent/TimeUtil.h"

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

namespace agent {
namespace {

// 32-битный атрибут окна (WINDOW / CARDINAL). X11 укладывает такие значения
// в unsigned long, поэтому берём первое слово массива.
template <typename T>
T ReadWindowLong(Display* display, ::Window window, Atom property, Atom expected_type) {
    Atom actual_type = None;
    int actual_format = 0;
    unsigned long nitems = 0;
    unsigned long bytes_after = 0;
    unsigned char* data = nullptr;
    T value{};
    if (XGetWindowProperty(display, window, property, 0, 1, False, expected_type, &actual_type,
                           &actual_format, &nitems, &bytes_after, &data) == Success &&
        data != nullptr) {
        if (actual_format == 32 && nitems >= 1) {
            value = static_cast<T>(*reinterpret_cast<unsigned long*>(data));
        }
        XFree(data);
    }
    return value;
}

// Текстовый атрибут окна. Формат 8 — строка в кодировке атрибута (UTF-8/LATIN1).
std::string ReadWindowText(Display* display, ::Window window, Atom property, Atom type) {
    Atom actual_type = None;
    int actual_format = 0;
    unsigned long nitems = 0;
    unsigned long bytes_after = 0;
    unsigned char* data = nullptr;
    std::string out;
    if (XGetWindowProperty(display, window, property, 0, 4096, False, type, &actual_type,
                           &actual_format, &nitems, &bytes_after, &data) == Success &&
        data != nullptr) {
        if (actual_format == 8) {
            out.assign(reinterpret_cast<char*>(data), nitems);
        }
        XFree(data);
    }
    return out;
}

class LinuxCollector final : public MetricsCollector {
public:
    explicit LinuxCollector(std::chrono::seconds collect_interval)
        : active_window_ms_(std::chrono::duration_cast<std::chrono::milliseconds>(collect_interval).count()) {
        display_ = XOpenDisplay(nullptr); // nullptr => взять из переменной окружения DISPLAY
        if (!display_) {
            // Headless (SSH без X, контейнер): агент продолжает работать,
            // поля окна остаются пустыми — как в Windows при отсутствии фокуса.
            std::cerr << "[collector] нет доступа к X11-сессии, метрики окна будут пустыми"
                      << std::endl;
        }
    }

    ~LinuxCollector() override {
        if (display_) {
            XCloseDisplay(display_);
        }
    }

    MetricRecord Collect() override {
        MetricRecord record;
        record.time = LocalTimeNow();
        if (display_) {
            const ::Window window = ActiveWindow();
            record.window_title = WindowTitle(window);
            record.process_name = ProcessName(window);
        }
        // Факт физической активности за интервал сбора (без кейлоггера).
        record.user_active = static_cast<long long>(IdleTimeMs()) < active_window_ms_;
        return record;
    }

private:
    // Окно в фокусе: EWMH-атрибут root _NET_ACTIVE_WINDOW, фолбэк — XGetInputFocus.
    ::Window ActiveWindow() const {
        const Atom active = XInternAtom(display_, "_NET_ACTIVE_WINDOW", False);
        ::Window window = ReadWindowLong< ::Window >(
            display_, RootWindow(display_, DefaultScreen(display_)), active, XA_WINDOW);
        if (window == None) {
            int revert_to = 0;
            XGetInputFocus(display_, &window, &revert_to);
        }
        return window;
    }

    // Заголовок: _NET_WM_NAME (UTF-8) предпочтительнее старого WM_NAME.
    std::string WindowTitle(::Window window) const {
        if (window == None) {
            return {};
        }
        const Atom net_wm_name = XInternAtom(display_, "_NET_WM_NAME", False);
        const Atom utf8_string = XInternAtom(display_, "UTF8_STRING", False);
        std::string title = ReadWindowText(display_, window, net_wm_name, utf8_string);
        if (title.empty()) {
            title = ReadWindowText(display_, window, XA_WM_NAME, XA_STRING);
        }
        return title;
    }

    // Имя процесса: _NET_WM_PID -> /proc/<pid>/comm (не все клиенты X ставят атрибут).
    std::string ProcessName(::Window window) const {
        if (window == None) {
            return {};
        }
        const Atom net_wm_pid = XInternAtom(display_, "_NET_WM_PID", False);
        const long pid = ReadWindowLong<long>(display_, window, net_wm_pid, XA_CARDINAL);
        if (pid <= 0) {
            return {};
        }
        std::ifstream input("/proc/" + std::to_string(pid) + "/comm");
        std::string name;
        std::getline(input, name);
        return name;
    }

    // Миллисекунды с последнего ввода мыши/клавиатуры (расширение Xss из libXext).
    // 0 при недоступности расширения => пользователь считается активным
    // (та же политика, что в Windows-ветке при ошибке GetLastInputInfo).
    long IdleTimeMs() const {
        int event_base = 0;
        int error_base = 0;
        if (!XScreenSaverQueryExtension(display_, &event_base, &error_base)) {
            return 0;
        }
        XScreenSaverInfo* info = XScreenSaverAllocInfo();
        if (!info) {
            return 0;
        }
        long idle_ms = 0;
        if (XScreenSaverQueryInfo(display_, DefaultScreen(display_), info)) {
            idle_ms = static_cast<long>(info->idle);
        }
        XFree(info);
        return idle_ms;
    }

    Display* display_ = nullptr;
    long long active_window_ms_ = 5000;
};

} // namespace

std::unique_ptr<MetricsCollector> CreateCollector(std::chrono::seconds collect_interval) {
    return std::make_unique<LinuxCollector>(collect_interval);
}

} // namespace agent