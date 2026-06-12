#include "minikv/wal/wal.h"
#include "crc32.h"
#include <cstring>
#include <filesystem>
#include <optional>
#include <vector>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#define WAL_OPEN _open
#define WAL_WRITE _write
#define WAL_READ _read
#define WAL_CLOSE _close
#define WAL_SYNC _commit
#define WAL_SSIZE ssize_t
#else
#include <unistd.h>
#include <fcntl.h>
#define WAL_OPEN open
#define WAL_WRITE write
#define WAL_READ read
#define WAL_CLOSE close
#define WAL_SYNC fdatasync
#define WAL_SSIZE ssize_t
#endif

namespace minikv {

static constexpr size_t kBlockSize = 4096;
static constexpr size_t kHeaderSize = 7;

enum RecordType : uint8_t {
  FULL = 1,
  FIRST = 2,
  MIDDLE = 3,
  LAST = 4
};

enum OpType : uint8_t {
  OP_PUT = 1,
  OP_DELETE = 2
};

struct WAL::Impl {
  int fd = -1;
  uint64_t log_number_ = 0;
  std::string dir_;
  size_t block_offset_ = 0;
  bool opened_ = false;

  std::string file_path() const {
    char buf[32];
    snprintf(buf, sizeof(buf), "wal_%06llu.log",
             static_cast<unsigned long long>(log_number_));
    return dir_ + "/" + buf;
  }

  void pad_block() {
    size_t remaining = kBlockSize - block_offset_;
    if (remaining > 0) {
      std::vector<uint8_t> zeros(remaining, 0);
      WAL_WRITE(fd, zeros.data(), static_cast<unsigned int>(remaining));
    }
    block_offset_ = 0;
  }

  void emit_physical_record(RecordType type, const uint8_t* data, size_t len) {
    std::vector<uint8_t> crc_buf(1 + len);
    crc_buf[0] = static_cast<uint8_t>(type);
    memcpy(crc_buf.data() + 1, data, len);
    uint32_t crc = crc32::compute(crc_buf.data(), crc_buf.size());

    uint8_t header[kHeaderSize];
    memcpy(header, &crc, 4);
    header[4] = static_cast<uint8_t>(len & 0xFF);
    header[5] = static_cast<uint8_t>((len >> 8) & 0xFF);
    header[6] = static_cast<uint8_t>(type);

    WAL_WRITE(fd, header, kHeaderSize);
    WAL_WRITE(fd, data, static_cast<unsigned int>(len));

    block_offset_ += kHeaderSize + len;
  }

  void emit_record(const uint8_t* data, size_t len) {
    size_t avail = kBlockSize - block_offset_;

    if (avail < kHeaderSize) {
      pad_block();
      avail = kBlockSize;
    }

    size_t max_payload = avail - kHeaderSize;

    if (len <= max_payload) {
      emit_physical_record(FULL, data, len);
    } else {
      emit_physical_record(FIRST, data, max_payload);
      data += max_payload;
      len -= max_payload;

      while (len > kBlockSize - kHeaderSize) {
        pad_block();
        emit_physical_record(MIDDLE, data, kBlockSize - kHeaderSize);
        data += kBlockSize - kHeaderSize;
        len -= kBlockSize - kHeaderSize;
      }

      pad_block();
      emit_physical_record(LAST, data, len);
    }
  }

  struct PhysicalRecord {
    RecordType type;
    std::vector<uint8_t> data;
  };

  std::optional<PhysicalRecord> read_physical_record(const uint8_t* buf,
                                                     size_t buf_len,
                                                     size_t& offset) {
    if (offset + kHeaderSize > buf_len) {
      return std::nullopt;
    }

    uint32_t crc;
    memcpy(&crc, buf + offset, 4);
    uint16_t length = buf[offset + 4] | (buf[offset + 5] << 8);
    RecordType type = static_cast<RecordType>(buf[offset + 6]);

    if (offset + kHeaderSize + length > buf_len) {
      return std::nullopt;
    }

    const uint8_t* data = buf + offset + kHeaderSize;
    std::vector<uint8_t> crc_buf(1 + length);
    crc_buf[0] = static_cast<uint8_t>(type);
    memcpy(crc_buf.data() + 1, data, length);
    uint32_t computed_crc = crc32::compute(crc_buf.data(), crc_buf.size());

    if (crc != computed_crc) {
      return std::nullopt;
    }

    PhysicalRecord rec;
    rec.type = type;
    rec.data.assign(data, data + length);
    offset += kHeaderSize + length;

    return rec;
  }

