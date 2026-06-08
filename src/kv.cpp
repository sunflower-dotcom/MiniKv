#include "minikv/kv.h"
#include "minikv/options.h"
#include <memory>
#include <filesystem>

namespace minikv {

struct DB::Impl {
  Options opts;
  std::string path;
  bool closed = false;
};

std::optional<DB> DB::open(const std::string& path, const Options& opts) {
  namespace fs = std::filesystem;
  auto dir = fs::path(path);
  if (!fs::exists(dir)) {
    fs::create_directories(dir);
  }
  DB db;
  db.p_ = std::make_unique<Impl>();
  db.p_->path = path;
  db.p_->opts = opts;
  return db;
}

bool DB::put(std::string_view key, std::string_view value) {
  (void)key; (void)value;
  // TODO: 阶段 2 — 写入 MemTable
  return true;
}

std::optional<std::string> DB::get(std::string_view key) const {
  (void)key;
  // TODO: 阶段 2 — 先查 MemTable，再查 SSTable
  return std::nullopt;
}

bool DB::del(std::string_view key) {
  (void)key;
  // TODO: 阶段 2 — 写入墓碑标记到 MemTable
  return true;
}

void DB::close() {
  if (p_) p_->closed = true;
}

DB::~DB() { close(); }

DB::DB(DB&&) noexcept = default;
DB& DB::operator=(DB&&) noexcept = default;

} // namespace minikv
