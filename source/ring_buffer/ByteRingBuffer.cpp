#include "ByteRingBuffer.h"

#include <algorithm>
#include <stdexcept>

ByteRingBuffer::ByteRingBuffer(std::size_t capacity_bytes)
    : buffer_(capacity_bytes, 0) {
    if (capacity_bytes == 0) {
        throw std::invalid_argument("ByteRingBuffer 용량은 0보다 커야 합니다.");
    }
}

void ByteRingBuffer::Push(const uint8_t* data, std::size_t size) {
    if (!data || size == 0) {
        return;
    }

    if (size >= buffer_.size()) {
        data += (size - buffer_.size());
        size = buffer_.size();
        head_ = 0;
        size_ = 0;
    }

    const std::size_t free_space = buffer_.size() - size_;
    if (size > free_space) {
        DropFront(size - free_space);
    }

    std::size_t tail = (head_ + size_) % buffer_.size();

    const std::size_t first = std::min(size, buffer_.size() - tail);
    std::copy(data, data + first, buffer_.begin() + static_cast<long>(tail));

    const std::size_t remain = size - first;
    if (remain > 0) {
        std::copy(data + first, data + size, buffer_.begin());
    }

    size_ += size;
}

void ByteRingBuffer::DropFront(std::size_t count) {
    if (count >= size_) {
        head_ = 0;
        size_ = 0;
        return;
    }

    head_ = (head_ + count) % buffer_.size();
    size_ -= count;
}

uint8_t ByteRingBuffer::At(std::size_t index) const {
    if (index >= size_) {
        throw std::out_of_range("ByteRingBuffer::At 범위 초과");
    }
    return buffer_[(head_ + index) % buffer_.size()];
}

std::size_t ByteRingBuffer::CopyFront(std::size_t size, std::vector<uint8_t>& out) const {
    const std::size_t copy_size = std::min(size, size_);
    out.resize(copy_size);

    if (copy_size == 0) {
        return 0;
    }

    const std::size_t first = std::min(copy_size, buffer_.size() - head_);
    std::copy(buffer_.begin() + static_cast<long>(head_),
              buffer_.begin() + static_cast<long>(head_ + first),
              out.begin());

    const std::size_t remain = copy_size - first;
    if (remain > 0) {
        std::copy(buffer_.begin(),
                  buffer_.begin() + static_cast<long>(remain),
                  out.begin() + static_cast<long>(first));
    }

    return copy_size;
}
