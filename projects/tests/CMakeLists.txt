cmake_minimum_required(VERSION 3.14)
project(gamecore_zone_tests)

# 平級獨立測試專案：本專案自行 find/link 平級 projects/core 已建好的 zone_core
# 靜態庫，並自備 doctest 與 zone_core header 需要的第三方 header（entt/cereal/spdlog）。
# 先建 projects/core（產出 zone_core 到 projects/core/build/bin），再建本專案。

if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
    set(CMAKE_POLICY_VERSION_MINIMUM "3.5" CACHE STRING
        "Allow FetchContent deps that still declare cmake_minimum_required < 3.5 (CMake 4.0+ compat)")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(BUILD_PATH "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BUILD_PATH}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${BUILD_PATH}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${BUILD_PATH}")

# ---- 平級的 core 專案：本測試獨立建置，link 它「已建好」的 zone_core ----
set(CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../core"
    CACHE PATH "Path to the sibling core project (headers under src/)")
set(CORE_LIB_DIR "${CORE_DIR}/build/bin"
    CACHE PATH "Directory containing the pre-built zone_core library")

find_library(ZONE_CORE_LIB
    NAMES zone_core libzone_core
    PATHS "${CORE_LIB_DIR}"
    NO_DEFAULT_PATH)
if(NOT ZONE_CORE_LIB)
    message(FATAL_ERROR
        "找不到 zone_core。請先建置 core：\n"
        "  cmake -S ${CORE_DIR} -B ${CORE_DIR}/build && cmake --build ${CORE_DIR}/build -j4\n"
        "或以 -DCORE_LIB_DIR=<路徑> 指定已建好的庫所在目錄（目前找的是 ${CORE_LIB_DIR}）。")
endif()

# ---- 測試自備的第三方 header/庫 ----
# zone_core 的 header 會 include entt/cereal/spdlog；zone_core 靜態庫也需要 spdlog
# 的符號在最終連結時可解析，故本專案同樣 FetchContent 這些依賴（版本與 core 對齊）。
include(FetchContent)
FetchContent_Declare(entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG        v3.16.0
    GIT_SHALLOW    TRUE
)
FetchContent_Declare(cereal
    GIT_REPOSITORY https://github.com/USCiLab/cereal.git
    GIT_TAG        v1.3.2
    GIT_SHALLOW    TRUE
)
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
    GIT_SHALLOW    TRUE
)
FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
    GIT_SHALLOW    TRUE
)
set(SKIP_PERFORMANCE_COMPARISON ON  CACHE BOOL "" FORCE)
set(BUILD_SANDBOX               OFF CACHE BOOL "" FORCE)
set(BUILD_DOC                   OFF CACHE BOOL "" FORCE)
set(WITH_WERROR                 OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE        OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS          OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(entt cereal spdlog doctest)

set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/src")
file(GLOB_RECURSE test_src "${SRC}/*.cpp")

add_executable(zone_test ${test_src})
target_compile_features(zone_test PRIVATE cxx_std_20)
target_include_directories(zone_test PRIVATE "${CORE_DIR}/src")
target_link_libraries(zone_test PRIVATE
    "${ZONE_CORE_LIB}"
    EnTT::EnTT
    cereal::cereal
    spdlog::spdlog
    doctest::doctest
)

# doctest 預設以前向宣告取代 <iosfwd>；MSVC 2022 + string_view 會因 ostream 不完整而報錯。
# 加此 define 讓 doctest 改用 #include <sstream>（完整標頭）。
target_compile_definitions(zone_test PRIVATE DOCTEST_CONFIG_USE_STD_HEADERS)

include(CTest)
enable_testing()
add_test(NAME zone_test COMMAND zone_test)
