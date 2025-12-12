cmake_minimum_required(VERSION 3.10)

project(lab_07  # ← Исправлено: имя проекта соответствует лабе 7
    VERSION 1.0.0
    DESCRIPTION "LAB7"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)

# Список всех файлов реализации
set(SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/npc.cpp
    ${SRC_DIR}/dragon.cpp
    ${SRC_DIR}/bull.cpp 
    ${SRC_DIR}/frog.cpp 
)

# === 🔥 Обязательно: найти потоки ===
find_package(Threads REQUIRED)

# Основной исполняемый файл
add_executable(lab_07_exe ${SOURCES})
target_include_directories(lab_07_exe PRIVATE ${INCLUDE_DIR})
target_link_libraries(lab_07_exe PRIVATE Threads::Threads)  # ← ЭТА СТРОКА БЫЛА ПРОПУЩЕНА!

# Подключаем GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git  
    GIT_TAG        v1.15.0
)
FetchContent_MakeAvailable(googletest)

# Тесты
set(TEST_SOURCES
    ${TEST_DIR}/tests.cpp
    ${SRC_DIR}/npc.cpp
    ${SRC_DIR}/dragon.cpp
    ${SRC_DIR}/bull.cpp
    ${SRC_DIR}/frog.cpp
)

add_executable(lab_07_tests ${TEST_SOURCES})
target_include_directories(lab_07_tests PRIVATE ${INCLUDE_DIR})
target_link_libraries(lab_07_tests PRIVATE GTest::gtest GTest::gtest_main Threads::Threads)

# Обнаружение тестов
include(GoogleTest)
gtest_discover_tests(lab_07_tests)

# Предупреждения
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(lab_07_exe PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(lab_07_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()