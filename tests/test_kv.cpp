#include <catch2/catch_test_macros.hpp>
#include "minikv/kv.h"
#include "minikv/options.h"
#include <filesystem>

namespace fs = std::filesystem;

TEST_CASE("DB::open creates directory", "[kv]") {
  fs::remove_all("./test_tmp/opendir");
  {
    auto db = minikv::DB::open("./test_tmp/opendir");
    REQUIRE(db.has_value());
    REQUIRE(fs::exists("./test_tmp/opendir"));
    db->close();
  }
  fs::remove_all("./test_tmp/opendir");
}

TEST_CASE("DB::put and get", "[kv]") {
  fs::remove_all("./test_tmp/db_basic");
  auto db = minikv::DB::open("./test_tmp/db_basic");
  REQUIRE(db.has_value());

  db->put("key1", "value1");
  db->put("key2", "value2");

  REQUIRE(db->get("key1") == "value1");
  REQUIRE(db->get("key2") == "value2");
  REQUIRE(db->get("missing") == std::nullopt);

  db->close();
  fs::remove_all("./test_tmp/db_basic");
}

TEST_CASE("DB::overwrite", "[kv]") {
  fs::remove_all("./test_tmp/db_overwrite");
  auto db = minikv::DB::open("./test_tmp/db_overwrite");
  REQUIRE(db.has_value());

  db->put("key", "old");
  db->put("key", "new");
  REQUIRE(db->get("key") == "new");

  db->close();
  fs::remove_all("./test_tmp/db_overwrite");
}

TEST_CASE("DB::del", "[kv]") {
  fs::remove_all("./test_tmp/db_del");
  auto db = minikv::DB::open("./test_tmp/db_del");
  REQUIRE(db.has_value());

  db->put("key", "value");
  db->del("key");

  auto result = db->get("key");
  REQUIRE(result.has_value());
  REQUIRE(*result == "__TOMBSTONE__");

  db->close();
  fs::remove_all("./test_tmp/db_del");
}

TEST_CASE("DB::crash recovery", "[kv]") {
  fs::remove_all("./test_tmp/db_recover");

  {
    auto db = minikv::DB::open("./test_tmp/db_recover");
    REQUIRE(db.has_value());
    db->put("key1", "value1");
    db->put("key2", "value2");
    db->close();
  }

  {
    auto db = minikv::DB::open("./test_tmp/db_recover");
    REQUIRE(db.has_value());
    REQUIRE(db->get("key1") == "value1");
    REQUIRE(db->get("key2") == "value2");
    db->close();
  }

  fs::remove_all("./test_tmp/db_recover");
}
