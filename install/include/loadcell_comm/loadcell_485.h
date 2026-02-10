#ifndef LOADCELL_485_H_
#define LOADCELL_485_H_

#include "loadcell_status.h"
#include <string>
#include <memory>

class SerialConfig;
class SerialPort;
class ByteRingBuffer;

namespace loadcell_comm {
class LoadCell485 {
public:
  LoadCell485();
  ~LoadCell485();

    LoadCell485(const LoadCell485 &) = delete;
    LoadCell485 &operator=(const LoadCell485 &) = delete;

    bool Open(const SerialConfig &cfg);
    bool Open();
    void Close() noexcept;
    bool IsOpen() const noexcept;

    ResultCode RecvOnce(LoadCellStatus &out_status);

    const std::string &GetLastError() const noexcept;

private:
    ResultCode TryParseOneFrame_(LoadCellStatus &out_status);
    void ApplyScale(const std::array<uint8_t, 25>& frame, LoadCellStatus &status) noexcept;

    void SetLastError(std::string msg) noexcept;

private:
    std::unique_ptr<class SerialPort> serial_port_;
    std::unique_ptr<class ByteRingBuffer> ring_buffer_;
    std::string last_error_;
};
}

#endif // LOADCELL_485_H_