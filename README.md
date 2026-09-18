# Агент мониторинга активности (C++20)

Прототип консольного агента по ТЗ: в фоновом режиме собирает метрики
активности пользователя (окно в фокусе, факт ввода), буферизует их в
потокобезопасной очереди и периодически отправляет POST-пакетом JSON
на демонстрационный сервер. При недоступности сервера данные остаются
в буфере, при завершении — сбрасываются в backup.json.

## Технологический стек
- C++20, CMake
- Windows: Win32 API; Linux: X11 (реализация пока заглушка —
  будет дописана в Linux-окружении)
- Сеть/JSON: cpp-httplib + nlohmann/json

## Осознанные решения
- **cpp-httplib и nlohmann/json лежат в `third_party/`** (single-header,
  подключаются как INTERFACE-библиотека в CMake). Это выбрано вместо
  Boost/POCO/libcurl: минимум внешних зависимостей и шагов сборки.
  Подробности — в `third_party/README.md`.
- **Linux/X11 — заглушка**: кодовая база платформо-независима, сборщик
  метрик закрыт интерфейсом `MetricsCollector`; X11-реализация
  подключается фабрикой `CreateCollector()` позже.

## Структура
```
include/agent/
  Platform.h          // платформенные макросы и защита
  Metric.h            // MetricRecord, Package (формат API)
  Config.h            // интервалы 5с/30с, batch 10, лимит 100, endpoint
  MetricsCollector.h  // интерфейс сборщика + фабрика по платформе
  MetricsQueue.h      // потокобезопасный буфер с лимитом
  Sender.h            // цикл отправки JSON + retry
  BackupWriter.h      // сброс буфера в backup.json
  SignalHandler.h     // SIGINT/SIGTERM/Ctrl+C -> флаг остановки
src/
  main.cpp            // точка входа (пока каркас)
third_party/          // httplib.h, nlohmann/json.hpp
```

## Сборка
```
cmake -S . -B build
cmake --build build
```
