#include <catch2/catch_test_macros.hpp>
#include "minikv/wal/wal.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static const std::string kTestDir = "./test_tmp/wal_test";

TEST_CASE("WAL create and close", "[wal]") {
  fs::remove_all(kTestDir);
  fs::create_directories(kTestDir);

  minikv::WAL wal;
  REQUIRE_FALSE(wal.is_open());

  wal.open(kTestDir, 1);
  REQUIRE(wal.is_open());
  REQUIRE(wal.log_number() == 1);
  REQUIRE(fs::exists(wal.file_path()));

  wal.close();
  REQUIRE_FALSE(wal.is_open());

  fs::remove_all(kTestDir);
}

TEST_CASE("WAL append and read", "[wal]") {
  fs::remove_all(kTestDir);
  fs::create_directories(kTestDir);

  minikv::WAL wal;
  wal.open(kTestDir, 1);

  wal.append_put("key1", "value1");
  wal.append_put("key2", "value2");
  wal.append_delete("key3");
  wal.append_put("key4", "value4");

  wal.sync();
  wal.close();

  auto records = wal.read_all();
  REQUIRE(records.size() == 4);

  REQUIRE(records[0].type == minikv::WAL::Record::PUT);
  REQUIRE(records[0].key == "key1");
  REQUIRE(records[0].value == "value1");

  REQUIRE(records[1].type == minikv::WAL::Record::PUT);
  REQUIRE(records[1].key == "key2");
  REQUIRE(records[1].value == "value2");

  REQUIRE(records[2].type == minikv::WAL::Record::DELETE);
  REQUIRE(records[2].key == "key3");

  REQUIRE(records[3].type == minikv::WAL::Record::PUT);
  REQUIRE(records[3].key == "key4");
  REQUIRE(records[3].value == "value4");

  fs::remove_all(kTestDir);
}

TEST_CASE("WAL empty read", "[wal]") {
  fs::remove_all(kTestDir);
  fs::create_directories(kTestDir);

  minikv::WAL wal;
  wal.open(kTestDir, 1);
  wal.close();

  auto records = wal.read_all();
  REQUIRE(records.empty());

  fs::remove_all(kTestDir);
}

TEST_CASE("WAL large value", "[wal]") {
  fs::remove_all(kTestDir);
  fs::create_directories(kTestDir);

  minikv::WAL wal;
  wal.open(kTestDir, 1);

  std::string large_value(4000, 'x');
  wal.append_put("big", large_value);
  wal.sync();
  wal.close();

  auto records = wal.read_all();
  REQUIRE(records.size() == 1);
  REQUIRE(records[0].type == minikv::WAL::Record::PUT);
  REQUIRE(records[0].key == "big");
  REQUIRE(records[0].value == large_value);

  fs::remove_all(kTestDir);
}

TEST_CASE("WAL remove file", "[wal]") {
  fs::remove_all(kTestDir);
  fs::create_directories(kTestDir);

  minikv::WAL wal;
  wal.open(kTestDir, 1);
  std::string path = wal.file_path();
  REQUIRE(fs::exists(path));

  wal.remove();
  REQUIRE_FALSE(fs::exists(path));

  fs::remove_all(kTestDir);
}

TEST_CASE("WAL file rotation", "[wal]") {
  fs::remove_all(kTestDir);
  fs::create_directories(kTestDir);

  minikv::WAL wal1;
  wal1.open(kTestDir, 1);
  wal1.append_put("a", "1");
  wal1.sync();
  wal1.close();

  minikv::WAL wal2;
  wal2.open(kTestDir, 2);
  wal2.append_put("b", "2");
  wal2.sync();
  wal2.close();

  REQUIRE(fs::exists(wal1.file_path()));
  REQUIRE(fs::exists(wal2.file_path()));

  auto records1 = wal1.read_all();
  REQUIRE(records1.size() == 1);
  REQUIRE(records1[0].key == "a");

  auto records2 = wal2.read_all();
  REQUIRE(records2.size() == 1);
  REQUIRE(records2[0].key == "b");

  wal1.remove();
  wal2.remove();
  fs::remove_all(kTestDir);
}
