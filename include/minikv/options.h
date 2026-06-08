#pragma once
#include <cstddef>
#include <string>

namespace minikv {

struct Options {
  // 内存表大小阈值（字节），超过后 flush 到 SSTable
  size_t memtable_size_limit = 4 * 1024 * 1024; // 4MB

  // 数据目录
  std::string data_dir = ".";

  // 是否启用 WAL（预写日志）
  bool wal_enabled = true;

  // SSTable 块大小
  size_t block_size = 4096; // 4KB
};

} // namespace minikv
