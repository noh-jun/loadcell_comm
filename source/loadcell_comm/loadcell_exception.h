#ifndef LOADCELL_EXCEPTION_H_
#define LOADCELL_EXCEPTION_H_

#include <exception>
#include <string>
#include "loadcell_status.h"

namespace loadcell_comm {
class LoadCell485Exception final : public std::exception {
public:
  LoadCell485Exception(ResultCode code, std::string message);

  int Code() const noexcept;
  const char *what() const noexcept override;

private:
  std::string code_to_string(ResultCode code) const noexcept;

private:
  ResultCode code_;
  std::string message_;
};
}

#endif // LOADCELL_EXCEPTION_H_