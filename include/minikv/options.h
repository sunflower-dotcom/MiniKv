#pragma once
#include <cstddef>
#include <string>

namespace minikv {

struct Options {
  size_t memtable_size_limit = 4 * 1024 * 1024;
  std::string data_dir = ".";
  bool wal_enabled = true;
  size_t block_size = 4096;

  enum SyncMode {
    SYNC_EVERY_WRITE,
    SYNC_BATCH,
    SYNC_NONE
  };
  SyncMode wal_sync_mode = SYNC_EVERY_WRITE;
  size_t wal_sync_batch_size = 64;
};

} // namespace minikv
