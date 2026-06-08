#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <optional>
#include <vector>

namespace minikv {

// ── MemTable：内存跳表 ──────────────────────────────────────────
//
//  接口设计保持简单，内部使用跳表实现 O(log n) 读写。
//  当 size() 超过阈值时，上层（DB）负责 flush 到 SSTable。

class MemTable {
public:
  MemTable();
  ~MemTable();

  // 写入 kv，如果 key 已存在则覆盖
  void put(std::string_view key, std::string_view value);

  // 读取 key 对应的 value，不存在返回 nullopt
  std::optional<std::string> get(std::string_view key) const;

  // 删除 key（写入墓碑标记）
  void del(std::string_view key);

  // 当前内存占用（字节），用于判断是否需要 flush
  size_t approximate_size() const;

  // 按序遍历所有非删除的 key-value，用于 flush
  std::vector<std::pair<std::string, std::string>> iter() const;

  // 清空（flush 之后调用）
  void clear();

private:
  struct Impl;
  std::unique_ptr<Impl> p_;
};

} // namespace minikv
