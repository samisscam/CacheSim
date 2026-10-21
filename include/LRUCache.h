#pragma once
#include "EvictionPolicy.h"
#include <stdexcept>
#include <unordered_map>

// LRU eviction with O(1) get/put: a hash map indexes into a doubly linked
// list ordered by recency (head = most recent, tail = least recent).
// Sentinel head/tail nodes remove the empty-list edge cases.
template <typename K, typename V>
class LRUCache : public EvictionPolicy<K, V> {
    struct Node {
        K key;
        V value;
        Node* prev = nullptr;
        Node* next = nullptr;
        Node(const K& k, const V& v) : key(k), value(v) {}
    };

public:
    explicit LRUCache(std::size_t capacity) : capacity_(capacity) {
        if (capacity_ == 0) {
            throw std::invalid_argument("LRUCache capacity must be > 0");
        }
        head_ = new Node(K{}, V{});
        tail_ = new Node(K{}, V{});
        head_->next = tail_;
        tail_->prev = head_;
    }

    ~LRUCache() override {
        Node* cur = head_;
        while (cur) {
            Node* next = cur->next;
            delete cur;
            cur = next;
        }
    }

    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;

    std::optional<V> get(const K& key) override {
        auto it = map_.find(key);
        if (it == map_.end()) return std::nullopt;
        moveToFront(it->second);
        return it->second->value;
    }

    void put(const K& key, const V& value) override {
        auto it = map_.find(key);
        if (it != map_.end()) {
            it->second->value = value;
            moveToFront(it->second);
            return;
        }

        Node* node = new Node(key, value);
        map_[key] = node;
        pushFront(node);

        if (map_.size() > capacity_) {
            evictLRU();
        }
    }

    bool erase(const K& key) override {
        auto it = map_.find(key);
        if (it == map_.end()) return false;
        unlink(it->second);
        delete it->second;
        map_.erase(it);
        return true;
    }

    bool contains(const K& key) const override {
        return map_.find(key) != map_.end();
    }

    std::size_t size() const override { return map_.size(); }
    std::size_t capacity() const override { return capacity_; }

    std::optional<K> peekEvictionCandidate() const override {
        if (map_.empty()) return std::nullopt;
        return tail_->prev->key;
    }

private:
    void unlink(Node* node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    void pushFront(Node* node) {
        node->next = head_->next;
        node->prev = head_;
        head_->next->prev = node;
        head_->next = node;
    }

    void moveToFront(Node* node) {
        unlink(node);
        pushFront(node);
    }

    void evictLRU() {
        Node* victim = tail_->prev;
        if (victim == head_) return;
        unlink(victim);
        map_.erase(victim->key);
        delete victim;
    }

    std::size_t capacity_;
    Node* head_;
    Node* tail_;
    std::unordered_map<K, Node*> map_;
};
