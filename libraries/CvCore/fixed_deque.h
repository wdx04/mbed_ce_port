#pragma once

#include <stddef.h>

template <typename T, size_t Capacity>
class fixed_deque {
public:
    fixed_deque() : size_(0), front_(0), rear_(0), default_value_()
    {}

    void push_back(const T& value) {
        if (size_ == Capacity) {
            // Automatically dequeue the front element
            pop_front();
        }
        data_[rear_] = value;
        rear_ = (rear_ + 1) % Capacity;
        size_++;
    }

    void pop_back() {
        if (size_ == 0) {
            return;
        }
        rear_ = (rear_ - 1 + Capacity) % Capacity;
        size_--;
    }

    void push_front(const T& value) {
        if (size_ == Capacity) {
            // Automatically dequeue the rear element
            pop_back();
        }
        front_ = (front_ - 1 + Capacity) % Capacity;
        data_[front_] = value;
        size_++;
    }

    void pop_front() {
        if (size_ == 0) {
            return;
        }
        front_ = (front_ + 1) % Capacity;
        size_--;
    }

    T& front() {
        if (size_ == 0) {
            // Return default value of type T when queue is empty
            return default_value_;
        }
        return data_[front_];
    }

    T& back() {
        if (size_ == 0) {
            // Return default value of type T when queue is empty
            return default_value_;
        }
        return data_[(rear_ - 1 + Capacity) % Capacity];
    }

    void clear() {
        size_ = 0;
        front_ = 0;
        rear_ = 0;
    }

    size_t size() const {
        return size_;
    }

    bool empty() const {
        return size_ == 0;
    }

private:
    T data_[Capacity];
    size_t size_;
    size_t front_;
    size_t rear_;
    T default_value_; // Default value for type T
};
