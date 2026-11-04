/**
 * priority_queue.h - Self-written binary heap priority queue.
 *
 * The comparator has the SAME meaning as in sorting.h: comp(a, b) returns
 * true when `a` should come BEFORE `b` (a has higher priority). top() always
 * returns the element that comes first.
 *
 * Stored in a vector as an implicit complete binary tree:
 *   parent(i) = (i-1)/2, children(i) = 2i+1 and 2i+2.
 *
 * push: O(log n)   pop: O(log n)   top: O(1)
 */
#ifndef SKILL_ISSUE_PRIORITY_QUEUE_H
#define SKILL_ISSUE_PRIORITY_QUEUE_H

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename T, typename Compare>
class PriorityQueue {
public:
    explicit PriorityQueue(Compare comp) : comp_(comp) {}

    bool empty() const { return data_.empty(); }
    std::size_t size() const { return data_.size(); }

    /** Highest-priority element. Throws if the queue is empty. */
    const T& top() const {
        if (data_.empty()) throw std::out_of_range("PriorityQueue::top on empty queue");
        return data_.front();
    }

    /** Adds an element and restores the heap order by sifting it up. */
    void push(const T& value) {
        data_.push_back(value);
        siftUp(data_.size() - 1);
    }

    /** Removes the highest-priority element. Throws if empty. */
    void pop() {
        if (data_.empty()) throw std::out_of_range("PriorityQueue::pop on empty queue");
        data_.front() = data_.back();   // move last leaf to the root
        data_.pop_back();
        if (!data_.empty()) siftDown(0);
    }

private:
    std::vector<T> data_;
    Compare comp_;

    // Move node i up while it has higher priority than its parent.
    void siftUp(std::size_t i) {
        while (i > 0) {
            std::size_t parent = (i - 1) / 2;
            if (!comp_(data_[i], data_[parent])) break;
            std::swap(data_[i], data_[parent]);
            i = parent;
        }
    }

    // Move node i down while a child has higher priority than it.
    void siftDown(std::size_t i) {
        const std::size_t n = data_.size();
        while (true) {
            std::size_t best = i, l = 2 * i + 1, r = 2 * i + 2;
            if (l < n && comp_(data_[l], data_[best])) best = l;
            if (r < n && comp_(data_[r], data_[best])) best = r;
            if (best == i) break;
            std::swap(data_[i], data_[best]);
            i = best;
        }
    }
};

#endif // SKILL_ISSUE_PRIORITY_QUEUE_H
