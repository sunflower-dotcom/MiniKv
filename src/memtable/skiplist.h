#pragma once
#include <cstddef>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace minikv {

class SkipList {
  static constexpr int kMaxLevel = 16;
  static constexpr double kProbability = 0.5;

  struct Node {
    std::string key;
    std::string value;
    std::vector<Node*> next;
    Node(std::string k, std::string v, int level)
        : key(std::move(k)), value(std::move(v)), next(level, nullptr) {}
  };

public:
  SkipList() : level_(1), size_bytes_(0), rng_(std::random_device{}()) {
    head_ = new Node("", "", kMaxLevel);
  }

  ~SkipList() {
    clear();
    delete head_;
  }

  SkipList(const SkipList&) = delete;
  SkipList& operator=(const SkipList&) = delete;
  SkipList(SkipList&&) = delete;
  SkipList& operator=(SkipList&&) = delete;

  void put(std::string_view key, std::string_view value) {
    Node* update[kMaxLevel]{};
    Node* cur = head_;
    for (int i = level_ - 1; i >= 0; --i) {
      while (cur->next[i] != nullptr &&
             std::less<>{}(cur->next[i]->key, key)) {
        cur = cur->next[i];
      }
      update[i] = cur;
    }

    cur = cur->next[0];
    if (cur != nullptr && cur->key == key) {
      size_bytes_ -= cur->key.size() + cur->value.size();
      cur->value = value;
      size_bytes_ += cur->key.size() + cur->value.size();
      return;
    }

    int new_level = random_level();
    if (new_level > level_) {
      for (int i = level_; i < new_level; ++i) {
        update[i] = head_;
      }
      level_ = new_level;
    }

    auto* node = new Node(std::string(key), std::string(value), new_level);
    size_bytes_ += node->key.size() + node->value.size();
    for (int i = 0; i < new_level; ++i) {
      node->next[i] = update[i]->next[i];
      update[i]->next[i] = node;
    }
  }

  std::optional<std::string> get(std::string_view key) const {
    const Node* cur = head_;
    for (int i = level_ - 1; i >= 0; --i) {
      while (cur->next[i] != nullptr &&
             std::less<>{}(cur->next[i]->key, key)) {
        cur = cur->next[i];
      }
    }
    cur = cur->next[0];
    if (cur != nullptr && cur->key == key) {
      return cur->value;
    }
    return std::nullopt;
  }

  size_t size_bytes() const { return size_bytes_; }

  std::vector<std::pair<std::string, std::string>> iter() const {
    std::vector<std::pair<std::string, std::string>> result;
    const Node* cur = head_->next[0];
    while (cur != nullptr) {
      if (cur->value != "__TOMBSTONE__") {
        result.emplace_back(cur->key, cur->value);
      }
      cur = cur->next[0];
    }
    return result;
  }

  void clear() {
    Node* cur = head_->next[0];
    while (cur != nullptr) {
      Node* tmp = cur;
      cur = cur->next[0];
      delete tmp;
    }
    for (int i = 0; i < kMaxLevel; ++i) {
      head_->next[i] = nullptr;
    }
    level_ = 1;
    size_bytes_ = 0;
  }

private:
  int random_level() {
    int level = 1;
    while (level < kMaxLevel &&
           std::uniform_real_distribution<double>(0.0, 1.0)(rng_) <
               kProbability) {
      ++level;
    }
    return level;
  }

  Node* head_;
  int level_;
  size_t size_bytes_;
  std::mt19937 rng_;
};

} // namespace minikv
