#include "minikv/kv.h"
#include "minikv/memtable/memtable.h"
#include "minikv/wal/wal.h"
#include <filesystem>
#include <memory>
#include <regex>

namespace minikv {

struct DB::Impl {
  Options opts;
  std::string path;
  bool closed = false;
  MemTable memtable;
  std::unique_ptr<WAL> wal;
  uint64_t log_number = 0;
  uint64_t next_file_number = 1;

  void scan_wal_files() {
    namespace fs = std::filesystem;
    uint64_t max_num = 0;

    for (const auto& entry : fs::directory_iterator(path)) {
      if (!entry.is_regular_file()) continue;
      std::string name = entry.path().filename().string();

      std::regex re(R"(wal_(\d+)\.log)");
      std::smatch match;
      if (std::regex_match(name, match, re)) {
        uint64_t num = std::stoull(match[1].str());
        if (num >= next_file_number) {
          next_file_number = num + 1;
        }
        if (num > max_num) {
          max_num = num;
        }
      }
    }

    log_number = max_num;
  }

  void replay_wal() {
    if (log_number == 0) return;

    WAL replay_wal;
    replay_wal.open(path, log_number);
    auto records = replay_wal.read_all();
    replay_wal.close();

    for (const auto& rec : records) {
      if (rec.type == WAL::Record::PUT) {
        memtable.put(rec.key, rec.value);
      } else if (rec.type == WAL::Record::DELETE) {
        memtable.del(rec.key);
      }
    }
  }

  void create_new_wal() {
    log_number = next_file_number++;
    wal = std::make_unique<WAL>();
    wal->open(path, log_number);
  }

  void sync_wal() {
    if (!opts.wal_enabled || !wal) return;

    switch (opts.wal_sync_mode) {
      case Options::SYNC_EVERY_WRITE:
        wal->sync();
        break;
      case Options::SYNC_BATCH:
        break;
      case Options::SYNC_NONE:
        break;
    }
  }
};

std::optional<DB> DB::open(const std::string& path, const Options& opts) {
  namespace fs = std::filesystem;
  if (!fs::exists(path)) {
    fs::create_directories(path);
  }

  DB db;
  db.p_ = std::make_unique<Impl>();
  db.p_->path = path;
  db.p_->opts = opts;

  if (opts.wal_enabled) {
    db.p_->scan_wal_files();
    db.p_->replay_wal();
    db.p_->create_new_wal();
  }

  return db;
}

bool DB::put(std::string_view key, std::string_view value) {
  if (p_->closed) return false;

  if (p_->opts.wal_enabled && p_->wal) {
    p_->wal->append_put(key, value);
    p_->sync_wal();
  }

  p_->memtable.put(key, value);
  return true;
}

std::optional<std::string> DB::get(std::string_view key) const {
  if (p_->closed) return std::nullopt;
  return p_->memtable.get(key);
}

bool DB::del(std::string_view key) {
  if (p_->closed) return false;

  if (p_->opts.wal_enabled && p_->wal) {
    p_->wal->append_delete(key);
    p_->sync_wal();
  }

  p_->memtable.del(key);
  return true;
}

void DB::close() {
  if (p_ && !p_->closed) {
    if (p_->wal) {
      p_->wal->close();
    }
    p_->closed = true;
  }
}

DB::~DB() { close(); }

DB::DB(DB&&) noexcept = default;
DB& DB::operator=(DB&&) noexcept = default;

} // namespace minikv
