cmake_minimum_required(VERSION 3.14)
project(gamecore_zone_core)

# 平級獨立專案：本專案只負責 C++ 核心（zone_core 靜態庫）與選用的 Godot
# GDExtension 綁定（zone_gd）。測試是另一個獨立專案 projects/tests，會 find/link
# 本專案已建好的 zone_core，不再由此 add_subdirectory。

# CMake 4.0+ 不再接受 cmake_minimum_required(VERSION < 3.5)。
# 許多 FetchContent 依賴（spdlog v1.14.1 等）仍用舊宣告；透過此變數相容。
if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
    set(CMAKE_POLICY_VERSION_MINIMUM "3.5" CACHE STRING
        "Allow FetchContent deps that still declare cmake_minimum_required < 3.5 (CMake 4.0+ compat)")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# UTF-8 source files (Traditional Chinese comments); tell MSVC explicitly.
if(MSVC)
    add_compile_options(/utf-8)
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(BUILD_PATH ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY             "${BUILD_PATH}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY             "${BUILD_PATH}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY             "${BUILD_PATH}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG       "${BUILD_PATH}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE     "${BUILD_PATH}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG       "${BUILD_PATH}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE     "${BUILD_PATH}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG       "${BUILD_PATH}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE     "${BUILD_PATH}")

# ---- vendored dependencies (FetchContent) ----
include(FetchContent)

# 與 medps 對齊：EnTT v3.16.0、cereal v1.3.2
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

# cereal: 只要 header 介面，關掉所有附加建置
set(SKIP_PERFORMANCE_COMPARISON ON  CACHE BOOL "" FORCE)
set(BUILD_SANDBOX               OFF CACHE BOOL "" FORCE)
set(BUILD_DOC                   OFF CACHE BOOL "" FORCE)
set(WITH_WERROR                 OFF CACHE BOOL "" FORCE)
# spdlog: 不建範例與測試
set(SPDLOG_BUILD_EXAMPLE        OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS          OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(entt cereal spdlog)

# ---- zone_core（godot-free STATIC）----
set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/src")
file(GLOB_RECURSE core_src
    "${SRC}/*.cpp" "${SRC}/*.hpp" "${SRC}/*.h"
)
# 核心編譯單元不得包含前端；/gbind/ 是 GDExtension 綁定，另行建置
list(FILTER core_src EXCLUDE REGEX "/gbind/")
source_group(TREE "${SRC}" FILES ${core_src})

add_library(zone_core STATIC ${core_src})
target_compile_features(zone_core PUBLIC cxx_std_20)
target_include_directories(zone_core PUBLIC "${SRC}")
target_link_libraries(zone_core PUBLIC
    EnTT::EnTT
    cereal::cereal
    spdlog::spdlog
)

# ---- 選用：Godot 4 GDExtension 前端綁定（暫緩，預設 OFF）----
option(ZONE_BUILD_GDEXTENSION
    "Build the Godot 4 GDExtension facade (src/gbind)" OFF)
if(ZONE_BUILD_GDEXTENSION)
    # 預設指向 repo 外的 godot-cpp。本專案搬進 projects/core/ 後，比舊 repo-root
    # 多兩層，故相對路徑由 ../../projects 改為 ../../../../projects。
    # 建議改用 -DGODOT_CPP_DIR=<絕對路徑> 覆寫，較不受目錄深度影響。
    set(GODOT_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../projects/godot-cpp"
        CACHE PATH "Path to the godot-cpp checkout")
    add_subdirectory("${GODOT_CPP_DIR}" "${CMAKE_BINARY_DIR}/godot-cpp-build")

    file(GLOB_RECURSE gbind_src "${SRC}/gbind/*.cpp" "${SRC}/gbind/*.h")
    add_library(zone_gd SHARED ${gbind_src})
    set_target_properties(zone_gd PROPERTIES CXX_STANDARD 20)
    target_include_directories(zone_gd PRIVATE "${SRC}")
    target_link_libraries(zone_gd PRIVATE zone_core godot-cpp)
    # 資料目錄（原型 YAML 等）傳入編譯期常數
    target_compile_definitions(zone_gd PRIVATE
        ZONE_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    )

    # 建置後自動把 .so/.dll 複製到 Godot 專案的 res://bin/，
    # 省去每次重編後手動複製（projects/godot_zone/bin/ 為 gitignore）。
    add_custom_command(TARGET zone_gd POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/../godot_zone/bin"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:zone_gd>" "${CMAKE_CURRENT_SOURCE_DIR}/../godot_zone/bin/"
        COMMENT "複製 $<TARGET_FILE_NAME:zone_gd> → projects/godot_zone/bin/"
        VERBATIM
    )
endif()
