#ifndef BYTE_RING_BUFFER_HPP_
#define BYTE_RING_BUFFER_HPP_

#include <cstddef>
#include <cstdint>
#include <vector>

/**
 * @brief 고정 크기 바이트 링버퍼.
 *
 * - newest 우선 저장
 * - 용량 초과 시 oldest 데이터 자동 삭제
 * - 스트림 기반 시리얼 수신 데이터 누적용
 */
class ByteRingBuffer {
public:
    /**
     * @brief 링버퍼 생성자.
     * @param capacity_bytes 버퍼 용량(Byte 단위)
     */
    explicit ByteRingBuffer(std::size_t capacity_bytes);

    ByteRingBuffer(const ByteRingBuffer&) = delete;
    ByteRingBuffer& operator=(const ByteRingBuffer&) = delete;

    ByteRingBuffer(ByteRingBuffer&&) noexcept = default;
    ByteRingBuffer& operator=(ByteRingBuffer&&) noexcept = default;

    ~ByteRingBuffer() = default;

    /**
     * @brief 바이트 데이터를 링버퍼에 추가한다.
     * @param data 입력 데이터 포인터
     * @param size 입력 데이터 크기
     */
    void Push(const uint8_t* data, std::size_t size);

    /**
     * @brief 현재 저장된 바이트 수.
     * @return 저장된 바이트 수
     */
    std::size_t Size() const noexcept { return size_; }

    /**
     * @brief 버퍼 용량(Byte).
     * @return 용량
     */
    std::size_t Capacity() const noexcept { return buffer_.size(); }

    /**
     * @brief 앞쪽(oldest) 데이터 제거.
     * @param count 제거할 바이트 수
     */
    void DropFront(std::size_t count);

    /**
     * @brief 논리 인덱스 기준 바이트 조회.
     * @param index 0 = 가장 오래된 바이트
     * @return 바이트 값
     */
    uint8_t At(std::size_t index) const;

private:
    std::vector<uint8_t> buffer_;
    std::size_t head_ = 0;
    std::size_t size_ = 0;
};

#endif
