cmake_minimum_required(VERSION 3.16)
project(minikv VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler flags ──────────────────────────────────────────────
if(MSVC)
  add_compile_options(/W4 /utf-8)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# ── Library target ──────────────────────────────────────────────
set(MINIKV_SOURCES
  src/kv.cpp
  src/memtable/memtable.cpp
  src/memtable/skiplist.h
)

set(MINIKV_HEADERS
  include/minikv/kv.h
  include/minikv/options.h
  include/minikv/memtable/memtable.h
)

add_library(minikv STATIC ${MINIKV_SOURCES} ${MINIKV_HEADERS})
target_include_directories(minikv PUBLIC include)

# ── Demo binary ─────────────────────────────────────────────────
add_executable(minikv-cli src/main.cpp)
target_link_libraries(minikv-cli PRIVATE minikv)

# ── Tests ───────────────────────────────────────────────────────
enable_testing()
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)
include(Catch)

add_executable(minikv-tests
  tests/test_kv.cpp
  tests/test_memtable.cpp
)
target_link_libraries(minikv-tests PRIVATE minikv Catch2::Catch2WithMain)
catch_discover_tests(minikv-tests)
