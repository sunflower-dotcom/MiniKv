#include <catch2/catch_test_macros.hpp>
#include "minikv/memtable/memtable.h"

TEST_CASE("MemTable put and get", "[memtable]") {
  minikv::MemTable mt;
  mt.put("key1", "value1");
  mt.put("key2", "value2");

  REQUIRE(mt.get("key1") == "value1");
  REQUIRE(mt.get("key2") == "value2");
  REQUIRE(mt.get("missing") == std::nullopt);
}

TEST_CASE("MemTable overwrite", "[memtable]") {
  minikv::MemTable mt;
  mt.put("key", "old");
  mt.put("key", "new");

  REQUIRE(mt.get("key") == "new");
}

TEST_CASE("MemTable del and tombstone", "[memtable]") {
  minikv::MemTable mt;
  mt.put("key", "value");
  mt.del("key");

  // 删除后的 key 不可见
  auto result = mt.get("key");
  REQUIRE(result.has_value());
  REQUIRE(*result == "__TOMBSTONE__");

  // iter() 不返回已删除的 key
  auto entries = mt.iter();
  REQUIRE(entries.empty());
}

TEST_CASE("MemTable approximate_size", "[memtable]") {
  minikv::MemTable mt;
  REQUIRE(mt.approximate_size() == 0);

  mt.put("hello", "world");
  REQUIRE(mt.approximate_size() > 0);
}

TEST_CASE("MemTable clear", "[memtable]") {
  minikv::MemTable mt;
  mt.put("a", "1");
  mt.put("b", "2");
  mt.clear();

  REQUIRE(mt.approximate_size() == 0);
  REQUIRE(mt.get("a") == std::nullopt);
}
