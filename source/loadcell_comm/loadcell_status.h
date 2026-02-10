#ifndef LOADCELL_STATUS_H_
#define LOADCELL_STATUS_H_

#include <cstdint>

namespace loadcell_comm {
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

enum class ResultCode {
  kOk = 0,
  kNoFrame = 1,
  kIoReadFail = 2,
  kIoWriteFail = 3,
  kFrameInvalid = 4,
  kSanityFail = 5
};
} // namespace loadcell_comm

#endif // LOADCELL_STATUS_H_