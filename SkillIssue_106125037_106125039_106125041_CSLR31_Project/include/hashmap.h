/**
 * hashmap.h - Self-written hash map (separate chaining, automatic resize).
 *
 * Maps a key to a value in O(1) average time. Used by Platform as an index:
 *   user ID (string)  -> position in users_
 *   request ID (int)  -> position in requests_
 *
 * How it works:
 *   - An array of buckets; each bucket is a singly linked list of nodes.
 *   - hashKey(key) % bucketCount chooses the bucket.
 *   - When size / bucketCount exceeds 0.75 the table doubles and every node
 *     is re-inserted (rehash), which keeps chains short.
 */
#ifndef SKILL_ISSUE_HASHMAP_H
#define SKILL_ISSUE_HASHMAP_H

#include <cstddef>
#include <string>

// djb2 hash for strings: hash = hash * 33 + c
inline std::size_t hashKey(const std::string& key) {
    std::size_t h = 5381;
    for (unsigned char c : key) h = h * 33 + c;
    return h;
}

// Simple hash for integer keys.
inline std::size_t hashKey(int key) {
    return static_cast<std::size_t>(key) * 2654435761u;  // Knuth multiplicative
}

template <typename K, typename V>
class HashMap {
public:
    explicit HashMap(std::size_t initialBuckets = 16)
        : bucketCount_(initialBuckets < 4 ? 4 : initialBuckets), size_(0) {
        buckets_ = new Node*[bucketCount_]();   // all nullptr
    }

    ~HashMap() { destroy(); }

    // Copying a map would need a deep copy; Platform never copies it.
    HashMap(const HashMap&) = delete;
    HashMap& operator=(const HashMap&) = delete;

    /** Inserts key->value, or overwrites the value if the key exists. */
    void put(const K& key, const V& value) {
        Node* node = findNode(key);
        if (node) { node->value = value; return; }

        if (size_ + 1 > bucketCount_ * 3 / 4) resize(bucketCount_ * 2);

        std::size_t b = hashKey(key) % bucketCount_;
        buckets_[b] = new Node{key, value, buckets_[b]};  // insert at chain head
        ++size_;
    }

    /** Copies the value into `out` and returns true if the key exists. */
    bool get(const K& key, V& out) const {
        Node* node = findNode(key);
        if (!node) return false;
        out = node->value;
        return true;
    }

    bool contains(const K& key) const { return findNode(key) != nullptr; }

    /** Removes the key; returns false if it was not present. */
    bool remove(const K& key) {
        std::size_t b = hashKey(key) % bucketCount_;
        Node* prev = nullptr;
        for (Node* cur = buckets_[b]; cur; prev = cur, cur = cur->next) {
            if (cur->key == key) {
                if (prev) prev->next = cur->next; else buckets_[b] = cur->next;
                delete cur;
                --size_;
                return true;
            }
        }
        return false;
    }

    void clear() {
        for (std::size_t i = 0; i < bucketCount_; ++i) {
            Node* cur = buckets_[i];
            while (cur) { Node* next = cur->next; delete cur; cur = next; }
            buckets_[i] = nullptr;
        }
        size_ = 0;
    }

    std::size_t size() const { return size_; }
    std::size_t bucketCount() const { return bucketCount_; }

private:
    struct Node {
        K key;
        V value;
        Node* next;
    };

    Node** buckets_;
    std::size_t bucketCount_;
    std::size_t size_;

    Node* findNode(const K& key) const {
        for (Node* cur = buckets_[hashKey(key) % bucketCount_]; cur; cur = cur->next)
            if (cur->key == key) return cur;
        return nullptr;
    }

    // Allocates a bigger bucket array and re-inserts every node (rehash).
    void resize(std::size_t newCount) {
        Node** fresh = new Node*[newCount]();
        for (std::size_t i = 0; i < bucketCount_; ++i) {
            Node* cur = buckets_[i];
            while (cur) {
                Node* next = cur->next;
                std::size_t b = hashKey(cur->key) % newCount;
                cur->next = fresh[b];
                fresh[b] = cur;
                cur = next;
            }
        }
        delete[] buckets_;
        buckets_ = fresh;
        bucketCount_ = newCount;
    }

    void destroy() {
        clear();
        delete[] buckets_;
        buckets_ = nullptr;
    }
};

#endif // SKILL_ISSUE_HASHMAP_H
