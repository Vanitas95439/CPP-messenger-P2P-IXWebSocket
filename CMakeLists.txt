cmake_minimum_required(VERSION 3.15)
project(SimpleMessenger)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Пути к исходникам
file(GLOB SOURCES "src/*.cpp")

# Добавляем библиотеку IXWebSocket
set(USE_ZLIB OFF CACHE BOOL "Disable ZLIB support" FORCE)
set(IXWEBSOCKET_USE_OPENSSL OFF CACHE BOOL "Disable OpenSSL" FORCE)

include(FetchContent)
FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(ixwebsocket)

# Добавляем .h файлы
include_directories(${PROJECT_SOURCE_DIR}/include)

# Создаем исполняемый файл
add_executable(${PROJECT_NAME} ${SOURCES})

# Линкуем с IXWebSocket
target_link_libraries(${PROJECT_NAME} PRIVATE ixwebsocket)

