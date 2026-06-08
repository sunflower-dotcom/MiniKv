#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <optional>

#include "minikv/options.h"

namespace minikv {

// ── Key-Value 存储公共接口 ──────────────────────────────────────
//
//  LSM-Tree 结构（7 阶段逐步实现）：
//  1. MemTable  - 内存跳表，写入入口
//  2. WAL       - 预写日志，崩溃恢复
//  3. SSTable   - 磁盘有序不可变文件
//  4. Compaction - 多 SSTable 归并合并
//  5. GC        - 过期版本回收
//  6. Bloom     - 加速不存在的 key 判断
//  7. Benchmark - 性能对比

class DB {
public:
  // 打开或创建数据库
  static std::optional<DB> open(const std::string& path,
                                const Options& opts = Options{});

  // 写入键值对
  bool put(std::string_view key, std::string_view value);

  // 读取键对应的值
  std::optional<std::string> get(std::string_view key) const;

  // 删除键
  bool del(std::string_view key);

  // 关闭数据库（析构也会自动调用）
  void close();

  ~DB();

  // 禁止拷贝，允许移动
  DB(const DB&) = delete;
  DB& operator=(const DB&) = delete;
  DB(DB&&) noexcept;
  DB& operator=(DB&&) noexcept;

private:
  DB() = default;
  struct Impl;
  std::unique_ptr<Impl> p_;
};

} // namespace minikv
