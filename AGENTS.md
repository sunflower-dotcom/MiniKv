# AGENTS.md — MiniKV

## Build & Test

```bash
# Configure (defaults to Debug; use -DCMAKE_BUILD_TYPE=Release for release)
cmake -B build
cmake --build build

# Run all tests (CMake-based test runner)
cd build && ctest

# Or run the test binary directly
./build/minikv-tests

# Run a single test by tag
./build/minikv-tests "[kv]"
./build/minikv-tests "[memtable]"
./build/minikv-tests "[wal]"

# Interactive CLI
./build/minikv-cli
```

## Architecture

- **Static library** `minikv` (src/) + **public header** in `include/minikv/`
- **Phase-based development** (7 stages in README.md). Currently: Phase 1 (skeleton) done; Phase 2 (MemTable) has a real SkipList implementation in `src/memtable/skiplist.h`; Phase 3 (WAL) has a real write-ahead log implementation in `src/wal/wal.cpp`.
- **PIMPL idiom**: every public class (`DB`, `MemTable`) hides implementation in a `struct Impl` with `std::unique_ptr<Impl> p_`
- **Tombstone pattern**: deletions are represented as `__TOMBSTONE__` sentinel values in MemTable
- **Config namespace**: all public types are in `namespace minikv`
- Options struct lives in `include/minikv/options.h`

## Dev Conventions

- C++20, `#pragma once`, snake_case filenames
- MSVC: `/W4 /utf-8`; Clang/GCC: `-Wall -Wextra -Wpedantic -Werror`
- Catch2 v3 (fetched via FetchContent) for tests; test files registered in `CMakeLists.txt` with `catch_discover_tests`
- Test databases write to `test_tmp/` (`.gitignore`d). Do not commit test data.
- `CMakeLists.txt` is the single source of truth for source files; adding new `.cpp`/`.h` files requires updating it
