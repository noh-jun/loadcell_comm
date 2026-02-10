#pragma once
#include <cstdint>
#include <string>

struct SerialConfig {
    std::string device;        // e.g. "/dev/ttyUSB0"
    int baudrate = 19200;      // e.g. 9600/19200/115200...
    uint8_t data_bits = 8;     // 5..8
    char parity = 'N';         // 'N','E','O'
    uint8_t stop_bits = 1;     // 1 or 2
    bool rtscts = false;       // HW flow control
    bool xonxoff = false;      // SW flow control

    // read behavior
    // VMIN/VTIME 기반: "최소 n바이트 or 타임아웃" 스타일
    uint8_t vmin = 0;          // 0이면 non-blocking-ish (VTIME에 의존)
    uint8_t vtime_ds = 1;      // deciseconds (0.1s 단위) 0..255
};
