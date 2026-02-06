#pragma once
#include "SerialConfig.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

class SerialPort {
public:
    SerialPort() = default;
    explicit SerialPort(SerialConfig cfg) : config_(std::move(cfg)) {}

    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    SerialPort(SerialPort&&) noexcept;
    SerialPort& operator=(SerialPort&&) noexcept;

    virtual ~SerialPort(); // RAII: Close()

    bool Open(const SerialConfig& cfg);
    bool Open();

    void Close() noexcept;
    bool IsOpen() const noexcept { return fd_ >= 0; }

    long Read(uint8_t* buf, std::size_t len) noexcept;
    long Write(const uint8_t* buf, std::size_t len) noexcept;

    const std::optional<SerialConfig>& Config() const noexcept { return config_; }
    const std::string& LastError() const noexcept { return last_error_; }

protected:
    int Fd() const noexcept { return fd_; }
    void SetLastError(std::string msg) noexcept { last_error_ = std::move(msg); }

private:
    int fd_ = -1;
    std::optional<SerialConfig> config_;
    std::string last_error_;

    bool ConfigureTermios_(int fd, const SerialConfig& cfg) noexcept;
};


