cmake_minimum_required(VERSION 3.20)
project(monitoring_agent CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Header-only зависимости из third_party/ (см. third_party/README.md)
add_library(third_party INTERFACE)
target_include_directories(third_party INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

add_executable(agent
    src/main.cpp
    src/MetricsQueue.cpp
    src/Sender.cpp
    src/BackupWriter.cpp
    src/SignalHandler.cpp
)
target_include_directories(agent PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(agent PRIVATE third_party)

if (MSVC)
    # Исходники в UTF-8 (комментарии и строки на русском).
    target_compile_options(agent PRIVATE /utf-8)
endif()

if (WIN32)
    target_sources(agent PRIVATE src/CollectorWindows.cpp)
    # cpp-httplib (winsock)
    target_link_libraries(agent PRIVATE ws2_32)
else()
    target_sources(agent PRIVATE src/CollectorLinux.cpp)
    find_package(Threads REQUIRED)
    target_link_libraries(agent PRIVATE Threads::Threads)
    # X11: Xlib — окно в фокусе; Xss (X Screen Saver, пакет libxss-dev) —
    # простой с последнего ввода. CMake знает Xss как компонент X11.
    find_package(X11 REQUIRED COMPONENTS Xss)
    target_link_libraries(agent PRIVATE X11::X11 X11::Xss)
endif()