#include <iostream>
#include "minikv/kv.h"
#include "minikv/options.h"

int main() {
  auto db = minikv::DB::open("./testdb");
  if (!db.has_value()) {
    std::cerr << "Failed to open database\n";
    return 1;
  }

  std::cout << "MiniKV — LSM-Tree Storage Engine v0.1.0\n";
  std::cout << "Ready. Type commands: put <k> <v> | get <k> | del <k> | quit\n";

  std::string cmd, key, value;
  while (std::cin >> cmd) {
    if (cmd == "quit") break;
    if (cmd == "put") {
      std::cin >> key >> value;
      db->put(key, value);
      std::cout << "OK\n";
    } else if (cmd == "get") {
      std::cin >> key;
      auto v = db->get(key);
      if (v) std::cout << *v << "\n";
      else   std::cout << "(not found)\n";
    } else if (cmd == "del") {
      std::cin >> key;
      db->del(key);
      std::cout << "OK\n";
    }
  }

  return 0;
}
