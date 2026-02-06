// loadcell_485.cpp
#include "loadcell_485.hpp"

#include "ByteRingBuffer.hpp"
#include "SerialPort.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <chrono>

namespace {
constexpr uint8_t kHeader0 = 0x55;
constexpr uint8_t kHeader1 = 0xAB;
constexpr uint8_t kHeader2 = 0x01;

constexpr std::size_t kMinFrameBytes = 25;
constexpr std::size_t kMaxFrameBytes = 64;

constexpr std::size_t kRingBufferBytes = 2048;
constexpr std::size_t kOneReadBytes = 256;

constexpr std::size_t kOffsetLength = 3;

constexpr std::size_t kOffsetGross = 4;
constexpr std::size_t kOffsetRight = 8;
constexpr std::size_t kOffsetLeft = 12;

constexpr std::size_t kOffsetRightBattery = 16;
constexpr std::size_t kOffsetRightCharge = 17;
constexpr std::size_t kOffsetRightOnline = 18;

constexpr std::size_t kOffsetLeftBattery = 19;
constexpr std::size_t kOffsetLeftCharge = 20;
constexpr std::size_t kOffsetLeftOnline = 21;

constexpr std::size_t kOffsetGrossNet = 22;
constexpr std::size_t kOffsetOverload = 23;
constexpr std::size_t kOffsetTolerance = 24;

std::size_t ComputeTotalFrameBytes(uint8_t length_field) noexcept {
  // Length field 정의(주석): "The number of bytes of data following this length field"
  // => total = 4 + length
  const std::size_t candidate = 4 + static_cast<std::size_t>(length_field);
  if (candidate < kMinFrameBytes || candidate > kMaxFrameBytes) {
    return kMinFrameBytes;  // fallback: 고정 25 bytes
  }
  return candidate;
}

std::string ToHexString(const uint8_t* data, std::size_t len) {
  // "AA BB CC ..."
  std::string out;
  out.reserve(len * 3);
  char buf[4] = {};
  for (std::size_t i = 0; i < len; ++i) {
    std::snprintf(buf, sizeof(buf), "%02X", data[i]);
    out.append(buf);
    if (i + 1 < len) {
      out.push_back(' ');
    }
  }
  return out;
}
}  // namespace

LoadCell485Exception::LoadCell485Exception(int code, std::string message)
    : code_(code), message_(std::move(message)) {}

LoadCell485::LoadCell485()
    : serial_port_(std::make_unique<SerialPort>()),
      ring_buffer_(std::make_unique<ByteRingBuffer>(kRingBufferBytes)) {}

LoadCell485::~LoadCell485() = default;

bool LoadCell485::Open(const SerialConfig& cfg) { return serial_port_->Open(cfg); }
bool LoadCell485::Open() { return serial_port_->Open(); }

void LoadCell485::Close() noexcept { serial_port_->Close(); }
bool LoadCell485::IsOpen() const noexcept { return serial_port_->IsOpen(); }

const std::string& LoadCell485::LastError() const noexcept {
  return serial_port_->LastError();
}

void LoadCell485::SetDebugDumpEnabled(bool enabled) noexcept {
  debug_dump_enabled_ = enabled;
}

int LoadCell485::SendPoll() {
  // 요청-응답형일 때만 사용(스트리밍이면 불필요)
  constexpr std::array<uint8_t, 4> poll = {0x55, 0xAB, 0x01, 0x00};

  const long written = serial_port_->Write(poll.data(), poll.size());
  if (written < 0 || static_cast<std::size_t>(written) != poll.size()) {
    last_io_error_ = serial_port_->LastError();
    return ResultCode::kIoWriteFail;
  }
  return ResultCode::kOk;
}

int LoadCell485::RecvOnce(LoadCellStatus& out_status) {
  std::array<uint8_t, kOneReadBytes> temp{};
  const long read_bytes = serial_port_->Read(temp.data(), temp.size());
  if (read_bytes < 0) {
    last_io_error_ = serial_port_->LastError();
    return ResultCode::kIoReadFail;
  }

  if (read_bytes > 0) {
    ring_buffer_->Push(temp.data(), static_cast<std::size_t>(read_bytes));
  }

  return TryParseOneFrame_(out_status);
}

