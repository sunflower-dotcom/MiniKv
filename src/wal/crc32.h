#pragma once
#include <cstdint>
#include <cstddef>

namespace minikv {
namespace crc32 {

inline uint32_t table[256];

inline void init_table() {
  for (uint32_t i = 0; i < 256; ++i) {
    uint32_t crc = i;
    for (int j = 0; j < 8; ++j) {
      crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320u : (crc >> 1);
    }
    table[i] = crc;
  }
}

inline uint32_t compute(const void* data, size_t len) {
  static bool initialized = false;
  if (!initialized) {
    init_table();
    initialized = true;
  }

  auto* p = static_cast<const uint8_t*>(data);
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < len; ++i) {
    crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
  }
  return crc ^ 0xFFFFFFFFu;
}

} // namespace crc32
} // namespace minikv
