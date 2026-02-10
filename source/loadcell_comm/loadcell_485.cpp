#include "loadcell_485.h"
#include "ByteRingBuffer.h"
#include "SerialPort.h"
#include "loadcell_status.h"
#include <array>
#include <cstddef>
#include <vector>

namespace {
constexpr std::size_t kRingBufferBytes = 2048;
constexpr std::size_t kOneReadBytes = 256;

constexpr uint8_t kHeader0 = 0x55;
constexpr uint8_t kHeader1 = 0xAB;
constexpr uint8_t kHeader2 = 0x01;

constexpr std::size_t kMinFrameBytes = 25;

constexpr std::size_t kHeader0Pos = 0;
constexpr std::size_t kHeader1Pos = 1;
constexpr std::size_t kHeader2Pos = 2;

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

int32_t Read32BE_(const uint8_t* data) {
  const uint32_t u =
      (static_cast<uint32_t>(data[0]) << 24) |
      (static_cast<uint32_t>(data[1]) << 16) |
      (static_cast<uint32_t>(data[2]) << 8) |
      static_cast<uint32_t>(data[3]);
  return static_cast<int32_t>(u);
}

}  // namespace

namespace loadcell_comm {
LoadCell485::LoadCell485()
    : serial_port_(std::make_unique<SerialPort>()),
      ring_buffer_(std::make_unique<ByteRingBuffer>(kRingBufferBytes)) {}

LoadCell485::~LoadCell485() = default;

bool LoadCell485::Open(const SerialConfig &cfg) {
  bool flag = serial_port_->Open(cfg);
  if (!flag)
    last_error_ = serial_port_->LastError();

  return flag;
}

bool LoadCell485::Open() {
  bool flag = serial_port_->Open();
  if (!flag)
    last_error_ = serial_port_->LastError();

  return flag;
}

void LoadCell485::Close() noexcept { serial_port_->Close(); }

bool LoadCell485::IsOpen() const noexcept { return serial_port_->IsOpen(); }

ResultCode LoadCell485::RecvOnce(LoadCellStatus &out_status) {
  std::array<uint8_t, kOneReadBytes> temp{};
  const long read_bytes = serial_port_->Read(temp.data(), temp.size());
  if (read_bytes < 0) {
    last_error_ = serial_port_->LastError();
    return ResultCode::kIoReadFail;
  }

  if (read_bytes > 0)
    ring_buffer_->Push(temp.data(), static_cast<std::size_t>(read_bytes));

  return TryParseOneFrame_(out_status);
}

const std::string &LoadCell485::GetLastError() const noexcept {
  return last_error_;
}

ResultCode LoadCell485::TryParseOneFrame_(LoadCellStatus &out_status) {
  const std::size_t buffer_size = ring_buffer_->Size();
  if(buffer_size < kMinFrameBytes) {
    SetLastError("Not enough data in buffer: (" + std::to_string(ring_buffer_->Size()) + " bytes)");
    return ResultCode::kNoFrame;
  }

  // 임시 버퍼에 복사
  std::vector<uint8_t> temp_buffer;
  ring_buffer_->CopyFront(buffer_size, temp_buffer);

  // 버퍼에서 헤더 영역 검색
  const std::size_t headers_found = buffer_size - kMinFrameBytes + 1;
  bool found = false;
  std::size_t first_header_index = 0;
  for (; first_header_index < headers_found; ++first_header_index) {
    if(temp_buffer[first_header_index + kHeader0Pos] == kHeader0 &&
       temp_buffer[first_header_index + kHeader1Pos] == kHeader1 &&
       temp_buffer[first_header_index + kHeader2Pos] == kHeader2) {
      found = true;
      break;
    }
  }

  // 헤더 탐색 실패 시 헤더 탐색 영역만큼 버퍼 비우기
  if (false == found) {
    SetLastError("No valid header found in buffer");
    ring_buffer_->DropFront(headers_found);
    return ResultCode::kNoFrame;
  }

  // 데이터 파싱
  std::array<uint8_t, kMinFrameBytes> frames{};
  std::copy(temp_buffer.begin() + first_header_index,
            temp_buffer.begin() + first_header_index + kMinFrameBytes,
            frames.begin());
  ApplyScale(frames, out_status);

  // 프레임을 버퍼에서 제거
  ring_buffer_->DropFront(first_header_index + kMinFrameBytes);

  return ResultCode::kOk;
}

void LoadCell485::ApplyScale(const std::array<uint8_t, kMinFrameBytes>& frame, LoadCellStatus &status) noexcept {
  // weight: 4바이트 big-endian 정수로 디코딩
  const int32_t gross = Read32BE_(frame.data() + kOffsetGross);
  const int32_t right = Read32BE_(frame.data() + kOffsetRight);
  const int32_t left = Read32BE_(frame.data() + kOffsetLeft);

  // 현재: 스케일 미확정 → 단순 캐스팅
  status.gross_weight = static_cast<double>(gross);
  status.right_weight = static_cast<double>(right);
  status.left_weight = static_cast<double>(left);

  // 상태 필드
  status.right_battery_percent = frame[kOffsetRightBattery];
  status.right_charge_status = frame[kOffsetRightCharge];
  status.right_online_status = frame[kOffsetRightOnline];

  status.left_battery_percent = frame[kOffsetLeftBattery];
  status.left_charge_status = frame[kOffsetLeftCharge];
  status.left_online_status = frame[kOffsetLeftOnline];

  status.gross_net_mark = frame[kOffsetGrossNet];
  status.overload_mark = frame[kOffsetOverload];
  status.out_of_tolerance_mark = frame[kOffsetTolerance];

  // 문서에서 scale/offset 발견 시 여기만 수정:
  // status.gross_weight = static_cast<double>(gross) * 0.1;
  // status.gross_weight = (static_cast<double>(gross) - offset) * gain;
}

void LoadCell485::SetLastError(std::string msg) noexcept {
  last_error_ = std::move(msg);
}

} // namespace loadcell_comm