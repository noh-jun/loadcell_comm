#include "SerialPort.hpp"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

static std::string SysErr(const char *where) {
  return std::string(where) + ": " + std::strerror(errno);
}

SerialPort::SerialPort(SerialPort &&o) noexcept {
  fd_ = o.fd_;
  o.fd_ = -1;
  config_ = std::move(o.config_);
  last_error_ = std::move(o.last_error_);
}

SerialPort &SerialPort::operator=(SerialPort &&o) noexcept {
  if (this == &o)
    return *this;

  Close();

  fd_ = o.fd_;
  o.fd_ = -1;
  config_ = std::move(o.config_);
  last_error_ = std::move(o.last_error_);

  return *this;
}

SerialPort::~SerialPort() { Close(); }

bool SerialPort::Open(const SerialConfig &cfg) {
  config_ = cfg; // 요구사항: 멤버 저장
  return Open(); // 요구사항: no-arg Open이 멤버 cfg 사용
}

bool SerialPort::Open() {
  if (!config_) {
    SetLastError("Open(): SerialConfig is not set");
    return false;
  }

  if (IsOpen())
    Close();

  int fd = ::open(config_->device.c_str(), O_RDWR | O_NOCTTY | O_CLOEXEC);
  if (fd < 0) {
    SetLastError(SysErr("open"));
    return false;
  }

  if (!ConfigureTermios_(fd, *config_)) {
    ::close(fd);
    return false;
  }

  fd_ = fd;
  return true;
}

void SerialPort::Close() noexcept {
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

long SerialPort::Read(uint8_t *buf, std::size_t len) noexcept {
  if (!IsOpen()) {
    SetLastError("Read(): port is not open");
    return -1;
  }

  ssize_t r = ::read(fd_, buf, len);

  if (r < 0)
    SetLastError(SysErr("read"));

  return (long)r;
}

long SerialPort::Write(const uint8_t *buf, std::size_t len) noexcept {
  if (!IsOpen()) {
    SetLastError("Write(): port is not open");
    return -1;
  }

  ssize_t w = ::write(fd_, buf, len);

  if (w < 0)
    SetLastError(SysErr("write"));

  return (long)w;
}

bool SerialPort::ConfigureTermios_(int fd, const SerialConfig &cfg) noexcept {
  termios tio{};
  if (::tcgetattr(fd, &tio) != 0) {
    SetLastError(SysErr("tcgetattr"));
    return false;
  }

  ::cfmakeraw(&tio);

  // baud (예: 19200만 쓸거면 더 단순화 가능)
  speed_t sp{};
  switch (cfg.baudrate) {
  case 9600:
    sp = B9600;
    break;

  case 19200:
    sp = B19200;
    break;

  case 115200:
    sp = B115200;
    break;

  default:
    SetLastError("Unsupported baudrate");
    return false;
  }

  if (cfsetispeed(&tio, sp) != 0 || cfsetospeed(&tio, sp) != 0) {
    SetLastError(SysErr("cfsetispeed/cfsetospeed"));
    return false;
  }

  // 8N1 기본
  tio.c_cflag |= (CLOCAL | CREAD);
  tio.c_cflag &= ~CSIZE;
  tio.c_cflag |= CS8;
  tio.c_cflag &= ~(PARENB | PARODD);
  tio.c_cflag &= ~CSTOPB;

  // VMIN/VTIME
  tio.c_cc[VMIN] = cfg.vmin;
  tio.c_cc[VTIME] = cfg.vtime_ds;

  if (::tcsetattr(fd, TCSANOW, &tio) != 0) {
    SetLastError(SysErr("tcsetattr"));
    return false;
  }
  
  ::tcflush(fd, TCIFLUSH);
  return true;
}
