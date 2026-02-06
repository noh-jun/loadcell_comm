// loadcell_485.hpp
#ifndef LOADCELL_485_HPP_
#define LOADCELL_485_HPP_

#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>

class ByteRingBuffer;
class SerialPort;
struct SerialConfig;

struct LoadCellStatus {
  double gross_weight = 0.0;
  double right_weight = 0.0;
  double left_weight = 0.0;

  uint8_t right_battery_percent = 0;
  uint8_t right_charge_status = 0; // 0: uncharged, 1: charging
  uint8_t right_online_status = 0; // 0: online, 1: offline, 2: hw failure

  uint8_t left_battery_percent = 0;
  uint8_t left_charge_status = 0; // 0: uncharged, 1: charging
  uint8_t left_online_status = 0; // 0: online, 1: offline, 2: hw failure

  uint8_t gross_net_mark = 0;        // 0: gross, 1: net
  uint8_t overload_mark = 0;         // 0: not overloaded, 1: overload
  uint8_t out_of_tolerance_mark = 0; // 0: ok, 1: left, 2: right
};

class LoadCell485Exception final : public std::exception {
public:
  LoadCell485Exception(int code, std::string message);

  int Code() const noexcept { return code_; }
  const char *what() const noexcept override { return message_.c_str(); }

private:
  int code_;
  std::string message_;
};

class LoadCell485 final {
public:
  struct ResultCode {
    static constexpr int kOk = 0;
    static constexpr int kNoFrame = 1;
    static constexpr int kIoReadFail = 2;
    static constexpr int kIoWriteFail = 3;
    static constexpr int kFrameInvalid = 4;
    static constexpr int kSanityFail = 5;
  };

  LoadCell485();
  ~LoadCell485();

  LoadCell485(const LoadCell485 &) = delete;
  LoadCell485 &operator=(const LoadCell485 &) = delete;

  bool Open(const SerialConfig &cfg);
  bool Open();
  void Close() noexcept;
  bool IsOpen() const noexcept;

  const std::string &LastError() const noexcept;
  const std::string &LastIoError() const noexcept { return last_io_error_; }

  // 초기 검증/디버깅 용도: 프레임 덤프(스로틀링 적용)
  void SetDebugDumpEnabled(bool enabled) noexcept;

  // (선택) 요청-응답형인 경우 폴링 프레임 전송
  int SendPoll();

  // non-blocking: 1회 read + 1프레임 파싱 시도
  int RecvOnce(LoadCellStatus &out_status);

private:
  int TryParseOneFrame_(LoadCellStatus &out_status);

  static void ApplyScale_(const uint8_t *frame, std::size_t frame_len,
                          LoadCellStatus &io_status);

  static int32_t ReadS32BE_(const uint8_t *p) noexcept;

  static bool SanityCheck_(const LoadCellStatus &status) noexcept;

  void DumpFrameThrottled_(const uint8_t *frame, std::size_t frame_len,
                           const char *tag);

  static std::uint64_t NowEpochMs_() noexcept;

private:
  std::unique_ptr<SerialPort> serial_port_;
  std::unique_ptr<ByteRingBuffer> ring_buffer_;
  std::string last_io_error_;

  bool debug_dump_enabled_ = false;

  // 로그 스로틀링 상태(1초 간격)
  std::uint64_t last_dump_epoch_ms_ = 0;
  int dump_suppressed_ = 0;
};

#endif // LOADCELL_485_HPP_
