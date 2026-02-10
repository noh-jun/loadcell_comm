#include "loadcell_exception.h"
#include "loadcell_status.h"

namespace loadcell_comm {
LoadCell485Exception::LoadCell485Exception(ResultCode code, std::string message)
    : code_(code) {
  message_ = "[" + code_to_string(code_) + "]: " + message;
}

int LoadCell485Exception::Code() const noexcept {
  return static_cast<int>(code_);
}

const char *LoadCell485Exception::what() const noexcept {
  return message_.c_str();
}

std::string
LoadCell485Exception::code_to_string(ResultCode code) const noexcept {
  switch (code) {
  case ResultCode::kOk:
    return "Ok";
  case ResultCode::kFrameTooShort:
    return "Frame Too Short";
  case ResultCode::kNoFrame:
    return "No Frame";
  case ResultCode::kIoReadFail:
    return "IO Read Fail";
  default:
    return "Unknown Error: " + std::to_string(static_cast<int>(code));
  }
}

} // namespace loadcell_comm