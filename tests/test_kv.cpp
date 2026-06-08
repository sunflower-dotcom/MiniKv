#include <catch2/catch_test_macros.hpp>
#include "minikv/kv.h"
#include "minikv/options.h"

TEST_CASE("DB::open creates directory", "[kv]") {
  auto db = minikv::DB::open("./test_tmp/opendir");
  REQUIRE(db.has_value());
}

TEST_CASE("DB::put and get placeholder", "[kv]") {
  auto db = minikv::DB::open("./test_tmp/basic");
  REQUIRE(db.has_value());
  // 阶段 2 后将有实际 put/get 数据通路
  db->put("hello", "world");
  // get 目前返回 nullopt（骨架阶段）
}

TEST_CASE("DB::del placeholder", "[kv]") {
  auto db = minikv::DB::open("./test_tmp/del");
  REQUIRE(db.has_value());
  db->del("expired");
}
