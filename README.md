# Агент мониторинга активности (C++20)

Консольный агент: в фоне собирает метрики активности пользователя
(окно в фокусе, факт ввода), буферизует их и периодически отправляет
POST-пакетом JSON на демонстрационный сервер. При недоступности сервера
данные остаются в буфере (до 100 записей), при завершении по
Ctrl+C / SIGTERM неотправленное сбрасывается в `backup.json`.

## Что делает

- Каждые **5 секунд** фиксирует: имя процесса и заголовок окна в фокусе,
  факт активности пользователя (мышь/клавиатура) — без кейлоггера,
  только факт ввода.
- Раз в **30 секунд** или при накоплении **10 записей** отправляет пакет
  `POST /` с `Content-Type: application/json`:

```json
{
  "agent_id": "DESKTOP-MIDDLE-C",
  "timestamp": 1792147320,
  "payload": [
    { "time": "2026-09-15 13:55:00", "process_name": "chrome.exe",
      "window_title": "…", "user_active": true }
  ]
}
```

- Сервер недоступен → пакет возвращается в буфер, повтор через 5 с.
- Выход (Ctrl+C / SIGINT / SIGTERM) → финальная попытка отправки,
  затем остаток в `backup.json`.

## Стек

- C++20, CMake (>= 3.20)
- Windows: Win32 API; Linux: X11 (Xlib + расширение Xss из libXext)
- Сеть/JSON: cpp-httplib и nlohmann/json — header-only, лежат в `third_party/`,
  ничего скачивать не нужно

## Сборка

Windows (Visual Studio 2022):

```
cmake -S . -B build
cmake --build build --config Release
```

Linux (пакеты: `libx11-dev libxss-dev`, на Fedora — `libX11-devel libXss-devel`):

```
sudo apt install g++ cmake libx11-dev libxss-dev
cmake -S . -B build
cmake --build build -j
```

## Запуск

1. Поднять демонстрационный сервер, принимающий POST:

```
python3 tools/demo_server.py
```

2. Запустить агента из каталога сборки:

```
./build/agent             # Linux
build\Release\agent.exe   # Windows
```

Остановка — Ctrl+C: агент корректно завершится и при недоступном
сервере сохранит остаток буфера в `backup.json`.

## Структура

```
include/agent/        # интерфейсы модулей
  MetricsCollector.h  # сбор метрик (Win32 / X11)
  MetricsQueue.h      # потокобезопасный буфер с лимитом 100
  Sender.h            # отправка JSON + retry
  BackupWriter.h      # сброс в backup.json
  SignalHandler.h     # Ctrl+C / SIGINT / SIGTERM
  Config.h            # интервалы, batch, endpoint
src/                  # реализации; коллекторы — CollectorWindows.cpp / CollectorLinux.cpp
third_party/          # httplib.h, nlohmann/json.hpp (header-only)
```