  std::optional<WAL::Record> parse_logical_record(const uint8_t* data,
                                                  size_t len) {
    if (len < 3) return std::nullopt;

    uint8_t op = data[0];
    uint16_t key_len = data[1] | (data[2] << 8);

    if (len < static_cast<size_t>(3 + key_len)) return std::nullopt;

    WAL::Record rec;
    rec.key = std::string(reinterpret_cast<const char*>(data + 3), key_len);

    if (op == OP_PUT) {
      if (len < static_cast<size_t>(3 + key_len + 2)) return std::nullopt;
      uint16_t val_len = data[3 + key_len] | (data[4 + key_len] << 8);
      if (len < static_cast<size_t>(3 + key_len + 2 + val_len))
        return std::nullopt;
      rec.value = std::string(reinterpret_cast<const char*>(data + 5 + key_len),
                              val_len);
      rec.type = WAL::Record::PUT;
    } else if (op == OP_DELETE) {
      rec.type = WAL::Record::DELETE;
    } else {
      return std::nullopt;
    }

    return rec;
  }
};

WAL::WAL() : p_(std::make_unique<Impl>()) {}
WAL::~WAL() { close(); }
WAL::WAL(WAL&&) noexcept = default;
WAL& WAL::operator=(WAL&&) noexcept = default;

void WAL::open(const std::string& dir, uint64_t log_number) {
  close();

  p_->dir_ = dir;
  p_->log_number_ = log_number;
  p_->block_offset_ = 0;

  namespace fs = std::filesystem;
  if (!fs::exists(dir)) {
    fs::create_directories(dir);
  }

#ifdef _WIN32
  p_->fd = WAL_OPEN(p_->file_path().c_str(),
                    O_WRONLY | O_CREAT | O_APPEND | O_BINARY,
                    _S_IREAD | _S_IWRITE);
#else
  p_->fd = WAL_OPEN(p_->file_path().c_str(),
                    O_WRONLY | O_CREAT | O_APPEND, 0644);
#endif
  if (p_->fd < 0) {
    return;
  }

  p_->opened_ = true;
}

void WAL::close() {
  if (p_->fd >= 0) {
    WAL_CLOSE(p_->fd);
    p_->fd = -1;
  }
  p_->opened_ = false;
}

void WAL::remove() {
  close();
  namespace fs = std::filesystem;
  fs::remove(p_->file_path());
}

void WAL::append_put(std::string_view key, std::string_view value) {
  if (!p_->opened_) return;

  size_t payload_len = 1 + 2 + key.size() + 2 + value.size();
  std::vector<uint8_t> payload(payload_len);

  size_t offset = 0;
  payload[offset++] = OP_PUT;
  payload[offset++] = static_cast<uint8_t>(key.size() & 0xFF);
  payload[offset++] = static_cast<uint8_t>((key.size() >> 8) & 0xFF);
  memcpy(payload.data() + offset, key.data(), key.size());
  offset += key.size();
  payload[offset++] = static_cast<uint8_t>(value.size() & 0xFF);
  payload[offset++] = static_cast<uint8_t>((value.size() >> 8) & 0xFF);
  memcpy(payload.data() + offset, value.data(), value.size());

  p_->emit_record(payload.data(), payload.size());
}

void WAL::append_delete(std::string_view key) {
  if (!p_->opened_) return;

  size_t payload_len = 1 + 2 + key.size();
  std::vector<uint8_t> payload(payload_len);

  size_t offset = 0;
  payload[offset++] = OP_DELETE;
  payload[offset++] = static_cast<uint8_t>(key.size() & 0xFF);
  payload[offset++] = static_cast<uint8_t>((key.size() >> 8) & 0xFF);
  memcpy(payload.data() + offset, key.data(), key.size());

  p_->emit_record(payload.data(), payload.size());
}

void WAL::sync() {
  if (p_->fd >= 0) {
    WAL_SYNC(p_->fd);
  }
}

bool WAL::is_open() const { return p_->opened_; }
uint64_t WAL::log_number() const { return p_->log_number_; }
std::string WAL::file_path() const { return p_->file_path(); }

std::vector<WAL::Record> WAL::read_all() const {
  std::vector<Record> result;

  namespace fs = std::filesystem;
  if (!fs::exists(file_path())) {
    return result;
  }

  auto file_size = fs::file_size(file_path());
  if (file_size == 0) {
    return result;
  }

#ifdef _WIN32
  int read_fd = WAL_OPEN(file_path().c_str(), O_RDONLY | O_BINARY);
#else
  int read_fd = WAL_OPEN(file_path().c_str(), O_RDONLY);
#endif
  if (read_fd < 0) {
    return result;
  }

  std::vector<uint8_t> file_data(static_cast<size_t>(file_size));
  size_t total_read = 0;
  while (total_read < file_size) {
    WAL_SSIZE n = WAL_READ(read_fd, file_data.data() + total_read,
                           static_cast<unsigned int>(file_size - total_read));
    if (n <= 0) break;
    total_read += static_cast<size_t>(n);
  }
  WAL_CLOSE(read_fd);

  size_t offset = 0;
  std::vector<uint8_t> assembled_data;
  bool assembling = false;

  while (offset < file_data.size()) {
    if (offset + kHeaderSize > file_data.size()) break;

    bool all_zero = true;
    for (size_t i = offset; i < std::min(offset + kHeaderSize, file_data.size());
         ++i) {
      if (file_data[i] != 0) {
        all_zero = false;
        break;
      }
    }
    if (all_zero) break;

    auto phys_rec =
        p_->read_physical_record(file_data.data(), file_data.size(), offset);
    if (!phys_rec) break;

    if (phys_rec->type == FULL) {
      auto logical = p_->parse_logical_record(phys_rec->data.data(),
                                              phys_rec->data.size());
      if (logical) {
        result.push_back(std::move(*logical));
      }
      assembling = false;
    } else if (phys_rec->type == FIRST) {
      assembled_data = std::move(phys_rec->data);
      assembling = true;
    } else if (phys_rec->type == MIDDLE && assembling) {
      assembled_data.insert(assembled_data.end(), phys_rec->data.begin(),
                           phys_rec->data.end());
    } else if (phys_rec->type == LAST && assembling) {
      assembled_data.insert(assembled_data.end(), phys_rec->data.begin(),
                           phys_rec->data.end());
      auto logical = p_->parse_logical_record(assembled_data.data(),
                                              assembled_data.size());
      if (logical) {
        result.push_back(std::move(*logical));
      }
      assembling = false;
    } else {
      assembling = false;
    }
  }

  return result;
}

} // namespace minikv
