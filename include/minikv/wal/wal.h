#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace minikv {

class WAL {
public:
  WAL();
  ~WAL();

  WAL(const WAL&) = delete;
  WAL& operator=(const WAL&) = delete;
  WAL(WAL&&) noexcept;
  WAL& operator=(WAL&&) noexcept;

  void open(const std::string& dir, uint64_t log_number);
  void close();
  void remove();

  void append_put(std::string_view key, std::string_view value);
  void append_delete(std::string_view key);
  void sync();

  bool is_open() const;
  uint64_t log_number() const;
  std::string file_path() const;

  struct Record {
    enum Type { PUT, DELETE };
    Type type;
    std::string key;
    std::string value;
  };
  std::vector<Record> read_all() const;

private:
  struct Impl;
  std::unique_ptr<Impl> p_;
};

} // namespace minikv
