#include "minikv/memtable/memtable.h"
#include "skiplist.h"
#include <mutex>

namespace minikv {

struct MemTable::Impl {
  SkipList data;
  std::mutex mtx;
};

MemTable::MemTable() : p_(std::make_unique<Impl>()) {}
MemTable::~MemTable() = default;

void MemTable::put(std::string_view key, std::string_view value) {
  std::lock_guard lock(p_->mtx);
  p_->data.put(key, value);
}

std::optional<std::string> MemTable::get(std::string_view key) const {
  std::lock_guard lock(p_->mtx);
  return p_->data.get(key);
}

void MemTable::del(std::string_view key) {
  put(key, "__TOMBSTONE__");
}

size_t MemTable::approximate_size() const {
  std::lock_guard lock(p_->mtx);
  return p_->data.size_bytes();
}

std::vector<std::pair<std::string, std::string>> MemTable::iter() const {
  std::lock_guard lock(p_->mtx);
  return p_->data.iter();
}

void MemTable::clear() {
  std::lock_guard lock(p_->mtx);
  p_->data.clear();
}

} // namespace minikv