int LoadCell485::TryParseOneFrame_(LoadCellStatus& out_status) {
  while (ring_buffer_->Size() >= 3) {
    // 1) 헤더 탐색(55 AB 01)
    std::size_t start = 0;
    bool found = false;
    for (; start + 2 < ring_buffer_->Size(); ++start) {
      if (ring_buffer_->At(start) == kHeader0 &&
          ring_buffer_->At(start + 1) == kHeader1 &&
          ring_buffer_->At(start + 2) == kHeader2) {
        found = true;
        break;
      }
    }

    if (!found) {
      // 더 이상 헤더가 없으면 버퍼 비우고 다음 호출을 기다린다.
      ring_buffer_->DropFront(ring_buffer_->Size());
      return ResultCode::kNoFrame;
    }

    if (start > 0) {
      ring_buffer_->DropFront(start);
    }

    // 2) length 포함 최소 4바이트 필요
    if (ring_buffer_->Size() < 4) {
      return ResultCode::kNoFrame;
    }

    const uint8_t length_field = ring_buffer_->At(kOffsetLength);
    const std::size_t total = ComputeTotalFrameBytes(length_field);
    if (ring_buffer_->Size() < total) {
      return ResultCode::kNoFrame;
    }

    // 3) 프레임 복사(At 기반)
    std::vector<uint8_t> frame(total);
    for (std::size_t i = 0; i < total; ++i) {
      frame[i] = ring_buffer_->At(i);
    }

    // 4) 프레임 소비(정상/비정상 관계없이 1프레임 단위로 제거)
    ring_buffer_->DropFront(total);

    if (total < kMinFrameBytes) {
      return ResultCode::kFrameInvalid;
    }

    // 5) payload 디코드 + 스케일 적용(현재는 정수 디코드→double 캐스팅만)
    LoadCellStatus decoded{};
    ApplyScale_(frame.data(), frame.size(), decoded);

    // 6) 최소 sanity check(End marker/CRC가 없을 때 방어선)
    if (!SanityCheck_(decoded)) {
      DumpFrameThrottled_(frame.data(), frame.size(), "sanity_fail");
      throw LoadCell485Exception(ResultCode::kSanityFail,
                                 "Load cell frame sanity check failed");
    }

    // 7) 초기 검증용 프레임 덤프(옵션)
    if (debug_dump_enabled_) {
      DumpFrameThrottled_(frame.data(), frame.size(), "ok");
    }

    out_status = decoded;
    return ResultCode::kOk;
  }

  return ResultCode::kNoFrame;
}

void LoadCell485::ApplyScale_(const uint8_t* frame, std::size_t frame_len, LoadCellStatus& io_status) {
  (void)frame_len;

  // weight: 4바이트 big-endian 정수로 디코딩
  const int32_t gross = ReadS32BE_(frame + kOffsetGross);
  const int32_t right = ReadS32BE_(frame + kOffsetRight);
  const int32_t left = ReadS32BE_(frame + kOffsetLeft);

  // 현재: 스케일 미확정 → 단순 캐스팅
  io_status.gross_weight = static_cast<double>(gross);
  io_status.right_weight = static_cast<double>(right);
  io_status.left_weight = static_cast<double>(left);

  // 상태 필드
  io_status.right_battery_percent = frame[kOffsetRightBattery];
  io_status.right_charge_status = frame[kOffsetRightCharge];
  io_status.right_online_status = frame[kOffsetRightOnline];

  io_status.left_battery_percent = frame[kOffsetLeftBattery];
  io_status.left_charge_status = frame[kOffsetLeftCharge];
  io_status.left_online_status = frame[kOffsetLeftOnline];

  io_status.gross_net_mark = frame[kOffsetGrossNet];
  io_status.overload_mark = frame[kOffsetOverload];
  io_status.out_of_tolerance_mark = frame[kOffsetTolerance];

  // 문서에서 scale/offset 발견 시 여기만 수정:
  // io_status.gross_weight = static_cast<double>(gross) * 0.1;
  // io_status.gross_weight = (static_cast<double>(gross) - offset) * gain;
}

int32_t LoadCell485::ReadS32BE_(const uint8_t* p) noexcept {
  const uint32_t u =
      (static_cast<uint32_t>(p[0]) << 24) |
      (static_cast<uint32_t>(p[1]) << 16) |
      (static_cast<uint32_t>(p[2]) << 8) |
      static_cast<uint32_t>(p[3]);
  return static_cast<int32_t>(u);
}

bool LoadCell485::SanityCheck_(const LoadCellStatus& status) noexcept {
  if (status.left_battery_percent > 100 || status.right_battery_percent > 100) {
    return false;
  }
  if (status.left_charge_status > 1 || status.right_charge_status > 1) {
    return false;
  }
  if (status.left_online_status > 2 || status.right_online_status > 2) {
    return false;
  }
  if (status.gross_net_mark > 1) {
    return false;
  }
  if (status.overload_mark > 1) {
    return false;
  }
  if (status.out_of_tolerance_mark > 2) {
    return false;
  }
  return true;
}

void LoadCell485::DumpFrameThrottled_(const uint8_t* frame, std::size_t frame_len, const char* tag) {
  const std::uint64_t now_ms = NowEpochMs_();
  if (last_dump_epoch_ms_ == 0 || (now_ms - last_dump_epoch_ms_) >= 1000ULL) {
    if (dump_suppressed_ > 0) {
      std::printf("[loadcell_485][%s] frame_len=%zu (+%d suppressed)\n",
                  tag, frame_len, dump_suppressed_);
    } else {
      std::printf("[loadcell_485][%s] frame_len=%zu\n", tag, frame_len);
    }
    std::printf("[loadcell_485][%s] %s\n", tag, ToHexString(frame, frame_len).c_str());

    last_dump_epoch_ms_ = now_ms;
    dump_suppressed_ = 0;
  } else {
    ++dump_suppressed_;
  }
}

std::uint64_t LoadCell485::NowEpochMs_() noexcept {
  // 외부 라이브러리 의존 최소화: std::chrono 사용
  // (파일 전체에 chrono include가 없으므로 여기서만 include)
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}
